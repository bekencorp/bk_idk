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

#include "flash_test.h"
#include "statis.h"

#include <string.h>
#include <components/log.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include <common/sys_config.h>
#include "flash_driver.h"
#include "sys_ll.h"

#if CONFIG_FLASH_CBUS
#include "security.h"
#endif

#define FLASH_REL_TAG "flash"

#define FLASH_IO_TRACE_UP(x)   //(GPIO_UP(x))
#define FLASH_IO_TRACE_DOWN(x) //(GPIO_DOWN(x))

static beken_thread_t s_flash_reliability_thread = NULL;
static uint8_t s_flash_verify_read_buf[FLASH_RELIABILITY_CHUNK_BYTES];
static uint8_t s_flash_verify_write_buf[FLASH_RELIABILITY_CHUNK_BYTES];
/* Active flash clock slot during multirate verify (index into g_reliability_stats.flash.per_clock). */
static flash_clock_mode_t s_multirate_active_clock_mode;

_Static_assert(FLASH_CLOCK_MODE_120M == FLASH_RELIABILITY_CLOCK_FREQ_COUNT - 1,
	       "flash_clock_mode_t must match FLASH_RELIABILITY_CLOCK_FREQ_COUNT");

static const char* flash_clock_mode_to_string(flash_clock_mode_t mode)
{
	switch (mode) {
		case FLASH_CLOCK_MODE_40M:
			return "40MHz";
		case FLASH_CLOCK_MODE_80M:
			return "80MHz";
		case FLASH_CLOCK_MODE_120M:
			return "120MHz";
		default:
			return "Unknown";
	}
}

static void flash_reliability_fill_verify_write_pattern(void)
{
	for (uint32_t i = 0; i < FLASH_RELIABILITY_CHUNK_BYTES; i++) {
		s_flash_verify_write_buf[i] = (uint8_t)(i & 0xFF);
	}
}

//TODO put it to hal layer
static void flash_reliability_set_flash_clock_mode(flash_clock_mode_t clock_mode)
{
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0);
    switch (clock_mode) {
        case FLASH_CLOCK_MODE_40M:
            sys_ll_set_cpu_clk_div_mode2_cksel_flash(0);
            break;
        case FLASH_CLOCK_MODE_80M:
            sys_ll_set_cpu_clk_div_mode2_cksel_flash(2);
            break;
        case FLASH_CLOCK_MODE_120M:
            sys_ll_set_cpu_clk_div_mode2_cksel_flash(3);
            break;
        default:
            break;
    }
}

static void flash_reliability_restore_default_flash_clock(void)
{
	flash_reliability_set_flash_clock_mode(FLASH_CLOCK_MODE_80M);
}

static void flash_reliability_erase_sector_and_verify_erased(uint32_t addr)
{
	uint32_t i;

	bk_flash_erase_sector(addr);
	memset(s_flash_verify_read_buf, 0, FLASH_RELIABILITY_CHUNK_BYTES);
	bk_flash_read_bytes(addr, s_flash_verify_read_buf, FLASH_RELIABILITY_CHUNK_BYTES);

	for (i = 0; i < FLASH_RELIABILITY_CHUNK_BYTES; i++) {
		if (s_flash_verify_read_buf[i] != FLASH_RELIABILITY_ERASED_BYTE) {
			BK_RAW_LOGI(FLASH_REL_TAG, "%x rd[%d]:%02x\r\n", addr + i, i, s_flash_verify_read_buf[i]);
			g_reliability_stats.flash.per_clock[s_multirate_active_clock_mode].erase_error_count++;
			return;
		}
	}
}

static void flash_reliability_write_chunk_and_verify(uint32_t addr)
{
	uint32_t i;

	memset(s_flash_verify_read_buf, 0, FLASH_RELIABILITY_CHUNK_BYTES);
	bk_flash_write_bytes(addr, s_flash_verify_write_buf, FLASH_RELIABILITY_CHUNK_BYTES);
	bk_flash_read_bytes(addr, s_flash_verify_read_buf, FLASH_RELIABILITY_CHUNK_BYTES);

	for (i = 0; i < FLASH_RELIABILITY_CHUNK_BYTES; i++) {
		if (s_flash_verify_write_buf[i] != s_flash_verify_read_buf[i]) {
			BK_RAW_LOGI(FLASH_REL_TAG, "%x w[%d]:%02x r[%d]:%02x\r\n", addr + i, i,
				s_flash_verify_write_buf[i], i, s_flash_verify_read_buf[i]);
			g_reliability_stats.flash.per_clock[s_multirate_active_clock_mode].dbus_write_error_count++;
			return;
		}
	}
}

static void flash_reliability_dbus_verify(void)
{
	uint32_t current_addr = CONFIG_FLASH_TEST_PARTITION_OFFSET;

	FLASH_IO_TRACE_UP(2);

	do {
		FLASH_IO_TRACE_UP(3);
		flash_reliability_erase_sector_and_verify_erased(current_addr);
		FLASH_IO_TRACE_DOWN(3);

		FLASH_IO_TRACE_UP(4);
		flash_reliability_write_chunk_and_verify(current_addr);
		FLASH_IO_TRACE_DOWN(4);

		current_addr += FLASH_RELIABILITY_CHUNK_BYTES;
	} while (current_addr < CONFIG_FLASH_TEST_PARTITION_OFFSET + CONFIG_FLASH_TEST_PARTITION_SIZE);

	FLASH_IO_TRACE_DOWN(2);
}

