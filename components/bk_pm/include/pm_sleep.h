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

void pm_cpu_wfi_process();
void pm_normal_sleep_process();
uint32_t pm_low_voltage_process();

void pm_deep_sleep_prepare();
void pm_deep_sleep_process();
#if CONFIG_PM_SUPER_DEEP_SLEEP
void pm_super_deep_sleep_prepare();
void pm_super_deep_sleep_process();
#endif

bk_err_t pm_lv_enter_time_out_clear();