#include <stdlib.h>
#include "cli.h"
#include "driver/flash.h"
#include <driver/flash_partition.h>
#include "sys_driver.h"
#include "flash_bypass.h"
#if CONFIG_FLASH_BYPASS_OTP_V2
#include "flash_bypass_otp.h"
#endif
#include "flash.h"
#include <driver/psram.h>
#include "bk_misc.h"
#include "driver/wdt.h"
#include "bk_wdt.h"
#include <driver/aon_rtc.h>
#include <common/bk_assert.h>

#if CONFIG_FLASH_RACE_TEST
#include "flash_thread_safety_test.h"
#endif
#if CONFIG_FLASH_UT_TEST
#include "flash_ut_test.h"
#endif

#define  TICK_PER_US    26

beken_thread_t idle_read_flash_handle = NULL;
beken_thread_t idle_read_psram_handle = NULL;
static volatile uint32_t idle_read_flash_count = 0;

static bk_err_t test_flash_write(volatile uint32_t start_addr, uint32_t len)
{
	uint32_t i;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	uint32_t tmp = addr + length;

	for (i = 0; i < 256; i++)
		buf[i] = i;

	for (; addr < tmp; addr += 256) {
		#if (CONFIG_TASK_WDT)
    		bk_task_wdt_feed();
		#endif
		int int_level = rtos_enter_critical();
		BK_DUMP_OUT("write addr(size:256):%d\r\n", addr);
		rtos_exit_critical(int_level);
		bk_flash_write_bytes(addr, (uint8_t *)buf, 256);
	}

	return kNoErr;
}

static bk_err_t test_flash_erase(volatile uint32_t start_addr, uint32_t len)
{
	bk_err_t err = BK_OK;
    uint32_t addr = start_addr;
    uint32_t length = len;

#if (CONFIG_TASK_WDT)
    bk_task_wdt_feed();
#endif

	BK_DUMP_OUT("erase addr: 0x%08X\r\n", addr);
	if((err = bk_flash_erase_fast(start_addr,length))!=0)
	{
		BK_DUMP_OUT("erase addr failed!\r\n");
	}

    return err;
}

static bk_err_t test_flash_read(volatile uint32_t start_addr, uint32_t len)
{
	uint32_t i, j, tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	tmp = addr + length;

	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
		int int_level = rtos_enter_critical();
		BK_DUMP_OUT("read addr:%x\r\n", addr);
		rtos_exit_critical(int_level);
		for (i = 0; i < 16; i++) {
			for (j = 0; j < 16; j++)
				BK_DUMP_OUT("%02x ", buf[i * 16 + j]);
			BK_DUMP_OUT("\r\n");
		}
	}

	return kNoErr;
}

#define IDLE_READ_FLASH_PAUSE_EVERY_N_READS  1024
#define IDLE_READ_FLASH_PAUSE_MS             1

static void idle_read_flash_pause_if_needed(void)
{
	if ((idle_read_flash_count % IDLE_READ_FLASH_PAUSE_EVERY_N_READS) != 0) {
		return;
	}
	rtos_delay_milliseconds(IDLE_READ_FLASH_PAUSE_MS);
}

static bk_err_t test_flash_read_without_print(volatile uint32_t start_addr, uint32_t len)
{
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	tmp = addr + length;

	while (1) {
		for (addr = start_addr; addr < tmp; addr += 256) {
			bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
			idle_read_flash_count++;
			idle_read_flash_pause_if_needed();
		}
	}

	return kNoErr;
}

static bk_err_t test_flash_read_time(volatile uint32_t start_addr, uint32_t len)
{
	UINT32 time_start, time_end;
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;

	tmp = addr + length;
	beken_time_get_time((beken_time_t *)&time_start);
	BK_DUMP_OUT("read time start:%d\r\n", time_start);

	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
	}
	beken_time_get_time((beken_time_t *)&time_end);
	BK_DUMP_OUT("read time end:%d\r\n", time_end);
	BK_DUMP_OUT("cost time:%d\r\n", time_end - time_start);

	return kNoErr;
}

