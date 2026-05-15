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
#include <os/str.h>
#include <os/mem.h>

typedef struct {
	uint8_t gpio_num;
	gpio_id_t gpio[3];
} gpio_ate_connection_t;

#define GPIO_HIGH_LEVEL   1
#define GPIO_LOW_LEVEL    0
#define GPIO_ATE_MAX_CONNECTION_NUM   ((SOC_GPIO_NUM + 1) / 2)
#define GPIO_ATE_GPIO_BITMAP_WORD_NUM ((SOC_GPIO_NUM + 31) / 32)
#define GPIO_ATE_GPIO_UNUSED          0xFF

#if !defined(GPIO_ATE_TEST_CONFIG)
#error "GPIO_ATE_TEST_CONFIG must be defined in gpio_map.h"
#endif

static const gpio_ate_connection_t s_gpio_default_test_table[] = GPIO_ATE_TEST_CONFIG;
static gpio_ate_connection_t s_gpio_test_table[GPIO_ATE_MAX_CONNECTION_NUM];

static uint8_t s_gpio_test_table_num = 0;
static uint32_t s_gpio_test_bitmap[GPIO_ATE_GPIO_BITMAP_WORD_NUM] = {0};
static uint8_t s_gpio_test_user_configured = 0;

static bk_err_t gpio_ate_input_output_test(const gpio_ate_connection_t *table, uint8_t table_num,
										   gpio_id_t output_gpio, gpio_id_t input_gpio1, gpio_id_t input_gpio2)
{
	uint8_t input_val = 0;
	gpio_id_t gpio_id_tmp = 0;
	gpio_config_t gpio_mode = {
		.io_mode = GPIO_INPUT_ENABLE,
		.pull_mode = GPIO_PULL_UP_EN,
		.func_mode = GPIO_SECOND_FUNC_DISABLE,
	};

	/* input, pull up */
	for (int i = 0; i < table_num; i++) {
		for (int j = 0; j < table[i].gpio_num; j++) {
			gpio_id_tmp = table[i].gpio[j];
			if (gpio_id_tmp == output_gpio) {
				continue;
			}
			BK_LOG_ON_ERR(bk_gpio_set_config(gpio_id_tmp, &gpio_mode));
		}
	}

	BK_LOG_ON_ERR(bk_gpio_enable_output(output_gpio));
	BK_LOG_ON_ERR(bk_gpio_set_output_low(output_gpio));
	for (int i = 0; i < table_num; i++) {
		for (int j = 0; j < table[i].gpio_num; j++) {
			gpio_id_tmp = table[i].gpio[j];
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
	for (int i = 0; i < table_num; i++) {
		for (int j = 0; j < table[i].gpio_num; j++) {
			gpio_id_tmp = table[i].gpio[j];
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

static void gpio_ate_log_result(bk_err_t ret)
{
	if (ret == BK_OK) {
		BK_LOG_RAW("1-ok\r\n");
	} else {
		BK_LOG_RAW("1-failed\r\n");
	}
}

static bk_err_t gpio_ate_run_test_table(const gpio_ate_connection_t *table, uint8_t table_num)
{
	bk_err_t ret = BK_OK;

	BK_LOG_RAW("==========GPIO test start==========\r\n");
	for (int i = 0; i < table_num; i++) {
		if (table[i].gpio_num == 3) {
			BK_LOG_RAW("  GPIO%d --- GPIO%d --- GPIO%d test: ", table[i].gpio[0], table[i].gpio[1], table[i].gpio[2]);
			ret = gpio_ate_input_output_test(table, table_num, table[i].gpio[0], table[i].gpio[1], table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			ret = gpio_ate_input_output_test(table, table_num, table[i].gpio[1], table[i].gpio[0], table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			ret = gpio_ate_input_output_test(table, table_num, table[i].gpio[2], table[i].gpio[0], table[i].gpio[1]);
			if (ret != BK_OK) {
				goto exit;
			}
			BK_LOG_RAW("pass\r\n");
		} else {
			BK_LOG_RAW("  GPIO%d --- GPIO%d test: ", table[i].gpio[0], table[i].gpio[1]);
			ret = gpio_ate_input_output_test(table, table_num, table[i].gpio[0], table[i].gpio[1], table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			ret = gpio_ate_input_output_test(table, table_num, table[i].gpio[1], table[i].gpio[0], table[i].gpio[2]);
			if (ret != BK_OK) {
				goto exit;
			}
			BK_LOG_RAW("pass\r\n");
		}
	}
	ret = BK_OK;
	goto exit;

exit:
	gpio_ate_log_result(ret);
	return ret;
}

static void gpio_ate_dump_test_groups(const gpio_ate_connection_t *table, uint8_t table_num)
{
	BK_LOG_RAW("==========GPIO test debug groups==========\r\n");
	for (int i = 0; i < table_num; i++) {
		if (table[i].gpio_num == 3) {
			BK_LOG_RAW("  GPIO%d --- GPIO%d --- GPIO%d\r\n", table[i].gpio[0], table[i].gpio[1], table[i].gpio[2]);
		} else {
			BK_LOG_RAW("  GPIO%d --- GPIO%d\r\n", table[i].gpio[0], table[i].gpio[1]);
		}
	}
	BK_LOG_RAW("==========GPIO test debug groups end======\r\n");
}

bk_err_t bk_gpio_ate_test(void)
{
	return gpio_ate_run_test_table(s_gpio_default_test_table, sizeof(s_gpio_default_test_table) / sizeof(s_gpio_default_test_table[0]));
}

static uint8_t gpio_ate_gpio_is_used(const uint32_t bitmap[GPIO_ATE_GPIO_BITMAP_WORD_NUM], uint32_t gpio_id)
{
	uint32_t word = gpio_id / 32;
	uint32_t bit = gpio_id % 32;

	return ((bitmap[word] >> bit) & 0x1) != 0;
}

static void gpio_ate_gpio_set_used(uint32_t bitmap[GPIO_ATE_GPIO_BITMAP_WORD_NUM], uint32_t gpio_id)
{
	uint32_t word = gpio_id / 32;
	uint32_t bit = gpio_id % 32;

	bitmap[word] |= (1U << bit);
}

static bk_err_t gpio_ate_parse_group_token(gpio_ate_connection_t *group, const char *token)
{
	bk_err_t ret = BK_OK;
	char pair_buf[16] = {0};
	char *split = NULL;
	char *split2 = NULL;
	char *end = NULL;
	uint32_t gpio0 = 0;
	uint32_t gpio1 = 0;
	uint32_t gpio2 = GPIO_ATE_GPIO_UNUSED;

	if ((group == NULL) || (token == NULL) || (os_strlen(token) >= sizeof(pair_buf))) {
		ret = BK_ERR_PARAM;
		goto exit;
	}

	os_strcpy(pair_buf, token);
	split = os_strchr(pair_buf, '_');
	if ((split == NULL) || (split == pair_buf) || (*(split + 1) == '\0')) {
		ret = BK_ERR_PARAM;
		goto exit;
	}
	split2 = os_strchr(split + 1, '_');
	if (split2 && ((split2 == (split + 1)) || (*(split2 + 1) == '\0') || (os_strchr(split2 + 1, '_') != NULL))) {
		ret = BK_ERR_PARAM;
		goto exit;
	}

	*split = '\0';
	gpio0 = os_strtoul(pair_buf, &end, 10);
	if ((end == pair_buf) || (*end != '\0')) {
		ret = BK_ERR_PARAM;
		goto exit;
	}

	if (split2) {
		*split2 = '\0';
	}
	gpio1 = os_strtoul(split + 1, &end, 10);
	if ((end == (split + 1)) || (*end != '\0')) {
		ret = BK_ERR_PARAM;
		goto exit;
	}

	if (split2) {
		gpio2 = os_strtoul(split2 + 1, &end, 10);
		if ((end == (split2 + 1)) || (*end != '\0')) {
			ret = BK_ERR_PARAM;
			goto exit;
		}
	}

	if ((gpio0 >= SOC_GPIO_NUM) || (gpio1 >= SOC_GPIO_NUM) || ((gpio2 != GPIO_ATE_GPIO_UNUSED) && (gpio2 >= SOC_GPIO_NUM))) {
		ret = BK_ERR_PARAM;
		goto exit;
	}

	group->gpio_num = split2 ? 3 : 2;
	group->gpio[0] = (gpio_id_t)gpio0;
	group->gpio[1] = (gpio_id_t)gpio1;
	group->gpio[2] = (gpio_id_t)gpio2;

exit:
	return ret;
}

bk_err_t bk_gpio_ate_test_config(int pair_argc, char **pair_argv)
{
	bk_err_t ret = BK_OK;
	uint32_t bitmap_tmp[GPIO_ATE_GPIO_BITMAP_WORD_NUM] = {0};

	if (pair_argc < 1) {
		BK_LOG_RAW("usage: gpio_ate_test config g0_g1 [g2_g3 ...]\r\n");
		ret = BK_ERR_PARAM;
		goto exit;
	}

	if (s_gpio_test_user_configured == 0) {
		s_gpio_test_user_configured = 1;
		s_gpio_test_table_num = 0;
		os_memset(s_gpio_test_bitmap, 0, sizeof(s_gpio_test_bitmap));
	}

	if ((uint32_t)s_gpio_test_table_num + pair_argc > GPIO_ATE_MAX_CONNECTION_NUM) {
		BK_LOG_RAW("table full\r\n");
		ret = BK_ERR_PARAM;
		goto exit;
	}

	for (int i = 0; i < pair_argc; i++) {
		gpio_ate_connection_t group = {0};
		ret = gpio_ate_parse_group_token(&group, pair_argv[i]);

		if (ret != BK_OK) {
			BK_LOG_RAW("invalid group:%s\r\n", pair_argv[i]);
			ret = BK_ERR_PARAM;
			goto exit;
		}

		for (int j = 0; j < group.gpio_num; j++) {
			uint32_t gpio_id = group.gpio[j];
			if (gpio_ate_gpio_is_used(s_gpio_test_bitmap, gpio_id) || gpio_ate_gpio_is_used(bitmap_tmp, gpio_id)) {
				BK_LOG_RAW("gpio overlap:%d\r\n", gpio_id);
				ret = BK_ERR_PARAM;
				goto exit;
			}
			gpio_ate_gpio_set_used(bitmap_tmp, gpio_id);
		}
	}

	for (int i = 0; i < pair_argc; i++) {
		gpio_ate_connection_t group = {0};

		gpio_ate_parse_group_token(&group, pair_argv[i]);
		s_gpio_test_table[s_gpio_test_table_num] = group;
		s_gpio_test_table_num++;
	}

	for (int i = 0; i < GPIO_ATE_GPIO_BITMAP_WORD_NUM; i++) {
		s_gpio_test_bitmap[i] |= bitmap_tmp[i];
	}

	BK_LOG_RAW("config ok, group:%d total:%d\r\n", pair_argc, s_gpio_test_table_num);
	ret = BK_OK;

exit:
	gpio_ate_log_result(ret);
	return ret;
}

bk_err_t bk_gpio_ate_test_start(void)
{
	if (s_gpio_test_table_num == 0) {
		BK_LOG_RAW("empty config\r\n");
		return BK_ERR_PARAM;
	}

	return gpio_ate_run_test_table(s_gpio_test_table, s_gpio_test_table_num);
}

bk_err_t bk_gpio_ate_test_show(void)
{
	if (s_gpio_test_table_num > 0) {
		gpio_ate_dump_test_groups(s_gpio_test_table, s_gpio_test_table_num);
	} else {
		gpio_ate_dump_test_groups(s_gpio_default_test_table, sizeof(s_gpio_default_test_table) / sizeof(s_gpio_default_test_table[0]));
	}
	gpio_ate_log_result(BK_OK);
	return BK_OK;
}
