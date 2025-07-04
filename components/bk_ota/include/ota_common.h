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

#define OTA_LOGE BK_LOGE
#define OTA_LOGI BK_LOGI
#define OTA_LOGD BK_LOGD

bk_err_t common_ota_partition_check(const bk_logic_partition_t *partition, uint32_t off, const uint8_t *buf, uint32_t len);
bk_err_t common_ota_confirm(const bk_logic_partition_t *partition, uint32_t status);