static bk_err_t test_flash_count_time(volatile uint32_t start_addr, uint32_t len, uint32_t test_times)
{
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	int32_t tick_cnt = 0;
	uint32_t print_cnt = 100;
	struct timeval rtc_start_time = {0, 0};
	struct timeval rtc_end_time = {0, 0};

	bk_wdt_stop();
#if (CONFIG_TASK_WDT)
	bk_task_wdt_stop();
#endif
	tmp = addr + length;
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);

	BK_DUMP_OUT("----- FLASH COUNT TIME TEST BEGIN -----\r\n");
	BK_DUMP_OUT("===============================\r\n");

	for(int i = 0; i <= test_times; i++) {
		bk_rtc_gettimeofday(&rtc_start_time, 0);
		for (addr = start_addr; addr < tmp; addr += 256) {
			os_memset(buf, 0, 256);
			bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
		}
		bk_rtc_gettimeofday(&rtc_end_time, 0);
		tick_cnt = 1000000 * (rtc_end_time.tv_sec - rtc_start_time.tv_sec) + rtc_end_time.tv_usec - rtc_start_time.tv_usec;
		if(i % print_cnt == 0)
			BK_DUMP_OUT("[read %d time] >>>>> cost time: %d us.\r\n", i, tick_cnt);


		bk_rtc_gettimeofday(&rtc_start_time, 0);
		for (addr = start_addr; addr < tmp; addr += 0x1000) {
			bk_flash_erase_sector(addr);
		}
		bk_rtc_gettimeofday(&rtc_end_time, 0);
		tick_cnt = 1000000 * (rtc_end_time.tv_sec - rtc_start_time.tv_sec) + rtc_end_time.tv_usec - rtc_start_time.tv_usec;
		if(i % print_cnt == 0)
			BK_DUMP_OUT("[erase %d time] >>>>> cost time: %d us.\r\n", i, tick_cnt);

		//check erase data valid
		if(i % print_cnt == 0) {
			for (addr = start_addr; addr < tmp; addr += 256) {
				os_memset(buf, 0, 256);
				bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
				for (int j = 0; j < 256; j++) {
					if(buf[j] != 0xff)
						BK_DUMP_OUT("[erase %d time ERROR]: addr = 0x%x.\r\n", i, addr + j);
				}
			}
		}


		for (int j = 0; j < 256; j++)
			buf[j] = j;

		bk_rtc_gettimeofday(&rtc_start_time, 0);
		for (addr = start_addr; addr < tmp; addr += 256) {
			bk_flash_write_bytes(addr, (uint8_t *)buf, 256);
		}
		bk_rtc_gettimeofday(&rtc_end_time, 0);
		tick_cnt = 1000000 * (rtc_end_time.tv_sec - rtc_start_time.tv_sec) + rtc_end_time.tv_usec - rtc_start_time.tv_usec;
		if(i % print_cnt == 0)
			BK_DUMP_OUT("[write %d time] >>>>> cost time: %d us.\r\n", i, tick_cnt);

		//check write data valid
		if(i % print_cnt == 0) {
			for (addr = start_addr; addr < tmp; addr += 256) {
				os_memset(buf, 0, 256);
				bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
				for (int j = 0; j < 256; j++) {
					if(buf[j] != j)
						BK_DUMP_OUT("[write %d time ERROR]: addr = 0x%x.\r\n", i, addr);
				}
			}
		}


		bk_rtc_gettimeofday(&rtc_start_time, 0);
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		bk_rtc_gettimeofday(&rtc_end_time, 0);
		tick_cnt = 1000000 * (rtc_end_time.tv_sec - rtc_start_time.tv_sec) + rtc_end_time.tv_usec - rtc_start_time.tv_usec;
		if(i % print_cnt == 0) {
			BK_DUMP_OUT("[enable security %d time] >>>>> cost time: %d us.\r\n", i, tick_cnt);
			BK_DUMP_OUT("===============================\r\n");
		}
	}

	bk_rtc_gettimeofday(&rtc_start_time, 0);
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
	bk_rtc_gettimeofday(&rtc_end_time, 0);
	tick_cnt = 1000000 * (rtc_end_time.tv_sec - rtc_start_time.tv_sec) + rtc_end_time.tv_usec - rtc_start_time.tv_usec;
	BK_DUMP_OUT("[en/dis security 0 time] >>>>> cost time: %d us.\r\n", tick_cnt);

	BK_DUMP_OUT("----- FLASH COUNT TIME TEST END -----\r\n");

	bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
	return kNoErr;
}

