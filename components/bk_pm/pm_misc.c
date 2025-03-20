// Copyright 2020-2023 Beken
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

#include<components/sensor.h>
#include "sys_driver.h"

#include "driver/aon_rtc.h"
#include "bk_pm_internal_api.h"


/*=====================DEFINE  SECTION  START=====================*/

#define PM_CP1_SOC_AON_RTC_REG_BASE          (SOC_AON_RTC_REG_BASE)
#define PM_AON_RTC_CNT_VAL_L_OFFSET          (0x3*4)
#define PM_AON_RTC_CNT_VAL_H_OFFSET          (0xa*4)

#define PM_AON_RTC_CNT_VAL_L_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_L_OFFSET)
#define PM_AON_RTC_CNT_VAL_H_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_H_OFFSET)

#if CONFIG_PM_LV_TIME_COST_DEBUG
#define PM_SOC_AON_PMU_REG_BASE              (SOC_AON_PMU_REG_BASE)
#define PM_AON_PMU_LPO_OFFSET                (0x41*4)

#define PM_AON_PMU_LPO_ADDR                  (SOC_AON_PMU_REG_BASE + PM_AON_PMU_LPO_OFFSET)

#define PM_LPO_EX32K_TICK_COUNT              (32768)
#define PM_LPO_ROSC_TICK_COUNT               (32000)
#endif


/*=====================DEFINE  SECTION  END=======================*/

/*=====================VARIABLE  SECTION  START===================*/
#if CONFIG_PM_LV_TIME_COST_DEBUG
static uint64_t s_wakeup_lv_tick_step[PM_LV_STEP_MAX] = {0};
#endif
/*=====================VARIABLE  SECTION  END=====================*/

/*================FUNCTION DECLARATION  SECTION  START============*/


/*================FUNCTION DECLARATION  SECTION  END===============*/
uint64_t pm_cp1_aon_rtc_counter_get()
{
	volatile uint32_t val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
	volatile uint32_t val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);

	while (REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR) != val
		|| REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR) != val_hi)
	{
		val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
		val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);
	}
	return (((uint64_t)(val_hi) << 32) + val);
}


void pm_printf_current_temperature(void)
{
#if CONFIG_TEMP_DETECT
	float temp;

	bk_sensor_get_current_temperature(&temp);
	os_printf("current chip temperature about %.2f\r\n",temp);
#endif
}


/*Why redefine the function:*/
/*The low-voltage code needs to be placed in the ITCM and cannot be in the flash. The existing interface code is large in ITCM,
and putting all of it in the ITCM will occupy more ITCM/iram space.
Additionally, this is only test code that will only be allowed to run after a command is issued to start it*/
#if CONFIG_PM_LV_TIME_COST_DEBUG
__attribute__((section(".iram"))) uint64_t pm_rtc_cur_tick_get()
{
	#if CONFIG_AON_RTC
	volatile uint32_t val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
	volatile uint32_t val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);
	while (REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR) != val
		|| REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR) != val_hi)
	{
		val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
		val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);
	}
	return (((uint64_t)(val_hi) << 32) + val);
	#else
	return BK_OK;
	#endif
}
__attribute__((section(".iram")))bk_err_t pm_wakeup_lv_rtc_tick_clear()
{
	for(int i = PM_LV_WAKEUP_STEP_0; i <= PM_LV_WAKEUP_STEP_5; i++)
	{
		s_wakeup_lv_tick_step[i] = 0;
	}
	return BK_OK;
}
__attribute__((section(".iram"))) bk_err_t pm_enter_lv_rtc_tick_clear()
{
	for(int i = PM_LV_ENTER_STEP_0; i <= PM_LV_ENTER_STEP_5; i++)
	{
		s_wakeup_lv_tick_step[i] = 0;
	}
	return BK_OK;
}
__attribute__((section(".itcm_sec_code"))) bk_err_t pm_lv_rtc_tick_set(pm_lv_step_e step,uint64_t tick)
{
	if(step < PM_LV_STEP_MAX)
	{
		s_wakeup_lv_tick_step[step] = tick;
	}
	return BK_OK;
}
uint64_t pm_lv_rtc_tick_get(pm_lv_step_e step)
{
	if(step < PM_LV_STEP_MAX)
	{
		return s_wakeup_lv_tick_step[step];
	}
	return 0;
}

 uint64_t pm_lv_rtc_interval_get(pm_lv_step_e step)
{
	uint32_t tick_count = 0.0;
	uint64_t interval = 0;
	if(step > PM_LV_STEP_MAX)
	{
		return 0;
	}
	uint32_t val = REG_READ(PM_AON_PMU_LPO_ADDR);
	if((val&0x3) == PM_LPO_SRC_X32K)
	{
		tick_count = PM_LPO_EX32K_TICK_COUNT;
	}
	else
	{
		tick_count = PM_LPO_ROSC_TICK_COUNT;
	}
	if(step == PM_LV_ENTER_STEP_0)
	{
		interval = (uint64_t)s_wakeup_lv_tick_step[step];
	}
	else
	{
		interval = (uint64_t)(((s_wakeup_lv_tick_step[step] - s_wakeup_lv_tick_step[step-1])*1000000)/tick_count);
	}
	return interval;
}
#endif
