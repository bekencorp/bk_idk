// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <stdbool.h>
#include <driver/flash.h>
#include "flash_import_internal.h"

typedef enum {
    FLASH_IMPORT_PD_MODE_1TO1 = 0,
    FLASH_IMPORT_PD_MODE_0TO0,
    FLASH_IMPORT_PD_MODE_0TO1,
    FLASH_IMPORT_PD_MODE_CNT
} flash_import_pd_mode_t;

#define FLASH_IMPORT_PD_LOG_INTERVAL          (100U)

#define FLASH_IMPORT_PD_TARGET_ADDR_1TO1      (0x400000U)
#define FLASH_IMPORT_PD_TARGET_ADDR_0TO0      (0x500000U)
#define FLASH_IMPORT_PD_TARGET_ADDR_0TO1      (0x600000U)

typedef struct {
    uint32_t               total_prog_cnt;
    uint32_t               first_err_prog_cnt;
    uint32_t               err_cnt;
    uint32_t               prev_err_cnt;
    uint32_t               target_err_cnt;
    uint32_t               next_err_cnt;
    uint32_t               curr_loop_err_cnt;
    uint32_t               curr_prev_err_cnt;
    uint32_t               curr_target_err_cnt;
    uint32_t               curr_next_err_cnt;
} flash_import_pd_stats_t;

typedef struct {
    beken_thread_t          task_handle;
    volatile bool           stop_requested;
    uint32_t                mode;
    uint32_t                target_addr;
    uint8_t                 *buf;
    flash_import_pd_stats_t stats;
    flash_import_context_t  saved_ctx;
    bool                    ctx_saved;
} flash_import_pd_ctx_t;

static flash_import_pd_ctx_t s_pd_ctx;

static uint32_t flash_import_pd_target_addr(uint32_t mode)
{
    switch (mode) {
    case FLASH_IMPORT_PD_MODE_1TO1:
        return FLASH_IMPORT_PD_TARGET_ADDR_1TO1;
    case FLASH_IMPORT_PD_MODE_0TO0:
        return FLASH_IMPORT_PD_TARGET_ADDR_0TO0;
    default:
        return FLASH_IMPORT_PD_TARGET_ADDR_0TO1;
    }
}

static uint8_t flash_import_pd_prog_pattern(uint32_t mode)
{
    return (mode == FLASH_IMPORT_PD_MODE_0TO0) ? 0x00U : 0xFFU;
}

static uint8_t flash_import_pd_expect_pattern(uint32_t mode)
{
    return (mode == FLASH_IMPORT_PD_MODE_1TO1) ? 0xFFU : 0x00U;
}

static bk_err_t flash_import_pd_prepare_layout(flash_import_pd_ctx_t *ctx)
{
    uint32_t target_addr = flash_import_pd_target_addr(ctx->mode);

    if ((target_addr & (FLASH_IMPORT_SECTOR_SIZE - 1U)) != 0U) {
        os_printf("program_disturb: target addr 0x%08x not sector aligned\r\n", target_addr);
        return BK_FAIL;
    }

    if (target_addr < FLASH_IMPORT_SECTOR_SIZE) {
        os_printf("program_disturb: target addr 0x%08x too low\r\n", target_addr);
        return BK_FAIL;
    }

    ctx->target_addr = target_addr;
    return BK_OK;
}

