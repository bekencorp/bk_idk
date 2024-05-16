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

#pragma once

#include <common/sys_config.h>

#define TEMPD_TASK_PRIO                             4
#define TEMPD_TASK_STACK_SIZE                       1536 // 1024
#define TEMPD_QUEUE_LEN                             (5)

//The maximum retry number when failed to get the temperature, or
//when the temperature is out of range.
#define TEMPD_MAX_RETRY_NUM                         3

#define TEMPD_DISPLAY_RAW_DATA                      0

#if (CONFIG_SOC_BK7251)
#define ADC_TEMP_SENSOR_CHANNEL                     8
#elif(CONFIG_SOC_BK7271)
#define ADC_TEMP_SENSOR_CHANNEL                     0
#else
#define ADC_TEMP_SENSOR_CHANNEL                     7
#endif

#if (CONFIG_SOC_BK7231N || CONFIG_SOC_BK7236A)
#define ADC_TEMP_SATURATE_MODE                      ADC_SATURATE_MODE_1
#elif(CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#define ADC_TEMP_SATURATE_MODE                      ADC_SATURATE_MODE_2
#elif(CONFIG_SOC_BK7256XX)
#define ADC_TEMP_SATURATE_MODE                      ADC_SATURATE_MODE_3
#else
#define ADC_TEMP_SATURATE_MODE                      ADC_SATURATE_MODE_3
#endif

#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236A) || (CONFIG_SOC_BK7256XX)
#define ADC_TEMP_BUFFER_SIZE                        (5+5)//(+5 for skip)
#define ADC_TMEP_LSB_PER_10DEGREE                   (20)
#elif (CONFIG_SOC_BK7236XX)
#if CONFIG_SDMADC_TEMP
#define ADC_TEMP_BUFFER_SIZE                        (5+20)//(+5 for skip)
#define ADC_TMEP_LSB_PER_10DEGREE                   (252)//(180) for ana_reg5_adc_div=1/7
#else
#define ADC_TEMP_BUFFER_SIZE                        (5+5)//(+5 for skip)
#define ADC_TMEP_LSB_PER_10DEGREE                   (46)//(30) for ana_reg5_adc_div=1/7
#endif
#elif (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#if CONFIG_SDMADC_TEMP
#define ADC_TEMP_BUFFER_SIZE                        (5+20)//(+5 for skip)
#define ADC_TMEP_LSB_PER_10DEGREE                   (252)//(180) for ana_reg5_adc_div=1/7
#else
#define ADC_TEMP_BUFFER_SIZE                        (5+5)//(+5 for skip)
#define ADC_TMEP_LSB_PER_10DEGREE                   (40)//(30) for ana_reg5_adc_div=1/7
#endif
#else
#define ADC_TEMP_BUFFER_SIZE                        5
#define ADC_TMEP_LSB_PER_10DEGREE                   (12)// 7231:24,7231U:22,
#endif

/* From 0 to ADC_TMEP_DETECT_INTERVAL_CHANGE (120s), the detect interval is 
 * ADC_TMEP_DETECT_INTERVAL_INIT (1s); then the detect interval is changed
 * to ADC_TMEP_DETECT_INTERVAL (15s).
 * */
#define ADC_TMEP_DETECT_INTERVAL_INIT               (1)   // 1s
#define ADC_TMEP_DETECT_INTERVAL                    (15)  // 15s  how many second
#define ADC_TMEP_DETECT_INTERVAL_CHANGE             (30) // 30s
#define ADC_TMEP_XTAL_INIT                          (60)  // 60s

#define ADC_TMEP_DIST_INTIAL_VAL                    (0)

#define ADC_TMEP_10DEGREE_PER_DBPWR                 (1) // 7231:1,7231U:1,
#if(CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#if CONFIG_SDMADC_TEMP
#define ADC_TEMP_VAL_MIN                            (3192) //(32768-20000)/4
#else
#define ADC_TEMP_VAL_MIN                            (10)
#endif
#else
#define ADC_TEMP_VAL_MIN                            (50)
#endif
#if CONFIG_SDMADC_TEMP
#define ADC_TEMP_VAL_MAX                            (8192) //32768/4
#else
#define ADC_TEMP_VAL_MAX                            (1365) //for ana_reg5_adc_div=1/3
#endif
#define ADC_XTAL_DIST_INTIAL_VAL                    (70)

#if (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#define TEMP_DETEC_ADC_CLK	                    203125
#define TEMP_DETEC_ADC_SAMPLE_RATE	            0
#else
#define TEMP_DETEC_ADC_CLK	                    750000
#define TEMP_DETEC_ADC_SAMPLE_RATE	            32
#endif

#define TEMP_DETEC_ADC_STEADY_CTRL	            7
#define TEMP_DETECT_ONESHOT_TIMER               1


#define TEMPD_TAG "tempd"
#define TEMPD_LOGI(...) BK_LOGI(TEMPD_TAG, ##__VA_ARGS__)
#define TEMPD_LOGW(...) BK_LOGW(TEMPD_TAG, ##__VA_ARGS__)
#define TEMPD_LOGE(...) BK_LOGE(TEMPD_TAG, ##__VA_ARGS__)
#define TEMPD_LOGD(...) BK_LOGD(TEMPD_TAG, ##__VA_ARGS__)

typedef struct {
	uint16_t last_detect_val;
	uint16_t detect_interval;
	uint16_t detect_threshold;
	uint16_t inital_data;
	uint16_t dist_inital;
	uint16_t last_xtal_val;
	uint16_t xtal_threshold_val;
	uint16_t xtal_init_val;
	uint32_t detect_cnt;
	beken_timer_t detect_timer;
#if TEMP_DETECT_ONESHOT_TIMER
    beken2_timer_t detect_oneshot_timer;
#endif
} temp_detect_config_t;

enum {
	TMPD_PAUSE_TIMER          = 0,
	TMPD_RESTART_TIMER,
	TMPD_CHANGE_PARAM,
	TMPD_TIMER_EXPIRED,
	VOLT_PAUSE_TIMER,
	VOLT_RESTART_TIMER,
	VOLT_TIMER_EXPIRED,
	TMPD_DEINIT,
};

typedef struct temp_message {
	uint32_t temp_msg;
} tempd_msg_t;

int temp_detect_send_msg(uint32_t msg_type);
