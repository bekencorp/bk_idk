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
#include <string.h>
#include <driver/aon_rtc.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include <components/system.h>
#include "modules/pm.h"
#include "flash_import_internal.h"

#define FLASH_IMPORT_SLEEP_MS                 4000U
#define SLEEP_RESULT_MAGIC                    0x534C5032U
#define FLASH_IMPORT_SLEEP_RTC_THRESHOLD_MS   500U
#define FLASH_IMPORT_SLEEP_LOG_PREFIX         "flash_import sleep"
#define FLASH_IMPORT_SLEEP_ERASE_PATTERN      0xFFU

typedef enum {
    SLEEP_STATE_PRE = 0,
    SLEEP_STATE_POST,
} sleep_state_t;

typedef struct {
    const char *name;
    bool pre_erase;
    bool pre_write;
    bool post_erase;
    bool post_write;
} sleep_case_ops_t;

static const sleep_case_ops_t s_sleep_ops[] = {
    /* name,              pre_erase, pre_write, post_erase, post_write */
    { "ERASE_WRITE",       true,      true,      false,     false     },
    { "ERASE_ONLY",        true,      false,     false,     true      },
    { "ERASE_WRITE_ERASE", true,      false,     false,     false     },
    { "WRITE_ONLY",        false,     true,      true,      false     },
};

#define SLEEP_CASE_COUNT  (sizeof(s_sleep_ops) / sizeof(s_sleep_ops[0]))

typedef struct {
    uint32_t magic;
    union {
        struct {
            uint32_t case_id   : 2;
            uint32_t state     : 1;
            uint32_t is_deep   : 1;
            uint32_t test_addr : 28;
        } bits;
        uint32_t value;
    } data;
} sleep_result_t;

#define SLEEP_RESULTS_PER_SECTOR  (FLASH_IMPORT_SECTOR_SIZE / sizeof(sleep_result_t))

static beken_thread_t s_sleep_task_handle = NULL;
static volatile bool s_sleep_lv_wakeup_flag = false;
static sleep_result_t s_sleep_result = {0};
static uint32_t s_sleep_result_addr = 0;
static uint32_t s_sleep_test_start_addr = 0;
static uint32_t s_sleep_test_end_addr = 0;
static uint8_t *s_sleep_buf = NULL;
static uint8_t *s_sleep_expect_buf = NULL;

static bool flash_import_sleep_result_is_valid(const sleep_result_t *result)
{
    uint32_t test_addr = result->data.bits.test_addr;

    if (result->magic == 0xFFFFFFFFU) {
        return false;
    }
    if (result->magic != SLEEP_RESULT_MAGIC) {
        return false;
    }
    if (result->data.bits.case_id >= SLEEP_CASE_COUNT) {
        return false;
    }
    if (result->data.bits.state > SLEEP_STATE_POST) {
        return false;
    }
    if (test_addr < s_sleep_test_start_addr || test_addr >= s_sleep_test_end_addr) {
        return false;
    }
    if ((test_addr - s_sleep_test_start_addr) % FLASH_IMPORT_SECTOR_SIZE != 0U) {
        return false;
    }

    return true;
}

static bk_err_t flash_import_sleep_get_test_range(void)
{
    const bk_logic_partition_t *test_part = NULL;
    uint32_t test_start = 0;
    uint32_t test_end = 0;

    if (s_sleep_test_start_addr != 0U) {
        return BK_OK;
    }

    test_part = bk_flash_partition_get_info(FLASH_IMPORT_FLASH_TEST_PART_ID);
    if (test_part == NULL) {
        test_part = bk_flash_partition_get_info_by_name(FLASH_IMPORT_FLASH_TEST_PART_NAME);
    }
    if (test_part == NULL) {
        os_printf("sleep: flash_test partition lookup fail\r\n");
        return BK_FAIL;
    }

    test_start = test_part->partition_start_addr;
    test_end = test_start + test_part->partition_length;

    s_sleep_result_addr = test_start;
    s_sleep_test_start_addr = test_start + FLASH_IMPORT_SECTOR_SIZE;
    s_sleep_test_end_addr = test_end;

    if (s_sleep_test_start_addr + FLASH_IMPORT_SECTOR_SIZE > s_sleep_test_end_addr) {
        os_printf("sleep: flash_test too small start:0x%x end:0x%x\r\n",
                  s_sleep_test_start_addr, s_sleep_test_end_addr);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t flash_import_sleep_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_FOUR);
    ret = flash_import_read_bytes(buf, addr, size);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static bk_err_t flash_import_sleep_erase(uint32_t addr)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    flash_protect_type_t old_protect = flash_import_get_protect();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    ret = flash_import_erase_4k(addr);
    flash_import_set_protect(old_protect);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static bk_err_t flash_import_sleep_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    flash_line_mode_t old_mode = bk_flash_get_line_mode();
    flash_protect_type_t old_protect = flash_import_get_protect();
    bk_err_t ret = BK_OK;

    (void)flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_set_protect(FLASH_PROTECT_NONE);
    ret = flash_import_write_bytes(buf, addr, size);
    flash_import_set_protect(old_protect);
    (void)flash_import_set_line_mode(old_mode);
    return ret;
}

