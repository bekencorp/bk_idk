/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2017-2019 Linaro LTD
 * Copyright (c) 2016-2019 JUUL Labs
 * Copyright (c) 2019-2024 Arm Limited
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

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <flash_map_backend/flash_map_backend.h>

#include "bootutil/image.h"
#include "bootutil/crypto/sha.h"
#include "bootutil/sign_key.h"
#include "bootutil/security_cnt.h"
#include "bootutil/fault_injection_hardening.h"

#include "mcuboot_config/mcuboot_config.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/bootutil_public.h"

#ifdef MCUBOOT_ENC_IMAGES
#include "bootutil/enc_key.h"
#endif
#if defined(MCUBOOT_SIGN_RSA)
#include "mbedtls/rsa.h"
#endif
#if defined(MCUBOOT_SIGN_EC256)
#include "mbedtls/ecdsa.h"
#endif
#if defined(MCUBOOT_ENC_IMAGES) || defined(MCUBOOT_SIGN_RSA) || \
    defined(MCUBOOT_SIGN_EC256)
#include "mbedtls/asn1.h"
#endif

#include "bootutil_priv.h"
#include "flash_partition.h"
#include "flash.h"

#if CONFIG_BL2_SKIP_VALIDATE
#include "partitions_gen.h"
#include "aon_pmu_hal.h"
#include "components/system.h"
#endif

/*
 * Compute SHA hash over the image.
 * (SHA384 if ECDSA-P384 is being used,
 *  SHA256 otherwise).
 */
static int
bootutil_img_hash(struct enc_key_data *enc_state, int image_index,
                  struct image_header *hdr, const struct flash_area *fap,
                  uint8_t *tmp_buf, uint32_t tmp_buf_sz, uint8_t *hash_result,
                  uint8_t *seed, int seed_len)
{
    bootutil_sha_context sha_ctx;
    uint32_t blk_sz;
    uint32_t size;
    uint16_t hdr_size;
    uint32_t off;
    int rc;
    int ret = 0;
    uint32_t blk_off;
    uint32_t tlv_off;
#if CONFIG_DIRECT_XIP
    uint32_t execute_enable_saved = 0;
    bool need_restore_execute_enable = false;
#endif

#if (BOOT_IMAGE_NUMBER == 1) || !defined(MCUBOOT_ENC_IMAGES) || \
    defined(MCUBOOT_RAM_LOAD)
    (void)enc_state;
    (void)image_index;
    (void)hdr_size;
    (void)blk_off;
    (void)tlv_off;
#ifdef MCUBOOT_RAM_LOAD
    (void)blk_sz;
    (void)off;
    (void)rc;
    (void)fap;
    (void)tmp_buf;
    (void)tmp_buf_sz;
#endif
#endif

#ifdef MCUBOOT_ENC_IMAGES
    /* Encrypted images only exist in the secondary slot */
    if (MUST_DECRYPT(fap, image_index, hdr) &&
            !boot_enc_valid(enc_state, image_index, fap)) {
        return -1;
    }
#endif

    /* Hash is computed over image header and image itself. */
    size = hdr_size = hdr->ih_hdr_size;
    size += hdr->ih_img_size;
    tlv_off = size;

    /* If protected TLVs are present they are also hashed. */
    size += hdr->ih_protect_tlv_size;

    /* Disable Dcache when te200 calculate hash*/
    flush_all_dcache();
    enable_dcache(0);

#if CONFIG_OTA_OVERWRITE
    if( fap->fa_off == partition_get_phy_offset(PARTITION_OTA )) {
        /* For OTA partition, CRC check is not applicable, proceed with SHA init */
        bootutil_sha_init(&sha_ctx);
        
        /* in some cases (split image) the hash is seeded with data from
         * the loader image */
        if (seed && (seed_len > 0)) {
            bootutil_sha_update(&sha_ctx, seed, seed_len);
        }
        
        for (off = 0; off < size; off += blk_sz) {
            blk_sz = size - off;
            if (blk_sz > tmp_buf_sz) {
                blk_sz = tmp_buf_sz;
            }
            rc = flash_area_read(fap, off, tmp_buf, blk_sz);
            if (rc) {
                bootutil_sha_drop(&sha_ctx);
                return rc;
            }

            bootutil_sha_update(&sha_ctx, tmp_buf, blk_sz);
        }
    }
    else 
#endif /* CONFIG_OTA_OVERWRITE */
    {
        uint32_t addr = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(fap->fa_off));
        uint32_t crc_check_addr = addr;  // Address for CRC check

#if CONFIG_DIRECT_XIP
        uint32_t sec_addr = partition_get_phy_offset(PARTITION_XIP_B);
        uint32_t pri_addr = partition_get_phy_offset(PARTITION_XIP_A);
        
        if(fap->fa_off == sec_addr){
            // For slot1, use slot0 address and set XIP mode
            addr = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(pri_addr));
            crc_check_addr = addr;
            execute_enable_saved = flash_get_excute_enable();
            flash_set_excute_enable(1);
            need_restore_execute_enable = true;
        } else if(fap->fa_off == pri_addr){
            addr = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(pri_addr));
            crc_check_addr = addr;
        }
