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
#include "cli.h"                    /**< CLI interface */
#include <os/os.h>                  /**< Operating system interface */
#include <driver/psram.h>           /**< PSRAM driver interface */
#include <driver/aon_rtc.h>         /**< Always-on RTC driver */

/* Conditional includes */
#if CONFIG_TRNG_SUPPORT
#include <driver/trng.h>            /**< True Random Number Generator */
#endif

/* Hardware abstraction includes */
#include "bk_general_dma.h"         /**< General DMA interface */
#include <driver/dma.h>             /**< DMA driver interface */
#include "soc/mapping.h"            /**< SoC memory mapping */
#include "psram_hal.h"              /**< PSRAM hardware abstraction layer */
#include "cache.h"                  /**< Cache management */
#include "bk_sensor_internal.h"     /**< Sensor internal interface */

#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"                  /**< EF (Embedded Flash) interface */
#endif

/* PSRAM test configuration constants */
#define PSRAM_PRESSURE_TEST_LEN   (CONFIG_PSRAM_HEAP_SIZE)  /**< Total test length in bytes */
#define PSRAM_PRESSURE_BUF_LEN    (1024 * 4)                /**< Test buffer length in bytes */
#define PSRAM_PRESSURE_ADDR       (0x60000000)              /**< PSRAM test base address */

/* Magic numbers for test patterns */
#define PSRAM_TEST_PATTERN        0xA55AA55A                /**< Test pattern for PSRAM testing */

/* External function declarations */
extern int bk_rand(void);                                   /**< Random number generator */
void flush_all_dcache(void);                                /**< Flush all data cache */
/**
 * @brief PSRAM reliability test handler
 * 
 * This function performs a comprehensive PSRAM reliability test including:
 * - PSRAM initialization
 * - Random data generation and writing
 * - Data verification and error counting
 * - Memory cleanup
 * 
 * @param number Pointer to store error count (optional)
 * 
 * @return 1 if test completed successfully, 0 if PSRAM initialization failed
 * 
 * @note This function allocates memory for test buffer and must be called
 *       in a context where memory allocation is available
 */
uint32_t cli_psram_test_handle(uint32_t *number)
{
	uint32_t total_words, correct_words, correctness_rate;
	uint32_t test_len = PSRAM_PRESSURE_TEST_LEN;      /* Total test length */
	uint32_t buffer_len = PSRAM_PRESSURE_BUF_LEN;     /* Buffer length for each iteration */
	uint32_t *data = (uint32_t *)os_malloc(PSRAM_PRESSURE_BUF_LEN);  /* Test data buffer */

	/* Check memory allocation */
	if(data == NULL)
	{
		CLI_LOGI("Failed to allocate memory for test buffer\r\n");
		return 0;
	}

	bk_err_t ret = bk_psram_init();
	if(ret != BK_OK)
	{
		CLI_LOGI("PSRAM initialization failed\r\n");
		os_free(data);  /* Free allocated memory before returning */
		data = NULL;    /* Set pointer to NULL to prevent dangling pointer */
		return 0;
	}

	/* Generate test data pattern */
	for (int i = 0; i < buffer_len / 4; i++)
	{
#if 1  // CONFIG_TRNG_SUPPORT
		data[i] = bk_rand() + i;  /* Use random data for testing */
#else
		data[i] = PSRAM_TEST_PATTERN + i + (i << 8) + (i << 16) + (i << 24);  /* Use pattern data */
#endif
	}

	/* Initialize test variables */
	uint32_t i = 0;                    /* Loop counter */
	uint32_t error_num = 0;            /* Error counter */
	uint32_t read_value = 0;           /* Value read from PSRAM */
	uint32_t base_addr = PSRAM_PRESSURE_ADDR;  /* PSRAM base address */

	/* Write test data to PSRAM */
	CLI_LOGI("PSRAM write test: %08x-%08x\r\n", base_addr, base_addr + test_len);

	/* Write test data in chunks */
	for (i = 0; i < test_len / buffer_len; i++)
	{
		for (int j = 0; j < buffer_len / 4; j++)
		{
			*((volatile uint32_t *)(base_addr + i*buffer_len + j*4)) = data[j];
		}
		/* flush_all_dcache(); */  /* Cache flush disabled for testing */
	}

	CLI_LOGI("PSRAM read test: %08x-%08x\r\n", base_addr, base_addr + test_len);

	/* Read and verify test data */
	for (i = 0; i < test_len / buffer_len; i++)
	{
		for (int k = 0; k < buffer_len / 4; k++)
		{
			read_value = *((volatile uint32_t *)(base_addr + i*buffer_len + k*4));

			if (read_value != data[k])
			{
				CLI_LOGI("Error: addr %08x, read %08x, expected %08x, diff %08x\n", 
						base_addr + i * buffer_len + k * 0x4, read_value, data[k], read_value^data[k]);
				error_num++;
				goto my_test_exit;
			}
		}
	}

my_test_exit:
	/* Calculate and display test results */
	total_words = test_len / 4;
	correct_words = total_words - error_num;
	correctness_rate = (correct_words * 100) / total_words;
	
	CLI_LOGI("PSRAM test completed: errors=%ld, correctness_rate=%ld%%\r\n", 
			 error_num, correctness_rate);
		
	/* Return error count if requested */
	if(number)
	{
		*number = error_num;
	}
	
	/* Clean up allocated memory */
	if(data)
	{
		os_free(data);
		data = NULL;    /* Set pointer to NULL to prevent dangling pointer */
	}

	return 1;
}
