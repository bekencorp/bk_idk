// Copyright 2024-2025 Beken
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

#include <components/log.h>
#include <components/system.h>
#include <os/mem.h>
#include <driver/flash_partition.h>
#include <driver/flash.h>
#include <driver/aon_rtc.h>
#include <modules/wifi_types.h>
#include <os/os.h>

#define RF_TEST_TAG "phy_test"
#define RF_TEST_LOGI(...) BK_LOGI(RF_TEST_TAG, ##__VA_ARGS__)
#define RF_TEST_LOGW(...) BK_LOGW(RF_TEST_TAG, ##__VA_ARGS__)
#define RF_TEST_LOGE(...) BK_LOGE(RF_TEST_TAG, ##__VA_ARGS__)
#define RF_TEST_LOGD(...) BK_LOGD(RF_TEST_TAG, ##__VA_ARGS__)

#if CONFIG_PHY_TEST

#if CONFIG_HTOL_RELIABILITY_TEST
/*
                                    user_config_partition
    0x3da000                                                                    0x3db000
        |------------------- 3072_B -------------------|---------- 1024_B ----------|
                    "save working time"                    "save reset num and type"
        |------------------------------- 4096_Byte --------------   ----------------|
*/
#define HTOL_RESET_RECORD_OFFSET    (3 * 1024)
#define HTOL_RESET_RECORD_LENGTH    (1 * 1024)

#define HTOL_SECEND_PER_DAY         (60 * 60 * 24)

#define HTOL_WORKDAY_RECORD_OFFSET  (0)
#define HTOL_WORKDAY_RECORD_LENGTH  (3 * 1024)

#define HTOL_TRX_11B_TARGET_POWER   (20.0)

typedef struct {
	uint8_t                reset_num;
	RESET_SOURCE_STATUS    reset_type;
} chip_reset_event_info_t;

static uint8_t htol_current_reset_num = 0;
static bool is_htol_session_header_recorded = false;

static bk_err_t htol_partition_read(uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
	if (pt == NULL) {
		return BK_ERR_NULL_PARAM;
	}

	return bk_flash_read_bytes(pt->partition_start_addr + offset, out_buffer, buffer_len);
}

static bk_err_t htol_partition_write(const uint8_t *in_buffer, uint32_t offset, uint32_t buffer_len)
{
	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
	if (pt == NULL) {
		return BK_ERR_NULL_PARAM;
	}

	return bk_flash_write_bytes(pt->partition_start_addr + offset, in_buffer, buffer_len);
}

static bk_err_t htol_record_reset_event(void)
{
	bk_err_t ret = BK_OK;
	chip_reset_event_info_t reset_event = {0};
	uint8_t *read_buf = (uint8_t *)os_malloc(HTOL_RESET_RECORD_LENGTH);
	if (read_buf == NULL) {
		RF_TEST_LOGW("%s malloc failed\r\n", __func__);
		return BK_ERR_NO_MEM;
	}
	reset_event.reset_type = bk_misc_get_reset_reason();
	RF_TEST_LOGI("reset_type = 0x%x\n", reset_event.reset_type);

	// the default value is 0xff in flash
	// to avoid (RESET_SOURCE_UNKNOWN == the default value in flash)
	if (reset_event.reset_type == RESET_SOURCE_UNKNOWN) {
		reset_event.reset_type = 0x1f;
	}

	ret = htol_partition_read(read_buf,
		HTOL_RESET_RECORD_OFFSET,
		HTOL_RESET_RECORD_LENGTH);
	if (ret != BK_OK) {
		goto out;
	}
	for (uint32_t i = 0; i < HTOL_RESET_RECORD_LENGTH;) {
		if ((read_buf[i] == 0xff) && (read_buf[i + 1] == 0xff)) {
			reset_event.reset_num = (i / 2) + 1;
			ret = htol_partition_write((uint8_t *)(&reset_event),
				HTOL_RESET_RECORD_OFFSET + i,
				sizeof(reset_event));
			if (ret != BK_OK) {
				goto out;
			}
			// Cache this boot session id for workday records.
			htol_current_reset_num = reset_event.reset_num;
			break;
		}
		i = i + 2;
	}

out:
	os_free(read_buf);
	return ret;
}

static uint32_t htol_test_working_days_delta = 0;
static uint32_t htol_get_working_days(void)
{
	static struct timeval htol_test_start_time   = {0};
	static uint32_t htol_test_start_days         = 0xffffffff;
	static uint32_t htol_test_working_days_now   = 0;
	static uint32_t htol_test_working_days_last  = 0;

	bk_rtc_gettimeofday(&htol_test_start_time, 0);

	htol_test_working_days_now   = htol_test_start_time.tv_sec / HTOL_SECEND_PER_DAY;

	if (htol_test_start_days == 0xffffffff) {
		htol_test_start_days = htol_test_working_days_now;
		htol_test_working_days_now = 0;
	} else {
		htol_test_working_days_now -= htol_test_start_days;
	}

	htol_test_working_days_delta = htol_test_working_days_now - htol_test_working_days_last;
	if (htol_test_working_days_delta >= 1) {
		RF_TEST_LOGW("SIGNIFY HTOL TEST GO %d days\n", htol_test_working_days_now);
		htol_test_working_days_last = htol_test_working_days_now;
	}

	return htol_test_working_days_now;
}