#endif
#ifdef CONFIG_DBUS_CHECK_CRC
        /* CRITICAL: Perform CRC check BEFORE initializing SHA context */
        /* This prevents hardware SHA accelerator state pollution if CRC fails */
        /* Hardware SHA accelerators may allocate resources during init, which can */
        /* cause state corruption if not properly cleaned up after CRC failure */
        BOOT_LOG_INF("check crc16");
        if (check_crc(crc_check_addr, size) != true) {
            BOOT_LOG_ERR("app crc16 error");
            /* Return immediately without initializing SHA context */
            /* This prevents hardware SHA accelerator state pollution */
            ret = -1;
            goto _restore_execute_enable;
        } else {
            BOOT_LOG_INF("app crc16 right");
        }
#endif
        /* Initialize SHA context only after CRC check passes */
        /* This ensures hardware SHA accelerator is only initialized when needed */
        /* Critical: Ensure any previous hardware SHA state is fully cleared before init */
        /* This is especially important after a previous validation failure */

        bootutil_sha_init(&sha_ctx);

        /* in some cases (split image) the hash is seeded with data from
         * the loader image */
        if (seed && (seed_len > 0)) {
            bootutil_sha_update(&sha_ctx, seed, seed_len);
        }
        uint32_t hash_calc_addr = crc_check_addr + FLASH_BASE_ADDRESS;
        bootutil_sha_update(&sha_ctx,
                            (void*)hash_calc_addr,
                            size);
    }
    bootutil_sha_finish(&sha_ctx, hash_result);
    
    bootutil_sha_drop(&sha_ctx);

_restore_execute_enable:
#if CONFIG_DIRECT_XIP
    // Restore XIP state if changed
    if (need_restore_execute_enable) {
        flash_set_excute_enable(execute_enable_saved);
    }
#endif
    enable_dcache(1);
    return ret;
}
/*
 * Currently, we only support being able to verify one type of
 * signature, because there is a single verification function that we
 * call.  List the type of TLV we are expecting.  If we aren't
 * configured for any signature, don't define this macro.
 */
#if (defined(MCUBOOT_SIGN_RSA)      + \
     defined(MCUBOOT_SIGN_EC256)    + \
     defined(MCUBOOT_SIGN_EC384)    + \
     defined(MCUBOOT_SIGN_ED25519)) > 1
#error "Only a single signature type is supported!"
#endif

#if defined(MCUBOOT_SIGN_RSA)
#    if MCUBOOT_SIGN_RSA_LEN == 2048
#        define EXPECTED_SIG_TLV IMAGE_TLV_RSA2048_PSS
#    elif MCUBOOT_SIGN_RSA_LEN == 3072
#        define EXPECTED_SIG_TLV IMAGE_TLV_RSA3072_PSS
#    else
#        error "Unsupported RSA signature length"
#    endif
#    define SIG_BUF_SIZE (MCUBOOT_SIGN_RSA_LEN / 8)
#    define EXPECTED_SIG_LEN(x) ((x) == SIG_BUF_SIZE) /* 2048 bits */
#elif defined(MCUBOOT_SIGN_EC256) || \
      defined(MCUBOOT_SIGN_EC384) || \
      defined(MCUBOOT_SIGN_EC)
