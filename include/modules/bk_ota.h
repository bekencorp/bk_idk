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

#include <sdkconfig.h>

#ifdef CONFIG_BK_OTA
#include <driver/flash_partition.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bk_ota_s;
typedef struct bk_ota_s bk_ota_t;

/** @brief   This struct defines ota control.  */
typedef struct {
	bk_err_t (*info)(const bk_ota_t *ota, uint32_t *pad_size, uint32_t *length);
	bk_err_t (*write)(const bk_ota_t *ota, uint32_t off, const uint8_t *buf, uint32_t len);
	bk_err_t (*read)(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len);
	bk_err_t (*erase)(const bk_ota_t *ota);
	bk_err_t (*erase_sector)(const bk_ota_t *ota, uint32_t off);
	bk_err_t (*verify)(const bk_ota_t *ota, void* metadata);
	bk_err_t (*accept)(const bk_ota_t *ota);

	bk_err_t (*confirm)(const bk_logic_partition_t *partition);
	bk_err_t (*cancel)(const bk_logic_partition_t *partition);

#if CONFIG_OTA_UPDATE_PUBKEY
	bk_err_t (*updatekey)(const bk_logic_partition_t *app_partition, const bk_logic_partition_t *key_partition);
#endif
	bk_err_t (*reboot)(void);
} bk_ota_ops_t;

/** @brief   This struct defines ota control.  */
typedef struct bk_ota_s {
	bk_ota_ops_t ops;
	const bk_logic_partition_t* app_partition;
	const bk_logic_partition_t* ota_partition;
	const bk_logic_partition_t* control_partition;
#if CONFIG_OTA_UPDATE_PUBKEY
	const bk_logic_partition_t* key_partition;
#endif
} bk_ota_t;

bk_ota_t* bk_ota_get_xip_handle(void);
bk_ota_t* bk_ota_get_ow_handle(void);
bk_ota_t* bk_ota_addr_to_handle(uint32_t addr);
bk_err_t bk_ota_xip_deinit(void);
bk_err_t bk_ota_ow_deinit(void);

bk_err_t bk_ota_get_info(bk_ota_t* ota, uint32_t *pad_size, uint32_t* length);
bk_err_t bk_ota_write(bk_ota_t* ota, uint32_t off, const uint8_t *buf, uint32_t len);
bk_err_t bk_ota_read(bk_ota_t* ota, uint32_t off, uint8_t *buf, uint32_t len);
bk_err_t bk_ota_erase(bk_ota_t* ota);
bk_err_t bk_ota_erase_sector(bk_ota_t* ota, uint32_t off);
bk_err_t bk_ota_verify(bk_ota_t* ota, void* metadata);
bk_err_t bk_ota_confirm(bk_ota_t* ota);
bk_err_t bk_ota_cancel(bk_ota_t* ota);
bk_err_t bk_ota_accept(bk_ota_t* ota);
bk_err_t bk_ota_reboot(bk_ota_t* ota);
void bk_ota_accept_image(void);
#if CONFIG_OTA_CONFIRM_UPDATE
bk_err_t bk_ota_confirm_update(void);
bk_err_t bk_ota_cancel_update(uint8_t erase_ota);
#endif
#if CONFIG_OTA_UPDATE_PUBKEY
bk_err_t bk_ota_update_public_key(bk_ota_t* ota);
#endif
#ifdef __cplusplus
}
#endif
#endif
