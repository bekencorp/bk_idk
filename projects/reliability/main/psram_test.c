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
#include "statis.h"

#include <components/log.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/psram.h>
#include <driver/trng.h>

/* 1: LA capture (QSPI0 4KB + internal 8MB, GPIO trigger)
 * 0: original reliability test (internal PSRAM random verify @0x60000000) */
#define PSRAM_RELIABILITY_LA_MODE     1

#define PSRAM_REL_TAG                 "psram_rel"

#if PSRAM_RELIABILITY_LA_MODE
#define PSRAM_REL_THREAD_STACK        8192
#define PSRAM_REL_START_DELAY_MS      200

#include <driver/aon_rtc.h>
#include <driver/gpio.h>
#include <driver/io_matrix.h>
#include <driver/qspi.h>
#include <driver/qspi_psram.h>
#include "qspi_psram.h"
#include "sys_driver.h"

#define PSRAM_INTERNAL_BASE_ADDR      (0x60000000U)
#define PSRAM_INTERNAL_TEST_BYTES     (0x800000U)
#define PSRAM_QSPI0_LA_TEST_BYTES     (0x1000U)

#define PSRAM_LA_DELAY_MS             500
#define PSRAM_LA_INTER_PHASE_MS       50
#define PSRAM_LA_EXT_TRIGGER_GPIO     12
#define PSRAM_LA_INT_TRIGGER_GPIO     20
#define PSRAM_LA_QSPI0_CLK_GPIO       14
#define PSRAM_LA_QSPI0_CSN_GPIO       15
#define PSRAM_LA_QSPI0_IO0_GPIO       16
#define PSRAM_LA_QSPI0_IO1_GPIO       17
#define PSRAM_LA_QSPI0_IO2_GPIO       18
#define PSRAM_LA_QSPI0_IO3_GPIO       19

#define PSRAM_REL_STEP(step, fmt, ...) \
	BK_RAW_LOGI(PSRAM_REL_TAG, "[S%d] " fmt "\r\n", (step), ##__VA_ARGS__)

static bool s_qspi0_bus_ready;
static bool s_la_sequence_armed;

static uint64_t psram_reliability_time_delta_us(uint64_t start, uint64_t end)
{
	if (end == 0 || start >= end) {
		return 0;
	}

	return end - start;
}

static void psram_reliability_log_rate(const char *tag, const char *op, uint32_t test_len,
				       uint64_t time_us)
{
	if (time_us == 0) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "%s %s rate: invalid timer\r\n", tag, op);
		return;
	}

	uint32_t kb_per_s =
		(uint32_t)(((uint64_t)test_len * 1000000ULL) / time_us / 1024ULL);

	BK_RAW_LOGI(PSRAM_REL_TAG, "%s %s %u bytes in %u us, rate %u KB/s\r\n", tag, op, test_len,
		   (uint32_t)time_us, kb_per_s);
}

static void psram_reliability_la_gpio_init(gpio_id_t gpio)
{
	bk_iomx_set_gpio_func(gpio, FUNC_CODE_OUTPUT);
	bk_iomx_disable_pull(gpio);
	bk_iomx_output_low(gpio);
}

static void psram_reliability_la_trigger_init(void)
{
	psram_reliability_la_gpio_init((gpio_id_t)PSRAM_LA_EXT_TRIGGER_GPIO);
	psram_reliability_la_gpio_init((gpio_id_t)PSRAM_LA_INT_TRIGGER_GPIO);
}

static void psram_reliability_la_ext_begin(void)
{
	bk_iomx_output_high((gpio_id_t)PSRAM_LA_EXT_TRIGGER_GPIO);
}

static void psram_reliability_la_ext_end(void)
{
	bk_iomx_output_low((gpio_id_t)PSRAM_LA_EXT_TRIGGER_GPIO);
}

static void psram_reliability_la_int_begin(void)
{
	bk_iomx_output_high((gpio_id_t)PSRAM_LA_INT_TRIGGER_GPIO);
}

static void psram_reliability_la_int_end(void)
{
	bk_iomx_output_low((gpio_id_t)PSRAM_LA_INT_TRIGGER_GPIO);
}

static void psram_reliability_la_wait_arm(const char *phase, gpio_id_t trig_gpio)
{
	if (!s_la_sequence_armed) {
		BK_RAW_LOGI(PSRAM_REL_TAG,
			    ">>> LA ARM: sequence starts in %u ms (ext=P%d int=P%d) <<<\r\n",
			    (uint32_t)PSRAM_LA_DELAY_MS, PSRAM_LA_EXT_TRIGGER_GPIO,
			    PSRAM_LA_INT_TRIGGER_GPIO);
		rtos_delay_milliseconds((uint32_t)PSRAM_LA_DELAY_MS);
		s_la_sequence_armed = true;
	} else {
		rtos_delay_milliseconds((uint32_t)PSRAM_LA_INTER_PHASE_MS);
	}

	BK_RAW_LOGI(PSRAM_REL_TAG, ">>> LA phase: %s (GPIO%d) <<<\r\n", phase, trig_gpio);
}

static void psram_reliability_la_log_pin_map(void)
{
	BK_RAW_LOGI(PSRAM_REL_TAG, "=== Logic Analyzer Pin Map ===\r\n");
	BK_RAW_LOGI(PSRAM_REL_TAG,
		    "LA ext trig: GPIO%d (HIGH=busy, LOW=done)\r\n", PSRAM_LA_EXT_TRIGGER_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG, "QSPI0 ext   : indirect quad continuous (cmd wr=0x38 rd=0xEB)\r\n");
	BK_RAW_LOGI(PSRAM_REL_TAG, "  CLK  -> GPIO%d\r\n", PSRAM_LA_QSPI0_CLK_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG, "  CSN  -> GPIO%d\r\n", PSRAM_LA_QSPI0_CSN_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG, "  IO0  -> GPIO%d (DQ0/SI)\r\n", PSRAM_LA_QSPI0_IO0_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG, "  IO1  -> GPIO%d (DQ1/SO)\r\n", PSRAM_LA_QSPI0_IO1_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG, "  IO2  -> GPIO%d (DQ2/WP)\r\n", PSRAM_LA_QSPI0_IO2_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG, "  IO3  -> GPIO%d (DQ3/HOLD)\r\n", PSRAM_LA_QSPI0_IO3_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG,
		    "Internal PSRAM: CPU @0x60000000, test %u bytes\r\n", PSRAM_INTERNAL_TEST_BYTES);
	BK_RAW_LOGI(PSRAM_REL_TAG,
		    "LA int trig : GPIO%d (HIGH=busy, LOW=done)\r\n", PSRAM_LA_INT_TRIGGER_GPIO);
	BK_RAW_LOGI(PSRAM_REL_TAG, "==============================\r\n");
}

static bool psram_reliability_qspi0_eid_valid(const uint8_t *eid)
{
	bool all_ff = true;
	bool all_00 = true;

	for (uint32_t i = 0; i < 8U; i++) {
		if (eid[i] != 0xFFU) {
			all_ff = false;
		}
		if (eid[i] != 0x00U) {
			all_00 = false;
		}
	}
	if (all_ff || all_00) {
		return false;
	}
	if (eid[4] == 0x0DU) {
		return true;
	}
	if (eid[2] == 0x14U) {
		return true;
	}
	return false;
}

static bk_err_t psram_reliability_qspi0_read_eid(qspi_id_t id, uint8_t *eid)
{
	for (int attempt = 0; attempt < 3; attempt++) {
		if (attempt > 0) {
			bk_qspi_psram_reset(id);
			rtos_delay_milliseconds(2);
		}
		if (bk_qspi_psram_read_eid(id, eid, 8) != BK_OK) {
			continue;
		}
		if (psram_reliability_qspi0_eid_valid(eid)) {
			return BK_OK;
		}
	}
	return BK_FAIL;
}

