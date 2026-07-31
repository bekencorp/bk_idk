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
#include <components/ate.h>
#include <os/mem.h>
#include <driver/flash.h>
#include <os/os.h>
#include "bk_pm_model.h"
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#include "cache.h"
#include "partitions_gen.h"
#if CONFIG_BL2_WDT
#include "wdt_hal.h"
#endif


#if CONFIG_FLASH_QUAD_ENABLE
//#include "flash_bypass.h"
extern UINT8 flash_get_line_mode(void);
extern void flash_set_line_mode(UINT8 mode);
#endif

typedef struct {
	flash_hal_t hal;
	uint32_t flash_id;
	const flash_config_t *flash_cfg;
} flash_driver_t;

#define FLASH_GET_PROTECT_CFG(cfg) ((cfg) & FLASH_STATUS_REG_PROTECT_MASK)
#define FLASH_GET_CMP_CFG(cfg)     (((cfg) >> FLASH_STATUS_REG_PROTECT_OFFSET) & FLASH_STATUS_REG_PROTECT_MASK)

#define FLASH_RETURN_ON_DRIVER_NOT_INIT() do {\
	if (!s_flash_is_init) {\
		return BK_ERR_FLASH_NOT_INIT;\
	}\
} while(0)

#define FLASH_RETURN_ON_WRITE_ADDR_OUT_OF_RANGE(addr, len) do {\
	if ((addr >= s_flash.flash_cfg->flash_size) ||\
		(len > s_flash.flash_cfg->flash_size) ||\
		((addr + len) > s_flash.flash_cfg->flash_size)) {\
		FLASH_LOGW("write error[addr:0x%x len:0x%x]\r\n", addr, len);\
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;\
	}\
} while(0)

