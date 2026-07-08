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
#include <driver/aon_rtc.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "modules/pm.h"
#include "sdkconfig.h"
#include "flash_import_internal.h"

#define FLASH_IMPORT_DRIVER_LOG_INTERVAL    (160U)

typedef struct {
    uint32_t sector_cnt;
    uint32_t sector_err_cnt;
    uint32_t erase_err_cnt;
    uint32_t erase_verify_err_cnt;
    uint32_t write_fail_miss_cnt;
    uint32_t write_err_cnt;
    uint32_t write_verify_err_cnt;
    uint32_t erase_fail_miss_cnt;
} flash_import_driver_result_t;

typedef struct {
    uint32_t read_2line_us;
    uint32_t read_4line_us;
} flash_import_driver_sector_timing_t;

static beken_thread_t s_driver_task_handle = NULL;
static flash_import_driver_result_t s_driver_result = {0};
static flash_import_context_t s_driver_saved_ctx = FLASH_IMPORT_CONTEXT_INITIALIZER;
static uint32_t s_driver_test_start_addr = 0;
static uint32_t s_driver_test_end_addr = 0;
static bool s_driver_context_saved = false;
static uint8_t *s_driver_buf = NULL;
static uint8_t *s_driver_pattern_buf = NULL;
static flash_import_driver_sector_timing_t s_driver_sector_timing = {0};
static const uint32_t s_driver_flash_clk_hz[] = {
#if CONFIG_FLASH_IMPORT_DRIVER_LOW_FLASH_CLK_48M
    FLASH_CLK_FREQ_48M_HZ,
#else
    FLASH_CLK_FREQ_40M_HZ,
#endif
    FLASH_CLK_FREQ_60M_HZ,
    FLASH_CLK_FREQ_80M_HZ,
    FLASH_CLK_FREQ_120M_HZ,
};

static uint32_t flash_import_driver_flash_clk_count(void)
{
    return (uint32_t)(sizeof(s_driver_flash_clk_hz) / sizeof(s_driver_flash_clk_hz[0]));
}

static uint32_t flash_import_driver_time_delta_us(uint64_t start_us, uint64_t end_us)
{
    return (uint32_t)(end_us - start_us);
}

static void flash_import_driver_set_line_mode(flash_line_mode_t line_mode)
{
    (void)flash_import_set_line_mode(line_mode);
}

static void flash_import_driver_set_protect(flash_protect_type_t protect)
{
    flash_import_set_protect(protect);
}

static bk_err_t flash_import_driver_set_flash_clk_hz(uint32_t hz)
{
    return bk_flash_set_clk_freq(hz);
}

static bk_err_t flash_import_driver_set_cpu_freq_mhz(uint32_t cpu_freq_mhz)
{
    switch (cpu_freq_mhz) {
    case 120:
        return bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_120M);
    case 160:
#if CONFIG_RELIABILITY_VOTE_PM_320_FOR_CPU_FREQ_160
        return bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_320M);
#else
        return bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_160M);
#endif
    case 240:
        return bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_240M);
    default:
        return bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_120M);
    }
}

static bk_err_t flash_import_driver_read_core(uint32_t addr, uint8_t *buf, uint32_t size,
                                              flash_line_mode_t line_mode, uint32_t *op_us)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    bk_err_t ret = BK_OK;
    uint64_t t_start = 0;
    uint64_t t_end = 0;

    (void)flash_import_set_line_mode(line_mode);
    if (op_us != NULL) {
        t_start = bk_aon_rtc_get_us();
    }
    ret = flash_import_read_bytes(buf, addr, size);
    if (op_us != NULL) {
        t_end = bk_aon_rtc_get_us();
        *op_us = flash_import_driver_time_delta_us(t_start, t_end);
    }
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static void flash_import_driver_read_with_line(uint32_t addr, uint8_t *buf, uint32_t size,
                                               flash_line_mode_t line_mode)
{
    (void)flash_import_driver_read_core(addr, buf, size, line_mode, NULL);
}

static void flash_import_driver_fill_write_pattern(uint8_t *buf, uint32_t size, uint32_t addr)
{
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = (uint8_t)((addr + i) ^ 0xA5U);
    }
}