static bk_err_t psram_reliability_qspi0_bus_init(void)
{
	bk_err_t ret;
	uint8_t eid[8] = {0};
	uint32_t psram_id;

	PSRAM_REL_STEP(3, "qspi0 bus init start");

	ret = bk_qspi_driver_init();
	if (ret != BK_OK) {
		return ret;
	}

	ret = bk_qspi_psram_init(QSPI_ID_0);
	if (ret != BK_OK) {
		return ret;
	}

	sys_drv_psram_ldo_enable(1);
	bk_psram_set_voltage(PSRAM_OUT_3_0V);
	rtos_delay_milliseconds(5);

	if (psram_reliability_qspi0_read_eid(QSPI_ID_0, eid) != BK_OK) {
		return BK_FAIL;
	}
	psram_id = ((uint32_t)eid[0] << 24) | ((uint32_t)eid[1] << 16) |
		   ((uint32_t)eid[2] << 8) | eid[3];

	BK_RAW_LOGI(PSRAM_REL_TAG,
		    "[qspi0] psram_id=0x%08x eid=%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
		    psram_id, eid[0], eid[1], eid[2], eid[3], eid[4], eid[5], eid[6],
		    eid[7]);
	if (!psram_reliability_qspi0_eid_valid(eid)) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "[qspi0] no valid PSRAM on QSPI0, skip\r\n");
		return BK_ERR_NOT_FOUND;
	}

	ret = bk_qspi_psram_enter_quad_mode(QSPI_ID_0);
	if (ret != BK_OK) {
		return ret;
	}
	ret = bk_qspi_psram_quad_write(QSPI_ID_0);
	if (ret != BK_OK) {
		return ret;
	}
	ret = bk_qspi_psram_quad_read(QSPI_ID_0);
	PSRAM_REL_STEP(8, "qspi0 bus init ok");
	return ret;
}

static bk_err_t psram_reliability_qspi0_prepare(void)
{
	bk_err_t ret;

	if (s_qspi0_bus_ready) {
		return BK_OK;
	}

	ret = psram_reliability_qspi0_bus_init();
	if (ret == BK_OK) {
		s_qspi0_bus_ready = true;
	}
	return ret;
}

static bk_err_t psram_reliability_qspi0_la_write(uint32_t test_len)
{
	uint8_t *buf = (uint8_t *)os_malloc(test_len);

	if (buf == NULL) {
		return BK_ERR_NO_MEM;
	}
	os_memset(buf, 0x44, test_len);

	psram_reliability_la_wait_arm("QSPI0 continuous WRITE",
				      (gpio_id_t)PSRAM_LA_EXT_TRIGGER_GPIO);
	psram_reliability_la_ext_begin();
	uint64_t t0 = bk_aon_rtc_get_us();
	bk_err_t ret = bk_qspi_psram_write(QSPI_ID_0, 0, buf, test_len);
	uint64_t t1 = bk_aon_rtc_get_us();
	psram_reliability_la_ext_end();

	psram_reliability_log_rate("[qspi0_ind]", "LA write", test_len,
				   psram_reliability_time_delta_us(t0, t1));
	os_free(buf);
	BK_RAW_LOGI(PSRAM_REL_TAG, "[qspi0_ind] LA continuous write %u bytes ret=%d\r\n", test_len, ret);
	return ret;
}

static bk_err_t psram_reliability_qspi0_la_read(uint32_t test_len)
{
	uint8_t *buf = (uint8_t *)os_zalloc(test_len);
	uint32_t mismatch = 0;

	if (buf == NULL) {
		return BK_ERR_NO_MEM;
	}

	psram_reliability_la_wait_arm("QSPI0 continuous READ",
				      (gpio_id_t)PSRAM_LA_EXT_TRIGGER_GPIO);
	psram_reliability_la_ext_begin();
	uint64_t t0 = bk_aon_rtc_get_us();
	bk_err_t ret = bk_qspi_psram_read(QSPI_ID_0, 0, buf, test_len);
	uint64_t t1 = bk_aon_rtc_get_us();
	psram_reliability_la_ext_end();

	if (ret == BK_OK) {
		for (uint32_t i = 0; i < test_len; i++) {
			if (buf[i] != 0x44) {
				mismatch++;
			}
		}
	}

	psram_reliability_log_rate("[qspi0_ind]", "LA read", test_len,
				   psram_reliability_time_delta_us(t0, t1));
	os_free(buf);
	BK_RAW_LOGI(PSRAM_REL_TAG, "[qspi0_ind] LA continuous read %u bytes ret=%d mismatches=%u\r\n",
		    test_len, ret, mismatch);
	return (ret == BK_OK && mismatch == 0) ? BK_OK : BK_FAIL;
}