#    define EXPECTED_SIG_TLV IMAGE_TLV_ECDSA_SIG
#    define SIG_BUF_SIZE 128
#    define EXPECTED_SIG_LEN(x) (1) /* always true, ASN.1 will validate */
#elif defined(MCUBOOT_SIGN_ED25519)
#    define EXPECTED_SIG_TLV IMAGE_TLV_ED25519
#    define SIG_BUF_SIZE 64
#    define EXPECTED_SIG_LEN(x) ((x) == SIG_BUF_SIZE)
#else
#    define SIG_BUF_SIZE 32 /* no signing, sha256 digest only */
#endif

#if (defined(MCUBOOT_HW_KEY)       + \
     defined(MCUBOOT_BUILTIN_KEY)) > 1
#error "Please use either MCUBOOT_HW_KEY or the MCUBOOT_BUILTIN_KEY feature."
#endif

#ifdef EXPECTED_SIG_TLV

#if !defined(MCUBOOT_BUILTIN_KEY)
#if !defined(MCUBOOT_HW_KEY)
/* The key TLV contains the hash of the public key. */
#   define EXPECTED_KEY_TLV     IMAGE_TLV_KEYHASH
#   define KEY_BUF_SIZE         IMAGE_HASH_SIZE
#else
/* The key TLV contains the whole public key.
 * Add a few extra bytes to the key buffer size for encoding and
 * for public exponent.
 */
#   define EXPECTED_KEY_TLV     IMAGE_TLV_CUSTOM_PUBKEY
#   define KEY_BUF_SIZE         (91)
#endif /* !MCUBOOT_HW_KEY */

#if !defined(MCUBOOT_HW_KEY)
static int
bootutil_find_key(uint8_t *keyhash, uint8_t keyhash_len)
{
    bootutil_sha_context sha_ctx;
    int i;
    const struct bootutil_key *key;
    uint8_t hash[IMAGE_HASH_SIZE];

    if (keyhash_len > IMAGE_HASH_SIZE) {
        return -1;
    }

    for (i = 0; i < bootutil_key_cnt; i++) {
        key = &bootutil_keys[i];
        bootutil_sha_init(&sha_ctx);
        bootutil_sha_update(&sha_ctx, key->key, *key->len);
        bootutil_sha_finish(&sha_ctx, hash);
        if (!memcmp(hash, keyhash, keyhash_len)) {
            bootutil_sha_drop(&sha_ctx);
            return i;
        }
    }
    bootutil_sha_drop(&sha_ctx);
    return -1;
}
#else /* !MCUBOOT_HW_KEY */
extern unsigned int pub_key_len;
static int
bootutil_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len)
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
#endif /* !MCUBOOT_HW_KEY */
#endif /* !MCUBOOT_BUILTIN_KEY */
#endif /* EXPECTED_SIG_TLV */

/**
 * Reads the value of an image's security counter.
 *
 * @param hdr           Pointer to the image header structure.
 * @param fap           Pointer to a description structure of the image's
 *                      flash area.
 * @param security_cnt  Pointer to store the security counter value.
 *
 * @return              0 on success; nonzero on failure.
 */
int32_t
bootutil_get_img_security_cnt(struct image_header *hdr,
                              const struct flash_area *fap,
                              uint32_t *img_security_cnt)
{
    struct image_tlv_iter it;
    uint32_t off;
    uint16_t len;
    int32_t rc;

    if ((hdr == NULL) ||
        (fap == NULL) ||
        (img_security_cnt == NULL)) {
        /* Invalid parameter. */
        return BOOT_EBADARGS;
    }

    /* The security counter TLV is in the protected part of the TLV area. */
    if (hdr->ih_protect_tlv_size == 0) {
        return BOOT_EBADIMAGE;
    }

    rc = bootutil_tlv_iter_begin(&it, hdr, fap, IMAGE_TLV_SEC_CNT, true);
    if (rc) {
        return rc;
    }

    /* Traverse through the protected TLV area to find
     * the security counter TLV.
     */

    rc = bootutil_tlv_iter_next(&it, &off, &len, NULL);
    if (rc != 0) {
        /* Security counter TLV has not been found. */
        return -1;
    }

    if (len != sizeof(*img_security_cnt)) {
        /* Security counter is not valid. */
        return BOOT_EBADIMAGE;
    }

    rc = LOAD_IMAGE_DATA(hdr, fap, off, img_security_cnt, len);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    return 0;
}

