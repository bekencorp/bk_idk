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
//
#pragma once
#include <common/bk_include.h>
#include <driver/aud_dtmf_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief AUD API
 * @defgroup bk_api_aud AUD API group
 * @{
 */


/**
 * @brief     Init the adc module of audio
 *
 * This API init the adc module:
 *  - Set adc work mode: adc/dtmf
 *  - Configure the adc/dtmf parameters
 *  - disable adc/dtmf
 *  - disable adc/dtmf interrupts
 *
 * @param adc_work_mode adc work mode adc/dtmf
 * @param adc_config adc configure of adc work mode
 * @param dtmf_config dtmf configure of dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is error
 *    - BK_ERR_NULL_PARAM: config is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_init(aud_dtmf_config_t *dtmf_config);

/**
 * @brief     Deinit adc module
 *
 * This API deinit the adc module of audio:
 *   - Disable adc and dtmf
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_deinit(void);

/**
 * @brief     Get the dtmf fifo address in adc work mode
 *
 * @param dtmf_fifo_addr dtmf fifo address of dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_get_fifo_addr(uint32_t *dtmf_fifo_addr);

/**
 * @brief     Get the adc status information in adc work mode
 *
 * This API get the adc status of adc work mode:
 *   - Get fifo status
 *
 * @param adc_status adc fifo status and agc status
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_get_fifo_data(uint32_t *dtmf_data);

/**
 * @brief     Get the dtmf status information in dtmf work mode
 *
 * This API get the adc status of dtmf work mode:
 *   - Get fifo status
 *
 * @param dtmf_status dtmf fifo status
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_get_status(uint32_t *dtmf_status);

/**
 * @brief     Enable adc interrupt
 *
 * This API enable adc interrupt:
 *   - Enable adc interrupt if work mode is adc work mode
 *   - Enable dtmf interrupt if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_enable_int(void);

/**
 * @brief     Disable adc interrupt
 *
 * This API disable adc interrupt:
 *   - Disable adc interrupt if work mode is adc work mode
 *   - Disable dtmf interrupt if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_disable_int(void);

/**
 * @brief     Start adc
 *
 * This API start adc:
 *   - Enable adc if work mode is adc work mode
 *   - Enable dtmf if work mode is dtmf work mode
 *
 * Usage example:
 *
 *     //init audio driver
 *     bk_aud_driver_init();
 *
 *     //init adc configuration
 *     aud_adc_config_t adc_config;
 *     adc_config.samp_rate = AUD_ADC_SAMP_RATE_8K;
 *     adc_config.adc_enable = AUD_ADC_DISABLE;
 *     adc_config.line_enable = AUD_ADC_LINE_DISABLE;
 *     adc_config.dtmf_enable = AUD_DTMF_DISABLE;
 *     adc_config.adc_hpf2_coef_B2 = 0;
 *     adc_config.adc_hpf2_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
 *     adc_config.adc_hpf1_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
 *     adc_config.adc_set_gain = 0x2d;
 *     adc_config.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
 *     adc_config.adc_hpf2_coef_B0 = 0;
 *     adc_config.adc_hpf2_coef_B1 = 0;
 *     adc_config.adc_hpf2_coef_A0 = 0;
 *     adc_config.adc_hpf2_coef_A1 = 0;
 *     adc_config.dtmf_wr_threshold = 8;
 *     adc_config.adcl_wr_threshold = 8;
 *     adc_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
 *     adc_config.adcl_int_enable = AUD_ADCL_INT_DISABLE;
 *     adc_config.loop_adc2dac = AUD_LOOP_ADC2DAC_DISABLE;
 *     adc_config.agc_noise_thrd = 101;
 *     adc_config.agc_noise_high = 101;
 *     adc_config.agc_noise_low = 160;
 *     adc_config.agc_noise_min = 1;
 *     adc_config.agc_noise_tout = 0;
 *     adc_config.agc_high_dur = 3;
 *     adc_config.agc_low_dur = 3;
 *     adc_config.agc_min = 1;
 *     adc_config.agc_max = 4;
 *     adc_config.agc_ng_method = AUD_AGC_NG_METHOD_MUTE;
 *     adc_config.agc_ng_enable = AUD_AGC_NG_DISABLE;
 *     adc_config.agc_decay_time = AUD_AGC_DECAY_TIME_128;
 *     adc_config.agc_attack_time = AUD_AGC_ATTACK_TIME_128;
 *     adc_config.agc_high_thrd = 18;
 *     adc_config.agc_low_thrd = 0;
 *     adc_config.agc_iir_coef = AUD_AGC_IIR_COEF_1_1024;
 *     adc_config.agc_enable = AUD_AGC_DISABLE;
 *     adc_config.manual_pga_value = 0;
 *     adc_config.manual_pga_enable = AUD_GAC_MANUAL_PGA_DISABLE;
 *     bk_aud_adc_init(AUD_ADC_WORK_MODE_ADC, &adc_config, NULL);
 *     CLI_LOGI("init adc successful\n");
 *
 *     //start adc and dac
 *     bk_aud_start_adc();
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_start(void);

/**
 * @brief     Stop adc
 *
 * This API stop adc:
 *   - Disable adc if work mode is adc work mode
 *   - Disable dtmf if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dtmf_stop(void);


/* set adcl_wr_threshold */
bk_err_t bk_aud_dtmf_set_dtmf_wr_threshold(uint32_t value);

/* register audio interrupt */
bk_err_t bk_aud_dtmf_register_isr(aud_isr_t isr);

bk_err_t bk_aud_dtmf_start_loop_test(void);

/* stop adc to dac test */
bk_err_t bk_aud_dtmf_stop_loop_test(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
