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

/**
 * @file
 * @brief Public MCUBoot interface API implementation
 *
 * This file contains API implementation which can be combined with
 * the application in order to interact with the MCUBoot bootloader.
 * This file contains shared code-base betwen MCUBoot and the application
 * which controls DFU process.
 */

#include <string.h>
#include <inttypes.h>
#include <stddef.h>

#include "sysflash/sysflash.h"
#include "flash_map_backend/flash_map_backend.h"

#include "bootutil/image.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/bootutil_log.h"

#include "bootutil/boot_public_hooks.h"
#include "bootutil_priv.h"
#include "bootutil_misc.h"
#include "flash_partition.h"
#include "partitions_gen.h"
#include <driver/flash.h>

#if CONFIG_BL2_WDT
#include <driver/wdt_hal.h>
#endif

#ifdef CONFIG_MCUBOOT
BOOT_LOG_MODULE_DECLARE(mcuboot);
#else
BOOT_LOG_MODULE_REGISTER(mcuboot_util);
#endif

#if BOOT_MAX_ALIGN == 8
const union boot_img_magic_t boot_img_magic = {
    .val = {
        0x77, 0xc2, 0x95, 0xf3,
        0x60, 0xd2, 0xef, 0x7f,
        0x35, 0x52, 0x50, 0x0f,
        0x2c, 0xb6, 0x79, 0x80
    }
};
#else
const union boot_img_magic_t boot_img_magic = {
    .align = BOOT_MAX_ALIGN,
    .magic = {
        0x2d, 0xe1,
        0x5d, 0x29, 0x41, 0x0b,
        0x8d, 0x77, 0x67, 0x9c,
        0x11, 0x0f, 0x1f, 0x8a
    }
};
#endif

struct boot_swap_table {
    uint8_t magic_primary_slot;
    uint8_t magic_secondary_slot;
    uint8_t image_ok_primary_slot;
    uint8_t image_ok_secondary_slot;
    uint8_t copy_done_primary_slot;

    uint8_t swap_type;
};

/**
 * This set of tables maps image trailer contents to swap operation type.
 * When searching for a match, these tables must be iterated sequentially.
 *
 * NOTE: the table order is very important. The settings in the secondary
 * slot always are priority to the primary slot and should be located
 * earlier in the table.
 *
 * The table lists only states where there is action needs to be taken by
 * the bootloader, as in starting/finishing a swap operation.
 */
static const struct boot_swap_table boot_swap_tables[] = {
    {
        .magic_primary_slot =       BOOT_MAGIC_ANY,
        .magic_secondary_slot =     BOOT_MAGIC_GOOD,
        .image_ok_primary_slot =    BOOT_FLAG_ANY,
        .image_ok_secondary_slot =  BOOT_FLAG_UNSET,
        .copy_done_primary_slot =   BOOT_FLAG_ANY,
        .swap_type =                BOOT_SWAP_TYPE_TEST,
    },
    {
        .magic_primary_slot =       BOOT_MAGIC_ANY,
        .magic_secondary_slot =     BOOT_MAGIC_GOOD,
        .image_ok_primary_slot =    BOOT_FLAG_ANY,
        .image_ok_secondary_slot =  BOOT_FLAG_SET,
        .copy_done_primary_slot =   BOOT_FLAG_ANY,
        .swap_type =                BOOT_SWAP_TYPE_PERM,
    },
    {
        .magic_primary_slot =       BOOT_MAGIC_GOOD,
        .magic_secondary_slot =     BOOT_MAGIC_UNSET,
        .image_ok_primary_slot =    BOOT_FLAG_UNSET,
        .image_ok_secondary_slot =  BOOT_FLAG_ANY,
        .copy_done_primary_slot =   BOOT_FLAG_SET,
        .swap_type =                BOOT_SWAP_TYPE_REVERT,
    },
};

#define BOOT_SWAP_TABLES_COUNT \
    (sizeof boot_swap_tables / sizeof boot_swap_tables[0])

static int
boot_flag_decode(uint8_t flag)
{
    if (flag != BOOT_FLAG_SET) {
        return BOOT_FLAG_BAD;
    }
    return BOOT_FLAG_SET;
}

uint32_t
boot_swap_info_off(const struct flash_area *fap)
{
    return boot_copy_done_off(fap) - BOOT_MAX_ALIGN;
}

#if CONFIG_DIRECT_XIP
static  uint32_t
boot_xip_magic_off(uint8_t index)
{
	uint32_t phy_offset = 0xFFFFFFFF;
	uint32_t vir_offset = 0xFFFFFFFF;
	if(index == 0) {
		vir_offset = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_XIP_A_PHY_PARTITION_OFFSET)) +
						(FLASH_PHY2VIRTUAL(CONFIG_XIP_A_PHY_PARTITION_SIZE) & (~(4096 - 1))) - 48;
	} else if (index == 1){
		vir_offset = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_XIP_B_PHY_PARTITION_OFFSET)) +
						(FLASH_PHY2VIRTUAL(CONFIG_XIP_B_PHY_PARTITION_SIZE) & (~(4096 - 1))) - 48;
	}
	phy_offset = FLASH_VIRTUAL2PHY(vir_offset);
	return phy_offset;
}