static bk_err_t htol_record_working_time(void)
{
	bk_err_t ret = BK_OK;
	uint32_t next_record_offset = 0;
	uint8_t *read_buf = (uint8_t *)os_malloc(HTOL_WORKDAY_RECORD_LENGTH);
	if (read_buf == NULL) {
		RF_TEST_LOGW("%s malloc failed\r\n", __func__);
		return BK_ERR_NO_MEM;
	}

	ret = htol_partition_read(read_buf,
		HTOL_WORKDAY_RECORD_OFFSET,
		HTOL_WORKDAY_RECORD_LENGTH);
	if (ret != BK_OK) {
		goto out;
	}

	// Scan the next writable 2-byte record slot.
	for (next_record_offset = 0; (next_record_offset + 1) < HTOL_WORKDAY_RECORD_LENGTH; next_record_offset += 2) {
		if ((read_buf[next_record_offset] == 0xff) && (read_buf[next_record_offset + 1] == 0xff)) {
			break;
		}
	}
	if ((next_record_offset + 1) >= HTOL_WORKDAY_RECORD_LENGTH) {
		RF_TEST_LOGW("workday area full\n");
		goto out;
	}

	// Write one session header: [0xA5, reset_num].
	if (!is_htol_session_header_recorded && (htol_current_reset_num != 0)) {
		uint8_t session_header[2] = {0xA5, htol_current_reset_num};
		ret = htol_partition_write(session_header,
			HTOL_WORKDAY_RECORD_OFFSET + next_record_offset,
			sizeof(session_header));
		if (ret != BK_OK) {
			goto out;
		}
		next_record_offset += 2;
		if ((next_record_offset + 1) >= HTOL_WORKDAY_RECORD_LENGTH) {
			is_htol_session_header_recorded = true;
			goto out;
		}
		is_htol_session_header_recorded = true;
	}

	uint8_t working_days = (uint8_t)htol_get_working_days();
	if (htol_test_working_days_delta >= 1) {
		// Append day record as [0x5A, day_num].
		uint8_t day_record[2] = {0x5A, working_days};
		ret = htol_partition_write(day_record,
			HTOL_WORKDAY_RECORD_OFFSET + next_record_offset,
			sizeof(day_record));
		if (ret != BK_OK) {
			goto out;
		}
	}

out:
	os_free(read_buf);
	return ret;
}

extern int do_rx_sensitivity(void *cmdtp, int flag, int argc, char *const argv[]);
extern int do_evm(void *cmdtp, int flag, int argc, char *const argv[]);
static bk_err_t bk_htol_test_trx_cycle(void)
{
	char *const txevm_start[]  = {"txevm",  "-b", "0", "-r", "1", "-c", "1", "-w", "0", "-t", "0"};
	char *const txevm_stop[]   = {"txevm",  "-e", "0"};
	char *const rxsens_start[] = {"rxsens", "-b", "0", "-c", "1", "-d", "2"};
	char *const rxsens_stop[]  = {"rxsens", "-e", "0"};

	RF_TEST_LOGI("================ ENTER SIGNIFY PRESSURE TEST ================\r\n");
	GPIO_UP(19);
	do_evm(NULL, 0, sizeof(txevm_start) / sizeof(char *), txevm_start);
	rtos_delay_milliseconds(100);
	do_evm(NULL, 0, sizeof(txevm_stop) / sizeof(char *), txevm_stop);

	GPIO_DOWN(19);
	do_rx_sensitivity(NULL, 0, sizeof(rxsens_start) / sizeof(char *), rxsens_start);
	rtos_delay_milliseconds(600);
	do_rx_sensitivity(NULL, 0, sizeof(rxsens_stop) / sizeof(char *), rxsens_stop);
	RF_TEST_LOGI("================ EXIT SIGNIFY PRESSURE TEST ================\r\n");

	return BK_OK;
}

/*
 * HTOL TEST(demo for Signify)
 *
 * STEP1: record chip_reset type and chip_reset nums;
 * STEP2: re-record working time when erery chip_reset;
 * STEP3: do tx&rx cycle to simulate working environment;
 */
extern int manual_cal_set_tx_power(wifi_standard standard, float powerdBm);
static void bk_phy_test_htol_test(void)
{
	BK_LOG_ON_ERR(htol_record_reset_event());

	manual_cal_set_tx_power(WIFI_STANDARD_11B, HTOL_TRX_11B_TARGET_POWER);

	while(1) {
		BK_LOG_ON_ERR(htol_record_working_time());
		BK_LOG_ON_ERR(bk_htol_test_trx_cycle());
	}
}

extern bool htol_is_enabled(void);

static void bk_phy_test_htol_test_thread(void *arg)
{
	bk_phy_test_htol_test();
	rtos_delete_thread(NULL);
}

void start_bk_phy_test_htol_test_thread(void)
{
	if (!htol_is_enabled()) {
		return;
	}
	RF_TEST_LOGI("start htol_test thread.\r\n");
	rtos_create_thread(
		NULL,
		BEKEN_APPLICATION_PRIORITY,
		"htol_test",
		(beken_thread_function_t)bk_phy_test_htol_test_thread,
		CONFIG_APP_MAIN_TASK_STACK_SIZE,
		(beken_thread_arg_t)0);
}
#endif

#endif