static void test_idle_read_flash(void *arg) {
	#if CONFIG_INT_WDT
	bk_wdt_stop();
	#endif
	#if (CONFIG_TASK_WDT)
	bk_task_wdt_stop();
	#endif
#if CONFIG_SOC_BK7236
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_120M);
#endif
	bk_logic_partition_t *ota_partition = bk_flash_partition_get_info(BK_PARTITION_OTA);
	uint32_t flash_size = bk_flash_get_current_total_size();
	if (ota_partition != NULL) {
		uint32_t ota_start_addr = ota_partition->partition_start_addr;
		uint32_t read_len = flash_size - ota_start_addr;
		BK_DUMP_OUT("ota_start_addr = 0x%08X\n", ota_start_addr);
		BK_DUMP_OUT("read_len = 0x%08X (%u bytes)\n", read_len, read_len);
		test_flash_read_without_print(ota_start_addr, read_len);
	}
	rtos_delete_thread(&idle_read_flash_handle);
}

#define write_data(addr,val)                    *((volatile unsigned long *)(addr)) = val
#define read_data(addr,val)                     val = *((volatile unsigned long *)(addr))
#define get_addr_data(addr)                     *((volatile unsigned long *)(addr))

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
bool s_is_cacheable = false;
bool s_task_stop = false;
#define PSRAM_DATA_ADDR     0x60000000
static void test_idle_read_psram(void *arg) {
	uint32_t i, val = 0;
	uint32_t buf_len = 512;

	BK_DUMP_OUT("enter test_idle_read_psram\r\n");

	uint32_t *test_data = (uint32_t *)psram_malloc(buf_len);
	if (test_data == NULL) {
		CLI_LOGE("test_data buffer malloc failed\r\n");
		return;
	}

	while (1) {
		if(s_is_cacheable == false) {
			for(i=0;i<1024;i++){
				write_data((PSRAM_DATA_ADDR+i*0x4),0x11+i);
				write_data((PSRAM_DATA_ADDR + 0x1000 +i*0x4),0x22+i);
				write_data((PSRAM_DATA_ADDR + 0x2000 +i*0x4),0x33+i);
				write_data((PSRAM_DATA_ADDR + 0x3000 +i*0x4),0x44+i);
			}
			for(i=0;i<1024;i++){
				write_data((PSRAM_DATA_ADDR + 0x4000 +i*0x4),0x55+i);
				write_data((PSRAM_DATA_ADDR + 0x5000 +i*0x4),0x66+i);
				write_data((PSRAM_DATA_ADDR + 0x6000 +i*0x4),0x77+i);
				write_data((PSRAM_DATA_ADDR + 0x7000 +i*0x4),0x88+i);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR+i*0x4);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR + 0x4000 +i*0x4);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR + i*0x4);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR + 0x4000 +i*0x4);
			}

			if(s_task_stop == true) {
				BK_DUMP_OUT("exit test_idle_read_psram non-cacheable.\r\n");
				break;
			}
		} else {
			for (i = 0; i < buf_len / 4; i++) {
				test_data[i] = 0x11223344 + i;
			}
			for (i = 0; i < buf_len / 4; i++) {
				val = test_data[i];
			}
			if(s_task_stop == true) {
				BK_DUMP_OUT("exit test_idle_read_psram cacheable, val = 0x%x\r\n", val);
				break;
			}
		}
	}

	if (test_data) {
		os_free(test_data);
	}
	test_data = NULL;

	if (idle_read_psram_handle) {
		BK_DUMP_OUT("=====idle_read_psram task stop end========\n");
		rtos_delete_thread(&idle_read_psram_handle);
		idle_read_psram_handle = NULL;
	}
}
#endif
static void flash_command_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	char cmd = 0;
	uint32_t len = 0;
	uint32_t addr = 0;

#if CONFIG_FLASH_UT_TEST
	if (argc >= 2 && os_strcmp(argv[1], "ut") == 0) {
		bk_err_t ret = flash_driver_test();

		if (ret == BK_OK) {
			BK_DUMP_OUT("flash_driver_test pass\r\n");
			msg = CLI_CMD_RSP_SUCCEED;
		} else {
			BK_DUMP_OUT("flash_driver_test fail, err=0x%x\r\n", ret);
			msg = CLI_CMD_RSP_ERROR;
		}
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}
#endif

