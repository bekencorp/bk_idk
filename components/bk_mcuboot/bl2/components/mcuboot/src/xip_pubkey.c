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
#include "driver/flash.h"
#include "cache.h"

#if CONFIG_OTA_UPDATE_PUBKEY
#include <mbedtls/sha256.h>
#include "bootutil/crypto/ecdsa.h"

// Type definitions and macros for TLV parsing
typedef struct image_header image_header_t;
typedef struct image_tlv image_tlv_t;
#define TLV_TOTAL_SIZE 512
#define MAX_PUBKEY_LEN 128

static inline uint32_t addr_to_slot(uint32_t addr)  
{
    if (addr == partition_get_phy_offset(PARTITION_XIP_A)) {
        return 0;
    } else if (addr == partition_get_phy_offset(PARTITION_XIP_B)) {
        return 1;
    }
    return NO_ACTIVE_SLOT;
}

static inline uint32_t slot_to_addr(uint32_t slot)
{
    if (slot == 0) {
        return partition_get_phy_offset(PARTITION_XIP_A);
    } else if (slot == 1) {
        return partition_get_phy_offset(PARTITION_XIP_B);
    }
    return 0;
}

static inline uint32_t cbus_read_base_addr(void)
{
    return FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_get_phy_offset(PARTITION_XIP_A)));
}

static int read_tlv_from_slot(uint32_t slot, uint32_t tlv_type, uint8_t* tlv_data, uint16_t* tlv_data_size)
{
    image_header_t hdr;
    uint32_t fa_off = cbus_read_base_addr();

    bk_flash_read_cbus_xip(slot, fa_off, &hdr, sizeof(image_header_t));
    
    if (hdr.ih_magic != IMAGE_MAGIC) {
        BOOT_LOG_ERR("Invalid image magic in slot");
        return -1;
    }
    
    uint32_t tlv_begin = hdr.ih_hdr_size + hdr.ih_img_size;
    uint32_t offset = 0;
    image_tlv_t tlv;
    
    // Find TLV
    while(offset < TLV_TOTAL_SIZE) {
        uint32_t tlv_addr = fa_off + tlv_begin + offset;
        bk_flash_read_cbus_xip(slot, tlv_addr, (void *)&tlv, sizeof(image_tlv_t));
        
        if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
            offset += sizeof(image_tlv_t);
        } else if (tlv.it_type == tlv_type) {
            // Found TLV
            offset += sizeof(image_tlv_t);
            if (tlv.it_len > MAX_PUBKEY_LEN) {
                BOOT_LOG_ERR("TLV length too large: %d > %d", tlv.it_len, tlv_data_size);
                return -1;
            }
            uint32_t tlv_data_addr = fa_off + tlv_begin + offset;
            bk_flash_read_cbus_xip(slot, tlv_data_addr, tlv_data, tlv.it_len);
            if (tlv_data_size != NULL) {
                *tlv_data_size = tlv.it_len;
            }
            return 0;
        } else {
            offset += (sizeof(image_tlv_t) + tlv.it_len);
        }
    }
        
    return -1;
}

// Helper function to read 0xA0 pubkey from a slot partition
// This function reads 0xA0 from last_slot_addr
// Always use slot0 address, set XIP mode based on which slot we need to read
static int read_pubkey_from_slot(uint32_t slot, uint8_t* pubkey, uint16_t* pubkey_size)
{
    return read_tlv_from_slot(slot, IMAGE_TLV_CUSTOM_PUBKEY, pubkey, pubkey_size);
}

// Always use slot0 address, set XIP mode based on which slot we need to read
static int read_root_pubkey_from_slot(uint32_t slot, uint8_t* pubkey, uint16_t* pubkey_size)
{
    return read_tlv_from_slot(slot, IMAGE_TLV_CUSTOM_ROOT_PUBKEY, pubkey, pubkey_size);
}

