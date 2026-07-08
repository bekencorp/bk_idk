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
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_import_internal.h"

#define FLASH_IMPORT_LOOP_LOG_INTERVAL    (100U)
#define FLASH_IMPORT_LOOP_PATTERN_XOR     (0xA5U)

typedef struct {
    uint32_t loop_cnt;
    uint32_t sector_cnt;
    uint32_t test_cnt;
    uint32_t error_cnt;
    uint32_t erase_err_cnt;
    uint32_t write_p1_err_cnt;
    uint32_t write_p2_err_cnt;
} flash_import_loop_stats_t;

static beken_thread_t s_loop_task_handle = NULL;
static bool s_loop_stop_requested = false;
static bool s_loop_context_saved = false;
static uint32_t s_loop_test_start_addr = 0;
static uint32_t s_loop_test_end_addr = 0;
static flash_import_loop_stats_t s_loop_stats = {0};
static flash_import_context_t s_loop_saved_ctx = FLASH_IMPORT_CONTEXT_INITIALIZER;
static uint8_t *s_loop_buf = NULL;
static uint8_t *s_loop_expect_buf = NULL;

static bk_err_t flash_import_loop_get_test_range(void)
{
    const bk_logic_partition_t *app_part = NULL;
    uint32_t app_end = 0;

    app_part = bk_flash_partition_get_info(FLASH_IMPORT_APP_PART_ID);
    if (app_part == NULL) {
        app_part = bk_flash_partition_get_info_by_name(FLASH_IMPORT_APP_PART_NAME);
    }
    if (app_part == NULL) {
        app_part = bk_flash_partition_get_info_by_name(FLASH_IMPORT_APP_PART_ALIAS);
    }
    if (app_part == NULL) {
        os_printf("loop: app partition lookup fail\r\n");
        return BK_FAIL;
    }

    app_end = app_part->partition_start_addr + app_part->partition_length;
    s_loop_test_start_addr = (app_end + FLASH_IMPORT_SECTOR_SIZE - 1U)
                             & ~(FLASH_IMPORT_SECTOR_SIZE - 1U);
    s_loop_test_end_addr = bk_flash_get_current_total_size();
    if (s_loop_test_start_addr >= s_loop_test_end_addr) {
        os_printf("loop: invalid range start:0x%x end:0x%x\r\n",
                  s_loop_test_start_addr, s_loop_test_end_addr);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t flash_import_loop_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_FOUR);
    ret = flash_import_read_bytes(buf, addr, size);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static bk_err_t flash_import_loop_erase(uint32_t addr)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    ret = flash_import_erase_4k(addr);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static bk_err_t flash_import_loop_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    ret = flash_import_write_bytes(buf, addr, size);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static void flash_import_loop_fill_p1(uint8_t *buf, uint32_t addr, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = (uint8_t)((addr + i) ^ FLASH_IMPORT_LOOP_PATTERN_XOR);
    }
}

static void flash_import_loop_fill_p2(uint8_t *buf, uint32_t addr, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = (uint8_t)(~((uint8_t)((addr + i) ^ FLASH_IMPORT_LOOP_PATTERN_XOR)));
    }
}

static void flash_import_loop_print_summary(void)
{
    os_printf("loop summary: loops:%u sectors:%u tests:%u err:%u erase:%u p1:%u p2:%u\r\n",
              s_loop_stats.loop_cnt,
              s_loop_stats.sector_cnt,
              s_loop_stats.test_cnt,
              s_loop_stats.error_cnt,
              s_loop_stats.erase_err_cnt,
              s_loop_stats.write_p1_err_cnt,
              s_loop_stats.write_p2_err_cnt);
}

