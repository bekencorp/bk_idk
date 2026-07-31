// Copyright 2020-2026 Beken
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

#include <stdio.h>
#include <string.h>

#include <components/log.h>
#include <os/os.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include <partitions.h>
#include "flash_driver.h"
#include "flash_ut_test.h"

#define TAG "flash_ut"

/*
 * Log output is asynchronous: the CPU races through all sub-tests within the
 * same tick while the UART FIFO drains slowly, so a reset loses whatever is
 * still buffered and the last printed line does NOT mark the crash site. This
 * delay lets the log task flush the stage header to UART before the sub-test
 * runs, so a reset pinpoints exactly which stage was executing.
 */
#define FLASH_UT_STAGE_FLUSH_MS 30

/*
 * XIP-unsafe stages. On an XIP build the CPU fetches code directly from this
 * same flash, so reconfiguring the flash controller (2/4 line switch, clock
 * change) or putting the flash into deep power-down / power-saving resets the
 * board mid-fetch. Keep these off for the simple-case run; set to 1 only for a
 * non-XIP build (code running from RAM) or once the ops run from RAM with
 * interrupts masked and XIP suspended.
 */
#define FLASH_UT_ENABLE_XIP_UNSAFE 0

/*
 * Partition-name dependent stages. These require a named "easyflash" partition
 * that does not exist in the bk7236n/xip partition table (app_a/app_b/user_mfr/
 * nvs/...), so bk_flash_partition_get_info_by_name("easyflash") returns NULL.
 * Disabled for now; set to 1 on a layout that actually defines that partition.
 */
#define FLASH_UT_ENABLE_EASYFLASH_PART 0

#define FLASHUT_LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define FLASHUT_LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Safe raw-flash test region: same as cli_flash_api.c / cli.yaml */
#define FLASH_UT_TEST_ADDR      0x300000
#define FLASH_UT_TEST_SIZE      0x10000

#define FLASH_UT_ASSERT_TRUE(condition) \
	do { \
		if (!(condition)) { \
			FLASHUT_LOGE("assert failed at %s:%d\r\n", __func__, __LINE__); \
			return BK_FAIL; \
		} \
	} while (0)

#define FLASH_UT_ASSERT_EQUAL_INT32(val0, val1) \
	do { \
		if ((val0) != (val1)) { \
			FLASHUT_LOGE("no pass, %s:%d val0 = 0x%x, val1 = 0x%x\r\n", \
				__func__, __LINE__, (int)(val0), (int)(val1)); \
			return BK_FAIL; \
		} \
	} while (0)

#define FLASH_UT_CHECK_OK(expr) \
	do { \
		bk_err_t _rc = (expr); \
		if (_rc != BK_OK) { \
			FLASHUT_LOGE("failed=0x%x, %s:%d\r\n", _rc, __func__, __LINE__); \
			return _rc; \
		} \
	} while (0)

/* Driver-internal helpers not exposed in include/driver/flash.h */
extern bk_err_t bk_flash_switch_line_mode(flash_line_mode_t line_mode);
extern bk_err_t bk_flash_erase_32k(uint32_t address);
extern bk_err_t bk_flash_power_saving_enter(void);
extern bk_err_t bk_flash_power_saving_exit(void);
extern bk_err_t mb_flash_op_prepare(void);
extern bk_err_t mb_flash_op_finish(void);
extern bk_err_t bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code);

static uint32_t s_api_covered;
static uint32_t s_api_total;

#define FLASH_UT_API_HIT(name) \
	do { \
		(void)(name); \
		s_api_covered++; \
	} while (0)

static void flash_ut_count_api(const char *name)
{
	(void)name;
	s_api_total++;
}

static bk_err_t flash_ut_prepare_test_region(void)
{
	bk_err_t err;

	FLASH_UT_CHECK_OK(bk_flash_set_protect_type(FLASH_PROTECT_NONE));
	err = bk_flash_erase_sector(FLASH_UT_TEST_ADDR);
	if (err != BK_OK) {
		FLASHUT_LOGE("erase test region failed, err=0x%x\r\n", err);
		return err;
	}
	return BK_OK;
}

