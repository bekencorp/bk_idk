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

#include "xip_loader.h"
#include "flash_driver.h"
#include "cache.h"
#include "bootutil_priv.h"

static inline const char* image_ok_str(uint32_t image_ok)
{
	if (image_ok == XIP_IMAGE_OK) {
		return "OK";
	} else if (image_ok == XIP_IMAGE_TEST_FIRST) {
		return "FIRST_TEST_FAIL";
	} else if (image_ok == XIP_IMAGE_TEST_SECOND) {
		return "SECOND_TEST_FAIL";
	} else if (image_ok == XIP_IMAGE_TEST_FINAL) {
		return "FINAL_TEST_FAIL";
	} else if (image_ok == XIP_IMAGE_UNKNOWN) {
		return "DOWNLOADED";
	} else {
		return "INVALID";
	}
}

static int
boot_version_cmp(const struct image_version *ver1,
                 const struct image_version *ver2)
{
    if (ver1->iv_major > ver2->iv_major) {
        return 1;
    }
    if (ver1->iv_major < ver2->iv_major) {
        return -1;
    }
    /* The major version numbers are equal, continue comparison. */
    if (ver1->iv_minor > ver2->iv_minor) {
        return 1;
    }
    if (ver1->iv_minor < ver2->iv_minor) {
        return -1;
    }
    /* The minor version numbers are equal, continue comparison. */
    if (ver1->iv_revision > ver2->iv_revision) {
        return 1;
    }
    if (ver1->iv_revision < ver2->iv_revision) {
        return -1;
    }

    return 0;
}

#if CONFIG_ANTI_ROLLBACK
static uint32_t
find_slot_with_highest_version(struct boot_loader_state *state)
{
    uint32_t slot;
    uint32_t candidate_slot = NO_ACTIVE_SLOT;
    
    for (slot = 0; slot < BOOT_NUM_SLOTS; slot++) {
        if (state->slot_usage[BOOT_CURR_IMG(state)].slot_available[slot]) {
            if (candidate_slot == NO_ACTIVE_SLOT) {
                candidate_slot = slot;
            } else {
                int rc = boot_version_cmp(
                            &boot_img_hdr(state, slot)->ih_ver,
                            &boot_img_hdr(state, candidate_slot)->ih_ver);
                if (rc == 1) {
                    /* The version of the image being examined is greater than
                     * the version of the current candidate.
                     */
                    candidate_slot = slot;
                }
            }
        }
    }
    return candidate_slot;
}

#else

static inline uint32_t xip_get_peer_slot(uint32_t slot)
{
    if (slot == BOOT_PRIMARY_SLOT) {
        return BOOT_SECONDARY_SLOT;
    } else {
        return BOOT_PRIMARY_SLOT;
    }
}

