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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!


#pragma once
#include <soc/soc.h>
#include "hal_port.h"
#include "system_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_otp_read_nsc(uint32_t item);
bk_err_t bk_otp_update_nsc(uint8_t map_id, uint32_t item, uint8_t* buf, uint32_t size);
bk_err_t bk_otp_read_permission_nsc(uint8_t map_id, uint32_t item, uint32_t* permission);
bk_err_t bk_otp_write_permission_nsc(uint8_t map_id, uint32_t item, uint32_t permission);

#ifdef __cplusplus
}
#endif