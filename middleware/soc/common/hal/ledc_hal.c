// Copyright 2022-2023 Beken
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

#include "ledc_hal.h"

bk_err_t ledc_hal_init(ledc_hal_t *hal)
{
	ledc_ll_reset(&hal->hw);
	return BK_OK;
}

bk_err_t ledc_hal_deinit(ledc_hal_t *hal)
{
	ledc_ll_deinit(&hal->hw);
	return BK_OK;
}

bk_err_t ledc_hal_configure(ledc_hal_t *hal, const ledc_timing_t *timing)
{
	if (timing == NULL) {
		return BK_ERR_NULL_PARAM;
	}

	ledc_ll_set_timing(&hal->hw, timing->reset_period_number, timing->code_period_number,
			   timing->t0_high_number, timing->t1_high_number);
	ledc_ll_set_need_write_thr(&hal->hw, timing->need_write_thr);
	ledc_ll_set_send_length(&hal->hw, timing->send_length);
	ledc_ll_enable_fifo(&hal->hw);

	return BK_OK;
}

bk_err_t ledc_hal_apply_ws2812_timing(ledc_hal_t *hal)
{
	ledc_timing_t timing = {
		.reset_period_number = LEDC_V_WS2812_RESET_PERIOD,
		.code_period_number = LEDC_V_WS2812_CODE_PERIOD,
		.t0_high_number = LEDC_V_WS2812_T0_HIGH,
		.t1_high_number = LEDC_V_WS2812_T1_HIGH,
		.need_write_thr = LEDC_V_WS2812_WRITE_THR,
		.send_length = 0,
	};

	return ledc_hal_configure(hal, &timing);
}

bk_err_t ledc_hal_set_write_threshold(ledc_hal_t *hal, uint8_t thr)
{
	ledc_ll_set_need_write_thr(&hal->hw, thr);
	return BK_OK;
}

bk_err_t ledc_hal_enable_tx_int(ledc_hal_t *hal, bool send_finish, bool need_write)
{
	ledc_ll_disable_interrupt(&hal->hw);

	if (send_finish) {
		ledc_ll_enable_send_finish_int(&hal->hw);
	}

	if (need_write) {
		ledc_ll_enable_write_int(&hal->hw);
	}

	return BK_OK;
}