#if (CONFIG_SYSTEM_CTRL)
	if (os_strcmp(argv[1], "config") == 0) {
		uint32_t target_clk_mhz = os_strtoul(argv[2], NULL, 10);
		uint32_t flash_line_mode = os_strtoul(argv[3], NULL, 10);
		uint32_t target_clk_hz = 0;
		bk_err_t config_ok = BK_OK;

		switch (target_clk_mhz) {
		case 26:
			target_clk_hz = FLASH_CLK_FREQ_26M_HZ;
			break;
		case 60:
			target_clk_hz = FLASH_CLK_FREQ_60M_HZ;
			break;
		case 80:
			target_clk_hz = FLASH_CLK_FREQ_80M_HZ;
			break;
		case 120:
			target_clk_hz = FLASH_CLK_FREQ_120M_HZ;
			break;
		default:
			BK_DUMP_OUT("Error: Unsupported clock frequency %u MHz. Supported: 26, 60, 80, 120\n", target_clk_mhz);
			config_ok = BK_FAIL;
			break;
		}

		if (config_ok == BK_OK) {
			bk_flash_set_clk_freq(target_clk_hz);
			extern bk_err_t bk_flash_switch_line_mode(flash_line_mode_t line_mode);
			if (flash_line_mode == 2) {
				bk_flash_switch_line_mode(FLASH_LINE_MODE_TWO);
				CLI_LOGI("switch to 2 line.\r\n");
			} else {
				bk_flash_switch_line_mode(FLASH_LINE_MODE_FOUR);
				CLI_LOGI("switch to 4 line.\r\n");
			}
			BK_DUMP_OUT("Flash clock configured: %u MHz\n", target_clk_mhz);
			BK_DUMP_OUT("flash_line_mode = %u\n", flash_line_mode);
		}

		return;
	}

//used for set flash status in itcm, not used now
#if 0
	if (os_strcmp(argv[1], "qe") == 0) {
		uint32_t param = 0;
		uint32_t quad_enable = os_strtoul(argv[2], NULL, 10);
		uint32_t delay_cycle1 = os_strtoul(argv[3], NULL, 10);
		uint32_t delay_cycle2 = os_strtoul(argv[4], NULL, 10);

		for(int i = 0; i< 20; i++) {
			bk_flash_set_line_mode(2);
			flash_bypass_quad_test(quad_enable, delay_cycle1, delay_cycle2);
			while (REG_READ(REG_FLASH_OPERATE_SW) & BUSY_SW);
			param = bk_flash_read_status_reg();
			if (quad_enable) {
				if(param & 0x200){
					break;
				} else {
					BK_DUMP_OUT("retry quad test, i = %d, flash status: 0x%x.\n", i, param);
				}
			} else {
				if(param & 0x200){
					BK_DUMP_OUT("retry quad test, i = %d, flash status: 0x%x.\n", i, param);
				} else {
					break;
				}
			}
		}

		if (quad_enable) {
			if(param & 0x200){
				BK_DUMP_OUT("flash quad enable success, flash status: 0x%x.\n", param);
			} else {
				BK_DUMP_OUT("flash quad enable fail, flash status: 0x%x.\n", param);
			}
		} else {
			if(param & 0x200){
				BK_DUMP_OUT("flash quad disable fail, flash status: 0x%x.\n", param);
			} else {
				BK_DUMP_OUT("flash quad disable success, flash status: 0x%x.\n", param);
			}
		}
		return;
	}
