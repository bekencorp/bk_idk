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
#include <string.h>
#include <common/bk_typedef.h>
#include "partitions_gen.h"
#include "bootutil/bootutil_log.h"
#include "bootutil_priv.h"
#include "flash_partition.h"
#include "bootutil/sign_key.h"
#include "bootutil/crypto/sha.h"
#include "security.h"
#include "bootutil/image.h"
#include "sdkconfig.h"

#if CONFIG_DIRECT_XIP && CONFIG_OTA_VERIFY_TRUST_CHAIN
#include "driver/flash.h"
extern uint32_t flash_get_excute_enable(void);
extern void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size);

// Type definitions and macros for TLV parsing
typedef struct image_header image_header_t;
typedef struct image_tlv image_tlv_t;
#define TLV_TOTAL_SIZE 512
#define MAX_PUBKEY_LEN 128

// Helper function to read 0xA0 pubkey from a slot partition
static int read_pubkey_0xa0_from_slot(uint32_t slot_addr, uint8_t* pubkey, uint16_t* pubkey_size)
{
    image_header_t hdr;
    uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(slot_addr));
    bk_flash_read_cbus(fa_off, &hdr, sizeof(image_header_t));
    
    if (hdr.ih_magic != IMAGE_MAGIC) {
        BOOT_LOG_ERR("Invalid image magic in slot");
        return -1;
    }
    
    uint32_t tlv_begin = hdr.ih_hdr_size + hdr.ih_img_size;
    uint32_t offset = 0;
    image_tlv_t tlv;
    
    // Find 0xA0 TLV
    while(offset < TLV_TOTAL_SIZE) {
        bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));
        
        if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
            offset += sizeof(image_tlv_t);
        } else if (tlv.it_type == IMAGE_TLV_CUSTOM) {
            // Found 0xA0 TLV
            offset += sizeof(image_tlv_t);
            if (tlv.it_len > MAX_PUBKEY_LEN) {
                return -1;
            }
            bk_flash_read_cbus(fa_off + tlv_begin + offset, pubkey, tlv.it_len);
            if (pubkey_size != NULL) {
                *pubkey_size = tlv.it_len;
            }
            return 0;
        } else {
            offset += (sizeof(image_tlv_t) + tlv.it_len);
        }
    }
    
    return -1;
}

// Helper function to read 0xA1 pubkey from a slot partition
static int read_pubkey_0xa1_from_slot(uint32_t slot_addr, uint8_t* pubkey, uint16_t* pubkey_size)
{
    image_header_t hdr;
    uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(slot_addr));
    bk_flash_read_cbus(fa_off, &hdr, sizeof(image_header_t));
    
    if (hdr.ih_magic != IMAGE_MAGIC) {
        BOOT_LOG_ERR("Invalid image magic in slot");
        return -1;
    }
    
    uint32_t tlv_begin = hdr.ih_hdr_size + hdr.ih_img_size;
    uint32_t offset = 0;
    image_tlv_t tlv;
    
    // Find 0xA1 TLV
    while(offset < TLV_TOTAL_SIZE) {
        bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));
        
        if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
            offset += sizeof(image_tlv_t);
        } else if (tlv.it_type == IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY) {
            // Found 0xA1 TLV
            offset += sizeof(image_tlv_t);
            if (tlv.it_len > MAX_PUBKEY_LEN) {
                return -1;
            }
            bk_flash_read_cbus(fa_off + tlv_begin + offset, pubkey, tlv.it_len);
            if (pubkey_size != NULL) {
                *pubkey_size = tlv.it_len;
            }
            return 0;
        } else {
            offset += (sizeof(image_tlv_t) + tlv.it_len);
        }
    }
    
    return -1;
}

