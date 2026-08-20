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

#define FLASH_IMPORT_SE_TARGET_ADDR           (0x680000U)

#define FLASH_IMPORT_SE_SLOT_LEN              (4U)
#define FLASH_IMPORT_SE_WRITE_PAGE_NUM        (2U)
#define FLASH_IMPORT_SE_WRITE_SIZE            (FLASH_IMPORT_SE_WRITE_PAGE_NUM * FLASH_IMPORT_PAGE_SIZE)  // 2 * 256 = 512
#define FLASH_IMPORT_SE_WIN_SIZE              (FLASH_IMPORT_SECTOR_SIZE * 3U)  // 4096 * 3 = 12288 (12KB)

#define FLASH_IMPORT_SE_ROUND_LIMIT           (100000U)
#define FLASH_IMPORT_SE_READONLY_LOG_INTERVAL (50000U)

typedef struct {
    uint32_t               round_cnt;
    uint32_t               readonly_read_cnt;
    uint32_t               erase_vfy_byte_err;
    uint32_t               wr_page_byte_err;
    uint32_t               wr_non_page_byte_err;
    uint32_t               readonly_target_err;
    uint32_t               readonly_prev_next_err;
    uint32_t               readonly_target_err_max;
    uint32_t               readonly_prev_next_err_max;
} flash_import_se_stats_t;

typedef struct {
    beken_thread_t          task_handle;
    volatile bool           stop_requested;
    uint32_t                win_addr;
    uint8_t                 *buf;
    flash_import_se_stats_t stats;
    flash_import_context_t  saved_ctx;
    bool                    ctx_saved;
} flash_import_se_ctx_t;

static flash_import_se_ctx_t s_se_ctx;

static bk_err_t flash_import_se_prepare_layout(void)
{
    if ((FLASH_IMPORT_SE_TARGET_ADDR & (FLASH_IMPORT_SECTOR_SIZE - 1U)) != 0U) {
        os_printf("soft_erase: target addr 0x%08x not sector aligned\r\n",
                  FLASH_IMPORT_SE_TARGET_ADDR);
        return BK_FAIL;
    }

    if (FLASH_IMPORT_SE_TARGET_ADDR < FLASH_IMPORT_SECTOR_SIZE) {
        os_printf("soft_erase: target addr 0x%08x too low\r\n", FLASH_IMPORT_SE_TARGET_ADDR);
        return BK_FAIL;
    }

    s_se_ctx.win_addr = FLASH_IMPORT_SE_TARGET_ADDR - FLASH_IMPORT_SECTOR_SIZE;
    return BK_OK;
}