static uint32_t
boot_xip_copy_done_off(uint8_t index)
{
    return boot_xip_magic_off(index) - 8;
}

static uint32_t
boot_xip_image_ok_off(uint8_t index)
{
    return boot_xip_magic_off(index) - 16;
}
#endif

/**
 * Determines if a status source table is satisfied by the specified magic
 * code.
 *
 * @param tbl_val               A magic field from a status source table.
 * @param val                   The magic value in a trailer, encoded as a
 *                                  BOOT_MAGIC_[...].
 *
 * @return                      1 if the two values are compatible;
 *                              0 otherwise.
 */
int
boot_magic_compatible_check(uint8_t tbl_val, uint8_t val)
{
    switch (tbl_val) {
    case BOOT_MAGIC_ANY:
        return 1;

    case BOOT_MAGIC_NOTGOOD:
        return val != BOOT_MAGIC_GOOD;

    default:
        return tbl_val == val;
    }
}

bool bootutil_buffer_is_erased(const struct flash_area *area,
                               const void *buffer, size_t len)
{
    size_t i;
    uint8_t *u8b;
    uint8_t erased_val;

    if (buffer == NULL || len == 0) {
        return false;
    }

    erased_val = flash_area_erased_val(area);
    for (i = 0, u8b = (uint8_t *)buffer; i < len; i++) {
        if (u8b[i] != erased_val) {
            return false;
        }
    }

    return true;
}

static int
boot_read_flag(const struct flash_area *fap, uint8_t *flag, uint32_t off)
{
    int rc;

    rc = flash_area_read(fap, off, flag, sizeof *flag);
    if (rc < 0) {
        return BOOT_EFLASH;
    }
    if (bootutil_buffer_is_erased(fap, flag, sizeof *flag)) {
        *flag = BOOT_FLAG_UNSET;
    } else {
        *flag = boot_flag_decode(*flag);
    }

    return 0;
}

static inline int
boot_read_copy_done(const struct flash_area *fap, uint8_t *copy_done)
{
    return boot_read_flag(fap, copy_done, boot_copy_done_off(fap));
}

#if CONFIG_DIRECT_XIP
int
boot_read_xip_status(uint8_t index, uint32_t type)
{
    uint32_t off = boot_xip_image_ok_off(index);

    if (off == UINT32_MAX) {
        BOOT_LOG_ERR("invalid read image ok offset=0x%08x, type=%d, slot=%d", off, type, index);
        return 0;
    }

    uint32_t status = 0;
    bk_flash_read_bytes(off, &status, 4);

    if ((status != XIP_IMAGE_OK) &&
        (status != XIP_IMAGE_TEST_FIRST) &&
        (status != XIP_IMAGE_TEST_SECOND) &&
        (status != XIP_IMAGE_TEST_FINAL) &&
        (status != XIP_IMAGE_UNKNOWN)) {
        uint32_t sector = off & (~(uint32_t)0xFFF);
        bk_flash_erase_sector(sector);
        bk_flash_read_bytes(off, &status, 4);
    }

    return status;
}

void
boot_write_xip_status(uint8_t index, uint32_t type, uint32_t status)
{
    uint32_t off = boot_xip_image_ok_off(index);

    if (off == UINT32_MAX) {
        BOOT_LOG_ERR("invalid write image ok offset, slot=%d", index);
        return;
    }
    bk_flash_write_bytes(off, &status, 4);
    //TODO re-read the status to ensure the write is successful
}

int boot_read_running_slot(void)
{
    uint32_t offset = CONFIG_XIP_OTA_CONTROL_PHY_PARTITION_OFFSET;
    uint32_t running_slot_value = 0;
    uint32_t running_slot = 0;

    bk_flash_read_bytes(offset, &running_slot_value, 4);
    if (running_slot_value == 0xFFFFFFFF) {
        return 0;
    } else {
        return 1;
    }
    BOOT_LOG_DBG("running slot = %d", slot_str(running_slot));
    return running_slot;
}
#endif

