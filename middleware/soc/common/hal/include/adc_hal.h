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

#ifdef __cplusplus
extern "C" {
#endif

#include "adc_hw.h"
#include "adc_ll.h"
#include <driver/hal/hal_adc_types.h>
#include "adc_map.h"
#include <common/bk_err.h>
#include "hal_config.h"

typedef struct {
	adc_hw_t *hw;
} adc_hal_t;


#define adc_hal_set_sleep_mode(hal)		adc_ll_set_sleep_mode((hal)->hw)
#define adc_hal_set_single_step_mode(hal)	adc_ll_set_single_step_mode((hal)->hw)
#define adc_hal_set_software_control_mode(hal)	adc_ll_set_software_control_mode((hal)->hw)
#define adc_hal_get_mode(hal)			adc_ll_get_adc_mode((hal)->hw)
#define adc_hal_set_continuous_mode(hal)	adc_ll_set_continuous_mode((hal)->hw)
#define adc_hal_enbale(hal)			adc_ll_enable((hal)->hw)
#define adc_hal_disbale(hal)			adc_ll_disable((hal)->hw)
#define adc_hal_sel_channel(hal, id)		adc_ll_sel_channel((hal)->hw, id)
#define adc_hal_wait_4_cycle(hal)		adc_ll_wait_4_cycle((hal)->hw)
#define adc_hal_wait_8_cycle(hal)		adc_ll_wait_8_cycle((hal)->hw)
#define adc_hal_clear_int_status(hal)		adc_ll_clear_int_status((hal)->hw)
#define adc_hal_enable_32m_clk(hal)		adc_ll_enable_32m_clk((hal)->hw)
#define adc_hal_disable_32m_clk(hal)		adc_ll_disable_32m_clk((hal)->hw)
#define adc_hal_set_pre_div(hal, div)		adc_ll_set_pre_div((hal)->hw, div)
#define adc_hal_set_sample_rate(hal, rate)	adc_ll_set_sample_rate((hal)->hw, rate)
#define adc_hal_set_adc_filter(hal, filter)	adc_ll_set_adc_filter((hal)->hw, filter)
#define adc_hal_is_fifo_empty(hal)		adc_ll_is_fifo_empty((hal)->hw)
#define adc_hal_is_fifo_full(hal)		adc_ll_is_fifo_full((hal)->hw)
#define adc_hal_check_adc_busy(hal)		adc_ll_check_adc_busy((hal)->hw)
#define adc_hal_check_adc_enable(hal)		adc_ll_check_adc_enable((hal)->hw)

#define adc_hal_get_adc_raw_data(hal)		adc_ll_get_adc_raw_data((hal)->hw)

#define adc_hal_set_fifo_threshold(hal, value)	adc_ll_set_fifo_threshold((hal)->hw, value)
#define adc_hal_set_steady_ctrl(hal, value)	adc_ll_set_steady_ctrl((hal)->hw, value)

#define adc_hal_is_over_flow(hal)		adc_ll_is_over_flow((hal)->hw)

#define adc_hal_get_adc_data(hal)		adc_ll_get_adc_data((hal)->hw)

#define adc_hal_enable_bypass_calib(hal)	adc_ll_enable_bypass_calib((hal)->hw)
#define adc_hal_disable_bypass_calib(hal)	adc_ll_disable_bypass_calib((hal)->hw)

#define adc_hal_is_analog_channel(hal, id)      adc_ll_is_analog_channel((hal)->hw, (id))
#define adc_hal_is_valid_channel(hal, id)       (adc_ll_is_analog_channel((hal)->hw, (id)) ||\
				adc_ll_is_digital_channel((hal)->hw, (id)))

bk_err_t adc_hal_init(adc_hal_t *hal);
bk_err_t adc_hal_deinit(adc_hal_t *hal);
bk_err_t adc_hal_set_clk(adc_hal_t *hal, adc_src_clk_t src_clk, uint32_t adc_clk);
bk_err_t adc_hal_set_mode(adc_hal_t *hal, adc_mode_t adc_mode);
bk_err_t adc_hal_start_commom(adc_hal_t *hal);
bk_err_t adc_hal_stop_commom(adc_hal_t *hal);
bk_err_t adc_hal_set_saturate_mode(adc_hal_t *hal, adc_saturate_mode_t mode);
uint16_t adc_hal_get_single_step_adc_data(adc_hal_t *hal);

#ifdef SOC_ADC_FIFO_DATA_SUPPORT
#define adc_hal_get_fifo_data(hal) adc_ll_get_fifo_data((hal)->hw)
#endif

#ifdef SOC_ADC_DC_OFFSET_SUPPORT
#define adc_hal_set_dc_offset(hal, value) adc_ll_set_dc_offset((hal)->hw, value)
#define adc_hal_get_dc_offset(hal, value) adc_ll_get_dc_offset((hal)->hw)
#endif

#ifdef SOC_ADC_GAIN_SUPPORT
#define adc_hal_set_gain(hal, value) adc_ll_set_gain((hal)->hw, value)
#define adc_hal_get_gain(hal) adc_ll_get_gain((hal)->hw)
#endif

#define SARADC_AUTOTEST    0

#if SARADC_AUTOTEST
bk_err_t adc_hal_set_div(adc_hal_t *hal, uint32_t div);
#endif

#if CFG_HAL_DEBUG_ADC
void adc_struct_dump(void);
#else
#define adc_struct_dump()
#endif

#ifdef __cplusplus
}
#endif