static const flash_config_t flash_config[] = {
	/* flash_id, status_reg_size, flash_size,    line_mode,           cmp_post, protect_post, protect_mask, protect_all, protect_none, protect_half, unprotect_last_block. quad_en_post, quad_en_val, coutinuous_read_mode_bits_val, mode_sel*/
	{0x1C7016,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x16,         0x01B,                0,            0,           0xA5,                          0x01}, //en_25qh32b
	{0x1C7015,   1,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x0d,         0x0d,                 0,            0,           0xA5,                          0x01}, //en_25qh16b
	{0x0B4014,   2,               FLASH_SIZE_1M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0C,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_25f08b
	{0x0B4015,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_25f16b
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,           1,            0xA0,                          0x02}, //xtx_25f32b
#else
	{0x0B4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_25f32b
#endif
	{0x0B4017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x05,        0x00,         0x0E,         0x109,                9,            1,           0xA0,                          0x01}, //xtx_25f64b
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B6017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 0,        2,            0x0F,         0x0F,        0x00,         0x0E,         0x00E,                9,            1,           0xA0,                          0x02}, //xt_25q64d
#else
	{0x0B6017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO,  0,        2,            0x0F,         0x0F,        0x00,         0x0E,         0x00E,                0,            0,           0xA0,                          0x01}, //xt_25q64d
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B6018,   2,               FLASH_SIZE_16M, FLASH_LINE_MODE_FOUR,  0,	    2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                9,            1,           0xA0,                          0x02}, //xt_25q128d
#else
	{0x0B6018,   1,               FLASH_SIZE_16M, FLASH_LINE_MODE_TWO,   0,     2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //xt_25q128d
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B4018,   2,               FLASH_SIZE_16M, FLASH_LINE_MODE_FOUR, 0,       2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                9,            1,           0xA0,                          0x02}, //xt_25F128F-W
#else
	{0x0B4018,   1,               FLASH_SIZE_16M, FLASH_LINE_MODE_TWO,  0,       2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //xt_25F128F-W
#endif
	{0x0E4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_FT25H32
	{0x1C4116,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                0,            0,           0xA0,                          0x01}, //en_25qe32a(not support 4 line)
	{0x5E5018,   1,               FLASH_SIZE_16M, FLASH_LINE_MODE_TWO, 0, 	    2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //zb_25lq128c
	{0xC84015,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //gd_25q16c
	{0xC84017,   1,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //gd_25q16c
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC84016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                9,            1,           0xA0,                          0x02}, //gd_25q32c
#else
	{0xC84016,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                0,            0,           0xA0,                          0x01}, //gd_25q32c
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86018,   2,               FLASH_SIZE_16M,FLASH_LINE_MODE_FOUR, 0,       2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                9,            1,           0xA0,                          0x02}, //gd_25lq128e
#else
	{0xC86018,   1,               FLASH_SIZE_16M,FLASH_LINE_MODE_TWO,  0,       2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //gd_25lq128e
#endif
	{0xC86515,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //gd_25w16e
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86516,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x09,         0x0E,         0x1F,                 9,            1,           0xA0,                          0x02}, //gd_25wq32e
#else
	{0xC86516,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x09,         0x0E,         0x1F,                 0,            0,           0xA0,                          0x01}, //gd_25wq32e
#endif
	{0xEF4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x00,         0x101,                9,            1,           0xA0,                          0x01}, //w_25q32(bfj)
#if CONFIG_FLASH_QUAD_ENABLE
	{0x204118,	 2, 			  FLASH_SIZE_16M,FLASH_LINE_MODE_FOUR, 0,		2,			  0x0F, 		0x0F,		 0x00,		   0x0A,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //xm_25qu128c
#else
	{0x204118,	 1, 			  FLASH_SIZE_16M,FLASH_LINE_MODE_TWO,  0,		2,			  0x0F, 		0x0F,		 0x00,		   0x0A,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //xm_25qu128c
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0x204017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x02}, //xmc_25qh64d
#else
	{0x204017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xmc_25qh64d
#endif
	{0x204016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xmc_25qh32b
	{0xC22315,   1,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 0,        2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                6,            1,           0xA5,                          0x01}, //mx_25v16b
	{0xEB6015,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //zg_th25q16b
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86517,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //gd_25Q32E
#else
	{0xC86517,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //gd_25Q32E
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xCD6016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x09,         0x0E,         0x1F,                 9,            1,           0xA0,                          0x02}, //th_25q32ub
#else
	{0xCD6016,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x09,         0x0E,         0x1F,                 0,            0,           0xA0,                          0x01}, //th_25q32ub
#endif
	
#if CONFIG_FLASH_QUAD_ENABLE
	{0xCD6017,   3,               FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                9,            1,           0xA0,                          0x02}, //th_25q64ha
#else
	{0xCD6017,   3,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                9,            1,           0xA0,                          0x01}, //th_25q64ha
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0x856017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                9,            1,           0xA0,                          0x02}, //p_25q64su
#else
	{0x856017,   1,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO,  0,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                0,            0,           0xA0,                          0x01}, //p_25q64su
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xCD7017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR,14,       2,            0x1F,         0x1F,        0x00,         0x0E,          0x1F,                9,            1,           0xA0,                          0x02}, //th_25q64ub
#else
	{0xCD7017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO,0,         2,            0x1F,         0x1F,        0x00,         0x0E,          0x1F,                0,            0,           0xA0,                          0x01}, //th_25q64ub
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B7517,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR,14,       2,            0x1F,         0x1F,        0x00,         0x0E,          0x1F,                9,            1,           0xA0,                          0x02}, //xtd_25w64a
#else
	{0x0B7517,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO,0,         2,            0x1F,         0x1F,        0x00,         0x0E,          0x1F,                0,            0,           0xA0,                          0x01}, //xtd_25w64a
#endif
	{0x000000,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x00,        0x00,         0x00,         0x000,                0,            0,           0x00,                          0x01}, //default
};
static flash_driver_t s_flash = {0};
static bool s_flash_is_init = false;
static beken_mutex_t s_flash_mutex = NULL;
#if (CONFIG_SOC_BK7256XX)
static uint32_t s_hold_low_speed_status = 0;
#endif
#define FLASH_MAX_WAIT_CB_CNT (4)
static flash_wait_callback_t s_flash_wait_cb[FLASH_MAX_WAIT_CB_CNT] = {NULL};
#if CONFIG_FLASH_WRITE_32B_WITH_READ_BACK || CONFIG_FLASH_WRITE_SOFT_ERASE_OPTIM_ENABLE
static bool s_is_puya_flash = false;
#endif

bk_err_t bk_flash_register_wait_cb(flash_wait_callback_t wait_cb)
{
	uint32_t i = 0;

	for(i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++)
	{
		if(s_flash_wait_cb[i] == NULL)
		{
			s_flash_wait_cb[i] = wait_cb;
			break;
		}
	}

	if(i == FLASH_MAX_WAIT_CB_CNT)
	{
		FLASH_LOGE("cb is full\r\n");
		return BK_ERR_FLASH_WAIT_CB_FULL;
	}

	return BK_OK;
}

bk_err_t bk_flash_unregister_wait_cb(flash_wait_callback_t wait_cb)
{
	uint32_t i = 0;

	for(i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++)
	{
		if(s_flash_wait_cb[i] == wait_cb)
		{
			s_flash_wait_cb[i] = NULL;
			break;
		}
	}

	if(i == FLASH_MAX_WAIT_CB_CNT)
	{
		FLASH_LOGE("cb isn't registered\r\n");
		return BK_ERR_FLASH_WAIT_CB_NOT_REGISTER;
	}

	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) void flash_waiting_cb(void)
{
	uint32_t i = 0;

	for(i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++)
	{
		if(s_flash_wait_cb[i])
		{
			s_flash_wait_cb[i]();
		}
	}
}

void flash_switch_to_line_mode_two(void)
{
	if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
}

void flash_restore_line_mode(void)
{
	if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
		bk_flash_set_line_mode(FLASH_LINE_MODE_FOUR);
}

static void flash_init_common(void)
{
	int ret = rtos_init_mutex(&s_flash_mutex);
	BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
}

static void flash_deinit_common(void)
{
	int ret = rtos_deinit_mutex(&s_flash_mutex);
	BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
}

static void flash_get_current_config(void)
{
	bool cfg_success = false;

	for (uint32_t i = 0; i < (ARRAY_SIZE(flash_config) - 1); i++) {
		if (s_flash.flash_id == flash_config[i].flash_id) {
			s_flash.flash_cfg = &flash_config[i];
			cfg_success = true;
			break;
		}
	}

	if (!cfg_success) {
		s_flash.flash_cfg = &flash_config[ARRAY_SIZE(flash_config) - 1];
		for(int i = 0; i < 10; i++) {
			FLASH_LOGE("This flash is not identified, choose default config\r\n");
		}
	}
}

static uint32_t flash_get_protect_cfg(flash_protect_type_t type)
{
	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_none);
	case FLASH_PROTECT_ALL:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_all);
	case FLASH_PROTECT_HALF:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_half);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->unprotect_last_block);
	default:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_all);
	}
}

static void flash_set_protect_cfg(uint32_t *status_reg_val, uint32_t new_protect_cfg)
{
	*status_reg_val &= ~(s_flash.flash_cfg->protect_mask << s_flash.flash_cfg->protect_post);
	*status_reg_val |= ((new_protect_cfg & s_flash.flash_cfg->protect_mask) << s_flash.flash_cfg->protect_post);
}

static uint32_t flash_get_cmp_cfg(flash_protect_type_t type)
{
	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_none);
	case FLASH_PROTECT_ALL:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_all);
	case FLASH_PROTECT_HALF:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_half);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->unprotect_last_block);
	default:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_all);
	}
}

static void flash_set_cmp_cfg(uint32_t *status_reg_val, uint32_t new_cmp_cfg)
{
	*status_reg_val &= ~(FLASH_CMP_MASK << s_flash.flash_cfg->cmp_post);
	*status_reg_val |= ((new_cmp_cfg & FLASH_CMP_MASK) << s_flash.flash_cfg->cmp_post);
}

static bool flash_is_need_update_status_reg(uint32_t protect_cfg, uint32_t cmp_cfg, uint32_t status_reg_val)
{
	uint32_t cur_protect_val_in_status_reg = (status_reg_val >> s_flash.flash_cfg->protect_post) & s_flash.flash_cfg->protect_mask;
	uint32_t cur_cmp_val_in_status_reg = (status_reg_val >> s_flash.flash_cfg->cmp_post) & FLASH_CMP_MASK;

	if (cur_protect_val_in_status_reg != protect_cfg ||
		cur_cmp_val_in_status_reg != cmp_cfg) {
		return true;
	} else {
		return false;
	}
}

