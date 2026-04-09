// Copyright 2023-2024 Beken
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

#include "gpio_driver_base.h"
#include "gpio_driver.h"
#include "driver/gpio.h"
#include <soc/gpio_map.h>

typedef struct {
	uint8_t gpio_num;
	gpio_id_t gpio[3];
} gpio_ate_connection_t;

#define GPIO_HIGH_LEVEL   1
#define GPIO_LOW_LEVEL    0

#if !defined(GPIO_ATE_TEST_CONFIG) || !defined(GPIO_ATE_TEST_LIST)
#error "GPIO_ATE_TEST_CONFIG and GPIO_ATE_TEST_LIST must be defined in gpio_map.h"
#endif

static const gpio_id_t s_gpio_list[] = GPIO_ATE_TEST_LIST;
static const gpio_ate_connection_t s_gpio_test_table[] = GPIO_ATE_TEST_CONFIG;

static bk_err_t gpio_ate_input_output_test(gpio_id_t output_gpio, gpio_id_t input_gpio1, gpio_id_t input_gpio2)
{
	uint8_t input_val = 0;
	gpio_id_t gpio_id_tmp = 0;
	gpio_config_t gpio_mode = {
		.io_mode = GPIO_INPUT_ENABLE,
		.pull_mode = GPIO_PULL_UP_EN,
		.func_mode = GPIO_SECOND_FUNC_DISABLE,
	};

	/* input, pull up */
	for (int i = 0; i < (sizeof(s_gpio_list) / sizeof(s_gpio_list[0])); i++) {
		if (s_gpio_list[i] == output_gpio) {
			continue;
		}
		BK_LOG_ON_ERR(bk_gpio_set_config(s_gpio_list[i], &gpio_mode));
	}

	BK_LOG_ON_ERR(bk_gpio_enable_output(output_gpio));
	BK_LOG_ON_ERR(bk_gpio_set_output_low(output_gpio));
	for (int i = 0; i < sizeof(s_gpio_test_table) / sizeof(s_gpio_test_table[0]); i++) {
		for (int j = 0; j < s_gpio_test_table[i].gpio_num; j++) {
			gpio_id_tmp = s_gpio_test_table[i].gpio[j];
			if (gpio_id_tmp == output_gpio) {
				continue;
			}
			input_val = bk_gpio_get_input(gpio_id_tmp);
			if (gpio_id_tmp == input_gpio1 || gpio_id_tmp == input_gpio2) {
				if (input_val != GPIO_LOW_LEVEL) {
					BK_LOG_RAW("failed, GPIO%d output value:0, GPIO%d input value:%d\r\n", output_gpio, gpio_id_tmp, input_val);
					return BK_ERR_GPIO_ATE_TEST_FAILED;
				}
			} else {
				if (input_val != GPIO_HIGH_LEVEL) {
					BK_LOG_RAW("failed, GPIO%d and GPIO%d are short-circuited\r\n", output_gpio, gpio_id_tmp);
					return BK_ERR_GPIO_ATE_TEST_FAILED;
				}
			}
		}
	}

	BK_LOG_ON_ERR(bk_gpio_set_output_high(output_gpio));
	for (int i = 0; i < sizeof(s_gpio_test_table) / sizeof(s_gpio_test_table[0]); i++) {
		for (int j = 0; j < s_gpio_test_table[i].gpio_num; j++) {
			gpio_id_tmp = s_gpio_test_table[i].gpio[j];
			if (gpio_id_tmp == output_gpio) {
				continue;
			}
			input_val = bk_gpio_get_input(gpio_id_tmp);
			if (input_val != GPIO_HIGH_LEVEL) {
				BK_LOG_RAW("failed, GPIO%d output value:1, GPIO%d input value:%d\r\n", output_gpio, gpio_id_tmp, input_val);
				return BK_ERR_GPIO_ATE_TEST_FAILED;
			}
		}
	}

	return BK_OK;
}

bk_err_t bk_gpio_ate_test(void)
{
	bk_err_t ret = BK_OK;

	BK_LOG_RAW("==========GPIO test start==========\r\n");
	for (int i = 0; i < sizeof(s_gpio_test_table) / sizeof(s_gpio_test_table[0]); i++) {
		if (s_gpio_test_table[i].gpio_num == 3) {
			BK_LOG_RAW("  GPIO%d --- GPIO%d --- GPIO%d test: ", s_gpio_test_table[i].gpio[0], s_gpio_test_table[i].gpio[1], s_gpio_test_table[i].gpio[2]);
			ret = gpio_ate_input_output_test(s_gpio_test_table[i].gpio[0], s_gpio_test_table[i].gpio[1], s_gpio_test_table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			ret = gpio_ate_input_output_test(s_gpio_test_table[i].gpio[1], s_gpio_test_table[i].gpio[0], s_gpio_test_table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			ret = gpio_ate_input_output_test(s_gpio_test_table[i].gpio[2], s_gpio_test_table[i].gpio[0], s_gpio_test_table[i].gpio[1]);
			if (ret != BK_OK) {
				goto exit;
			}
			BK_LOG_RAW("pass\r\n");
		} else {
			BK_LOG_RAW("  GPIO%d --- GPIO%d test: ", s_gpio_test_table[i].gpio[0], s_gpio_test_table[i].gpio[1]);
			ret = gpio_ate_input_output_test(s_gpio_test_table[i].gpio[0], s_gpio_test_table[i].gpio[1], s_gpio_test_table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			ret = gpio_ate_input_output_test(s_gpio_test_table[i].gpio[1], s_gpio_test_table[i].gpio[0], s_gpio_test_table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			BK_LOG_RAW("pass\r\n");
		}
	}
	BK_LOG_RAW("1-ok\r\n");
	return BK_OK;

exit:
	BK_LOG_RAW("1-failed\r\n");
	return ret;
}
