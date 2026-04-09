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

/* System includes */
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "sys_ll.h"
#include "modules/pm.h"
#include "flash_test.h"
#include "wifi_test.h"
#include "statis.h"
#include "cli_statis.h"
#include "psram_test.h"

static void user_app_vote_cpu_freq(void)
{
	switch (CONFIG_RELIABILITY_CPU_FREQ_MHZ) {
	case 120:
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_120M);
		break;
	case 160:
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_160M);
		break;
	case 240:
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_240M);
		break;
	default:
		/* Fallback to 120M to keep the behavior deterministic. */
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_120M);
		break;
	}
	BK_RAW_LOGI("APP_MAIN", "Current CPU freq: %d MHz\r\n", CONFIG_RELIABILITY_CPU_FREQ_MHZ);
}

void user_app_main(void)
{
	user_app_vote_cpu_freq();

	statis_load_from_flash();

	int current_temp_code = get_current_temperature_code();
	BK_RAW_LOGI("APP_MAIN", "Current temperature code: %d\r\n", current_temp_code);

#if CONFIG_CLI && (CONFIG_WIFI_RELIABILITY_TEST || CONFIG_FLASH_RELIABILITY_TEST)
	statis_cli_register_cmds();
#endif

#if CONFIG_FLASH_RELIABILITY_TEST
	flash_reliability_test_start();
#endif

#if CONFIG_PSRAM_RELIABILITY_TEST
	psram_reliability_test_start();
#endif

#if CONFIG_WIFI_RELIABILITY_TEST
	wifi_reliability_start();
#endif
}

int main(void)
{
	bk_init();
	user_app_main();
	return 0;
}