// Helper function to check if slot has 0xA1 and 0xA2 TLVs
static int check_slot_has_trust_chain(uint32_t slot_addr, bool *has_a1, bool *has_a2)
{
    image_header_t hdr;
    uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(slot_addr));
    bk_flash_read_cbus(fa_off, &hdr, sizeof(image_header_t));
    
    if (hdr.ih_magic != IMAGE_MAGIC) {
        return -1;
    }
    
    uint32_t tlv_begin = hdr.ih_hdr_size + hdr.ih_img_size;
    uint32_t offset = 0;
    image_tlv_t tlv;
    *has_a1 = false;
    *has_a2 = false;
    
    while(offset < TLV_TOTAL_SIZE) {
        bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));
        
        if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
            offset += sizeof(image_tlv_t);
        } else if (tlv.it_type == IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY) {
            *has_a1 = true;
            offset += (sizeof(image_tlv_t) + tlv.it_len);
        } else if (tlv.it_type == IMAGE_TLV_CUSTOM_PUBKEY_SIG) {
            *has_a2 = true;
            offset += (sizeof(image_tlv_t) + tlv.it_len);
        } else {
            offset += (sizeof(image_tlv_t) + tlv.it_len);
        }
    }
    
    return 0;
}

// Main logic: find key based on trust chain verification
extern uint32_t flash_get_excute_enable(void);
extern bk_err_t bk_boot_write_ota_confirm(uint8_t image_index, uint32_t status);
    
// TLV definitions (these are already defined in bootutil/image.h, but we define them here for clarity)
#define IMAGE_TLV_CUSTOM               0xA0
#define IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY 0xA1
#define IMAGE_TLV_CUSTOM_PUBKEY_SIG    0xA2
// Note: IMAGE_TLV_PROT_INFO_MAGIC and IMAGE_TLV_INFO_MAGIC are already defined in bootutil/image.h
// Note: image_header_t and image_tlv_t are already defined via typedef above (lines 49-50)
// Note: Do not redefine image_header_t and image_tlv_t here - they are already typedef'd from bootutil/image.h

