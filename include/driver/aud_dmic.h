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
#include <driver/aud_dmic_types.h>

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
bk_err_t bk_aud_dmic_init(aud_dmic_config_t *dmic_config);

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
bk_err_t bk_aud_dmic_deinit(void);

/**
 * @brief     Set the sample rate in adc work mode
 *
 * @param samp_rate adc sample rate of adc work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dmic_set_samp_rate(uint32_t samp_rate);

bk_err_t bk_aud_dmic_get_fifo_addr(uint32_t *dmic_fifo_addr);

/* get dtmf fifo data */
bk_err_t bk_aud_dmic_get_fifo_data(uint32_t *dmic_data);

/* get audio adc fifo and agc status */
bk_err_t bk_aud_dmic_get_status(uint32_t *dmic_status);

/* start adc to dac test */
bk_err_t bk_aud_dmic_start_loop_test(void);

/* stop adc to dac test */
bk_err_t bk_aud_dmic_stop_loop_test(void);

/* enable adc interrupt */
bk_err_t bk_aud_dmic_enable_int(void);

/* disable adc interrupt */
bk_err_t bk_aud_dmic_disable_int(void);

/* enable adc and adc start work */
bk_err_t bk_aud_dmic_start(void);

/* disable adc and adc stop work */
bk_err_t bk_aud_dmic_stop(void);

/* set adcl_wr_threshold */
bk_err_t bk_aud_dmic_set_dmic_wr_threshold(uint32_t value);

/* register audio interrupt */
bk_err_t bk_aud_dmic_register_isr(aud_isr_t isr);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif
