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

#include <string.h>

#include "cli.h"
#include <components/log.h>

#define FLASH_IMPORT_CLI_TAG "flash_import_cli"

static void flash_import_cli_show_usage(void)
{
	BK_RAW_LOGI(FLASH_IMPORT_CLI_TAG, "usage: flash_import endurance {start|stop|show}\r\n");
}

static void cli_flash_import_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc == 2) {
		if (strcmp(argv[1], "endurance") == 0) {
			goto usage;
		}
	}

	if (argc == 3) {
		if (strcmp(argv[1], "endurance") == 0) {
			if (strcmp(argv[2], "start") == 0) {
				flash_import_case_endurance_start();
			} else if (strcmp(argv[2], "stop") == 0) {
				flash_import_case_endurance_stop();
			} else if (strcmp(argv[2], "show") == 0) {
				flash_import_case_endurance_show();
			} else {
				goto usage;
			}
			return;
		}
	}

usage:
	flash_import_cli_show_usage();
}

#define FLASH_IMPORT_CLI_CMD_CNT (sizeof(s_flash_import_cli_commands) / sizeof(s_flash_import_cli_commands[0]))
static const struct cli_command s_flash_import_cli_commands[] = {
	{ "flash_import", "flash_import endurance", cli_flash_import_cmd },
};

void flash_import_cli_register_cmds(void)
{
	(void)cli_register_commands(s_flash_import_cli_commands, FLASH_IMPORT_CLI_CMD_CNT);
}

#endif /* CONFIG_CLI */