#if CONFIG_OTA_CONFIRM_UPDATE
uint32_t
bk_boot_ota_confirm_offset(uint8_t image_index)
{
#if CONFIG_MIXED_OTA
    if (image_index == 0) {
        return CONFIG_OW_OTA_CONTROL_PHY_PARTITION_OFFSET + CONFIG_OW_OTA_CONTROL_PHY_PARTITION_SIZE - 4;
    } else {
        return CONFIG_XIP_OTA_CONTROL_PHY_PARTITION_OFFSET + CONFIG_XIP_OTA_CONTROL_PHY_PARTITION_SIZE - 4;
    }
#endif
#if CONFIG_OTA_OVERWRITE
    return CONFIG_OW_OTA_CONTROL_PHY_PARTITION_OFFSET + CONFIG_OW_OTA_CONTROL_PHY_PARTITION_SIZE - 4;
#endif
#if CONFIG_DIRECT_XIP
    return CONFIG_XIP_OTA_CONTROL_PHY_PARTITION_OFFSET + CONFIG_XIP_OTA_CONTROL_PHY_PARTITION_SIZE - 4;
#endif
}

bk_err_t 
bk_boot_write_ota_confirm(uint8_t image_index, uint32_t status)
{
    uint8_t retry_cnt = 3;
    uint32_t check_value = 0;
    uint32_t offset = bk_boot_ota_confirm_offset(image_index);
    bk_flash_read_bytes(offset, (uint8_t*)&check_value, 4);
    if (check_value == status) {
        return BK_OK;
    }
    while (retry_cnt--) {
        bk_flash_erase_sector(offset);
        bk_flash_write_bytes(offset, (uint8_t*)&status, 4);
        bk_flash_read_bytes(offset, (uint8_t*)&check_value, 4);
        if (check_value == status) {
            return BK_OK;
        }
    }
    BOOT_LOG_ERR("write ota confirm=%#x fail\r\n", check_value);
    return BK_FAIL;
}

bool
bk_boot_check_ota_confirm(uint8_t index, uint32_t value)
{
    uint32_t phy_off = bk_boot_ota_confirm_offset(index);
    uint32_t status;
    bk_flash_read_bytes(phy_off, &status, 4);

    BOOT_LOG_INF("get image%d ota confirm=%#x",index, status);
    if (status == value) {
        return true;
    } else {
        return false;
    }
}

int
bk_boot_erase_ota_confirm(uint8_t image_index)
{
    uint8_t retry = 3;
    uint32_t check_value;
    uint32_t ff = 0xFFFFFFFF;
    uint32_t phy_off = bk_boot_ota_confirm_offset(image_index);
    bk_flash_read_bytes(phy_off, (uint8_t*)&check_value, 4);
    if (check_value == ff) {
        return 0;
    }

    while(retry--) {
        bk_flash_erase_sector(phy_off);
        bk_flash_read_bytes(phy_off, (uint8_t*)&check_value, 4);
        if (check_value == ff) {
            BOOT_LOG_INF("erase image%d ota confirm", image_index);
            return 0;
        }
    }
    BOOT_LOG_ERR("erase ota confirm=%#x fail\r\n", check_value);
    return BK_FAIL;
}

#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE

#define BOOT_FAIL_STAGE_ERASE         0x4552  /* 'ER' - Erase stage */
#define BOOT_FAIL_STAGE_OTA_DECOMP    0x4F44  /* 'OD' - OTA Decompress stage */
#define BOOT_FAIL_STAGE_APP_VERIFY    0x4156  /* 'AV' - App Verify stage */
#define BOOT_FAIL_STAGE_OTA_VERIFY    0x4F56  /* 'OV' - OTA Verify stage */
#define BOOT_FAIL_STAGE_CBUS_READ     0x4352  /* 'CR' - CBUS Read stage */
#define BOOT_FAIL_STAGE_PUBKEY_VERIFY 0x5056  /* 'PV' - Public Key Verify stage */
#define BOOT_FAIL_STAGE_APP_HDR       0x4148  /* 'AH' - App Header stage */
#define BOOT_FAIL_STAGE_OTA_HDR       0x4F48  /* 'OH' - OTA Header stage */

typedef enum {
    BOOT_FAIL_STAGE_IDX_ERASE = 0,
    BOOT_FAIL_STAGE_IDX_OTA_DECOMP,
    BOOT_FAIL_STAGE_IDX_APP_VERIFY,
    BOOT_FAIL_STAGE_IDX_OTA_VERIFY,
    BOOT_FAIL_STAGE_IDX_CBUS_READ,
    BOOT_FAIL_STAGE_IDX_PUBKEY_VERIFY,
    BOOT_FAIL_STAGE_IDX_APP_HDR,
    BOOT_FAIL_STAGE_IDX_OTA_HDR,
    BOOT_FAIL_STAGE_IDX_MAX
} boot_fail_stage_idx_t;

typedef struct {
    uint16_t error_code;
    uint16_t error_count;
} boot_fail_record_t;

#define BOOT_FAIL_RECORD_SIZE        4
#define BOOT_FAIL_TOTAL_SIZE         128
#define BOOT_FAIL_START_OFFSET       8
#define BOOT_FAIL_MAX_COUNT          16