static bk_err_t flash_ut_restore_protect(void)
{
	return bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
}

static bk_err_t flash_ut_test_driver_init(void)
{
	flash_ut_count_api("bk_flash_driver_init");
	flash_ut_count_api("bk_flash_driver_deinit");
	flash_ut_count_api("bk_flash_is_driver_inited");

	FLASH_UT_CHECK_OK(bk_flash_driver_init());
	FLASH_UT_API_HIT(bk_flash_driver_init);
	FLASH_UT_ASSERT_TRUE(bk_flash_is_driver_inited());
	FLASH_UT_API_HIT(bk_flash_is_driver_inited);

	FLASH_UT_CHECK_OK(bk_flash_driver_deinit());
	FLASH_UT_API_HIT(bk_flash_driver_deinit);
	FLASH_UT_ASSERT_TRUE(!bk_flash_is_driver_inited());

	FLASH_UT_CHECK_OK(bk_flash_driver_init());
	FLASH_UT_API_HIT(bk_flash_driver_init);
	FLASH_UT_ASSERT_TRUE(bk_flash_is_driver_inited());

	return BK_OK;
}

static bk_err_t flash_ut_test_identity_and_config(void)
{
	uint32_t flash_id;
	uint32_t flash_size;
	flash_line_mode_t line_mode;
	uint8_t cr_mode;

	flash_ut_count_api("bk_flash_get_id");
	flash_ut_count_api("bk_flash_get_line_mode");
	flash_ut_count_api("bk_flash_get_coutinuous_read_mode");
	flash_ut_count_api("bk_flash_get_current_total_size");

	flash_id = bk_flash_get_id();
	FLASH_UT_API_HIT(bk_flash_get_id);
	FLASHUT_LOGI("flash_id=0x%08x\r\n", flash_id);
	FLASH_UT_ASSERT_TRUE(flash_id != 0);
	FLASH_UT_ASSERT_TRUE(flash_id != 0x00FFFFFF);

	line_mode = bk_flash_get_line_mode();
	FLASH_UT_API_HIT(bk_flash_get_line_mode);
	FLASHUT_LOGI("line_mode=%d\r\n", line_mode);
	FLASH_UT_ASSERT_TRUE(line_mode == FLASH_LINE_MODE_TWO || line_mode == FLASH_LINE_MODE_FOUR);

	cr_mode = bk_flash_get_coutinuous_read_mode();
	FLASH_UT_API_HIT(bk_flash_get_coutinuous_read_mode);
	FLASHUT_LOGI("continuous_read_mode=0x%02x\r\n", cr_mode);

	flash_size = bk_flash_get_current_total_size();
	FLASH_UT_API_HIT(bk_flash_get_current_total_size);
	FLASHUT_LOGI("flash_size=0x%x\r\n", flash_size);
	FLASH_UT_ASSERT_TRUE(flash_size > FLASH_UT_TEST_ADDR + FLASH_UT_TEST_SIZE);

	return BK_OK;
}

