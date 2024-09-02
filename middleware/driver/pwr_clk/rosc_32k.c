// Copyright 2020-2021 Beken
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

#include <common/bk_include.h>
#include <driver/rosc_32k.h>
#include <driver/aon_rtc.h>
#include <driver/ckmn.h>
#include <sys_driver.h>
#include <os/mem.h>
#include "aon_pmu_driver.h"
#include "rosc.h"


#define ROSC_CLK_26M                     (26 * 1000 * 1000)
#define ROSC_CLK_32K                     (32 * 1000)

#define ROSC_CKMN_PPM                    (600)
#define ROSC_CKMN_MEASURE_TIME_THRESHOLD (64)

#define ROSC_CALIB_THREAD_PRIORITY       (8)
#define ROSC_CALIB_THREAD_NAME           "rosc_calib"
#define ROSC_CALIB_THREAD_STACK_SIZE     (0x4<<10)
#define ROSC_CALIB_MANUAL_MODE           (1)
#define ROSC_CALIB_TRIG_ONCE             (3)
#define ROSC_CALIB_CIN0_DEFAULT          (0x40)
#define ROSC_CALIB_CIN0_MAX              (0x80)
#define ROSC_CALIB_CIN1_DEFAULT          (0x100)
#define ROSC_CALIB_CIN1_MAX              (0x200)
#define ROSC_CALIB_32K_COUNT_2MS         (2*32)

#if ROSC_CALIB_DEBUG
#define ROSC_CAL_s_records_cnt_MAX 50

static rosc_calib_record_t* s_records_array = NULL;
static uint32_t s_records_cnt = 0;
#endif

static double s_rosc_freq_hz = ROSC_CLK_32K;
static int32_t s_rosc_ppm = ROSC_CKMN_PPM;


double bk_rosc_32k_get_freq(void)
{
	return s_rosc_freq_hz;
}

bk_err_t bk_rosc_32k_get_ppm(void)
{
	return s_rosc_ppm;
}

static void rosc_ckmn_isr(void)
{
	// GPIO_UP(16); // 1us
	uint32_t cnt_32k = bk_ckmn_get_rc32k_count();
	uint32_t cnt_26m = bk_ckmn_get_rc26m_count();
	double theta, freq;
	// GPIO_DOWN(16); // 25us 17us
	freq = ROSC_CLK_26M * cnt_32k / (double)cnt_26m;
	if (cnt_32k <= ROSC_CKMN_MEASURE_TIME_THRESHOLD)
	{
		s_rosc_freq_hz = (freq * 2 + s_rosc_freq_hz) / 3;
	} else {
		s_rosc_freq_hz = freq;
	}
	// GPIO_UP(16); // 17us
	theta = s_rosc_freq_hz / ROSC_CLK_32K;
	s_rosc_ppm = (int32_t)(1e6 * (1 - theta) + ROSC_CKMN_PPM * theta);
	// GPIO_DOWN(16);
	// CKMN_LOGI("ckmn_rosc_isr rosc_freq:%7.2f\r\n", s_rosc_freq_hz);
	bk_ckmn_ckest_disable();
}

bk_err_t bk_rosc_32k_ckest_prog(uint32_t count)
{
#if CONFIG_ROSC_COMPENSATION
	bk_aon_rtc_get_current_tick_with_compensation(AON_RTC_ID_1); // trig tick compensation
#endif
	BK_LOG_ON_ERR(bk_ckmn_set_rc32k_count(count));
	BK_LOG_ON_ERR(bk_ckmn_register_isr(CKMN_INT_CKEST, rosc_ckmn_isr));
	BK_LOG_ON_ERR(bk_ckmn_ckest_enable());

	return BK_OK;
}

#if CONFIG_ROSC_CALIB_SW
static float s_freq_32k = 0.0;
static beken_semaphore_t s_rosc_calib_sema = NULL;
static beken_thread_t s_rosc_calib_thread = NULL;


static void rosc_calib_delay_for_ckest_ready(void)
{
	// wait at least one 32k tick for ckest to reset ok
	uint64_t tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	while(bk_aon_rtc_get_current_tick(AON_RTC_ID_1) < tick + 5);
}

