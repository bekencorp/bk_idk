// Copyright 2021-2025 Beken
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

#include <common/bk_include.h>

bk_err_t bk_pm_module_power_on(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules, pm_power_module_name_e module);
bk_err_t bk_pm_module_power_off(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules, pm_power_module_name_e module);
void pm_check_power_on_module(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules);
void pm_check_power_off_module(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules);