static bk_err_t psram_reliability_internal_bus_init(void)
{
	bk_err_t ret = bk_psram_init();

	if (ret != BK_OK) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "[internal] bk_psram_init failed: %d\r\n", ret);
		return ret;
	}

	BK_RAW_LOGI(PSRAM_REL_TAG, "[internal] bk_psram_init ok\r\n");
	return BK_OK;
}

static bk_err_t psram_reliability_internal_la_write(uint32_t test_len)
{
	volatile uint32_t *base = (volatile uint32_t *)(uintptr_t)PSRAM_INTERNAL_BASE_ADDR;
	uint32_t words = test_len / 4U;

	psram_reliability_la_wait_arm("Internal PSRAM CPU WRITE",
				      (gpio_id_t)PSRAM_LA_INT_TRIGGER_GPIO);
	psram_reliability_la_int_begin();
	uint64_t t0 = bk_aon_rtc_get_us();
	for (uint32_t i = 0; i < words; i++) {
		base[i] = 0xAAAAAAAAU;
	}
	uint64_t t1 = bk_aon_rtc_get_us();
	psram_reliability_la_int_end();

	psram_reliability_log_rate("[internal]", "LA write", test_len,
				   psram_reliability_time_delta_us(t0, t1));
	BK_RAW_LOGI(PSRAM_REL_TAG, "[internal] LA write %u bytes @%08x\r\n", test_len,
		    PSRAM_INTERNAL_BASE_ADDR);
	return BK_OK;
}

static bk_err_t psram_reliability_internal_la_read(uint32_t test_len)
{
	volatile uint32_t *base = (volatile uint32_t *)(uintptr_t)PSRAM_INTERNAL_BASE_ADDR;
	uint32_t words = test_len / 4U;
	uint32_t mismatch = 0;

	psram_reliability_la_wait_arm("Internal PSRAM CPU READ",
				      (gpio_id_t)PSRAM_LA_INT_TRIGGER_GPIO);
	psram_reliability_la_int_begin();
	uint64_t t0 = bk_aon_rtc_get_us();
	for (uint32_t i = 0; i < words; i++) {
		if (base[i] != 0xAAAAAAAAU) {
			mismatch++;
		}
	}
	uint64_t t1 = bk_aon_rtc_get_us();
	psram_reliability_la_int_end();

	psram_reliability_log_rate("[internal]", "LA read", test_len,
				   psram_reliability_time_delta_us(t0, t1));
	BK_RAW_LOGI(PSRAM_REL_TAG, "[internal] LA read %u bytes mismatches=%u\r\n", test_len,
		    mismatch);
	return (mismatch == 0) ? BK_OK : BK_FAIL;
}

static bk_err_t psram_reliability_la_capture_run(void)
{
	bk_err_t ret = BK_OK;

	psram_reliability_la_trigger_init();
	psram_reliability_la_log_pin_map();
	s_la_sequence_armed = false;
	rtos_delay_milliseconds(200);

	ret = psram_reliability_qspi0_prepare();
	if (ret == BK_ERR_NOT_FOUND) {
		BK_RAW_LOGI(PSRAM_REL_TAG, "LA: no external PSRAM, skip QSPI0 phases\r\n");
	} else if (ret != BK_OK) {
		return ret;
	} else {
		ret = psram_reliability_qspi0_la_write(PSRAM_QSPI0_LA_TEST_BYTES);
		if (ret != BK_OK) {
			return ret;
		}
		ret = psram_reliability_qspi0_la_read(PSRAM_QSPI0_LA_TEST_BYTES);
		if (ret != BK_OK) {
			return ret;
		}
	}

	ret = psram_reliability_internal_bus_init();
	if (ret != BK_OK) {
		return ret;
	}
	ret = psram_reliability_internal_la_write(PSRAM_INTERNAL_TEST_BYTES);
	if (ret != BK_OK) {
		return ret;
	}
	ret = psram_reliability_internal_la_read(PSRAM_INTERNAL_TEST_BYTES);

	BK_RAW_LOGI(PSRAM_REL_TAG, "LA capture sequence done ret=%d\r\n", ret);
	return ret;
}

#else /* !PSRAM_RELIABILITY_LA_MODE */

#define PSRAM_REL_THREAD_STACK        1536

