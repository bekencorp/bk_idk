/**
 ******************************************************************************
 * @file    BkDriverFlash.h
 * @brief   This file provides all the headers of Flash operation functions.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2017 BEKEN Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */
#include <common/bk_include.h>
#include <os/os.h>
#include "BkDriverFlash.h"
#include "bk_flash.h"
#include "bk_drv_model.h"
#include <common/bk_kernel_err.h>
#include "bk_uart.h"
#include <os/mem.h>
/* Logic partition on flash devices */

#ifdef CONFIG_SOC_BK7256XX
extern const bk_logic_partition_t bk7256_partitions[BK_PARTITION_MAX];
#else
const bk_logic_partition_t bk7231_partitions[BK_PARTITION_MAX] = {
	[BK_PARTITION_BOOTLOADER] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "Bootloader",
		.partition_start_addr      = 0x00000000,
		.partition_length          = 0x0F000,
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
	[BK_PARTITION_APPLICATION] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "Application",
		.partition_start_addr      = 0x11000,
#if CONFIG_SUPPORT_MATTER	|| 	 CONFIG_FLASH_SIZE_4M               
		.partition_length          = 0x1A9000,
#else
		.partition_length          = 0x143000,
#endif
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
	[BK_PARTITION_OTA] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "ota",
#if CONFIG_FLASH_SIZE_4M 
		.partition_start_addr      = 0x1BA000,
		.partition_length          = 0x1A9000, //1700KB
#elif CONFIG_SUPPORT_MATTER
		.partition_start_addr      = 0x1BA000,
		.partition_length          = 0x11000, //68KB
#else
		.partition_start_addr      = 0x132000,
		.partition_length          = 0xAE000, //696KB
#endif		
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
#if CONFIG_SUPPORT_MATTER
	[BK_PARTITION_MATTER_FLASH] =
	{
		.partition_owner		   = BK_FLASH_EMBEDDED,
		.partition_description	   = "Matter",
		#if CONFIG_FLASH_SIZE_4M 
		.partition_start_addr	   = 0x363000,
		#else
		partition_start_addr	   = 0x1CB000,
		#endif
		.partition_length		   = 0x15000, //84KB
		.partition_options		   = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
#endif
	[BK_PARTITION_RF_FIRMWARE] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "RF Firmware",
#if (CONFIG_FLASH_SIZE_4M)
		.partition_start_addr      = 0x3FE000,
#else 
		.partition_start_addr      = 0x1e0000,// bootloader unused space for rf cal+mac related info.
#endif
		.partition_length          = 0x1000,
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
	[BK_PARTITION_NET_PARAM] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "NET info",
#if (CONFIG_FLASH_SIZE_4M)
		.partition_start_addr      = 0x3FF000,
#else 
		.partition_start_addr      = 0x1e1000,// for net related info
#endif
		.partition_length          = 0x1000,
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
};
#endif

static void BkFlashPartitionAssert(bk_partition_t inPartition)
{
	BK_ASSERT(BK_PARTITION_BOOTLOADER < BK_PARTITION_MAX);
}

static uint32_t BkFlashPartitionIsValid(bk_partition_t inPartition)
{
	if ((inPartition >= BK_PARTITION_BOOTLOADER) && (inPartition < BK_PARTITION_MAX))
		return 1;
	else
		return 0;
}

bk_logic_partition_t *bk_flash_get_info(bk_partition_t inPartition)
{
	bk_logic_partition_t *pt = NULL;

	BkFlashPartitionAssert(inPartition); /* ASSERT VERIFIED */

	if (BkFlashPartitionIsValid(inPartition)){
#if (CONFIG_SOC_BK7256XX)
		pt = (bk_logic_partition_t *)&bk7256_partitions[inPartition];
#else
		pt = (bk_logic_partition_t *)&bk7231_partitions[inPartition];
#endif
		}
	else
		pt =	NULL;
	return pt;
}

bk_err_t BkFlashInit(void)
{
	return 0;
}

bk_err_t BkFlashUninit(void)
{
	return 0;
}

bk_err_t bk_flash_erase(bk_partition_t inPartition, uint32_t off_set, uint32_t size)
{
	uint32_t i;
	uint32_t param;
	UINT32 status;
	DD_HANDLE flash_hdl;
	uint32_t start_sector, end_sector;
	bk_logic_partition_t *partition_info;
	GLOBAL_INT_DECLARATION();

	partition_info = bk_flash_get_info(inPartition);
	start_sector = off_set >> 12;
	end_sector = (off_set + size - 1) >> 12;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	BK_ASSERT(DD_HANDLE_UNVALID != flash_hdl); /* ASSERT VERIFIED */
	for (i = start_sector; i <= end_sector; i ++) {
		param = partition_info->partition_start_addr + (i << 12);
		GLOBAL_INT_DISABLE();
		ddev_control(flash_hdl, CMD_FLASH_ERASE_SECTOR, (void *)&param);
		GLOBAL_INT_RESTORE();
	}

	return kNoErr;
}

bk_err_t bk_flash_write(bk_partition_t inPartition, volatile uint32_t off_set, uint8_t *inBuffer, uint32_t inBufferLength)
{
	UINT32 status;
	DD_HANDLE flash_hdl;
	uint32_t start_addr;
	bk_logic_partition_t *partition_info;
	GLOBAL_INT_DECLARATION();

	if (NULL == inBuffer) {
		os_printf("%s inBuffer=NULL\r\n", __FUNCTION__);
		return kParamErr;
	}

	partition_info = bk_flash_get_info(inPartition);
	if (NULL == partition_info) {
		os_printf("%s partiion not found\r\n", __FUNCTION__);
		return kNotFoundErr;
	}

	start_addr = partition_info->partition_start_addr + off_set;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	if (DD_HANDLE_UNVALID == flash_hdl) {
		os_printf("%s open failed\r\n", __FUNCTION__);
		return kOpenErr;
	}

	GLOBAL_INT_DISABLE();
	ddev_write(flash_hdl, (char *)inBuffer, inBufferLength, start_addr);
	GLOBAL_INT_RESTORE();

	return kNoErr;
}

bk_err_t bk_flash_read(bk_partition_t inPartition, volatile uint32_t off_set, uint8_t *outBuffer, uint32_t inBufferLength)
{
	UINT32 status;
	uint32_t start_addr;
	DD_HANDLE flash_hdl;
	bk_logic_partition_t *partition_info;
	GLOBAL_INT_DECLARATION();

	if (NULL == outBuffer) {
		os_printf("%s outBuffer=NULL\r\n", __FUNCTION__);
		return kParamErr;
	}

	partition_info = bk_flash_get_info(inPartition);
	if (NULL == partition_info) {
		os_printf("%s partiion not found\r\n", __FUNCTION__);
		return kNotFoundErr;
	}

	start_addr = partition_info->partition_start_addr + off_set;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	if (DD_HANDLE_UNVALID == flash_hdl) {
		os_printf("%s open failed\r\n", __FUNCTION__);
		return kOpenErr;
	}

	GLOBAL_INT_DISABLE();
	ddev_read(flash_hdl, (char *)outBuffer, inBufferLength, start_addr);
	GLOBAL_INT_RESTORE();

	return kNoErr;
}

bk_err_t bk_flash_enable_security(PROTECT_TYPE type)
{
	DD_HANDLE flash_hdl;
	UINT32 status;
	uint32_t param = type;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	if (DD_HANDLE_UNVALID == flash_hdl) {
		os_printf("%s open failed\r\n", __FUNCTION__);
		return kOpenErr;
	}
	ddev_control(flash_hdl, CMD_FLASH_SET_PROTECT, (void *)&param);

	return kNoErr;
}


bk_err_t bk_flash_get_security(PROTECT_TYPE *protect_flag)
{
	DD_HANDLE flash_hdl;
	UINT32 status;
	uint32_t param;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	ddev_control(flash_hdl, CMD_FLASH_GET_PROTECT, &param);
	*protect_flag = param;

	return kNoErr;
}

bk_err_t bk_flash_abs_addr_read(unsigned int off_set, unsigned char *outBuffer, unsigned int size)
{
	UINT32 status;
	DD_HANDLE flash_hdl;
	GLOBAL_INT_DECLARATION();

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);

	GLOBAL_INT_DISABLE();
	ddev_read(flash_hdl, (char *)outBuffer, size, off_set);
	GLOBAL_INT_RESTORE();

	return kNoErr;
}

