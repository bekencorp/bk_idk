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
#include <components/sensor.h>
#include <os/os.h>
#include "sys_ll.h"
#include "modules/pm.h"
#include "flash_test.h"
#include "wifi_test.h"
#include "statis.h"
#include "cli_statis.h"
#include "flash_import/cli_flash_import.h"
#include "flash_import/flash_import_internal.h"
#include "psram_test.h"
#include "htol_test.h"

static void user_app_vote_cpu_freq(void)
{
	switch (CONFIG_RELIABILITY_CPU_FREQ_MHZ) {
	case 120:
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_120M);
		break;
	case 160:
#if CONFIG_RELIABILITY_VOTE_PM_320_FOR_CPU_FREQ_160
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_320M);
#else
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_160M);
#endif
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

static int user_app_get_temperature_code(void)
{
	float temperature = 0;
	int ret = BK_OK;
	int retry = 0;

	for (retry = 0; retry < 40; retry++) {
		ret = bk_sensor_get_current_temperature(&temperature);
		if (ret == BK_OK) {
			return (int)temperature;
		}

		if (ret == BK_ERR_NOT_INIT) {
			(void)bk_sensor_init();
			(void)bk_sensor_start();
		} else if (ret != BK_ERR_TRY_AGAIN) {
			break;
		}

		rtos_delay_milliseconds(50);
	}

	BK_RAW_LOGI("APP_MAIN", "temp read fail:%d\r\n", ret);
	return 0;
}

void user_app_main(void)
{
	user_app_vote_cpu_freq();

#if CONFIG_WIFI_RELIABILITY_TEST || CONFIG_FLASH_RELIABILITY_TEST
	statis_load_from_flash();
#endif

	int current_temp_code = user_app_get_temperature_code();
	BK_RAW_LOGI("APP_MAIN", "Current temperature code: %d\r\n", current_temp_code);

#if CONFIG_CLI && (CONFIG_WIFI_RELIABILITY_TEST || CONFIG_FLASH_RELIABILITY_TEST)
	statis_cli_register_cmds();
#endif

#if CONFIG_FLASH_IMPORT_SLEEP_TEST
	flash_import_case_sleep_resume();
#endif

#if CONFIG_CLI && CONFIG_FLASH_IMPORT_TEST
	flash_import_cli_register_cmds();
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

#if CONFIG_HTOL_RELIABILITY_TEST
	htol_reliability_test_start();
#endif
}

int main(void)
{
	bk_init();
	user_app_main();
	return 0;
}