/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2016-2020 Linaro LTD
 * Copyright (c) 2016-2019 JUUL Labs
 * Copyright (c) 2019-2023 Arm Limited
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2024 Beken
 *
 * Original license:
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef COMMON_LOADER_H
#define COMMON_LOADER_H

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "bootutil/bootutil.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/image.h"
#include "bootutil_priv.h"
#include "swap_priv.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/security_cnt.h"
#include "bootutil/boot_record.h"
#include "bootutil/fault_injection_hardening.h"
#include "bootutil/ramload.h"
#include "bootutil/boot_hooks.h"
#include "bootutil/mcuboot_status.h"
#include "partitions_gen.h"
#include "flash_partition.h"
#include "boot_hal.h"
#include "mcuboot_config/mcuboot_config.h"

#define TARGET_STATIC static
#define PUBKEY_MAX_LEN 128
#define TLV_TOTAL_SIZE 1024

int boot_add_shared_data(struct boot_loader_state *state, uint8_t active_slot);
int boot_update_hw_rollback_protection(struct boot_loader_state *state);
fih_ret boot_validate_slot(struct boot_loader_state *state, int slot, struct boot_status *bs);
int boot_read_image_size(struct boot_loader_state *state, int slot, uint32_t *size);
int boot_read_image_headers(struct boot_loader_state *state, bool require_all, struct boot_status *bs);
void fill_rsp(struct boot_loader_state *state, struct boot_rsp *rsp);
int boot_check_header_erased(struct boot_loader_state *state, int slot);
int boot_erase_region(const struct flash_area *fap, uint32_t off, uint32_t sz);
int boot_update_security_counter(uint8_t image_index, int slot, struct image_header *hdr);
bool boot_is_header_valid(const struct image_header *hdr, const struct flash_area *fap);
bool bk_skip_validate(void);

#if CONFIG_OTA_UPDATE_PUBKEY
#include <common/bk_err.h>

typedef struct {
	uint8_t crc;
} CRC8_Context;

static inline uint8_t UpdateCRC8(uint8_t crcIn, uint8_t byte)
{
	uint8_t crc = crcIn;
	uint8_t i;

	crc ^= byte;

	for (i = 0; i < 8; i++) {
		if (crc & 0x01) {
			crc = (crc >> 1) ^ 0x8C;
		} else {
			crc >>= 1;
		}
	}
	return crc;
}

static inline void CRC8_Init(CRC8_Context *inContext)
{
	inContext->crc = 0;
}

static inline void CRC8_Update(CRC8_Context *inContext, const void *inSrc, size_t inLen)
{
	const uint8_t *src = (const uint8_t *) inSrc;
	const uint8_t *srcEnd = src + inLen;

	while (src < srcEnd) {
		inContext->crc = UpdateCRC8(inContext->crc, *src++);
	}
}

void get_pubkey_info(uint32_t addr, uint8_t *buf, uint32_t size);
bk_err_t read_pubkey_from_backup(uint32_t phy_off, uint8_t *key_out, uint16_t max_len,
				 uint16_t *key_len_out);
int pubkey_backup_sector_header_empty(uint32_t phy_off);
bk_err_t load_pubkey_from_flash_backup(uint8_t *pubkey_buf, uint16_t *pubkey_len_inout);
#endif

#endif // COMMON_LOADER_H