static const uint16_t boot_fail_count_seq[BOOT_FAIL_MAX_COUNT] = {
    0xFFFF, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF0, 0xFFE0, 0xFFC0, 0xFF80,
    0xFF00, 0xFE00, 0xFC00, 0xF800, 0xF000, 0xE000, 0xC000, 0x8000
};

static uint32_t
bk_boot_overwrite_confirm_off(void)
{
    return (uint32_t)(CONFIG_OW_OTA_CONTROL_PHY_PARTITION_OFFSET
                      + CONFIG_OW_OTA_CONTROL_PHY_PARTITION_SIZE - 4);
}

static uint32_t
bk_boot_overwrite_app_fail_off(void)
{
    return (uint32_t)(CONFIG_OW_OTA_CONTROL_PHY_PARTITION_OFFSET
                      + CONFIG_OW_OTA_CONTROL_PHY_PARTITION_SIZE - 8);
}

static uint16_t
boot_fail_get_error_code(boot_fail_stage_idx_t stage_idx)
{
    switch (stage_idx) {
    case BOOT_FAIL_STAGE_IDX_ERASE:
        return BOOT_FAIL_STAGE_ERASE;
    case BOOT_FAIL_STAGE_IDX_OTA_DECOMP:
        return BOOT_FAIL_STAGE_OTA_DECOMP;
    case BOOT_FAIL_STAGE_IDX_APP_VERIFY:
        return BOOT_FAIL_STAGE_APP_VERIFY;
    case BOOT_FAIL_STAGE_IDX_OTA_VERIFY:
        return BOOT_FAIL_STAGE_OTA_VERIFY;
    case BOOT_FAIL_STAGE_IDX_CBUS_READ:
        return BOOT_FAIL_STAGE_CBUS_READ;
    case BOOT_FAIL_STAGE_IDX_PUBKEY_VERIFY:
        return BOOT_FAIL_STAGE_PUBKEY_VERIFY;
    case BOOT_FAIL_STAGE_IDX_APP_HDR:
        return BOOT_FAIL_STAGE_APP_HDR;
    case BOOT_FAIL_STAGE_IDX_OTA_HDR:
        return BOOT_FAIL_STAGE_OTA_HDR;
    default:
        return 0;
    }
}

static boot_fail_stage_idx_t
boot_fail_get_stage_idx_by_error(uint16_t error_code)
{
    boot_fail_stage_idx_t stage_idx = BOOT_FAIL_STAGE_IDX_MAX;

    for (boot_fail_stage_idx_t idx = 0; idx < BOOT_FAIL_STAGE_IDX_MAX; idx++) {
        if (boot_fail_get_error_code(idx) == error_code) {
            stage_idx = idx;
            break;
        }
    }

    return stage_idx;
}

static uint16_t
boot_fail_get_next_count(uint16_t current_count)
{
    for (int i = 0; i < BOOT_FAIL_MAX_COUNT - 1; i++) {
        if (current_count == boot_fail_count_seq[i]) {
            return boot_fail_count_seq[i + 1];
        }
    }

    return 0x0000;
}

void
bk_boot_write_ota_fail(uint16_t error_code)
{
    boot_fail_stage_idx_t stage_idx = boot_fail_get_stage_idx_by_error(error_code);

    if (stage_idx >= BOOT_FAIL_STAGE_IDX_MAX) {
        BOOT_LOG_ERR("Invalid error_code: 0x%04x", error_code);
        return;
    }

    uint32_t base_off = bk_boot_overwrite_app_fail_off();
    uint32_t record_off = base_off - (uint32_t)(stage_idx * BOOT_FAIL_RECORD_SIZE);

    boot_fail_record_t record;
    bk_flash_read_bytes(record_off, (uint8_t *)&record, sizeof(boot_fail_record_t));

    if (record.error_count == 0x8000) {
        BOOT_LOG_INF("Stage 0x%04x reached max count (15 times)", error_code);
        return;
    } else {
        uint16_t next_count = boot_fail_get_next_count(record.error_count);
        record.error_count = next_count;
    }
    record.error_code |= error_code;

    int retry_count = 3;
    bool write_success = false;

    flash_protect_type_t protect = bk_flash_get_protect_type();
    bk_flash_set_protect_type(FLASH_PROTECT_NONE);

    for (int retry = 0; retry < retry_count; retry++) {
        bk_flash_write_bytes(record_off, (uint8_t *)&record, sizeof(boot_fail_record_t));

        boot_fail_record_t verify_record;
        bk_flash_read_bytes(record_off, (uint8_t *)&verify_record, sizeof(boot_fail_record_t));

        if (verify_record.error_code == record.error_code &&
            verify_record.error_count == record.error_count) {
            BOOT_LOG_ERR("Write record: stage=0x%04x, count=0x%04x, off=0x%x (retry=%d)",
                         record.error_code, record.error_count, record_off, retry);
            write_success = true;
            break;
        } else {
            BOOT_LOG_ERR("Write fail record verify failed after %d retries: expected(0x%04x,0x%04x), got(0x%04x,0x%04x)",
                         retry_count,
                         record.error_code, record.error_count,
                         verify_record.error_code, verify_record.error_count);
        }
    }

    if (!write_success) {
        BOOT_LOG_ERR("Failed to write fail record after %d attempts", retry_count);
    }

    bk_flash_set_protect_type(protect);
}

