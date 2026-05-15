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
#pragma once

#include <sdkconfig.h>

#if (CONFIG_AT_CMD || CONFIG_AT) && (CONFIG_OTA_HTTP || (CONFIG_OTA_HTTPS && CONFIG_OTA_TEST))

#ifdef __cplusplus
extern "C" {
#endif

int ota_at_schedule_url(const char *url);

#if CONFIG_AT_CMD
void ota_at_cli_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif

#endif