#ifndef ALLOW_ROGUE_TLVS
/*
 * The following list of TLVs are the only entries allowed in the unprotected
 * TLV section.  All other TLV entries must be in the protected section.
 */
static const uint16_t allowed_unprot_tlvs[] = {
     IMAGE_TLV_KEYHASH,
     IMAGE_TLV_PUBKEY,
     IMAGE_TLV_SHA256,
     IMAGE_TLV_SHA384,
     IMAGE_TLV_RSA2048_PSS,
     IMAGE_TLV_ECDSA224,
     IMAGE_TLV_ECDSA_SIG,
     IMAGE_TLV_RSA3072_PSS,
     IMAGE_TLV_ED25519,
     IMAGE_TLV_ENC_RSA2048,
     IMAGE_TLV_ENC_KW,
     IMAGE_TLV_ENC_EC256,
     IMAGE_TLV_ENC_X25519,
     /* Mark end with ANY. */
     IMAGE_TLV_ANY,
};
#endif

static int
bootutil_hash_hash(uint8_t *digest, uint32_t digest_sz, uint8_t *hash_result)
{
    bootutil_sha_context sha256_ctx;
    bootutil_sha_init(&sha256_ctx);
    bootutil_sha_update(&sha256_ctx, digest, digest_sz);
    bootutil_sha_finish(&sha256_ctx, hash_result);
    bootutil_sha_drop(&sha256_ctx);
    return 0;
}

/*
 * Verify the integrity of the image.
 * Return non-zero if image could not be validated/does not validate.
 */