#endif /* CONFIG_OTA_OVERWRITE */
#endif /* CONFIG_OTA_CONFIRM_UPDATE */


int
boot_read_swap_state(const struct flash_area *fap,
                     struct boot_swap_state *state)
{
    uint8_t magic[BOOT_MAGIC_SZ];
    uint32_t off;
    uint8_t swap_info;
    int rc;

    off = boot_magic_off(fap);
    rc = flash_area_read(fap, off, magic, BOOT_MAGIC_SZ);
    if (rc < 0) {
        return BOOT_EFLASH;
    }
    if (bootutil_buffer_is_erased(fap, magic, BOOT_MAGIC_SZ)) {
        state->magic = BOOT_MAGIC_UNSET;
    } else {
        state->magic = boot_magic_decode(magic);
    }

    off = boot_swap_info_off(fap);
    rc = flash_area_read(fap, off, &swap_info, sizeof swap_info);
    if (rc < 0) {
        return BOOT_EFLASH;
    }

    /* Extract the swap type and image number */
    state->swap_type = BOOT_GET_SWAP_TYPE(swap_info);
    state->image_num = BOOT_GET_IMAGE_NUM(swap_info);

    if (bootutil_buffer_is_erased(fap, &swap_info, sizeof swap_info) ||
            state->swap_type > BOOT_SWAP_TYPE_REVERT) {
        state->swap_type = BOOT_SWAP_TYPE_NONE;
        state->image_num = 0;
    }

    rc = boot_read_copy_done(fap, &state->copy_done);
    if (rc) {
        return BOOT_EFLASH;
    }

    return boot_read_image_ok(fap, &state->image_ok);
}

int
boot_read_swap_state_by_id(int flash_area_id, struct boot_swap_state *state)
{
    const struct flash_area *fap;
    int rc;

    rc = flash_area_open(flash_area_id, &fap);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_read_swap_state(fap, state);
    flash_area_close(fap);
    return rc;
}

int
boot_write_magic(const struct flash_area *fap)
{
    uint32_t off;
    uint32_t pad_off;
    int rc;
    uint8_t magic[BOOT_MAGIC_ALIGN_SIZE];
    uint8_t erased_val;

    off = boot_magic_off(fap);

    /* image_trailer structure was modified with additional padding such that
     * the pad+magic ends up in a flash minimum write region. The address
     * returned by boot_magic_off() is the start of magic which is not the
     * start of the flash write boundary and thus writes to the magic will fail.
     * To account for this change, write to magic is first padded with 0xFF
     * before writing to the trailer.
     */
    pad_off = ALIGN_DOWN(off, BOOT_MAX_ALIGN);

    erased_val = flash_area_erased_val(fap);

    memset(&magic[0], erased_val, sizeof(magic));
    memcpy(&magic[BOOT_MAGIC_ALIGN_SIZE - BOOT_MAGIC_SZ], BOOT_IMG_MAGIC, BOOT_MAGIC_SZ);

    BOOT_LOG_DBG("writing magic; fa_id=%d off=0x%lx (0x%lx)",
                 flash_area_get_id(fap), (unsigned long)off,
                 (unsigned long)(flash_area_get_off(fap) + off));
    rc = flash_area_write(fap, pad_off, &magic[0], BOOT_MAGIC_ALIGN_SIZE);

    if (rc != 0) {
        return BOOT_EFLASH;
    }

    return 0;
}

/**
 * Write trailer data; status bytes, swap_size, etc
 *
 * @returns 0 on success, != 0 on error.
 */
int
boot_write_trailer(const struct flash_area *fap, uint32_t off,
        const uint8_t *inbuf, uint8_t inlen)
{
    uint8_t buf[BOOT_MAX_ALIGN];
    uint8_t erased_val;
    uint32_t align;
    int rc;

    align = flash_area_align(fap);
    align = ALIGN_UP(inlen, align);
    if (align > BOOT_MAX_ALIGN) {
        return -1;
    }
    erased_val = flash_area_erased_val(fap);

    memcpy(buf, inbuf, inlen);
    memset(&buf[inlen], erased_val, align - inlen);

    rc = flash_area_write(fap, off, buf, align);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    return 0;
}

