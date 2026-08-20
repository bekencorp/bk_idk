// Copyright 2020-2025 Beken
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

#include "cli_flash_import.h"

#if CONFIG_CLI

#include "flash_import_internal.h"
#include "sdkconfig.h"

#include <string.h>

#include "cli.h"
#include <components/log.h>
#include <os/str.h>

#define FLASH_IMPORT_CLI_TAG "flash_import_cli"

struct flash_import_subcmd {
	const char *name;
	const char *usage;
	int (*handler)(int argc, char **argv);
	uint8_t subargc_min;
	uint8_t subargc_max;
};

static int flash_import_zero_read_cmd(int argc, char **argv)
{
	if (argc < 1) {
		return -1;
	}

	if (strcmp(argv[0], "start") == 0) {
		flash_import_case_zero_read_start();
		return 0;
	}

	if (strcmp(argv[0], "stop") == 0) {
		flash_import_case_zero_read_stop();
		return 0;
	}

	if (strcmp(argv[0], "show") == 0) {
		flash_import_case_zero_read_show();
		return 0;
	}

	return -1;
}

static int flash_import_endurance_cmd(int argc, char **argv)
{
	if (argc < 1) {
		return -1;
	}

	if (strcmp(argv[0], "start") == 0) {
		flash_import_case_endurance_start();
		return 0;
	}

	if (strcmp(argv[0], "stop") == 0) {
		flash_import_case_endurance_stop();
		return 0;
	}

	if (strcmp(argv[0], "show") == 0) {
		flash_import_case_endurance_show();
		return 0;
	}

	return -1;
}

static int flash_import_driver_cmd(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	flash_import_case_driver_run();
	return 0;
}

static int flash_import_loop_cmd(int argc, char **argv)
{
	if (argc < 1) {
		return -1;
	}

	if (strcmp(argv[0], "start") == 0) {
		flash_import_case_loop_start();
		return 0;
	}

	if (strcmp(argv[0], "stop") == 0) {
		flash_import_case_loop_stop();
		return 0;
	}

	if (strcmp(argv[0], "show") == 0) {
		flash_import_case_loop_show();
		return 0;
	}

	return -1;
}

static int flash_import_soft_erase_cmd(int argc, char **argv)
{
	if (argc < 1) {
		return -1;
	}

	if (strcmp(argv[0], "start") == 0) {
		flash_import_case_soft_erase_start();
		return 0;
	}

	if (strcmp(argv[0], "stop") == 0) {
		flash_import_case_soft_erase_stop();
		return 0;
	}

	if (strcmp(argv[0], "show") == 0) {
		flash_import_case_soft_erase_show();
		return 0;
	}

	return -1;
}

static int flash_import_disturb_cmd(int argc, char **argv)
{
	if (argc < 1) {
		return -1;
	}

	if (strcmp(argv[0], "stop") == 0) {
		flash_import_case_pd_stop();
		return 0;
	}

	if (strcmp(argv[0], "show") == 0) {
		flash_import_case_pd_show();
		return 0;
	}

	uint32_t mode;

	if (strcmp(argv[0], "1to1") == 0) {
		mode = 0U;
	} else if (strcmp(argv[0], "0to0") == 0) {
		mode = 1U;
	} else if (strcmp(argv[0], "0to1") == 0) {
		mode = 2U;
	} else {
		return -1;
	}

	flash_import_case_pd_start(mode);
	return 0;
}

#if CONFIG_FLASH_IMPORT_SLEEP_TEST
static int flash_import_lvsleep_cmd(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	flash_import_case_sleep_run(false);
	return 0;
}

static int flash_import_deepsleep_cmd(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	flash_import_case_sleep_run(true);
	return 0;
}
#endif

static const struct flash_import_subcmd s_flash_import_subcmds[] = {
	{ "endurance",  "usage: flash_import endurance {start|stop|show}",        flash_import_endurance_cmd,  1, 1 },
	{ "zero_read",  "usage: flash_import zero_read {start|stop|show}",        flash_import_zero_read_cmd,  1, 1 },
	{ "loop",       "usage: flash_import loop {start|stop|show}",             flash_import_loop_cmd,       1, 1 },
	{ "disturb",    "usage: flash_import disturb {1to1|0to0|0to1|stop|show}", flash_import_disturb_cmd,    1, 1 },
	{ "soft_erase", "usage: flash_import soft_erase {start|stop|show}",       flash_import_soft_erase_cmd, 1, 1 },
#if CONFIG_FLASH_IMPORT_SLEEP_TEST
	{ "lvsleep",    "usage: flash_import lvsleep",                            flash_import_lvsleep_cmd,    0, 0 },
	{ "deepsleep",  "usage: flash_import deepsleep",                          flash_import_deepsleep_cmd,  0, 0 },
#endif
	{ "driver",     "usage: flash_import driver",                             flash_import_driver_cmd,     0, 0 },
};

#define FLASH_IMPORT_SUBCMD_CNT (sizeof(s_flash_import_subcmds) / sizeof(s_flash_import_subcmds[0]))

static void flash_import_cli_show_usage(void)
{
	uint32_t i = 0;

	for (i = 0; i < FLASH_IMPORT_SUBCMD_CNT; i++) {
		BK_RAW_LOGI(FLASH_IMPORT_CLI_TAG, "%s\r\n", s_flash_import_subcmds[i].usage);
	}
}

static void cli_flash_import_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t i = 0;
	int ret = -1;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		goto usage;
	}

	for (i = 0; i < FLASH_IMPORT_SUBCMD_CNT; i++) {
		const struct flash_import_subcmd *subcmd = &s_flash_import_subcmds[i];
		int subargc = argc - 2;

		if (strcmp(argv[1], subcmd->name) != 0) {
			continue;
		}

		if (subargc < (int)subcmd->subargc_min || subargc > (int)subcmd->subargc_max) {
			goto usage;
		}

		ret = subcmd->handler(subargc, &argv[2]);
		if (ret == 0) {
			return;
		}

		goto usage;
	}

usage:
	flash_import_cli_show_usage();
}

#define FLASH_IMPORT_CLI_CMD_CNT (sizeof(s_flash_import_clis) / sizeof(s_flash_import_clis[0]))
static const struct cli_command s_flash_import_clis[] = {
	{ "flash_import", "flash_import subcommands", cli_flash_import_cmd },
};

void flash_import_cli_register_cmds(void)
{
	bk_err_t ret = cli_register_commands(s_flash_import_clis, FLASH_IMPORT_CLI_CMD_CNT);

	if (ret != BK_OK) {
		BK_RAW_LOGI(FLASH_IMPORT_CLI_TAG, "flash_import cli reg fail:%d\r\n", ret);
	}
}

#endif /* CONFIG_CLI */
