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

#include "statis.h"

#include <string.h>
#include <components/log.h>
#include <driver/flash.h>
#include "crc32.h"
#include "partitions_gen.h"

#define STATIS_TAG "statis"

reliability_persisted_stats_t g_reliability_stats = {0};
static beken_mutex_t s_statis_save_mutex = NULL;
static bool s_statis_save_mutex_inited = false;

static bool statis_payload_crc_is_valid(const void *statis, size_t statis_size)
{
	uint32_t calc_crc = 0;
	uint32_t saved_crc = 0;

	if (statis == NULL || statis_size < sizeof(uint32_t)) {
		return false;
	}

	saved_crc = *(const uint32_t *)((const uint8_t *)statis + statis_size - sizeof(uint32_t));
	if (saved_crc == 0xFFFFFFFFU) {
		return false;
	}

	calc_crc = crc32_calc(calc_crc, statis, statis_size - sizeof(uint32_t));
	return (calc_crc == saved_crc);
}

bool statis_load_from_flash(void)
{
	const uint32_t main_addr = CONFIG_TEST_STATIS_A_PARTITION_OFFSET;
	const uint32_t backup_addr = CONFIG_TEST_STATIS_B_PARTITION_OFFSET;
	const size_t statis_size = sizeof(g_reliability_stats);

	bk_flash_read_bytes(main_addr, (uint8_t *)&g_reliability_stats, statis_size);
	if (statis_payload_crc_is_valid(&g_reliability_stats, statis_size)) {
		BK_RAW_LOGI(STATIS_TAG, "loaded stats from primary partition\r\n");
		return true;
	}

	bk_flash_read_bytes(backup_addr, (uint8_t *)&g_reliability_stats, statis_size);
	if (statis_payload_crc_is_valid(&g_reliability_stats, statis_size)) {
		BK_RAW_LOGI(STATIS_TAG, "loaded stats from backup partition\r\n");
		return true;
	}

	memset(&g_reliability_stats, 0, statis_size);
	BK_RAW_LOGI(STATIS_TAG, "no valid persisted stats, using defaults\r\n");
	return false;
}

void statis_save_to_flash(void)
{
	uint32_t crc = 0;
	uint32_t *crc_ptr = NULL;
	const uint32_t main_addr = CONFIG_TEST_STATIS_A_PARTITION_OFFSET;
	const uint32_t backup_addr = CONFIG_TEST_STATIS_B_PARTITION_OFFSET;
	const size_t statis_size = sizeof(g_reliability_stats);

	if (!s_statis_save_mutex_inited) {
		if (rtos_init_mutex(&s_statis_save_mutex) != kNoErr) {
			BK_RAW_LOGI(STATIS_TAG, "mutex init failed\r\n");
			return;
		}
		s_statis_save_mutex_inited = true;
	}

	if (rtos_lock_mutex(&s_statis_save_mutex) != kNoErr) {
		BK_RAW_LOGI(STATIS_TAG, "mutex lock failed\r\n");
		return;
	}

	crc_ptr = (uint32_t *)((uint8_t *)&g_reliability_stats + statis_size - sizeof(uint32_t));
	crc = crc32_calc(crc, &g_reliability_stats, statis_size - sizeof(uint32_t));
	*crc_ptr = crc;

	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	bk_flash_erase_sector(main_addr);
	bk_flash_write_bytes(main_addr, (uint8_t *)&g_reliability_stats, statis_size);

	bk_flash_erase_sector(backup_addr);
	bk_flash_write_bytes(backup_addr, (uint8_t *)&g_reliability_stats, statis_size);
	bk_flash_set_protect_type(FLASH_PROTECT_ALL);
	(void)rtos_unlock_mutex(&s_statis_save_mutex);
}

void statis_update_psram_test_result(psram_reliability_stats_t *psram_statis, uint32_t ret, uint32_t error_num)
{
	if (psram_statis == NULL) {
		return;
	}

	if (ret == 0) {
		psram_statis->psram_init_error_count++;
	} else if ((ret == 1) && (error_num > 0)) {
		psram_statis->psram_error_count++;
	}
}

void statis_print_flash(const flash_reliability_stats_t *flash_statis, const char *tag)
{
	if (flash_statis == NULL) {
		return;
	}

	static const char *clock_labels[FLASH_RELIABILITY_CLOCK_FREQ_COUNT] = { "40MHz", "80MHz", "120MHz" };

	BK_RAW_LOGI(STATIS_TAG, "%s total_test_count: %d\r\n", tag, flash_statis->test_count);

	BK_RAW_LOGI(STATIS_TAG, "error_type        %-12s %-12s %-12s\r\n",
		clock_labels[0], clock_labels[1], clock_labels[2]);

	BK_RAW_LOGI(STATIS_TAG, "erase_err        %-12d %-12d %-12d\r\n",
		flash_statis->per_clock[0].erase_error_count,
		flash_statis->per_clock[1].erase_error_count,
		flash_statis->per_clock[2].erase_error_count);

	BK_RAW_LOGI(STATIS_TAG, "dbus_wr_err      %-12d %-12d %-12d\r\n",
		flash_statis->per_clock[0].dbus_write_error_count,
		flash_statis->per_clock[1].dbus_write_error_count,
		flash_statis->per_clock[2].dbus_write_error_count);

	BK_RAW_LOGI(STATIS_TAG, "cbus_wr_err      %-12d %-12d %-12d\r\n",
		flash_statis->per_clock[0].cbus_write_error_count,
		flash_statis->per_clock[1].cbus_write_error_count,
		flash_statis->per_clock[2].cbus_write_error_count);
}