int
boot_write_trailer_flag(const struct flash_area *fap, uint32_t off,
        uint8_t flag_val)
{
    const uint8_t buf[1] = { flag_val };
    return boot_write_trailer(fap, off, buf, 1);
}

int
boot_write_image_ok(const struct flash_area *fap)
{
    uint32_t off;

    off = boot_image_ok_off(fap);
    BOOT_LOG_DBG("writing image_ok; fa_id=%d off=0x%lx (0x%lx)",
                 flash_area_get_id(fap), (unsigned long)off,
                 (unsigned long)(flash_area_get_off(fap) + off));
    return boot_write_trailer_flag(fap, off, BOOT_FLAG_SET);
}

int
boot_read_image_ok(const struct flash_area *fap, uint8_t *image_ok)
{
    return boot_read_flag(fap, image_ok, boot_image_ok_off(fap));
}

/**
 * Writes the specified value to the `swap-type` field of an image trailer.
 * This value is persisted so that the boot loader knows what swap operation to
 * resume in case of an unexpected reset.
 */
int
boot_write_swap_info(const struct flash_area *fap, uint8_t swap_type,
                     uint8_t image_num)
{
    uint32_t off;
    uint8_t swap_info;

    BOOT_SET_SWAP_INFO(swap_info, image_num, swap_type);
    off = boot_swap_info_off(fap);
    BOOT_LOG_DBG("writing swap_info; fa_id=%d off=0x%lx (0x%lx), swap_type=0x%x"
                 " image_num=0x%x",
                 flash_area_get_id(fap), (unsigned long)off,
                 (unsigned long)(flash_area_get_off(fap) + off),
                 swap_type, image_num);
    return boot_write_trailer(fap, off, (const uint8_t *) &swap_info, 1);
}

int
boot_swap_type_multi(int image_index)
{
    const struct boot_swap_table *table;
    struct boot_swap_state primary_slot;
    struct boot_swap_state secondary_slot;
    int rc;
    size_t i;

    rc = BOOT_HOOK_CALL(boot_read_swap_state_primary_slot_hook,
                        BOOT_HOOK_REGULAR, image_index, &primary_slot);
    if (rc == BOOT_HOOK_REGULAR)
    {
        rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_PRIMARY(image_index),
                                        &primary_slot);
    }
    if (rc) {
        return BOOT_SWAP_TYPE_PANIC;
    }

    rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_SECONDARY(image_index),
                                    &secondary_slot);
    if (rc == BOOT_EFLASH) {
        BOOT_LOG_INF("Secondary image of image pair (%d.) "
                     "is unreachable. Treat it as empty", image_index);
        secondary_slot.magic = BOOT_MAGIC_UNSET;
        secondary_slot.swap_type = BOOT_SWAP_TYPE_NONE;
        secondary_slot.copy_done = BOOT_FLAG_UNSET;
        secondary_slot.image_ok = BOOT_FLAG_UNSET;
        secondary_slot.image_num = 0;
    } else if (rc) {
        return BOOT_SWAP_TYPE_PANIC;
    }

    for (i = 0; i < BOOT_SWAP_TABLES_COUNT; i++) {
        table = boot_swap_tables + i;

        if (boot_magic_compatible_check(table->magic_primary_slot,
                                        primary_slot.magic) &&
            boot_magic_compatible_check(table->magic_secondary_slot,
                                        secondary_slot.magic) &&
            (table->image_ok_primary_slot == BOOT_FLAG_ANY   ||
                table->image_ok_primary_slot == primary_slot.image_ok) &&
            (table->image_ok_secondary_slot == BOOT_FLAG_ANY ||
                table->image_ok_secondary_slot == secondary_slot.image_ok) &&
            (table->copy_done_primary_slot == BOOT_FLAG_ANY  ||
                table->copy_done_primary_slot == primary_slot.copy_done)) {
            BOOT_LOG_INF("Image index: %d, Swap type: %s", image_index,
                         table->swap_type == BOOT_SWAP_TYPE_TEST   ? "test"   :
                         table->swap_type == BOOT_SWAP_TYPE_PERM   ? "perm"   :
                         table->swap_type == BOOT_SWAP_TYPE_REVERT ? "revert" :
                         "BUG; can't happen");
            if (table->swap_type != BOOT_SWAP_TYPE_TEST &&
                    table->swap_type != BOOT_SWAP_TYPE_PERM &&
                    table->swap_type != BOOT_SWAP_TYPE_REVERT) {
                return BOOT_SWAP_TYPE_PANIC;
            }
            return table->swap_type;
        }
    }

    BOOT_LOG_INF("Image index: %d, Swap type: none", image_index);
    return BOOT_SWAP_TYPE_NONE;
}