static bk_err_t flash_import_loop_buffers_init(void)
{
    s_loop_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
    if (s_loop_buf == NULL) {
        os_printf("loop: buf alloc fail\r\n");
        return BK_FAIL;
    }

    s_loop_expect_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
    if (s_loop_expect_buf == NULL) {
        os_printf("loop: expect buf alloc fail\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

static void flash_import_loop_buffers_deinit(void)
{
    if (s_loop_expect_buf != NULL) {
        os_free(s_loop_expect_buf);
        s_loop_expect_buf = NULL;
    }

    if (s_loop_buf != NULL) {
        os_free(s_loop_buf);
        s_loop_buf = NULL;
    }
}

static bk_err_t flash_import_loop_prepare_sector(uint32_t addr)
{
    bk_err_t ret = BK_OK;

    flash_import_set_protect(FLASH_PROTECT_NONE);
    ret = flash_import_loop_erase(addr);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    if (ret != BK_OK) {
        return ret;
    }

    os_memset(s_loop_buf, 0, FLASH_IMPORT_SECTOR_SIZE);
    ret = flash_import_loop_read(addr, s_loop_buf, FLASH_IMPORT_SECTOR_SIZE);
    if (ret != BK_OK) {
        return ret;
    }

    os_memset(s_loop_expect_buf, 0xFFU, FLASH_IMPORT_SECTOR_SIZE);
    if (os_memcmp(s_loop_buf, s_loop_expect_buf, FLASH_IMPORT_SECTOR_SIZE) != 0) {
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t flash_import_loop_prepare_region(void)
{
    uint32_t addr = 0;
    uint32_t prepare_cnt = 0;

    os_printf("loop: prepare start 0x%08x-0x%08x\r\n", s_loop_test_start_addr, s_loop_test_end_addr);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    os_memset(s_loop_expect_buf, 0xFFU, FLASH_IMPORT_SECTOR_SIZE);

    for (addr = s_loop_test_start_addr; addr < s_loop_test_end_addr; addr += FLASH_IMPORT_SECTOR_SIZE) {
        if (flash_import_loop_read(addr, s_loop_buf, FLASH_IMPORT_SECTOR_SIZE) != BK_OK) {
            os_printf("loop: prepare read fail 0x%08x\r\n", addr);
            flash_import_set_protect(FLASH_PROTECT_ALL);
            return BK_FAIL;
        }

        if (os_memcmp(s_loop_buf, s_loop_expect_buf, FLASH_IMPORT_SECTOR_SIZE) == 0) {
            continue;
        }

        if (flash_import_loop_prepare_sector(addr) != BK_OK) {
            os_printf("loop: prepare fail 0x%08x\r\n", addr);
            flash_import_set_protect(FLASH_PROTECT_ALL);
            return BK_FAIL;
        }
        prepare_cnt++;
    }

    flash_import_set_protect(FLASH_PROTECT_ALL);
    os_printf("loop: prepare done, erased %u sectors\r\n", prepare_cnt);
    return BK_OK;
}

static bk_err_t flash_import_loop_init(void)
{
    bk_err_t ret = BK_OK;

    flash_import_context_save(&s_loop_saved_ctx);
    s_loop_context_saved = true;

    ret = flash_import_loop_get_test_range();
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_loop_buffers_init();
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_loop_prepare_region();
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

static void flash_import_loop_deinit(void)
{
    if (s_loop_context_saved) {
        flash_import_context_restore(&s_loop_saved_ctx);
        s_loop_context_saved = false;
    }

    flash_import_loop_buffers_deinit();
    s_loop_task_handle = NULL;
    rtos_delete_thread(NULL);
}

static bk_err_t flash_import_loop_test_sector(uint32_t addr)
{
    bk_err_t ret = BK_OK;

    if (s_loop_stop_requested) {
        return BK_FAIL;
    }

    os_memset(s_loop_buf, 0, FLASH_IMPORT_SECTOR_SIZE);

    flash_import_set_protect(FLASH_PROTECT_NONE);
    ret = flash_import_loop_erase(addr);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_loop_read(addr, s_loop_buf, FLASH_IMPORT_SECTOR_SIZE);
    if (ret != BK_OK) {
        return ret;
    }

    s_loop_stats.test_cnt++;
    os_memset(s_loop_expect_buf, 0xFFU, FLASH_IMPORT_SECTOR_SIZE);
    if (os_memcmp(s_loop_buf, s_loop_expect_buf, FLASH_IMPORT_SECTOR_SIZE) != 0) {
        s_loop_stats.erase_err_cnt++;
        s_loop_stats.error_cnt++;
        os_printf("loop err: erase 0x%06x test_cnt=%u total_err=%u\r\n",
                  addr, s_loop_stats.test_cnt, s_loop_stats.error_cnt);
    }

    if (s_loop_stop_requested) {
        return BK_FAIL;
    }

    flash_import_loop_fill_p1(s_loop_buf, addr, FLASH_IMPORT_SECTOR_SIZE);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    ret = flash_import_loop_write(addr, s_loop_buf, FLASH_IMPORT_SECTOR_SIZE);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    if (ret != BK_OK) {
        return ret;
    }

    os_memset(s_loop_buf, 0, FLASH_IMPORT_SECTOR_SIZE);
    ret = flash_import_loop_read(addr, s_loop_buf, FLASH_IMPORT_SECTOR_SIZE);
    if (ret != BK_OK) {
        return ret;
    }

    s_loop_stats.test_cnt++;
    flash_import_loop_fill_p1(s_loop_expect_buf, addr, FLASH_IMPORT_SECTOR_SIZE);
    if (os_memcmp(s_loop_buf, s_loop_expect_buf, FLASH_IMPORT_SECTOR_SIZE) != 0) {
        s_loop_stats.write_p1_err_cnt++;
        s_loop_stats.error_cnt++;
        os_printf("loop err: write_p1 0x%06x test_cnt=%u total_err=%u\r\n",
                  addr, s_loop_stats.test_cnt, s_loop_stats.error_cnt);
    }

    if (s_loop_stop_requested) {
        return BK_FAIL;
    }

    flash_import_loop_fill_p2(s_loop_buf, addr, FLASH_IMPORT_SECTOR_SIZE);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    ret = flash_import_loop_write(addr, s_loop_buf, FLASH_IMPORT_SECTOR_SIZE);
    flash_import_set_protect(FLASH_PROTECT_ALL);
    if (ret != BK_OK) {
        return ret;
    }

    os_memset(s_loop_buf, 0, FLASH_IMPORT_SECTOR_SIZE);
    ret = flash_import_loop_read(addr, s_loop_buf, FLASH_IMPORT_SECTOR_SIZE);
    if (ret != BK_OK) {
        return ret;
    }

    s_loop_stats.test_cnt++;
    os_memset(s_loop_expect_buf, 0x00U, FLASH_IMPORT_SECTOR_SIZE);
    if (os_memcmp(s_loop_buf, s_loop_expect_buf, FLASH_IMPORT_SECTOR_SIZE) != 0) {
        s_loop_stats.write_p2_err_cnt++;
        s_loop_stats.error_cnt++;
        os_printf("loop err: write_p2 0x%06x test_cnt=%u total_err=%u\r\n",
                  addr, s_loop_stats.test_cnt, s_loop_stats.error_cnt);
    }

    return BK_OK;
}

static void flash_import_loop_main_loop(void)
{
    uint32_t addr = s_loop_test_start_addr;

    if (bk_flash_set_clk_freq(FLASH_CLK_FREQ_80M_HZ) != BK_OK) {
        os_printf("loop: set clk 80M fail\r\n");
        return;
    }

    os_printf("loop: range 0x%08x-0x%08x step 4K\r\n", s_loop_test_start_addr, s_loop_test_end_addr);

    while (!s_loop_stop_requested) {
        (void)flash_import_loop_test_sector(addr);

        s_loop_stats.sector_cnt++;
        addr += FLASH_IMPORT_SECTOR_SIZE;

        if ((s_loop_stats.sector_cnt % FLASH_IMPORT_LOOP_LOG_INTERVAL) == 0U) {
            flash_import_loop_print_summary();
        }

        if (addr >= s_loop_test_end_addr) {
            s_loop_stats.loop_cnt++;
            addr = s_loop_test_start_addr;
        }
    }

    flash_import_loop_print_summary();
    os_printf("loop: stopped\r\n");
}

static void flash_import_loop_thread(beken_thread_arg_t arg)
{
    (void)arg;

    if (flash_import_loop_init() == BK_OK) {
        flash_import_loop_main_loop();
    }

    flash_import_loop_deinit();
}

void flash_import_case_loop_start(void)
{
    bk_err_t ret = BK_OK;

    if (s_loop_task_handle != NULL) {
        os_printf("loop: already running\r\n");
        return;
    }

    os_memset(&s_loop_stats, 0, sizeof(s_loop_stats));
    s_loop_stop_requested = false;
    ret = rtos_create_thread(&s_loop_task_handle,
                             4,
                             "flash_import_loop",
                             (beken_thread_function_t)flash_import_loop_thread,
                             4096,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK) {
        os_printf("loop: create thread fail:%d\r\n", ret);
        s_loop_task_handle = NULL;
    }
}

void flash_import_case_loop_stop(void)
{
    if (s_loop_task_handle != NULL) {
        s_loop_stop_requested = true;
        os_printf("loop: stop requested\r\n");
    } else {
        os_printf("loop: not running\r\n");
    }
}

void flash_import_case_loop_show(void)
{
    os_printf("\r\nBuild_info:%s_%s\r\n", __DATE__, __TIME__);
    os_printf("flash id:%lx\r\n", bk_flash_get_id());
    os_printf("loop task:%s\r\n", (s_loop_task_handle != NULL) ? "running" : "stopped");
    os_printf("loop range:0x%08x-0x%08x\r\n", s_loop_test_start_addr, s_loop_test_end_addr);
    os_printf("loop_cnt:%u\r\n", s_loop_stats.loop_cnt);
    os_printf("sector_cnt:%u\r\n", s_loop_stats.sector_cnt);
    os_printf("test_cnt:%u\r\n", s_loop_stats.test_cnt);
    os_printf("error_cnt:%u\r\n", s_loop_stats.error_cnt);
    os_printf("erase_err_cnt:%u\r\n", s_loop_stats.erase_err_cnt);
    os_printf("write_p1_err_cnt:%u\r\n", s_loop_stats.write_p1_err_cnt);
    os_printf("write_p2_err_cnt:%u\r\n", s_loop_stats.write_p2_err_cnt);
}
