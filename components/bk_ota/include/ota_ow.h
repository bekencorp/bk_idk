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

#include <stdint.h>

bk_err_t ow_ota_get_info(const bk_ota_t *ota, uint32_t *pad_offset, uint32_t *length);
bk_err_t ow_ota_write_dbus(const bk_ota_t *ota, uint32_t off, const uint8_t *buf, uint32_t len);
bk_err_t ow_ota_write_cbus(const bk_ota_t *ota, uint32_t off, const uint8_t *buf, uint32_t len);
bk_err_t ow_ota_read_dbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len);
bk_err_t ow_ota_read_cbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len);
bk_err_t ow_ota_erase(const bk_ota_t *ota);
bk_err_t ow_ota_erase_sector(const bk_ota_t *ota, uint32_t off);
bk_err_t ow_ota_verify(const bk_ota_t *ota, void* metadata);
bk_err_t ow_ota_confirm(const bk_logic_partition_t *partition);
bk_err_t ow_ota_cancel(const bk_logic_partition_t *control_partition);
bk_err_t ow_ota_accept(const bk_ota_t *ota);

bk_err_t ow_ota_read_primary_version(const bk_ota_t *ota);
bk_err_t ow_ota_read_compress_version(const bk_ota_t *ota);

bk_err_t ow_ota_reboot(void);

void ow_bl2_ota_get_pack_addrs(uint32_t *bl2_address, uint32_t *partition_address);