void statis_print_psram(const psram_reliability_stats_t *psram_statis, const char *tag)
{
	if (psram_statis == NULL) {
		return;
	}

	BK_RAW_LOGI(STATIS_TAG, "%s test_count: %d\r\n", tag, psram_statis->test_count);
	BK_RAW_LOGI(STATIS_TAG, "%s psram_init_error_count: %d\r\n", tag, psram_statis->psram_init_error_count);
	BK_RAW_LOGI(STATIS_TAG, "%s psram_error_count: %d\r\n", tag, psram_statis->psram_error_count);
}

void statis_print_wifi(const wifi_reliability_stats_t *wifi_statis, const char *tag)
{
	if (wifi_statis == NULL) {
		return;
	}

	// Event columns as specified in order: CONNECT, GOT_IP4, DISCONNECT, DHCP_TIMEOUT, DUMP
	const char *col_names[] = { "conn", "gotip", "disc", "dhcp", "dump" };
	int col_indices[] = {
		WIFI_STATIS_EVENT_CONNECT,
		WIFI_STATIS_EVENT_GOT_IP4,
		WIFI_STATIS_EVENT_DISCONNECT,
		WIFI_STATIS_EVENT_DHCP_TIMEOUT,
		WIFI_STATIS_EVENT_DUMP
	};
	const int num_cols = sizeof(col_indices) / sizeof(col_indices[0]);

	// Temp ranges: < -20, -20~20, 20~100, >100
	// STATIS_LOW_TEMP_NUM = 40, STATIS_HIGH_TEMP_NUM = 125
	// Range mapping in temp_count_tab:
	// idx 0:         < -40
	// idx 1 ~ 40:   -40 ~ 0
	// idx 41 ~ 61:   1 ~ 21
	// ...
	// idx 141:      >125

	// But per prompt: < -20, -20 ~ 20, 20 ~ 100, > 100
	// So corresponding idx ranges:
	// < -20: idx 0 ~ idx_20m1
	// -20 ~ 20: idx_20m1+1 ~ idx_20p1
	// 20 ~ 100: idx_20p1+1 ~ idx_100
	// > 100: idx_100+1 ~ last

	int temp_ranges[5]; // 4 ranges + last index+1
	int i, j;

	const int temp_20m1 = (-20) + (STATIS_LOW_TEMP_NUM + 1); // temp=-20, idx=21
	const int temp_20p1 = (20) + (STATIS_LOW_TEMP_NUM + 1);  // temp=20,  idx=61
	const int temp_100  = (100) + (STATIS_LOW_TEMP_NUM + 1); // temp=100, idx=141
	const int idx_first = 0;
	const int idx_last  = STATIS_TEMP_RESULT_LEN - 1;

	// Compute boundaries
	temp_ranges[0] = idx_first;                           // < -20:   [0, temp_20m1-1]
	temp_ranges[1] = temp_20m1;                           // -20~20:  [temp_20m1, temp_20p1]
	temp_ranges[2] = temp_20p1 + 1;                       // 20~100:  [temp_20p1+1, temp_100]
	temp_ranges[3] = temp_100 + 1;                        // >100:    [temp_100+1, idx_last]
	temp_ranges[4] = idx_last + 1;

	// Table header
	BK_RAW_LOGI(STATIS_TAG, "%-10s", "Temp");
	for (i = 0; i < num_cols; ++i)
		BK_RAW_LOGI(STATIS_TAG, "  %-8s", col_names[i]);
	BK_RAW_LOGI(STATIS_TAG, "\r\n");

	uint32_t total[num_cols];
	memset(total, 0, sizeof(total));

	const char *temp_labels[4] = { "<-20", "-20~20", "20~100", ">100" };

	for (i = 0; i < 4; ++i) {
		uint32_t counts[num_cols];
		memset(counts, 0, sizeof(counts));

		int start = temp_ranges[i];
		int end = temp_ranges[i+1] - 1;
		if (i == 0) end = temp_ranges[1] - 1; // < -20: [0, temp_20m1-1]
		if (i == 1) end = temp_ranges[2] - 1; // -20 ~ 20: [temp_20m1, temp_20p1]
		if (i == 2) end = temp_ranges[3] - 1; // 20 ~ 100: [temp_20p1+1, temp_100]
		if (i == 3) end = temp_ranges[4] - 1; // >100: [temp_100+1, idx_last]

		for (int idx = start; idx <= end; ++idx) {
			for (j = 0; j < num_cols; ++j) {
				counts[j] += wifi_statis->temp_count_tab[idx][col_indices[j]];
			}
		}

		BK_RAW_LOGI(STATIS_TAG, "%-10s", temp_labels[i]);
		for (j = 0; j < num_cols; ++j) {
			BK_RAW_LOGI(STATIS_TAG, "  %-8d", counts[j]);
			total[j] += counts[j];
		}
		BK_RAW_LOGI(STATIS_TAG, "\r\n");
	}

	// Print total row
	BK_RAW_LOGI(STATIS_TAG, "%-10s", "total");
	for (i = 0; i < num_cols; ++i)
		BK_RAW_LOGI(STATIS_TAG, "  %-8d", total[i]);
	BK_RAW_LOGI(STATIS_TAG, "\r\n");
}

void statis_print_all_results(void)
{
	BK_RAW_LOGI(STATIS_TAG, "sys_dump_count: %d\r\n", g_reliability_stats.sys_dump_count);

#if CONFIG_FLASH_RELIABILITY_TEST
	statis_print_flash(&g_reliability_stats.flash, "flash");
#endif

#if CONFIG_PSRAM_RELIABILITY_TEST
	statis_print_psram(&g_reliability_stats.psram, "psram");
#endif

#if CONFIG_WIFI_RELIABILITY_TEST
	statis_print_wifi(&g_reliability_stats.wifi, "wifi");
#endif
}