// Copyright 2010-2020 Espressif Systems (Shanghai) PTE LTD
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

extern "C" uint8_t esp_rom_crc8_le(uint8_t crc, uint8_t const *buf, uint32_t len);
extern "C" uint16_t esp_rom_crc16_le(uint16_t crc, uint8_t const *buf, uint32_t len);
extern "C" uint32_t esp_rom_crc32_le(uint32_t crc, uint8_t const *buf, uint32_t len);

