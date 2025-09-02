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

#include <common/bk_include.h>
#include "_otp.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    uint32_t flash_id;
    uint8_t efstoflash_otp_load;
    uint8_t efstoflash_otp_shaddr;
    uint8_t flash_otp_section_id;
    uint32_t flash_otp_software_offset;
    uint32_t flash_otp_operate_addr;
} flash_otp_config_t;

void bk_flash_otp_init(uint8_t section_id);

bk_err_t bk_flash_otp_update(uint32_t item_id, uint8_t* buf, size_t size);

bk_err_t bk_flash_otp_read(uint32_t item_id, uint8_t* buf, size_t size);

bk_err_t bk_flash_otp_erase(uint32_t item_id, size_t size);

#ifdef __cplusplus
}
#endif