#if defined(CONFIG_PSRAM_HEAP_SIZE)
#define PSRAM_RELIABILITY_REGION_BYTES (CONFIG_PSRAM_HEAP_SIZE)
#else
#define PSRAM_RELIABILITY_REGION_BYTES (0x800000U)
#endif
#define PSRAM_RELIABILITY_STRIPE_BYTES (1024 * 4)
#define PSRAM_RELIABILITY_BASE_ADDR    (0x60000000U)

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
				*((volatile uint32_t *)(uintptr_t)(base_addr + i * buffer_len +
								   (uint32_t)k * 4U));

			if (read_value != data[k]) {
				if (error_num == 0) {
					BK_RAW_LOGI(PSRAM_REL_TAG,
						    "first mismatch @%08x read %08x expect %08x xor %08x\r\n",
						    base_addr + i * buffer_len + (uint32_t)k * 4U,
						    read_value, data[k], read_value ^ data[k]);
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

#endif /* PSRAM_RELIABILITY_LA_MODE */

static beken_thread_t s_psram_reliability_thread = NULL;

static void psram_reliability_test_run(void)
{
#if PSRAM_RELIABILITY_LA_MODE
	bk_err_t test_ret;

	PSRAM_REL_STEP(1, "test_run enter");
	g_reliability_stats.psram.test_count++;

	test_ret = psram_reliability_la_capture_run();
	if (test_ret == BK_OK) {
		statis_update_psram_test_result(&g_reliability_stats.psram, 1U, 0U);
		BK_RAW_LOGI(PSRAM_REL_TAG, "LA iteration ok total=%u\r\n",
			    g_reliability_stats.psram.test_count);
	} else {
		statis_update_psram_test_result(&g_reliability_stats.psram, 0U, 0U);
		BK_RAW_LOGI(PSRAM_REL_TAG, "LA iteration fail ret=%d total=%u\r\n", test_ret,
			    g_reliability_stats.psram.test_count);
	}

	PSRAM_REL_STEP(42, "test_run exit");
	BK_RAW_LOGI(PSRAM_REL_TAG, "psram test run end ret=%d\r\n", test_ret);
#else
	g_reliability_stats.psram.test_count++;

	uint32_t mismatch_count = 0;
	uint32_t ret = psram_reliability_verify_full_region(&mismatch_count);

	BK_RAW_LOGI(PSRAM_REL_TAG, "verify ret=%u mismatches=%u\r\n", ret, mismatch_count);
	if ((ret == 0) || ((ret == 1) && (mismatch_count > 0))) {
		BK_RAW_LOGI(PSRAM_REL_TAG,
			    "iteration failed: psram_total=%u psram_init_err=%u psram_err=%u\r\n",
			    g_reliability_stats.psram.test_count,
			    g_reliability_stats.psram.psram_init_error_count,
			    g_reliability_stats.psram.psram_error_count);
	} else {
		BK_RAW_LOGI(PSRAM_REL_TAG, "iteration ok total=%u\r\n",
			    g_reliability_stats.psram.test_count);
	}
#endif

	statis_save_to_flash();
	statis_print_all_results();
}

static void psram_reliability_worker_thread(beken_thread_arg_t arg)
{
	(void)arg;

#if PSRAM_RELIABILITY_LA_MODE
	PSRAM_REL_STEP(0, "worker thread running, delay %u ms", PSRAM_REL_START_DELAY_MS);
	rtos_delay_milliseconds(PSRAM_REL_START_DELAY_MS);
#else
	/* original mode: run immediately */
#endif

	psram_reliability_test_run();

	s_psram_reliability_thread = NULL;
	rtos_delete_thread(NULL);
}

void psram_reliability_test_start(void)
{
	bk_err_t ret;

	if (s_psram_reliability_thread != NULL) {
		return;
	}

#if PSRAM_RELIABILITY_LA_MODE
	PSRAM_REL_STEP(98, "create psram_rel thread stack=%u", PSRAM_REL_THREAD_STACK);
#else
	BK_RAW_LOGI(PSRAM_REL_TAG, "start worker thread\r\n");
#endif

	ret = rtos_create_thread(&s_psram_reliability_thread, 4, "psram_rel",
				   (beken_thread_function_t)psram_reliability_worker_thread,
				   PSRAM_REL_THREAD_STACK, NULL);
	if (ret != BK_OK) {
		BK_RAW_LOGE(PSRAM_REL_TAG, "create thread failed: %d\r\n", ret);
	}
}

