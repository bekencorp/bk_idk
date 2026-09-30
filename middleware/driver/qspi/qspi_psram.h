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

/*
 * Driver-internal QSPI PSRAM helpers.
 * Public API remains in include/driver/qspi_psram.h only.
 */

#include <driver/qspi.h>

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_qspi_psram_reset(qspi_id_t id);
bk_err_t bk_qspi_psram_read_eid(qspi_id_t id, uint8_t *eid, uint32_t size);

#ifdef __cplusplus
}
#endif
