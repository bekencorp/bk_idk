// Copyright 2020-2026 Beken
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

#include <os/os.h>
#include <driver/ledc.h>
#include <driver/hal/hal_ledc_types.h>
#include "cli.h"
#include "ledc_hal.h"

#define LEDC_CLI_GPIO_MIN       2
#define LEDC_CLI_GPIO_MAX       31
#define LEDC_CLI_TX_INIT_GPIO   GPIO_18

#define LEDC_CLI_TX_LEN_DEFAULT         100
#define LEDC_CLI_TX_T0_DEFAULT          20
#define LEDC_CLI_TX_T1_DEFAULT          50
#define LEDC_CLI_TX_RESET_PERIOD        100
#define LEDC_CLI_TX_CODE_PERIOD         100
#define LEDC_CLI_TX_NEED_WRITE_THR      0x50
#define LEDC_CLI_TEST_LOOP_DEFAULT      50

static bool cli_ledc_log_ret(const char *op_name, bk_err_t ret)
{
	if (ret != BK_OK) {
		CLI_LOGE("%s failed, ret=%d\r\n", op_name, ret);
		return false;
	}

	return true;
}

static void cli_ledc_help(void)
{
	CLI_LOGI("Usage:\r\n");
	CLI_LOGI("  ledc_driver init|deinit\r\n");
	CLI_LOGI("  ledc test <gpio> <color> [count]\r\n");
	CLI_LOGI("  ledc tx_init [len] [t0] [t1] [gpio]\r\n");
	CLI_LOGI("  gpio: %d-%d (P0/P1 often reserved for UART)\r\n", LEDC_CLI_GPIO_MIN, LEDC_CLI_GPIO_MAX);
	CLI_LOGI("  color: 0=red, 1=green, 2=blue, 3=white\r\n");
	CLI_LOGI("  count: demo loop times, default %d\r\n", LEDC_CLI_TEST_LOOP_DEFAULT);
	CLI_LOGI("  tx_init: len/t0/t1 in hex; default len=0x64 t0=0x14 t1=0x32 gpio=%d\r\n",
		 LEDC_CLI_TX_INIT_GPIO);
	CLI_LOGI("Examples:\r\n");
	CLI_LOGI("  ledc_driver init\r\n");
	CLI_LOGI("  ledc test 18 0 50\r\n");
	CLI_LOGI("  ledc tx_init 0x64 0x14 0x32 18\r\n");
}

static void cli_ledc_tx_init_cmd(uint32_t len, uint32_t t0_high, uint32_t t1_high, gpio_id_t gpio)
{
	ledc_config_t cfg = {0};
	ledc_hal_t hal = {0};
	bk_err_t ret;
	uint32_t i;

	if (len == 0 || len > LEDC_SEND_LENGTH_MAX_BYTES) {
		CLI_LOGE("len(0x%x) must be 1-0x%x\r\n", len, LEDC_SEND_LENGTH_MAX_BYTES);
		return;
	}

	if (t0_high > 0xFF || t1_high > 0xFF) {
		CLI_LOGE("t0/t1 must be 0-0xFF\r\n");
		return;
	}

	ret = bk_ledc_driver_init();
	if (!cli_ledc_log_ret("ledc driver init", ret)) {
		return;
	}

	bk_ledc_deinit();

	cfg.gpio = gpio;
	cfg.use_ws2812_timing = false;
	cfg.timing.reset_period_number = LEDC_CLI_TX_RESET_PERIOD;
	cfg.timing.code_period_number = LEDC_CLI_TX_CODE_PERIOD;
	cfg.timing.t0_high_number = (uint8_t)t0_high;
	cfg.timing.t1_high_number = (uint8_t)t1_high;
	cfg.timing.need_write_thr = LEDC_CLI_TX_NEED_WRITE_THR;
	cfg.timing.send_length = len;

	ret = bk_ledc_init(&cfg);
	if (!cli_ledc_log_ret("ledc init", ret)) {
		return;
	}

	for (i = 1; i <= (len / 2); i++) {
		ledc_hal_write_fifo(&hal, (uint8_t)i);
	}

	ledc_hal_set_write_threshold(&hal, LEDC_CLI_TX_NEED_WRITE_THR);
	ledc_hal_enable_tx_int(&hal, true, true);
	ledc_hal_pulse_start(&hal);

	CLI_LOGI("ledc tx_init gpio=%d len=0x%x t0=0x%x t1=0x%x fifo_words=%d\r\n",
		 gpio, len, t0_high, t1_high, len / 2);
}

