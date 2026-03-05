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
#include "ow_loader.h"

#if (BOOT_IMAGE_NUMBER > 1)
#define IMAGES_ITER(x) for ((x) = 0; (x) < BOOT_IMAGE_NUMBER; ++(x))
#else
#define IMAGES_ITER(x)
#endif

static int boot_copy_image(struct boot_loader_state *state, struct boot_status *bs)
{
    int rc;
    const struct flash_area *fap_primary_slot;
    const struct flash_area *fap_secondary_slot;
    uint8_t image_index;

    (void)bs;

    image_index = BOOT_CURR_IMG(state);

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(image_index),
            &fap_primary_slot);
    assert (rc == 0);

    rc = flash_area_open(FLASH_AREA_IMAGE_SECONDARY(image_index),
            &fap_secondary_slot);
    assert (rc == 0);

    BOOT_LOG_INF("Image %d copying the secondary slot to the primary slots", image_index);
    rc = boot_copy_region(state, fap_secondary_slot, fap_primary_slot, 0, 0);
    if (rc != 0) {
        BOOT_LOG_ERR("Overwrite copy fail.");
        return rc;
    }

#if CONFIG_ANTI_ROLLBACK
    /* Update the stored security counter with the new image's security counter
     * value. Both slots hold the new image at this point, but the secondary
     * slot's image header must be passed since the image headers in the
     * boot_data structure have not been updated yet.
     */
    rc = boot_update_security_counter(BOOT_CURR_IMG(state), BOOT_PRIMARY_SLOT,
                                boot_img_hdr(state, BOOT_SECONDARY_SLOT));
    if (rc != 0) {
        BOOT_LOG_ERR("Security counter update failed after image upgrade.");
        return rc;
    }
#endif /* CONFIG_ANTI_ROLLBACK */

    flash_area_close(fap_primary_slot);
    flash_area_close(fap_secondary_slot);

    return 0;
}

static int boot_perform_update(struct boot_loader_state *state, struct boot_status *bs)
{
    int rc;
    rc = boot_copy_image(state, bs);
    return rc;
}

static int
boot_validated_swap_type(struct boot_loader_state *state,
                         struct boot_status *bs)
{
    int swap_type;
    FIH_DECLARE(fih_rc, FIH_FAILURE);

#if CONFIG_OTA_CONFIRM_UPDATE
    if ((bk_boot_check_ota_confirm(0, OTA_CONFIRM))) {
        BOOT_LOG_INF("confirm set");
        swap_type = BOOT_SWAP_TYPE_TEST;
    } else {
        BOOT_LOG_INF("confirm unset");
        swap_type = BOOT_SWAP_TYPE_NONE;
    }
#else
    swap_type = boot_swap_type_multi(BOOT_CURR_IMG(state));
#endif
    if (BOOT_IS_UPGRADE(swap_type)) {
        /* Boot loader wants to switch to the secondary slot.
         * Ensure image is valid.
         */
        FIH_CALL(boot_validate_slot, fih_rc, state, BOOT_SECONDARY_SLOT, bs);
        if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
            BOOT_LOG_INF("validate ota slot error");
            if (FIH_EQ(fih_rc, FIH_NO_BOOTABLE_IMAGE)) {
                swap_type = BOOT_SWAP_TYPE_NONE;
            } else {
                swap_type = BOOT_SWAP_TYPE_FAIL;
            }
        }
    }
    BOOT_LOG_INF("Swap type: %s", swap_type == BOOT_SWAP_TYPE_TEST   ? "test"   :
                                  swap_type == BOOT_SWAP_TYPE_NONE   ? "none"   :
                                  "BUG; can't happen");
    return swap_type;
}

static void
boot_prepare_image_for_update(struct boot_loader_state *state,
                              struct boot_status *bs)
{
    int rc;
    struct image_header *hdr;
    FIH_DECLARE(fih_rc, FIH_FAILURE);

    /* Attempt to read an image header from each slot. */
    rc = boot_read_image_headers(state, false, NULL);
    if (rc != 0) {
        /* Continue with next image if there is one. */
        BOOT_LOG_WRN("Failed reading image headers; Image=%u",
                BOOT_CURR_IMG(state));
        BOOT_SWAP_TYPE(state) = BOOT_SWAP_TYPE_NONE;
        return;
    }

    hdr = boot_img_hdr(state, 0);
    if (boot_is_header_valid(hdr, BOOT_IMG_AREA(state, 0))) {
        BOOT_LOG_IMAGE_INFO(0, hdr);
    }

    BOOT_SWAP_TYPE(state) = boot_validated_swap_type(state, bs);
}