static void rosc_calib_ckmn_isr(void)
{
	int rc26m_count = 0;
	int rosc_count = 0;

	bk_ckmn_ckest_disable();
	rc26m_count = bk_ckmn_get_rc26m_count();
	rosc_count = bk_ckmn_get_rc32k_count();

	if (rc26m_count <= 0 || rosc_count <= 0) {
		ROSC_LOGI("ckmn get rc26m cycle failed!\n");
		return;
	}

	s_freq_32k = ROSC_CLK_26M * (rosc_count / ((float)rc26m_count));
	rtos_set_semaphore(&s_rosc_calib_sema);
}

static void rosc_calib_thread(void *args)
{
	bk_err_t ret;
	uint8_t progress_count = 0;
	uint32_t cin0 = ROSC_CALIB_CIN0_DEFAULT;
	uint32_t cin1 = ROSC_CALIB_CIN1_DEFAULT;
	float loss, _loss = 0.0;

	// init
	ROSC_LOGI("ckmn calib rosc thread start\r\n");
	sys_drv_rosc_calibration(ROSC_CALIB_MANUAL_MODE, cin0 + (cin1 << 16)); // manual mode
	// calibration progress
	while (1)
	{
		// get 32k_freq
		bk_ckmn_set_rc32k_count(ROSC_CALIB_32K_COUNT_2MS); // 2ms
		bk_ckmn_register_isr(CKMN_INT_CKEST, rosc_calib_ckmn_isr);
		rosc_calib_delay_for_ckest_ready();
		bk_ckmn_ckest_enable();
		// wait for ckmn isr
		ret = rtos_get_semaphore(&s_rosc_calib_sema, 100);
		if(kNoErr == ret) {
			// loss check
			loss = s_freq_32k - ROSC_CLK_32K;
#if ROSC_CALIB_DEBUG
			rosc_calib_record_t record = {progress_count, cin0, cin1, loss};
			rosc_calib_records_insert(&record);
#endif
			if (loss == 0) {
				break;
			}
			if (loss * _loss < 0) {
				if ((loss < 0) ? (-loss < _loss) : (loss < -_loss)) {
					if (progress_count++ == 3)
						break;
				}
			}
			_loss = loss;
			// next prog
			switch (progress_count)
			{
			case 0:
				if (loss > 0) cin0--;
				else cin0++;
				break;
			case 1:
				if (loss > 0) cin1-=32;
				else cin1+=20;
				break;
			case 2:
				if (loss > 0) cin1-=4;
				else cin1+=4;
				break;
			case 3:
				if (loss > 0) cin1--;
				else cin1++;
				break;
			default:
				// restart calib
				progress_count = 0;
				break;
			}

			if (cin0 < ROSC_CALIB_CIN0_MAX && cin1 < ROSC_CALIB_CIN1_MAX)
			{
				sys_drv_rosc_calibration(ROSC_CALIB_MANUAL_MODE, cin0 + (cin1 << 16)); // manual mode
			} else {
				sys_drv_rosc_calibration(ROSC_CALIB_MANUAL_MODE, ROSC_CALIB_CIN0_DEFAULT + (ROSC_CALIB_CIN1_DEFAULT << 16));
				ret = BK_FAIL;
				break;
			}
		}
	}

	// complete
	if (ret) {
		ROSC_LOGE("rosc calib failed reason code: %d\r\n", ret);
	} else {
		ROSC_LOGI("rosc calib complete %d %d\r\n", cin0, cin1);
	}

#if ROSC_CALIB_DEBUG
	rosc_calib_records_dump();

	s_records_cnt = 0;

	os_free(s_records_array);
	s_records_array = NULL;
#endif

	rtos_deinit_semaphore(&s_rosc_calib_sema);
	s_rosc_calib_sema = NULL;

	rtos_delete_thread(NULL);
	s_rosc_calib_thread = NULL;
}