static bk_err_t flash_import_sleep_buffer_init(void)
{
    if (s_sleep_buf == NULL) {
        s_sleep_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
        if (s_sleep_buf == NULL) {
            os_printf("sleep: buf alloc fail\r\n");
            return BK_FAIL;
        }
    }

    if (s_sleep_expect_buf == NULL) {
        s_sleep_expect_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
        if (s_sleep_expect_buf == NULL) {
            os_printf("sleep: expect buf alloc fail\r\n");
            return BK_FAIL;
        }
    }

    return BK_OK;
}

static void flash_import_sleep_buffer_deinit(void)
{
    if (s_sleep_expect_buf != NULL) {
        os_free(s_sleep_expect_buf);
        s_sleep_expect_buf = NULL;
    }

    if (s_sleep_buf != NULL) {
        os_free(s_sleep_buf);
        s_sleep_buf = NULL;
    }
}

static bk_err_t flash_import_sleep_load_result(void)
{
    bk_err_t ret = BK_OK;

    ret = flash_import_sleep_get_test_range();
    if (ret != BK_OK) {
        return ret;
    }
    ret = flash_import_sleep_buffer_init();
    if (ret != BK_OK) {
        return ret;
    }

    return flash_import_sleep_read(s_sleep_result_addr, s_sleep_buf, FLASH_IMPORT_SECTOR_SIZE);
}

static bk_err_t flash_import_sleep_result_restore(sleep_result_t *result)
{
    bool found = false;
    bk_err_t ret = BK_OK;

    ret = flash_import_sleep_load_result();
    if (ret != BK_OK) {
        return ret;
    }

    const sleep_result_t *saved_results = (const sleep_result_t *)s_sleep_buf;

    for (uint32_t result_idx = 0; result_idx < SLEEP_RESULTS_PER_SECTOR; result_idx++) {
        const sleep_result_t *saved_result = &saved_results[result_idx];

        if (saved_result->magic == 0xFFFFFFFFU) {
            break;
        }
        if (flash_import_sleep_result_is_valid(saved_result)) {
            *result = *saved_result;
            found = true;
            continue;
        }
        break;
    }

    if (found) {
        os_printf("sleep: resume addr:0x%x case:%u %s state:%u deep:%u\r\n",
                  result->data.bits.test_addr, result->data.bits.case_id,
                  s_sleep_ops[result->data.bits.case_id].name,
                  result->data.bits.state, result->data.bits.is_deep);
        return BK_OK;
    }

    result->magic = 0;
    result->data.bits.test_addr = s_sleep_test_start_addr;
    result->data.bits.case_id = 0;
    result->data.bits.is_deep = 0;
    result->data.bits.state = SLEEP_STATE_PRE;

    return BK_OK;
}