typedef struct {
    uint16_t it_magic;
    uint16_t it_tlv_tot;
} image_tlv_info_t;
#endif


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
bk_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len, uint32_t current_slot_addr)
{
    (void)current_slot_addr; // Not used in this code path
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
    uint16_t phy_length = VIR_LEN_TO_PHY_LEN(vir_length);
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
bk_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len, uint32_t current_slot_addr)
{
#if CONFIG_DIRECT_XIP
#if CONFIG_OTA_VERIFY_TRUST_CHAIN
    uint32_t slot0_addr = partition_get_phy_offset(PARTITION_XIP_A);
    uint32_t slot1_addr = partition_get_phy_offset(PARTITION_XIP_B);
    uint32_t last_slot_addr;
    int ret;
    FIH_DECLARE(fih_rc, FIH_FAILURE);
    
    if (current_slot_addr == slot0_addr) {
        last_slot_addr = slot1_addr;
        BOOT_LOG_DBG("Validating slot0 (Primary), last run was in slot1 (Secondary)");
    } else if (current_slot_addr == slot1_addr) {
        last_slot_addr = slot0_addr;
        BOOT_LOG_DBG("Validating slot1 (Secondary), last run was in slot0 (Primary)");
    } else {
        BOOT_LOG_ERR("Invalid current_slot_addr: 0x%x, expected slot0=0x%x or slot1=0x%x", 
                     current_slot_addr, slot0_addr, slot1_addr);
        goto fail;
    }
    
    // Check if current slot has trust chain TLVs (0xA1 and 0xA2)
    bool has_a1 = false, has_a2 = false;
    ret = check_slot_has_trust_chain(current_slot_addr, &has_a1, &has_a2);
    if (ret != 0) {
        BOOT_LOG_ERR("Failed to check trust chain TLVs in current slot");
        goto fail;
    }
    
    if (!has_a1 && !has_a2) {
        // Case 2.1: No 0xA1 and 0xA2, compare 0xA0 from both slots
        BOOT_LOG_DBG("Current slot has no trust chain TLVs, comparing 0xA0 from both slots");
        
        uint8_t last_slot_0xa0[MAX_PUBKEY_LEN];
        uint16_t last_slot_0xa0_len = 0;
        uint8_t current_slot_0xa0[MAX_PUBKEY_LEN];
        uint16_t current_slot_0xa0_len = 0;
        
        ret = read_pubkey_0xa0_from_slot(last_slot_addr, last_slot_0xa0, &last_slot_0xa0_len);
        if (ret != 0) {
            BOOT_LOG_ERR("Failed to read 0xA0 from last slot");
            goto fail;
        }
        
        // Current slot's 0xA0 is the key parameter passed in
        if (key_len > MAX_PUBKEY_LEN) {
            BOOT_LOG_ERR("Key length too large");
            goto fail;
        }
        memcpy(current_slot_0xa0, key, key_len);
        current_slot_0xa0_len = key_len;
        
        // Compare 0xA0 pubkeys
        if (last_slot_0xa0_len != current_slot_0xa0_len) {
            BOOT_LOG_ERR("0xA0 pubkey length mismatch: last_slot=%d, current_slot=%d", 
                         last_slot_0xa0_len, current_slot_0xa0_len);
            goto fail;
        }
        
        FIH_CALL(boot_fih_memequal, fih_rc, last_slot_0xa0, current_slot_0xa0, last_slot_0xa0_len);
        if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
            BOOT_LOG_ERR("0xA0 pubkey mismatch between slots");
            goto fail;
        }
        
        BOOT_LOG_DBG("new_img_sign_pubkey 0xA0 pubkey match, key found");
    } else {
        // Case 2.2: Has 0xA1 and 0xA2, compare last slot's 0xA0 with current slot's 0xA1
        BOOT_LOG_DBG("Current slot has trust chain TLVs, comparing last slot 0xA0 with current slot 0xA1");
        
        uint8_t last_slot_0xa0[MAX_PUBKEY_LEN];
        uint16_t last_slot_0xa0_len = 0;
        uint8_t current_slot_0xa1[MAX_PUBKEY_LEN];
        uint16_t current_slot_0xa1_len = 0;
        
        ret = read_pubkey_0xa0_from_slot(last_slot_addr, last_slot_0xa0, &last_slot_0xa0_len);
        if (ret != 0) {
            BOOT_LOG_ERR("Failed to read 0xA0 from last slot");
            goto fail;
        }
        
        ret = read_pubkey_0xa1_from_slot(current_slot_addr, current_slot_0xa1, &current_slot_0xa1_len);
        if (ret != 0) {
            BOOT_LOG_ERR("Failed to read 0xA1 from current slot");
            goto fail;
        }
        
        // Compare last slot's 0xA0 with current slot's 0xA1
        if (last_slot_0xa0_len != current_slot_0xa1_len) {
            BOOT_LOG_ERR("Pubkey length mismatch: last_slot_0xA0=%d, current_slot_0xA1=%d", 
                         last_slot_0xa0_len, current_slot_0xa1_len);
            goto fail;
        }
        
        FIH_CALL(boot_fih_memequal, fih_rc, last_slot_0xa0, current_slot_0xa1, last_slot_0xa0_len);
        if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
            BOOT_LOG_ERR("Pubkey mismatch: last slot 0xA0 != current slot 0xA1");
            goto fail;
        }
        
        BOOT_LOG_DBG("Pubkey match (last slot 0xA0 == current slot 0xA1), key found");
    }
    
    // Key found and verified, accept it
    bootutil_keys[0].key = key;
    pub_key_len = key_len;
    BOOT_LOG_DBG("XIP mode: accept pubkey from 0xA0 TLV (len=%d) after trust chain verification", key_len);
    return 0;
    
fail:
    // Key verification failed, clear confirm and boot from other slot
    BOOT_LOG_ERR("Key verification failed, clearing OTA confirm and booting from other slot");
    bk_boot_write_ota_confirm(image_index, 0xFFFFFFFF); // Clear confirm
    return -1;
#else
    bootutil_keys[0].key = key;
    pub_key_len = key_len;
    BOOT_LOG_DBG("XIP mode: accept pubkey from 0xA0 TLV (len=%d)", key_len);
    return 0;
#endif

#else
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
#endif
}
#endif