bk_err_t bk_rosc_32k_calib()
{
	bk_err_t ret = BK_OK;

	if ((s_rosc_calib_sema != NULL) || (s_rosc_calib_thread != NULL))
	{
		ROSC_LOGE("rosc calib is ongoing\r\n");
		return BK_FAIL;
	}

	ret = rtos_init_semaphore(&s_rosc_calib_sema, 1);
	if (kNoErr != ret)
	{
		ROSC_LOGE("init sema ret=%d\r\n", ret);
		goto err_exit;
	}

	ret = rtos_create_thread(
								&s_rosc_calib_thread,
								ROSC_CALIB_THREAD_PRIORITY,
								ROSC_CALIB_THREAD_NAME,
								rosc_calib_thread,
								ROSC_CALIB_THREAD_STACK_SIZE,
								NULL
							);
	if (kNoErr != ret)
	{
		ROSC_LOGE("init thread ret=%d\r\n", ret);
		goto err_exit;
	}

#if ROSC_CALIB_DEBUG
	s_records_array = (rosc_calib_record_t*)os_malloc(sizeof(rosc_calib_record_t) * ROSC_CAL_s_records_cnt_MAX);
	if (NULL == s_records_array)
	{
		ROSC_LOGE("debug records malloc fail\r\n");
	}
#endif

	return ret;

err_exit:

	if (s_rosc_calib_sema)
	{
		rtos_deinit_semaphore(&s_rosc_calib_sema);
		s_rosc_calib_sema = NULL;
	}

	if (s_rosc_calib_thread)
	{
		rtos_delete_thread(&s_rosc_calib_thread);
		s_rosc_calib_thread = NULL;
	}

	return ret;
}
#endif

#if CONFIG_ROSC_COMPENSATION
#define ROSC_COMPE_DEBUG_INTV 1000 // *31.25us 1000 is recommanded for normal debug and 10000 for low voltage sleep (> dtim3)

static uint64_t s_base_tick = 0;
static double s_tick_diff = 0;


int64_t bk_rosc_32k_get_tick_diff(uint64_t tick)
{
	int64_t tick_diff = 0;

	if (s_base_tick == 0)
	{
		if (aon_pmu_drv_lpo_src_get() == PM_LPO_SRC_ROSC)
		{
			ROSC_LOGE("should not be here: %s %d\r\n", __func__, __LINE__);
		}
		ROSC_LOGI("rosc rtc tick compensation start\r\n");
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	// if tick is 0 means get tick diff without recalculation
	if (tick != 0)
	{
		if (aon_pmu_drv_lpo_src_get() == PM_LPO_SRC_ROSC)
		{
#if ROSC_COMPE_DEBUG
			if (tick - s_base_tick >= ROSC_COMPE_DEBUG_INTV)
			ROSC_LOGI("rosc rtc tick compe: %d+%d->%d\r\n", (int32_t)s_tick_diff, (uint32_t)s_base_tick, (uint32_t)(s_base_tick + s_tick_diff));
#endif
			if (tick < s_base_tick)
			{
				ROSC_LOGW("check if tick is overflow %d %d %d\r\n", (int32_t)s_tick_diff, (uint32_t)s_base_tick, (uint32_t)tick);
			}
			else
			{
				s_tick_diff += (tick - s_base_tick) * (ROSC_CLK_32K / s_rosc_freq_hz - 1);
			}
		}
		s_base_tick = tick;
	}
	tick_diff = (int64_t)s_tick_diff;
	GLOBAL_INT_RESTORE();

	return tick_diff;
}

int64_t bk_rosc_32k_get_time_diff(void)
{
	int64_t time_diff = 0;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	time_diff = (int64_t)(s_tick_diff * 1000 / 32);
	GLOBAL_INT_RESTORE();

	return time_diff;
}
#else
int64_t bk_rosc_32k_get_tick_diff(uint64_t tick)
{
	return 0;
}
int64_t bk_rosc_32k_get_time_diff(void)
{
	return 0;
}
#endif

#if ROSC_CALIB_DEBUG
static bk_err_t rosc_calib_records_insert(rosc_calib_record_t* record)
{
	bk_err_t ret = BK_OK;
	rosc_calib_record_t* records_node;

	if (NULL == s_records_array)
	{
		return BK_FAIL;
	}

	if (s_records_cnt >= ROSC_CAL_s_records_cnt_MAX)
	{
		// ROSC_LOGI("records array is full\r\n");
		return BK_FAIL;
	}

	records_node = &s_records_array[s_records_cnt++];
	records_node->progress_count = record->progress_count;
	records_node->cin0 = record->cin0;
	records_node->cin1 = record->cin1;
	records_node->loss = record->loss;

	return ret;
}

static void rosc_calib_records_dump(void)
{
	uint32_t i;
	rosc_calib_record_t* records_node;

	ROSC_LOGI("rosc calib records dump start\r\n");

	for (i = 0; i < s_records_cnt; i++)
	{
		records_node = &s_records_array[i];
		ROSC_LOGI("%d %d %d %f\r\n", 	records_node->progress_count,
											records_node->cin0,
											records_node->cin1,
											records_node->loss);
	}

	ROSC_LOGI("rosc calib records dump end\r\n");
}
#endif