static bk_err_t flash_import_sleep_result_save(sleep_result_t *result)
{
    sleep_result_t to_save = {0};
    uint32_t next_idx = 0;
    bk_err_t ret = BK_OK;

    ret = flash_import_sleep_load_result();
    if (ret != BK_OK) {
        return ret;
    }

    const sleep_result_t *saved_results = (const sleep_result_t *)s_sleep_buf;

    for (uint32_t result_idx = 0; result_idx < SLEEP_RESULTS_PER_SECTOR; result_idx++) {
        const sleep_result_t *saved_result = &saved_results[result_idx];

        if (saved_result->magic == 0xFFFFFFFFU) {
            break;
        }
        if (!flash_import_sleep_result_is_valid(saved_result)) {
            break;
        }
        next_idx = result_idx + 1U;
    }

    if (next_idx >= SLEEP_RESULTS_PER_SECTOR) {
        ret = flash_import_sleep_erase(s_sleep_result_addr);
        if (ret != BK_OK) {
            os_printf("sleep: result sector erase fail\r\n");
            return ret;
        }
        next_idx = 0;
    }

    to_save = *result;
    to_save.magic = SLEEP_RESULT_MAGIC;

    ret = flash_import_sleep_write(s_sleep_result_addr + next_idx * sizeof(sleep_result_t),
                                   (const uint8_t *)&to_save, sizeof(to_save));
    if (ret != BK_OK) {
        os_printf("sleep: result write fail idx:%u\r\n", next_idx);
        return ret;
    }

    return ret;
}

static void flash_import_sleep_fill_pattern(uint8_t *buf, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = (uint8_t)(i & 0xFFU);
    }
}