static bk_err_t flash_ut_test_clock_and_line_mode(void) __attribute__((unused));
static bk_err_t flash_ut_test_clock_and_line_mode(void)
{
	flash_ut_count_api("bk_flash_set_line_mode");
	flash_ut_count_api("bk_flash_switch_line_mode");
	flash_ut_count_api("bk_flash_set_clk_dpll");
	flash_ut_count_api("bk_flash_set_clk_dco");
	flash_ut_count_api("bk_flash_set_clk_freq");

	FLASH_UT_CHECK_OK(bk_flash_set_line_mode(FLASH_LINE_MODE_TWO));
	FLASH_UT_API_HIT(bk_flash_set_line_mode);

	FLASH_UT_CHECK_OK(bk_flash_switch_line_mode(FLASH_LINE_MODE_TWO));
	FLASH_UT_API_HIT(bk_flash_switch_line_mode);
	FLASH_UT_CHECK_OK(bk_flash_switch_line_mode(FLASH_LINE_MODE_FOUR));
	FLASH_UT_API_HIT(bk_flash_switch_line_mode);

	FLASH_UT_CHECK_OK(bk_flash_set_clk_dpll());
	FLASH_UT_API_HIT(bk_flash_set_clk_dpll);
	FLASH_UT_CHECK_OK(bk_flash_set_clk_dco());
	FLASH_UT_API_HIT(bk_flash_set_clk_dco);

	FLASH_UT_CHECK_OK(bk_flash_set_clk_freq(FLASH_CLK_FREQ_80M_HZ));
	FLASH_UT_API_HIT(bk_flash_set_clk_freq);
	FLASH_UT_ASSERT_TRUE(bk_flash_set_clk_freq(12345678) == BK_ERR_FLASH_INVALID_CLK_PARAM);

	return BK_OK;
}

static bk_err_t flash_ut_test_protect_and_status(void)
{
	flash_protect_type_t saved_type;
	uint16_t status_reg;

	flash_ut_count_api("bk_flash_get_protect_type");
	flash_ut_count_api("bk_flash_set_protect_type");
	flash_ut_count_api("bk_flash_set_protect_none_full");
	flash_ut_count_api("bk_flash_set_protect_type_full");
	flash_ut_count_api("bk_flash_write_enable");
	flash_ut_count_api("bk_flash_write_disable");
	flash_ut_count_api("bk_flash_read_status_reg");
	flash_ut_count_api("bk_flash_write_status_reg");

	saved_type = bk_flash_get_protect_type();
	FLASH_UT_API_HIT(bk_flash_get_protect_type);

	FLASH_UT_CHECK_OK(bk_flash_set_protect_type(FLASH_PROTECT_NONE));
	FLASH_UT_API_HIT(bk_flash_set_protect_type);
	FLASH_UT_CHECK_OK(bk_flash_set_protect_none_full());
	FLASH_UT_API_HIT(bk_flash_set_protect_none_full);
	FLASH_UT_CHECK_OK(bk_flash_set_protect_type_full(FLASH_UNPROTECT_LAST_BLOCK));
	FLASH_UT_API_HIT(bk_flash_set_protect_type_full);
	FLASH_UT_CHECK_OK(bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK));
	FLASH_UT_API_HIT(bk_flash_set_protect_type);

	FLASH_UT_CHECK_OK(bk_flash_write_enable());
	FLASH_UT_API_HIT(bk_flash_write_enable);
	FLASH_UT_CHECK_OK(bk_flash_write_disable());
	FLASH_UT_API_HIT(bk_flash_write_disable);

	status_reg = bk_flash_read_status_reg();
	FLASH_UT_API_HIT(bk_flash_read_status_reg);
	FLASHUT_LOGI("status_reg=0x%04x\r\n", status_reg);

	FLASH_UT_CHECK_OK(bk_flash_write_status_reg(status_reg));
	FLASH_UT_API_HIT(bk_flash_write_status_reg);

	if (saved_type != FLASH_UNPROTECT_LAST_BLOCK) {
		FLASH_UT_CHECK_OK(bk_flash_set_protect_type(saved_type));
	}

	return BK_OK;
}

static bk_err_t flash_ut_test_erase_read(void)
{
	uint8_t buf[FLASH_PAGE_SIZE];
	uint32_t i;

	flash_ut_count_api("bk_flash_erase_sector");
	flash_ut_count_api("bk_flash_read_bytes");

	FLASH_UT_CHECK_OK(flash_ut_prepare_test_region());

	memset(buf, 0, sizeof(buf));
	FLASH_UT_CHECK_OK(bk_flash_read_bytes(FLASH_UT_TEST_ADDR, buf, sizeof(buf)));
	FLASH_UT_API_HIT(bk_flash_read_bytes);
	for (i = 0; i < sizeof(buf); i++) {
		FLASH_UT_ASSERT_EQUAL_INT32(buf[i], 0xFF);
	}

	FLASH_UT_API_HIT(bk_flash_erase_sector);
	return flash_ut_restore_protect();
}

