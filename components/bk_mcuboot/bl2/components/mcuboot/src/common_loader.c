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

#include "common_loader.h"
#include <driver/flash.h>

int
boot_add_shared_data(struct boot_loader_state *state,
                    uint8_t active_slot)
{
#if defined(MCUBOOT_MEASURED_BOOT) || defined(MCUBOOT_DATA_SHARING)
    int rc;

#ifdef MCUBOOT_DATA_SHARING
    int max_app_size;
#endif

#ifdef MCUBOOT_MEASURED_BOOT
    rc = boot_save_boot_status(BOOT_CURR_IMG(state),
                                boot_img_hdr(state, active_slot),
                                BOOT_IMG_AREA(state, active_slot));
    if (rc != 0) {
        BOOT_LOG_ERR("Failed to add image data to shared area");
        return rc;
    }
#endif /* MCUBOOT_MEASURED_BOOT */

#ifdef MCUBOOT_DATA_SHARING
    max_app_size = app_max_size(state);
    rc = boot_save_shared_data(boot_img_hdr(state, active_slot),
                                BOOT_IMG_AREA(state, active_slot),
                                active_slot, max_app_size);
    if (rc != 0) {
        BOOT_LOG_ERR("Failed to add data to shared memory area.");
        return rc;
    }
#endif /* MCUBOOT_DATA_SHARING */

    return 0;

#else /* MCUBOOT_MEASURED_BOOT || MCUBOOT_DATA_SHARING */
    (void) (state);
    (void) (active_slot);

    return 0;
#endif
}

int
boot_update_hw_rollback_protection(struct boot_loader_state *state)
{
#ifdef MCUBOOT_HW_ROLLBACK_PROT
    int rc;

    /* Update the stored security counter with the active image's security
    * counter value. It will only be updated if the new security counter is
    * greater than the stored value.
    *
    * In case of a successful image swapping when the swap type is TEST the
    * security counter can be increased only after a reset, when the swap
    * type is NONE and the image has marked itself "OK" (the image_ok flag
    * has been set). This way a "revert" can be performed when it's
    * necessary.
    */
    if (BOOT_SWAP_TYPE(state) == BOOT_SWAP_TYPE_NONE) {
        rc = boot_update_security_counter(
                                BOOT_CURR_IMG(state),
                                BOOT_PRIMARY_SLOT,
                                boot_img_hdr(state, BOOT_PRIMARY_SLOT));
        if (rc != 0) {
            BOOT_LOG_ERR("Security counter update failed after image "
                            "validation.");
            return rc;
        }
    }

    return 0;

#else /* MCUBOOT_HW_ROLLBACK_PROT */
    (void) (state);

    return 0;
#endif
}

static inline bool
boot_data_is_set_to(uint8_t val, void *data, size_t len)
{
    uint8_t i;
    uint8_t *p = (uint8_t *)data;
    for (i = 0; i < len; i++) {
        if (val != p[i]) {
            return false;
        }
    }
    return true;
}

int
boot_check_header_erased(struct boot_loader_state *state, int slot)
{
    const struct flash_area *fap;
    struct image_header *hdr;
    uint8_t erased_val;
    int area_id;
    int rc;

    area_id = flash_area_id_from_multi_image_slot(BOOT_CURR_IMG(state), slot);
    rc = flash_area_open(area_id, &fap);
    if (rc != 0) {
        return -1;
    }

    erased_val = flash_area_erased_val(fap);
    flash_area_close(fap);

    hdr = boot_img_hdr(state, slot);
    if (!boot_data_is_set_to(erased_val, &hdr->ih_magic, sizeof(hdr->ih_magic))) {
        return -1;
    }

    return 0;
}

int
boot_erase_region(const struct flash_area *fap, uint32_t off, uint32_t sz)
{
    return flash_area_erase(fap, off, sz);
}


static fih_ret
boot_image_check(struct boot_loader_state *state, struct image_header *hdr,
                const struct flash_area *fap, struct boot_status *bs)
{
    TARGET_STATIC uint8_t tmpbuf[BOOT_TMPBUF_SZ];
    uint8_t image_index;
    int rc;
    FIH_DECLARE(fih_rc, FIH_FAILURE);

#if (BOOT_IMAGE_NUMBER == 1)
    (void)state;
#endif

    (void)bs;
    (void)rc;

    image_index = BOOT_CURR_IMG(state);

    FIH_CALL(bootutil_img_validate, fih_rc, BOOT_CURR_ENC(state), image_index,
            hdr, fap, tmpbuf, BOOT_TMPBUF_SZ, NULL, 0, NULL);

    FIH_RET(fih_rc);
}

