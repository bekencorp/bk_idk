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

#include "driver/flash_partition.h"

/*
 * Standard: OTA confirm is stored in the last 4 bytes of the OTA control partition.
 * Same rule for overwrite (ow_ota_control) and xip (ota_control) so that bootloader
 * and application use the same address: partition_start + partition_size - 4.
 */
#define OTA_CONFIRM_PHY_OFFSET(partition_start, partition_size) \
    ((uint32_t)((partition_start) + (partition_size) - 4))

/** Get physical offset of OTA confirm from partition (last 4 bytes). */
uint32_t ota_confirm_phy_offset(const bk_logic_partition_t *partition);

bk_err_t ota_confirm(const bk_logic_partition_t *partition, uint32_t status);
bk_err_t ota_cancel(const bk_logic_partition_t *partition);

#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
bk_err_t ota_control_backup_fail_stage_if_dirty(const bk_logic_partition_t *control_partition);
#endif