static flash_protect_type_t flash_get_protect_type(uint32_t status_reg)
{
	uint16_t protect_value;

	protect_value = (status_reg >> s_flash.flash_cfg->protect_post) & s_flash.flash_cfg->protect_mask;
	if (protect_value == s_flash.flash_cfg->protect_all)
		return FLASH_PROTECT_ALL;
	else if (protect_value == s_flash.flash_cfg->protect_none)
		return FLASH_PROTECT_NONE;
	else if (protect_value == s_flash.flash_cfg->protect_half)
		return FLASH_PROTECT_HALF;
	else if (protect_value == s_flash.flash_cfg->unprotect_last_block)
		return FLASH_UNPROTECT_LAST_BLOCK;
	else
		return FLASH_PROTECT_NONE;
}

static void flash_write_status_reg(uint32_t status_reg_val)
{
#if CONFIG_FLASH_WRITE_STATUS_VOLATILE
	flash_hal_set_volatile_status_write(&s_flash.hal);
#endif

	flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg_val);

#if CONFIG_FLASH_WRITE_STATUS_VOLATILE
	flash_hal_clear_volatile_status_write(&s_flash.hal);
#endif
}

static void flash_set_protect_type(flash_protect_type_t type)
{
	uint32_t protect_cfg = flash_get_protect_cfg(type);
	uint32_t cmp_cfg = flash_get_cmp_cfg(type);
	uint32_t status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);

	if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
		flash_set_protect_cfg(&status_reg, protect_cfg);
		flash_set_cmp_cfg(&status_reg, cmp_cfg);

		FLASH_LOGD("write status reg:%x, status_reg_size:%d\r\n", status_reg, s_flash.flash_cfg->status_reg_size);
		flash_write_status_reg(status_reg);
	}
}

static void flash_set_qe(void)
{
	uint32_t status_reg;

	flash_hal_wait_op_done(&s_flash.hal);

	status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	if (status_reg & (s_flash.flash_cfg->quad_en_val << s_flash.flash_cfg->quad_en_post)) {
		return;
	}

	status_reg |= s_flash.flash_cfg->quad_en_val << s_flash.flash_cfg->quad_en_post;
	flash_write_status_reg(status_reg);
}

static void flash_clear_qe(void)
{
	uint32_t status_reg;

	flash_hal_wait_op_done(&s_flash.hal);

	status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	if (!(status_reg & (s_flash.flash_cfg->quad_en_val << s_flash.flash_cfg->quad_en_post))) {
		return;
	}

	status_reg &= ~(s_flash.flash_cfg->quad_en_val << s_flash.flash_cfg->quad_en_post);
	flash_write_status_reg(status_reg);
}

static void flash_set_qwfr(void)
{
	flash_hal_set_mode(&s_flash.hal, s_flash.flash_cfg->mode_sel);
}

static bool flash_is_hw_sr_protect_active(uint32_t status_reg)
{
	return (status_reg & FLASH_SR_SRP0_BIT) && !(status_reg & FLASH_SR_SRP1_BIT);
}

static void flash_apply_hw_sr_protect(void)
{
	uint32_t status_reg;
	uint8_t sr_width = s_flash.flash_cfg->status_reg_size;

	if (sr_width < 2) {
		return;
	}

	flash_switch_to_line_mode_two();

	flash_hal_wait_op_done(&s_flash.hal);
	status_reg = flash_hal_read_status_reg(&s_flash.hal, sr_width);

	if (status_reg & FLASH_SR_SRP1_BIT) {
		FLASH_LOGW("flash sr locked (SRP1=1), skip hw_sr_protect\r\n");
		goto restore_line_mode;
	}

	/* Hardware protect per datasheet: SRP1=0, SRP0=1, WP#=0 */
	if (flash_is_hw_sr_protect_active(status_reg)) {
		flash_hal_set_wp_value(&s_flash.hal, 0);
		goto restore_line_mode;
	}

	status_reg |= FLASH_SR_SRP0_BIT;
	status_reg &= ~FLASH_SR_SRP1_BIT;

	flash_hal_set_wp_value(&s_flash.hal, 1);
	flash_hal_write_enable(&s_flash.hal);

	flash_write_status_reg(status_reg);

	status_reg = flash_hal_read_status_reg(&s_flash.hal, sr_width);
	if (!flash_is_hw_sr_protect_active(status_reg)) {
		FLASH_LOGW("flash hw_sr_protect verify failed, sr=0x%x\r\n", status_reg);
		goto restore_line_mode;
	}

restore_line_mode:
	flash_restore_line_mode();
}

static void flash_read_common(uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t buf[FLASH_BUFFER_LEN] = {0};
	uint8_t *pb = (uint8_t *)&buf[0];

	if (len == 0) {
		return;
	}

	while (len) {
		uint32_t int_level = rtos_disable_int();
		flash_hal_wait_op_done(&s_flash.hal);

		flash_hal_set_op_cmd_read(&s_flash.hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash.hal);
		}
		rtos_enable_int(int_level);

		for (uint32_t i = address % FLASH_BYTES_CNT; i < FLASH_BYTES_CNT; i++) {
			*buffer++ = pb[i];
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}
	}
}

static void flash_read_word_common(uint32_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t buf[FLASH_BUFFER_LEN] = {0};
	//nt8_t *pb = (uint8_t *)&buf[0];
	uint32_t *pb = (uint32_t *)&buf[0];

	if (len == 0) {
		return;
	}

	while (len) {
		uint32_t int_level = rtos_disable_int();

		flash_hal_wait_op_done(&s_flash.hal);

		flash_hal_set_op_cmd_read(&s_flash.hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash.hal);
		}

		rtos_enable_int(int_level);

		for (uint32_t i = address % (FLASH_BYTES_CNT/4); i < (FLASH_BYTES_CNT/4); i++) {
			*buffer++ = pb[i];
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}
	}
}