static bk_err_t flash_import_sleep_read_check(uint32_t addr, bool erase_check)
{
    bk_err_t ret = BK_OK;

    if (erase_check) {
        os_memset(s_sleep_expect_buf, FLASH_IMPORT_SLEEP_ERASE_PATTERN, FLASH_IMPORT_SECTOR_SIZE);
    } else {
        flash_import_sleep_fill_pattern(s_sleep_expect_buf, FLASH_IMPORT_SECTOR_SIZE);
    }

    ret = flash_import_sleep_read(addr, s_sleep_buf, FLASH_IMPORT_SECTOR_SIZE);
    if (ret != BK_OK) {
        return ret;
    }
    if (os_memcmp(s_sleep_buf, s_sleep_expect_buf, FLASH_IMPORT_SECTOR_SIZE) != 0) {
        os_printf("sleep: check fail addr:0x%x ff:%u\r\n", addr, (uint32_t)erase_check);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t flash_import_sleep_check_adjacent_sectors(uint32_t addr)
{
    bk_err_t ret = BK_OK;

    if (addr > s_sleep_test_start_addr) {
        ret = flash_import_sleep_read_check(addr - FLASH_IMPORT_SECTOR_SIZE, true);
        if (ret != BK_OK) {
            return ret;
        }
    }
    if (addr + FLASH_IMPORT_SECTOR_SIZE < s_sleep_test_end_addr) {
        ret = flash_import_sleep_read_check(addr + FLASH_IMPORT_SECTOR_SIZE, true);
        if (ret != BK_OK) {
            return ret;
        }
    }

    return BK_OK;
}

static bk_err_t flash_import_sleep_run_phase(const sleep_result_t *result, bool do_erase, bool do_write)
{
    uint32_t addr = result->data.bits.test_addr;
    bk_err_t ret = BK_OK;

    if (do_erase) {
        ret = flash_import_sleep_erase(addr);
        if (ret != BK_OK) {
            return ret;
        }
        ret = flash_import_sleep_read_check(addr, true);
        if (ret != BK_OK) {
            return ret;
        }
    }
    if (do_write) {
        flash_import_sleep_fill_pattern(s_sleep_buf, FLASH_IMPORT_SECTOR_SIZE);
        ret = flash_import_sleep_write(addr, s_sleep_buf, FLASH_IMPORT_SECTOR_SIZE);
        if (ret != BK_OK) {
            return ret;
        }
        ret = flash_import_sleep_read_check(addr, false);
        if (ret != BK_OK) {
            return ret;
        }
    }

    return BK_OK;
}

static bk_err_t flash_import_sleep_run_ops(const sleep_result_t *result, const sleep_case_ops_t *ops, bool is_post)
{
    uint32_t addr = result->data.bits.test_addr;
    bk_err_t ret = BK_OK;

    if (!is_post) {
        return flash_import_sleep_run_phase(result, ops->pre_erase, ops->pre_write);
    }

    ret = flash_import_sleep_read_check(addr, ops->pre_erase && !ops->pre_write);
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_sleep_run_phase(result, ops->post_erase, ops->post_write);
    if (ret != BK_OK) {
        return ret;
    }

    return flash_import_sleep_check_adjacent_sectors(addr);
}

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
static void flash_import_sleep_lv_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
    (void)id;
    (void)name_p;
    (void)param;

    bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
    s_sleep_lv_wakeup_flag = true;
    os_printf("sleep: lv rtc cb src:%d\r\n", bk_pm_exit_low_vol_wakeup_source_get());
}
#endif

static bool flash_import_sleep_is_deep_rtc_wakeup(void)
{
    if (bk_pm_deep_sleep_wakeup_source_get() == PM_WAKEUP_SOURCE_INT_RTC) {
        return true;
    }
    if (bk_misc_get_reset_reason() == RESET_SOURCE_DEEPPS_RTC) {
        return true;
    }
    return false;
}

static bk_err_t flash_import_sleep_resume(sleep_result_t *result, bool *printed)
{
    bk_err_t ret = BK_FAIL;

    if (printed) {
        *printed = false;
    }
    ret = flash_import_sleep_result_restore(result);
    if (ret != BK_OK) {
        return ret;
    }
    if (result->data.bits.state != SLEEP_STATE_POST) {
        return BK_FAIL;
    }

    if (result->data.bits.is_deep != 0U) {
        if (!flash_import_sleep_is_deep_rtc_wakeup()) {
            os_printf("sleep: deep wake fail pm_src:%u reset:0x%x\r\n",
                      (unsigned)bk_pm_deep_sleep_wakeup_source_get(),
                      bk_misc_get_reset_reason());
            os_printf("%s fail\r\n", FLASH_IMPORT_SLEEP_LOG_PREFIX);
            if (printed) {
                *printed = true;
            }
            return BK_FAIL;
        }
    }

    if (result->data.bits.case_id >= SLEEP_CASE_COUNT) {
        os_printf("sleep: post invalid case:%u\r\n", result->data.bits.case_id);
        return BK_FAIL;
    }

    ret = flash_import_sleep_run_ops(result, &s_sleep_ops[result->data.bits.case_id], true);
    if (ret != BK_OK) {
        os_printf("sleep: post fail case:%u %s\r\n",
                  result->data.bits.case_id, s_sleep_ops[result->data.bits.case_id].name);
        return ret;
    }

    uint32_t next_case = result->data.bits.case_id + 1U;

    if (next_case >= SLEEP_CASE_COUNT) {
        result->data.bits.case_id = 0;
        result->data.bits.test_addr += FLASH_IMPORT_SECTOR_SIZE;
        if (result->data.bits.test_addr >= s_sleep_test_end_addr) {
            result->data.bits.test_addr = s_sleep_test_start_addr;
        }
    } else {
        result->data.bits.case_id = next_case;
    }

    result->data.bits.state = SLEEP_STATE_PRE;

    ret = flash_import_sleep_result_save(result);
    os_printf("%s %s\r\n", FLASH_IMPORT_SLEEP_LOG_PREFIX, ret == BK_OK ? "pass" : "fail");
    if (printed) {
        *printed = true;
    }
    return ret;
}

bk_err_t flash_import_case_sleep_resume(void)
{
    return flash_import_sleep_resume(&s_sleep_result, NULL);
}

static void flash_import_sleep_run_thread(beken_thread_arg_t arg)
{
    bool is_deep = ((uint32_t)arg != 0U);
    bk_err_t ret = BK_FAIL;
    bool resume_printed = false;
    sleep_result_t *result = &s_sleep_result;

    ret = flash_import_sleep_result_restore(result);
    if (ret != BK_OK) {
        goto done;
    }
    if (result->data.bits.state == SLEEP_STATE_POST) {
        bool flash_deep = (result->data.bits.is_deep != 0U);

        if (is_deep != flash_deep) {
            os_printf("sleep: cli %s flash %s mismatch\r\n",
                      is_deep ? "deep" : "lv", flash_deep ? "deep" : "lv");
            goto done;
        }
        ret = flash_import_sleep_resume(result, &resume_printed);
        goto done;
    }

    if (result->data.bits.state != SLEEP_STATE_PRE) {
        os_printf("sleep: bad state:%u\r\n", result->data.bits.state);
        goto done;
    }

    if (result->data.bits.case_id >= SLEEP_CASE_COUNT) {
        goto done;
    }

    os_printf("sleep: case:%u %s addr:0x%08x\r\n",
              result->data.bits.case_id, s_sleep_ops[result->data.bits.case_id].name,
              result->data.bits.test_addr);

    ret = flash_import_sleep_run_ops(result, &s_sleep_ops[result->data.bits.case_id], false);
    if (ret != BK_OK) {
        os_printf("sleep: pre fail case:%u %s\r\n",
                  result->data.bits.case_id, s_sleep_ops[result->data.bits.case_id].name);
        goto done;
    }

    result->data.bits.state = SLEEP_STATE_POST;
    result->data.bits.is_deep = is_deep ? 1U : 0U;
    ret = flash_import_sleep_result_save(result);
    if (ret != BK_OK) {
        goto done;
    }

    if (is_deep) {
        flash_import_sleep_buffer_deinit();

        if (FLASH_IMPORT_SLEEP_MS < FLASH_IMPORT_SLEEP_RTC_THRESHOLD_MS) {
            os_printf("sleep: deep time %u too small\r\n", FLASH_IMPORT_SLEEP_MS);
            goto done;
        }

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
        alarm_info_t deep_alarm = {
            "deep_ps",
            FLASH_IMPORT_SLEEP_MS * AON_RTC_MS_TICK_CNT,
            1,
            NULL,
            NULL
        };

        bk_alarm_unregister(AON_RTC_ID_1, (uint8_t *)"deep_ps");
        bk_alarm_register(AON_RTC_ID_1, &deep_alarm);
#endif

        bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
        bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_BTSP, PM_POWER_MODULE_STATE_OFF);
        bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_WIFIP_MAC, PM_POWER_MODULE_STATE_OFF);
        bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_AUDP, PM_POWER_MODULE_STATE_OFF);
        bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_VIDP, PM_POWER_MODULE_STATE_OFF);

