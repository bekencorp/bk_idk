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

#include "psram_test.h"
#include "flash_test.h"
#include "statis.h"

#include <components/log.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/psram.h>
#include <driver/dma.h>

#include "bk_general_dma.h"
#include "soc/mapping.h"
#include "psram_hal.h"
#include "cache.h"
#include "bk_sensor_internal.h"
#include "driver/trng.h"

#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"
#endif

#define PSRAM_REL_TAG "psram_rel"

#define PSRAM_RELIABILITY_REGION_BYTES (CONFIG_PSRAM_HEAP_SIZE)
#define PSRAM_RELIABILITY_STRIPE_BYTES (1024 * 4)
#define PSRAM_RELIABILITY_BASE_ADDR    (0x60000000U)

static beken_thread_t s_psram_reliability_thread = NULL;

/* Returns 0 on fatal setup failure, 1 after verify (see *mismatch_count) */
static uint32_t psram_reliability_verify_full_region(uint32_t *mismatch_count)
{
	uint32_t total_words, correct_words, correctness_rate;
	uint32_t test_len = PSRAM_RELIABILITY_REGION_BYTES;
	uint32_t buffer_len = PSRAM_RELIABILITY_STRIPE_BYTES;
	uint32_t *data = (uint32_t *)os_malloc(PSRAM_RELIABILITY_STRIPE_BYTES);

	if (data == NULL) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "malloc stripe buffer failed\r\n");
		return 0;
	}

	bk_err_t ret = bk_psram_init();
	if (ret != BK_OK) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "bk_psram_init failed: %d\r\n", ret);
		statis_update_psram_test_result(&g_reliability_stats.psram, 0, 0);
		os_free(data);
		return 0;
	}

	for (int i = 0; i < (int)(buffer_len / 4U); i++) {
		data[i] = bk_rand() + (uint32_t)i;
	}

	uint32_t i = 0;
	uint32_t error_num = 0;
	uint32_t read_value = 0;
	uint32_t base_addr = PSRAM_RELIABILITY_BASE_ADDR;

	BK_RAW_LOGI(PSRAM_REL_TAG, "write sweep %08x .. %08x\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / buffer_len; i++) {
		for (int j = 0; j < (int)(buffer_len / 4U); j++) {
			*((volatile uint32_t *)(uintptr_t)(base_addr + i * buffer_len + (uint32_t)j * 4U)) =
				data[j];
		}
	}

	BK_RAW_LOGI(PSRAM_REL_TAG, "read verify %08x .. %08x\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / buffer_len; i++) {
		for (int k = 0; k < (int)(buffer_len / 4U); k++) {
			read_value =
				*((volatile uint32_t *)(uintptr_t)(base_addr + i * buffer_len + (uint32_t)k * 4U));

			if (read_value != data[k]) {
				if (error_num == 0) {
					BK_RAW_LOGI(PSRAM_REL_TAG,
						"first mismatch @%08x read %08x expect %08x xor %08x\r\n",
						base_addr + i * buffer_len + (uint32_t)k * 4U, read_value, data[k],
						read_value ^ data[k]);
				}
				error_num++;
			}
		}
	}

	total_words = test_len / 4U;
	correct_words = total_words - error_num;
	correctness_rate = (correct_words * 100U) / total_words;

	if (error_num == 0) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "verify passed (words=%u)\r\n", total_words);
	} else {
		BK_RAW_LOGI(PSRAM_REL_TAG, "verify failed mismatches=%u correctness=%u%%\r\n", error_num,
			correctness_rate);
	}

	statis_update_psram_test_result(&g_reliability_stats.psram, 1, error_num);

	if (mismatch_count) {
		*mismatch_count = error_num;
	}

	os_free(data);
	return 1;
}

static void psram_reliability_worker_thread(beken_thread_arg_t arg)
{
	(void)arg;

	uint32_t need_save = 0;

	g_reliability_stats.psram.test_count++;
#if CONFIG_PSRAM_RELIABILITY_TEST
	{
		uint32_t mismatch_count = 0;
		uint32_t ret = psram_reliability_verify_full_region(&mismatch_count);

		BK_RAW_LOGI(PSRAM_REL_TAG, "verify ret=%u mismatches=%u\r\n", ret, mismatch_count);
		if ((ret == 0) || ((ret == 1) && (mismatch_count > 0))) {
			BK_RAW_LOGI(PSRAM_REL_TAG,
				"iteration failed: psram_total=%u psram_init_err=%u psram_err=%u\r\n",
				g_reliability_stats.psram.test_count,
				g_reliability_stats.psram.psram_init_error_count,
				g_reliability_stats.psram.psram_error_count);
			need_save = 1;
		} else {
			BK_RAW_LOGI(PSRAM_REL_TAG, "iteration ok total=%u\r\n",
				g_reliability_stats.psram.test_count);
		}
	}
#else
	BK_RAW_LOGI(PSRAM_REL_TAG, "PSRAM reliability disabled in build\r\n");
#endif

	statis_save_to_flash();
	statis_print_all_results();

	s_psram_reliability_thread = NULL;
	rtos_delete_thread(NULL);
}

void psram_reliability_test_start(void)
{
	if (s_psram_reliability_thread == NULL) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "start worker thread\r\n");
		rtos_create_thread(&s_psram_reliability_thread, 4, "psram_rel",
				   (beken_thread_function_t)psram_reliability_worker_thread, 1536, NULL);
	}
}