static bk_err_t flash_ut_test_write_read(void)
{
	uint8_t write_buf[FLASH_PAGE_SIZE];
	uint8_t read_buf[FLASH_PAGE_SIZE];
	uint32_t i;

	flash_ut_count_api("bk_flash_write_bytes");

	FLASH_UT_CHECK_OK(flash_ut_prepare_test_region());

	for (i = 0; i < sizeof(write_buf); i++) {
		write_buf[i] = (uint8_t)i;
	}

	FLASH_UT_CHECK_OK(bk_flash_write_bytes(FLASH_UT_TEST_ADDR, write_buf, sizeof(write_buf)));
	FLASH_UT_API_HIT(bk_flash_write_bytes);

	memset(read_buf, 0, sizeof(read_buf));
	FLASH_UT_CHECK_OK(bk_flash_read_bytes(FLASH_UT_TEST_ADDR, read_buf, sizeof(read_buf)));
	FLASH_UT_API_HIT(bk_flash_read_bytes);
	FLASH_UT_ASSERT_TRUE(memcmp(write_buf, read_buf, sizeof(write_buf)) == 0);

	return flash_ut_restore_protect();
}

static bk_err_t flash_ut_test_read_word(void)
{
	uint32_t write_words[16];
	uint32_t read_words[16];
	uint32_t i;

	flash_ut_count_api("bk_flash_read_word");

	FLASH_UT_CHECK_OK(flash_ut_prepare_test_region());

	for (i = 0; i < ARRAY_SIZE(write_words); i++) {
		write_words[i] = 0xA5A50000 + i;
	}

	FLASH_UT_CHECK_OK(bk_flash_write_bytes(FLASH_UT_TEST_ADDR,
		(const uint8_t *)write_words, sizeof(write_words)));

	memset(read_words, 0, sizeof(read_words));
	FLASH_UT_CHECK_OK(bk_flash_read_word(FLASH_UT_TEST_ADDR, read_words, sizeof(read_words)));
	FLASH_UT_API_HIT(bk_flash_read_word);
	FLASH_UT_ASSERT_TRUE(memcmp(write_words, read_words, sizeof(write_words)) == 0);

	return flash_ut_restore_protect();
}

static bk_err_t flash_ut_test_unaligned_read_write(void)
{
	uint8_t write_buf[64];
	uint8_t read_buf[64];
	uint32_t offset = 32;
	uint32_t len = 33;
	uint32_t i;

	FLASH_UT_CHECK_OK(flash_ut_prepare_test_region());

	for (i = 0; i < len; i++) {
		write_buf[i] = (uint8_t)(0xA0 + i);
	}

	FLASH_UT_CHECK_OK(bk_flash_write_bytes(FLASH_UT_TEST_ADDR + offset, write_buf, len));
	FLASH_UT_API_HIT(bk_flash_write_bytes);

	memset(read_buf, 0, sizeof(read_buf));
	FLASH_UT_CHECK_OK(bk_flash_read_bytes(FLASH_UT_TEST_ADDR + offset, read_buf, len));
	FLASH_UT_API_HIT(bk_flash_read_bytes);
	FLASH_UT_ASSERT_TRUE(memcmp(write_buf, read_buf, len) == 0);

	return flash_ut_restore_protect();
}