#if CONFIG_FLASH_CBUS

/* CBUS program + readback compare (mirror dbus write_chunk_and_verify). */
static void flash_reliability_write_chunk_and_verify_cbus(uint32_t addr)
{
	uint32_t i;

	memset(s_flash_verify_read_buf, 0, FLASH_RELIABILITY_CHUNK_BYTES);
	bk_flash_write_cbus(addr, s_flash_verify_write_buf, FLASH_RELIABILITY_CHUNK_BYTES);
	bk_flash_read_cbus(addr, s_flash_verify_read_buf, FLASH_RELIABILITY_CHUNK_BYTES);

	for (i = 0; i < FLASH_RELIABILITY_CHUNK_BYTES; i++) {
		if (s_flash_verify_write_buf[i] != s_flash_verify_read_buf[i]) {
			BK_RAW_LOGI(FLASH_REL_TAG, "cbus %x w[%d]:%02x r[%d]:%02x\r\n", addr + i, i,
				    s_flash_verify_write_buf[i], i, s_flash_verify_read_buf[i]);
			g_reliability_stats.flash.per_clock[s_multirate_active_clock_mode].cbus_write_error_count++;
			return;
		}
	}
}

static void flash_reliability_cbus_verify(void)
{
    uint32_t phy_start_addr = CONFIG_FLASH_TEST_PARTITION_OFFSET;
    uint32_t phy_end_addr = CONFIG_FLASH_TEST_PARTITION_OFFSET + CONFIG_FLASH_TEST_PARTITION_SIZE - FLASH_RELIABILITY_CHUNK_BYTES;


    uint32_t cbus_start_addr = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_start_addr));
    uint32_t cbus_end_addr = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_end_addr)) - FLASH_RELIABILITY_CHUNK_BYTES;

	FLASH_IO_TRACE_UP(2);
	uint32_t current_addr = phy_start_addr;
	do {
		FLASH_IO_TRACE_UP(3);
		flash_reliability_erase_sector_and_verify_erased(current_addr);
        current_addr += FLASH_RELIABILITY_CHUNK_BYTES;
		FLASH_IO_TRACE_DOWN(3);
	} while (current_addr < phy_end_addr);


    current_addr = cbus_start_addr;
	do {
		FLASH_IO_TRACE_UP(4);
		flash_reliability_write_chunk_and_verify_cbus(current_addr);
        current_addr += FLASH_RELIABILITY_CHUNK_BYTES;
		FLASH_IO_TRACE_DOWN(4);
	} while (current_addr < cbus_end_addr);

	FLASH_IO_TRACE_DOWN(2);
}

#endif /* CONFIG_FLASH_CBUS */

static void flash_reliability_verify(void)
{
	flash_clock_mode_t flash_freqs[] = {FLASH_CLOCK_MODE_80M, FLASH_CLOCK_MODE_120M };
    uint32_t flash_freqs_count = sizeof(flash_freqs) / sizeof(flash_freqs[0]);
    flash_clock_mode_t flash_freq;

	flash_reliability_fill_verify_write_pattern();

	for (uint32_t i = 0; i < flash_freqs_count; i++) {
		flash_freq = flash_freqs[i];
		s_multirate_active_clock_mode = flash_freq;
		BK_RAW_LOGI(FLASH_REL_TAG, "flash freq %s start\r\n", flash_clock_mode_to_string(flash_freq));
		flash_reliability_set_flash_clock_mode(flash_freq);
		flash_reliability_dbus_verify();
#if CONFIG_FLASH_CBUS
		flash_reliability_cbus_verify();
#endif
		BK_RAW_LOGI(FLASH_REL_TAG, "flash freq %s end\r\n", flash_clock_mode_to_string(flash_freq));
	}

	flash_reliability_restore_default_flash_clock();
}

static void flash_reliability_worker_thread(beken_thread_arg_t arg)
{
	(void)arg;

	FLASH_IO_TRACE_UP(15);
	g_reliability_stats.flash.test_count++;

    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	volatile uint32_t start_time = rtos_get_time();
	flash_reliability_verify();
	volatile uint32_t end_time = rtos_get_time();
	BK_RAW_LOGI(FLASH_REL_TAG, "tested time: %d us\r\n", end_time - start_time);
    bk_flash_set_protect_type(FLASH_PROTECT_ALL);

	statis_save_to_flash();
	statis_print_all_results();

	s_flash_reliability_thread = NULL;
	FLASH_IO_TRACE_DOWN(15);
    rtos_delete_thread(NULL);
}

void flash_reliability_test_start(void)
{
	if (s_flash_reliability_thread == NULL) {
		rtos_create_thread(&s_flash_reliability_thread, 4, "flash_rel",
				   (beken_thread_function_t)flash_reliability_worker_thread, 1536, NULL);
	}
}