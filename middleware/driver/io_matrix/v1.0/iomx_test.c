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

#include <os/os.h>
#include "cli.h"
#include <driver/io_matrix.h>
#include "iomx_driver.h"

static void cli_iomx_help(void)
{
	CLI_LOGI("iomx_driver init\n");
	CLI_LOGI("iomx_driver deinit\n");
	CLI_LOGI("iomx set [id]\n");
	CLI_LOGI("iomx get [id]\n");
}

static void cli_iomx_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_iomx_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_iomx_driver_init());
		CLI_LOGI("iomx driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_iomx_driver_deinit());
		CLI_LOGI("iomx driver deinit\n");
	} else {
		cli_iomx_help();
		return;
	}
}

static void cli_iomx_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_iomx_help();
		return;
	}

	if (os_strcmp(argv[1], "get") == 0) {
                uint32_t ret;
		uint32_t gpio_id = os_strtoul(argv[2], NULL, 10);

		ret = bk_iomx_get_gpio_func_code(gpio_id);

		CLI_LOGI("iomx gpio_id[%d] get func code = %d\n", gpio_id, ret);
	} else if (os_strcmp(argv[1], "set") == 0) {
		uint32_t gpio_id = os_strtoul(argv[2], NULL, 10);
		uint32_t code = os_strtoul(argv[3], NULL, 10);

		BK_LOG_ON_ERR(bk_iomx_set_gpio_func(gpio_id, code));
		CLI_LOGI("iomx set func code[%d] of gpio id:%d\n", code, gpio_id);
	} else {
		cli_iomx_help();
		return;
	}
}

#define IOMX_CMD_CNT (sizeof(s_iomx_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_iomx_commands[] = {
	{"iomx_driver", "{init|deinit}", cli_iomx_driver_cmd},
	{"iomx", "iomx {start|stop|feed} [...]", cli_iomx_cmd}
};

int bk_iomx_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_iomx_driver_init());
	return cli_register_module_test_feature(s_iomx_commands, IOMX_CMD_CNT);
}