#if (CONFIG_CPU_CNT > 1)
        extern void stop_cpu1_core(void);
        stop_cpu1_core();
        bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_OFF);
#endif

        bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

        while (true) {
            rtos_delay_milliseconds(100);
        }
    }

    uint32_t wait_count = 0;
    const uint32_t wait_interval_ms = 100U;
    const uint32_t timeout_ms = FLASH_IMPORT_SLEEP_MS + 1000U;

    if (FLASH_IMPORT_SLEEP_MS < FLASH_IMPORT_SLEEP_RTC_THRESHOLD_MS) {
        os_printf("sleep: lv time %u too small\r\n", FLASH_IMPORT_SLEEP_MS);
        goto done;
    }

    s_sleep_lv_wakeup_flag = false;
    bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
    alarm_info_t lv_alarm = {
        "lv_test",
        FLASH_IMPORT_SLEEP_MS * AON_RTC_MS_TICK_CNT,
        1,
        flash_import_sleep_lv_rtc_callback,
        NULL
    };

    bk_alarm_unregister(AON_RTC_ID_1, (uint8_t *)"lv_test");
    bk_alarm_register(AON_RTC_ID_1, &lv_alarm);
#endif

    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0);

    while (!s_sleep_lv_wakeup_flag && (wait_count * wait_interval_ms < timeout_ms)) {
        rtos_delay_milliseconds(wait_interval_ms);
        wait_count++;
    }

    if (!s_sleep_lv_wakeup_flag) {
        os_printf("sleep: lv wakeup timeout\r\n");
        bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
        goto done;
    }

    ret = flash_import_sleep_resume(result, &resume_printed);

done:
    if (!resume_printed) {
        os_printf("%s %s\r\n", FLASH_IMPORT_SLEEP_LOG_PREFIX, ret == BK_OK ? "pass" : "fail");
    }

    flash_import_sleep_buffer_deinit();
    s_sleep_task_handle = NULL;
    rtos_delete_thread(NULL);
}

void flash_import_case_sleep_run(bool is_deep)
{
    bk_err_t ret = BK_OK;

    if (s_sleep_task_handle != NULL) {
        os_printf("%s fail\r\n", FLASH_IMPORT_SLEEP_LOG_PREFIX);
        return;
    }

    ret = rtos_create_thread(&s_sleep_task_handle,
                             4,
                             "flash_import_sleep",
                             (beken_thread_function_t)flash_import_sleep_run_thread,
                             4096,
                             (beken_thread_arg_t)(uint32_t)is_deep);
    if (ret != BK_OK) {
        s_sleep_task_handle = NULL;
        os_printf("%s fail\r\n", FLASH_IMPORT_SLEEP_LOG_PREFIX);
    }
}