static uint32_t
find_slot_via_ota_confirm_and_image_state(struct boot_loader_state *state, bool ota_confirm_set, uint8_t *slot_retry_count)
{
    uint32_t invalid_slot_num = 0;

    uint32_t running_slot = boot_read_running_slot();
    uint32_t preferred_slot = 0;

    BOOT_LOG_INF("running slot %s", slot_str(running_slot));
#if CONFIG_OTA_CONFIRM_UPDATE
    if (ota_confirm_set) {
        preferred_slot = xip_get_peer_slot(running_slot);
        BOOT_LOG_INF("OTA confirm set, try to use %s", slot_str(preferred_slot));
    } else {
        preferred_slot = running_slot;
        BOOT_LOG_INF("OTA confirm not set, try to use %s", slot_str(preferred_slot));
    }
#else
    preferred_slot = running_slot;
    BOOT_LOG_INF("OTA confirm not set, try to use %s", slot_str(preferred_slot));
#endif

    if ((state->slot_usage[BOOT_CURR_IMG(state)].slot_available[preferred_slot] == false) || (slot_retry_count[preferred_slot] >= 3)) {
        BOOT_LOG_INF("%s retry=%d, hdr_valid=%d, switch to peer slot", slot_str(preferred_slot), slot_retry_count[preferred_slot], state->slot_usage[BOOT_CURR_IMG(state)].slot_available[preferred_slot]);
        preferred_slot = xip_get_peer_slot(preferred_slot);
        invalid_slot_num++;
        if ((state->slot_usage[BOOT_CURR_IMG(state)].slot_available[preferred_slot] == false) || (slot_retry_count[preferred_slot] >= 3)) {
            BOOT_LOG_INF("both A/B retry=%d, hdr_valid=%d", slot_retry_count[preferred_slot], state->slot_usage[BOOT_CURR_IMG(state)].slot_available[preferred_slot]);
            return NO_ACTIVE_SLOT;
        }
    }

    uint32_t image_status = boot_read_xip_status(preferred_slot, XIP_IMAGE_OK_TYPE);

    if (image_status == XIP_IMAGE_TEST_FINAL) {
        preferred_slot = xip_get_peer_slot(preferred_slot);
        invalid_slot_num++;
        BOOT_LOG_INF("%s test fail, switch to %s", slot_str(xip_get_peer_slot(preferred_slot)), slot_str(preferred_slot));
    } else if (image_status == XIP_IMAGE_UNKNOWN) {
        boot_write_xip_status(preferred_slot, XIP_IMAGE_OK_TYPE, XIP_IMAGE_TEST_FIRST);
        BOOT_LOG_INF("%s state UNKNOWN -> TEST_FIRST", slot_str(preferred_slot));
    } else if (image_status == XIP_IMAGE_TEST_FIRST) {
        boot_write_xip_status(preferred_slot, XIP_IMAGE_OK_TYPE, XIP_IMAGE_TEST_SECOND);
        BOOT_LOG_INF("%s state TEST_FIRST -> TEST_SECOND", slot_str(preferred_slot));
    } else if (image_status == XIP_IMAGE_TEST_SECOND) {
        boot_write_xip_status(preferred_slot, XIP_IMAGE_OK_TYPE, XIP_IMAGE_TEST_FINAL);
        BOOT_LOG_INF("%s state TEST_SECOND -> TEST_FINAL", slot_str(preferred_slot));
    } else {
        BOOT_LOG_INF("%s state %s", slot_str(preferred_slot), image_ok_str(image_status));
    }

    if (invalid_slot_num >= BOOT_NUM_SLOTS) {
        return NO_ACTIVE_SLOT;
    }

    return preferred_slot;
}
#endif

static uint32_t
find_preferred_slot(struct boot_loader_state *state, bool ota_confirm_set, uint8_t *slot_retry_count)
{
#if CONFIG_ANTI_ROLLBACK
    return find_slot_with_highest_version(state);
#else
    return find_slot_via_ota_confirm_and_image_state(state, ota_confirm_set, slot_retry_count);
#endif
}

static int
boot_get_slot_usage(struct boot_loader_state *state)
{
    uint32_t slot;
    int fa_id;
    int rc;
    struct image_header *hdr = NULL;


    /* Open all the slots */
    for (slot = 0; slot < BOOT_NUM_SLOTS; slot++) {
        fa_id = flash_area_id_from_multi_image_slot(
                                            BOOT_CURR_IMG(state), slot);
        rc = flash_area_open(fa_id, &BOOT_IMG_AREA(state, slot));
        assert(rc == 0);
    }

    /* Attempt to read an image header from each slot. */
    rc = boot_read_image_headers(state, false, NULL);
    if (rc != 0) {
        BOOT_LOG_WRN("Failed reading image headers.");
        return rc;
    }

    /* Check headers in all slots */
    for (slot = 0; slot < BOOT_NUM_SLOTS; slot++) {
        hdr = boot_img_hdr(state, slot);

        if (boot_is_header_valid(hdr, BOOT_IMG_AREA(state, slot))) {
            state->slot_usage[BOOT_CURR_IMG(state)].slot_available[slot] = true;
            state->slot_usage[BOOT_CURR_IMG(state)].slot_state[slot] = FIRST_TEST;
            BOOT_LOG_IMAGE_INFO(slot, hdr);
        } else {
            state->slot_usage[BOOT_CURR_IMG(state)].slot_available[slot] = false;
            BOOT_LOG_INF("image%d %s: not found", BOOT_CURR_IMG(state), slot_str(slot));
        }
    }

    state->slot_usage[BOOT_CURR_IMG(state)].active_slot = NO_ACTIVE_SLOT;

    return 0;
}


