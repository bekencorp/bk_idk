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
#include <driver/gpio.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_import_internal.h"
#include "crc32.h"

#define FLASH_IMPORT_ENDURANCE_MAIN_OFFSET          (FLASH_IMPORT_SECTOR_SIZE)

#if CONFIG_FLASH_IMPORT_TEST_WITH_CBUS
#define FLASH_IMPORT_ENDURANCE_DATA_OFFSET          (0x11000U)
#else
#define FLASH_IMPORT_ENDURANCE_DATA_OFFSET          (FLASH_IMPORT_SECTOR_SIZE * 2U)
#endif

#define FLASH_IMPORT_ENDURANCE_LAYOUT_BYTES         (FLASH_IMPORT_ENDURANCE_DATA_OFFSET + \
                                                     FLASH_IMPORT_SECTOR_SIZE)
#define FLASH_IMPORT_ENDURANCE_MAIN_SAVE_INTERVAL   (10U)
#define FLASH_IMPORT_ENDURANCE_BACKUP_SAVE_INTERVAL (100U)
#define FLASH_IMPORT_ENDURANCE_LOG_INTERVAL         (100U)
#define FLASH_IMPORT_ENDURANCE_LOOP_LIMIT           (100000U)
#define FLASH_IMPORT_ENDURANCE_READ_LOG_INTERVAL    (100000U)
#define FLASH_IMPORT_ENDURANCE_CRC_INVALID          (0xFFFFFFFFU)

typedef struct {
    uint32_t data_total_cnt;
    uint32_t total_erase_error_cnt;
    uint32_t total_write_error_cnt;
    uint32_t data_erase_error_cnt;
    uint32_t data_write_error_cnt;
    uint32_t data_erase_op_cnt;
    uint32_t data_write_op_cnt;
    uint32_t main_erase_op_cnt;
    uint32_t main_write_op_cnt;
    uint32_t main_erase_error_cnt;
    uint32_t main_write_error_cnt;
    uint32_t backup_erase_op_cnt;
    uint32_t backup_write_op_cnt;
    uint32_t backup_erase_error_cnt;
    uint32_t backup_write_error_cnt;
    uint32_t global_protect_cnt;
    uint32_t global_unprotect_cnt;
    uint32_t crc;
} flash_import_endurance_stats_t;

typedef enum {
    FLASH_IMPORT_ENDURANCE_SAVE_SCHEDULED,
    FLASH_IMPORT_ENDURANCE_SAVE_BOTH,
} flash_import_endurance_save_policy_t;

typedef struct {
    gpio_id_t protect_all;
    gpio_id_t protect_none;
    gpio_id_t erase;
    gpio_id_t write;
    gpio_id_t read;
} flash_import_endurance_gpio_cfg_t;

#if CONFIG_SOC_BK7236
static const flash_import_endurance_gpio_cfg_t s_endurance_gpio_cfg = {
    .protect_all  = GPIO_14,
    .protect_none = GPIO_15,
    .erase        = GPIO_16,
    .write        = GPIO_17,
    .read         = GPIO_18,
};
#elif CONFIG_SOC_BK7236N
static const flash_import_endurance_gpio_cfg_t s_endurance_gpio_cfg = {
    .protect_all  = GPIO_14,
    .protect_none = GPIO_15,
    .erase        = GPIO_16,
    .write        = GPIO_17,
    .read         = GPIO_20,
};
#elif CONFIG_SOC_BK7239N
static const flash_import_endurance_gpio_cfg_t s_endurance_gpio_cfg = {
    .protect_all  = GPIO_4,
    .protect_none = GPIO_5,
    .erase        = GPIO_15,
    .write        = GPIO_16,
    .read         = GPIO_17,
};
#endif

