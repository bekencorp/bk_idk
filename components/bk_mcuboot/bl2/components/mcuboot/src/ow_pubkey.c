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

#include <driver/flash_partition.h>
#include <driver/flash.h>
#include "partitions_gen.h"
#include "bootutil/bootutil_log.h"
#include "bootutil_priv.h"
#include "bootutil/sign_key.h"
#include "bootutil/crypto/sha.h"
#include "security.h"
#include "bootutil/image.h"
#include "sdkconfig.h"
#include "common_loader.h"

extern unsigned int pub_key_len;

#if CONFIG_OTA_UPDATE_PUBKEY
static bk_err_t read_pubkey_from_primary(const bk_logic_partition_t *partition, uint8_t *pubkey,
                                         uint16_t *pubkey_size)
{
    uint32_t key_len = 0;
    struct image_header primary_hdr;
    uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition->partition_start_addr));
    uint32_t tlv_begin;
    uint32_t offset = 0;
    struct image_tlv tlv;

    bk_flash_read_cbus(fa_off, &primary_hdr, sizeof(primary_hdr));
    tlv_begin = primary_hdr.ih_hdr_size + primary_hdr.ih_img_size;

    while (offset < TLV_TOTAL_SIZE) {
        bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(tlv));

        if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
            offset += sizeof(struct image_tlv);
        } else if (tlv.it_type != IMAGE_TLV_CUSTOM_PUBKEY) {
            offset += (sizeof(struct image_tlv) + tlv.it_len);
        } else {
            offset += sizeof(struct image_tlv);
            key_len = tlv.it_len;
            break;
        }
    }

    if (offset > TLV_TOTAL_SIZE || key_len > PUBKEY_MAX_LEN) {
        return BK_FAIL;
    }

    bk_flash_read_cbus(fa_off + tlv_begin + offset, pubkey, key_len);

    if (pubkey_size != NULL) {
        *pubkey_size = (uint16_t)key_len;
    }

    return BK_OK;
}

static bk_err_t ow_get_pubkey(const bk_logic_partition_t *app_partition,
                              uint8_t *pubkey_buf,
                              uint16_t *pubkey_len_inout)
{
    if (!app_partition || !pubkey_buf || !pubkey_len_inout) {
        return BK_ERR_PARAM;
    }

#if CONFIG_OTA_UPDATE_PUBKEY
    bk_err_t ret = load_pubkey_from_flash_backup(pubkey_buf, pubkey_len_inout);
    if (ret == BK_OK) {
        return BK_OK;
    }
#endif

    return read_pubkey_from_primary(app_partition, pubkey_buf, pubkey_len_inout);
}

int bk_read_pubkey_from_primary(uint8_t *pubkey, uint32_t key_size)
{
    bk_logic_partition_t app_partition;
    uint16_t len_io;

    if (!pubkey || key_size == 0) {
        return -1;
    }

    memset(&app_partition, 0, sizeof(app_partition));
    app_partition.partition_start_addr = partition_get_phy_offset(PARTITION_OVERWRITE);

    len_io = 0;
    if (read_pubkey_from_primary(&app_partition, pubkey, &len_io) != BK_OK) {
        return -1;
    }

    if ((uint32_t)len_io > key_size) {
        return -1;
    }

    return (int)len_io;
}
#endif

int
bk_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len, uint32_t current_slot_addr)
{
#if CONFIG_OTA_UPDATE_PUBKEY
    uint8_t check_key[PUBKEY_MAX_LEN];
    uint16_t pubkey_len_io;
    bk_logic_partition_t app_partition;
    bk_err_t ret;
    int fih_rc;

    (void)image_index;
    (void)current_slot_addr;

    memset(&app_partition, 0, sizeof(app_partition));
    app_partition.partition_start_addr = partition_get_phy_offset(PARTITION_OVERWRITE);

    pubkey_len_io = PUBKEY_MAX_LEN;
    ret = ow_get_pubkey(&app_partition, check_key, &pubkey_len_io);
    if (ret != BK_OK) {
        return -1;
    }

    if (pubkey_len_io != key_len) {
        BOOT_LOG_ERR("pubkey len mismatch primary=%#x outer=%#x", pubkey_len_io, key_len);
        return -1;
    }

    FIH_CALL(boot_fih_memequal, fih_rc, key, check_key, pubkey_len_io);

    if (FIH_EQ(fih_rc, FIH_SUCCESS)) {
        bootutil_keys[0].key = key;
        pub_key_len = key_len;
        return 0;
    }

    BOOT_LOG_ERR("pubkey of back and OTA-outer not equal!");
    return -1;
#else
    bootutil_keys[0].key = key;
    pub_key_len = key_len;
    BOOT_LOG_DBG("pubkey verified");
    return 0;
#endif
}