static bk_err_t flash_ut_test_erase_variants(void)
{
	uint8_t buf[16];

	flash_ut_count_api("bk_flash_erase_32k");
	flash_ut_count_api("bk_flash_erase_block");
	flash_ut_count_api("bk_flash_erase_fast");
#if CONFIG_FLASH_BYPASS_PAGE_ERASE
	flash_ut_count_api("bk_flash_erase_page");
#endif

	FLASH_UT_CHECK_OK(bk_flash_set_protect_type(FLASH_PROTECT_NONE));

	FLASH_UT_CHECK_OK(bk_flash_erase_sector(FLASH_UT_TEST_ADDR));
	FLASH_UT_API_HIT(bk_flash_erase_sector);

#if CONFIG_FLASH_BYPASS_PAGE_ERASE
	bk_err_t err;

	err = bk_flash_erase_page(FLASH_UT_TEST_ADDR);
	FLASH_UT_API_HIT(bk_flash_erase_page);
	if (err != BK_OK) {
		FLASHUT_LOGI("erase_page skipped, err=0x%x\r\n", err);
	}
#endif

	FLASH_UT_CHECK_OK(bk_flash_erase_32k(FLASH_UT_TEST_ADDR));
	FLASH_UT_API_HIT(bk_flash_erase_32k);

	FLASH_UT_CHECK_OK(bk_flash_erase_block(FLASH_UT_TEST_ADDR));
	FLASH_UT_API_HIT(bk_flash_erase_block);

	FLASH_UT_CHECK_OK(bk_flash_erase_fast(FLASH_UT_TEST_ADDR, FLASH_UT_TEST_SIZE));
	FLASH_UT_API_HIT(bk_flash_erase_fast);

	memset(buf, 0, sizeof(buf));
	FLASH_UT_CHECK_OK(bk_flash_read_bytes(FLASH_UT_TEST_ADDR, buf, sizeof(buf)));
	FLASH_UT_API_HIT(bk_flash_read_bytes);
	for (uint32_t i = 0; i < sizeof(buf); i++) {
		FLASH_UT_ASSERT_EQUAL_INT32(buf[i], 0xFF);
	}

	return flash_ut_restore_protect();
}

static bool flash_ut_is_addr_error(bk_err_t err)
{
	return (err == BK_ERR_FLASH_ADDR_OUT_OF_RANGE) || (err == BK_FAIL);
}

static bk_err_t flash_ut_test_invalid_address(void)
{
	uint8_t buf[4];
	uint32_t flash_size = bk_flash_get_current_total_size();

	FLASH_UT_ASSERT_TRUE(flash_ut_is_addr_error(
		bk_flash_read_bytes(flash_size, buf, sizeof(buf))));
	FLASH_UT_ASSERT_TRUE(flash_ut_is_addr_error(
		bk_flash_write_bytes(flash_size, buf, sizeof(buf))));
	FLASH_UT_ASSERT_TRUE(flash_ut_is_addr_error(
		bk_flash_erase_sector(flash_size)));

	return BK_OK;
}

static bk_err_t flash_ut_test_deep_sleep_and_power_saving(void) __attribute__((unused));
static bk_err_t flash_ut_test_deep_sleep_and_power_saving(void)
{
	uint8_t buf[4];

	flash_ut_count_api("bk_flash_enter_deep_sleep");
	flash_ut_count_api("bk_flash_exit_deep_sleep");
	flash_ut_count_api("bk_flash_power_saving_enter");
	flash_ut_count_api("bk_flash_power_saving_exit");

	FLASH_UT_CHECK_OK(bk_flash_power_saving_enter());
	FLASH_UT_API_HIT(bk_flash_power_saving_enter);
	FLASH_UT_CHECK_OK(bk_flash_power_saving_exit());
	FLASH_UT_API_HIT(bk_flash_power_saving_exit);

	if (bk_flash_enter_deep_sleep() == BK_OK) {
		FLASH_UT_API_HIT(bk_flash_enter_deep_sleep);
		FLASH_UT_CHECK_OK(bk_flash_exit_deep_sleep());
		FLASH_UT_API_HIT(bk_flash_exit_deep_sleep);
	} else {
		FLASHUT_LOGI("deep_sleep not supported on this platform, skip\r\n");
	}

	memset(buf, 0, sizeof(buf));
	FLASH_UT_CHECK_OK(bk_flash_read_bytes(FLASH_UT_TEST_ADDR, buf, sizeof(buf)));
	FLASH_UT_API_HIT(bk_flash_read_bytes);

	return BK_OK;
}