#endif
#endif
	if (os_strcmp(argv[1], "idle_read_start") == 0) {
		idle_read_flash_count = 0;
		rtos_create_thread(&idle_read_flash_handle,
			4,
			"idle_read_flash",
			(beken_thread_function_t) test_idle_read_flash,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);

		return;
	} else if (os_strcmp(argv[1], "idle_read_stop") == 0) {
		if (idle_read_flash_handle) {
			rtos_delete_thread(&idle_read_flash_handle);
			idle_read_flash_handle = NULL;
			BK_DUMP_OUT("idle_read_flash task stop, count = %u\n", (unsigned int)idle_read_flash_count);
		}
		return;
	}

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
	if (os_strcmp(argv[1], "idle_read_psram_start") == 0) {
		s_task_stop = false;
		uint32_t task_prio = os_strtoul(argv[2], NULL, 10);
		BK_DUMP_OUT("idle_read_psram task start: task_prio = %u.\n", task_prio);
		rtos_create_thread(&idle_read_psram_handle, task_prio,
			"idle_read_psram",
			(beken_thread_function_t) test_idle_read_psram,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);

		return;
	} else if (os_strcmp(argv[1], "idle_read_psram_stop") == 0) {
		s_task_stop = true;
		BK_DUMP_OUT("=====idle_read_psram task stop begin========\n");
		return;
	} else if (os_strcmp(argv[1], "psram_cache") == 0) {
		uint32_t psram_cache_flag = os_strtoul(argv[2], NULL, 10);
		if(psram_cache_flag == 1){
			s_is_cacheable = true;
			BK_DUMP_OUT("idle_read_psram switch to cacheable.\n");
		} else {
			s_is_cacheable = false;
			BK_DUMP_OUT("idle_read_psram switch to non-cacheable.\n");
		}
		return;
	}
#endif

	if (os_strcmp(argv[1], "U") == 0) {
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		return;
	} else if (os_strcmp(argv[1], "P") == 0) {
		bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		return;
	} else if (os_strcmp(argv[1], "RSR") == 0) {
#if (CONFIG_SYS_CPU0)
		uint16_t sts_val = bk_flash_read_status_reg();
		BK_DUMP_OUT("read sts_val = 0x%x\n", sts_val);
#else
		BK_DUMP_OUT("notice: this is cpu0 cmd only\n");
#endif
		return;
	} else if (os_strcmp(argv[1], "WSR") == 0) {
#if (CONFIG_SYS_CPU0)
		uint16_t sts_val = os_strtoul(argv[2], NULL, 16);
		bk_flash_write_status_reg(sts_val);
#else
		BK_DUMP_OUT("notice: this is cpu0 cmd only\n");
#endif
		return;
	} else if (os_strcmp(argv[1], "C") == 0) {
#if (CONFIG_SYS_CPU0)
		addr = os_strtoul(argv[2], NULL, 16);
		len = os_strtoul(argv[3], NULL, 16);
		uint32_t test_times = os_strtoul(argv[4], NULL, 10);
		test_flash_count_time(addr, len, test_times);
#else
		BK_DUMP_OUT("notice: this is cpu0 cmd only\n");
#endif
		return;
	}

	if (argc == 4) {
		cmd = argv[1][0];
		addr = os_strtoul(argv[2], NULL, 16);
		len = os_strtoul(argv[3], NULL, 16);

		switch (cmd) {
		case 'E':
			bk_flash_set_protect_type(FLASH_PROTECT_NONE);
			test_flash_erase(addr, len);
			bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
			msg = CLI_CMD_RSP_SUCCEED;
			break;

		case 'R':
			test_flash_read(addr, len);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'W':
			bk_flash_set_protect_type(FLASH_PROTECT_NONE);
			test_flash_write(addr, len);
			bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		//to check whether protection mechanism can work
		case 'N':
			test_flash_erase(addr, len);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'M':
			test_flash_write(addr, len);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'T':
			test_flash_read_time(addr, len);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		default:
			BK_DUMP_OUT("flash_test <R/W/E/M/N/T> <start_addr> <len>\r\n");
			msg = CLI_CMD_RSP_ERROR;
			break;
		}
	} else {
		BK_DUMP_OUT("flash_test <R/W/E/M/N/T> <start_addr> <len>\r\n");
		msg = CLI_CMD_RSP_ERROR;
	}
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#if (CONFIG_FLASH_BYPASS_OTP_OPERATION || CONFIG_FLASH_BYPASS_OTP_V2)
static uint8_t hex_char_to_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return (uint8_t)(c - '0');
	if (c >= 'a' && c <= 'f')
		return (uint8_t)(c - 'a' + 10);
	if (c >= 'A' && c <= 'F')
		return (uint8_t)(c - 'A' + 10);
	return 0;
}

static void hex_str_to_buf(const char *hex_str, uint8_t *buf, uint32_t len)
{
	for (uint32_t i = 0; i < len; i++)
		buf[i] = (uint8_t)(hex_char_to_nibble(hex_str[i * 2]) * 16 + hex_char_to_nibble(hex_str[i * 2 + 1]));
}
#endif