#if 0
//extern part_flag update_part_flag;
bool flash_is_area_write_disable(uint32_t addr)
{
	uint32_t firmware_area_end_address = 0;
	bk_logic_partition_t * flash_pt = NULL;

	flash_pt = bk_flash_partition_get_info(BK_PARTITION_BOOTLOADER);
	if(!flash_pt)
	{
		FLASH_LOGE("get partition ota fail\r\n");
		return true;
	}
	firmware_area_end_address = flash_pt->partition_start_addr + flash_pt->partition_length;
	if (addr < firmware_area_end_address) {
		FLASH_LOGE("valid write/erase start address = 0x%x, but current address = 0x%x.\r\n", firmware_area_end_address, addr);
		BK_ASSERT(addr >= firmware_area_end_address);
		return true;
	}
	return false;
}
#else
bool flash_is_area_write_disable(uint32_t addr)
{
	return false;
}
#endif

static bk_err_t flash_32b_write_program(const uint8_t *buf, uint32_t addr)
{
	const uint32_t *wbuf = (const uint32_t *)buf;
	uint32_t int_level = rtos_disable_int();
	flash_hal_wait_op_done(&s_flash.hal);
	flash_hal_write_enable(&s_flash.hal);
	for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
		flash_hal_write_data(&s_flash.hal, wbuf[i]);
	}
	flash_hal_set_op_cmd_write(&s_flash.hal, addr);
	rtos_enable_int(int_level);

	return BK_OK;
}

static bk_err_t flash_write_common_32b(const uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t buf[FLASH_BUFFER_LEN];
	uint8_t *pb = (uint8_t *)&buf[0];
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);

	FLASH_RETURN_ON_WRITE_ADDR_OUT_OF_RANGE(addr, len);

	while (len) {
#if CONFIG_FLASH_WRITE_32B_WITH_READ_BACK
		if (s_is_puya_flash) {
			flash_read_common(pb, addr, FLASH_BYTES_CNT);
		} else {
			os_memset(pb, 0xFF, FLASH_BYTES_CNT);
		}
#else
		os_memset(pb, 0xFF, FLASH_BYTES_CNT);
#endif
		for (uint32_t i = address % FLASH_BYTES_CNT; i < FLASH_BYTES_CNT; i++) {
			pb[i] = *buffer++;
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}

		flash_32b_write_program(pb, addr);

		addr += FLASH_BYTES_CNT;
	}
	return BK_OK;
}

void flash_lock(void)
{
	rtos_lock_mutex(&s_flash_mutex);
}

void flash_unlock(void)
{
	rtos_unlock_mutex(&s_flash_mutex);
}

static void flash_enable_line_mode_switch(bool enable)
{
	if (enable) {
		sys_drv_set_sys2flsh_2wire(0);
	} else {
		sys_drv_set_sys2flsh_2wire(1);
	}
}

bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode)
{
	flash_hal_clear_qwfr(&s_flash.hal);
	flash_enable_line_mode_switch(true);
	if (FLASH_LINE_MODE_TWO == line_mode) {
		if (s_flash.flash_cfg != NULL && (1 == s_flash.flash_cfg->quad_en_val)) {
			flash_clear_qe();
		}
		flash_hal_set_dual_mode(&s_flash.hal);
	} else if (FLASH_LINE_MODE_FOUR == line_mode) {
		flash_hal_set_quad_m_value(&s_flash.hal, s_flash.flash_cfg->coutinuous_read_mode_bits_val);
		if (s_flash.flash_cfg != NULL && (1 == s_flash.flash_cfg->quad_en_val)) {
			flash_set_qe();
		}
		flash_set_qwfr();
	}
	flash_enable_line_mode_switch(false);
	return BK_OK;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_cpu_write_enable(void)
{
        flash_hal_enable_cpu_data_wr(&s_flash.hal);
        return BK_OK;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_cpu_write_disable(void)
{
        flash_hal_disable_cpu_data_wr(&s_flash.hal);
        return BK_OK;
}

bk_err_t bk_flash_driver_init(void)
{
	if (s_flash_is_init) {
		return BK_OK;
	}
#if CONFIG_FLASH_QUAD_ENABLE
#if CONFIG_FLASH_ORIGIN_API
	if (FLASH_LINE_MODE_FOUR == flash_get_line_mode())
		flash_set_line_mode(FLASH_LINE_MODE_TWO);
#endif
#endif
	os_memset(&s_flash, 0, sizeof(s_flash));
	flash_hal_init(&s_flash.hal);
	bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
	s_flash.flash_id = flash_hal_get_id(&s_flash.hal);
	FLASH_LOGI("id=0x%x\r\n", s_flash.flash_id);
	flash_get_current_config();
	flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
#if CONFIG_FLASH_WRITE_32B_WITH_READ_BACK || CONFIG_FLASH_WRITE_SOFT_ERASE_OPTIM_ENABLE
	s_is_puya_flash = (s_flash.flash_id == FLASH_ID_P25Q64SU);
#endif
#if (0 == CONFIG_JTAG)
	flash_hal_disable_cpu_data_wr(&s_flash.hal);
#endif
	bk_flash_set_line_mode(s_flash.flash_cfg->line_mode);
	flash_hal_set_default_clk(&s_flash.hal);
	flash_apply_hw_sr_protect();

#if (CONFIG_SOC_BK7256XX)
	#if CONFIG_ATE_TEST
	bk_flash_clk_switch(FLASH_SPEED_LOW, 0);
	#else
	bk_flash_clk_switch(FLASH_SPEED_HIGH, 0);
	#endif
#endif



	flash_enable_line_mode_switch(false);

	flash_init_common();
	s_flash_is_init = true;

	return BK_OK;
}

bk_err_t bk_flash_driver_deinit(void)
{
	if (!s_flash_is_init) {
		return BK_OK;
	}
	flash_deinit_common();
	s_flash_is_init = false;

	return BK_OK;
}
#if CONFIG_FLASH_PAGE_WRITE_ENABLE
static bk_err_t flash_page_write_program(const uint8_t *buf, uint32_t addr)
{
	flash_hal_wait_op_done(&s_flash.hal);
	flash_hal_set_pw_write(&s_flash.hal, 1U);

	uint32_t int_level = rtos_disable_int();
	flash_hal_wait_op_done(&s_flash.hal);
	flash_hal_set_page_write_mem_addr_clr(&s_flash.hal, 1U);
	for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
		flash_hal_set_page_write_mem_data(&s_flash.hal, buf[i]);
	}
	flash_hal_set_op_cmd_write(&s_flash.hal, addr);
	flash_hal_wait_op_done(&s_flash.hal);
	rtos_enable_int(int_level);

	flash_hal_set_pw_write(&s_flash.hal, 0U);
	return BK_OK;
}

#if CONFIG_OTA_DBUS_WRITE

static struct {
	bool pg_cached;
	uint32_t pg_addr;
	uint8_t pg_buf[FLASH_PAGE_SIZE];
} s_ota_dbus_pg;

static void ota_dbus_page_flush(void)
{
	if (!s_ota_dbus_pg.pg_cached) {
		return;
	}
	if (flash_page_write_program(s_ota_dbus_pg.pg_buf, s_ota_dbus_pg.pg_addr) != BK_OK) {
		FLASH_LOGE("puya page program failed, addr=0x%x\r\n", s_ota_dbus_pg.pg_addr);
	}
	s_ota_dbus_pg.pg_cached = false;
}

static void dbus_page_merge_buf(uint32_t phy_addr, const uint8_t *data, uint32_t len)
{
	while (len > 0U) {
		uint32_t page_addr = phy_addr & ~(FLASH_PAGE_SIZE - 1U);
		uint32_t page_off = phy_addr & (FLASH_PAGE_SIZE - 1U);
		uint32_t page_remain = FLASH_PAGE_SIZE - page_off;

		if (page_remain > len) {
			page_remain = len;
		}

		if (!s_ota_dbus_pg.pg_cached || s_ota_dbus_pg.pg_addr != page_addr) {
			ota_dbus_page_flush();
			s_ota_dbus_pg.pg_addr = page_addr;
			/* OTA target is erased before write; skip read, use 0xFF baseline for merge */
			os_memset(s_ota_dbus_pg.pg_buf, 0xFF, FLASH_PAGE_SIZE);
			s_ota_dbus_pg.pg_cached = true;
		}

		for (uint32_t i = 0; i < page_remain; i++) {
			s_ota_dbus_pg.pg_buf[page_off + i] &= data[i];
		}

		phy_addr += page_remain;
		data += page_remain;
		len -= page_remain;

		if (page_off + page_remain == FLASH_PAGE_SIZE) {
			ota_dbus_page_flush();
		}
	}
}

static bk_err_t write_flash_with_dbus_page(uint32_t phy_addr, const uint8_t *data, uint32_t phy_len)
{
	uint32_t wr_phy = phy_addr;
	flash_protect_type_t protect_type;
	bk_err_t ret = BK_OK;

	if (data == NULL || phy_len == 0U || (phy_len % 34U) != 0U) {
		return BK_ERR_PARAM;
	}

	FLASH_RETURN_ON_DRIVER_NOT_INIT();
	FLASH_RETURN_ON_WRITE_ADDR_OUT_OF_RANGE(phy_addr, phy_len);

	protect_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	flash_switch_to_line_mode_two();

	s_ota_dbus_pg.pg_cached = false;
	for (uint32_t off = 0; off < phy_len; off += 34U) {
#if CONFIG_BL2_WDT
		if ((off & 0x1FFFU) == 0U) {
			BL2_WDT_FEED();
		}
#endif
		dbus_page_merge_buf(wr_phy, data + off, 34U);
		wr_phy += 34U;
	}
	ota_dbus_page_flush();

	bk_flash_set_protect_type(protect_type);
	flash_restore_line_mode();
	return ret;
}
#endif

#endif

#if CONFIG_FLASH_WRITE_SOFT_ERASE_OPTIM_ENABLE
#define FLASH_PAGE_WRITE_MIN_LEN             4U
#define FLASH_PAGE_LAST_32B_OFF              (FLASH_PAGE_SIZE - FLASH_BYTES_CNT)

static bk_err_t flash_page_write_empty_check(const uint8_t *page_buf, uint32_t check_off, uint32_t check_len)
{
	for (uint32_t i = 0; i < check_len; i++) {
		if (page_buf[check_off + i] != 0xFFU) {
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static void flash_page_write_merge(uint8_t *page_buf, uint32_t page_off, const uint8_t *buffer, uint32_t merge_len)
{
	for (uint32_t i = 0; i < merge_len; i++) {
		page_buf[page_off + i] &= buffer[i];
	}
}

static bk_err_t flash_page_write_with_32b(const uint8_t *page_buf, uint32_t page_addr, uint32_t page_off, uint32_t write_len)
{
	uint32_t end_off = page_off + write_len;
	uint32_t cur_off = page_off;

	while (cur_off < end_off) {
		uint32_t write_off = cur_off;

		if ((write_off + FLASH_BYTES_CNT) > FLASH_PAGE_SIZE) {
			write_off = FLASH_PAGE_LAST_32B_OFF;
		}
		bk_err_t ret = flash_32b_write_program(&page_buf[write_off], page_addr + write_off);
		if (ret != BK_OK) {
			return ret;
		}
		if (write_off == FLASH_PAGE_LAST_32B_OFF) {
			break;
		}
		cur_off = write_off + FLASH_BYTES_CNT;
	}

	return BK_OK;
}

static bk_err_t flash_write_soft_erase_optim(const uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint8_t check_buf[FLASH_PAGE_SIZE];
	uint8_t expect_buf[FLASH_PAGE_SIZE];
	uint32_t remain = len;
	bk_err_t ret;

	FLASH_RETURN_ON_WRITE_ADDR_OUT_OF_RANGE(address, len);

	if (len < FLASH_PAGE_WRITE_MIN_LEN) {
		FLASH_LOGW("write error:len %u < %u\r\n", len, FLASH_PAGE_WRITE_MIN_LEN);
		return BK_FAIL;
	}

	while (remain > 0U) {
		uint32_t page_addr = address & ~(FLASH_PAGE_SIZE - 1U);
		uint32_t page_off = address - page_addr;
		uint32_t chunk_len = FLASH_PAGE_SIZE - page_off;

		if (chunk_len > remain) {
			chunk_len = remain;
		}

		bool use_page_program = false;

		if ((page_off == 0U) && (chunk_len == FLASH_PAGE_SIZE)) {
			use_page_program = true;
		}

		uint32_t check_off = page_off;
		uint32_t check_len = chunk_len;

		if (use_page_program) {
			check_off = 0U;
			check_len = FLASH_PAGE_SIZE;
		}

		os_memset(expect_buf, 0, FLASH_PAGE_SIZE);
		flash_read_common(expect_buf, page_addr, FLASH_PAGE_SIZE);
		ret = flash_page_write_empty_check(expect_buf, check_off, check_len);
		if (ret != BK_OK) {
			FLASH_LOGW("write error:not erased, addr:0x%x len:0x%x\r\n",
				page_addr + check_off, check_len);
			return ret;
		}

		flash_page_write_merge(expect_buf, page_off, buffer, chunk_len);

#if CONFIG_FLASH_PAGE_WRITE_ENABLE
		if (use_page_program) {
			ret = flash_page_write_program(expect_buf, page_addr);
		} else {
			ret = flash_page_write_with_32b(expect_buf, page_addr, page_off, chunk_len);
		}
#else
		ret = flash_page_write_with_32b(expect_buf, page_addr, page_off, chunk_len);
#endif
		if (ret != BK_OK) {
			return ret;
		}

		os_memset(check_buf, 0, FLASH_PAGE_SIZE);
		flash_read_common(check_buf, page_addr, FLASH_PAGE_SIZE);
		if (os_memcmp(check_buf, expect_buf, FLASH_PAGE_SIZE) != 0) {
			FLASH_LOGW("flash page:0x%x verify fail\r\n", page_addr);
			return BK_FAIL;
		}

		buffer += chunk_len;
		address += chunk_len;
		remain -= chunk_len;
	}

	return BK_OK;
}
#endif

static bk_err_t flash_write_common(const uint8_t *buffer, uint32_t address, uint32_t len)
{
#if CONFIG_FLASH_WRITE_SOFT_ERASE_OPTIM_ENABLE
	if (s_is_puya_flash) {
		return flash_write_soft_erase_optim(buffer, address, len);
	}
#endif
	return flash_write_common_32b(buffer, address, len);
}

static bk_err_t flash_write_no_lock(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	bk_err_t ret = BK_FAIL;
	flash_protect_type_t protect_type = bk_flash_get_protect_type();

	if (bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK) {
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);

		if (bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK) {
			flash_switch_to_line_mode_two();
			ret = flash_write_common(user_buf, address, size);
		}
	}
	bk_flash_set_protect_type(protect_type);
	flash_restore_line_mode();

	return ret;
}

static bk_err_t flash_erase_block(uint32_t erase_addr, int type)
{
	FLASH_LOGI("erase: %x.%d\r\n", erase_addr, type);
	flash_switch_to_line_mode_two();
	if (erase_addr >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", erase_addr);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (flash_is_area_write_disable(erase_addr)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	uint32_t erase_size = 0;

	if (type == FLASH_OP_CMD_SE) {
		erase_size = FLASH_SECTOR_SIZE;
	} else if (type == FLASH_OP_CMD_BE1) {
		erase_size = FLASH_BLOCK32_SIZE;
	} else if (type == FLASH_OP_CMD_BE2) {
		erase_size = FLASH_BLOCK_SIZE;
	} else {
		return BK_FAIL;
	}

	bk_err_t ret_val = BK_FAIL;
	flash_protect_type_t protect_type = bk_flash_get_protect_type();

	if (bk_flash_partition_write_perm_check_by_addr(erase_addr, erase_size, FLASH_API_MAGIC_CODE) == BK_OK) {
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);

		if (bk_flash_partition_write_perm_check_by_addr(erase_addr, erase_size, FLASH_API_MAGIC_CODE) == BK_OK) {
			uint32_t int_level = rtos_disable_int();
#if CONFIG_FLASH_QUAD_ENABLE
			flash_switch_to_line_mode_two();
#endif
			flash_hal_wait_op_done(&s_flash.hal);
			flash_hal_write_enable(&s_flash.hal);
			flash_hal_erase_block(&s_flash.hal, erase_addr, type);
			rtos_enable_int(int_level);
			ret_val = BK_OK;
		}
	}
	bk_flash_set_protect_type(protect_type);
	flash_restore_line_mode();

	return ret_val;
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_erase_sector(uint32_t address)
{
	return flash_erase_block(address & FLASH_ERASE_SECTOR_MASK, FLASH_OP_CMD_SE);
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_erase_block_64k(uint32_t address)
{
	return flash_erase_block(address & FLASH_ERASE_BLOCK_64K_MASK, FLASH_OP_CMD_BE2);
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_erase_block_32k(uint32_t address)
{
	return flash_erase_block(address & FLASH_ERASE_BLOCK_32K_MASK, FLASH_OP_CMD_BE1);
}

bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("read error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	flash_read_common(user_buf, address, size);

	return BK_OK;
}

bk_err_t bk_flash_read_word(uint32_t address, uint32_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("read error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	flash_read_word_common(user_buf, address, size);

	return BK_OK;
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (flash_is_area_write_disable(address)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

#if CONFIG_OTA_DBUS_WRITE && CONFIG_FLASH_PAGE_WRITE_ENABLE
	/* Puya: 256B page program, source is 34B units with CRC in payload */
	if (s_flash.flash_id == FLASH_ID_P25Q64SU && (size % 34U) == 0U) {
		return write_flash_with_dbus_page(address, user_buf, size);
	}
#endif

#if CONFIG_BL2_WDT
	if (s_flash.flash_id == FLASH_ID_P25Q64SU && size > FLASH_SECTOR_SIZE) {
		uint32_t off = 0;

		while (off < size) {
			uint32_t chunk = size - off;
			bk_err_t ret;

			if (chunk > FLASH_SECTOR_SIZE) {
				chunk = FLASH_SECTOR_SIZE;
			}
			BL2_WDT_FEED();
			ret = flash_write_no_lock(address + off, user_buf + off, chunk);
			if (ret != BK_OK) {
				return ret;
			}
			off += chunk;
		}
		return BK_OK;
	}
#endif
	return flash_write_no_lock(address, user_buf, size);
}

#if !CONFIG_SECUREBOOT
__attribute__((section(".iram")))
static void *flash_memcpy(void *d, const void *s, size_t n)
{
        /* attempt word-sized copying only if buffers have identical alignment */

        unsigned char *d_byte = (unsigned char *)d;
        const unsigned char *s_byte = (const unsigned char *)s;
        const uint32_t mask = sizeof(uint32_t) - 1;

        if ((((uint32_t)d ^ (uint32_t)s_byte) & mask) == 0) {

                /* do byte-sized copying until word-aligned or finished */

                while (((uint32_t)d_byte) & mask) {
                        if (n == 0) {
                                return d;
                        }
                        *(d_byte++) = *(s_byte++);
                        n--;
                };

                /* do word-sized copying as long as possible */

                uint32_t *d_word = (uint32_t *)d_byte;
                const uint32_t *s_word = (const uint32_t *)s_byte;

                while (n >= sizeof(uint32_t)) {
                        *(d_word++) = *(s_word++);
                        n -= sizeof(uint32_t);
                }

                d_byte = (unsigned char *)d_word;
                s_byte = (unsigned char *)s_word;
        }

        /* do byte-sized copying until finished */

        while (n > 0) {
                *(d_byte++) = *(s_byte++);
                n--;
        }

        return d;
}
#endif

__attribute__((section(".iram")))
void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
	flash_switch_to_line_mode_two();
	flash_memcpy((char*)user_buf, (const char*)(0x02000000+address),size);
	flash_restore_line_mode();
}


__attribute__((section(".iram")))
void bk_flash_read_cbus_xip(uint32_t slot, uint32_t address, void *user_buf, uint32_t size)
{
	uint32_t old_excute_enable = flash_get_excute_enable();
	uint32_t new_excute_enable = (slot == 1) ? 1 : 0;
	uint8_t dummy;

	enable_dcache(0);

	if (old_excute_enable != new_excute_enable) {
		bk_flash_read_cbus(0, &dummy, 1);  //Read a different address to flush flash FIFO
		flash_set_excute_enable(new_excute_enable);
	}

	bk_flash_read_cbus(address, user_buf, size);

	if (old_excute_enable != new_excute_enable) {
		bk_flash_read_cbus(0, &dummy, 1);  //Read a different address to flush flash FIFO
		flash_set_excute_enable(old_excute_enable);
	}
	enable_dcache(1);
}

#define CEIL_ALIGN_34(addr) (((addr) + 34 - 1) / 34 * 34)

__attribute__((section(".iram")))
bool is_addr_write_primary_cbus(uint32_t address, uint32_t size)
{
	uint32_t primary_vir_start = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(get_flash_map_offset(0)));
	uint32_t primary_vir_end = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(get_flash_map_offset(0) + get_flash_map_phy_size(0)) - 34);
	if (address >= primary_vir_start && address + size <= primary_vir_end) {
		return true;
	} else {
		return false;
	}
}

__attribute__((section(".iram")))
void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	flash_protect_type_t protect_type;
	uint16_t status_reg;

	flash_switch_to_line_mode_two();

	status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	protect_type = flash_get_protect_type(status_reg);

	if (bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK) {
		flash_set_protect_type(FLASH_PROTECT_NONE);

		if (bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK) {
			bk_flash_cpu_write_enable();
#if CONFIG_BL2_WDT
			if (s_flash.flash_id == FLASH_ID_P25Q64SU) {
				for (uint32_t off = 0; off < size; ) {
					uint32_t chunk = size - off;

					if (chunk > (16 * 1024)) {
						chunk = 16 * 1024;
					}
					flash_memcpy((char *)(0x02000000 + address + off),
						(const char *)user_buf + off, chunk);
					off += chunk;
					BL2_WDT_FEED();
				}
				while (flash_hal_is_busy(&s_flash.hal)) {
					BL2_WDT_FEED();
				}
			} else
#endif
			{
				flash_memcpy((char *)(0x02000000 + address), (const char *)user_buf, size);
				while (flash_hal_is_busy(&s_flash.hal));
			}
			bk_flash_cpu_write_disable();
		}
	}
	flash_set_protect_type(protect_type);

	flash_restore_line_mode();
}

__attribute__((section(".iram")))
void bk_flash_write_primary_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	flash_switch_to_line_mode_two();
	if (!is_addr_write_primary_cbus(address, size)) {
		return;
	}
	bk_flash_cpu_write_enable();
	if (!is_addr_write_primary_cbus(address, size)) {
		return;
	}

	flash_memcpy((char*)(0x02000000+address), (const char*)user_buf,size);

	while(flash_hal_is_busy(&s_flash.hal));

	bk_flash_cpu_write_disable();
	flash_restore_line_mode();
}

uint32_t bk_flash_get_id(void)
{
	flash_switch_to_line_mode_two();
	s_flash.flash_id = flash_hal_get_id(&s_flash.hal);
	flash_restore_line_mode();
	return s_flash.flash_id;
}

flash_line_mode_t bk_flash_get_line_mode(void)
{
	return s_flash.flash_cfg->line_mode;
}

bk_err_t bk_flash_set_clk_dpll(void)
{
	flash_switch_to_line_mode_two();
	sys_drv_flash_set_dpll();
	flash_hal_set_clk_dpll(&s_flash.hal);
	flash_restore_line_mode();

	return BK_OK;
}

bk_err_t bk_flash_set_clk_dco(void)
{
	flash_switch_to_line_mode_two();
	sys_drv_flash_set_dco();
	bool ate_enabled = ate_is_enabled();
	flash_hal_set_clk_dco(&s_flash.hal, ate_enabled);
	flash_restore_line_mode();

	return BK_OK;
}

#if (CONFIG_SOC_BK7256XX)
bk_err_t bk_flash_set_clk(flash_clk_src_t flash_src_clk, uint8_t flash_dpll_div)
{
	if ((FLASH_CLK_DPLL == flash_src_clk) && (flash_dpll_div == 0)) {
		FLASH_LOGE("flash 120M clock not support.\r\n");
		return BK_FAIL;
	}
	if (FLASH_CLK_APLL == flash_src_clk) {
		FLASH_LOGE("flash apll clock not support.\r\n");
		return BK_FAIL;
	}

	uint32_t int_level = rtos_disable_int();
	if((sys_drv_flash_get_clk_sel() == flash_src_clk) && (sys_drv_flash_get_clk_div() == flash_dpll_div)) {
		rtos_enable_int(int_level);
		return BK_OK;
	}
	if (FLASH_CLK_DPLL == flash_src_clk) {
		sys_drv_flash_set_clk_div(flash_dpll_div);
	}
	sys_drv_flash_cksel(flash_src_clk);
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_flash_clk_switch(uint32_t flash_speed_type, uint32_t modules)
{
	uint32_t int_level = rtos_disable_int();
	int chip_id = 0;

	switch (flash_speed_type) {
		case FLASH_SPEED_LOW:
			s_hold_low_speed_status |= modules;
			FLASH_LOGD("%s: set low, 0x%x 0x%x\r\n", __func__, s_hold_low_speed_status, modules);
			if (s_hold_low_speed_status) {
				bk_flash_set_clk(FLASH_CLK_XTAL, FLASH_DPLL_DIV_VALUE_TEN);
			}
			break;

		case FLASH_SPEED_HIGH:
			s_hold_low_speed_status &= ~(modules);
			FLASH_LOGD("%s: clear low bit, 0x%x 0x%x\r\n", __func__, s_hold_low_speed_status, modules);
			if (0 == s_hold_low_speed_status) {
				chip_id = bk_get_hardware_chip_id_version();
				//chipC version with GD flash switch to 80M for peformance
				if ((chip_id == CHIP_VERSION_C) && ((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_GD)
				|| ((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_TH)
				|| ((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_PUYA)) {
					bk_flash_set_clk(FLASH_CLK_DPLL, FLASH_DPLL_DIV_VALUE_SIX);
				} else {
					bk_flash_set_clk(FLASH_CLK_DPLL, FLASH_DPLL_DIV_VALUE_TEN);
				}
			}
			break;
	}
	rtos_enable_int(int_level);

	return BK_OK;
}
#endif

bk_err_t bk_flash_write_enable(void)
{
	flash_switch_to_line_mode_two();
	flash_hal_write_enable(&s_flash.hal);
	flash_restore_line_mode();
	return BK_OK;
}

bk_err_t bk_flash_write_disable(void)
{
	flash_switch_to_line_mode_two();
	flash_hal_write_disable(&s_flash.hal);
	flash_restore_line_mode();
	return BK_OK;
}

uint16_t bk_flash_read_status_reg(void)
{
	flash_switch_to_line_mode_two();
	uint16_t sr_data = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	flash_restore_line_mode();
	return sr_data;
}

bk_err_t bk_flash_write_status_reg(uint16_t status_reg_data)
{
	flash_switch_to_line_mode_two();
	flash_write_status_reg(status_reg_data);
	flash_restore_line_mode();
	return BK_OK;
}

flash_protect_type_t bk_flash_get_protect_type(void)
{
	uint32_t type = 0;
	uint16_t protect_value = 0;

	flash_switch_to_line_mode_two();
	protect_value = flash_hal_get_protect_value(&s_flash.hal, s_flash.flash_cfg->status_reg_size,
												s_flash.flash_cfg->protect_post, s_flash.flash_cfg->protect_mask,
												s_flash.flash_cfg->cmp_post);
	if (protect_value == s_flash.flash_cfg->protect_all)
		type = FLASH_PROTECT_ALL;
	else if (protect_value == s_flash.flash_cfg->protect_none)
		type = FLASH_PROTECT_NONE;
	else if (protect_value == s_flash.flash_cfg->protect_half)
		type = FLASH_PROTECT_HALF;
	else if (protect_value == s_flash.flash_cfg->unprotect_last_block)
		type = FLASH_UNPROTECT_LAST_BLOCK;
	else
		type = -1;

	flash_restore_line_mode();
	return type;
}

bk_err_t bk_flash_set_protect_type(flash_protect_type_t type)
{
	flash_switch_to_line_mode_two();
	flash_set_protect_type(type);
	flash_restore_line_mode();
	return BK_OK;
}

bool bk_flash_is_driver_inited()
{
	return s_flash_is_init;
}

uint32_t bk_flash_get_current_total_size(void)
{
	return s_flash.flash_cfg->flash_size;
}

#define FLASH_OPERATE_SIZE_AND_OFFSET    (4096)
bk_err_t bk_spec_flash_write_bytes(bk_partition_t partition, const uint8_t *user_buf, uint32_t size,uint32_t offset)
{
	bk_logic_partition_t *bk_ptr = NULL;
	u8 *save_flashdata_buff  = NULL;
	flash_protect_type_t protect_type;
     
	bk_ptr = bk_flash_partition_get_info(partition);
	if((size + offset) > FLASH_OPERATE_SIZE_AND_OFFSET)
		return BK_FAIL;
	
	save_flashdata_buff= os_malloc(bk_ptr->partition_length);
	if(save_flashdata_buff == NULL)
	{
		os_printf("save_flashdata_buff malloc err\r\n");
		return BK_FAIL;
	}

	bk_flash_read_bytes((bk_ptr->partition_start_addr),(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);
    
	protect_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    
	bk_flash_erase_sector(bk_ptr->partition_start_addr);
	os_memcpy((save_flashdata_buff + offset), user_buf, size);
	bk_flash_write_bytes(bk_ptr->partition_start_addr ,(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);	
    	bk_flash_set_protect_type(protect_type);
        
	os_free(save_flashdata_buff);
	save_flashdata_buff = NULL;
    
	return BK_OK;

}

void bk_flash_set_base_addr(uint32_t addr)
{
	flash_hal_set_base_addr(&s_flash.hal, addr);
}

int bk_flash_set_dbus_security_region(uint32_t id, uint32_t start, uint32_t end, bool secure)
{
	if (id >= FLASH_DBUS_REGION_MAX) {
		return BK_ERR_FLASH_DBUS_REGION;
	}

	if (end <= start) {
		return BK_ERR_FLASH_ADDR;
	}

	if (end >= s_flash.flash_cfg->flash_size) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	flash_hal_set_dbus_region(&s_flash.hal, id, start, end, secure);
	return BK_OK;
}

void flash_set_xip_offset(uint32_t primary_start, uint32_t secondary_start, uint32_t code_size)
{
	flash_hal_set_offset_begin(&s_flash.hal,primary_start);
	flash_hal_set_offset_end(&s_flash.hal,primary_start+code_size);
	flash_hal_set_addr_offset(&s_flash.hal,secondary_start-primary_start);
}

void flash_set_excute_enable(int enable)
{
	flash_hal_set_offset_enable(&s_flash.hal, enable);
}

uint32_t flash_get_excute_enable()
{
	return flash_hal_read_offset_enable(&s_flash.hal);
}
