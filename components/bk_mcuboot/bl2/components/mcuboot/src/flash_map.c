/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "flash_map.h"
#include "flash_map_backend/flash_map_backend.h"
#include "bootutil_priv.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/bootutil_public.h"
#include "Driver_Flash.h"
#include "sys_driver.h"
#include "bk_tfm_mpc.h"
#include <components/log.h>
#include "partitions_gen.h"
#include <driver/flash.h>
#include "cache.h"

uint32_t flash_area_read_offset_enable(void);

#define TAG "flash_map"
#define FLASH_PROGRAM_UNIT    TFM_HAL_FLASH_PROGRAM_UNIT

#if CONFIG_OTA_ENCRYPTED
/*
 * OTA slot layout on flash:
 *   [0, ih_hdr_size)              mcuboot header, dbus plaintext
 *   [ih_hdr_size, ih_hdr_size+ih_img_size)  compressed payload, cbus decrypt
 *   [ih_hdr_size+ih_img_size, end)          TLV trailer, dbus plaintext
 */
static int flash_area_read_ota_encrypted(const struct flash_area *area, uint32_t off,
                                         void *dst, uint32_t len)
{
    uint32_t phy_base = area->fa_off;
    uint32_t vir_base = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_base));
    uint32_t hdr_size = 0U;
    uint32_t tlv_off = 0U;
    uint8_t *out = dst;

    while (len > 0U) {
        uint32_t boundary;
        int encrypted;

        if (hdr_size == 0U) {
            struct image_header hdr;

            bk_flash_read_bytes(area->fa_off, &hdr, sizeof(hdr));
            if (hdr.ih_magic != IMAGE_MAGIC) {
                return -1;
            }
            hdr_size = hdr.ih_hdr_size;
            tlv_off = hdr.ih_hdr_size + hdr.ih_img_size;
        }

        if (off < hdr_size) {
            /* plaintext header */
            encrypted = 0;
            boundary = hdr_size;
        } else if (off >= tlv_off) {
            /* plaintext TLV trailer */
            encrypted = 0;
            boundary = off + len;
        } else {
            /* encrypted payload */
            encrypted = 1;
            boundary = tlv_off;
        }

        uint32_t chunk = boundary - off;
        if (chunk > len) {
            chunk = len;
        }

        if (encrypted) {
#if CONFIG_DBUS_CHECK_CRC
            if (check_crc(vir_base + off, chunk) == false) {
                BOOT_LOG_ERR("addr = %#x,len=%#x,crc16 fail", vir_base + off, chunk);
                bk_boot_write_ota_fail(BOOT_FAIL_STAGE_CBUS_READ);
                return -1;
            }
#endif
            bk_flash_read_cbus(vir_base + off, out, chunk);
        } else {
            bk_flash_read_bytes(phy_base + off, out, chunk);
        }

        off += chunk;
        out += chunk;
        len -= chunk;
    }

    return 0;
}
#endif

extern const struct flash_area flash_map[];
extern const int flash_map_entry_num;

/*
 * `open` a flash area.  The `area` in this case is not the individual
 * sectors, but describes the particular flash area in question.
 */
int flash_area_open(uint8_t id, const struct flash_area **area)
{
    int i;

    for (i = 0; i < flash_map_entry_num; i++) {
        if (id == flash_map[i].fa_id) {
            break;
        }
    }
    if (i == flash_map_entry_num) {
        return -1;
    }

    *area = &flash_map[i];
    return 0;
}

void flash_area_close(const struct flash_area *area)
{
    /* Nothing to do. */
}

/*
 * Read/write/erase. Offset is relative from beginning of flash area.
 * `off` and `len` can be any alignment.
 * Return 0 on success, other value on failure.
 */
int flash_area_read_dbus(const struct flash_area *area, uint32_t off, void *dst,
                    uint32_t len)
{
    SYS_LOCK_DECLARATION();
    SYS_LOCK();

    bk_flash_read_bytes(area->fa_off + off, dst, len);

    SYS_UNLOCK();
    return 0;
}

