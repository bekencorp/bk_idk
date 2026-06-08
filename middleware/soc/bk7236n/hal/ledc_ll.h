// Copyright 2025-2026 Beken
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
#include "hal_port.h"
#include "ledc_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEDC_LL_REG_BASE           SOC_LEDC_REG_BASE
#define LEDC_LL_REG()              ((ledc_hw_t *)SOC_LEDC_REG_BASE)
#define LEDC_LL_GPIO_PIN           GPIO_18

static inline void ledc_ll_reset(ledc_hw_t *hw)
{
	LEDC_LL_REG()->ctrl.v = 0;
	LEDC_LL_REG()->send_number_l.v = 0;
	LEDC_LL_REG()->send_number_h.v = 0;
	LEDC_LL_REG()->need_read_thr.v = 0;
	LEDC_LL_REG()->need_write_thr.v = 0;
	LEDC_LL_REG()->int_en.v = 0;
	LEDC_LL_REG()->int_status.v = 0;
}

static inline void ledc_ll_deinit(ledc_hw_t *hw)
{
	ledc_ll_reset(hw);
}

static inline void ledc_ll_enable_fifo(ledc_hw_t *hw)
{
	LEDC_LL_REG()->ctrl.fifo_enable = 1;
}

static inline void ledc_ll_disable_fifo(ledc_hw_t *hw)
{
	LEDC_LL_REG()->ctrl.fifo_enable = 0;
}

static inline void ledc_ll_start(ledc_hw_t *hw)
{
	LEDC_LL_REG()->ctrl.led_start = 1;
}

static inline void ledc_ll_stop(ledc_hw_t *hw)
{
	LEDC_LL_REG()->ctrl.led_start = 0;
}

static inline void ledc_ll_pulse_start(ledc_hw_t *hw)
{
	LEDC_LL_REG()->ctrl.led_start = 1;
	LEDC_LL_REG()->ctrl.led_start = 0;
}

static inline void ledc_ll_set_send_length(ledc_hw_t *hw, uint32_t length)
{
	LEDC_LL_REG()->send_number_l.send_number_l = length & 0xFF;
	LEDC_LL_REG()->send_number_h.send_number_h = (length >> 8) & 0xF;
}

static inline void ledc_ll_set_need_write_thr(ledc_hw_t *hw, uint8_t thr)
{
	LEDC_LL_REG()->need_write_thr.need_write_thr = thr;
}

static inline void ledc_ll_set_timing(ledc_hw_t *hw, uint8_t reset_period, uint8_t code_period,
				      uint8_t t0_high, uint8_t t1_high)
{
	LEDC_LL_REG()->reset_period.reset_period_number = reset_period;
	LEDC_LL_REG()->code_period.code_period_number = code_period;
	LEDC_LL_REG()->t0_timing.t0_high_number = t0_high;
	LEDC_LL_REG()->t1_timing.t1_high_number = t1_high;
}

static inline void ledc_ll_write_fifo(ledc_hw_t *hw, uint8_t data)
{
	LEDC_LL_REG()->fifo_wdata.write_data = data;
}

static inline uint32_t ledc_ll_get_interrupt_status(ledc_hw_t *hw)
{
	return LEDC_LL_REG()->int_status.v;
}

static inline bool ledc_ll_is_fifo_full(ledc_hw_t *hw)
{
	return !!(LEDC_LL_REG()->status.fifo_full);
}

static inline bool ledc_ll_is_fifo_empty(ledc_hw_t *hw)
{
	return !!(LEDC_LL_REG()->status.fifo_empty);
}

static inline bool ledc_ll_is_busy(ledc_hw_t *hw)
{
	return !!(LEDC_LL_REG()->status.busy_now);
}

static inline bool ledc_ll_is_transfer_done(ledc_hw_t *hw)
{
	return !!(LEDC_LL_REG()->status.led_over_int);
}

static inline void ledc_ll_enable_send_finish_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_en.send_finish_inten = 1;
}

static inline void ledc_ll_disable_send_finish_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_en.send_finish_inten = 0;
}

static inline void ledc_ll_enable_write_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_en.need_wr_inten = 1;
}

static inline void ledc_ll_disable_write_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_en.need_wr_inten = 0;
}

static inline void ledc_ll_enable_read_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_en.need_read_inten = 1;
}

static inline void ledc_ll_disable_read_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_en.need_read_inten = 0;
}

static inline void ledc_ll_disable_interrupt(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_en.v = 0;
}

static inline void ledc_ll_clear_send_finish_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_status.send_finish_int = 1;
}

static inline void ledc_ll_clear_write_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_status.need_wr_int = 1;
}

static inline void ledc_ll_clear_read_int(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_status.need_read_int = 1;
}

static inline void ledc_ll_clear_interrupt_status(ledc_hw_t *hw)
{
	LEDC_LL_REG()->int_status.v = 0xFF;
}

#ifdef __cplusplus
}
#endif