#define FLASH_IMPORT_END_GPIO_PROTECT_ALL_PIN       (s_endurance_gpio_cfg.protect_all)
#define FLASH_IMPORT_END_GPIO_PROTECT_NONE_PIN      (s_endurance_gpio_cfg.protect_none)
#define FLASH_IMPORT_END_GPIO_ERASE_PIN             (s_endurance_gpio_cfg.erase)
#define FLASH_IMPORT_END_GPIO_WRITE_PIN             (s_endurance_gpio_cfg.write)
#define FLASH_IMPORT_END_GPIO_READ_PIN              (s_endurance_gpio_cfg.read)
#define FLASH_IMPORT_END_GPIO_PIN_CNT               (sizeof(s_endurance_gpio_cfg) / sizeof(gpio_id_t))

#define FLASH_IMPORT_END_GPIO_PROTECT_ALL_UP()      ((void)bk_gpio_set_output_high(FLASH_IMPORT_END_GPIO_PROTECT_ALL_PIN))
#define FLASH_IMPORT_END_GPIO_PROTECT_ALL_DOWN()    ((void)bk_gpio_set_output_low(FLASH_IMPORT_END_GPIO_PROTECT_ALL_PIN))
#define FLASH_IMPORT_END_GPIO_PROTECT_NONE_UP()     ((void)bk_gpio_set_output_high(FLASH_IMPORT_END_GPIO_PROTECT_NONE_PIN))
#define FLASH_IMPORT_END_GPIO_PROTECT_NONE_DOWN()   ((void)bk_gpio_set_output_low(FLASH_IMPORT_END_GPIO_PROTECT_NONE_PIN))
#define FLASH_IMPORT_END_GPIO_ERASE_UP()            ((void)bk_gpio_set_output_high(FLASH_IMPORT_END_GPIO_ERASE_PIN))
#define FLASH_IMPORT_END_GPIO_ERASE_DOWN()          ((void)bk_gpio_set_output_low(FLASH_IMPORT_END_GPIO_ERASE_PIN))
#define FLASH_IMPORT_END_GPIO_WRITE_UP()            ((void)bk_gpio_set_output_high(FLASH_IMPORT_END_GPIO_WRITE_PIN))
#define FLASH_IMPORT_END_GPIO_WRITE_DOWN()          ((void)bk_gpio_set_output_low(FLASH_IMPORT_END_GPIO_WRITE_PIN))
#define FLASH_IMPORT_END_GPIO_READ_UP()             ((void)bk_gpio_set_output_high(FLASH_IMPORT_END_GPIO_READ_PIN))
#define FLASH_IMPORT_END_GPIO_READ_DOWN()           ((void)bk_gpio_set_output_low(FLASH_IMPORT_END_GPIO_READ_PIN))

static beken_thread_t s_endurance_task_handle = NULL;
static bool s_endurance_stop_requested = false;
static uint32_t s_endurance_stats_backup_addr = 0;
static uint32_t s_endurance_stats_main_addr = 0;
static uint32_t s_endurance_data_addr = 0;
static flash_import_endurance_stats_t s_endurance_stats = {0};
static uint32_t s_endurance_read_op_cnt = 0;
static uint32_t s_endurance_read_error_cnt = 0;
static uint8_t *s_endurance_read_buf = NULL;
static uint8_t *s_endurance_write_buf = NULL;

static flash_import_context_t s_endurance_saved_ctx = FLASH_IMPORT_CONTEXT_INITIALIZER;
static bool s_endurance_context_saved = false;

static void flash_import_endurance_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
    FLASH_IMPORT_END_GPIO_READ_UP();
    flash_import_read_bytes(buf, addr, size);
    FLASH_IMPORT_END_GPIO_READ_DOWN();
}

static void flash_import_endurance_unprotect(void)
{
    s_endurance_stats.global_unprotect_cnt++;
    FLASH_IMPORT_END_GPIO_PROTECT_NONE_UP();
    flash_import_set_protect(FLASH_PROTECT_NONE);
    FLASH_IMPORT_END_GPIO_PROTECT_NONE_DOWN();
}