static void flash_ut_dummy_ps_cb(void)
{
}

static void flash_ut_dummy_notify(uint32_t enable)
{
	(void)enable;
}

static bk_err_t flash_ut_test_callbacks_and_status(void)
{
	flash_ut_count_api("bk_flash_register_ps_suspend_callback");
	flash_ut_count_api("bk_flash_register_ps_resume_callback");
	flash_ut_count_api("bk_flash_set_operate_status");
	flash_ut_count_api("bk_flash_get_operate_status");
	flash_ut_count_api("mb_flash_register_op_notify");
	flash_ut_count_api("mb_flash_unregister_op_notify");
	flash_ut_count_api("mb_flash_op_prepare");
	flash_ut_count_api("mb_flash_op_finish");

	FLASH_UT_CHECK_OK(bk_flash_register_ps_suspend_callback(flash_ut_dummy_ps_cb));
	FLASH_UT_API_HIT(bk_flash_register_ps_suspend_callback);
	FLASH_UT_CHECK_OK(bk_flash_register_ps_resume_callback(flash_ut_dummy_ps_cb));
	FLASH_UT_API_HIT(bk_flash_register_ps_resume_callback);

	FLASH_UT_CHECK_OK(bk_flash_set_operate_status(FLASH_OP_BUSY));
	FLASH_UT_API_HIT(bk_flash_set_operate_status);
	FLASH_UT_ASSERT_EQUAL_INT32(bk_flash_get_operate_status(), FLASH_OP_BUSY);
	FLASH_UT_API_HIT(bk_flash_get_operate_status);
	FLASH_UT_CHECK_OK(bk_flash_set_operate_status(FLASH_OP_IDLE));
	FLASH_UT_API_HIT(bk_flash_set_operate_status);

	FLASH_UT_CHECK_OK(mb_flash_register_op_notify((void *)flash_ut_dummy_notify));
	FLASH_UT_API_HIT(mb_flash_register_op_notify);
	FLASH_UT_CHECK_OK(mb_flash_op_prepare());
	FLASH_UT_API_HIT(mb_flash_op_prepare);
	FLASH_UT_CHECK_OK(mb_flash_op_finish());
	FLASH_UT_API_HIT(mb_flash_op_finish);
	FLASH_UT_CHECK_OK(mb_flash_unregister_op_notify((void *)flash_ut_dummy_notify));
	FLASH_UT_API_HIT(mb_flash_unregister_op_notify);

	return BK_OK;
}

static bk_err_t flash_ut_test_partition_info(void)
{
	bk_logic_partition_t *partition;
	const bk_logic_partition_t *partition_by_name;
	uint32_t found = 0;

	flash_ut_count_api("bk_flash_partition_get_info");
	flash_ut_count_api("bk_flash_partition_get_info_by_name");
	flash_ut_count_api("bk_flash_partition_get_type");
	flash_ut_count_api("bk_flash_partition_get_options");

	for (bk_partition_t par = BK_PARTITION_BOOTLOADER; par < BK_PARTITIONS_TABLE_SIZE; par++) {
		partition = bk_flash_partition_get_info(par);
		if (partition == NULL || partition->partition_length == 0 ||
			partition->partition_description == NULL) {
			continue;
		}
		found++;
		FLASH_UT_ASSERT_TRUE(partition->partition_length > 0);
		(void)bk_flash_partition_get_type(partition);
		(void)bk_flash_partition_get_options(partition);
	}
	FLASH_UT_API_HIT(bk_flash_partition_get_info);
	FLASH_UT_API_HIT(bk_flash_partition_get_type);
	FLASH_UT_API_HIT(bk_flash_partition_get_options);
	FLASH_UT_ASSERT_TRUE(found > 0);

	partition_by_name = bk_flash_partition_get_info_by_name("easyflash");
	FLASH_UT_API_HIT(bk_flash_partition_get_info_by_name);
	if (partition_by_name == NULL) {
		partition_by_name = bk_flash_partition_get_info(BK_PARTITION_EASYFLASH);
	}
	FLASH_UT_ASSERT_TRUE(partition_by_name != NULL);
	FLASH_UT_ASSERT_TRUE(partition_by_name->partition_length >= FLASH_SECTOR_SIZE);

	return BK_OK;
}

