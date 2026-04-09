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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * IEEE CRC-32 (Ethernet polynomial). For the first call pass crc as 0.
 *
 * @param crc previous CRC value (0 to start)
 * @param buf input buffer
 * @param size byte length
 * @return updated CRC-32
 */
uint32_t crc32_calc(uint32_t crc, const void *buf, size_t size);

#ifdef __cplusplus
}
#endif