int
boot_write_copy_done(const struct flash_area *fap)
{
    uint32_t off;

    off = boot_copy_done_off(fap);
    BOOT_LOG_DBG("writing copy_done; fa_id=%d off=0x%lx (0x%lx)",
                 flash_area_get_id(fap), (unsigned long)off,
                 (unsigned long)(flash_area_get_off(fap) + off));
    return boot_write_trailer_flag(fap, off, BOOT_FLAG_SET);
}

#ifndef MCUBOOT_BOOTUTIL_LIB_FOR_DIRECT_XIP

static int flash_area_to_image(const struct flash_area *fa)
{
#if BOOT_IMAGE_NUMBER > 1
    uint8_t i = 0;
    int id = flash_area_get_id(fa);

    while (i < BOOT_IMAGE_NUMBER) {
        if (FLASH_AREA_IMAGE_PRIMARY(i) == id || (FLASH_AREA_IMAGE_SECONDARY(i) == id)) {
            return i;
        }

        ++i;
    }
#else
    (void)fa;
#endif
    return 0;
}

int
boot_set_next(const struct flash_area *fa, bool active, bool confirm)
{
    struct boot_swap_state slot_state;
    int rc;

    if (active) {
        confirm = true;
    }

    rc = boot_read_swap_state(fa, &slot_state);
    if (rc != 0) {
        return rc;
    }

    switch (slot_state.magic) {
    case BOOT_MAGIC_GOOD:
        /* If non-active then swap already scheduled, else confirm needed.*/

        if (active && slot_state.image_ok == BOOT_FLAG_UNSET) {
            /* Intentionally do not check copy_done flag to be able to
             * confirm a padded image which has been programmed using
             * a programming interface.
             */
            rc = boot_write_image_ok(fa);
        }

        break;

    case BOOT_MAGIC_UNSET:
        if (!active) {
            rc = boot_write_magic(fa);

            if (rc == 0 && confirm) {
                rc = boot_write_image_ok(fa);
            }

            if (rc == 0) {
                uint8_t swap_type;

                if (confirm) {
                    swap_type = BOOT_SWAP_TYPE_PERM;
                } else {
                    swap_type = BOOT_SWAP_TYPE_TEST;
                }
                rc = boot_write_swap_info(fa, swap_type, flash_area_to_image(fa));
            }
        }
        break;

    case BOOT_MAGIC_BAD:
        if (active) {
            rc = BOOT_EBADVECT;
        } else {
            /* The image slot is corrupt.  There is no way to recover, so erase the
             * slot to allow future upgrades.
             */
            flash_area_erase(fa, 0, flash_area_get_size(fa));
            rc = BOOT_EBADIMAGE;
        }
        break;

    default:
        /* Something is not OK, this should never happen */
        assert(0);
        rc = BOOT_EBADIMAGE;
    }

    return rc;
}
#else
int
boot_set_next(const struct flash_area *fa, bool active, bool confirm)
{
    struct boot_swap_state slot_state;
    int rc;

    if (active) {
        /* The only way to set active slot for next boot is to confirm it,
         * as DirectXIP will conclude that, since slot has not been confirmed
         * last boot, it is bad and will remove it.
         */
        confirm = true;
    }

    rc = boot_read_swap_state(fa, &slot_state);
    if (rc != 0) {
        return rc;
    }

    switch (slot_state.magic) {
    case BOOT_MAGIC_UNSET:
        /* Magic is needed for MCUboot to even consider booting an image */
        rc = boot_write_magic(fa);
        if (rc != 0) {
            break;
        }
        /* Pass */

    case BOOT_MAGIC_GOOD:
        if (confirm) {
            if (slot_state.copy_done == BOOT_FLAG_UNSET) {
                /* Magic is needed for DirectXIP to even try to boot application.
                 * DirectXIP will set copy-done flag before attempting to boot
                 * application. Next boot, application that has copy-done flag
                 * is expected to already have ok flag, otherwise it will be removed.
                 */
                rc = boot_write_copy_done(fa);
                if (rc != 0) {
                    break;
                }
            }

            if (slot_state.image_ok == BOOT_FLAG_UNSET) {
                rc = boot_write_image_ok(fa);
                if (rc != 0) {
                    break;
                }
            }
        }
        break;

    case BOOT_MAGIC_BAD:
        /* This image will not be boot next time anyway */
        rc = BOOT_EBADIMAGE;
        break;

    default:
        /* Something is not OK, this should never happen */
        assert(0);
        rc = BOOT_EBADSTATUS;
    }

    return rc;
}
#endif

/*
 * This function is not used by the bootloader itself, but its required API
 * by external tooling like mcumgr.
 */
int
boot_swap_type(void)
{
    return boot_swap_type_multi(0);
}

