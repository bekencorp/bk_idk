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

#pragma once

#include <soc/soc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CLOCK_XTAL_FLASH_26M = 1,
	CLOCK_PLL_FLASH_40M,
	CLOCK_PLL_FLASH_80M,
} flash_clock_t;

typedef enum {
	CLOCK_XTAL_CPU_26M = 1,
	CLOCK_PLL_CPU_40M,
	CLOCK_PLL_CPU_80M,
	CLOCK_PLL_CPU_120M,
} cpu_clock_t;

typedef enum {
	CLOCK_XTAL_SPI_26M = 0,
	CLOCK_PLL_SPI_40M,
} spi_clock_t;

void enable_high_freq(void);
void hal_enable_pll(void);
void hal_disable_pll(void);
void hal_enable_high_freq(void);
void hal_set_cpu_clock(cpu_clock_t clock);
void hal_set_spi_clock(spi_clock_t clock);
void hal_set_flash_clock(flash_clock_t clock);
void spi_hal_set_baud_rate(void);
void change_uart_high_baudrate(uint32_t baudrate);

#ifdef __cplusplus
}
#endif
