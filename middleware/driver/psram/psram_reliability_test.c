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

#include "cli.h"

#include <os/os.h>
#include <driver/psram.h>
#include <driver/aon_rtc.h>
#if CONFIG_TRNG_SUPPORT
#include <driver/trng.h>
#endif
#include "bk_general_dma.h"
#include <driver/dma.h>
#include "soc/mapping.h"
#include "psram_hal.h"
#include "cache.h"
#include "bk_sensor_internal.h"

#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"
#endif

#define PSRAM_PRESSURE_TEST_LEN   (CONFIG_PSRAM_HEAP_SIZE)
#define PSRAM_PRESSURE_BUF_LEN    (1024 * 4)
#define PSRAM_PRESSURE_ADDR       (0x60000000)
extern int bk_rand(void);
void flush_all_dcache(void);
uint32_t cli_psram_test_handle(uint32_t *number)
{
	uint32_t test_len = PSRAM_PRESSURE_TEST_LEN, buffer_len = PSRAM_PRESSURE_BUF_LEN;
	uint32_t *data = (uint32_t *)os_malloc(PSRAM_PRESSURE_BUF_LEN);

	bk_err_t ret = bk_psram_init();
	if(ret != BK_OK)
	{
		return 0;
	}

	for (int i = 0; i < buffer_len / 4; i++)
	{
#if 1//CONFIG_TRNG_SUPPORT
		data[i] = bk_rand() + i;
#else
		data[i] = 0xA55AA55A + i + (i << 8) + (i << 16) + (i << 24);
#endif
	}

	uint32_t i = 0;
	uint32_t error_num = 0;
	uint32_t value = 0;
	uint32_t base_addr = PSRAM_PRESSURE_ADDR;

//1Byte
	CLI_LOGI("abcd begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / buffer_len; i++)
	{
		for (int j = 0; j < buffer_len / 4; j++)
		{
			*((volatile uint32_t *)(base_addr + i*buffer_len + j*4)) = data[j];
		}
		//flush_all_dcache();
	}

	CLI_LOGI("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	for (i = 0; i < test_len / buffer_len; i++)
	{
		for (int k = 0; k < buffer_len / 4; k++)
		{
			value = *((volatile uint32_t *)(base_addr + i*buffer_len + k*4));

			if (value != data[k])
			{
				CLI_LOGI("err: addr %08x, val %08x %08x %08x\n", base_addr + i * buffer_len + k * 0x4, value, data[k], value^data[k]);
				error_num++;
				goto my_test_exit;
			}
		}
		//flush_all_dcache();
	}

my_test_exit:
	CLI_LOGI("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, (test_len / 4 - error_num) * 100 / (test_len / 4));
	if(number)
	{
		*number = error_num;
	}
	if(data)
		os_free(data);

	return 1;
}