#if CONFIG_FLASH_BYPASS_OTP_OPERATION
static void flash_bypass_command_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_SUCCEED;
	uint8_t *buf = NULL;
	flash_bypass_otp_ctrl_t otp_op = {0};

	if ((argc >= 6) && (os_strcmp(argv[1], "otp") == 0)) {
		char cmd = argv[2][0];
		uint8_t idx = (uint8_t)os_strtoul(argv[3], NULL, 16);
		uint32_t offset = os_strtoul(argv[4], NULL, 16);
		uint32_t length = os_strtoul(argv[5], NULL, 16);
		int ret;
		if (argc == 7) {
			buf = (uint8_t *)os_malloc(length);
			if (buf)
				hex_str_to_buf(argv[6], buf, length);
		}

		otp_op.otp_idx = idx;
		otp_op.addr_offset = offset;
		otp_op.read_len = length;
		otp_op.read_buf = (uint8_t *)os_malloc(otp_op.read_len);
		if (otp_op.read_buf == NULL) {
			BK_DUMP_OUT("%s malloc err\r\n", __func__);
			msg = CLI_CMD_RSP_ERROR;
			goto flash_bypass_exit;
		}
		otp_op.write_len = length;
		otp_op.write_buf = buf;

		switch (cmd) {
		// flash_bypass_test otp E 1 0 0
		case 'E':
			ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_EARSE, &otp_op);
			if (ret != BK_OK) {
				BK_DUMP_OUT("%s fail\r\n", __func__);
				msg = CLI_CMD_RSP_ERROR;
				goto flash_bypass_exit;
			}
			break;

		// flash_bypass_test otp R 1 0 10
		case 'R':
			ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_READ, &otp_op);
			if (ret != BK_OK) {
				BK_DUMP_OUT("%s fail\r\n", __func__);
				msg = CLI_CMD_RSP_ERROR;
				goto flash_bypass_exit;
			}
			break;

		// flash_bypass_test otp W 1 0 4 11111111
		case 'W':
			ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_WRITE, &otp_op);
			if (ret != BK_OK) {
				BK_DUMP_OUT("%s fail\r\n", __func__);
				msg = CLI_CMD_RSP_ERROR;
				goto flash_bypass_exit;
			}
			break;

		// flash_bypass_test otp L 2 0 0
		case 'L':
			ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_LOCK, &otp_op);
			if (ret != BK_OK) {
				BK_DUMP_OUT("%s fail\r\n", __func__);
				msg = CLI_CMD_RSP_ERROR;
				goto flash_bypass_exit;
			}
			break;

		// flash_bypass_test otp U 2 0 a
		case 'U':
			ret = flash_bypass_otp_operation(FLASH_BYPASS_GET_UID, &otp_op);
			if (ret != BK_OK) {
				BK_DUMP_OUT("%s fail\r\n", __func__);
				msg = CLI_CMD_RSP_ERROR;
				goto flash_bypass_exit;
			}
			break;

		default:
			BK_DUMP_OUT("%d flash_bypass_test otp read/write/earse/lock\r\n", __LINE__);
			msg = CLI_CMD_RSP_ERROR;
			break;
		}
	} else {
		BK_DUMP_OUT("flash_bypass_test otp read/write/earse/lock\r\n");
		msg = CLI_CMD_RSP_ERROR;
	}

flash_bypass_exit:
	if (otp_op.read_buf != NULL) {
		os_free(otp_op.read_buf);
	}
	if (buf != NULL) {
		os_free(buf);
	}
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

