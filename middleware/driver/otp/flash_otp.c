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

#include <common/bk_include.h>
#include <string.h>
#include "sys_hal.h"
#include <driver/flash.h>
#include "flash_otp.h"
#include "CheckSumUtils.h"
#include "_otp.c"

static flash_otp_config_t s_flash_otp_config;
static bool flash_otp_is_inited;

static int switch_map()
{
	if(otp_map == NULL) {
		if (s_flash_otp_config.flash_otp_section_id == 1) {
			otp_map = otp_map_1;
		} else if (s_flash_otp_config.flash_otp_section_id == 2) {
			otp_map = otp_map_2;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
	return 0;
}

static int flash_otp_on()
{
    return 0;
}

static int flash_otp_off()
{
    return 0;
}

static int otp_init()
{
	if (flash_otp_is_inited != true) {
		return -1;
	}
	int ret = switch_map();
	if(ret != 0) {
		return ret;
	}
	ret = flash_otp_on();
	if(ret != 0) {
		flash_otp_off();
		otp_map = NULL;
		return -1;
	}
	return ret;
}

static int otp_deinit()
{
	flash_otp_off();
	otp_map = NULL;
	return 0;
}
extern void delay_us(UINT32 us);
static uint32_t _otp_read(uint32_t location)
{
	uint32_t address = s_flash_otp_config.flash_otp_operate_addr + 4 * location;
	uint32_t value  = (*(volatile uint32_t *)(address));
	delay_us(100);
	return value;
}

static void _otp_write(uint32_t location, uint32_t value)
{
	uint32_t address = s_flash_otp_config.flash_otp_operate_addr + 4 * location;
	(*(volatile uint32_t *)(address)) = (value);
	delay_us(100);
}

static bool otp_check_crc8(uint8_t* buf, size_t buflen)
{
	CRC8_Context crc8;
	uint8_t cal_value;
	uint8_t store_value = buf[buflen - 1];
	CRC8_Init(&crc8);
	CRC8_Update(&crc8, buf, buflen - 1);
	CRC8_Final(&crc8, &cal_value);
	if (cal_value == store_value) {
		return true;
	} else {
		return false;
	}
}

static void otp_fill_crc8(uint8_t* buf, size_t buflen)
{
	CRC8_Context crc8;
	uint8_t fill_value;
	CRC8_Init(&crc8);
	CRC8_Update(&crc8, buf, buflen - 1);
	CRC8_Final(&crc8, &fill_value);
	buf[buflen - 1] = fill_value;
}

static bk_err_t otp_read(uint8_t item, uint8_t* buf, uint32_t size)
{
	bk_err_t ret = BK_OK;
	if (otp_init() != 0) {
		return BK_FAIL;
	}

	uint8_t* read_buf = NULL;
	uint8_t* p_read_buf = NULL;
	uint32_t location = otp_map[item].offset / 4;
	uint32_t start = otp_map[item].offset % 4;
	uint32_t value;
	uint32_t read_size;
	uint32_t read_buf_len;

	if (size > otp_map[item].allocated_size) {
		ret =  BK_ERR_OTP_ADDR_OUT_OF_RANGE;
		goto end;
	}

	if (otp_map[item].crc_en == OTP_NEED_CRC) {
		read_size = otp_map[item].allocated_size;
	} else if (otp_map[item].crc_en == OTP_NO_NEED_CRC) {
		read_size = size;
	} else {
		ret = BK_ERR_OTP_CRC_WRONG;
		goto end;
	}
	if (read_size == 0) {
		ret = BK_ERR_OTP_INDEX_WRONG;
		goto end;
	}
	read_buf_len = read_size;
	read_buf = (uint8_t*)malloc(read_buf_len);
	memset(read_buf, 0, read_buf_len);
	if (read_buf == NULL) {
		ret = BK_ERR_NO_MEM;
		goto end;
	}
	p_read_buf = read_buf;

	while (read_size > 0) {
		value = _otp_read(location);

		uint8_t * src_data = (uint8_t *)&value;
		int       cpy_cnt;

		src_data += start;

		cpy_cnt = (read_size >= (4 - start)) ? (4 - start) : read_size;

		switch ( cpy_cnt ) {
			case 4:
				*p_read_buf++ = *src_data++;
			case 3:
				*p_read_buf++ = *src_data++;
			case 2:
				*p_read_buf++ = *src_data++;
			case 1:
				*p_read_buf++ = *src_data++;
		}

		read_size -= cpy_cnt;
		location++;
		start = 0;
	}

	if (otp_map[item].crc_en == OTP_NEED_CRC) {
		uint8_t* ff_buf = malloc(read_buf_len);
		memset(ff_buf, 0xFF, read_buf_len);
		if (memcmp(read_buf, ff_buf, read_buf_len) != 0) {
			if (otp_check_crc8(read_buf, read_buf_len) == false) {
				free(ff_buf);
				ret = BK_ERR_OTP_CRC_WRONG;
				goto end;
			}
		}
		free(ff_buf);
	}

	memcpy(buf, read_buf, size);
end:
	if (read_buf != NULL) {
		free(read_buf);
	}
	otp_deinit();
	return ret;
}

static bk_err_t otp_update(uint8_t item, uint8_t* buf, uint32_t size)
{
	bk_err_t ret = BK_OK;
	if (otp_init() != 0) {
		return BK_FAIL;
	}

	if (size > otp_map[item].allocated_size || size == 0) {
		ret = BK_ERR_OTP_ADDR_OUT_OF_RANGE;
		goto end;
	}
	if (otp_map[item].crc_en == OTP_NEED_CRC) {
		if (size != otp_map[item].allocated_size && buf[size - 1] != 0xFF) {
			ret = BK_ERR_OTP_CRC_WRONG;
			goto end;
		}
		otp_fill_crc8(buf, otp_map[item].allocated_size);
	}
	uint32_t location = otp_map[item].offset / 4;
	uint32_t start = otp_map[item].offset % 4;
	uint32_t value = 0;
	uint32_t check_value = 0;

	uint8_t * dst_data = NULL;
	uint8_t * back_buf = buf;
	uint32_t  back_size = size;

	while(size > 0) {
		value = _otp_read(location);

		int       cmp_cnt = (size >= (4 - start)) ? (4 - start) : size;
		uint32_t  cmp_result = 0;

		dst_data = (uint8_t *)&value;
		dst_data += start;

		/* the initial data of every FLASH OTP memory bit is 1. */
		/* FLASH OTP memory bit can be changed to 0, */
		/* once it is changed to 0, it can't be reset to 1. */
		/* so check the target bit value before write to OTP memory. */
		/* if try to change OTP bit from 0 to 1, return fail. */
		switch( cmp_cnt ) {
			case 4:
				cmp_result |= ((*buf) & ~(*dst_data));
				buf++; dst_data++;
			case 3:
				cmp_result |= ((*buf) & ~(*dst_data));
				buf++; dst_data++;
			case 2:
				cmp_result |= ((*buf) & ~(*dst_data));
				buf++; dst_data++;
			case 1:
				cmp_result |= ((*buf) & ~(*dst_data));
				buf++; dst_data++;
		}

		if (cmp_result != 0)
			break;

		size -= cmp_cnt;
		location++;
		start = 0;
	}

	if(size > 0) {
		ret = BK_ERR_OTP_UPDATE_NOT_EQUAL;
		goto end;
	}

	/* restore all variables. */
	location = otp_map[item].offset / 4;
	start = otp_map[item].offset % 4;
	size = back_size;
	buf = back_buf;

	while(size > 0) {
		int cpy_cnt = (size >= (4 - start)) ? (4 - start) : size;

		value = _otp_read(location);
		dst_data = (uint8_t *)&value;
		dst_data += start;

		switch( cpy_cnt ) {
			case 4:
				*dst_data++ = *buf++;
			case 3:
				*dst_data++ = *buf++;
			case 2:
				*dst_data++ = *buf++;
			case 1:
				*dst_data++ = *buf++;
		}

		_otp_write(location, value);
		check_value = _otp_read(location);
		if (check_value != value) {
			ret = BK_ERR_OTP_UPDATE_NOT_EQUAL;
			goto end;
		}

		size -= cpy_cnt;
		location++;
		start = 0;
	}

end:
	otp_deinit();
	return ret;
}

bk_err_t bk_flash_otp_update(uint32_t item_id, uint8_t* buf, size_t size)
{
	uint8_t line_mode;
	bk_flash_lock( &line_mode);
	bk_err_t ret = otp_update(item_id, buf, size);
	bk_flash_unlock(&line_mode);
	return ret;
}

bk_err_t bk_flash_otp_read(uint32_t item_id, uint8_t* buf, size_t size)
{
	uint8_t line_mode;
	bk_flash_lock(&line_mode);
	bk_err_t ret = otp_read(item_id, buf, size);
	bk_flash_unlock(&line_mode);
	return ret;
}

void bk_flash_otp_init(uint8_t section_id)
{
	//decided by flash id
    s_flash_otp_config.efstoflash_otp_load = 0;
    s_flash_otp_config.efstoflash_otp_shaddr = 0; // value has benn set in efuse
	s_flash_otp_config.flash_otp_section_id = section_id;
	if (section_id == 1) {
		s_flash_otp_config.flash_otp_software_offset = 0x1000;
	} else if (section_id == 2) {
		s_flash_otp_config.flash_otp_software_offset = 0x2000;
	} else if (section_id == 3) {
		s_flash_otp_config.flash_otp_software_offset = 0x3000;
	}
	
	s_flash_otp_config.flash_otp_operate_addr = SOC_FLASH_OTP_REG_BASE + 
									s_flash_otp_config.flash_otp_software_offset;
	flash_otp_is_inited = true;

}