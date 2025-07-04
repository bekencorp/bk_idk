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

#include <common/bk_include.h>
#include <driver/pwr_clk.h>
#if CONFIG_MAILBOX
#include <driver/mailbox_channel.h>
#endif
#include <modules/pm.h>
#include "sys_driver.h"
#if CONFIG_PSRAM
#include <driver/psram.h>
#endif
#include <os/mem.h>
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <driver/psram.h>
#include <components/system.h>
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"

/*=====================DEFINE  SECTION  START=====================*/
#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms
#define PM_BOOT_CP1_WAITING_TIEM             (500) // 0.5s
#define PM_CP1_RECOVERY_DEFAULT_VALUE        (0xFFFFFFFFFFFFFFFF)
#define PM_OPEN_CP1_TIMEOUT                  (20000) //20s
#define PM_SEMA_WAIT_FOREVER                 (0xFFFFFFFF)    /*Wait Forever*/

#define PM_BOOT_CP1_TRY_COUNT                (3)

#define PWR_TAG "PWR"
#define PWR_LOGI(...) BK_LOGI(SOC_TAG, ##__VA_ARGS__)
#define PWR_LOGW(...) BK_LOGW(SOC_TAG, ##__VA_ARGS__)
#define PWR_LOGE(...) BK_LOGE(SOC_TAG, ##__VA_ARGS__)
#define PWR_LOGD(...) BK_LOGD(SOC_TAG, ##__VA_ARGS__)

/*=====================DEFINE  SECTION  END=====================*/

/*=====================VARIABLE  SECTION  START=================*/

/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/

/*================FUNCTION DECLARATION  SECTION  END========*/

pm_lpo_src_e bk_clk_32k_customer_config_get(void)
{
#if CONFIG_LPO_MP_A_FORCE_USE_EXT32K
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK))
	{
		return PM_LPO_SRC_X32K;
	}
	else
	{
		#if CONFIG_EXTERN_32K
			return PM_LPO_SRC_X32K;
		#elif CONFIG_LPO_SRC_26M32K
			return PM_LPO_SRC_DIVD;
		#else
			return PM_LPO_SRC_ROSC;
		#endif
	}
#else
	#if CONFIG_EXTERN_32K
		return PM_LPO_SRC_X32K;
	#elif CONFIG_LPO_SRC_26M32K
		return PM_LPO_SRC_DIVD;
	#else
		return PM_LPO_SRC_ROSC;
	#endif
#endif
	return PM_LPO_SRC_ROSC;
}