static bk_err_t flash_import_pd_buffers_init(flash_import_pd_ctx_t *ctx)
{
    ctx->buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
    if (ctx->buf == NULL) {
        os_printf("program_disturb: buf alloc fail\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

static void flash_import_pd_buffers_deinit(flash_import_pd_ctx_t *ctx)
{
    if (ctx->buf != NULL) {
        os_free(ctx->buf);
        ctx->buf = NULL;
    }
}

static bk_err_t flash_import_pd_init_context(flash_import_pd_ctx_t *ctx)
{
    bk_err_t ret = BK_OK;

    flash_import_context_save(&ctx->saved_ctx);
    ctx->ctx_saved = true;

    ret = flash_import_pd_buffers_init(ctx);
    if (ret != BK_OK) {
        flash_import_pd_buffers_deinit(ctx);
        return ret;
    }

    os_memset(&ctx->stats, 0, sizeof(ctx->stats));
    return BK_OK;
}

static void flash_import_pd_deinit_context(flash_import_pd_ctx_t *ctx)
{
    if (ctx->ctx_saved) {
        flash_import_context_restore(&ctx->saved_ctx);
        ctx->ctx_saved = false;
    }

    flash_import_pd_buffers_deinit(ctx);
    ctx->task_handle = NULL;
    rtos_delete_thread(NULL);
}

static bk_err_t flash_import_pd_do_erase_addr(uint32_t addr)
{
    flash_line_mode_t old_mode = FLASH_LINE_MODE_TWO;
    bk_err_t ret = BK_OK;

    old_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    ret = flash_import_erase_4k(addr);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static void flash_import_pd_do_write(flash_import_pd_ctx_t *ctx, uint32_t addr, uint32_t len)
{
    flash_line_mode_t old_mode = FLASH_LINE_MODE_TWO;

    old_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    (void)flash_import_write_bytes(ctx->buf, addr, len);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    (void)flash_import_set_line_mode(old_mode);
}

static void flash_import_pd_do_read(flash_import_pd_ctx_t *ctx, uint32_t addr, uint32_t len)
{
    flash_line_mode_t old_mode = FLASH_LINE_MODE_TWO;

    old_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    (void)flash_import_read_bytes(ctx->buf, addr, len);
    (void)flash_import_set_line_mode(old_mode);
}

static bk_err_t flash_import_pd_erase_all_sectors(flash_import_pd_ctx_t *ctx)
{
    if (flash_import_pd_do_erase_addr(ctx->target_addr - FLASH_IMPORT_SECTOR_SIZE) != BK_OK) {
        return BK_FAIL;
    }

    if (flash_import_pd_do_erase_addr(ctx->target_addr) != BK_OK) {
        return BK_FAIL;
    }

    if (flash_import_pd_do_erase_addr(ctx->target_addr + FLASH_IMPORT_SECTOR_SIZE) != BK_OK) {
        return BK_FAIL;
    }

    return BK_OK;
}

static void flash_import_pd_count_errors(flash_import_pd_ctx_t *ctx, uint8_t pattern,
                                         uint32_t sector_idx)
{
    uint32_t err_cnt = 0;

    for (uint32_t byte_idx = 0; byte_idx < FLASH_IMPORT_SECTOR_SIZE; byte_idx++) {
        if (ctx->buf[byte_idx] != pattern) {
            err_cnt++;
        }
    }

    ctx->stats.err_cnt += err_cnt;
    ctx->stats.curr_loop_err_cnt += err_cnt;
    if (sector_idx == 0U) {
        ctx->stats.prev_err_cnt += err_cnt;
        ctx->stats.curr_prev_err_cnt += err_cnt;
    } else if (sector_idx == 1U) {
        ctx->stats.target_err_cnt += err_cnt;
        ctx->stats.curr_target_err_cnt += err_cnt;
    } else {
        ctx->stats.next_err_cnt += err_cnt;
        ctx->stats.curr_next_err_cnt += err_cnt;
    }
}

static void flash_import_pd_verify_sector(flash_import_pd_ctx_t *ctx, uint32_t addr,
                                          uint8_t pattern, uint32_t sector_idx)
{
    if (ctx->stop_requested) {
        return;
    }

    flash_import_pd_do_read(ctx, addr, FLASH_IMPORT_SECTOR_SIZE);
    flash_import_pd_count_errors(ctx, pattern, sector_idx);
}

static void flash_import_pd_verify_all_sectors(flash_import_pd_ctx_t *ctx)
{
    uint8_t target_pattern = flash_import_pd_expect_pattern(ctx->mode);

    flash_import_pd_verify_sector(ctx, ctx->target_addr - FLASH_IMPORT_SECTOR_SIZE, 0xFFU, 0U);
    flash_import_pd_verify_sector(ctx, ctx->target_addr, target_pattern, 1U);
    flash_import_pd_verify_sector(ctx, ctx->target_addr + FLASH_IMPORT_SECTOR_SIZE, 0xFFU, 2U);
}

static void flash_import_pd_update_first_error(flash_import_pd_ctx_t *ctx)
{
    if (ctx->stats.first_err_prog_cnt != 0U) {
        return;
    }

    if (ctx->stats.curr_loop_err_cnt == 0U) {
        return;
    }

    ctx->stats.first_err_prog_cnt = ctx->stats.total_prog_cnt;
}

static void flash_import_pd_print_summary(flash_import_pd_ctx_t *ctx)
{
    os_printf("pd mode%u prog=%u first_err_prog=%u target=%u/%u prev=%u/%u next=%u/%u\r\n",
              ctx->mode,
              ctx->stats.total_prog_cnt,
              ctx->stats.first_err_prog_cnt,
              ctx->stats.curr_target_err_cnt, ctx->stats.target_err_cnt,
              ctx->stats.curr_prev_err_cnt, ctx->stats.prev_err_cnt,
              ctx->stats.curr_next_err_cnt, ctx->stats.next_err_cnt);
}

static void flash_import_pd_run(flash_import_pd_ctx_t *ctx)
{
    os_printf("pd mode%u: start prev=0x%08x target=0x%08x next=0x%08x\r\n",
              ctx->mode, ctx->target_addr - FLASH_IMPORT_SECTOR_SIZE,
              ctx->target_addr, ctx->target_addr + FLASH_IMPORT_SECTOR_SIZE);

    if (bk_flash_set_clk_freq(FLASH_CLK_FREQ_80M_HZ) != BK_OK) {
        os_printf("pd mode%u: set clk 80M fail\r\n", ctx->mode);
        return;
    }

    if (flash_import_pd_erase_all_sectors(ctx) != BK_OK) {
        os_printf("pd mode%u: initial erase fail target=0x%08x\r\n", ctx->mode, ctx->target_addr);
        return;
    }

    if (ctx->mode == FLASH_IMPORT_PD_MODE_0TO0 || ctx->mode == FLASH_IMPORT_PD_MODE_0TO1) {
        os_memset(ctx->buf, 0x00U, FLASH_IMPORT_SECTOR_SIZE);
        flash_import_pd_do_write(ctx, ctx->target_addr, FLASH_IMPORT_SECTOR_SIZE);
    }

    while (!ctx->stop_requested) {
        ctx->stats.total_prog_cnt++;
        ctx->stats.curr_loop_err_cnt = 0;
        ctx->stats.curr_prev_err_cnt = 0;
        ctx->stats.curr_target_err_cnt = 0;
        ctx->stats.curr_next_err_cnt = 0;

        os_memset(ctx->buf, flash_import_pd_prog_pattern(ctx->mode), FLASH_IMPORT_SECTOR_SIZE);
        flash_import_pd_do_write(ctx, ctx->target_addr, FLASH_IMPORT_SECTOR_SIZE);
        flash_import_pd_verify_all_sectors(ctx);
        flash_import_pd_update_first_error(ctx);

        if ((ctx->stats.total_prog_cnt % FLASH_IMPORT_PD_LOG_INTERVAL) == 0U) {
            flash_import_pd_print_summary(ctx);
        }
    }
}

static void flash_import_pd_thread(beken_thread_arg_t arg)
{
    flash_import_pd_ctx_t *ctx = (flash_import_pd_ctx_t *)arg;

    if (ctx == NULL) {
        rtos_delete_thread(NULL);
        return;
    }

    if (flash_import_pd_init_context(ctx) != BK_OK) {
        flash_import_pd_deinit_context(ctx);
        return;
    }

    flash_import_pd_run(ctx);
    flash_import_pd_deinit_context(ctx);
}

void flash_import_case_pd_start(uint32_t mode)
{
    flash_import_pd_ctx_t *ctx = &s_pd_ctx;
    bk_err_t ret = BK_OK;

    if (mode >= (uint32_t)FLASH_IMPORT_PD_MODE_CNT) {
        return;
    }

    if (ctx->task_handle != NULL) {
        os_printf("pd: busy, stop current test first\r\n");
        return;
    }

    ctx->mode = mode;
    if (flash_import_pd_prepare_layout(ctx) != BK_OK) {
        return;
    }

    ctx->stop_requested = false;
    ret = rtos_create_thread(&ctx->task_handle,
                             4,
                             "flash_import_pd",
                             (beken_thread_function_t)flash_import_pd_thread,
                             4096,
                             (beken_thread_arg_t)ctx);
    if (ret != BK_OK) {
        os_printf("pd mode%u: create thread fail:%d\r\n", mode, ret);
        ctx->task_handle = NULL;
        return;
    }
}

void flash_import_case_pd_stop(void)
{
    flash_import_pd_ctx_t *ctx = &s_pd_ctx;

    if (ctx->task_handle != NULL) {
        ctx->stop_requested = true;
        return;
    }

    os_printf("pd: not running\r\n");
}

void flash_import_case_pd_show(void)
{
    flash_import_pd_ctx_t *ctx = &s_pd_ctx;

    if (ctx->task_handle != NULL) {
        os_printf("pd mode%u: still running, stop first\r\n", ctx->mode);
        return;
    }

    os_printf("\r\nBuild_info:%s_%s\r\n", __DATE__, __TIME__);
    os_printf("flash id:%lx\r\n", bk_flash_get_id());
    os_printf("pd mode%u task: stopped\r\n", ctx->mode);
    os_printf("pd mode%u prev_addr: 0x%08x target_addr: 0x%08x next_addr: 0x%08x\r\n",
              ctx->mode, ctx->target_addr - FLASH_IMPORT_SECTOR_SIZE,
              ctx->target_addr, ctx->target_addr + FLASH_IMPORT_SECTOR_SIZE);
    os_printf("pd mode%u prog=%u first_err_prog=%u target=%u/%u prev=%u/%u next=%u/%u\r\n",
              ctx->mode,
              ctx->stats.total_prog_cnt,
              ctx->stats.first_err_prog_cnt,
              ctx->stats.curr_target_err_cnt, ctx->stats.target_err_cnt,
              ctx->stats.curr_prev_err_cnt, ctx->stats.prev_err_cnt,
              ctx->stats.curr_next_err_cnt, ctx->stats.next_err_cnt);
}
