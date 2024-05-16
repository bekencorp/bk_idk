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

#include "bk_arch.h"
#include "armstar.h"

void arch_init(void)
{
}

void smem_reset_lastblock(void)
{
}

__attribute__((section(".itcm_sec_code"))) void arch_deep_sleep(void)
{
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	__WFI();
}

__attribute__((section(".iram"))) void arch_sleep(void)
{
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	__WFI();
}

__attribute__((section(".itcm_sec_code")))uint64_t check_IRQ_pending(void)
{
	return NVIC_GetAllPendingIRQ();
}

void arch_atomic_set(  volatile uint32_t *lk ) /* PRIVILEGED_FUNCTION */
{
	int result;

	do
	{
		while(__LDAEX(lk) != 0);

		result = __STREXW(1, lk);
	} while(result != 0);

	return;
}

void arch_atomic_clear(volatile uint32_t *lk)
{
	__STL(0, lk);

	return;
}

void arch_fence(void)
{
}

void arch_dwt_trap_write(uint32_t addr, uint32_t data)
{
	DCB->DEMCR &= ~(DCB_DEMCR_TRCENA_Msk | DCB_DEMCR_MON_EN_Msk);

	DWT->COMP2 = addr;
	DWT->FUNCTION2 = 0x815;
	DWT->COMP3 = data;
	DWT->FUNCTION3 = 0x81b;

	DCB->DEMCR |= (DCB_DEMCR_TRCENA_Msk | DCB_DEMCR_MON_EN_Msk);
}

void arch_dwt_trap_disable(void)
{
	DCB->DEMCR &= ~(DCB_DEMCR_TRCENA_Msk | DCB_DEMCR_MON_EN_Msk);
	DWT->FUNCTION0 = 0;
	DWT->FUNCTION1 = 0;
	DWT->FUNCTION2 = 0;
	DWT->FUNCTION3 = 0;
}

// eof

