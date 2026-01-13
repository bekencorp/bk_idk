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

static bool g_fast_boot_secondary = false;

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

static uint32_t
find_slot_via_version_and_ota_confirm(struct boot_loader_state *state, bool ota_confirm_set)
{
    uint32_t slot;
    uint32_t candidate_slot = NO_ACTIVE_SLOT;

    g_fast_boot_secondary = false;

    uint32_t ota_preferred_slot;
    
#if CONFIG_OTA_CONFIRM_UPDATE
    /* Use cached OTA confirm status passed as parameter */
    if (ota_confirm_set) {
        ota_preferred_slot = 1;
    } else {
        ota_preferred_slot = 0;
    }
#else
    /*TODO use magic in image to decide which image to load*/
    ota_preferred_slot = 0;
#endif

    /* First, determine the preferred slot based on version comparison if both slots are available */
    /* Only allow version-based selection if OTA confirm flag is set */
    uint32_t version_preferred_slot = NO_ACTIVE_SLOT;
    bool prefer_by_version = false;
    
    /* Check if only one slot is available - use it directly */
    bool slot0_available = state->slot_usage[BOOT_CURR_IMG(state)].slot_available[0];
    bool slot1_available = state->slot_usage[BOOT_CURR_IMG(state)].slot_available[1];
    
    if (slot0_available && !slot1_available) {
        /* Only slot0 available - use it directly */
        slot = 0;
        BOOT_LOG_INF("Only A available, using A");
    } else if (!slot0_available && slot1_available) {
        /* Only slot1 available - use it directly */
        slot = 1;
        BOOT_LOG_INF("Only B available, using B");
    } else if (slot0_available && slot1_available) {
        /* Both slots available - compare versions and choose the higher one */
        struct image_header *hdr_slot0 = boot_img_hdr(state, 0);
        struct image_header *hdr_slot1 = boot_img_hdr(state, 1);
        
        if (boot_is_header_valid(hdr_slot0, BOOT_IMG_AREA(state, 0)) &&
            boot_is_header_valid(hdr_slot1, BOOT_IMG_AREA(state, 1))) {
            int version_cmp = boot_version_cmp(&hdr_slot0->ih_ver, &hdr_slot1->ih_ver);
            if (version_cmp > 0) {
                /* Slot 0 has higher version */
                version_preferred_slot = 0;
                prefer_by_version = true;
            } else if (version_cmp < 0) {
                /* Slot 1 has higher version */
                version_preferred_slot = 1;
                prefer_by_version = true;
            }
        }
        
        /* Choose the final slot: always prefer version-based selection if available */
        /* OTA confirm flag only affects whether to clear the confirm flag after successful validation */
        if (prefer_by_version && version_preferred_slot != NO_ACTIVE_SLOT) {
            slot = version_preferred_slot;
            if (ota_confirm_set) {
                BOOT_LOG_INF("Version comparison: %s slot version is higher, prefer by version (OTA confirm set)", 
                            slot_str(slot));
            } else {
                BOOT_LOG_INF("Version comparison: %s slot version is higher, prefer by version (OTA confirm not set)", 
                            slot_str(slot));
            }
        } else {
            slot = ota_preferred_slot;
            BOOT_LOG_INF("Using OTA preferred slot: %s", slot_str(slot));
        }
    } else {
        /* No slots available - use OTA preferred slot as fallback */
        slot = ota_preferred_slot;
        BOOT_LOG_INF("No available slots detected, using OTA preferred slot: %s", slot_str(slot));
    }

    if (!state->slot_usage[BOOT_CURR_IMG(state)].slot_available[slot]) {
        uint32_t other_slot = slot ^ 1;
        if (state->slot_usage[BOOT_CURR_IMG(state)].slot_available[other_slot]) {
            BOOT_LOG_INF("%s slot unavailable, fallback to %s", slot_str(slot), slot_str(other_slot));
            slot = other_slot;
        } else {
            BOOT_LOG_ERR("No available slots to boot");
            return NO_ACTIVE_SLOT;
        }
    }

    if (!boot_is_header_valid(boot_img_hdr(state, slot), BOOT_IMG_AREA(state, slot))) {
        BOOT_LOG_ERR("%s slot header invalid, abort selecting slot", slot_str(slot));
        return NO_ACTIVE_SLOT;
    }
    
    uint32_t image_ok = boot_read_xip_status(slot, XIP_IMAGE_OK_TYPE);
    BOOT_LOG_INF("xip prefer %s: state=%s(%x)", slot_str(slot), image_ok_str(image_ok), image_ok);

    /* Remove fast path to force all slots to go through validation for security */
    /* No longer return directly based on image_ok status, must go through boot_validate_slot validation */

    if (image_ok == XIP_IMAGE_TEST_FINAL) {
        // TODO use partition api to erase
        // flash_area_erase_fast(get_flash_map_offset(2+slot), get_flash_map_phy_size(2+slot));
#if CONFIG_OTA_CONFIRM_UPDATE
        bk_boot_switch_xip_confirm();
#endif
        candidate_slot = slot ^ 1;
        BOOT_LOG_INF("%s slot test fail, switch to %s", slot_str(slot), slot_str(candidate_slot));
    } else if (image_ok == XIP_IMAGE_UNKNOWN) {
        boot_write_xip_status(slot, XIP_IMAGE_OK_TYPE, XIP_IMAGE_TEST_FIRST);
        candidate_slot = slot;
    } else if (image_ok == XIP_IMAGE_TEST_FIRST) {
        boot_write_xip_status(slot, XIP_IMAGE_OK_TYPE, XIP_IMAGE_TEST_SECOND);
        candidate_slot = slot;
    } else if (image_ok == XIP_IMAGE_TEST_SECOND) {
        boot_write_xip_status(slot, XIP_IMAGE_OK_TYPE, XIP_IMAGE_TEST_FINAL);
        candidate_slot = slot;
    } else if (image_ok == XIP_IMAGE_OK) {
        candidate_slot = slot;
    } else {
        candidate_slot = slot ^ 1;
        BOOT_LOG_INF("%s not valid, switch to %s", slot_str(slot), slot_str(candidate_slot));
    }

    return candidate_slot;
}
#endif

