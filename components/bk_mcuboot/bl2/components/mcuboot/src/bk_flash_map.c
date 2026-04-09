/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include "flash_map.h"

#include "Driver_Flash.h"
#include "flash_partition.h"
#include "partitions_gen.h"

/* When undefined FLASH_DEV_NAME_0 or FLASH_DEVICE_ID_0 , default */
#if !defined(FLASH_DEV_NAME_0) || !defined(FLASH_DEVICE_ID_0)
#define FLASH_DEV_NAME_0  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_0 FLASH_DEVICE_ID
#endif

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Flash device names must be specified by target */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_0;

struct flash_area flash_map[];

uint32_t get_flash_map_offset(uint32_t index)
{
	return flash_map[index].fa_off;
}

uint32_t get_flash_map_size(uint32_t index)
{
	return flash_map[index].fa_size;
}

uint32_t get_flash_map_phy_size(uint32_t index)
{
	return flash_map[index].fa_phy_size;
}

int flash_map_init(void)
{
	uint32_t size;

#if CONFIG_OTA_OVERWRITE
	uint32_t slot_id = FLASH_AREA_OVERWRITE_ID;
	size = partition_get_phy_size(PARTITION_OVERWRITE);
	flash_map[slot_id].fa_phy_size = size;
	flash_map[slot_id].fa_off = partition_get_phy_offset(PARTITION_OVERWRITE);
	flash_map[slot_id].fa_size = FLASH_PHY2VIRTUAL(size);
	flash_map[slot_id].fa_size &= ~(0xFFF);

	slot_id = FLASH_AREA_OTA_ID;
	flash_map[slot_id].fa_off = partition_get_phy_offset(PARTITION_OTA);
	flash_map[slot_id].fa_size = partition_get_phy_size(PARTITION_OTA);
	flash_map[slot_id].fa_phy_size = partition_get_phy_size(PARTITION_OTA);
#endif

#if CONFIG_DIRECT_XIP
	size = partition_get_phy_size(PARTITION_XIP_A);
	uint32_t slot_a = FLASH_AREA_XIP_A_ID;
	uint32_t slot_b = FLASH_AREA_XIP_B_ID;
	flash_map[slot_a].fa_phy_size = size;
	flash_map[slot_a].fa_off = partition_get_phy_offset(PARTITION_XIP_A);
	flash_map[slot_a].fa_size = FLASH_PHY2VIRTUAL(size);
	flash_map[slot_a].fa_size &= ~(0xFFF);

	size = partition_get_phy_size(PARTITION_XIP_B);
	flash_map[slot_b].fa_off = partition_get_phy_offset(PARTITION_XIP_B);
	flash_map[slot_b].fa_phy_size = size;
	flash_map[slot_b].fa_size = FLASH_PHY2VIRTUAL(size);
	flash_map[slot_b].fa_size &= ~(0xFFF);
	uint32_t primary_vir_start = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(flash_map[slot_a].fa_off));
	uint32_t secondary_vir_start = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(flash_map[slot_b].fa_off));
	flash_set_xip_offset(primary_vir_start, secondary_vir_start, flash_map[slot_a].fa_phy_size);
#endif
	return 0;
}

struct flash_area flash_map[] = {
	{
		.fa_id = FLASH_AREA_OVERWRITE_ID,
		.fa_device_id = FLASH_DEVICE_ID,
	},
	{
		.fa_id = FLASH_AREA_OTA_ID,
		.fa_device_id = FLASH_DEVICE_ID,
	},
	{
		.fa_id = FLASH_AREA_XIP_A_ID,
		.fa_device_id = FLASH_DEVICE_ID,
	},
	{
		.fa_id = FLASH_AREA_XIP_B_ID,
		.fa_device_id = FLASH_DEVICE_ID,
	}
};

const int flash_map_entry_num = ARRAY_SIZE(flash_map);