// Always use slot0 address, set XIP mode based on which slot we need to read
static int read_signature_from_slot(uint32_t slot, uint8_t* signature, uint16_t* signature_size)
{
    return read_tlv_from_slot(slot, IMAGE_TLV_CUSTOM_PUBKEY_SIG, signature, signature_size);
}

static int check_slot_has_trust_chain(uint32_t slot_addr, bool *has_a1, bool *has_a2)
{
    image_header_t hdr;
    uint32_t slot0_addr = partition_get_phy_offset(PARTITION_XIP_A);
    uint32_t slot1_addr = partition_get_phy_offset(PARTITION_XIP_B);
    uint32_t read_slot_addr = slot_addr;
    uint32_t slot = 0;
    bool need_restore_xip = false;
    
    // If slot1, use slot0 address and set XIP mode
    if (slot_addr == slot1_addr) {
        read_slot_addr = slot0_addr;
        slot = 1;
    }
    
    uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(read_slot_addr));
    bk_flash_read_cbus_xip(slot, fa_off, &hdr, sizeof(image_header_t));
    
    if (hdr.ih_magic != IMAGE_MAGIC) {
        return -1;
    }
    
    uint32_t tlv_begin = hdr.ih_hdr_size + hdr.ih_img_size;
    uint32_t offset = 0;
    image_tlv_t tlv;
    *has_a1 = false;
    *has_a2 = false;
    
    while(offset < TLV_TOTAL_SIZE) {
        bk_flash_read_cbus_xip(slot, fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));
        
        if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
            offset += sizeof(image_tlv_t);
        } else if (tlv.it_type == IMAGE_TLV_CUSTOM_ROOT_PUBKEY) {
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

static int verify_signature_with_bootutil(const uint8_t *data, uint16_t data_len,
                                          const uint8_t *signature, uint16_t signature_len,
                                          const uint8_t *verify_pubkey, uint16_t verify_pubkey_len,
                                          const char *slot_name)
{
    FIH_DECLARE(fih_rc, FIH_FAILURE);
    
    // Calculate hash of data
    uint8_t data_hash[32];
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0);
    mbedtls_sha256_update(&sha256_ctx, data, data_len);
    mbedtls_sha256_finish(&sha256_ctx, data_hash);
    mbedtls_sha256_free(&sha256_ctx);
    
    // Setup bootutil_keys[0] for verification
    extern unsigned int pub_key_len;
    unsigned int saved_pub_key_len = pub_key_len;
    const uint8_t *saved_key = bootutil_keys[0].key;
    unsigned int *saved_len = bootutil_keys[0].len;
    
    bootutil_keys[0].key = verify_pubkey;
    pub_key_len = (unsigned int)verify_pubkey_len;
    bootutil_keys[0].len = &pub_key_len;
    
    FIH_CALL(bootutil_verify_sig, fih_rc,
             data_hash, sizeof(data_hash),
             signature, signature_len, 0);
    
    // Restore bootutil_keys[0]
    bootutil_keys[0].key = saved_key;
    pub_key_len = saved_pub_key_len;
    bootutil_keys[0].len = saved_len;
    
    if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
        if (slot_name) {
            BOOT_LOG_ERR("%s signature verification failed", slot_name);
        } else {
            BOOT_LOG_ERR("Signature verification failed");
        }
        return -1;
    }
    
    if (slot_name) {
        BOOT_LOG_INF("%s signature verification passed", slot_name);
    } else {
        BOOT_LOG_INF("Signature verification passed");
    }
    
    return 0;
}

// Main logic: find key based on trust chain verification
extern bk_err_t bk_boot_write_ota_confirm(uint8_t image_index, uint32_t status);
    
// TLV definitions (these are already defined in bootutil/image.h, but we define them here for clarity)
#define IMAGE_TLV_CUSTOM_PUBKEY        0xA0
#define IMAGE_TLV_CUSTOM_ROOT_PUBKEY   0xA1
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

