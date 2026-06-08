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

#include <driver/flash.h>

#if CONFIG_FLASH_CBUS
#include "security.h"
#endif

flash_protect_type_t flash_import_get_protect(void);
flash_line_mode_t flash_import_set_line_mode(flash_line_mode_t line_mode);
void flash_import_set_protect(flash_protect_type_t protect);
bk_err_t flash_import_read_bytes(uint8_t *buffer, uint32_t address, uint32_t len);
bk_err_t flash_import_write_bytes(const uint8_t *buffer, uint32_t address, uint32_t len);
bk_err_t flash_import_erase_4k(uint32_t address);
#if CONFIG_FLASH_CBUS
bk_err_t flash_import_write_cbus(const uint8_t *buffer, uint32_t address, uint32_t len);
bk_err_t flash_import_read_cbus(uint8_t *buffer, uint32_t address, uint32_t len);
#endif

void flash_import_case_endurance_start(void);
void flash_import_case_endurance_stop(void);
void flash_import_case_endurance_show(void);
void flash_import_case_zero_read_start(void);
void flash_import_case_zero_read_stop(void);
void flash_import_case_zero_read_show(void);
