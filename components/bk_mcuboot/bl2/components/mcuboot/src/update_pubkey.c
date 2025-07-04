/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2017-2019 Linaro LTD
 * Copyright (c) 2016-2019 JUUL Labs
 * Copyright (c) 2019-2023 Arm Limited
 * Copyright (c) 2020-2023 Nordic Semiconductor ASA
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

#include <stdlib.h>
#include <common/bk_typedef.h>
#include "partitions_gen.h"
#include "bootutil/bootutil_log.h"
#include "bootutil_priv.h"
#include "flash_partition.h"
#include "bootutil/sign_key.h"
#include "bootutil/crypto/sha.h"


typedef struct {
	uint8_t crc;
} CRC8_Context;

static uint8_t UpdateCRC8(uint8_t crcIn, uint8_t byte)
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

static void CRC8_Init( CRC8_Context *inContext )
{
    inContext->crc = 0;
}

static void CRC8_Update( CRC8_Context *inContext, const void *inSrc, size_t inLen )
{
	const uint8_t *src = (const uint8_t *) inSrc;
	const uint8_t *srcEnd = src + inLen;
	while ( src < srcEnd ) {
		inContext->crc = UpdateCRC8(inContext->crc, *src++);
	}
}

extern unsigned int pub_key_len;

#if CONFIG_OTA_UPDATE_PUBKEY
int
bk_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len)
{
    uint32_t phy_off = partition_get_phy_offset(PARTITION_PUBLIC_KEY);
    uint32_t vir_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_off + 3)); 
    uint16_t vir_length;
    uint8_t* check_key;
    CRC8_Context crc8;
    CRC8_Context check_crc;
    int fih_rc;

    bk_flash_read_bytes(phy_off, (uint8_t *)&vir_length, 2); // vir_length = key_size
    if (key_len != vir_length) {
        BOOT_LOG_ERR("pubkey size mismatch: back=%#x, ota-outer=%#x", vir_length, key_len);
        return -1;
    }

    bk_flash_read_bytes(phy_off + 2, (uint8_t*)&check_crc, 1);
    uint16_t phy_length = ((vir_length + 31) >> 5) * 34; //ceil align 34
    uint8_t *enc_buf = (uint8_t*)malloc(phy_length);
    bk_flash_read_bytes(phy_off + 3, enc_buf, phy_length);
    CRC8_Init(&crc8);
    CRC8_Update(&crc8, enc_buf, phy_length);

    if (crc8.crc != check_crc.crc) {
        free(enc_buf);
        BOOT_LOG_ERR("back pubkey corrupted, crc8 expected=%#x, actual=%#x", check_crc.crc, crc8.crc);
        return -1;
    }

    check_key = (uint8_t *)malloc(key_len);
    bk_flash_read_cbus(vir_off, check_key, key_len);

    FIH_CALL(boot_fih_memequal, fih_rc, key, check_key, key_len);
    
    if (FIH_EQ(fih_rc, FIH_SUCCESS)) {
        bootutil_keys[0].key = key;
        pub_key_len = key_len;
        free(enc_buf);
        free(check_key);
        BOOT_LOG_INF("pubkey ok!");
        return 0;
    }

    BOOT_LOG_ERR("pubkey of back and OTA-outer not equal!");
    return -1;
}
#else
int
bk_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len)
{
    bootutil_sha_context sha_ctx;
    uint8_t hash[IMAGE_HASH_SIZE];
    uint8_t key_hash[IMAGE_HASH_SIZE];
    size_t key_hash_size = sizeof(key_hash);
    int rc;
    FIH_DECLARE(fih_rc, FIH_FAILURE);

    bootutil_sha_init(&sha_ctx);
    bootutil_sha_update(&sha_ctx, key, key_len);
    bootutil_sha_finish(&sha_ctx, hash);
    bootutil_sha_drop(&sha_ctx);

    rc = boot_retrieve_public_key_hash(image_index, key_hash, &key_hash_size);
    if (rc) {
        return -1;
    }

    /* Adding hardening to avoid this potential attack:
     *  - Image is signed with an arbitrary key and the corresponding public
     *    key is added as a TLV field.
     * - During public key validation (comparing against key-hash read from
     *   HW) a fault is injected to accept the public key as valid one.
     */
    FIH_CALL(boot_fih_memequal, fih_rc, hash, key_hash, key_hash_size);
    if (FIH_EQ(fih_rc, FIH_SUCCESS)) {
        bootutil_keys[0].key = key;
        pub_key_len = key_len;
        return 0;
    }

    return -1;
}
#endif