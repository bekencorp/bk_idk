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

#include <driver/ledc_types.h>
#include <driver/hal/hal_ledc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init LEDC driver (register ISR, enable clock)
 */
bk_err_t bk_ledc_driver_init(void);

/**
 * @brief Deinit LEDC driver
 */
bk_err_t bk_ledc_driver_deinit(void);

/**
 * @brief Init LEDC hardware (GPIO, timing, FIFO)
 */
bk_err_t bk_ledc_init(const ledc_config_t *cfg);

/**
 * @brief Deinit LEDC hardware
 */
bk_err_t bk_ledc_deinit(void);

/**
 * @brief Start interrupt-driven RGB strip transfer
 *
 * @param pixel_buf RGB buffer, at least led_num * 3 bytes valid until transfer completes
 * @param led_num number of LEDs (1 .. LEDC_LED_NUM_MAX); batches of LEDC_FIFO_BATCH_LED_MAX internally
 */
bk_err_t bk_ledc_write_pixels(uint8_t pixel_buf[][3], uint16_t led_num);

/**
 * @brief Demo: light one LED per loop (ported from ledc_example)
 *
 * @param loop_count demo loop times; 0 uses default (50)
 */
bk_err_t bk_ledc_test(gpio_id_t gpio, ledc_color_t color, uint32_t loop_count);

#ifdef __cplusplus
}
#endif
