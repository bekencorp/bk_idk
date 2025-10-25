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

#include <string.h>
#include <common/sys_config.h>
#include "bk_uart.h"
#if CONFIG_LWIP
#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>
#endif
#include "lwip/ip4.h"
#include "bk_private/bk_wifi.h"
#include "bk_wifi_private.h"
#include "bk_cli.h"
#include "cli.h"
#include <components/event.h>
#include <components/netif.h>
#include "bk_wifi.h"
#include "bk_wifi_types.h"
#if CONFIG_WIFI_CLI_RAW_LINK_ENABLE
#include <modules/raw_link.h>
#endif
#include "bk_wifi.h"
#include "ftp/ftpd.h"
#include <driver/flash.h>

#define WIFI_TEST_BASE_ADDR            0x210000
#define COUNT_SAVE_ADDR                (WIFI_TEST_BASE_ADDR)
#define COUNT_SAVE_BAKEUP_ADDR         (WIFI_TEST_BASE_ADDR - 4096)

#define WIFI_SAVE_RESULT_INTERVAL      6
#define WIFI_SAVE_BAK_RESULT_INTERVAL  (WIFI_SAVE_RESULT_INTERVAL * 5)

#define WIFI_SAVE_SSID                 "wifi_reliable_test"
#define WIFI_SAVE_KEY                  "12345678"

#define LOW_TEMP_NUM                   (40)
#define HIGH_TEMP_NUM                  (125)
#define TEMP_RESULT_LEN                (LOW_TEMP_NUM + HIGH_TEMP_NUM + 2)

struct wifi_test_st
{
	uint32_t total_test_time;   //  second
	uint32_t temp_count_tab[TEMP_RESULT_LEN][2];
	uint32_t crc;
};
struct wifi_test_st g_wifi_st_save = {0};
uint32_t g_wifi_test_run = 0;
int g_tmp_code = 0;
static beken_thread_t  restable_wifi_task_handle = NULL;

static const uint32_t crc32_table[] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/**
 * Calculate the CRC32 value of a memory buffer.
 *
 * @param crc accumulated CRC32 value, must be 0 on first call
 * @param buf buffer to calculate CRC32 value for
 * @param size bytes in buffer
 *
 * @return calculated CRC32 value
 */