bool
boot_is_header_valid(const struct image_header *hdr, const struct flash_area *fap)
{
    uint32_t size;
    
    BOOT_LOG_DBG("hdr: magic=0x%x, img_size=0x%x, hdr_size=0x%x", hdr->ih_magic, hdr->ih_img_size, hdr->ih_hdr_size);

    if (hdr->ih_magic != IMAGE_MAGIC) {
        BOOT_LOG_DBG("magic mismatch, hdr->ih_magic=0x%x, expected=0x%x", 
                     hdr->ih_magic, IMAGE_MAGIC);
        return false;
    }

    if (!boot_u32_safe_add(&size, hdr->ih_img_size, hdr->ih_hdr_size)) {
        BOOT_LOG_DBG("size overflow");
        return false;
    }

    if (size >= flash_area_get_size(fap)) {
        BOOT_LOG_DBG("size too large, size=0x%x, fa_size=0x%x", 
                     size, flash_area_get_size(fap));
        return false;
    }

    return true;
}

bool bk_skip_validate()
{
#if CONFIG_BL2_SKIP_VALIDATE
    uint32_t reset_reason = aon_pmu_hal_get_reset_reason();
    BOOT_LOG_INF("reset reason = %#x",reset_reason);

    if (reset_reason == RESET_SOURCE_POWERON || reset_reason == RESET_SOURCE_BOOTLOADER_UNKNOWN) {
        BOOT_LOG_INF("skip validate");
        return true;
    }
    else {
        return false;
    }
#else
    return false;
#endif
}

fih_ret
boot_validate_slot(struct boot_loader_state *state, int slot,
                struct boot_status *bs)
{
    const struct flash_area *fap;
    struct image_header *hdr;
    int area_id;
    FIH_DECLARE(fih_rc, FIH_FAILURE);
    int rc;

    area_id = flash_area_id_from_multi_image_slot(BOOT_CURR_IMG(state), slot);
    rc = flash_area_open(area_id, &fap);
    if (rc != 0) {
        FIH_RET(fih_rc);
    }

    hdr = boot_img_hdr(state, slot);
    if (boot_check_header_erased(state, slot) == 0 ||
        (hdr->ih_flags & IMAGE_F_NON_BOOTABLE)) {
        /* No bootable image in slot; continue booting from the primary slot. */
        fih_rc = FIH_NO_BOOTABLE_IMAGE;
        BOOT_LOG_ERR("boot img hdr fail");
        goto out;
    }

    BOOT_LOG_INF("starting validation of %s", slot_str(slot));
    BOOT_HOOK_CALL_FIH(boot_image_check_hook, FIH_BOOT_HOOK_REGULAR,
                    fih_rc, BOOT_CURR_IMG(state), slot);
    if (FIH_EQ(fih_rc, FIH_BOOT_HOOK_REGULAR))
    {
        FIH_CALL(boot_image_check, fih_rc, state, hdr, fap, bs);
    }
    BOOT_LOG_INF("%s is verified", slot_str(slot));
    if (!boot_is_header_valid(hdr, fap) || FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
        if ((slot != BOOT_PRIMARY_SLOT) || ARE_SLOTS_EQUIVALENT()) {
            BOOT_LOG_ERR("%s image is not valid!", slot_str(slot));
        }
        fih_rc = FIH_NO_BOOTABLE_IMAGE;
        goto out;
    }

out:
    flash_area_close(fap);

    FIH_RET(fih_rc);
}

int boot_read_image_size(struct boot_loader_state *state, int slot, uint32_t *size)
{
    const struct flash_area *fap;
    struct image_tlv_info info;
    uint32_t off;
    uint32_t protect_tlv_size;
    int area_id;
    int rc;

#if (BOOT_IMAGE_NUMBER == 1)
    (void)state;
#endif

    area_id = flash_area_id_from_multi_image_slot(BOOT_CURR_IMG(state), slot);
    rc = flash_area_open(area_id, &fap);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    off = BOOT_TLV_OFF(boot_img_hdr(state, slot));

    if (flash_area_read(fap, off, &info, sizeof(info))) {
        rc = BOOT_EFLASH;
        goto done;
    }

    protect_tlv_size = boot_img_hdr(state, slot)->ih_protect_tlv_size;
    if (info.it_magic == IMAGE_TLV_PROT_INFO_MAGIC) {
        if (protect_tlv_size != info.it_tlv_tot) {
            rc = BOOT_EBADIMAGE;
            goto done;
        }

        if (flash_area_read(fap, off + info.it_tlv_tot, &info, sizeof(info))) {
            rc = BOOT_EFLASH;
            goto done;
        }
    } else if (protect_tlv_size != 0) {
        rc = BOOT_EBADIMAGE;
        goto done;
    }

    if (info.it_magic != IMAGE_TLV_INFO_MAGIC) {
        rc = BOOT_EBADIMAGE;
        goto done;
    }

    *size = off + protect_tlv_size + info.it_tlv_tot;
    rc = 0;

done:
    flash_area_close(fap);
    return rc;
}

int
boot_read_image_header(struct boot_loader_state *state, int slot,
                    struct image_header *out_hdr, struct boot_status *bs)
{
    const struct flash_area *fap;
    int area_id;
    int rc = 0;

