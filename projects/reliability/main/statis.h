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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Slots align with flash_clock_mode_t in flash_test.h: [0]=40M, [1]=80M, [2]=120M.
 */
#define FLASH_RELIABILITY_CLOCK_FREQ_COUNT 3

typedef struct
{
	uint32_t erase_error_count;
	uint32_t dbus_write_error_count;
	uint32_t cbus_write_error_count;
} flash_reliability_per_clock_stats_t;

/* Flash reliability counters (stored in test_statis partition) */
typedef struct
{
	uint32_t test_count;
	flash_reliability_per_clock_stats_t per_clock[FLASH_RELIABILITY_CLOCK_FREQ_COUNT];
} flash_reliability_stats_t;

/* PSRAM reliability counters (same persisted blob) */
typedef struct
{
	uint32_t test_count;
	uint32_t psram_init_error_count;
	uint32_t psram_error_count;
} psram_reliability_stats_t;

#define STATIS_LOW_TEMP_NUM   40
#define STATIS_HIGH_TEMP_NUM  125
#define STATIS_TEMP_RESULT_LEN (STATIS_LOW_TEMP_NUM + STATIS_HIGH_TEMP_NUM + 2)

typedef enum
{
	WIFI_STATIS_EVENT_TICK = 0,
	WIFI_STATIS_EVENT_CONNECT,
	WIFI_STATIS_EVENT_DISCONNECT,
	WIFI_STATIS_EVENT_DHCP_TIMEOUT,
	WIFI_STATIS_EVENT_GOT_IP4,
	WIFI_STATIS_EVENT_DUMP,
	WIFI_STATIS_EVENT_COUNT,
} wifi_statis_event_t;

/* WiFi reliability histogram: disconnect/dump counts vs temperature bin */
typedef struct
{
	uint32_t total_test_time;
	uint32_t temp_count_tab[STATIS_TEMP_RESULT_LEN][WIFI_STATIS_EVENT_COUNT];
} wifi_reliability_stats_t;

typedef struct
{
	flash_reliability_stats_t flash;
	psram_reliability_stats_t psram;
	wifi_reliability_stats_t wifi;
	uint32_t sys_dump_count;
	uint32_t crc;
} reliability_persisted_stats_t;

extern reliability_persisted_stats_t g_reliability_stats;

bool statis_load_from_flash(void);
void statis_save_to_flash(void);
void statis_update_psram_test_result(psram_reliability_stats_t *psram_statis, uint32_t ret, uint32_t error_num);
void statis_print_all_results(void);
#ifdef __cplusplus
}
#endif
