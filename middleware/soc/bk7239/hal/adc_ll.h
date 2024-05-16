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

#include <soc/soc.h>
#include "adc_hw.h"
#include "system_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_LL_REG_BASE(_adc_unit_id)    (SOC_SADC_REG_BASE)

static inline void adc_ll_soft_reset(adc_hw_t *hw)
{
	hw->global_ctrl.soft_reset = 1;
}

static inline uint32_t adc_ll_get_device_id(adc_hw_t *hw)
{
	return hw->dev_id;
}

static inline uint32_t adc_ll_get_version_id(adc_hw_t *hw)
{
	return hw->dev_version;
}

static inline uint32_t adc_ll_get_dev_status(adc_hw_t *hw)
{
	return hw->dev_status;
}
static inline void adc_ll_init(adc_hw_t *hw)
{
	adc_ll_soft_reset(hw);
	hw->ctrl.v = 0;
	//update gadc configure for saradc when clock on, remove it if use sdmadc
	//keep it without recovery since there would be only one gadc finally
	sys_ll_set_ana_reg2_sp_nt_ctrl(0x3);
	sys_ll_set_ana_reg2_gadc_refbuf_ictrl(0x2);
}

static inline void adc_ll_deinit(adc_hw_t *hw)
{
	hw->ctrl.v = 0;
	hw->sat_ctrl.v = 0;
	hw->steady_ctrl.v = 0;
}

static inline void adc_ll_set_sleep_mode(adc_hw_t *hw)
{
	hw->ctrl.adc_mode = 0;
}

static inline void adc_ll_set_single_step_mode(adc_hw_t *hw)
{
	hw->ctrl.adc_mode = 1;
}

static inline void adc_ll_set_software_control_mode(adc_hw_t *hw)
{
	hw->ctrl.adc_mode = 2;
}

static inline void adc_ll_set_continuous_mode(adc_hw_t *hw)
{
	hw->ctrl.adc_mode = 3;
}

static inline uint32_t adc_ll_get_adc_mode(adc_hw_t *hw)
{
	return (hw->ctrl.adc_mode & 0x03);
}

static inline void adc_ll_enable(adc_hw_t *hw)
{
	hw->ctrl.adc_en = 1;
}

static inline void adc_ll_disable(adc_hw_t *hw)
{
	hw->ctrl.adc_en = 0;
}

static inline void adc_ll_sel_channel(adc_hw_t *hw, uint32_t id)
{
	hw->ctrl.adc_channel = (id & 0xFF);
}

static inline void adc_ll_wait_4_cycle(adc_hw_t *hw)
{
	hw->ctrl.adc_setting = 0;
}

static inline void adc_ll_wait_8_cycle(adc_hw_t *hw)
{
	hw->ctrl.adc_setting = 1;
}

static inline void adc_ll_clear_int_status(adc_hw_t *hw)
{
	hw->ctrl.adc_int_clear = 1;
}

static inline void adc_ll_enable_32m_clk(adc_hw_t *hw)
{
	hw->ctrl.adc_32m_mode = 1;
}

static inline void adc_ll_disable_32m_clk(adc_hw_t *hw)
{
	hw->ctrl.adc_32m_mode = 0;
}

static inline void adc_ll_set_pre_div(adc_hw_t *hw, uint32_t div)
{
	hw->ctrl.adc_div = (div & 0x3F);
}

static inline void adc_ll_set_sample_rate(adc_hw_t *hw, uint32_t sample_rate)
{
	hw->ctrl.adc_samp_rate = (sample_rate & 0x3F);
}

static inline void adc_ll_set_adc_filter(adc_hw_t *hw, uint32_t filter)
{
	hw->ctrl.adc_filter = (filter & 0x7F);
}

static inline bool adc_ll_check_adc_busy(adc_hw_t *hw)
{
	return !!(hw->ctrl.adc_busy) ;
}

static inline bool adc_ll_is_fifo_empty(adc_hw_t *hw)
{
	return !!(hw->ctrl.fifo_empty) ;
}

static inline bool adc_ll_is_fifo_full(adc_hw_t *hw)
{
	return !!(hw->ctrl.fifo_full) ;
}

static inline uint32_t adc_ll_get_adc_raw_data(adc_hw_t *hw)
{
	return hw->adc_raw_data ;
}

static inline void adc_ll_set_fifo_threshold(adc_hw_t *hw, uint32_t value)
{
	if(value >= 32)
		value = 0x1F;

	hw->steady_ctrl.fifo_level = (value & 0x1F);
}

static inline void adc_ll_set_steady_ctrl(adc_hw_t *hw, uint32_t value)
{
	hw->steady_ctrl.steady_ctrl = (value & 0x7);
}

//bk7231n :this sat_enable value set const to same as other soc
static inline void adc_ll_set_sat_ctrl(adc_hw_t *hw, uint32_t value)
{
	hw->sat_ctrl.sat_ctrl = (value & 0x03);
}

static inline uint32_t adc_ll_get_sat_ctrl(adc_hw_t *hw)
{
	return (hw->sat_ctrl.sat_ctrl & 0x03);
}

static inline void adc_ll_enable_sat_process(adc_hw_t *hw)
{
	hw->sat_ctrl.sat_enable = 1;
}

static inline void adc_ll_disable_sat_process(adc_hw_t *hw)
{
	hw->sat_ctrl.sat_enable = 0;
}

static inline bool adc_ll_is_over_flow(adc_hw_t *hw)
{
	return !!(hw->sat_ctrl.over_flow) ;
}

static inline uint16_t adc_ll_get_adc_data(adc_hw_t *hw)
{
	return (hw->adc_data.adc_data_16 & 0xFFFF);
}

static inline bool adc_ll_check_adc_enable(adc_hw_t *hw)
{
	return !!(hw->ctrl.adc_en);
}

static inline void adc_ll_enable_bypass_calib(adc_hw_t *hw)
{
	hw->steady_ctrl.bypass_calibration = 1;
}

static inline void adc_ll_disable_bypass_calib(adc_hw_t *hw)
{
	hw->steady_ctrl.bypass_calibration = 0;
}

static inline bool adc_ll_is_analog_channel(adc_hw_t *hw, int id)
{
	//channel	function
	//ADC0	Vbat
	//ADC1	GPIO25
	//ADC2	GPIO24
	//ADC3	GPIO23
	//ADC4	GPIO28
	//ADC5	GPIO22
	//ADC6	GPIO21
	//ADC7	vtemp
	//ADC8	tssio
	//ADC9	vout_td
	//ADC10	NA
	//ADC11	vpeak_lo
	//ADC12	NA
	//ADC13	NA
	//ADC14	gnd
	//ADC15	vrefp
	//analog channel map: B11001011 10000001 = 0xCB81
	return (0xCB81 & (1 << id));
}

static inline bool adc_ll_is_digital_channel(adc_hw_t *hw, int id)
{
	return ((id >= SOC_ADC_CHAN_MIN) && (id <= SOC_ADC_CHAN_MAX));
}

#define ADC_INIT_SATURATE_MODE  ADC_SATURATE_MODE_1

#ifdef __cplusplus
}
#endif