static void flash_import_driver_fill_erase_pattern(uint8_t *buf, uint32_t size)
{
    os_memset(buf, 0xFF, size);
}

static bk_err_t flash_import_driver_erase(uint32_t addr)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    ret = flash_import_erase_4k(addr);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static bk_err_t flash_import_driver_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    ret = flash_import_write_bytes(buf, addr, size);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static bk_err_t flash_import_driver_verify_read(uint32_t addr, const uint8_t *expect,
                                                flash_line_mode_t read_line, uint32_t *read_op_us)
{
    bk_err_t ret = BK_OK;

    ret = flash_import_driver_read_core(addr, s_driver_buf, FLASH_IMPORT_SECTOR_SIZE,
                                        read_line, read_op_us);
    if (ret != BK_OK) {
        return ret;
    }
    if (os_memcmp(s_driver_buf, expect, FLASH_IMPORT_SECTOR_SIZE) != 0) {
        return BK_FAIL;
    }
    return BK_OK;
}

static bk_err_t flash_import_driver_test_sector_phase(uint32_t addr, flash_line_mode_t read_line)
{
    bk_err_t ret = BK_OK;
    uint32_t *read_timing = NULL;

    flash_import_driver_set_protect(FLASH_PROTECT_NONE);
    if (flash_import_driver_erase(addr) != BK_OK) {
        s_driver_result.erase_err_cnt++;
        return BK_FAIL;
    }
    flash_import_driver_fill_erase_pattern(s_driver_pattern_buf, FLASH_IMPORT_SECTOR_SIZE);
    if (flash_import_driver_verify_read(addr, s_driver_pattern_buf, read_line, NULL) != BK_OK) {
        s_driver_result.erase_verify_err_cnt++;
        return BK_FAIL;
    }

    flash_import_driver_fill_write_pattern(s_driver_pattern_buf, FLASH_IMPORT_SECTOR_SIZE, addr);

    flash_import_driver_set_protect(FLASH_PROTECT_ALL);
    if (flash_import_driver_write(addr, s_driver_pattern_buf, FLASH_IMPORT_SECTOR_SIZE) != BK_OK) {
        s_driver_result.write_fail_miss_cnt++;
        ret = BK_FAIL;
    }
    if (flash_import_driver_verify_read(addr, s_driver_pattern_buf, read_line, NULL) == BK_OK) {
        s_driver_result.write_fail_miss_cnt++;
        ret = BK_FAIL;
    }

    flash_import_driver_set_protect(FLASH_PROTECT_NONE);
    if (flash_import_driver_write(addr, s_driver_pattern_buf, FLASH_IMPORT_SECTOR_SIZE) != BK_OK) {
        s_driver_result.write_err_cnt++;
        return BK_FAIL;
    }

    if (read_line == FLASH_LINE_MODE_FOUR) {
        read_timing = &s_driver_sector_timing.read_4line_us;
    } else {
        read_timing = &s_driver_sector_timing.read_2line_us;
    }
    if (flash_import_driver_verify_read(addr, s_driver_pattern_buf, read_line, read_timing) != BK_OK) {
        s_driver_result.write_verify_err_cnt++;
        return BK_FAIL;
    }

    flash_import_driver_fill_erase_pattern(s_driver_pattern_buf, FLASH_IMPORT_SECTOR_SIZE);
    flash_import_driver_set_protect(FLASH_PROTECT_ALL);
    if (flash_import_driver_erase(addr) != BK_OK) {
        s_driver_result.erase_fail_miss_cnt++;
        ret = BK_FAIL;
    }
    if (flash_import_driver_verify_read(addr, s_driver_pattern_buf, read_line, NULL) == BK_OK) {
        s_driver_result.erase_fail_miss_cnt++;
        ret = BK_FAIL;
    }

    return ret;
}

