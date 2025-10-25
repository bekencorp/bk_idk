// Copyright 2022-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "partitions_gen.h"

#define FLASH_AREA_IMAGE_SECTOR_SIZE    (0x1000)     /* 4 KB */

#define FLASH_TOTAL_SIZE                (0x00400000)

#define FLASH_BASE_ADDRESS              (0x02000000)

#define FLASH_IMAGE_OVERWRITE             0
#define FLASH_IMAGE_XIP                   1

#define FLASH_AREA_OVERWRITE_ID            (0)
#define FLASH_AREA_OTA_ID                  (1)
#define FLASH_AREA_XIP_A_ID                (2)
#define FLASH_AREA_XIP_B_ID                (3)

//TODO fix me
#define FLASH_MAX_PARTITION_SIZE         CONFIG_OVERWRITE_PHY_PARTITION_SIZE

/* The maximum number of status entries supported by the bootloader. */
#define MCUBOOT_STATUS_MAX_ENTRIES (FLASH_MAX_PARTITION_SIZE / FLASH_AREA_SCRATCH_SIZE)

/* Maximum number of image sectors supported by the bootloader. */
#define MCUBOOT_MAX_IMG_SECTORS    (FLASH_MAX_PARTITION_SIZE / FLASH_AREA_IMAGE_SECTOR_SIZE)

#define FLASH_DEV_NAME Driver_FLASH0
/* Smallest flash programmable unit in bytes */
#define TFM_HAL_FLASH_PROGRAM_UNIT       (0x1)


#define TFM_OTP_NV_COUNTERS_SECTOR_SIZE FLASH_AREA_IMAGE_SECTOR_SIZE

#define CONFIG_BL2_RAM_LOAD_ADDR               0x28040000

#define FLASH_AREA_IMAGE_PRIMARY(x)     (((x) == FLASH_IMAGE_OVERWRITE) ? FLASH_AREA_OVERWRITE_ID : \
					 (((x) == FLASH_IMAGE_XIP) ? FLASH_AREA_XIP_A_ID : 255) \
					)

#define FLASH_AREA_IMAGE_SECONDARY(x)   (((x) == FLASH_IMAGE_OVERWRITE) ? FLASH_AREA_OTA_ID : \
					 (((x) == FLASH_IMAGE_XIP) ? FLASH_AREA_XIP_B_ID : 255) \
					)