bk_err_t bk_flash_abs_addr_erase(unsigned int flashOffset, unsigned int size)
{
	UINT32 status, i;
	DD_HANDLE flash_hdl;
	UINT32 start_sector, end_sector, param;
	GLOBAL_INT_DECLARATION();

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);

	start_sector = flashOffset >> 12;
	end_sector = (flashOffset + size - 1) >> 12;

	GLOBAL_INT_DISABLE();
	for (i = start_sector; i <= end_sector; i ++) {
		param = i << 12;
		ddev_control(flash_hdl, CMD_FLASH_ERASE_SECTOR, (void *)&param);
	}
	GLOBAL_INT_RESTORE();

	return kNoErr;
}

bk_err_t bk_flash_abs_addr_write(unsigned int off_set, const unsigned char *inBuffer, unsigned int size, unsigned char eraseflag)
{
	UINT32 status;
	DD_HANDLE flash_hdl;
	GLOBAL_INT_DECLARATION();

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);

	if (eraseflag)
		bk_flash_abs_addr_erase(off_set, size);

	GLOBAL_INT_DISABLE();
	ddev_write(flash_hdl, (char *)inBuffer, size, off_set);
	GLOBAL_INT_RESTORE();

	return kNoErr;
}

bk_err_t test_flash_write(volatile uint32_t start_addr, uint32_t len)
{
	UINT32 status;
	DD_HANDLE flash_hdl;
	uint32_t i;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	uint32_t tmp = addr + length;

	for (i = 0; i < 256; i++)
		buf[i] = i;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	BK_ASSERT(DD_HANDLE_UNVALID != flash_hdl); /* ASSERT VERIFIED */
	for (; addr < tmp; addr += 256) {
		os_printf("write addr(size:256):%d\r\n", addr);
		ddev_write(flash_hdl, (char *)buf, 256, addr);
	}

	return kNoErr;
}
bk_err_t test_flash_erase(volatile uint32_t start_addr, uint32_t len)
{
	UINT32 status;
	DD_HANDLE flash_hdl;

	uint32_t addr = start_addr;
	uint32_t length = len;
	uint32_t tmp = addr + length;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	if (DD_HANDLE_UNVALID == flash_hdl) {
		os_printf("%s open failed\r\n", __FUNCTION__);
		return kOpenErr;
	}
	for (; addr < tmp; addr += 0x1000) {
		os_printf("erase addr:%d\r\n", addr);
		ddev_control(flash_hdl, CMD_FLASH_ERASE_SECTOR, (void *)&addr);

	}
	return kNoErr;
}

