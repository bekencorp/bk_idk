/*
 * Copyright (c) 2009-2018 Arm Limited.
 * Copyright (c) 2018-2020 ArmChina.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "system_star.h"
#include "armstar.h"
#if defined (__ARM_FEATURE_CMSE) &&  (__ARM_FEATURE_CMSE == 3U)
  #include "partition_star.h"
#endif

#if CONFIG_DEEP_LV
#include "deep_lv.h"
#endif

#include "sys_hal.h"
#include "timer_hal.h"
#include "wdt_hal.h"
/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define  SYSTEM_CLOCK    CONFIG_XTAL_FREQ

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
extern const VECTOR_TABLE_Type __VECTOR_TABLE[];

extern const VECTOR_TABLE_Type __VECTOR_TABLE_IRAM[];



/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit (void)
{
#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
	SCB->VTOR = (uint32_t) &(__VECTOR_TABLE[0]);
#endif

#ifdef UNALIGNED_SUPPORT_DISABLE
	SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif

#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#if CONFIG_SPE
	TZ_SAU_Setup();
#endif
#endif

#if CONFIG_SPE
    /* secureFault enable*/
	SCB->SHCSR |= SCB_SHCSR_SECUREFAULTENA_Msk;
#endif

	/* enable system fault exception*/
	SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
	SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;

	/* enable div_0_trp*/
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;

	/* disable unalign_trp */
	SCB->CCR &= (~SCB_CCR_UNALIGN_TRP_Msk);

#if (CONFIG_ICACHE)
	if (SCB->CLIDR & SCB_CLIDR_IC_Msk)
		SCB_EnableICache();
#endif
}

__IRAM_SEC void SystemInit_iram (void)
{
#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
	SCB->VTOR = (uint32_t) &(__VECTOR_TABLE_IRAM[0]);
#endif

#ifdef UNALIGNED_SUPPORT_DISABLE
	SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif

#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#if CONFIG_SPE
	TZ_SAU_Setup_iram();
#endif
#endif

#if CONFIG_SPE
    /* secureFault enable*/
	SCB->SHCSR |= SCB_SHCSR_SECUREFAULTENA_Msk;
#endif

	/* enable system fault exception*/
	SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
	SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;

	/* enable div_0_trp*/
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;

	/* disable unalign_trp */
	SCB->CCR &= (~SCB_CCR_UNALIGN_TRP_Msk);

#if (CONFIG_ICACHE)
	if (SCB->CLIDR & SCB_CLIDR_IC_Msk)
		SCB_EnableICache();
#endif

#if (CONFIG_DEEP_LV)
    dlv_startup();
#endif
}

extern uint32_t __STACK_LIMIT;
__IRAM_SEC void deep_lv_fast_boot(void)
{
	void Reset_Handler(void);
	uint32_t memory_is_retention(void);
	if(memory_is_retention())
	{
		__set_MSPLIM((uint32_t)(&__STACK_LIMIT));
		wdt_hal_close();
		timer_hal_us_init_iram();
		sys_hal_enable_pll_iram();
		sys_hal_enable_high_freq_iram();
		SystemInit_iram();
	}
	else
	{
		Reset_Handler();
	}
}