/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include "flash_map/flash_map.h"
#include "target.h"
#include "Driver_Flash.h"

/* When undefined FLASH_DEV_NAME_0 or FLASH_DEVICE_ID_0 , default */
#if !defined(FLASH_DEV_NAME_0) || !defined(FLASH_DEVICE_ID_0)
#define FLASH_DEV_NAME_0  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_0 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_1 or FLASH_DEVICE_ID_1 , default */
#if !defined(FLASH_DEV_NAME_1) || !defined(FLASH_DEVICE_ID_1)
#define FLASH_DEV_NAME_1  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_1 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_2 or FLASH_DEVICE_ID_2 , default */
#if !defined(FLASH_DEV_NAME_2) || !defined(FLASH_DEVICE_ID_2)
#define FLASH_DEV_NAME_2  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_2 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_3 or FLASH_DEVICE_ID_3 , default */
#if !defined(FLASH_DEV_NAME_3) || !defined(FLASH_DEVICE_ID_3)
#define FLASH_DEV_NAME_3  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_3 FLASH_DEVICE_ID
#endif

#if defined(MCUBOOT_SWAP_USING_SCRATCH)
/* When undefined FLASH_DEV_NAME_SCRATCH or FLASH_DEVICE_ID_SCRATCH , default */
#if !defined(FLASH_DEV_NAME_SCRATCH) || !defined(FLASH_DEVICE_ID_SCRATCH)
#define FLASH_DEV_NAME_SCRATCH  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_SCRATCH FLASH_DEVICE_ID
#endif
#endif  /* defined(MCUBOOT_SWAP_USING_SCRATCH) */

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Flash device names must be specified by target */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_0;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_1;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_2;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_3;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_SCRATCH;

struct flash_area flash_map[];

#define CONFIG_PPRO_OFFSET 1024

uint32_t get_flash_map_offset(uint32_t index)
{
    return flash_map[index].fa_off;
}

uint32_t get_flash_map_size(uint32_t index)
{
    return flash_map[index].fa_size;
}

int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

/*piece 4 uint8_t*/
uint32_t piece_address(uint8_t *array,uint32_t index)
{
    return ((uint32_t)(array[index]) << 24 | (uint32_t)(array[index+1])  << 16 | (uint32_t)(array[index+2])  << 8 | (uint32_t)((array[index+3])));
}

void load_partition_from_flash()
{
    uint8_t* buf = (uint8_t*)malloc(40*sizeof(uint8_t));
    if(buf == NULL){
        printf("memory malloc fails.\r\n");
        return ;
    }

    uint32_t i;
    char name[24];

    uint32_t partition_start = CONFIG_PARTITION_PHY_PARTITION_OFFSET + CONFIG_PPRO_OFFSET; // 1024bytes for ppro
    for(i=0;i<PARTITION_AMOUNT;++i){
        bk_flash_read_bytes(partition_start+40*i,buf,40);
        if(is_alpha(buf[0]) == 0){ // name should start A~Z,a~z
            break;
        } 

        int j;
        for(j=0;j<24;++j){
            if(buf[j] == 0xFF){
                break;
            }
            name[j] = buf[j];
        }
        name[j] = '\0';
        if(strcmp("primary_all",name) == 0){
            flash_map[0].fa_off    = piece_address(buf,24);
            flash_map[0].fa_size   = piece_address(buf,28);
#if CONFIG_OTA_OVERWRITE
        }else if(strcmp("ota",name) == 0) {
            flash_map[1].fa_off    = piece_address(buf,24);
            flash_map[1].fa_size   = piece_address(buf,28);
        }
#else
        }else if(strcmp("secondary_all",name) == 0) {
            flash_map[1].fa_off    = piece_address(buf,24);
            flash_map[1].fa_size   = piece_address(buf,28);
        }
#if defined(MCUBOOT_SWAP_USING_SCRATCH)
        else if(strcmp("scratch",name) == 0) {
            flash_map[2].fa_off    = piece_address(buf,24);
            flash_map[2].fa_size   = piece_address(buf,28);
        }
#endif
#endif
    }
    free(buf);
}

struct flash_area flash_map[] = {
#if CONFIG_OTA_OVERWRITE
    {
        .fa_id = FLASH_AREA_PRIMARY_ALL_ID,
        .fa_device_id = FLASH_DEVICE_ID,
        .fa_driver = &FLASH_DEV_NAME,
    },
    {
        .fa_id = FLASH_AREA_SECONDARY_ALL_ID,
        .fa_device_id = FLASH_DEVICE_ID,
        .fa_driver = &FLASH_DEV_NAME,
    },
#else
    {
        .fa_id = FLASH_AREA_PRIMARY_ALL_ID,
        .fa_device_id = FLASH_DEVICE_ID,
        .fa_driver = &FLASH_DEV_NAME,
    },
    {
        .fa_id = FLASH_AREA_SECONDARY_ALL_ID,
        .fa_device_id = FLASH_DEVICE_ID,
        .fa_driver = &FLASH_DEV_NAME,
    },
#endif
#if defined(MCUBOOT_SWAP_USING_SCRATCH)
    {
        .fa_id = FLASH_AREA_SCRATCH_ID,
        .fa_device_id = FLASH_DEVICE_ID_SCRATCH,
        .fa_driver = &FLASH_DEV_NAME_SCRATCH,
    },
#endif
};

const int flash_map_entry_num = ARRAY_SIZE(flash_map);
