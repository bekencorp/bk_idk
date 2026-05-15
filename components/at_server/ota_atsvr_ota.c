/*
 * Copyright 2020-2025 Beken
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sdkconfig.h>

#if CONFIG_AT && (CONFIG_OTA_HTTP || (CONFIG_OTA_HTTPS && CONFIG_OTA_TEST))

#include "ota_atsvr_ota.h"
#include "atsvr_unite.h"
#include <components/log.h>
#include "ota_at_command.h"

#define OTA_ATSVR_TAG "ota_atsvr"

static int ota_at_atsvr_cmd_handle(int sync, int argc, char **argv)
{
	(void)sync;

	if (argc != 1 || argv[0] == NULL) {
		BK_LOGE(OTA_ATSVR_TAG, "Usage: AT+OTA=<url>\r\n");
		atsvr_cmd_rsp_error();
		return -1;
	}

	if (ota_at_schedule_url(argv[0]) != 0) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	atsvr_cmd_rsp_ok();
	return 0;
}

static const struct _atsvr_command ota_atsvr_cmds_table[] = {
	ATSVR_CMD_HADLER("AT+OTA", "start http(s) ota:AT+OTA=<url>",
			 NULL, ota_at_atsvr_cmd_handle, false, 0, 0, NULL, false),
};

void ota_at_atsvr_init(void)
{
	atsvr_register_commands(ota_atsvr_cmds_table,
				sizeof(ota_atsvr_cmds_table) / sizeof(ota_atsvr_cmds_table[0]),
				"ota", NULL);
}

#endif