static bk_err_t flash_import_driver_get_test_range(void)
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
        os_printf("driver: app partition lookup fail\r\n");
        return BK_FAIL;
    }

    app_end = app_part->partition_start_addr + app_part->partition_length;
    s_driver_test_start_addr = (app_end + FLASH_IMPORT_SECTOR_SIZE - 1U)
                               & ~(FLASH_IMPORT_SECTOR_SIZE - 1U);
    s_driver_test_end_addr = bk_flash_get_current_total_size();
    if (s_driver_test_start_addr >= s_driver_test_end_addr) {
        os_printf("driver: invalid range start:0x%x end:0x%x\r\n", s_driver_test_start_addr, s_driver_test_end_addr);
        return BK_FAIL;
    }
    return BK_OK;
}

static bk_err_t flash_import_driver_buffers_init(void)
{
    s_driver_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
    if (s_driver_buf == NULL) {
        os_printf("driver: buf alloc fail\r\n");
        return BK_FAIL;
    }

    s_driver_pattern_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
    if (s_driver_pattern_buf == NULL) {
        os_printf("driver: pattern buf alloc fail\r\n");
        return BK_FAIL;
    }
    return BK_OK;
}

static bk_err_t flash_import_driver_setup_clock(void)
{
    bk_err_t ret = BK_OK;

    ret = flash_import_driver_set_cpu_freq_mhz(CONFIG_RELIABILITY_CPU_FREQ_MHZ);
    if (ret != BK_OK) {
        os_printf("driver: set cpu freq fail:%d\r\n", ret);
        return BK_FAIL;
    }

    os_printf("driver: cpu freq %u MHz\r\n", (unsigned)CONFIG_RELIABILITY_CPU_FREQ_MHZ);
    return BK_OK;
}

static bk_err_t flash_import_driver_test_region_init(void)
{
    uint32_t addr = 0;
    uint32_t scrub_cnt = 0;

    flash_import_driver_set_protect(FLASH_PROTECT_NONE);
    flash_import_driver_fill_erase_pattern(s_driver_pattern_buf, FLASH_IMPORT_SECTOR_SIZE);

    for (addr = s_driver_test_start_addr; addr < s_driver_test_end_addr; addr += FLASH_IMPORT_SECTOR_SIZE) {
        flash_import_driver_read_with_line(addr, s_driver_buf, FLASH_IMPORT_SECTOR_SIZE, FLASH_LINE_MODE_FOUR);
        if (os_memcmp(s_driver_buf, s_driver_pattern_buf, FLASH_IMPORT_SECTOR_SIZE) == 0) {
            continue;
        }

        if (flash_import_driver_erase(addr) != BK_OK) {
            os_printf("driver: scrub erase fail addr:0x%08x\r\n", addr);
            return BK_FAIL;
        }
        if (flash_import_driver_verify_read(addr, s_driver_pattern_buf, FLASH_LINE_MODE_FOUR, NULL) != BK_OK) {
            s_driver_result.erase_verify_err_cnt++;
            os_printf("driver: scrub verify fail addr:0x%08x\r\n", addr);
            return BK_FAIL;
        }
        scrub_cnt++;
    }

    os_printf("driver: region scrub done, erased %u sectors\r\n", scrub_cnt);
    return BK_OK;
}

