// Copyright 2022-2023 Beken
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
#include "soc/soc.h"

#define CORTEX_M33_REG_DAUTHCTRL           0xE000EE04
#define CORTEX_M33_CPU_DEBUG_ENABLE_VALUE  0x0000000F
#define CORTEX_M33_CPU_DEBUG_DISABLE_VALUE 0x00000005

void tfm_enable_swd(void)
{
	*((volatile uint32_t*)CORTEX_M33_REG_DAUTHCTRL) = CORTEX_M33_CPU_DEBUG_ENABLE_VALUE;
	*((volatile uint32_t*)0x44000450) = 0x50048;
	*((volatile uint32_t*)0x44000454) = 0x50048;
}