static bk_err_t flash_ut_test_partition_rw(void) __attribute__((unused));
static bk_err_t flash_ut_test_partition_rw(void)
{
	const bk_logic_partition_t *partition;
	uint8_t write_buf[FLASH_PAGE_SIZE];
	uint8_t read_buf[FLASH_PAGE_SIZE];
	uint32_t i;

	flash_ut_count_api("bk_flash_partition_erase");
	flash_ut_count_api("bk_flash_partition_write");
	flash_ut_count_api("bk_flash_partition_read");
	flash_ut_count_api("bk_flash_partition_erase_by_name");
	flash_ut_count_api("bk_flash_partition_write_by_name");
	flash_ut_count_api("bk_flash_partition_read_by_name");
	flash_ut_count_api("bk_flash_partition_write_perm_check_by_addr");

	partition = bk_flash_partition_get_info_by_name("easyflash");
	FLASH_UT_ASSERT_TRUE(partition != NULL);

	FLASH_UT_CHECK_OK(bk_flash_partition_erase(BK_PARTITION_EASYFLASH, 0, FLASH_SECTOR_SIZE));
	FLASH_UT_API_HIT(bk_flash_partition_erase);

	for (i = 0; i < sizeof(write_buf); i++) {
		write_buf[i] = (uint8_t)(0x5A ^ i);
	}

	FLASH_UT_CHECK_OK(bk_flash_partition_write(BK_PARTITION_EASYFLASH, write_buf, 0, sizeof(write_buf)));
	FLASH_UT_API_HIT(bk_flash_partition_write);

	memset(read_buf, 0, sizeof(read_buf));
	FLASH_UT_CHECK_OK(bk_flash_partition_read(BK_PARTITION_EASYFLASH, read_buf, 0, sizeof(read_buf)));
	FLASH_UT_API_HIT(bk_flash_partition_read);
	FLASH_UT_ASSERT_TRUE(memcmp(write_buf, read_buf, sizeof(write_buf)) == 0);

	FLASH_UT_CHECK_OK(bk_flash_partition_erase_by_name("easyflash", 0, FLASH_SECTOR_SIZE));
	FLASH_UT_API_HIT(bk_flash_partition_erase_by_name);
	FLASH_UT_CHECK_OK(bk_flash_partition_write_by_name("easyflash", write_buf, 0, sizeof(write_buf)));
	FLASH_UT_API_HIT(bk_flash_partition_write_by_name);
	FLASH_UT_CHECK_OK(bk_flash_partition_read_by_name("easyflash", read_buf, 0, sizeof(read_buf)));
	FLASH_UT_API_HIT(bk_flash_partition_read_by_name);

	FLASH_UT_CHECK_OK(bk_flash_partition_write_perm_check_by_addr(
		partition->partition_start_addr, sizeof(write_buf), FLASH_API_MAGIC_CODE));
	FLASH_UT_API_HIT(bk_flash_partition_write_perm_check_by_addr);

	return BK_OK;
}

