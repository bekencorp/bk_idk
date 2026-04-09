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

#include "cli_statis.h"

#if CONFIG_CLI

#include "statis.h"

#include <string.h>

#include "cli.h"
#include "cli_common.h"
#include <components/log.h>

#define STATIS_CLI_ARG_ERR_REC_CNT 20
#define STATIS_CLI_TAG "statis_cli"

static void cli_statis_action_handler(void **argtable)
{
	struct arg_lit *help = (struct arg_lit *)argtable[0];
	struct arg_str *action = (struct arg_str *)argtable[1];

	if (help->count > 0) {
		print_help(argtable[0], argtable);
		return;
	}

	if (action->count > 0) {
		const char *cmd = action->sval[0];

		if (strcmp(cmd, "load") == 0) {
			if (statis_load_from_flash()) {
				BK_RAW_LOGI(STATIS_CLI_TAG, "statis: loaded valid stats from flash\r\n");
			} else {
				BK_RAW_LOGI(STATIS_CLI_TAG, "statis: no valid flash image, RAM cleared to defaults\r\n");
			}
		} else if (strcmp(cmd, "save") == 0) {
			statis_save_to_flash();
			BK_RAW_LOGI(STATIS_CLI_TAG, "statis: saved stats to flash\r\n");
		} else if (strcmp(cmd, "show") == 0) {
			statis_print_all_results();
			BK_RAW_LOGI(STATIS_CLI_TAG, "statis: show complete\r\n");
		} else {
			BK_RAW_LOGI(STATIS_CLI_TAG, "statis: unknown action '%s' (use load|save|show)\r\n", cmd);
		}
		return;
	}

	print_help(argtable[0], argtable);
}

static void cli_statis_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
	struct arg_str *action =
		arg_str0("a", "action", "<load|save|show>", "Load, save, or print reliability statistics");
	struct arg_end *end = arg_end(STATIS_CLI_ARG_ERR_REC_CNT);

	void *argtable[] = { help, action, end };
	int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	common_cmd_handler(argc, argv, argtable, argtable_size, cli_statis_action_handler);
}

#define STATIS_CLI_CMD_CNT (sizeof(s_statis_cli_commands) / sizeof(struct cli_command))
static const struct cli_command s_statis_cli_commands[] = {
	{ "statis", "statis [-h] [-a load|save|show]", cli_statis_cmd },
};

void statis_cli_register_cmds(void)
{
	(void)cli_register_commands(s_statis_cli_commands, STATIS_CLI_CMD_CNT);
}

#endif /* CONFIG_CLI */
