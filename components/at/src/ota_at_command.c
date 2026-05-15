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

#if (CONFIG_AT_CMD || CONFIG_AT) && (CONFIG_OTA_HTTP || (CONFIG_OTA_HTTPS && CONFIG_OTA_TEST))

#include "ota_at_command.h"

#include <common/bk_include.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>
#include <stdlib.h>
#include <string.h>

#if CONFIG_OTA_HTTP
#include <modules/ota.h>
#endif

#if CONFIG_OTA_HTTPS && CONFIG_OTA_TEST
extern char *https_url;
void bk_https_start_download(beken_thread_arg_t arg);
#endif

#if CONFIG_AT_CMD
#include "at_common.h"
#endif

#define OTA_AT_TAG "ota_at"

#if CONFIG_OTA_HTTP
static void http_ota_at_thread(beken_thread_arg_t arg)
{
	char *url = (char *)arg;

	if (url) {
		(void)bk_http_ota_download(url);
		free(url);
	}
	rtos_delete_thread(NULL);
}
#endif

int ota_at_schedule_url(const char *url)
{
	if (url == NULL || url[0] == '\0') {
		BK_LOGE(OTA_AT_TAG, "url is null\r\n");
		return -1;
	}

#if CONFIG_OTA_HTTP
	{
		char *url_copy;
		UINT32 ret;

		url_copy = (char *)malloc(strlen(url) + 1);
		if (url_copy == NULL) {
			BK_LOGE(OTA_AT_TAG, "malloc url failed\r\n");
			return -1;
		}
		strcpy(url_copy, url);
		BK_LOGI(OTA_AT_TAG, "http_ota_start\r\n");
		ret = rtos_create_thread(NULL, 4,
				"http_ota",
				(beken_thread_function_t)http_ota_at_thread,
				5120,
				(beken_thread_arg_t)url_copy);
		if (kNoErr != ret) {
			BK_LOGE(OTA_AT_TAG, "http_ota_start failed\r\n");
			free(url_copy);
			return -1;
		}
		return 0;
	}
#elif (CONFIG_OTA_HTTPS && CONFIG_OTA_TEST)
	{
		UINT32 ret = 0;

		if (https_url != NULL) {
			free(https_url);
			https_url = NULL;
		}

		https_url = (char *)malloc(strlen(url) + 1);
		if (https_url == NULL) {
			BK_LOGE(OTA_AT_TAG, "malloc https_url failed\r\n");
			return -1;
		}

		strcpy(https_url, url);

		BK_LOGI(OTA_AT_TAG, "https_ota_start\r\n");
		ret = rtos_create_thread(NULL, 4,
				 "https_ota",
				 (beken_thread_function_t)bk_https_start_download,
				 5120,
				 0);
		if (kNoErr != ret) {
			BK_LOGE(OTA_AT_TAG, "https_ota_start failed\r\n");
			free(https_url);
			https_url = NULL;
			return -1;
		}

		return 0;
	}
#else
	BK_LOGE(OTA_AT_TAG, "OTA not supported in this build\r\n");
	return -1;
#endif
}

#if CONFIG_AT_CMD
void ota_at_cli_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = AT_CMD_RSP_ERROR;
	const char *url = NULL;

	(void)xWriteBufferLen;

	if (argc == 2 && argv[1] != NULL) {
		url = argv[1];
	} else if (argc == 1 && argv[0] != NULL) {
		const char *prefix = "AT+OTA=";

		if (strncmp(argv[0], prefix, strlen(prefix)) == 0 && argv[0][strlen(prefix)] != '\0') {
			url = argv[0] + strlen(prefix);
		}
	}

	if (url == NULL) {
		BK_LOGE(OTA_AT_TAG, "Usage: AT+OTA <url> or AT+OTA=<url>\r\n");
		goto out;
	}

	if (ota_at_schedule_url(url) != 0) {
		goto out;
	}

	msg = AT_CMD_RSP_SUCCEED;

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

#endif
