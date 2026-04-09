// Copyright 2020-2025 Beken
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

#include <stdint.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_RELIABILITY_CHUNK_BYTES     4096
#define FLASH_RELIABILITY_READ_LEN        4096

#define FLASH_RELIABILITY_MAX_WRITE_ERRORS  (0x0)
#define FLASH_RELIABILITY_MAX_ERASE_ERRORS (0x0)

#define FLASH_RELIABILITY_ERASED_BYTE       0xFF
#define FLASH_RELIABILITY_CRC_INVALID       0xFFFFFFFFU
#define FLASH_RELIABILITY_SECTOR_MASK       0xFFF
#define FLASH_RELIABILITY_SAVE_EVERY_N_ITERS 100

typedef enum {
    FLASH_CLOCK_MODE_40M = 0,
    FLASH_CLOCK_MODE_80M = 1,
    FLASH_CLOCK_MODE_120M = 2,
} flash_clock_mode_t;


void flash_reliability_test_start(void);

#ifdef __cplusplus
}
#endif