fih_ret
bootutil_img_validate(struct enc_key_data *enc_state, int image_index,
                      struct image_header *hdr, const struct flash_area *fap,
                      uint8_t *tmp_buf, uint32_t tmp_buf_sz, uint8_t *seed,
                      int seed_len, uint8_t *out_hash)
{
#if CONFIG_BL2_SKIP_VALIDATE
    uint32_t reset_reason = aon_pmu_hal_get_reset_reason();
    bool ota_confirm_set = bk_boot_check_ota_confirm(image_index, OTA_CONFIRM);
    bool can_skip = (reset_reason == RESET_SOURCE_POWERON) && (!ota_confirm_set);

    if (aon_pmu_hal_get_app_boot_validate_pending() != 0u) {
        can_skip = false;
        BOOT_LOG_INF("app_boot_validate_pending, force validate");
    }

#if CONFIG_DIRECT_XIP
    int slot = flash_area_id_to_multi_image_slot(image_index, flash_area_get_id(fap));
    uint32_t xip_status = (slot >= 0) ? boot_read_xip_status(slot, XIP_IMAGE_OK_TYPE) : 0xFFFFFFFF;
    if (can_skip && (slot >= 0) && (xip_status == XIP_IMAGE_OK)) {
        BOOT_LOG_INF("reset reason %#x, skip validate", reset_reason);
        FIH_RET(FIH_SUCCESS);
        return 0;
    }

    BOOT_LOG_INF("reset reason %#x, ota_confirm=%d, slot=%s, xip_status=%#x, do validate", 
                 reset_reason, ota_confirm_set, slot_str(slot), xip_status);
#elif CONFIG_OTA_OVERWRITE
    if (can_skip) {
        BOOT_LOG_INF("reset reason %#x, skip validate", reset_reason);
        return 0;
    }
    BOOT_LOG_INF("reset reason %#x, ota_confirm=%d, do validate", reset_reason, ota_confirm_set);
#endif
#endif

    uint32_t off;
    uint16_t len;
    uint16_t type;
    int image_hash_valid = 0;
#ifdef EXPECTED_SIG_TLV
    FIH_DECLARE(valid_signature, FIH_FAILURE);
#ifndef MCUBOOT_BUILTIN_KEY
    int key_id = -1;
#else
    /* Pass a key ID equal to the image index, the underlying crypto library
     * is responsible for mapping the image index to a builtin key ID.
     */
    int key_id = image_index;
#endif /* !MCUBOOT_BUILTIN_KEY */
#ifdef MCUBOOT_HW_KEY
    uint8_t key_buf[KEY_BUF_SIZE];
#endif
#endif /* EXPECTED_SIG_TLV */
    struct image_tlv_iter it;
    uint8_t buf[SIG_BUF_SIZE];
    uint8_t hash[IMAGE_HASH_SIZE];
    int rc = 0;
    FIH_DECLARE(fih_rc, FIH_FAILURE);
#ifdef MCUBOOT_HW_ROLLBACK_PROT
    fih_int security_cnt = fih_int_encode(INT_MAX);
    uint32_t img_security_cnt = 0;
    FIH_DECLARE(security_counter_valid, FIH_FAILURE);
#endif

    rc = bootutil_img_hash(enc_state, image_index, hdr, fap, tmp_buf,
            tmp_buf_sz, hash, seed, seed_len);
    if (rc) {
        goto out;
    }

    if (out_hash) {
        memcpy(out_hash, hash, IMAGE_HASH_SIZE);
    }

    rc = bootutil_tlv_iter_begin(&it, hdr, fap, IMAGE_TLV_ANY, false);
    if (rc) {
        BOOT_LOG_ERR("Failed to begin TLV iteration, rc=%d", rc);
        goto out;
    }

    if (it.tlv_end > bootutil_max_image_size(fap)) {
        BOOT_LOG_ERR("TLV end exceeds max image size: tlv_end=0x%x, max_size=0x%x", 
                     it.tlv_end, bootutil_max_image_size(fap));
        rc = -1;
        goto out;
    }

    /*
     * Traverse through all of the TLVs, performing any checks we know
     * and are able to do.
     */
    while (true) {
        rc = bootutil_tlv_iter_next(&it, &off, &len, &type);
        BOOT_LOG_DBG("tlv type=%#x,%d",type,bootutil_tlv_iter_is_prot(&it, off));
        if (rc < 0) {
            BOOT_LOG_ERR("TLV iteration error, rc=%d", rc);
            goto out;
        } else if (rc > 0) {
            break;
        }

#ifndef ALLOW_ROGUE_TLVS
        /*
         * Ensure that the non-protected TLV only has entries necessary to hold
         * the signature.  We also allow encryption related keys to be in the
         * unprotected area.
         */
        if (!bootutil_tlv_iter_is_prot(&it, off)) {
             bool found = false;
             for (const uint16_t *p = allowed_unprot_tlvs; *p != IMAGE_TLV_ANY; p++) {
                  if (type == *p) {
                       found = true;
                       break;
                  }
             }
             if (!found) {
                  FIH_SET(fih_rc, FIH_FAILURE);
                  goto out;
             }
        }
#endif

        if (type == EXPECTED_HASH_TLV) {
            /* Verify the image hash. This must always be present. */
            if (len != sizeof(hash)) {
                BOOT_LOG_ERR("TLV hash length mismatch: expected %d, got %d", sizeof(hash), len);
                rc = -1;
                goto out;
            }
            
            rc = LOAD_IMAGE_DATA(hdr, fap, off, buf, sizeof(hash));
            if (rc) {
                BOOT_LOG_ERR("Failed to read TLV hash data, rc=%d", rc);
                goto out;
            }

            FIH_CALL(boot_fih_memequal, fih_rc, hash, buf, sizeof(hash));
#if MCUBOOT_LOG_LEVEL == MCUBOOT_LOG_LEVEL_DEBUG
            DUMP_BUF("cal hash", hash, sizeof(hash));
            DUMP_BUF("store hash", buf, sizeof(hash));
#endif

            if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
                FIH_SET(fih_rc, FIH_FAILURE);
                BOOT_LOG_ERR("TLV hash verification failed!");
                goto out;
            }

            image_hash_valid = 1;
            BOOT_LOG_INF("hash ok!");

#if CONFIG_BL2_SKIP_VALIDATE
            if (image_hash_valid == 1 && (
#if CONFIG_OTA_OVERWRITE
                fap->fa_id == 0
#elif CONFIG_DIRECT_XIP
                (fap->fa_id == 2 || fap->fa_id == 3)
#endif
            )) {
                /* Do not clear OTA confirm here, it will be cleared after XIP switching */
                return 0;
            }
#endif

#ifdef EXPECTED_KEY_TLV
        } else if (type == EXPECTED_KEY_TLV) {
            /*
             * Determine which key we should be checking.
             */
            if (len > KEY_BUF_SIZE) {
                rc = -1;
                goto out;
            }
            rc = LOAD_IMAGE_DATA(hdr, fap, off, key_buf, len);
            if (rc) {
                goto out;
            }
            extern int bk_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len, uint32_t current_slot_addr);
            key_id = bk_find_key(image_index, key_buf, len, fap->fa_off);
            if (key_id == 0) {
                BOOT_LOG_INF("pubkey ok!");
            }
#endif /* !MCUBOOT_HW_KEY */
            /*
             * The key may not be found, which is acceptable.  There
             * can be multiple signatures, each preceded by a key.
             */
#ifdef EXPECTED_SIG_TLV
        } else if (type == EXPECTED_SIG_TLV) {
            /* Ignore this signature if it is out of bounds. */
            if (key_id < 0 || key_id >= bootutil_key_cnt) {
                key_id = -1;
                continue;
            }
            if (!EXPECTED_SIG_LEN(len) || len > sizeof(buf)) {
                rc = -1;
                goto out;
            }
            rc = LOAD_IMAGE_DATA(hdr, fap, off, buf, len);
            if (rc) {
                goto out;
            }
            FIH_CALL(bootutil_verify_sig, valid_signature, hash, sizeof(hash),
                                                           buf, len, key_id);
            BOOT_LOG_INF("verify signature %d", valid_signature);
            key_id = -1;
#endif /* EXPECTED_SIG_TLV */
#ifdef MCUBOOT_HW_ROLLBACK_PROT
        } else if (type == IMAGE_TLV_SEC_CNT) {
            /*
             * Verify the image's security counter.
             * This must always be present.
             */
            if (len != sizeof(img_security_cnt)) {
                /* Security counter is not valid. */
                rc = -1;
                goto out;
            }

            rc = LOAD_IMAGE_DATA(hdr, fap, off, &img_security_cnt, len);
            if (rc) {
                goto out;
            }

            FIH_CALL(boot_nv_security_counter_get, fih_rc, image_index,
                                                           &security_cnt);
            if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
                FIH_SET(fih_rc, FIH_FAILURE);
                BOOT_LOG_INF("fail get sc");
                goto out;
            }

            /* Compare the new image's security counter value against the
             * stored security counter value.
             */
            fih_rc = fih_ret_encode_zero_equality(img_security_cnt <
                                   (uint32_t)fih_int_decode(security_cnt));
            if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
                BOOT_LOG_INF("sc=%d < %d", img_security_cnt, security_cnt);
                FIH_SET(fih_rc, FIH_FAILURE);
                goto out;
            }

            /* The image's security counter has been successfully verified. */
            security_counter_valid = fih_rc;
#endif /* MCUBOOT_HW_ROLLBACK_PROT */
        }
    }

    rc = !image_hash_valid;
    if (rc) {
        goto out;
    }
#ifdef EXPECTED_SIG_TLV
    FIH_SET(fih_rc, valid_signature);
#endif
#ifdef MCUBOOT_HW_ROLLBACK_PROT
    if (FIH_NOT_EQ(security_counter_valid, FIH_SUCCESS)) {
        rc = -1;
        goto out;
    }
#endif

out:
    if (rc) {
#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
        /* Record failure based on partition type */
        if (fap->fa_id == 0) {
            /* PRIMARY slot - App verify failure */
            bk_boot_write_ota_fail(BOOT_FAIL_STAGE_APP_VERIFY);
        } else if (fap->fa_id == 1) {
            /* SECONDARY slot - OTA verify failure */
            bk_boot_write_ota_fail(BOOT_FAIL_STAGE_OTA_VERIFY);
        }
#endif
        FIH_SET(fih_rc, FIH_FAILURE);
    }

    FIH_RET(fih_rc);
}