bk_err_t test_flash_read(volatile uint32_t start_addr, uint32_t len)
{
	UINT32 status;
	DD_HANDLE flash_hdl;
	uint32_t i, j, tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	tmp = addr + length;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	if (DD_HANDLE_UNVALID == flash_hdl) {
		os_printf("%s open failed\r\n", __FUNCTION__);
		return kOpenErr;
	}
	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		ddev_read(flash_hdl, (char *)buf, 256, addr);
		os_printf("read addr:%x\r\n", addr);
		for (i = 0; i < 16; i++) {
			for (j = 0; j < 16; j++)
				os_printf("%02x ", buf[i * 16 + j]);
			os_printf("\r\n");
		}
	}

	return kNoErr;
}

bk_err_t test_flash_read_without_print(volatile uint32_t start_addr, uint32_t len)
{
	UINT32 status;
	DD_HANDLE flash_hdl;
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	tmp = addr + length;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	if (DD_HANDLE_UNVALID == flash_hdl) {
		os_printf("%s open failed\r\n", __FUNCTION__);
		return kOpenErr;
	}
	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		ddev_read(flash_hdl, (char *)buf, 256, addr);
	}

	return kNoErr;
}

bk_err_t test_flash_read_time(volatile uint32_t start_addr, uint32_t len)
{
	UINT32 status, time_start, time_end;
	DD_HANDLE flash_hdl;
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;

	tmp = addr + length;

	flash_hdl = ddev_open(DD_DEV_TYPE_FLASH, &status, 0);
	if (DD_HANDLE_UNVALID == flash_hdl) {
		os_printf("%s open failed\r\n", __FUNCTION__);
		return kOpenErr;
	}
	beken_time_get_time((beken_time_t *)&time_start);
	os_printf("read time start:%d\r\n", time_start);

	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		ddev_read(flash_hdl, (char *)buf, 256, addr);
	}
	beken_time_get_time((beken_time_t *)&time_end);
	os_printf("read time end:%d\r\n", time_end);
	os_printf("cost time:%d\r\n", time_end - time_start);

	return kNoErr;
}

static beken_mutex_t hal_flash_mutex;

int hal_flash_lock(void)
{
	rtos_lock_mutex(&hal_flash_mutex);
	return kNoErr;
}

int hal_flash_unlock(void)
{
	rtos_unlock_mutex(&hal_flash_mutex);
	return kNoErr;
}


int hal_flash_init(void)
{
	int ret = 0;

	ret = rtos_init_mutex(&hal_flash_mutex);
	if (ret != 0)
		return kGeneralErr;
	return kNoErr;
}

// EOF