static bk_err_t flash_import_se_buffers_init(void)
{
    s_se_ctx.buf = (uint8_t *)os_malloc(FLASH_IMPORT_SE_WIN_SIZE);
    if (s_se_ctx.buf == NULL) {
        os_printf("soft_erase: buf alloc fail\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

static void flash_import_se_buffers_deinit(void)
{
    if (s_se_ctx.buf != NULL) {
        os_free(s_se_ctx.buf);
        s_se_ctx.buf = NULL;
    }
}

static bk_err_t flash_import_se_init_context(void)
{
    bk_err_t ret = BK_OK;

    flash_import_context_save(&s_se_ctx.saved_ctx);
    s_se_ctx.ctx_saved = true;

    ret = flash_import_se_buffers_init();
    if (ret != BK_OK) {
        return ret;
    }

    os_memset(&s_se_ctx.stats, 0, sizeof(s_se_ctx.stats));
    return BK_OK;
}

static bk_err_t flash_import_se_enter(void)
{
    bk_err_t ret = flash_import_se_init_context();

    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_se_prepare_layout();
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

static void flash_import_se_exit(void)
{
    flash_import_se_buffers_deinit();

    if (s_se_ctx.ctx_saved) {
        flash_import_context_restore(&s_se_ctx.saved_ctx);
        s_se_ctx.ctx_saved = false;
    }

    s_se_ctx.task_handle = NULL;
    rtos_delete_thread(NULL);
}

static void flash_import_soft_erase_read(uint32_t addr, uint32_t len)
{
    flash_line_mode_t old_mode = FLASH_LINE_MODE_TWO;

    old_mode = flash_import_set_line_mode(FLASH_LINE_MODE_FOUR);
    (void)flash_import_read_bytes(s_se_ctx.buf, addr, len);
    (void)flash_import_set_line_mode(old_mode);
}

static void flash_import_soft_erase_erase(uint32_t addr)
{
    flash_line_mode_t old_mode = FLASH_LINE_MODE_TWO;
    uint8_t *vfy_buf = s_se_ctx.buf + (addr - s_se_ctx.win_addr);

    old_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    (void)flash_import_erase_4k(addr);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    (void)flash_import_set_line_mode(old_mode);

    old_mode = flash_import_set_line_mode(FLASH_LINE_MODE_FOUR);
    (void)flash_import_read_bytes(vfy_buf, addr, FLASH_IMPORT_SECTOR_SIZE);
    (void)flash_import_set_line_mode(old_mode);

    for (uint32_t byte_idx = 0; byte_idx < FLASH_IMPORT_SECTOR_SIZE; byte_idx++) {
        if (vfy_buf[byte_idx] != 0xFFU) {
            s_se_ctx.stats.erase_vfy_byte_err++;
        }
    }
}

static void flash_import_soft_erase_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    flash_line_mode_t old_mode = FLASH_LINE_MODE_TWO;

    old_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    (void)flash_import_write_bytes(buf, addr, len);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    (void)flash_import_set_line_mode(old_mode);
}

static void flash_import_se_get_check_result(uint32_t written_len, uint32_t page_addr, bool read_only)
{
    if (read_only) {
        s_se_ctx.stats.readonly_target_err = 0;
        s_se_ctx.stats.readonly_prev_next_err = 0;
    }

    for (uint32_t byte_idx = 0; byte_idx < FLASH_IMPORT_SE_WIN_SIZE; byte_idx++) {
        uint32_t flash_addr = s_se_ctx.win_addr + byte_idx;
        uint8_t expect = 0xFFU;

        if ((byte_idx >= FLASH_IMPORT_SECTOR_SIZE)
            && (byte_idx < (FLASH_IMPORT_SECTOR_SIZE + written_len))) {
            expect = (uint8_t)((byte_idx - FLASH_IMPORT_SECTOR_SIZE) & 0xFFU);
        }

        if (s_se_ctx.buf[byte_idx] == expect) {
            continue;
        }

        if (read_only) {
            if (byte_idx < FLASH_IMPORT_SECTOR_SIZE) {
                s_se_ctx.stats.readonly_prev_next_err++;
            } else if (byte_idx < (FLASH_IMPORT_SECTOR_SIZE * 2U)) {
                s_se_ctx.stats.readonly_target_err++;
            } else {
                s_se_ctx.stats.readonly_prev_next_err++;
            }
            continue;
        }

        if ((flash_addr >= page_addr)
            && (flash_addr < (page_addr + FLASH_IMPORT_PAGE_SIZE))) {
            s_se_ctx.stats.wr_page_byte_err++;
        } else {
            s_se_ctx.stats.wr_non_page_byte_err++;
        }
    }

    if (read_only) {
        if (s_se_ctx.stats.readonly_target_err > s_se_ctx.stats.readonly_target_err_max) {
            s_se_ctx.stats.readonly_target_err_max = s_se_ctx.stats.readonly_target_err;
        }
        if (s_se_ctx.stats.readonly_prev_next_err > s_se_ctx.stats.readonly_prev_next_err_max) {
            s_se_ctx.stats.readonly_prev_next_err_max = s_se_ctx.stats.readonly_prev_next_err;
        }
    }
}

static void flash_import_se_program_round(void)
{
    uint8_t write_buf[FLASH_IMPORT_SE_SLOT_LEN];
    uint32_t write_addr = s_se_ctx.win_addr + FLASH_IMPORT_SECTOR_SIZE;

    for (uint32_t i = 0; i < FLASH_IMPORT_SE_WRITE_SIZE; i += FLASH_IMPORT_SE_SLOT_LEN) {
        uint32_t page_addr = write_addr + (i & ~(FLASH_IMPORT_PAGE_SIZE - 1U));

        write_buf[0] = (uint8_t)(i);
        write_buf[1] = (uint8_t)(i + 1U);
        write_buf[2] = (uint8_t)(i + 2U);
        write_buf[3] = (uint8_t)(i + 3U);

        flash_import_soft_erase_write(write_addr + i, write_buf, FLASH_IMPORT_SE_SLOT_LEN);
        s_se_ctx.stats.round_cnt++;

        flash_import_soft_erase_read(s_se_ctx.win_addr, FLASH_IMPORT_SE_WIN_SIZE);
        flash_import_se_get_check_result(i + FLASH_IMPORT_SE_SLOT_LEN, page_addr, false);
    }
}

static bk_err_t flash_import_se_write_loop(void)
{
    while (s_se_ctx.stats.round_cnt < FLASH_IMPORT_SE_ROUND_LIMIT) {
        flash_import_soft_erase_erase(s_se_ctx.win_addr + FLASH_IMPORT_SECTOR_SIZE);
        s_se_ctx.stats.round_cnt++;
        flash_import_se_program_round();

        os_printf("soft_erase run round=%u/%u page=0x%08x byte_err page=%u non_page=%u\r\n",
                  s_se_ctx.stats.round_cnt, FLASH_IMPORT_SE_ROUND_LIMIT,
                  s_se_ctx.win_addr + FLASH_IMPORT_SECTOR_SIZE,
                  s_se_ctx.stats.wr_page_byte_err, s_se_ctx.stats.wr_non_page_byte_err);

        if (s_se_ctx.stop_requested) {
            return BK_FAIL;
        }
    }

    return BK_OK;
}

static void flash_import_se_read_loop(void)
{
    while (!s_se_ctx.stop_requested) {
        flash_import_soft_erase_read(s_se_ctx.win_addr, FLASH_IMPORT_SE_WIN_SIZE);
        s_se_ctx.stats.readonly_read_cnt++;
        flash_import_se_get_check_result(FLASH_IMPORT_SE_WRITE_SIZE, 0U, true);

        if ((s_se_ctx.stats.readonly_read_cnt % FLASH_IMPORT_SE_READONLY_LOG_INTERVAL) == 0U) {
            os_printf("soft_erase ro read=%u target_err=%u prev_next_err=%u target_max=%u prev_next_max=%u\r\n",
                      s_se_ctx.stats.readonly_read_cnt,
                      s_se_ctx.stats.readonly_target_err,
                      s_se_ctx.stats.readonly_prev_next_err,
                      s_se_ctx.stats.readonly_target_err_max,
                      s_se_ctx.stats.readonly_prev_next_err_max);
        }
    }
}

static void flash_import_se_print_summary(void)
{
    os_printf("soft_erase round=%u/%u read_only_read=%u\r\n",
              s_se_ctx.stats.round_cnt,
              FLASH_IMPORT_SE_ROUND_LIMIT,
              s_se_ctx.stats.readonly_read_cnt);

    os_printf("soft_erase byte_err page=%u non_page=%u erase_vfy=%u\r\n",
              s_se_ctx.stats.wr_page_byte_err,
              s_se_ctx.stats.wr_non_page_byte_err,
              s_se_ctx.stats.erase_vfy_byte_err);

    os_printf("soft_erase read_only_target_err_max=%u read_only_prev_next_err_max=%u\r\n",
              s_se_ctx.stats.readonly_target_err_max,
              s_se_ctx.stats.readonly_prev_next_err_max);
}

static void flash_import_se_main_loop(void)
{
    bk_err_t ret = BK_OK;

    os_printf("soft_erase: start prev=0x%08x target=0x%08x next=0x%08x\r\n",
              s_se_ctx.win_addr,
              s_se_ctx.win_addr + FLASH_IMPORT_SECTOR_SIZE,
              s_se_ctx.win_addr + (FLASH_IMPORT_SECTOR_SIZE * 2U));
    os_printf("soft_erase: flash id=%lx\r\n", bk_flash_get_id());

    if (bk_flash_set_clk_freq(FLASH_CLK_FREQ_80M_HZ) != BK_OK) {
        os_printf("soft_erase: set clk 80M fail\r\n");
        return;
    }

    flash_import_soft_erase_erase(s_se_ctx.win_addr);
    flash_import_soft_erase_erase(s_se_ctx.win_addr + (FLASH_IMPORT_SECTOR_SIZE * 2U));

    ret = flash_import_se_write_loop();
    if (ret != BK_OK) {
        goto exit;
    }

    os_printf("soft_erase: entering read-only phase\r\n");
    flash_import_se_read_loop();

exit:
    flash_import_se_print_summary();
}

static void flash_import_se_thread(beken_thread_arg_t arg)
{
    (void)arg;

    if (flash_import_se_enter() == BK_OK) {
        flash_import_se_main_loop();
    }

    flash_import_se_exit();
}

void flash_import_case_soft_erase_start(void)
{
    bk_err_t ret = BK_OK;

    if (s_se_ctx.task_handle != NULL) {
        os_printf("soft_erase: busy, stop current test first\r\n");
        return;
    }

    os_memset(&s_se_ctx, 0, sizeof(s_se_ctx));
    s_se_ctx.stop_requested = false;

    ret = rtos_create_thread(&s_se_ctx.task_handle,
                             4,
                             "flash_import_se",
                             (beken_thread_function_t)flash_import_se_thread,
                             4096,
                             NULL);
    if (ret != BK_OK) {
        os_printf("soft_erase: create thread fail:%d\r\n", ret);
        s_se_ctx.task_handle = NULL;
        return;
    }
}

void flash_import_case_soft_erase_stop(void)
{
    if (s_se_ctx.task_handle != NULL) {
        s_se_ctx.stop_requested = true;
        return;
    }

    os_printf("soft_erase: not running\r\n");
}

void flash_import_case_soft_erase_show(void)
{
    if (s_se_ctx.task_handle != NULL) {
        os_printf("soft_erase: still running, stop first\r\n");
        return;
    }

    os_printf("\r\nBuild_info:%s_%s\r\n", __DATE__, __TIME__);
    os_printf("flash id:%lx\r\n", bk_flash_get_id());
    os_printf("soft_erase prev_addr: 0x%08x target_addr: 0x%08x next_addr: 0x%08x\r\n",
              s_se_ctx.win_addr,
              s_se_ctx.win_addr + FLASH_IMPORT_SECTOR_SIZE,
              s_se_ctx.win_addr + (FLASH_IMPORT_SECTOR_SIZE * 2U));
    flash_import_se_print_summary();
}
