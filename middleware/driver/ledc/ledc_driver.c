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

#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/ledc.h>
#include <os/mem.h>
#include <os/os.h>
#include "gpio_driver.h"
#include "ledc_driver.h"
#include "ledc_hal.h"
#include "sys_driver.h"
#include <modules/pm.h>

#define LEDC_RETURN_ON_NOT_INIT() do { \
	if (!s_ledc_driver_is_init) { \
		return BK_ERR_LEDC_NOT_INIT; \
	} \
} while (0)

#define LEDC_RETURN_ON_HW_NOT_INIT() do { \
	if (!s_ledc.hw_inited) { \
		return BK_ERR_LEDC_NOT_INIT; \
	} \
} while (0)

typedef struct {
	ledc_hal_t hal;
	bool hw_inited;
	gpio_id_t gpio;
	uint16_t led_num;
	uint16_t led_count;
	uint8_t (*pixel_buf)[3];
} ledc_driver_t;

static ledc_driver_t s_ledc;
static bool s_ledc_driver_is_init;

static void ledc_clock_enable(void)
{
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_LED, CLK_PWR_CTRL_PWR_UP);
}

static void ledc_clock_disable(void)
{
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_LED, CLK_PWR_CTRL_PWR_DOWN);
}

static void ledc_interrupt_enable(void)
{
	sys_drv_int_enable(LED_INTERRUPT_CTRL_BIT);
}

static void ledc_interrupt_disable(void)
{
	sys_drv_int_disable(LED_INTERRUPT_CTRL_BIT);
}

static void ledc_init_gpio(gpio_id_t gpio)
{
	bk_gpio_map_dev_to_pin(gpio, GPIO_DEV_LEDC);
	bk_gpio_pull_up(gpio);
}

static void ledc_isr(void)
{
	uint32_t int_status = ledc_hal_get_interrupt_status(&s_ledc.hal);

	if (int_status & LEDC_F_NEED_WR_INT) {
		uint16_t start_count = s_ledc.led_count;
		uint16_t batch_end = s_ledc.led_count + LEDC_FIFO_BATCH_LED_MAX;

		if (batch_end > s_ledc.led_num) {
			batch_end = s_ledc.led_num;
		}

		while (s_ledc.led_count < batch_end) {
			// Order is configurable; TC1903Z is currently tested as R → G → B
			ledc_hal_write_fifo(&s_ledc.hal, s_ledc.pixel_buf[s_ledc.led_count][0]);
			ledc_hal_write_fifo(&s_ledc.hal, s_ledc.pixel_buf[s_ledc.led_count][1]);
			ledc_hal_write_fifo(&s_ledc.hal, s_ledc.pixel_buf[s_ledc.led_count][2]);
			s_ledc.led_count++;
		}

		if (s_ledc.led_count > start_count) {
			ledc_hal_pulse_start(&s_ledc.hal);
		}

		if (s_ledc.led_count >= s_ledc.led_num) {
			ledc_hal_disable_interrupt(&s_ledc.hal);
		}
	}

	ledc_hal_clear_interrupt_status(&s_ledc.hal);
}

static void ledc_fill_first_batch(void)
{
	uint16_t batch_end = LEDC_FIFO_BATCH_LED_MAX;

	if (batch_end > s_ledc.led_num) {
		batch_end = s_ledc.led_num;
	}

	s_ledc.led_count = 0;
	while (s_ledc.led_count < batch_end) {
		// Order is configurable; TC1903Z is currently tested as R → G → B
		ledc_hal_write_fifo(&s_ledc.hal, s_ledc.pixel_buf[s_ledc.led_count][0]);
		ledc_hal_write_fifo(&s_ledc.hal, s_ledc.pixel_buf[s_ledc.led_count][1]);
		ledc_hal_write_fifo(&s_ledc.hal, s_ledc.pixel_buf[s_ledc.led_count][2]);
		s_ledc.led_count++;
	}
}

