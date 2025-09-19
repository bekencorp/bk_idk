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

pm_wakeup_source_e bk_pm_wakeup_source_get(void);
pm_wakeup_source_e bk_pm_deep_sleep_wakeup_source_get();
pm_wakeup_source_e bk_pm_exit_low_vol_wakeup_source_get();

void pm_touched_wakeup_low_voltage();
void pm_rtc_wakeup_deep_sleep();
void pm_touched_wakeup_deep_sleep();

bk_err_t pm_wakeup_from_deepsleep_handle();
void pm_deep_sleep_wakeup_source_set();