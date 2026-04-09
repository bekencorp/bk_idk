/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>

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

#define TAG "flash_map"
#define FLASH_PROGRAM_UNIT    TFM_HAL_FLASH_PROGRAM_UNIT

// Convert physical address to virtual address based on CRC mode
static inline uint32_t phy_to_virtual_addr(uint32_t phy_addr)
{
    return FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_addr));
}

static inline uint32_t virtual_to_physical_offset(uint32_t virtual_off)
{
    return FLASH_VIRTUAL2PHY(virtual_off);
}

/**
 * Return the greatest value not greater than `value` that is aligned to
 * `alignment`.
 */
#define FLOOR_ALIGN(value, alignment) ((value) & ~((alignment) - 1))

/**
 * Return the least value not less than `value` that is aligned to `alignment`.
 */
#define CEILING_ALIGN(value, alignment) \
                         (((value) + ((alignment) - 1)) & ~((alignment) - 1))

extern const struct flash_area flash_map[];
extern const int flash_map_entry_num;

/* Valid entries for data item width */
static const uint32_t data_width_byte[] = {
    sizeof(uint8_t),
    sizeof(uint16_t),
    sizeof(uint32_t),
};

/*
 * Check the target address in the flash_area_xxx operation.
 */
static bool is_range_valid(const struct flash_area *area,
                           uint32_t off,
                           uint32_t len)
{
    uint32_t size;

    if (!area) {
        return false;
    }

    if (!boot_u32_safe_add(&size, off, len)) {
        return false;
    }

    if (area->fa_size < size) {
        return false;
    }

    return true;
}

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
#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
            bk_boot_write_ota_fail(BOOT_FAIL_STAGE_CBUS_READ);
#endif
            return -1;
        }
#endif
        bk_flash_read_cbus(fa_off + off,dst,len);
        return 0;
    } else if(area->fa_id == 1){
        bk_flash_read_bytes(fa_off + off,dst,len);
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

    uint32_t protect_type = bk_flash_get_protect_type();
    bk_flash_set_protect_type(0);
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
    uint32_t write_addr;
    uint32_t sec_off,nsec_off;

    bk_mpc_set_secure_attribute(MPC_DEV_FLASH,boundary,block_num,0); // turn NSPE to S,and resume
    enable_dcache(0);
    write_addr = (fa_off+off);
    write_addr |= 1 << 24;
    // bk_flash_write_cbus(write_addr,src,len);
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