static uint32_t my_calc_crc32(uint32_t crc, const void *buf, size_t size)
{
	const uint8_t *p;

	p = (const uint8_t *)buf;
	crc = crc ^ ~0U;

	while (size--) {
		crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ ~0U;
}

void cli_wifi_disconnect_handler(void)
{
	int get_current_temperature_code(void);
	int tmp_code = get_current_temperature_code();
	int idx;

	if(tmp_code < -(LOW_TEMP_NUM))
	{
		idx = 0;
	}
	else if(tmp_code > (HIGH_TEMP_NUM))
	{
		idx = TEMP_RESULT_LEN - 1;
	}
	else
	{
		idx = tmp_code + (LOW_TEMP_NUM + 1);
	}

	uint32_t count = g_wifi_st_save.temp_count_tab[idx][0];
	count++;
	g_wifi_st_save.temp_count_tab[idx][0] = count;
	//CLI_LOGI("BK STA disconnected, temp_code:%d, total_cnt:%d\n", tmp_code, count);
}

void cli_wifi_show_test_result(void)
{
	CLI_LOGI("\r\nBuild_info:%s_%s\r\n", __DATE__, __TIME__);
	if(g_wifi_test_run == 0)
	{
		CLI_LOGI("cli_restable_wifi no start\r\n");
		return;
	}

	CLI_LOGI(" wifi test result total time: %d (s)\r\n", g_wifi_st_save.total_test_time);
	CLI_LOGI(" wifi test result tab:\r\n");
	CLI_LOGI("%-12s   %-5s   %-5s\r\n",
		"temperature_code", "disc_count", "dump_count");
	for(int idx=0; idx<TEMP_RESULT_LEN; idx++)
	{
		if(idx == 0)
		{
			CLI_LOGI("\t<%d\t\t%d\t\t%d\r\n", -LOW_TEMP_NUM, g_wifi_st_save.temp_count_tab[idx][0], g_wifi_st_save.temp_count_tab[idx][1]);
		}
		else if(idx == TEMP_RESULT_LEN-1)
		{
			CLI_LOGI("\t>%d\t\t%d\t\t%d\r\n", HIGH_TEMP_NUM, g_wifi_st_save.temp_count_tab[idx][0], g_wifi_st_save.temp_count_tab[idx][1]);
		}
		else
		{
			CLI_LOGI("\t%d\t\t%d\t\t%d\r\n", idx-LOW_TEMP_NUM-1, g_wifi_st_save.temp_count_tab[idx][0], g_wifi_st_save.temp_count_tab[idx][1]);
		}
	}
	CLI_LOGI("\r\n");
}

void cli_wifi_show_bakup_test_result(void)
{
	struct wifi_test_st wifi_st_save = {0};
	bk_flash_read_bytes(COUNT_SAVE_BAKEUP_ADDR, (uint8_t*)&wifi_st_save, sizeof(wifi_st_save));
	uint32_t crc = 0;
	crc = my_calc_crc32(crc, &wifi_st_save, sizeof(wifi_st_save)-4);

	if ((crc != wifi_st_save.crc) || (wifi_st_save.crc == 0xffffffff))
	{
		os_printf("found bakup history info not effected, exit\r\n");
		return;
	}

	CLI_LOGI(" wifi test bakup result total time: %d (s)\r\n", wifi_st_save.total_test_time);
	CLI_LOGI(" wifi test bakup result tab:\r\n");
	CLI_LOGI("%-12s   %-5s   %-5s\r\n",
		"temperature_code", "disc_count", "dump_count");
	for(int idx=0; idx<TEMP_RESULT_LEN; idx++)
	{
		if(idx == 0)
		{
			CLI_LOGI("\t<%d\t\t%d\t\t%d\r\n", -LOW_TEMP_NUM, g_wifi_st_save.temp_count_tab[idx][0], g_wifi_st_save.temp_count_tab[idx][1]);
		}
		else if(idx == TEMP_RESULT_LEN-1)
		{
			CLI_LOGI("\t>%d\t\t%d\t\t%d\r\n", HIGH_TEMP_NUM, g_wifi_st_save.temp_count_tab[idx][0], g_wifi_st_save.temp_count_tab[idx][1]);
		}
		else
		{
			CLI_LOGI("\t%d\t\t%d\t\t%d\r\n", idx-LOW_TEMP_NUM-1, g_wifi_st_save.temp_count_tab[idx][0], g_wifi_st_save.temp_count_tab[idx][1]);
		}
	}
	CLI_LOGI("\r\n");
}

void cli_restable_wifi_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	cli_wifi_show_test_result();
	cli_wifi_show_bakup_test_result();
}

void cli_wifi_save_wifi_test_result(uint32 flag)
{
	static uint32_t g_save_flag = 0;

	if((g_save_flag == 0) || (flag))
	{
		g_save_flag = 1;
		uint32_t crc = 0;

		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		GLOBAL_INT_DECLARATION();

		GLOBAL_INT_DISABLE();
		crc = my_calc_crc32(crc, &g_wifi_st_save, sizeof(g_wifi_st_save)-4);
		g_wifi_st_save.crc = crc;
		GLOBAL_INT_RESTORE();

		bk_flash_erase_sector(COUNT_SAVE_ADDR);
		bk_flash_write_bytes(COUNT_SAVE_ADDR, (uint8_t *)&g_wifi_st_save, sizeof(g_wifi_st_save));

		if(((g_wifi_st_save.total_test_time) % WIFI_SAVE_BAK_RESULT_INTERVAL) == 0)
		{
			bk_flash_erase_sector(COUNT_SAVE_BAKEUP_ADDR);
			bk_flash_write_bytes(COUNT_SAVE_BAKEUP_ADDR, (uint8_t *)&g_wifi_st_save, sizeof(g_wifi_st_save));
		}
		bk_flash_set_protect_type(FLASH_PROTECT_ALL);
		g_save_flag = 0;
	}
}

