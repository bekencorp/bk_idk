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
#include <os/mem.h>
#include <driver/efuse.h>
#include "efuse_driver.h"
#include "efuse_hal.h"
#include "sys_driver.h"

typedef struct {
	efuse_hal_t hal;
} efuse_driver_t;

#define EFUSE_ADDR_MAX_VALUE    EFUSE_F_ADDR_V
#define EFUSE_USE_OTP_REPLACEMENT    0

#define EFUSE_INIT_IF_UNINITIALIZED() do {\
		if (!s_efuse_driver_is_init) {\
			bk_err_t ret = bk_efuse_driver_init();\
			if(BK_OK != ret){\
				return ret;\
			}\
		}\
	} while(0)

#define EFUSE_RETURN_ON_ADDR_OUT_OF_RANGE(addr) do {\
		if ((addr) > EFUSE_ADDR_MAX_VALUE) {\
			return BK_ERR_EFUSE_ADDR_OUT_OF_RANGE;\
		}\
	} while(0)

static void efuse_deinit_common(void)
{

}

bk_err_t bk_efuse_driver_init(void)
{
	return BK_OK;
}

bk_err_t bk_efuse_driver_deinit(void)
{
	return BK_OK;
}

extern bk_err_t bk_otp_efuse_write_32bit(uint32_t* values);
extern bk_err_t bk_otp_efuse_read_32bit(uint32_t* values);
bk_err_t bk_efuse_write_byte(uint8_t addr, uint8_t data)
{
#if CONFIG_OTP_EFUSE_SUPPORT
    bk_err_t ret;
    uint32_t offset = (uint32_t)addr % 4;
    uint32_t value_buffer[1];
    uint32_t value_to_write;

    ret = bk_otp_efuse_read_32bit(value_buffer);
    if (ret != BK_OK) {
        return BK_ERR_EFUSE_READ_FAIL;
    }

    value_to_write = value_buffer[0];
    switch (offset) {
        case 0:
            value_to_write = (value_to_write & 0xFFFFFF00) | (uint32_t)data;
            break;
        case 1:
            value_to_write = (value_to_write & 0xFFFF00FF) | ((uint32_t)data << 8);
            break;
        case 2:
            value_to_write = (value_to_write & 0xFF00FFFF) | ((uint32_t)data << 16);
            break;
        case 3:
            value_to_write = (value_to_write & 0x00FFFFFF) | ((uint32_t)data << 24);
            break;
        default:
            return BK_ERR_PARAM;
    }

    return bk_otp_efuse_write_32bit(&value_to_write);
#else
    return BK_OK;
#endif
}

bk_err_t bk_efuse_read_byte(uint8_t addr, uint8_t *data)
{
#if CONFIG_OTP_EFUSE_SUPPORT
    bk_err_t ret;
    uint32_t value_buffer[1];

    if (data == NULL) {
        return BK_ERR_PARAM;
    }
    ret = bk_otp_efuse_read_32bit(value_buffer);
    if (ret != BK_OK) {
        return BK_ERR_EFUSE_READ_FAIL;
    }
    switch (addr) {
        case 0:
            *data = (value_buffer[0] >> 0) & 0xFF;
            break;
        case 1:
            *data = (value_buffer[0] >> 8) & 0xFF;
            break;
        case 2:
            *data = (value_buffer[0] >> 16) & 0xFF;
            break;
        case 3:
            *data = (value_buffer[0] >> 24) & 0xFF;
            break;
        default:
            return BK_ERR_PARAM;
    }
#endif
    return BK_OK;
}