int flash_area_read(const struct flash_area *area, uint32_t off, void *dst,
                    uint32_t len)
{
    uint32_t fa_off = area->fa_off;
#if CONFIG_OTA_OVERWRITE || CONFIG_MIXED_OTA
    if(area->fa_id == 0){
        fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(fa_off));
#if CONFIG_DBUS_CHECK_CRC
        if (check_crc(fa_off + off,len) == false) {
            BOOT_LOG_ERR("addr = %#x,len=%#x,crc16 fail", fa_off + off, len);
            bk_boot_write_ota_fail(BOOT_FAIL_STAGE_CBUS_READ);
            return -1;
        }
#endif
        bk_flash_read_cbus(fa_off + off,dst,len);
        return 0;
    } else if(area->fa_id == 1){
#if CONFIG_OTA_ENCRYPTED
        return flash_area_read_ota_encrypted(area, off, dst, len);
#else
        bk_flash_read_bytes(fa_off + off, dst, len);
#endif
        return 0;
    }
#endif
#if CONFIG_DIRECT_XIP
    fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(area->fa_off));
    uint32_t fa_size = (FLASH_PHY2VIRTUAL(partition_get_phy_size(PARTITION_XIP_A)));
    if(off > fa_size){
        BK_LOGE(TAG, "cbus read offset 0x%x error,size=%#x\r\n",off,fa_size);
        return -1;
    }
#if CONFIG_DBUS_CHECK_CRC
    if (check_crc(fa_off + off,len) == false) {
        BOOT_LOG_ERR("area %d addr = %#x,len=%#x,crc16 fail", area->fa_id, fa_off + off, len);
        return -1;
    }
#endif
    if(area->fa_id == 2){
        bk_flash_read_cbus_xip(0, fa_off + off, dst, len);
        return 0;
    } else if(area->fa_id == 3){
        fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_get_phy_offset(PARTITION_XIP_A)));
        bk_flash_read_cbus_xip(1, fa_off + off, dst, len);
        return 0;
    }
#endif
}

#if CONFIG_DIRECT_XIP
uint32_t flash_write_bytes_dbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
    SYS_LOCK_DECLARATION();
    SYS_LOCK();

    flash_protect_type_t protect_type = bk_flash_get_protect_type();
    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    bk_flash_write_bytes(address, user_buf, size);
    bk_flash_set_protect_type(protect_type);
    SYS_UNLOCK();
    return 0;
}
#endif

uint32_t flash_area_update_dbus(const struct flash_area *area, uint32_t off,
                                const void *src, uint32_t len)
{
    SYS_LOCK_DECLARATION();
    SYS_LOCK();
#if CONFIG_DIRECT_XIP
    uint32_t update_id = (flash_area_read_offset_enable() ^ 1);
    uint32_t fa_off = CEIL_ALIGN_34((partition_get_phy_offset(update_id)));
    uint32_t phy_addr = off + fa_off;
    bk_flash_write_bytes(phy_addr, src, len);
#endif
#if CONFIG_OTA_OVERWRITE
    bk_flash_write_bytes(area->fa_off + off, src, len);
#endif
    SYS_UNLOCK();
    return 0;
}

/* Writes `len` bytes of flash memory at `off` from the buffer at `src`.
 * `off` and `len` can be any alignment.
 */
int flash_area_write(const struct flash_area *area, uint32_t off,
                     const void *src, uint32_t len)
{
#if CONFIG_DIRECT_XIP
    SYS_LOCK_DECLARATION();
    SYS_LOCK();

    uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_get_phy_offset(PARTITION_XIP_A)));
    uint32_t boundary = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_get_phy_offset(PARTITION_NSPE)));
    uint32_t block_num = (FLASH_PHY2VIRTUAL(partition_get_phy_size(PARTITION_NSPE))) / (64*1024);

    bk_mpc_set_secure_attribute(MPC_DEV_FLASH,boundary,block_num,0); // turn NSPE to S,and resume
    enable_dcache(0);
    (void)area;
    (void)fa_off;
    (void)off;
    (void)src;
    (void)len;
    enable_dcache(1);
    bk_mpc_set_secure_attribute(MPC_DEV_FLASH,boundary,block_num,1);

	SYS_UNLOCK();

    return 0;
#endif
    return 0;
}

int flash_area_erase(const struct flash_area *area, uint32_t off, uint32_t len)
{
    int flash_area_erase_fast(uint32_t off, uint32_t len);
    return flash_area_erase_fast(area->fa_off + off, len);
}