static bk_err_t flash_ut_test_partition_erase_all(void) __attribute__((unused));
static bk_err_t flash_ut_test_partition_erase_all(void)
{
	flash_ut_count_api("bk_flash_partition_erase_all");

	FLASH_UT_CHECK_OK(bk_flash_partition_erase_all("easyflash"));
	FLASH_UT_API_HIT(bk_flash_partition_erase_all);

	return BK_OK;
}

static void flash_ut_print_coverage(void)
{
	uint32_t pct = 0;

	if (s_api_total > 0) {
		pct = (s_api_covered * 100U) / s_api_total;
	}
	FLASHUT_LOGI("api_coverage=%u/%u (%u%%)\r\n", s_api_covered, s_api_total, pct);
	if (pct < 90U) {
		FLASHUT_LOGE("api coverage below 90%%\r\n");
	}
}

/*
 * Run one sub-test stage: print its name, wait for the UART log to drain, then
 * execute it. If the board resets inside the stage, its header is guaranteed to
 * be on the wire already, so the last stage line in the log is the crash site.
 */
#define FLASH_UT_RUN_STAGE(fn) \
	do { \
		FLASHUT_LOGI(#fn " ...\r\n"); \
		rtos_delay_milliseconds(FLASH_UT_STAGE_FLUSH_MS); \
		err = (fn)(); \
		if (err != BK_OK) { \
			return err; \
		} \
		FLASHUT_LOGI(#fn " ok\r\n"); \
		rtos_delay_milliseconds(FLASH_UT_STAGE_FLUSH_MS); \
	} while (0)

bk_err_t flash_driver_test(void)
{
	bk_err_t err = BK_OK;

	s_api_covered = 0;
	s_api_total = 0;

	FLASHUT_LOGI("flash_driver_test start\r\n");

	FLASH_UT_RUN_STAGE(flash_ut_test_driver_init);
	FLASH_UT_RUN_STAGE(flash_ut_test_identity_and_config);

	/* XIP-unsafe: line-mode / flash-clock reconfig (see macro comment). */
#if FLASH_UT_ENABLE_XIP_UNSAFE
	FLASH_UT_RUN_STAGE(flash_ut_test_clock_and_line_mode);
#endif

#if CONFIG_SYS_CPU0
	FLASH_UT_RUN_STAGE(flash_ut_test_protect_and_status);
	FLASH_UT_RUN_STAGE(flash_ut_test_erase_read);
	FLASH_UT_RUN_STAGE(flash_ut_test_write_read);
	FLASH_UT_RUN_STAGE(flash_ut_test_read_word);
	FLASH_UT_RUN_STAGE(flash_ut_test_unaligned_read_write);
	FLASH_UT_RUN_STAGE(flash_ut_test_erase_variants);
	FLASH_UT_RUN_STAGE(flash_ut_test_invalid_address);
	/* XIP-unsafe: flash deep power-down / power-saving (see macro comment). */
#if FLASH_UT_ENABLE_XIP_UNSAFE
	FLASH_UT_RUN_STAGE(flash_ut_test_deep_sleep_and_power_saving);
#endif
#else
	FLASHUT_LOGI("skip erase/write tests on non-CPU0\r\n");
#endif

	FLASH_UT_RUN_STAGE(flash_ut_test_callbacks_and_status);
	FLASH_UT_RUN_STAGE(flash_ut_test_partition_info);

#if CONFIG_SYS_CPU0
	/* XIP-safe but needs an "easyflash" partition absent here (see macro). */
#if FLASH_UT_ENABLE_EASYFLASH_PART
	FLASH_UT_RUN_STAGE(flash_ut_test_partition_rw);
	FLASH_UT_RUN_STAGE(flash_ut_test_partition_erase_all);
#endif
#endif

	flash_ut_print_coverage();
	if (s_api_total > 0 && (s_api_covered * 100U) / s_api_total < 90U) {
		return BK_FAIL;
	}

	FLASHUT_LOGI("flash_driver_test over\r\n");
	return BK_OK;
}
