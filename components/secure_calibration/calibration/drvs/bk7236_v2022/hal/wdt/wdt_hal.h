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

#include "wdt_ll.h"
#include "hal_wdt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	wdt_hw_t *hw;
	wdt_unit_t id;
} wdt_hal_t;

#define wdt_hal_reset_config_to_default(hal) wdt_ll_reset_config_to_default((hal)->hw)

bk_err_t wdt_hal_init(wdt_hal_t *hal);
bk_err_t wdt_hal_init_wdt(wdt_hal_t *hal, uint32_t timeout);

#if CFG_HAL_DEBUG_WDT
void wdt_struct_dump(void);
#else
#define wdt_struct_dump()
#endif

#ifdef __cplusplus
}
#endif

