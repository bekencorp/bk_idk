// Copyright 2020-2021 Beken
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

#include "soc.h"
#include "hal_flash_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_FLASH_NOT_INIT             (BK_ERR_FLASH_BASE - 1) /**< FLASH driver not init */
#define BK_ERR_FLASH_ADDR_OUT_OF_RANGE    (BK_ERR_FLASH_BASE - 2) /**< FLASH address is out of range */
#define BK_ERR_FLASH_PARTITION_NOT_FOUND  (BK_ERR_FLASH_BASE - 3) /**< FLASH partition not found */

typedef enum
{
	FLASH_LINE_MODE_TWO = 2,  /**< flash line mode 2 */
	FLASH_LINE_MODE_FOUR = 4, /**< flash line mode 4 */
} flash_line_mode_t;

typedef enum
{
	FLASH_PROTECT_NONE = 0,     /**< flash protect type none */
	FLASH_PROTECT_ALL,          /**< flash protect type all */
	FLASH_PROTECT_HALF,         /**< flash protect type half */
	FLASH_UNPROTECT_LAST_BLOCK, /**< flash protect type unprotect last block */
} flash_protect_type_t;

#ifdef __cplusplus
}
#endif