void cli_wifi_save_wifi_test_result_in_dump(void)
{
	if(g_wifi_test_run == 0)
		return;

	int tmp_code = g_tmp_code;
	int idx;

	if(tmp_code < -(LOW_TEMP_NUM))
	{
		idx = 0;
	}
	else if(tmp_code > (HIGH_TEMP_NUM))
	{
		idx = TEMP_RESULT_LEN - 1;
	}
	else
	{
		idx = tmp_code + (LOW_TEMP_NUM + 1);
	}

	uint32_t count = g_wifi_st_save.temp_count_tab[idx][1];
	count++;
	g_wifi_st_save.temp_count_tab[idx][1] = count;
	cli_wifi_save_wifi_test_result(1);
}

void cli_restable_wifi_main_test(beken_thread_arg_t arg)
{
	uint32 crc = 0;
	rtos_delay_milliseconds(5000);

	bk_flash_read_bytes(COUNT_SAVE_ADDR, (uint8_t*)&g_wifi_st_save, sizeof(g_wifi_st_save));
	crc = my_calc_crc32(crc, &g_wifi_st_save, sizeof(g_wifi_st_save)-4);

	if (g_wifi_st_save.crc == 0xffffffff)
	{
		os_printf("found history info not effected, use all 0 for init\r\n");
		os_memset(&g_wifi_st_save, 0, sizeof(g_wifi_st_save));
	}
	else if (crc != g_wifi_st_save.crc)
	{
		os_printf("found history info crc err\r\n");

		struct wifi_test_st wifi_st_save = {0};
		bk_flash_read_bytes(COUNT_SAVE_BAKEUP_ADDR, (uint8_t*)&wifi_st_save, sizeof(wifi_st_save));
		uint32 crc = 0;
		crc = my_calc_crc32(crc, &wifi_st_save, sizeof(wifi_st_save)-4);

		if ((crc != wifi_st_save.crc) || (wifi_st_save.crc == 0xffffffff))
		{
			//os_printf("found bakup history info not effected, exit\r\n");
			//restable_wifi_task_handle = NULL;
			//rtos_delete_thread(NULL);
			os_printf("found bakup history info not effected, use all 0 for init\r\n");
			os_memset(&g_wifi_st_save, 0, sizeof(g_wifi_st_save));
		}
		else
		{
			os_printf("found bakeup history info effected, use bakeup\r\n");
			memcpy(&g_wifi_st_save, &wifi_st_save, sizeof(wifi_st_save));
		}
	}
	else
	{
		//os_printf("found history info effected\r\n");
	}

	int get_current_temperature_code(void);
	g_tmp_code = get_current_temperature_code();
	g_wifi_test_run = 1;
	demo_sta_app_init((char *)WIFI_SAVE_SSID, WIFI_SAVE_KEY);

	while(1)
	{
		rtos_delay_milliseconds(1000);

		g_wifi_st_save.total_test_time++;
		g_tmp_code = get_current_temperature_code();

		if(((g_wifi_st_save.total_test_time) % WIFI_SAVE_RESULT_INTERVAL) == 0)
		{
			cli_wifi_save_wifi_test_result(0);
		}
	}

	restable_wifi_task_handle = NULL;
	rtos_delete_thread(NULL);
}

void cli_restable_wifi_start(void)
{
	if(restable_wifi_task_handle == NULL)
	{
		rtos_create_thread(&restable_wifi_task_handle,
			4,
			"wifi_test",
			(beken_thread_function_t)cli_restable_wifi_main_test,
			8192,
			NULL);
	}
}