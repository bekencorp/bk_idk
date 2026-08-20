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

#pragma once

#include "modules/https_ota.h"

/*
 * @brief    Notify OTA error event to registered callback
 *
 * This function fires HTTPS_OTA_ERROR_EVENT with the given error code.
 * The registered ota_event_handler is responsible for reporting the error,
 * such as printing OTA ERROR CODE in the application layer.
 *
 * @param[in] ota_config          pointer to https_ota_t struct
 * @param[in] err                 OTA error code to report
 *
 * @return
 *     - BK_FAIL:                 parameter pointer is empty or no event handler
 *     - BK_OK:                   error event reported successfully
 */
bk_err_t bk_https_ota_notify_error(https_ota_t* ota_config, bk_err_t err);