fih_ret
boot_load_and_validate_images(struct boot_loader_state *state)
{
    uint32_t active_slot;
    uint32_t preferred_slot;
    fih_ret fih_rc;
    bool ota_confirm_set = false;
    uint8_t retry_count = 0;
    uint8_t slot_retry_count[BOOT_NUM_SLOTS] = {0};

    ota_confirm_set = bk_boot_check_ota_confirm(1, OTA_CONFIRM);
    if (ota_confirm_set){
        BOOT_LOG_INF("ota confirm=0x%08x", OTA_CONFIRM);
    }

    while (retry_count < 6) {
        retry_count++;

        /* Go over all the slots and try to load one */
        active_slot = state->slot_usage[BOOT_CURR_IMG(state)].active_slot;
        if (active_slot != NO_ACTIVE_SLOT){
            /* A slot is already active, go to next image. */
            break;
        }

        preferred_slot = find_preferred_slot(state, ota_confirm_set, slot_retry_count);
        if (preferred_slot == NO_ACTIVE_SLOT) {
            BOOT_LOG_ERR("both A/B invalid");
            FIH_RET(FIH_FAILURE);
            break;
        }
 
        slot_retry_count[preferred_slot]++;
        FIH_CALL(boot_validate_slot, fih_rc, state, preferred_slot, NULL);
        if (FIH_EQ(fih_rc, FIH_SUCCESS)) {
            state->slot_usage[BOOT_CURR_IMG(state)].active_slot = preferred_slot;
            BOOT_LOG_INF("%s is valid, will boot from it", slot_str(preferred_slot));
            break;
        }
    }

    FIH_RET(FIH_SUCCESS);
}

fih_ret
xip_boot_go(struct boot_loader_state *state, struct boot_rsp *rsp)
{
    FIH_DECLARE(fih_rc, FIH_SUCCESS);
    int rc;

    BOOT_LOG_INF("xip begin");
    BOOT_CURR_IMG(state) = 1;
    rc = boot_get_slot_usage(state);
    FIH_CALL(boot_load_and_validate_images, fih_rc, state);
    if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
        FIH_SET(fih_rc, FIH_FAILURE);
        goto out;
    }

    uint32_t active_slot = state->slot_usage[BOOT_CURR_IMG(state)].active_slot;
    flash_set_excute_enable(active_slot);
    
    /* Re-read the header from flash to ensure we have the correct, up-to-date header */
    /* This is important especially when the other slot's data is corrupted */
    struct image_header *active_hdr = boot_img_hdr(state, active_slot);
    const struct flash_area *active_fap = BOOT_IMG_AREA(state, active_slot);
    rc = flash_area_read(active_fap, 0, active_hdr, sizeof(struct image_header));
    if (rc != 0) {
        BOOT_LOG_ERR("Failed to re-read header from %s slot", slot_str(active_slot));
        FIH_SET(fih_rc, FIH_FAILURE);
        goto out;
    }
    
    /* Verify the header is valid before using it */
    if (!boot_is_header_valid(active_hdr, active_fap)) {
        BOOT_LOG_ERR("Re-read header from %s slot is invalid", slot_str(active_slot));
        FIH_SET(fih_rc, FIH_FAILURE);
        goto out;
    }
    
    /* Fill the boot response structure with the actual active slot */
    rsp->br_flash_dev_id = flash_area_get_device_id(active_fap);
    rsp->br_image_off = boot_img_slot_off(state, active_slot);
    rsp->br_hdr = active_hdr;
    BOOT_LOG_FORCE("Jump to %s\r\n",slot_str(active_slot));
    
out:
    if (rc != 0) {
        FIH_SET(fih_rc, FIH_FAILURE);
    }

    FIH_RET(fih_rc);
}