static bk_err_t flash_import_driver_init(void)
{
    bk_err_t ret = BK_OK;

    flash_import_context_save(&s_driver_saved_ctx);
    s_driver_context_saved = true;

    ret = flash_import_driver_buffers_init();
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_driver_get_test_range();
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_driver_setup_clock();
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_driver_set_flash_clk_hz(FLASH_CLK_FREQ_80M_HZ);
    if (ret != BK_OK) {
        os_printf("driver: set flash clk 80M for scrub fail\r\n");
        return BK_FAIL;
    }

    ret = flash_import_driver_test_region_init();
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

static void flash_import_driver_deinit(void)
{
    if (s_driver_context_saved) {
        flash_import_context_restore(&s_driver_saved_ctx);
        s_driver_context_saved = false;
    }

    if (s_driver_buf != NULL) {
        os_free(s_driver_buf);
        s_driver_buf = NULL;
    }
    if (s_driver_pattern_buf != NULL) {
        os_free(s_driver_pattern_buf);
        s_driver_pattern_buf = NULL;
    }

    s_driver_task_handle = NULL;
    rtos_delete_thread(NULL);
}

static void flash_import_driver_run_sector_loop(void)
{
    uint32_t addr = 0;

    for (addr = s_driver_test_start_addr; addr < s_driver_test_end_addr; addr += FLASH_IMPORT_SECTOR_SIZE) {
        os_memset(&s_driver_sector_timing, 0, sizeof(s_driver_sector_timing));
        s_driver_result.sector_cnt++;
        if (flash_import_driver_test_sector_phase(addr, FLASH_LINE_MODE_TWO) != BK_OK) {
            s_driver_result.sector_err_cnt++;
            os_printf("driver: 2line fail addr:0x%08x\r\n", addr);
        }
        if (flash_import_driver_test_sector_phase(addr, FLASH_LINE_MODE_FOUR) != BK_OK) {
            s_driver_result.sector_err_cnt++;
            os_printf("driver: 4line read fail addr:0x%08x\r\n", addr);
        }

        if ((s_driver_result.sector_cnt % FLASH_IMPORT_DRIVER_LOG_INTERVAL) == 0U) {
            os_printf("driver: sectors:%u err:%u 2r:%u 4r:%u\r\n",
                      s_driver_result.sector_cnt,
                      s_driver_result.sector_err_cnt,
                      s_driver_sector_timing.read_2line_us,
                      s_driver_sector_timing.read_4line_us);
        }
    }
}

static void flash_import_driver_main_loop(void)
{
    uint32_t clk_idx = 0;
    uint32_t flash_mhz = 0;

    os_printf("driver: range 0x%08x-0x%08x step 4K\r\n", s_driver_test_start_addr, s_driver_test_end_addr);
    os_printf("driver: app end based scan\r\n");

    for (clk_idx = 0; clk_idx < flash_import_driver_flash_clk_count(); clk_idx++) {
        if (flash_import_driver_set_flash_clk_hz(s_driver_flash_clk_hz[clk_idx]) != BK_OK) {
            flash_mhz = s_driver_flash_clk_hz[clk_idx] / 1000000U;
            os_printf("driver: set flash clk %uM fail\r\n", flash_mhz);
            continue;
        }

        flash_mhz = s_driver_flash_clk_hz[clk_idx] / 1000000U;
        s_driver_result.sector_cnt = 0;
        s_driver_result.sector_err_cnt = 0;
        os_printf("driver: flash %uM test start\r\n", flash_mhz);
        flash_import_driver_run_sector_loop();
        os_printf("driver: flash %uM done sectors:%u err:%u\r\n",
                  flash_mhz,
                  s_driver_result.sector_cnt,
                  s_driver_result.sector_err_cnt);
    }

    os_printf("driver: all clk done\r\n");
    os_printf("driver: erase_err:%u ev_err:%u w_err:%u wv_err:%u\r\n",
              s_driver_result.erase_err_cnt,
              s_driver_result.erase_verify_err_cnt,
              s_driver_result.write_err_cnt,
              s_driver_result.write_verify_err_cnt);
    os_printf("driver: wfail_miss:%u efail_miss:%u\r\n",
              s_driver_result.write_fail_miss_cnt,
              s_driver_result.erase_fail_miss_cnt);
}

static void flash_import_driver_thread(beken_thread_arg_t arg)
{
    (void)arg;

    if (flash_import_driver_init() == BK_OK) {
        flash_import_driver_main_loop();
    }

    flash_import_driver_deinit();
}

void flash_import_case_driver_run(void)
{
    bk_err_t ret = BK_OK;

    if (s_driver_task_handle != NULL) {
        os_printf("driver: already running\r\n");
        return;
    }

    os_memset(&s_driver_result, 0, sizeof(s_driver_result));
    ret = rtos_create_thread(&s_driver_task_handle,
                             4,
                             "flash_import_driver",
                             (beken_thread_function_t)flash_import_driver_thread,
                             4096,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK) {
        os_printf("driver: create thread fail:%d\r\n", ret);
        s_driver_task_handle = NULL;
    }
}
