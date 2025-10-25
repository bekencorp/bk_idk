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

#include "flash_layout.h"
#include "idk_config.h"

#define BL2_HEAP_SIZE           (0x0004000)
#define BL2_MSP_STACK_SIZE      (0x0004800)

#define BL2_CODE_START    (0x02000000 + CONFIG_BL2_VIRTUAL_CODE_START)

#define BL2_CODE_SIZE     CONFIG_BL2_VIRTUAL_CODE_SIZE
#define BL2_CODE_LIMIT    (BL2_CODE_START + BL2_CODE_SIZE - 1)

#define SPINLOCK_RESERVED_SIZE 0x40
#define BL2_DATA_START    (0x28000000 + SPINLOCK_RESERVED_SIZE)
#define BL2_DATA_SIZE     KB(64)
#define BL2_IRAM_START    (BL2_DATA_START + BL2_DATA_SIZE + 32)
#define BL2_IRAM_SIZE     KB(16)
#define BL2_FREE_SRAM     (BL2_IRAM_START + BL2_IRAM_SIZE + 32)
#define BL2_DATA_LIMIT    (BL2_DATA_START + BL2_DATA_SIZE - 1)
