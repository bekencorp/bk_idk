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

#define PM_TAG "PM"

#if 1
#define PM_LOGI(...) BK_LOGI(PM_TAG, ##__VA_ARGS__)
#define PM_LOGW(...) BK_LOGW(PM_TAG, ##__VA_ARGS__)
#define PM_LOGE(...) BK_LOGE(PM_TAG, ##__VA_ARGS__)
#define PM_LOGD(...) BK_LOGD(PM_TAG, ##__VA_ARGS__)
#define PM_LOGV(...) BK_LOGV(PM_TAG, ##__VA_ARGS__)
#else
#define PM_LOGI(...) BK_LOGI(NULL, ##__VA_ARGS__)
#define PM_LOGW(...) BK_LOGW(NULL, ##__VA_ARGS__)
#define PM_LOGE(...) BK_LOGE(NULL, ##__VA_ARGS__)
#define PM_LOGD(...) BK_LOGD(NULL, ##__VA_ARGS__)
#define PM_LOGV(...) BK_LOGV(NULL, ##__VA_ARGS__)
#endif

void pm_debug_dump(void);
void pm_core_dump(void);
void pm_power_dump(void);

void pm_power_modules_dump(void);

void pm_debug_lowvol_sleep_hook(void);
void pm_debug_low_vol_wakeup_hook(void);


uint32_t pm_debug_mode();
void pm_debug_ctrl(uint32_t debug_en);

void pm_power_modules_dump_with_sleep_mode(pm_sleep_mode_e sleep_mode);

// TODO: rename or remove it
bk_err_t pm_debug_module_state(void);

void pm_debug_psram(void);
void pm_debug_psram_state(void);

const char *pm_sleep_mode_to_string(pm_sleep_mode_e sleep_mode);