static void cli_ledc_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_ledc_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		bk_err_t ret = bk_ledc_driver_init();
		if (!cli_ledc_log_ret("ledc driver init", ret)) {
			return;
		}
		CLI_LOGI("ledc driver init\r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		bk_err_t ret = bk_ledc_driver_deinit();
		if (!cli_ledc_log_ret("ledc driver deinit", ret)) {
			return;
		}
		CLI_LOGI("ledc driver deinit\r\n");
	} else {
		cli_ledc_help();
	}
}

static void cli_ledc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_ledc_help();
		return;
	}

	if (os_strcmp(argv[1], "test") == 0) {
		if (argc < 4) {
			CLI_LOGE("usage: ledc test <gpio> <color> [count]\r\n");
			return;
		}

		uint32_t gpio = os_strtoul(argv[2], NULL, 10);
		uint32_t color = os_strtoul(argv[3], NULL, 10);
		uint32_t loop_count = LEDC_CLI_TEST_LOOP_DEFAULT;

		if (argc >= 5) {
			loop_count = os_strtoul(argv[4], NULL, 10);
			if (loop_count == 0) {
				CLI_LOGE("count must be >= 1\r\n");
				return;
			}
		}

		if (gpio < LEDC_CLI_GPIO_MIN || gpio > LEDC_CLI_GPIO_MAX) {
			CLI_LOGE("gpio(%d) must be %d-%d\r\n", gpio, LEDC_CLI_GPIO_MIN, LEDC_CLI_GPIO_MAX);
			return;
		}

		if (color > LEDC_COLOR_WHITE) {
			CLI_LOGE("color(%d) must be 0-%d\r\n", color, LEDC_COLOR_WHITE);
			return;
		}

		CLI_LOGI("ledc test gpio=%d color=%d count=%d\r\n", gpio, color, loop_count);
		bk_err_t ret = bk_ledc_test((gpio_id_t)gpio, (ledc_color_t)color, loop_count);
		if (!cli_ledc_log_ret("ledc test", ret)) {
			return;
		}
		CLI_LOGI("ledc test done\r\n");
	} else if (os_strcmp(argv[1], "tx_init") == 0) {
		uint32_t len = LEDC_CLI_TX_LEN_DEFAULT;
		uint32_t t0_high = LEDC_CLI_TX_T0_DEFAULT;
		uint32_t t1_high = LEDC_CLI_TX_T1_DEFAULT;
		uint32_t gpio = LEDC_CLI_TX_INIT_GPIO;

		if (argc >= 3) {
			len = os_strtoul(argv[2], NULL, 16);
		}
		if (argc >= 4) {
			t0_high = os_strtoul(argv[3], NULL, 16);
		}
		if (argc >= 5) {
			t1_high = os_strtoul(argv[4], NULL, 16);
		}
		if (argc >= 6) {
			gpio = os_strtoul(argv[5], NULL, 10);
		}

		if (gpio < LEDC_CLI_GPIO_MIN || gpio > LEDC_CLI_GPIO_MAX) {
			CLI_LOGE("gpio(%d) must be %d-%d\r\n", gpio, LEDC_CLI_GPIO_MIN, LEDC_CLI_GPIO_MAX);
			return;
		}

		cli_ledc_tx_init_cmd(len, t0_high, t1_high, (gpio_id_t)gpio);
	} else {
		cli_ledc_help();
	}
}

#define LEDC_CMD_CNT (sizeof(s_ledc_commands) / sizeof(struct cli_command))
static const struct cli_command s_ledc_commands[] = {
	{"ledc_driver", "ledc_driver {init|deinit}", cli_ledc_driver_cmd},
	{"ledc", "ledc test {gpio} {color} [count] | tx_init [len] [t0] [t1] [gpio]", cli_ledc_cmd},
};

int cli_ledc_init(void)
{
	return cli_register_commands(s_ledc_commands, LEDC_CMD_CNT);
}
