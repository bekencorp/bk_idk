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

#include "hal_config.h"
#include "ledc_hw.h"
#include "ledc_ll.h"
#include <driver/hal/hal_ledc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	ledc_hw_t hw;
} ledc_hal_t;

#define ledc_hal_enable_fifo(hal)              ledc_ll_enable_fifo(&(hal)->hw)
#define ledc_hal_disable_fifo(hal)             ledc_ll_disable_fifo(&(hal)->hw)
#define ledc_hal_start(hal)                    ledc_ll_start(&(hal)->hw)
#define ledc_hal_stop(hal)                     ledc_ll_stop(&(hal)->hw)
#define ledc_hal_pulse_start(hal)              ledc_ll_pulse_start(&(hal)->hw)
#define ledc_hal_set_send_length(hal, len)     ledc_ll_set_send_length(&(hal)->hw, len)
#define ledc_hal_write_fifo(hal, data)         ledc_ll_write_fifo(&(hal)->hw, data)
#define ledc_hal_get_interrupt_status(hal)     ledc_ll_get_interrupt_status(&(hal)->hw)
#define ledc_hal_is_fifo_full(hal)             ledc_ll_is_fifo_full(&(hal)->hw)
#define ledc_hal_is_fifo_empty(hal)            ledc_ll_is_fifo_empty(&(hal)->hw)
#define ledc_hal_is_busy(hal)                  ledc_ll_is_busy(&(hal)->hw)
#define ledc_hal_clear_interrupt_status(hal)   ledc_ll_clear_interrupt_status(&(hal)->hw)
#define ledc_hal_disable_interrupt(hal)        ledc_ll_disable_interrupt(&(hal)->hw)

bk_err_t ledc_hal_init(ledc_hal_t *hal);
bk_err_t ledc_hal_deinit(ledc_hal_t *hal);
bk_err_t ledc_hal_configure(ledc_hal_t *hal, const ledc_timing_t *timing);
bk_err_t ledc_hal_apply_ws2812_timing(ledc_hal_t *hal);
bk_err_t ledc_hal_set_write_threshold(ledc_hal_t *hal, uint8_t thr);
bk_err_t ledc_hal_enable_tx_int(ledc_hal_t *hal, bool send_finish, bool need_write);

#ifdef __cplusplus
}
#endif