    (void)bs;

    area_id = flash_area_id_from_multi_image_slot(BOOT_CURR_IMG(state), slot);
    rc = flash_area_open(area_id, &fap);
    if (rc == 0) {
        rc = flash_area_read(fap, 0, out_hdr, sizeof *out_hdr);
        if (rc != 0) {
            BOOT_LOG_ERR("boot_read_image_header: flash_area_read failed, rc=%d", rc);
        }
        flash_area_close(fap);
    } else {
        BOOT_LOG_ERR("boot_read_image_header: flash_area_open failed, area_id=%d, rc=%d", area_id, rc);
    }

    if (rc != 0) {
        rc = BOOT_EFLASH;
    }

    return rc;
}

int
boot_read_image_headers(struct boot_loader_state *state, bool require_all,
        struct boot_status *bs)
{
    int rc;
    int i;
    for (i = 0; i < BOOT_NUM_SLOTS; i++) {
        rc = BOOT_HOOK_CALL(boot_read_image_header_hook, BOOT_HOOK_REGULAR,
                            BOOT_CURR_IMG(state), i, boot_img_hdr(state, i));
        if (rc == BOOT_HOOK_REGULAR)
        {
            rc = boot_read_image_header(state, i, boot_img_hdr(state, i), bs);
        }
        if (rc != 0) {
            /* If `require_all` is set, fail on any single fail, otherwisetianxin
            * if at least the first slot's header was read successfully,
            * then the boot loader can attempt a boot.
            *
            * Failure to read any headers is a fatal error.
            */
            if (i > 0 && !require_all) {
                return 0;
            } else {
                return rc;
            }
        }
    }

    return 0;
}


/**
 * Fills rsp to indicate how booting should occur.
 *
 * @param  state        Boot loader status information.
 * @param  rsp          boot_rsp struct to fill.
 */
void
fill_rsp(struct boot_loader_state *state, struct boot_rsp *rsp)
{
    uint32_t active_slot;

#if CONFIG_MIXED_OTA
    BOOT_CURR_IMG(state) = 1;
#endif
    
#if CONFIG_DIRECT_XIP || CONFIG_MIXED_OTA
    /* For XIP mode: Check if xip_boot_go has already set the rsp */
    /* xip_boot_go re-reads the header from flash and sets rsp->br_hdr correctly */
    /* We should not overwrite it if it's already set correctly */
    if (state->slot_usage[BOOT_CURR_IMG(state)].active_slot != NO_ACTIVE_SLOT) {
        active_slot = state->slot_usage[BOOT_CURR_IMG(state)].active_slot;
        uint32_t slot_off = boot_img_slot_off(state, active_slot);
        
        /* Check if rsp has already been set by xip_boot_go */
        /* If image_off matches the active_slot's offset, rsp was already set correctly */
        if (rsp->br_image_off == slot_off && rsp->br_hdr != NULL) {
            /* rsp has already been set by xip_boot_go, don't overwrite it */
            return;
        }
    } else {
        /* Fallback logic: prefer PRIMARY_SLOT if available, otherwise try SECONDARY_SLOT */
        /* This handles cases where xip_boot_go didn't set active_slot (e.g., first boot) */
        if (state->slot_usage[BOOT_CURR_IMG(state)].slot_available[BOOT_PRIMARY_SLOT]) {
            active_slot = BOOT_PRIMARY_SLOT;
        } else if (state->slot_usage[BOOT_CURR_IMG(state)].slot_available[BOOT_SECONDARY_SLOT]) {
            active_slot = BOOT_SECONDARY_SLOT;
        } else {
            /* If no slot is available, default to PRIMARY_SLOT (original behavior) */
            active_slot = BOOT_PRIMARY_SLOT;
        }
    }
#else
    /* For overwrite mode: Always use PRIMARY_SLOT (original behavior) */
    active_slot = BOOT_PRIMARY_SLOT;
#endif

    rsp->br_flash_dev_id = flash_area_get_device_id(BOOT_IMG_AREA(state, active_slot));
    rsp->br_image_off = boot_img_slot_off(state, active_slot);
    rsp->br_hdr = boot_img_hdr(state, active_slot);
}

#if CONFIG_ANTI_ROLLBACK
int
boot_update_security_counter(uint8_t image_index, int slot,
                            struct image_header *hdr)
{
    const struct flash_area *fap = NULL;
    uint32_t img_security_cnt;
    int rc;

    rc = flash_area_open(flash_area_id_from_multi_image_slot(image_index, slot),
                        &fap);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = bootutil_get_img_security_cnt(hdr, fap, &img_security_cnt);
    if (rc != 0) {
        goto done;
    }

    rc = boot_nv_security_counter_update(image_index, img_security_cnt);
    if (rc != 0) {
        goto done;
    }

done:
    flash_area_close(fap);
    return rc;
}
#endif /* CONFIG_ANTI_ROLLBACK */