/*TODO: support image > 1 overwrite upgrade*/
/*image > 1, confirm and test*/
fih_ret
overwrite_boot_go(struct boot_loader_state *state, struct boot_rsp *rsp)
{
    size_t slot;
    struct boot_status bs;
    int rc = -1;
    FIH_DECLARE(fih_rc, FIH_FAILURE);
    int fa_id;
    int image_index;
    bool has_upgrade;
    volatile int fih_cnt;

    /* The array of slot sectors are defined here (as opposed to file scope) so
     * that they don't get allocated for non-boot-loader apps.  This is
     * necessary because the gcc option "-fdata-sections" doesn't seem to have
     * any effect in older gcc versions (e.g., 4.8.4).
     */

    has_upgrade = false;

    /* Iterate over all the images. By the end of the loop the swap type has
     * to be determined for each image and all aborted swaps have to be
     * completed.
     */
    IMAGES_ITER(BOOT_CURR_IMG(state)) {
        /* Open primary and secondary image areas for the duration
         * of this call.
         */
        for (slot = 0; slot < BOOT_NUM_SLOTS; slot++) {
            fa_id = flash_area_id_from_multi_image_slot(image_index, slot);
            rc = flash_area_open(fa_id, &BOOT_IMG_AREA(state, slot));
            assert(rc == 0);

            if (rc != 0) {
                BOOT_LOG_ERR("Failed to open flash area ID %d (image %d slot %d): %d, "
                             "cannot continue", fa_id, image_index, (int8_t)slot, rc);
                FIH_PANIC;
            }
        }

        /* Determine swap type and complete swap if it has been aborted. */
        boot_prepare_image_for_update(state, &bs);

        if (BOOT_IS_UPGRADE(BOOT_SWAP_TYPE(state))) {
            has_upgrade = true;
        }
    }

    /* Trigger status change callback with upgrading status */
    mcuboot_status_change(MCUBOOT_STATUS_UPGRADING);

    /* Iterate over all the images. At this point there are no aborted swaps
     * and the swap types are determined for each image. By the end of the loop
     * all required update operations will have been finished.
     */
    IMAGES_ITER(BOOT_CURR_IMG(state)) {
        /* Set the previously determined swap type */
        bs.swap_type = BOOT_SWAP_TYPE(state);

        switch (BOOT_SWAP_TYPE(state)) {
        case BOOT_SWAP_TYPE_NONE:
            break;

        case BOOT_SWAP_TYPE_TEST:
            /* fallthrough */
        case BOOT_SWAP_TYPE_PERM:
            /* fallthrough */
        case BOOT_SWAP_TYPE_REVERT:
            rc = BOOT_HOOK_CALL(boot_perform_update_hook, BOOT_HOOK_REGULAR,
                                BOOT_CURR_IMG(state), &(BOOT_IMG(state, 1).hdr),
                                BOOT_IMG_AREA(state, BOOT_SECONDARY_SLOT));
            if (rc == BOOT_HOOK_REGULAR)
            {
                rc = boot_perform_update(state, &bs);
            }
            assert(rc == 0);
            break;

        case BOOT_SWAP_TYPE_FAIL:
            break;

        default:
            BOOT_SWAP_TYPE(state) = BOOT_SWAP_TYPE_PANIC;
        }

        if (BOOT_SWAP_TYPE(state) == BOOT_SWAP_TYPE_PANIC) {
            BOOT_LOG_ERR("swap type panic!");
            /* Loop forever... */
            FIH_PANIC;
        }
    }

    /* Iterate over all the images. At this point all required update operations
     * have finished. By the end of the loop each image in the primary slot will
     * have been re-validated.
     */
    FIH_SET(fih_cnt, 0);
    IMAGES_ITER(BOOT_CURR_IMG(state)) {
        if (BOOT_SWAP_TYPE(state) != BOOT_SWAP_TYPE_NONE) {
            /* Attempt to read an image header from each slot. Ensure that image
             * headers in slots are aligned with headers in boot_data.
	     * Note: Quite complicated internal logic of boot_read_image_headers
	     * uses boot state, the last parm, to figure out in which slot which
	     * header is located; when boot state is not provided, then it
	     * is assumed that headers are at proper slots (we are not in
	     * the middle of moving images, etc).
             */
            rc = boot_read_image_headers(state, false, NULL);
            if (rc != 0) {
                FIH_SET(fih_rc, FIH_FAILURE);
                goto out;
            }
            /* Since headers were reloaded, it can be assumed we just performed
             * a swap or overwrite. Now the header info that should be used to
             * provide the data for the bootstrap, which previously was at
             * secondary slot, was updated to primary slot.
             */
        }

        if (bk_skip_validate() == false || BOOT_SWAP_TYPE(state) != BOOT_SWAP_TYPE_NONE) {
            FIH_CALL(boot_validate_slot, fih_rc, state, BOOT_PRIMARY_SLOT, NULL);
        /* Check for all possible values is redundant in normal operation it
         * is meant to prevent FI attack.
         */
            if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS) ||
                FIH_EQ(fih_rc, FIH_FAILURE) ||
                FIH_EQ(fih_rc, FIH_NO_BOOTABLE_IMAGE)) {
                FIH_SET(fih_rc, FIH_FAILURE);
                BOOT_LOG_ERR("Validate new image fail");
                goto out;
            } else {
                BOOT_LOG_INF("Overwrite update success");
            }
        }

        rc = boot_update_hw_rollback_protection(state);
        if (rc != 0) {
            FIH_SET(fih_rc, FIH_FAILURE);
            goto out;
        }

        rc = boot_add_shared_data(state, BOOT_PRIMARY_SLOT);
        if (rc != 0) {
            FIH_SET(fih_rc, FIH_FAILURE);
            goto out;
        }
        ++fih_cnt;
    }
    /*
     * fih_cnt should be equal to BOOT_IMAGE_NUMBER now.
     * If this is not the case, at least one iteration of the loop
     * has been skipped.
     */
    if(FIH_NOT_EQ(fih_cnt, BOOT_IMAGE_NUMBER)) {
        FIH_PANIC;
    }

    (void*)rsp;
    BOOT_LOG_FORCE("Jump APP\r\n");
    fih_rc = FIH_SUCCESS;
out:
    /*
     * Since the boot_status struct stores plaintext encryption keys, reset
     * them here to avoid the possibility of jumping into an image that could
     * easily recover them.
     */
    memset(&bs, 0, sizeof(struct boot_status));

    FIH_RET(fih_rc);
}