static void flash_import_endurance_protect(void)
{
    s_endurance_stats.global_protect_cnt++;
    FLASH_IMPORT_END_GPIO_PROTECT_ALL_UP();
    flash_import_set_protect(FLASH_PROTECT_ALL);
    FLASH_IMPORT_END_GPIO_PROTECT_ALL_DOWN();
}

static bk_err_t flash_import_endurance_erase(uint32_t addr)
{
    bool erase_verify_failed = false;
    flash_line_mode_t old_line_mode = FLASH_LINE_MODE_TWO;

    old_line_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_endurance_unprotect();
    FLASH_IMPORT_END_GPIO_ERASE_UP();
    (void)flash_import_erase_4k(addr);
    FLASH_IMPORT_END_GPIO_ERASE_DOWN();
    if (addr == s_endurance_stats_main_addr) {
        s_endurance_stats.main_erase_op_cnt++;
    } else if (addr == s_endurance_stats_backup_addr) {
        s_endurance_stats.backup_erase_op_cnt++;
    } else {
        s_endurance_stats.data_erase_op_cnt++;
    }
    flash_import_endurance_protect();
    (void)flash_import_set_line_mode(old_line_mode);

    flash_import_endurance_read(addr, s_endurance_read_buf, FLASH_IMPORT_SECTOR_SIZE);
    for (uint32_t i = 0; i < FLASH_IMPORT_SECTOR_SIZE; i++) {
        if (s_endurance_read_buf[i] != 0xFFU) {
            erase_verify_failed = true;
            break;
        }
    }

    if (erase_verify_failed) {
        if (addr == s_endurance_stats_main_addr) {
            s_endurance_stats.main_erase_error_cnt++;
        } else if (addr == s_endurance_stats_backup_addr) {
            s_endurance_stats.backup_erase_error_cnt++;
        } else {
            s_endurance_stats.data_erase_error_cnt++;
        }
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t flash_import_endurance_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    flash_line_mode_t old_line_mode = FLASH_LINE_MODE_TWO;

    old_line_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_endurance_unprotect();
    FLASH_IMPORT_END_GPIO_WRITE_UP();
    (void)flash_import_write_bytes(buf, addr, size);
    FLASH_IMPORT_END_GPIO_WRITE_DOWN();
    if (addr == s_endurance_stats_main_addr) {
        s_endurance_stats.main_write_op_cnt++;
    } else if (addr == s_endurance_stats_backup_addr) {
        s_endurance_stats.backup_write_op_cnt++;
    } else {
        s_endurance_stats.data_write_op_cnt++;
    }
    flash_import_endurance_protect();
    (void)flash_import_set_line_mode(old_line_mode);

    flash_import_endurance_read(addr, s_endurance_read_buf, size);
    if (os_memcmp(s_endurance_read_buf, buf, size) != 0) {
        if (addr == s_endurance_stats_main_addr) {
            s_endurance_stats.main_write_error_cnt++;
        } else if (addr == s_endurance_stats_backup_addr) {
            s_endurance_stats.backup_write_error_cnt++;
        } else {
            s_endurance_stats.data_write_error_cnt++;
        }
        return BK_FAIL;
    }
    return BK_OK;
}

#if CONFIG_FLASH_IMPORT_TEST_WITH_CBUS
__attribute__((section(".iram"))) static bk_err_t flash_import_endurance_write_cbus(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    uint32_t int_status = 0;
    uint32_t cbus_addr = FLASH_PHY2VIRTUAL(addr);
    flash_line_mode_t old_line_mode = FLASH_LINE_MODE_TWO;

    int_status = rtos_disable_int();
    old_line_mode = flash_import_set_line_mode(FLASH_LINE_MODE_TWO);
    flash_import_endurance_unprotect();
    FLASH_IMPORT_END_GPIO_WRITE_UP();
    (void)flash_import_write_cbus(buf, cbus_addr | (1UL << 24), size);
    FLASH_IMPORT_END_GPIO_WRITE_DOWN();
    s_endurance_stats.data_write_op_cnt++;
    FLASH_IMPORT_END_GPIO_READ_UP();
    (void)flash_import_read_cbus(s_endurance_read_buf, cbus_addr, size);
    FLASH_IMPORT_END_GPIO_READ_DOWN();
    flash_import_endurance_protect();
    (void)flash_import_set_line_mode(old_line_mode);
    rtos_enable_int(int_status);

    if (os_memcmp(s_endurance_read_buf, buf, size) != 0) {
        s_endurance_stats.data_write_error_cnt++;
        return BK_FAIL;
    }

    return BK_OK;
}
#endif

static void flash_import_endurance_gpio_init(void)
{
    const gpio_id_t *pins = (const gpio_id_t *)&s_endurance_gpio_cfg;
    uint32_t i = 0;

    for (i = 0; i < FLASH_IMPORT_END_GPIO_PIN_CNT; i++) {
        (void)bk_gpio_enable_output(pins[i]);
        (void)bk_gpio_set_output_low(pins[i]);
    }
}

static bk_err_t flash_import_endurance_prepare_layout(void)
{
    const bk_logic_partition_t *part = NULL;

    part = bk_flash_partition_get_info(FLASH_IMPORT_FLASH_TEST_PART_ID);
    if (part == NULL) {
        part = bk_flash_partition_get_info_by_name(FLASH_IMPORT_FLASH_TEST_PART_NAME);
    }
    if (part == NULL || part->partition_length < FLASH_IMPORT_ENDURANCE_LAYOUT_BYTES) {
        os_printf("endurance: %s invalid\r\n", FLASH_IMPORT_FLASH_TEST_PART_NAME);
        return BK_FAIL;
    }

    if ((part->partition_start_addr & 0xFFFU) != 0U) {
        os_printf("endurance: partition align err\r\n");
        return BK_FAIL;
    }

    s_endurance_stats_backup_addr = part->partition_start_addr;
    s_endurance_stats_main_addr = part->partition_start_addr + FLASH_IMPORT_ENDURANCE_MAIN_OFFSET;
    s_endurance_data_addr = part->partition_start_addr + FLASH_IMPORT_ENDURANCE_DATA_OFFSET;
    return BK_OK;
}

static bk_err_t flash_import_endurance_restore_history_state(void)
{
    static bool first_restore_attempt = true;
    flash_import_endurance_stats_t main_stats = {0};
    flash_import_endurance_stats_t backup_stats = {0};
    uint32_t crc = 0;
    bool main_valid = false;
    bool backup_valid = false;

    flash_import_endurance_read(s_endurance_stats_main_addr, (uint8_t *)&main_stats, sizeof(main_stats));
    crc = crc32_calc(0, &main_stats, sizeof(main_stats) - sizeof(main_stats.crc));
    if ((main_stats.crc != FLASH_IMPORT_ENDURANCE_CRC_INVALID) && (crc == main_stats.crc)) {
        main_valid = true;
    }

    flash_import_endurance_read(s_endurance_stats_backup_addr, (uint8_t *)&backup_stats, sizeof(backup_stats));
    crc = crc32_calc(0, &backup_stats, sizeof(backup_stats) - sizeof(backup_stats.crc));
    if ((backup_stats.crc != FLASH_IMPORT_ENDURANCE_CRC_INVALID) && (crc == backup_stats.crc)) {
        backup_valid = true;
    }

    if (!main_valid && !backup_valid && !first_restore_attempt) {
        return BK_FAIL;
    }

    if (main_valid) {
        os_memcpy(&s_endurance_stats, &main_stats, sizeof(s_endurance_stats));
    } else if (backup_valid) {
        os_memcpy(&s_endurance_stats, &backup_stats, sizeof(s_endurance_stats));
    } else {
        first_restore_attempt = false;
        if (flash_import_endurance_erase(s_endurance_stats_main_addr) != BK_OK) {
            os_printf("endurance main erase verify err %x\r\n", s_endurance_stats_main_addr);
        }
        if (flash_import_endurance_erase(s_endurance_stats_backup_addr) != BK_OK) {
            os_printf("endurance backup erase verify err %x\r\n", s_endurance_stats_backup_addr);
        }
        os_memset(&s_endurance_stats, 0, sizeof(s_endurance_stats));
    }

    return BK_OK;
}

static bk_err_t flash_import_endurance_test_region_init(void)
{
    bk_err_t ret = BK_OK;

    ret = flash_import_endurance_prepare_layout();
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_endurance_restore_history_state();
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

static bk_err_t flash_import_endurance_buffers_init(void)
{
    s_endurance_read_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
    if (s_endurance_read_buf == NULL) {
        os_printf("endurance buf alloc fail\r\n");
        return BK_FAIL;
    }

    s_endurance_write_buf = (uint8_t *)os_malloc(FLASH_IMPORT_SECTOR_SIZE);
    if (s_endurance_write_buf == NULL) {
        os_printf("endurance write buf alloc fail\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

static void flash_import_endurance_buffers_deinit(void)
{
    if (s_endurance_read_buf != NULL) {
        os_free(s_endurance_read_buf);
        s_endurance_read_buf = NULL;
    }

    if (s_endurance_write_buf != NULL) {
        os_free(s_endurance_write_buf);
        s_endurance_write_buf = NULL;
    }
}

static bk_err_t flash_import_endurance_init(void)
{
    bk_err_t ret = BK_OK;

    flash_import_context_save(&s_endurance_saved_ctx);
    s_endurance_context_saved = true;
    flash_import_endurance_gpio_init();

    ret = flash_import_endurance_buffers_init();
    if (ret != BK_OK) {
        return ret;
    }

    ret = flash_import_endurance_test_region_init();
    if (ret != BK_OK) {
        return ret;
    }

    if (bk_flash_set_clk_freq(FLASH_CLK_FREQ_80M_HZ) != BK_OK) {
        os_printf("endurance set clk 80M fail\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

static void flash_import_endurance_update_crc(void)
{
    s_endurance_stats.crc = crc32_calc(0,
                                       &s_endurance_stats,
                                       sizeof(s_endurance_stats) - sizeof(s_endurance_stats.crc));
}

static void flash_import_endurance_update_total_error_cnt(void)
{
    s_endurance_stats.total_erase_error_cnt = s_endurance_stats.data_erase_error_cnt
                                            + s_endurance_stats.main_erase_error_cnt
                                            + s_endurance_stats.backup_erase_error_cnt;
    s_endurance_stats.total_write_error_cnt = s_endurance_stats.data_write_error_cnt
                                            + s_endurance_stats.main_write_error_cnt
                                            + s_endurance_stats.backup_write_error_cnt;
}

static void flash_import_endurance_save_stats_region(uint32_t addr)
{
    flash_import_endurance_stats_t stats_snapshot = {0};

    if (flash_import_endurance_erase(addr) != BK_OK) {
        os_printf("endurance stats erase verify err %x\r\n", addr);
    }
    flash_import_endurance_update_total_error_cnt();
    flash_import_endurance_update_crc();
    os_memcpy(&stats_snapshot, &s_endurance_stats, sizeof(stats_snapshot));
    if (flash_import_endurance_write(addr, (uint8_t *)&stats_snapshot, sizeof(stats_snapshot)) != BK_OK) {
        os_printf("endurance stats write verify err %x\r\n", addr);
    }
}

static void flash_import_endurance_save_stats(flash_import_endurance_save_policy_t policy)
{
    switch (policy) {
    case FLASH_IMPORT_ENDURANCE_SAVE_BOTH:
        flash_import_endurance_save_stats_region(s_endurance_stats_main_addr);
        flash_import_endurance_save_stats_region(s_endurance_stats_backup_addr);
        return;
    case FLASH_IMPORT_ENDURANCE_SAVE_SCHEDULED:
        break;
    }

    if ((s_endurance_stats.data_total_cnt % FLASH_IMPORT_ENDURANCE_MAIN_SAVE_INTERVAL) == 0U) {
        flash_import_endurance_save_stats_region(s_endurance_stats_main_addr);
    }
    if ((s_endurance_stats.data_total_cnt % FLASH_IMPORT_ENDURANCE_BACKUP_SAVE_INTERVAL) == 0U) {
        flash_import_endurance_save_stats_region(s_endurance_stats_backup_addr);
    }
}

static bk_err_t flash_import_endurance_erase_verify(uint32_t addr)
{
    if (flash_import_endurance_erase(addr) != BK_OK) {
        os_printf("Endurance test: erase verify err 0x%x\r\n", addr);
        return BK_FAIL;
    }

#if CONFIG_FLASH_IMPORT_TEST_WITH_CBUS
    uint32_t cbus_addr = FLASH_PHY2VIRTUAL(addr);
    uint32_t phy_end_addr = FLASH_VIRTUAL2PHY(cbus_addr + FLASH_IMPORT_SECTOR_SIZE);
    uint32_t phy_len = phy_end_addr - addr;
    uint32_t next_sector_addr = (phy_end_addr - 1U) & ~(FLASH_IMPORT_SECTOR_SIZE - 1U);

    if ((phy_len > FLASH_IMPORT_SECTOR_SIZE)
        && (phy_len < (FLASH_IMPORT_SECTOR_SIZE * 2U))) {
        if (flash_import_endurance_erase(next_sector_addr) != BK_OK) {
            os_printf("Endurance test: erase verify err 0x%x\r\n", next_sector_addr);
            return BK_FAIL;
        }
    }
#endif

    return BK_OK;
}

static bk_err_t flash_import_endurance_write_verify(uint32_t addr)
{
    bk_err_t ret = BK_OK;
#if CONFIG_FLASH_IMPORT_TEST_WITH_CBUS
    ret = flash_import_endurance_write_cbus(addr, s_endurance_write_buf, FLASH_IMPORT_SECTOR_SIZE);
#else
    ret = flash_import_endurance_write(addr, s_endurance_write_buf, FLASH_IMPORT_SECTOR_SIZE);
#endif

    if (ret != BK_OK) {
        os_printf("Endurance test: write verify err 0x%x\r\n", addr);
        return BK_FAIL;
    }
    return BK_OK;
}

static void flash_import_endurance_show_stats_region(const char *name, uint32_t addr)
{
    flash_import_endurance_stats_t stats = {0};
    uint32_t crc = 0;

    flash_import_endurance_read(addr, (uint8_t *)&stats, sizeof(stats));
    crc = crc32_calc(0, &stats, sizeof(stats) - sizeof(stats.crc));

    os_printf("%s:\r\n", name);
    os_printf("  data_total_cnt: %d\r\n", (int32_t)stats.data_total_cnt);
    os_printf("  data_erase_error_cnt: %d\r\n", (int32_t)stats.data_erase_error_cnt);
    os_printf("  data_write_error_cnt: %d\r\n", (int32_t)stats.data_write_error_cnt);
    os_printf("  data_erase_op_cnt: %d\r\n", (int32_t)stats.data_erase_op_cnt);
    os_printf("  data_write_op_cnt: %d\r\n", (int32_t)stats.data_write_op_cnt);
    os_printf("  main_erase_op_cnt: %d\r\n", (int32_t)stats.main_erase_op_cnt);
    os_printf("  main_write_op_cnt: %d\r\n", (int32_t)stats.main_write_op_cnt);
    os_printf("  main_erase_error_cnt: %d\r\n", (int32_t)stats.main_erase_error_cnt);
    os_printf("  main_write_error_cnt: %d\r\n", (int32_t)stats.main_write_error_cnt);
    os_printf("  backup_erase_op_cnt: %d\r\n", (int32_t)stats.backup_erase_op_cnt);
    os_printf("  backup_write_op_cnt: %d\r\n", (int32_t)stats.backup_write_op_cnt);
    os_printf("  backup_erase_error_cnt: %d\r\n", (int32_t)stats.backup_erase_error_cnt);
    os_printf("  backup_write_error_cnt: %d\r\n", (int32_t)stats.backup_write_error_cnt);
    os_printf("  global_protect_cnt: %d\r\n", (int32_t)stats.global_protect_cnt);
    os_printf("  global_unprotect_cnt: %d\r\n", (int32_t)stats.global_unprotect_cnt);
    os_printf("  current crc: %x - calc:%x\r\n", stats.crc, crc);

    if (stats.crc == FLASH_IMPORT_ENDURANCE_CRC_INVALID) {
        os_printf("  status: not initialized\r\n");
    } else if (crc != stats.crc) {
        os_printf("  status: crc error\r\n");
    } else {
        os_printf("  status: valid\r\n");
    }
}

static void flash_import_endurance_deinit(void)
{
    const gpio_id_t *pins = (const gpio_id_t *)&s_endurance_gpio_cfg;
    uint32_t i = 0;

    if (s_endurance_context_saved) {
        flash_import_context_restore(&s_endurance_saved_ctx);
        s_endurance_context_saved = false;
    }

    for (i = 0; i < FLASH_IMPORT_END_GPIO_PIN_CNT; i++) {
        (void)bk_gpio_set_output_low(pins[i]);
    }

    flash_import_endurance_buffers_deinit();
    s_endurance_task_handle = NULL;
    rtos_delete_thread(NULL);
}

static void flash_import_endurance_fill_write_buf(uint32_t loop_count)
{
    uint8_t loop_byte = (uint8_t)(loop_count & 0xFFU);
    uint8_t loop_byte_hi = (uint8_t)((loop_count >> 8) & 0xFFU);

    for (uint32_t i = 0; i < FLASH_IMPORT_SECTOR_SIZE; i++) {
        uint8_t group_val = (uint8_t)((i / 16U) + loop_byte);

        s_endurance_write_buf[i] = group_val ^ loop_byte_hi;
    }
}

static void flash_import_endurance_prepare_read_only_region(void)
{
    for (uint32_t i = 0; i < FLASH_IMPORT_SECTOR_SIZE; i++) {
        uint8_t group_val = (uint8_t)((i / 16U));

        s_endurance_write_buf[i] = group_val;
    }

    (void)flash_import_endurance_erase_verify(s_endurance_data_addr);
    (void)flash_import_endurance_write_verify(s_endurance_data_addr);
}

static bk_err_t flash_import_endurance_read_verify_data_region(void)
{
    (void)flash_import_read_bytes(s_endurance_read_buf,
                                   s_endurance_data_addr,
                                   FLASH_IMPORT_SECTOR_SIZE);
    s_endurance_read_op_cnt++;
    if (os_memcmp(s_endurance_read_buf, s_endurance_write_buf, FLASH_IMPORT_SECTOR_SIZE) != 0) {
        return BK_FAIL;
    }

    return BK_OK;
}

static void flash_import_endurance_print_combined_stats(void)
{
    os_printf("Endurance test: total_count %u, erase_errors %u, write_errors %u, reads %u, read_errors %u\r\n",
              s_endurance_stats.data_total_cnt,
              s_endurance_stats.data_erase_error_cnt,
              s_endurance_stats.data_write_error_cnt,
              s_endurance_read_op_cnt,
              s_endurance_read_error_cnt);
}

static bool flash_import_endurance_erase_write_loop(void)
{
    uint32_t session_loop = 0;

    while (session_loop < FLASH_IMPORT_ENDURANCE_LOOP_LIMIT) {
        session_loop++;
        flash_import_endurance_fill_write_buf(s_endurance_stats.data_total_cnt + 1U);
        (void)flash_import_endurance_erase_verify(s_endurance_data_addr);
        (void)flash_import_endurance_write_verify(s_endurance_data_addr);

        if (s_endurance_stop_requested) {
            flash_import_endurance_save_stats(FLASH_IMPORT_ENDURANCE_SAVE_BOTH);
            return false;
        }

        s_endurance_stats.data_total_cnt++;
        flash_import_endurance_save_stats(FLASH_IMPORT_ENDURANCE_SAVE_SCHEDULED);

        if ((session_loop % FLASH_IMPORT_ENDURANCE_LOG_INTERVAL) == 0U) {
            os_printf("Endurance test: session_loop %u, total_count: %u, erase_errors: %u, write_errors: %u\r\n",
                      session_loop,
                      s_endurance_stats.data_total_cnt,
                      s_endurance_stats.data_erase_error_cnt,
                      s_endurance_stats.data_write_error_cnt);
        }
    }

    flash_import_endurance_save_stats(FLASH_IMPORT_ENDURANCE_SAVE_BOTH);
    os_printf("Endurance test: entering read-only phase\r\n");
    return true;
}

static void flash_import_endurance_read_only_loop(void)
{
    flash_import_endurance_prepare_read_only_region();

    while (1) {
        if (s_endurance_stop_requested) {
            break;
        }

        if (flash_import_endurance_read_verify_data_region() != BK_OK) {
            s_endurance_read_error_cnt++;
        }

        if ((s_endurance_read_op_cnt % FLASH_IMPORT_ENDURANCE_READ_LOG_INTERVAL) == 0U) {
            flash_import_endurance_print_combined_stats();
        }
    }
}

static void flash_import_endurance_main_loop(void)
{
    bool session_complete = false;

    session_complete = flash_import_endurance_erase_write_loop();

    if (!s_endurance_stop_requested && session_complete) {
        flash_import_endurance_read_only_loop();
    }

    flash_import_endurance_print_combined_stats();
}

static void flash_import_endurance_thread(beken_thread_arg_t arg)
{
    (void)arg;

    if (flash_import_endurance_init() == BK_OK) {
        flash_import_endurance_main_loop();
    }

    flash_import_endurance_deinit();
}

void flash_import_case_endurance_start(void)
{
    bk_err_t ret = BK_OK;

    if (s_endurance_task_handle != NULL) {
        return;
    }

    s_endurance_stop_requested = false;
    ret = rtos_create_thread(&s_endurance_task_handle,
                             4,
                             "flash_import_endurance",
                             (beken_thread_function_t)flash_import_endurance_thread,
                             4096,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK) {
        os_printf("Create endurance thread failed: %d\r\n", ret);
        return;
    }
}

void flash_import_case_endurance_stop(void)
{
    if (s_endurance_task_handle != NULL) {
        os_printf("Requesting endurance test to stop...\r\n");
        s_endurance_stop_requested = true;
    } else {
        os_printf("Endurance test is not running\r\n");
    }
}

void flash_import_case_endurance_show(void)
{
    if (flash_import_endurance_prepare_layout() != BK_OK) {
        return;
    }

    os_printf("\r\nBuild_info:%s_%s\r\n", __DATE__, __TIME__);
    os_printf("flash id:%lx\r\n", bk_flash_get_id());
    os_printf("endurance part:%s\r\n", FLASH_IMPORT_FLASH_TEST_PART_NAME);
    os_printf("stats main:0x%08x backup:0x%08x data:0x%08x\r\n",
              s_endurance_stats_main_addr,
              s_endurance_stats_backup_addr,
              s_endurance_data_addr);
    os_printf("endurance task: %s\r\n",
              (s_endurance_task_handle != NULL) ? "running" : "stopped");
    flash_import_endurance_show_stats_region("main stats", s_endurance_stats_main_addr);
    flash_import_endurance_show_stats_region("backup stats", s_endurance_stats_backup_addr);
}