/**
 * Marks the image with the given index in the secondary slot as pending. On the
 * next reboot, the system will perform a one-time boot of the the secondary
 * slot image.
 *
 * @param image_index       Image pair index.
 *
 * @param permanent         Whether the image should be used permanently or
 *                          only tested once:
 *                               0=run image once, then confirm or revert.
 *                               1=run image forever.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
boot_set_pending_multi(int image_index, int permanent)
{
    const struct flash_area *fap;
    int rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_SECONDARY(image_index), &fap);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_set_next(fap, false, !(permanent == 0));

    flash_area_close(fap);
    return rc;
}

/**
 * Marks the image with index 0 in the secondary slot as pending. On the next
 * reboot, the system will perform a one-time boot of the the secondary slot
 * image. Note that this API is kept for compatibility. The
 * boot_set_pending_multi() API is recommended.
 *
 * @param permanent         Whether the image should be used permanently or
 *                          only tested once:
 *                               0=run image once, then confirm or revert.
 *                               1=run image forever.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
boot_set_pending(int permanent)
{
    return boot_set_pending_multi(0, permanent);
}

/**
 * Marks the image with the given index in the primary slot as confirmed.  The
 * system will continue booting into the image in the primary slot until told to
 * boot from a different slot.
 *
 * @param image_index       Image pair index.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
boot_set_confirmed_multi(int image_index)
{
    const struct flash_area *fap = NULL;
    int rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(image_index), &fap);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_set_next(fap, true, true);

    flash_area_close(fap);
    return rc;
}

/**
 * Marks the image with index 0 in the primary slot as confirmed.  The system
 * will continue booting into the image in the primary slot until told to boot
 * from a different slot.  Note that this API is kept for compatibility. The
 * boot_set_confirmed_multi() API is recommended.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
boot_set_confirmed(void)
{
    return boot_set_confirmed_multi(0);
}

int
boot_image_load_header(const struct flash_area *fa_p,
                       struct image_header *hdr)
{
    uint32_t size;
    int rc = flash_area_read(fa_p, 0, hdr, sizeof *hdr);

    if (rc != 0) {
        rc = BOOT_EFLASH;
        BOOT_LOG_ERR("Failed reading image header");
	return BOOT_EFLASH;
    }

    if (hdr->ih_magic != IMAGE_MAGIC) {
        BOOT_LOG_ERR("Bad image magic 0x%lx", (unsigned long)hdr->ih_magic);

        return BOOT_EBADIMAGE;
    }

    if (hdr->ih_flags & IMAGE_F_NON_BOOTABLE) {
        BOOT_LOG_ERR("Image not bootable");

        return BOOT_EBADIMAGE;
    }

    if (!boot_u32_safe_add(&size, hdr->ih_img_size, hdr->ih_hdr_size) ||
        size >= flash_area_get_size(fa_p)) {
        return BOOT_EBADIMAGE;
    }

    return 0;
}

#if CONFIG_DBUS_CHECK_CRC
static uint16_t crc16_table[256];
static bool is_table_generated = false;

typedef struct flash_data {
    uint8_t data[32];
    uint16_t crc16;
} flash_data_t;

static void generate_crc16_table(uint16_t *table, uint16_t polynomial)
{
    for (int i = 0; i< 256; i++) {
        uint16_t crc = i << 8;
        for (int j = 0; j < 8; j++) {
            if ( crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = crc << 1;
            }
        }
        table[i] = crc;
    }
    is_table_generated = true;
}

uint16_t calculate_crc16(const uint8_t *data, size_t length) 
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc >> 8) ^ data[i];
        crc = (crc << 8) ^ crc16_table[index];
    }
    return ((crc >> 8) & 0xFF) | ((crc & 0xFF) << 8);
}

bool check_crc(uint32_t vir_start, size_t size)
{
    flash_data_t enc_data;
    uint32_t phy_start = (vir_start >> 5) * 34;
    uint32_t ff_buf[34];

    size = (size + 32 - 1) & ~(32 - 1); // 32 align_up
    size = (size >> 5) *34;

    if (!is_table_generated) {
        uint16_t polynomial = 0x8005;
        generate_crc16_table(crc16_table, polynomial);
    }

    for (size_t i = 0; i < size / 34; i++) {
#if CONFIG_BL2_WDT
        wdt_hal_force_feed();
#endif
        bk_flash_read_bytes(phy_start + i*34, (uint8_t *)&enc_data, 34);
        memset(ff_buf, 0xFF, 34);
        if (memcmp(ff_buf, (const void *)&enc_data, 34) == 0) {
            continue;
        }
        if (enc_data.crc16 != calculate_crc16(enc_data.data, 32)) {
            BOOT_LOG_ERR("read crc = %#x,cal crc =%#x, addr=%#x",enc_data.crc16, calculate_crc16(enc_data.data, 32), phy_start + i*34);
            return false;
        }
    }
    return true;
}

#endif