#if CONFIG_FLASH_BYPASS_OTP_V2
static void flash_bypass_otp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern uint32_t bk_flash_get_crc_err_num(void);
	char *msg = CLI_CMD_RSP_ERROR;
	bk_err_t ret = BK_OK;
	flash_bypass_otp_err_t otp_ret = FLASH_BYPASS_OTP_OK;
	uint8_t idx;
	uint32_t offset, len;
	uint32_t crc_err_num_before = bk_flash_get_crc_err_num();
	uint32_t crc_err_num_after = crc_err_num_before;
	uint8_t *buf = NULL;

	if (argc < 3 || os_strcmp(argv[1], "otp") != 0) {
		ret = BK_ERR_PARAM;
		BK_DUMP_OUT("usage: otp test/R/W/E/L/RSR/WSR ...\r\n");
		goto out;
	}

	if (os_strcmp(argv[2], "test") == 0) {
		ret = flash_bypass_otp_test();
		msg = (ret == BK_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (argc >= 4 && os_strcmp(argv[2], "E") == 0) {
		idx = (uint8_t)os_strtoul(argv[3], NULL, 16);
		otp_ret = flash_bypass_otp_test_erase(idx);
		msg = (otp_ret == FLASH_BYPASS_OTP_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (argc >= 6 && os_strcmp(argv[2], "R") == 0) {
		idx = (uint8_t)os_strtoul(argv[3], NULL, 16);
		offset = os_strtoul(argv[4], NULL, 16);
		len = os_strtoul(argv[5], NULL, 16);
		if (len == 0 || len > 1024) {
			ret = BK_ERR_PARAM;
			BK_DUMP_OUT("len 1~1024\r\n");
			goto out;
		}
		buf = (uint8_t *)os_malloc(len);
		if (buf == NULL) {
			ret = BK_ERR_NO_MEM;
			BK_DUMP_OUT("malloc err\r\n");
			goto out;
		}
		os_memset(buf, 0, len);
		otp_ret = flash_bypass_otp_test_read(idx, offset, buf, len);
		if (otp_ret == FLASH_BYPASS_OTP_OK) {
			msg = CLI_CMD_RSP_SUCCEED;
			for (uint32_t i = 0; i < len; i++) {
				BK_DUMP_OUT("%02x ", buf[i]);
				if ((i + 1) % 16 == 0)
					BK_DUMP_OUT("\r\n");
			}
			if (len % 16 != 0)
				BK_DUMP_OUT("\r\n");
		}
		goto out;
	}

	if (argc >= 7 && os_strcmp(argv[2], "W") == 0) {
		idx = (uint8_t)os_strtoul(argv[3], NULL, 16);
		offset = os_strtoul(argv[4], NULL, 16);
		len = os_strtoul(argv[5], NULL, 16);
		if (len == 0 || len > 1024) {
			ret = BK_ERR_PARAM;
			BK_DUMP_OUT("len 1~1024\r\n");
			goto out;
		}
		if (os_strlen(argv[6]) < len * 2) {
			ret = BK_ERR_PARAM;
			BK_DUMP_OUT("hex len\r\n");
			goto out;
		}
		buf = (uint8_t *)os_malloc(len);
		if (buf == NULL) {
			ret = BK_ERR_NO_MEM;
			BK_DUMP_OUT("malloc err\r\n");
			goto out;
		}
		os_memset(buf, 0, len);
		hex_str_to_buf(argv[6], buf, len);
		otp_ret = flash_bypass_otp_test_write(idx, offset, buf, len);
		msg = (otp_ret == FLASH_BYPASS_OTP_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (argc >= 4 && os_strcmp(argv[2], "L") == 0) {
		idx = (uint8_t)os_strtoul(argv[3], NULL, 16);
		otp_ret = flash_bypass_otp_test_set_lock(idx);
		msg = (otp_ret == FLASH_BYPASS_OTP_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (argc >= 4 && os_strcmp(argv[2], "C") == 0) {
		bool locked = false;
		idx = (uint8_t)os_strtoul(argv[3], NULL, 16);
		otp_ret = flash_bypass_otp_test_is_lock(idx, &locked);
		if (otp_ret == FLASH_BYPASS_OTP_OK) {
			BK_DUMP_OUT("OTP%u lock: %s\r\n", idx, locked ? "LOCKED" : "UNLOCKED");
			msg = CLI_CMD_RSP_SUCCEED;
		} else {
			msg = CLI_CMD_RSP_ERROR;
		}
		goto out;
	}

	if (os_strcmp(argv[2], "RSR") == 0) {
		uint32_t sr_val = 0;
		otp_ret = flash_bypass_otp_test_read_sr(&sr_val);
		if (otp_ret == FLASH_BYPASS_OTP_OK) {
			BK_DUMP_OUT("SR 0x%08lx\r\n", (unsigned long)sr_val);
			msg = CLI_CMD_RSP_SUCCEED;
		}
		goto out;
	}

	if (argc >= 4 && os_strcmp(argv[2], "WSR") == 0) {
		uint32_t sr_val = os_strtoul(argv[3], NULL, 0);
		otp_ret = flash_bypass_otp_test_write_sr(sr_val);
		msg = (otp_ret == FLASH_BYPASS_OTP_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
		goto out;
	}

	ret = BK_ERR_PARAM;
	BK_DUMP_OUT("usage: otp test/R/W/E/L/RSR/WSR ...\r\n");
out:
	if (otp_ret != FLASH_BYPASS_OTP_OK)
		BK_DUMP_OUT("otp err code=%u\r\n", (uint32_t)otp_ret);
	crc_err_num_after = bk_flash_get_crc_err_num();
	BK_DUMP_OUT("flash crc err num before=%u after=%u\r\n",
		(unsigned int)crc_err_num_before, (unsigned int)crc_err_num_after);
	if (buf != NULL) {
		os_free(buf);
	}
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

#if CONFIG_FLASH_BYPASS_READ_UID
static void flash_bypass_read_uid_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_ERROR;
	bk_err_t ret = BK_OK;
	uint32_t len;
	uint8_t *buf = NULL;

	if (argc < 3 || os_strcmp(argv[1], "uid") != 0) {
		ret = BK_ERR_PARAM;
		BK_DUMP_OUT("usage: uid <len>\r\n");
		goto out;
	}

	len = os_strtoul(argv[2], NULL, 16);
	if (len == 0 || len > 16) {
		ret = BK_ERR_PARAM;
		BK_DUMP_OUT("len 1~16\r\n");
		goto out;
	}

	buf = (uint8_t *)os_malloc(len);
	if (buf == NULL) {
		ret = BK_ERR_NO_MEM;
		BK_DUMP_OUT("malloc err\r\n");
		goto out;
	}
	os_memset(buf, 0, len);

	extern bk_err_t bk_flash_switch_line_mode(flash_line_mode_t line_mode);

	if (bk_flash_get_line_mode() == FLASH_LINE_MODE_FOUR) {
		bk_flash_switch_line_mode(FLASH_LINE_MODE_TWO);
	}

	ret = flash_bypass_read_uid(buf, len);

	if (bk_flash_get_line_mode() == FLASH_LINE_MODE_FOUR) {
		bk_flash_switch_line_mode(FLASH_LINE_MODE_FOUR);
	}

	if (ret == BK_OK) {
		msg = CLI_CMD_RSP_SUCCEED;
		for (uint32_t i = 0; i < len; i++) {
			BK_DUMP_OUT("%02x ", buf[i]);
			if ((i + 1) % 16 == 0)
				BK_DUMP_OUT("\r\n");
		}
		if (len % 16 != 0)
			BK_DUMP_OUT("\r\n");
	}

out:
	if (buf != NULL) {
		os_free(buf);
	}
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

static void partShow_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_partition_t i;
	bk_logic_partition_t *partition;

	for (i = BK_PARTITION_BOOTLOADER; i <= BK_PARTITIONS_TABLE_SIZE; i++) {
		partition = bk_flash_partition_get_info(i);
		if (partition == NULL)
			continue;

		BK_DUMP_OUT("%4d | %11s |  Dev:%d  | 0x%08lx | 0x%08lx |\r\n", i,
				  partition->partition_description, partition->partition_owner,
				  partition->partition_start_addr, partition->partition_length);
	};

}

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
static const struct cli_command s_flash_commands[] = {
	{"fmap_test",    "flash_test memory map",      partShow_Command},
	{"flash_test",   "flash_test <cmd(R/W/E/N/ut)>", flash_command_test},
#if CONFIG_FLASH_BYPASS_OTP_OPERATION
	{"flash_bypass_test",   "flash_bypass_test otp R/W/E/L", flash_bypass_command_test},
#elif CONFIG_FLASH_BYPASS_OTP_V2
	{"flash_bypass_test",   "flash_bypass_test otp test/R/W/E/L/RSR/WSR ...", flash_bypass_otp_test_cmd},
#elif CONFIG_FLASH_BYPASS_READ_UID
	{"flash_bypass_test",   "flash_bypass_test uid <len>", flash_bypass_read_uid_cmd},
#endif

#if CONFIG_FLASH_RACE_TEST
	{"flash_race_test", "Flash race test suite", cli_flash_race_test_cmd},
#endif
};

int cli_flash_test_init(void)
{
	return cli_register_commands(s_flash_commands, FLASH_CMD_CNT);
}