static uint32_t
find_preferred_slot(struct boot_loader_state *state, bool ota_confirm_set)
{
#if CONFIG_ANTI_ROLLBACK
    return find_slot_with_highest_version(state);
#else
    return find_slot_via_version_and_ota_confirm(state, ota_confirm_set);
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
            BOOT_LOG_INF("Image %d %s: Image not found", BOOT_CURR_IMG(state), slot_str(slot));
        }
    }

    state->slot_usage[BOOT_CURR_IMG(state)].active_slot = NO_ACTIVE_SLOT;

    return 0;
}


fih_ret
boot_load_and_validate_images(struct boot_loader_state *state)
{
    uint32_t active_slot;
    uint32_t fallback_slot;
    int rc;
    fih_ret fih_rc;
    bool ota_confirm_set = false;

    ota_confirm_set = bk_boot_check_ota_confirm(1, OTA_CONFIRM);
    if (ota_confirm_set){
        BOOT_LOG_INF("ota confirm=0x%08x", OTA_CONFIRM);
    }

    while (true) {
        /* Go over all the slots and try to load one */
        active_slot = state->slot_usage[BOOT_CURR_IMG(state)].active_slot;
        if (active_slot != NO_ACTIVE_SLOT){
            /* A slot is already active, go to next image. */
            break;
        }

        /* Step 1: Find the slot with highest version */
        active_slot = find_preferred_slot(state, ota_confirm_set);
        if (active_slot == NO_ACTIVE_SLOT) {
            BOOT_LOG_ERR("No slot to load for image %d", BOOT_CURR_IMG(state));
            FIH_RET(FIH_FAILURE);
        }

        /* Determine fallback slot (the other partition) */
        fallback_slot = active_slot ^ 1;

        BOOT_LOG_INF("Found high version slot: %s (version comparison), will validate first", 
                     slot_str(active_slot));

        /* Step 2: Force validation of the high version slot */
        FIH_CALL(boot_validate_slot, fih_rc, state, active_slot, NULL);
        if (FIH_EQ(fih_rc, FIH_SUCCESS)) {
            /* High version slot validation successful */
            state->slot_usage[BOOT_CURR_IMG(state)].active_slot = active_slot;
            BOOT_LOG_INF("High version %s slot validation successful", slot_str(active_slot));
            
            /* Set fast boot flag (if it was secondary slot and image_ok is OK) */
            uint32_t image_ok = boot_read_xip_status(active_slot, XIP_IMAGE_OK_TYPE);
            if (active_slot == BOOT_SECONDARY_SLOT && image_ok == XIP_IMAGE_OK) {
                g_fast_boot_secondary = true;
            }
            break;
        } else {
            /* High version slot validation failed */
            BOOT_LOG_ERR("High version %s slot validation failed, will try fallback %s slot", 
                         slot_str(active_slot), slot_str(fallback_slot));
            
            boot_write_xip_status(active_slot, XIP_IMAGE_OK_TYPE, XIP_IMAGE_TEST_FINAL);
            BOOT_LOG_INF("Set %s slot status to TEST_FINAL due to validation failure", slot_str(active_slot));
            
            /* Critical: Reset execute_enable and flush cache before validating fallback slot */
            /* If Secondary slot validation failed, execute_enable might still be 1, which would cause */
            /* Primary slot hash calculation to read from wrong address */
            if(flash_get_excute_enable() == 1){
                flush_all_dcache();
                flash_set_excute_enable(0);
            }
            
            /* Check if fallback slot is available and has valid header */
            bool fallback_valid = false;
            if (state->slot_usage[BOOT_CURR_IMG(state)].slot_available[fallback_slot] &&
                boot_is_header_valid(boot_img_hdr(state, fallback_slot), 
                                    BOOT_IMG_AREA(state, fallback_slot))) {
                /* Step 3: Validate fallback slot */
                BOOT_LOG_INF("Validating fallback %s slot", slot_str(fallback_slot));
                FIH_CALL(boot_validate_slot, fih_rc, state, fallback_slot, NULL);
                if (FIH_EQ(fih_rc, FIH_SUCCESS)) {
                    /* Fallback slot validation successful */
                    fallback_valid = true;
                    state->slot_usage[BOOT_CURR_IMG(state)].active_slot = fallback_slot;
                    BOOT_LOG_INF("Fallback %s slot validation successful, will boot from fallback", 
                                 slot_str(fallback_slot));
                    break;
                } else {
                    /* Fallback slot validation also failed */
                    BOOT_LOG_ERR("Fallback %s slot validation also failed", slot_str(fallback_slot));
                    state->slot_usage[BOOT_CURR_IMG(state)].slot_available[fallback_slot] = false;
                    state->slot_usage[BOOT_CURR_IMG(state)].slot_state[fallback_slot] += 1;
                }
            } else {
                BOOT_LOG_ERR("Fallback %s slot is not available or header invalid", slot_str(fallback_slot));
            }
            
            /* Both slots validation failed, mark high version slot and continue retry */
            if (!fallback_valid) {
                state->slot_usage[BOOT_CURR_IMG(state)].slot_available[active_slot] = false;
                state->slot_usage[BOOT_CURR_IMG(state)].slot_state[active_slot] += 1;
                state->slot_usage[BOOT_CURR_IMG(state)].active_slot = NO_ACTIVE_SLOT;
                
                BOOT_LOG_ERR("validate %s slot fail, validate_state = %d", slot_str(active_slot),
                             state->slot_usage[BOOT_CURR_IMG(state)].slot_state[active_slot]);
                
                if (state->slot_usage[BOOT_CURR_IMG(state)].slot_state[active_slot] == FAIL_TEST) {
                    BOOT_LOG_ERR("%s slot invalid,offset=%#x,size=%#x", slot_str(active_slot),
                                 get_flash_map_offset(2 + active_slot), get_flash_map_phy_size(2 + active_slot));
#if CONFIG_OTA_CONFIRM_UPDATE
                    bk_boot_switch_xip_confirm();
#endif
                } else if (state->slot_usage[BOOT_CURR_IMG(state)].slot_state[active_slot] > FAIL_TEST) {
                    BOOT_LOG_ERR("all xip images are invalid");
                    FIH_RET(FIH_FAILURE);
                }
                
                /* Continue retry in the loop */
                continue;
            }
        }
    }

    /* Validation successful, clear OTA confirm flag */
#if CONFIG_OTA_CONFIRM_UPDATE
    active_slot = state->slot_usage[BOOT_CURR_IMG(state)].active_slot;
    if (!g_fast_boot_secondary) {
        if (ota_confirm_set) {
            /* OTA confirm flag is set, clear it after successful validation */
            bk_boot_erase_ota_confirm(1);
            BOOT_LOG_INF("OTA confirm flag cleared after successful validation");
        }
    }
#endif

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
