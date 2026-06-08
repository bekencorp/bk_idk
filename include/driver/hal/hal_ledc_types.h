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

#include <common/bk_err.h>
#include <driver/hal/hal_gpio_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** FIFO 260 bytes; max LEDs per FIFO fill batch (avoid overflow, same as ledc_example) */
#define LEDC_FIFO_DEPTH_BYTES           260
#define LEDC_FIFO_BATCH_LED_MAX         (LEDC_FIFO_DEPTH_BYTES / 3)

/** send_number: 8-bit low + 4-bit high */
#define LEDC_SEND_LENGTH_MAX_BYTES      0xFFF
#define LEDC_LED_NUM_MAX                (LEDC_SEND_LENGTH_MAX_BYTES / 3)

typedef struct {
	uint8_t reset_period_number;
	uint8_t code_period_number;
	uint8_t t0_high_number;
	uint8_t t1_high_number;
	uint8_t need_write_thr;
	uint32_t send_length;
} ledc_timing_t;

typedef struct {
	gpio_id_t gpio;
	ledc_timing_t timing;
	bool use_ws2812_timing;
} ledc_config_t;

typedef enum {
	LEDC_COLOR_RED = 0,
	LEDC_COLOR_GREEN,
	LEDC_COLOR_BLUE,
	LEDC_COLOR_WHITE,
} ledc_color_t;

#ifdef __cplusplus
}
#endif