bk_err_t bk_ledc_driver_init(void)
{
	if (s_ledc_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_ledc, 0, sizeof(s_ledc));
	ledc_hal_init(&s_ledc.hal);
	ledc_clock_enable();
	ledc_interrupt_enable();
	bk_int_isr_register(INT_SRC_LED, ledc_isr, NULL);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_LEDC, PM_POWER_MODULE_STATE_ON);
	s_ledc_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_ledc_driver_deinit(void)
{
	LEDC_RETURN_ON_NOT_INIT();

	if (s_ledc.hw_inited) {
		bk_ledc_deinit();
	}

	bk_int_isr_unregister(INT_SRC_LED);
	ledc_interrupt_disable();
	ledc_clock_disable();
	ledc_hal_deinit(&s_ledc.hal);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_LEDC, PM_POWER_MODULE_STATE_OFF);
	s_ledc_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_ledc_init(const ledc_config_t *cfg)
{
	BK_RETURN_ON_NULL(cfg);
	LEDC_RETURN_ON_NOT_INIT();

	if (s_ledc.hw_inited) {
		return BK_OK;
	}

	s_ledc.gpio = cfg->gpio;
	ledc_init_gpio(s_ledc.gpio);
	ledc_hal_init(&s_ledc.hal);

	if (cfg->use_ws2812_timing) {
		ledc_hal_apply_ws2812_timing(&s_ledc.hal);
	} else {
		ledc_hal_configure(&s_ledc.hal, &cfg->timing);
	}

	ledc_hal_clear_interrupt_status(&s_ledc.hal);
	s_ledc.hw_inited = true;
	LEDC_LOGI("ledc init ok, gpio:%d\r\n", s_ledc.gpio);

	return BK_OK;
}

bk_err_t bk_ledc_deinit(void)
{
	LEDC_RETURN_ON_NOT_INIT();

	if (!s_ledc.hw_inited) {
		return BK_OK;
	}

	ledc_hal_disable_interrupt(&s_ledc.hal);
	ledc_hal_deinit(&s_ledc.hal);
	s_ledc.hw_inited = false;
	s_ledc.pixel_buf = NULL;
	s_ledc.led_num = 0;
	s_ledc.led_count = 0;

	return BK_OK;
}

bk_err_t bk_ledc_write_pixels(uint8_t pixel_buf[][3], uint16_t led_num)
{
	LEDC_RETURN_ON_HW_NOT_INIT();
	BK_RETURN_ON_NULL(pixel_buf);

	if (led_num == 0 || led_num > LEDC_LED_NUM_MAX) {
		return BK_ERR_LEDC_PARAM;
	}

	/* FIFO 260B / 3B per LED: write first batch only, avoid FIFO overflow. ISR refills on need_write. */
	s_ledc.pixel_buf = pixel_buf;
	s_ledc.led_num = led_num;
	s_ledc.led_count = 0;

	ledc_hal_set_send_length(&s_ledc.hal, (uint32_t)led_num * 3);
	ledc_hal_set_write_threshold(&s_ledc.hal, LEDC_V_WS2812_WRITE_THR);
	ledc_hal_enable_tx_int(&s_ledc.hal, true, true);

	ledc_fill_first_batch();
	ledc_hal_pulse_start(&s_ledc.hal);

	return BK_OK;
}

#define LEDC_TEST_LED_NUM       50
#define LEDC_TEST_LOOP_DEFAULT  50

static uint8_t s_ledc_test_buf[LEDC_TEST_LED_NUM][3];

bk_err_t bk_ledc_test(gpio_id_t gpio, ledc_color_t color, uint32_t loop_count)
{
	ledc_config_t cfg = {
		.gpio = gpio,
		.use_ws2812_timing = true,
	};
	uint8_t test_index = 0;
	bk_err_t ret;

	ret = bk_ledc_driver_init();
	if (ret != BK_OK) {
		return ret;
	}

	ret = bk_ledc_init(&cfg);
	if (ret != BK_OK) {
		return ret;
	}

	if (loop_count == 0) {
		loop_count = LEDC_TEST_LOOP_DEFAULT;
	}

	for (uint32_t loop = 0; loop < loop_count; loop++) {
		os_memset(s_ledc_test_buf, 0, sizeof(s_ledc_test_buf));

		switch (color) {
		case LEDC_COLOR_RED:
			s_ledc_test_buf[test_index][0] = 0;
			s_ledc_test_buf[test_index][1] = 255;
			s_ledc_test_buf[test_index][2] = 0;
			break;
		case LEDC_COLOR_GREEN:
			s_ledc_test_buf[test_index][0] = 255;
			s_ledc_test_buf[test_index][1] = 0;
			s_ledc_test_buf[test_index][2] = 0;
			break;
		case LEDC_COLOR_BLUE:
			s_ledc_test_buf[test_index][0] = 0;
			s_ledc_test_buf[test_index][1] = 0;
			s_ledc_test_buf[test_index][2] = 255;
			break;
		default:
			s_ledc_test_buf[test_index][0] = 255;
			s_ledc_test_buf[test_index][1] = 255;
			s_ledc_test_buf[test_index][2] = 255;
			break;
		}

		test_index++;
		if (test_index >= LEDC_TEST_LED_NUM) {
			test_index = 0;
		}

		bk_ledc_write_pixels(s_ledc_test_buf, LEDC_TEST_LED_NUM);
		rtos_delay_milliseconds(100);
	}

	LEDC_LOGI("ledc test end\r\n");
	return BK_OK;
}