static bool key_is_equal(uint8_t *key1, uint16_t key1_len, uint8_t *key2, uint16_t key2_len)
{
    if (key1_len != key2_len) {
        return false;
    }

    FIH_DECLARE(fih_rc, FIH_FAILURE);
    FIH_CALL(boot_fih_memequal, fih_rc, key1, key2, key1_len);
    if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
        return false;
    }

    return true;
}

int
bk_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len, uint32_t current_slot_addr)
{
#if CONFIG_OTA_UPDATE_PUBKEY
    uint32_t running_slot = boot_read_running_slot();
    uint32_t verifying_slot = addr_to_slot(current_slot_addr);
    int ret;
    FIH_DECLARE(fih_rc, FIH_FAILURE);

    BOOT_LOG_DBG("verifying slot: %s, running slot: %s", slot_str(verifying_slot), slot_str(running_slot));
    ret = read_pubkey_from_slot(verifying_slot, key, &key_len);
    if (ret != 0) {
        BOOT_LOG_ERR("failed to read pubkey from %s", slot_str(verifying_slot));
        goto fail;
    }

    if (running_slot != verifying_slot) {
        BOOT_LOG_DBG("verifying pubkey of %s", slot_str(verifying_slot));
 
        uint8_t running_slot_pubkey[MAX_PUBKEY_LEN];
        uint16_t running_slot_pubkey_len = 0;
        ret = read_pubkey_from_slot(running_slot, running_slot_pubkey, &running_slot_pubkey_len);
        if (ret != 0) {
            BOOT_LOG_ERR("failed to read pubkey from %s", slot_str(running_slot));
            goto fail;
        }

        uint8_t running_slot_root_pubkey[MAX_PUBKEY_LEN];
        uint16_t running_slot_root_pubkey_len = 0;
        ret = read_root_pubkey_from_slot(running_slot, running_slot_root_pubkey, &running_slot_root_pubkey_len);
        if (ret == 0) {
            if (!key_is_equal(key, key_len, running_slot_pubkey, running_slot_pubkey_len)) {
                BOOT_LOG_ERR("%s pubkey mismatch with %s", slot_str(verifying_slot), slot_str(running_slot));
                goto fail;
            }
        } else {
            uint8_t verifying_slot_root_pubkey[MAX_PUBKEY_LEN];
            uint16_t verifying_slot_root_pubkey_len = 0;
            ret = read_root_pubkey_from_slot(verifying_slot, verifying_slot_root_pubkey, &verifying_slot_root_pubkey_len);
            if (ret == 0) {
                uint8_t signature[MAX_PUBKEY_LEN];
                uint16_t signature_len = 0;
                ret = read_signature_from_slot(verifying_slot, signature, &signature_len);
                if (ret != 0) {
                    goto fail;
                }

                if (!key_is_equal(verifying_slot_root_pubkey, verifying_slot_root_pubkey_len, running_slot_pubkey, running_slot_pubkey_len)) {
                    BOOT_LOG_ERR("%s root pubkey mismatch with %s", slot_str(verifying_slot), slot_str(running_slot));
                    goto fail;
                }
                
                ret = verify_signature_with_bootutil(key, key_len,
                                                     signature, signature_len,
                                                     running_slot_pubkey, running_slot_pubkey_len,
                                                     slot_str(verifying_slot));
                if (ret != 0) {
                    goto fail;
                }
            }
            else{
                if (!key_is_equal(key, key_len, running_slot_pubkey, running_slot_pubkey_len)) {
                    BOOT_LOG_ERR("%s pubkey mismatch with %s", slot_str(verifying_slot), slot_str(running_slot));
                    goto fail;
                }
            }
        }
    }
    
    // Key found and verified, accept it
    bootutil_keys[0].key = key;
    pub_key_len = key_len;
    BOOT_LOG_DBG("%s pubkey verified", slot_str(verifying_slot));
    return 0;
    
fail:
    BOOT_LOG_ERR("Failed to verify pubkey of %s", slot_str(verifying_slot));
    return -1;
#else
    bootutil_keys[0].key = key;
    pub_key_len = key_len;
    BOOT_LOG_DBG("pubkey verified");
    return 0;
#endif
}
