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
#include "../flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#include "flash_bypass.h"
#include "mb_ipc_cmd.h"
#include "CheckSumUtils.h"
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#if ((defined(CONFIG_SECURITY_OTA)) && (!defined(CONFIG_TFM_FWU)) && (!defined(CONFIG_BK_OTA)))
#include "partitions.h"
#include "_ota.h"
#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include "bk_wdt.h"
#endif
#endif

#if CONFIG_FLASH_V1P1

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#include "partitions_gen.h"
#endif

#if CONFIG_FLASH_QUAD_ENABLE
#include "flash_bypass.h"
extern UINT8 flash_get_line_mode(void);
#endif
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static volatile spinlock_t flash_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP



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
	{0x0B6017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR,  0,	    2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                9,            1,           0xA0,                          0x02}, //xt_25q64d
#else
	{0x0B6017,   1,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO,   0,      2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //xt_25q64d
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B6018,   2,               FLASH_SIZE_16M, FLASH_LINE_MODE_FOUR,  0,	    2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                9,            1,           0xA0,                          0x02}, //xt_25q128d
#else
	{0x0B6018,   1,               FLASH_SIZE_16M, FLASH_LINE_MODE_TWO,   0,     2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //xt_25q128d
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
	{0xC86516,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x0a,         0x0E,         0x102,                9,            1,           0xA0,                          0x02}, //gd_25wq32e
#else
	{0xC86516,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x0a,         0x0E,         0x102,                0,            0,           0xA0,                          0x01}, //gd_25wq32e
#endif
	{0xEF4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x00,         0x101,                9,            1,           0xA0,                          0x01}, //w_25q32(bfj)
#if CONFIG_FLASH_QUAD_ENABLE
	{0x204118,	 2, 			  FLASH_SIZE_16M,FLASH_LINE_MODE_FOUR, 0,		2,			  0x0F, 		0x0F,		 0x00,		   0x0A,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //xm_25qu128c
#else
	{0x204118,	 1, 			  FLASH_SIZE_16M,FLASH_LINE_MODE_TWO,  0,		2,			  0x0F, 		0x0F,		 0x00,		   0x0A,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //xm_25qu128c
#endif
	{0x204016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xmc_25qh32b
	{0xC22315,   1,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 0,        2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                6,            1,           0xA5,                          0x01}, //mx_25v16b
	{0xEB6015,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //zg_th25q16b
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86517,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x102, 			   9,			 1, 		  0xA0, 						 0x02}, //gd_25Q32E
#else
	{0xC86517,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x102, 			   0,			 0, 		  0xA0, 						 0x01}, //gd_25Q32E
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xCD6016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x0A,         0x0E,         0x102,                9,            1,           0xA0,                          0x02}, //th_25q32ub
#else
	{0xCD6016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO,  14,      2,            0x1F,         0x1F,        0x0A,         0x0E,         0x102,                9,            1,           0xA0,                          0x01}, //th_25q32ub
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0x856017,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x1F,                 9,			 1, 		  0xA0, 						 0x02}, //p_25q64su
#else
	{0x856017,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x1F,                 0,			 0, 		  0xA0, 						 0x01}, //p_25q64su
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

flash_driver_t s_flash = {0};
static bool s_flash_is_init = false;
static beken_mutex_t s_flash_mutex = NULL;
static flash_line_mode_t s_flash_runtime_line_mode = FLASH_LINE_MODE_TWO;
static PM_STATUS flash_ps_status;
static flash_ps_callback_t s_flash_ps_suspend_cb = NULL;
static flash_ps_callback_t s_flash_ps_resume_cb = NULL;
#if (CONFIG_SOC_BK7256XX)
static uint32_t s_hold_low_speed_status = 0;
#endif
#define FLASH_MAX_WAIT_CB_CNT (4)
static flash_wait_callback_t s_flash_wait_cb[FLASH_MAX_WAIT_CB_CNT] = {NULL};
static int flash_protect_count = 0;

/* Forward declarations for APIs used before definition. */
uint16_t bk_flash_read_status_reg(void);
bk_err_t bk_flash_write_status_reg(uint16_t status_reg_data);
flash_protect_type_t bk_flash_get_protect_type(void);
bk_err_t bk_flash_set_protect_type(flash_protect_type_t type);
bk_err_t bk_flash_set_protect_none_full(void);
bk_err_t bk_flash_set_protect_type_full(flash_protect_type_t type);
bk_err_t bk_flash_erase_32k(uint32_t address);
extern bk_err_t mb_flash_op_prepare(void);
extern bk_err_t mb_flash_op_finish(void);
extern int xTaskResumeAll(void);
extern void vTaskSuspendAll(void);

static inline uint32_t flash_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&flash_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void flash_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&flash_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

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

void flash_waiting_cb(void)
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

static UINT32 flash_ps_suspend(UINT32 ps_level)
{
	PM_STATUS *flash_ps_status_ptr = &flash_ps_status;

	switch (ps_level) {
	case NORMAL_PS:
	case LOWVOL_PS:
	case DEEP_PS:
	case IDLE_PS:
		if (s_flash_ps_suspend_cb) {
			s_flash_ps_suspend_cb();
		}
		if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode()) {
			bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
		}
		flash_ps_status_ptr->bits.unconditional_ps_sleeped = 1;
		flash_ps_status_ptr->bits.normal_ps_sleeped = 1;
		flash_ps_status_ptr->bits.lowvol_ps_sleeped = 1;
		flash_ps_status_ptr->bits.deep_ps_sleeped = 1;
		break;
	default:
		break;
	}
	return 0;
}

static UINT32 flash_ps_resume(UINT32 ps_level)
{
	PM_STATUS *flash_ps_status_ptr = &flash_ps_status;

	switch (ps_level) {
	case NORMAL_PS:
	case LOWVOL_PS:
	case DEEP_PS:
	case IDLE_PS:
		if (FLASH_LINE_MODE_FOUR == s_flash.flash_cfg->line_mode) {
			bk_flash_set_line_mode(FLASH_LINE_MODE_FOUR);
		}
		if (s_flash_ps_resume_cb) {
			s_flash_ps_resume_cb();
		}
		flash_ps_status_ptr->bits.unconditional_ps_sleeped = 0;
		flash_ps_status_ptr->bits.normal_ps_sleeped = 0;
		flash_ps_status_ptr->bits.lowvol_ps_sleeped = 0;
		flash_ps_status_ptr->bits.deep_ps_sleeped = 0;
		break;
	default:
		break;
	}
	return 0;
}

static PM_STATUS flash_ps_get_status(UINT32 flag)
{
	return flash_ps_status;
}

static DEV_PM_OPS_S flash_ps_ops = {
	.pm_init = NULL,
	.pm_deinit = NULL,
	.suspend = flash_ps_suspend,
	.resume = flash_ps_resume,
	.status = flash_ps_get_status,
	.get_sleep_time = NULL,
};

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

static inline bool is_64k_aligned(uint32_t addr)
{
	return ((addr & (KB(64) - 1)) == 0);
}

static inline bool is_32k_aligned(uint32_t addr)
{
	return ((addr & (KB(32) - 1)) == 0);
}

uint8_t bk_flash_get_coutinuous_read_mode(void)
{
	return 0;
}

bk_err_t bk_flash_switch_line_mode(flash_line_mode_t line_mode)
{
	(void)line_mode;
	return BK_OK;
}

bk_err_t bk_flash_dump_erase_sector(uint32_t address)
{
	(void)address;
	return BK_OK;
}

bk_err_t bk_flash_dump_write(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	(void)address;
	(void)user_buf;
	(void)size;
	return BK_OK;
}

bk_err_t bk_flash_erase_fast(uint32_t erase_off, uint32_t len)
{
	uint32_t erase_size = 0;
	int erase_remain = len;

	while (erase_remain > 0) {
		if ((erase_remain >= KB(64)) && is_64k_aligned(erase_off)) {
			FLASH_LOGD("64k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_block(erase_off);
			erase_size = KB(64);
		} else if ((erase_remain >= KB(32)) && is_32k_aligned(erase_off)) {
			FLASH_LOGD("32k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_32k(erase_off);
			erase_size = KB(32);
		} else {
			FLASH_LOGD("4k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_sector(erase_off);
			erase_size = KB(4);
		}
		erase_off += erase_size;
		erase_remain -= erase_size;
	}

	return BK_OK;
}

uint32_t flash_get_excute_enable(void)
{
	return flash_hal_read_offset_enable(&s_flash.hal);
}

__attribute__((section(".iram"))) void bk_flash_enable_cpu_data_wr(void)
{
	flash_hal_enable_cpu_data_wr(&s_flash.hal);
}

__attribute__((section(".iram"))) void bk_flash_disable_cpu_data_wr(void)
{
	flash_hal_disable_cpu_data_wr(&s_flash.hal);
}

__attribute__((section(".iram")))
static void flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	bk_flash_enable_cpu_data_wr();
	os_memcpy((void *)(0x02000000 + address), user_buf, size);
	while (flash_hal_is_busy(&s_flash.hal)) {
	}
	bk_flash_disable_cpu_data_wr();
}

__attribute__((section(".iram")))
void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	uint32_t line_mode = bk_flash_get_line_mode();
	uint32_t int_status = rtos_disable_int();

	(void)bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
#if CONFIG_CACHE_ENABLE
	enable_dcache(0);
#endif
	flash_write_cbus(address, user_buf, size);
#if CONFIG_CACHE_ENABLE
	enable_dcache(1);
#endif
	rtos_enable_int(int_status);
	(void)bk_flash_set_line_mode(line_mode);
}

__attribute__((section(".iram")))
void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
	os_memcpy(user_buf, (const void *)(0x02000000 + address), size);
}

bk_err_t bk_flash_check_crc(uint32_t phy_offset, uint32_t phy_size)
{
	(void)phy_offset;
	(void)phy_size;
	return BK_OK;
}

flash_line_mode_t flash_set_line_mode(flash_line_mode_t line_mode)
{
	flash_line_mode_t old_line_mode = bk_flash_get_line_mode();
	(void)bk_flash_set_line_mode(line_mode);
	return old_line_mode;
}

bk_err_t flash_write_common(const uint8_t *buffer, uint32_t address, uint32_t len)
{
	return bk_flash_write_bytes(address, buffer, len);
}

bk_err_t bk_flash_power_saving_enter(void)
{
	return bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
}

bk_err_t bk_flash_power_saving_exit(void)
{
	if ((s_flash.flash_cfg != NULL) && (s_flash.flash_cfg->line_mode == FLASH_LINE_MODE_FOUR)) {
		return bk_flash_set_line_mode(FLASH_LINE_MODE_FOUR);
	}

	return BK_OK;
}

__attribute__((section(".iram")))
void * __attribute__((optimize("-O3"))) bk_memcpy_4w(void *dst, const void *src, unsigned int size)
{
	unsigned char *dst_ptr = (unsigned char *)dst;
	const unsigned char *src_ptr = (const unsigned char *)src;
	unsigned int temp1, temp2, temp3, temp4;

	if ((((unsigned int)src_ptr ^ (unsigned int)dst_ptr) & (sizeof(unsigned int) - 1)) == 0) {
		while ((unsigned int)src_ptr & (sizeof(unsigned int) - 1)) {
			if (size == 0) {
				return dst;
			}

			size--;
			*dst_ptr++ = *src_ptr++;
		}

		const unsigned int *src_wptr = (const unsigned int *)src_ptr;
		unsigned int *dst_wptr = (unsigned int *)dst_ptr;

		while (size >= (sizeof(unsigned int) * 4)) {
			temp1 = src_wptr[0];
			temp2 = src_wptr[1];
			temp3 = src_wptr[2];
			temp4 = src_wptr[3];

			dst_wptr[0] = temp1;
			dst_wptr[1] = temp2;
			dst_wptr[2] = temp3;
			dst_wptr[3] = temp4;

			src_wptr += 4;
			dst_wptr += 4;
			size -= (sizeof(unsigned int) * 4);
		}

		while (size >= sizeof(unsigned int)) {
			*dst_wptr++ = *src_wptr++;
			size -= sizeof(unsigned int);
		}

		src_ptr = (const unsigned char *)src_wptr;
		dst_ptr = (unsigned char *)dst_wptr;
	}

	while (size > 0) {
		*dst_ptr++ = *src_ptr++;
		size--;
	}

	return dst;
}

#if ((defined(CONFIG_SECURITY_OTA)) && (!defined(CONFIG_TFM_FWU)) && (!defined(CONFIG_BK_OTA)))
#ifndef CEIL_ALIGN_34
#define CEIL_ALIGN_34(addr) (((addr) + 34 - 1) / 34 * 34)
#endif

#if CONFIG_OTA_OVERWRITE
static void bk_flash_overwrite_write_dbus(uint32_t off, const void *src, uint32_t len)
{
	uint32_t fa_addr = CONFIG_OTA_PHY_PARTITION_OFFSET;
	uint32_t addr = fa_addr + off;
	bk_flash_write_bytes(addr, src, len);
}

static void bk_flash_overwrite_update(uint32_t off, const void *src, uint32_t len)
{
	bk_flash_overwrite_write_dbus(off, src, len);
}
#endif

#if CONFIG_DIRECT_XIP
__attribute__((section(".iram")))
static void bk_flash_xip_write_cbus(uint32_t off, const void *src, uint32_t len)
{
	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET));

	if ((fa_off + off) & 0x31) {
		return;
	}

	uint32_t int_status = rtos_disable_int();
#if CONFIG_CACHE_ENABLE
	enable_dcache(0);
#endif
	uint32_t write_addr = (fa_off + off);
	write_addr |= 1U << 24;
	bk_flash_write_cbus(write_addr, src, len);
#if CONFIG_CACHE_ENABLE
	enable_dcache(1);
#endif
	rtos_enable_int(int_status);
}

void bk_flash_xip_write_dbus(uint32_t off, const void *src, uint32_t len)
{
	uint32_t update_id = (flash_get_excute_enable() ^ 1);
	uint32_t fa_addr;
	if (update_id == 0) {
		fa_addr = CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET;
	} else {
		fa_addr = CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET;
	}
	uint32_t addr = fa_addr + off;
	bk_flash_write_bytes(addr, src, len);
}

void bk_flash_xip_update(uint32_t off, const void *src, uint32_t len)
{
#if CONFIG_OTA_ENCRYPTED
	bk_flash_xip_write_dbus(off, src, len);
#else
	bk_flash_xip_write_cbus(off, src, len);
#endif
}

static uint32_t boot_xip_magic_off(uint32_t fa_id)
{
	uint32_t phy_offset = 0xFFFFFFFF;
	if (fa_id == 0) {
		phy_offset = CEIL_ALIGN_34(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4096);
	} else if (fa_id == 1) {
		phy_offset = CEIL_ALIGN_34(CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET + CONFIG_SECONDARY_ALL_PHY_PARTITION_SIZE - 4096);
	}
	return phy_offset;
}

void bk_flash_write_xip_status(uint32_t fa_id, uint32_t type, uint32_t status)
{
	uint32_t offset = boot_xip_magic_off(fa_id) + (type - 1) * 32;
	const uint8_t *value = (const uint8_t *)&status;
	bk_flash_write_bytes(offset, value, 4);
}
#endif /* CONFIG_DIRECT_XIP */

#if CONFIG_OTA_CONFIRM_UPDATE
static uint32_t boot_overwrite_magic_off(void)
{
	uint32_t phy_offset = (CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4);
	return phy_offset;
}

void bk_flash_ota_write_confirm(uint32_t status)
{
	uint32_t offset = boot_overwrite_magic_off();
	const uint8_t *value = (const uint8_t *)&status;
	bk_flash_write_bytes(offset, value, 4);
}

void bk_flash_ota_erase_confirm(void)
{
	bk_flash_erase_fast(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4096, 4096);
}
#endif

void bk_flash_ota_update(uint32_t off, const void *src, uint32_t len)
{
#if CONFIG_DIRECT_XIP
	bk_flash_xip_update(off, src, len);
#elif CONFIG_OTA_OVERWRITE
	bk_flash_overwrite_update(off, src, len);
#else
	(void)off;
	(void)src;
	(void)len;
#endif
}

void bk_flash_ota_erase(void)
{
#if CONFIG_DIRECT_XIP
	uint32_t update_id = flash_get_excute_enable() ^ 1;
	uint32_t erase_addr;
	if (update_id == 0) {
		erase_addr = CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET;
	} else {
		erase_addr = CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET;
	}
	uint32_t erase_size = CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE;
#elif CONFIG_OTA_OVERWRITE
	bk_flash_erase_fast(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE - 4096, 4096);
	uint32_t erase_addr = CONFIG_OTA_PHY_PARTITION_OFFSET;
	uint32_t erase_size = CONFIG_OTA_PHY_PARTITION_SIZE;
#else
	return;
#endif

#if CONFIG_INT_WDT
	bk_wdt_stop();
#endif
	bk_flash_erase_fast(erase_addr, erase_size);
#if CONFIG_INT_WDT
	bk_wdt_start(CONFIG_INT_WDT_PERIOD_MS);
#endif
}

#if CONFIG_DIRECT_XIP
void bk_flash_ota_write_magic(void)
{
	uint32_t update_id = flash_get_excute_enable() ^ 1;
	bk_flash_write_xip_status(update_id, XIP_MAGIC_TYPE, OTA_CONFIRM);
}
#endif
#endif /* CONFIG_SECURITY_OTA && !CONFIG_TFM_FWU && !CONFIG_BK_OTA */

#if CONFIG_FLASH_OTP
void bk_flash_lock(flash_line_mode_t *line_mode)
{
	(void)line_mode;
}

void bk_flash_unlock(flash_line_mode_t *line_mode)
{
	(void)line_mode;
}

void bk_flash_lock_otp(uint8_t block_index)
{
	(void)block_index;
}
#endif

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

static bool is_address_erase_writable(uint32_t address)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	return true;
#else
#if CONFIG_OTA_PHY_PARTITION_OFFSET && CONFIG_OTA_PHY_PARTITION_SIZE
	bool writable = (address < (CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_SIZE)) ? false : true;
	return writable;
#else
	return true;
#endif
#endif
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

static void calculateProtectionBits(uint32_t flash_size, uint32_t length, uint8_t* cmp, 
									uint8_t* bp4, uint8_t* bp3, uint8_t* bp2, uint8_t* bp1, uint8_t* bp0) {
	*cmp = 0; *bp4 = 1; *bp3 = 1; *bp2 = 1; *bp1 = 1; *bp0 = 1;
	if (flash_size == 0x400000) {
		// Protect area
		if (length >= 0x400000) { // 4MB
			*cmp = 0; *bp4 = 1; *bp3 = 1; *bp2 = 1; *bp1 = 1; *bp0 = 1;
		} else if (length >= 0x3F0000) { // 4032KB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 0; *bp1 = 0; *bp0 = 1;
		} else if (length >= 0x3E0000) { // 3968KB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 0; *bp1 = 1; *bp0 = 0;
		} else if (length >= 0x3C0000) { // 3840KB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 0; *bp1 = 1; *bp0 = 1;
		} else if (length >= 0x380000) { // 3584KB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 1; *bp1 = 0; *bp0 = 0;
		} else if (length >= 0x300000) { // 3MB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 1; *bp1 = 0; *bp0 = 1;
		} else if (length >= 0x200000) { // 2MB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 1; *bp1 = 1; *bp0 = 0;
		} else if (length >= 0x100000) { // 1MB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 1; *bp1 = 0; *bp0 = 1;
		} else if (length >= 0x80000) { // 512KB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 1; *bp1 = 0; *bp0 = 0;
		} else if (length >= 0x40000) { // 256KB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 0; *bp1 = 1; *bp0 = 1;
		} else if (length >= 0x20000) { // 128KB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 0; *bp1 = 1; *bp0 = 0;
		} else { // All
			*cmp = 0; *bp4 = 1; *bp3 = 1; *bp2 = 1; *bp1 = 1; *bp0 = 1;
		}
	}
	else if (flash_size == 0x800000) {
		// Protect area
		if (length >= 0x800000) { // 8MB
			*cmp = 0; *bp4 = 1; *bp3 = 1; *bp2 = 1; *bp1 = 1; *bp0 = 1;
		} else if (length >= 0x7F0000) { // 8064KB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 0; *bp1 = 0; *bp0 = 1;
		} else if (length >= 0x7C0000) { // 7936KB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 0; *bp1 = 1; *bp0 = 0;
		} else if (length >= 0x780000) { // 7680KB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 0; *bp1 = 1; *bp0 = 1;
		} else if (length >= 0x700000) { // 7MB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 1; *bp1 = 0; *bp0 = 0;
		} else if (length >= 0x600000) { // 6MB
			*cmp = 1; *bp4 = 0; *bp3 = 0; *bp2 = 1; *bp1 = 0; *bp0 = 1;
		} else if (length >= 0x400000) { // 4MB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 1; *bp1 = 1; *bp0 = 0;
		} else if (length >= 0x200000) { // 2MB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 1; *bp1 = 0; *bp0 = 1;
		} else if (length >= 0x100000) { // 1MB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 1; *bp1 = 0; *bp0 = 0;
		} else if (length >= 0x80000) { // 512KB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 0; *bp1 = 1; *bp0 = 1;
		} else if (length >= 0x40000) { // 256KB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 0; *bp1 = 1; *bp0 = 0;
		} else if (length >= 0x20000) { // 128KB
			*cmp = 0; *bp4 = 0; *bp3 = 1; *bp2 = 0; *bp1 = 0; *bp0 = 1;
		} else { // All
			*cmp = 0; *bp4 = 0; *bp3 = 0; *bp2 = 0; *bp1 = 0; *bp0 = 0;
		}
	}
	else
	{
		FLASH_LOGW("Error: unconfigured flash parameters.\r\n");
		BK_ASSERT(0);
	}
}

void flash_set_protect_type(flash_protect_type_t type)
{
	uint32_t protect_cfg;
	uint32_t cmp_cfg;
	uint32_t status_reg;

	if (type == FLASH_PROTECT_ALL || type == FLASH_UNPROTECT_LAST_BLOCK) {
#if !CONFIG_FLASH_PROTECTED_AREA_IS_ALL
#if (((CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)) && CONFIG_NVS_PHY_PARTITION_OFFSET)
		uint8_t cmp, bp4, bp3, bp2, bp1, bp0;
		calculateProtectionBits(s_flash.flash_cfg->flash_size,CONFIG_NVS_PHY_PARTITION_OFFSET, 
								&cmp, &bp4, &bp3, &bp2, &bp1, &bp0);
		protect_cfg = (bp4 << 4) | (bp3 << 3) | (bp2 << 2) | (bp1 << 1) | bp0;
		cmp_cfg = cmp;
#else
		protect_cfg = flash_get_protect_cfg(type);
		cmp_cfg = flash_get_cmp_cfg(type);
#endif
#else
		protect_cfg = flash_get_protect_cfg(type);
		cmp_cfg = flash_get_cmp_cfg(type);
#endif
	}
	else if (type == FLASH_PROTECT_APP)
	{
#if (((CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)) && \
		CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_OFFSET && CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_SIZE)
		uint8_t cmp, bp4, bp3, bp2, bp1, bp0;

		calculateProtectionBits(s_flash.flash_cfg->flash_size,(CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_SIZE), 
								&cmp, &bp4, &bp3, &bp2, &bp1, &bp0);
		protect_cfg = (bp4 << 4) | (bp3 << 3) | (bp2 << 2) | (bp1 << 1) | bp0;
		cmp_cfg = cmp;
#else
		protect_cfg = flash_get_protect_cfg(FLASH_PROTECT_APP);
		cmp_cfg = flash_get_cmp_cfg(FLASH_PROTECT_APP);
#endif
	}
	else
	{
		protect_cfg = flash_get_protect_cfg(type);
		cmp_cfg = flash_get_cmp_cfg(type);
	}

	status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);

	if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
		flash_set_protect_cfg(&status_reg, protect_cfg);
		flash_set_cmp_cfg(&status_reg, cmp_cfg);

#if CONFIG_FLASH_WRITE_STATUS_VOLATILE
		flash_hal_set_volatile_status_write(&s_flash.hal);
#endif
		//FLASH_LOGD("write status reg:%x, status_reg_size:%d\r\n", status_reg, s_flash.flash_cfg->status_reg_size);
		flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg);
#if CONFIG_FLASH_WRITE_STATUS_VOLATILE
		flash_hal_clear_volatile_status_write(&s_flash.hal);
#endif
	}
}

static void flash_unprotected() {
	if (flash_protect_count == 0) {
		flash_set_protect_type(FLASH_PROTECT_APP);
	}
	flash_protect_count++;
}

static void flash_protected() {
	flash_protect_count--;
	if (flash_protect_count < 0) {
		FLASH_LOGE("invalid protect: %d\r\n", flash_protect_count);
		BK_ASSERT(0);
	}
	
	if (flash_protect_count == 0) {
		flash_set_protect_type(FLASH_PROTECT_ALL);
	}
}

static bool flash_is_hw_sr_protect_active(uint32_t status_reg)
{
	return (status_reg & FLASH_SR_SRP0_BIT) && !(status_reg & FLASH_SR_SRP1_BIT);
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
	flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg);
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
	flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg);
}

static void flash_set_qwfr(void)
{
	flash_hal_set_mode(&s_flash.hal, s_flash.flash_cfg->mode_sel);
}

static void flash_switch_to_line_mode_two(void)
{
	if (FLASH_LINE_MODE_FOUR == s_flash.flash_cfg->line_mode) {
		flash_set_line_mode(FLASH_LINE_MODE_TWO);
	}
}

static void flash_restore_line_mode(void)
{
	if (FLASH_LINE_MODE_FOUR == s_flash.flash_cfg->line_mode) {
		flash_set_line_mode(FLASH_LINE_MODE_FOUR);
	}
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

	flash_hal_write_status_reg(&s_flash.hal, sr_width, status_reg);

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
		uint32_t int_level = flash_enter_critical();
		flash_hal_wait_op_done(&s_flash.hal);

		flash_hal_set_op_cmd_read(&s_flash.hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash.hal);
		}
		flash_exit_critical(int_level);

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
		uint32_t int_level = flash_enter_critical();

		flash_hal_wait_op_done(&s_flash.hal);

		flash_hal_set_op_cmd_read(&s_flash.hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash.hal);
		}

		flash_exit_critical(int_level);

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

//extern part_flag update_part_flag;
bool flash_is_area_write_disable(uint32_t addr)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	(void)addr;
	return false;
#else

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
#endif
}

static bk_err_t flash_write_common_inner(const uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t buf[FLASH_BUFFER_LEN];
	uint8_t *pb = (uint8_t *)&buf[0];
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	FLASH_RETURN_ON_WRITE_ADDR_OUT_OF_RANGE(addr, len);

	while (len) {
		os_memset(pb, 0xFF, FLASH_BYTES_CNT);
		for (uint32_t i = address % FLASH_BYTES_CNT; i < FLASH_BYTES_CNT; i++) {
			pb[i] = *buffer++;
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}
		uint32_t int_level = flash_enter_critical();
		flash_hal_wait_op_done(&s_flash.hal);
		flash_hal_write_enable(&s_flash.hal);

		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			flash_hal_write_data(&s_flash.hal, buf[i]);
		}

		flash_hal_set_op_cmd_write(&s_flash.hal, addr);
		flash_exit_critical(int_level);

		addr += FLASH_BYTES_CNT;
	}
	return BK_OK;
}

void flash_lock(void)
{
	bk_err_t ret = BK_OK;

	BK_ASSERT(0 == rtos_is_in_interrupt_context());
	if(rtos_local_irq_disabled())
	{
		FLASH_LOGW("flash_lock skip: local irq disabled\r\n");
		return;
	}

	rtos_lock_mutex(&s_flash_mutex);
	vTaskSuspendAll();
	ret = mb_flash_op_prepare();
	(void)ret;
}

void flash_unlock(void)
{
	bk_err_t ret = BK_OK;

	BK_ASSERT(0 == rtos_is_in_interrupt_context());
	if(rtos_local_irq_disabled())
	{
		FLASH_LOGW("flash_unlock skip: local irq disabled\r\n");
		return;
	}

	ret = mb_flash_op_finish();
	(void)ret;
	xTaskResumeAll();
	rtos_unlock_mutex(&s_flash_mutex);
}

#if defined(CONFIG_SECURITY_OTA) && !defined(CONFIG_TFM_FWU)
__attribute__((section(".iram")))
#endif
bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode)
{
	flash_hal_clear_qwfr(&s_flash.hal);
#if CONFIG_SOC_BK7236XX
	sys_drv_set_sys2flsh_2wire(0);
#endif
	if (FLASH_LINE_MODE_TWO == line_mode) {
		if (s_flash.flash_cfg != NULL && (1 == s_flash.flash_cfg->quad_en_val)) {
			flash_clear_qe();
		}
		flash_hal_set_mode(&s_flash.hal, FLASH_MODE_DUAL);
		s_flash_runtime_line_mode = FLASH_LINE_MODE_TWO;
	} else if (FLASH_LINE_MODE_FOUR == line_mode) {
		flash_hal_set_quad_m_value(&s_flash.hal, s_flash.flash_cfg->coutinuous_read_mode_bits_val);
		if (s_flash.flash_cfg != NULL && (1 == s_flash.flash_cfg->quad_en_val)) {
			flash_set_qe();
		}
		flash_hal_set_mode(&s_flash.hal, FLASH_MODE_QUAD);
		s_flash_runtime_line_mode = FLASH_LINE_MODE_FOUR;
	} else {
		s_flash_runtime_line_mode = FLASH_LINE_MODE_TWO;
	}
#if CONFIG_SOC_BK7236XX
	sys_drv_set_sys2flsh_2wire(1);
#endif
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
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
#endif
#endif
	os_memset(&s_flash, 0, sizeof(s_flash));
	flash_hal_init(&s_flash.hal);
	bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
	s_flash.flash_id = flash_hal_get_id(&s_flash.hal);
	flash_get_current_config();
	flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

#if CONFIG_BL2_UPGRADE_WITH_APP
	uint8_t bootloader_version[4];
	static const uint8_t expected_bl2_version[] = CONFIG_BOOTLOADER_VERSION_BYTES;

	bk_flash_read_bytes(0x132, bootloader_version, 4);
	if (!(bootloader_version[0] == expected_bl2_version[0]
		&& bootloader_version[1] == expected_bl2_version[1]
		&& bootloader_version[2] == expected_bl2_version[2]
		&& bootloader_version[3] == expected_bl2_version[3])) {
			uint16_t status_reg = bk_flash_read_status_reg();
			status_reg &= ~((0b111 << 2) | (0x1 << 14));
			bk_flash_write_status_reg(status_reg);
	}
#endif

	flash_hal_disable_cpu_data_wr(&s_flash.hal);
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

#if (CONFIG_SOC_BK7236XX)
	if(((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_GD)
	||((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_TH)
	|| ((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_PUYA)) {
		if((1 != sys_drv_flash_get_clk_sel()) || (1 != sys_drv_flash_get_clk_div())) {
			sys_drv_flash_set_clk_div(1); // dpll div 6 = 80M
			sys_drv_flash_cksel(1);
		}
	} else {
		if((1 != sys_drv_flash_get_clk_sel()) || (3 != sys_drv_flash_get_clk_div())) {
			sys_drv_flash_set_clk_div(3); // dpll div 10 = 48M
			sys_drv_flash_cksel(1);
		}
	}

	sys_drv_set_sys2flsh_2wire(1);

#endif

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

bk_err_t flash_erase_block(uint32_t address, int type)
{
#if CONFIG_FLASH_MB && CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
	int ret = BK_OK;
#endif
	uint32_t erase_addr = 0;
	uint32_t erase_size = 0;
	flash_line_mode_t old_line_mode;
	bool need_switch_line_mode = false;

	if (type == FLASH_OP_CMD_SE) {
		erase_size = FLASH_SECTOR_SIZE;
	} else if (type == FLASH_OP_CMD_BE1) {
		erase_size = FLASH_BLOCK32_SIZE;
	} else if (type == FLASH_OP_CMD_BE2) {
		erase_size = FLASH_BLOCK_SIZE;
	} else {
		FLASH_LOGE("erase error:invalid cmd type %d\r\n", type);
		return BK_FAIL;
	}

	erase_addr = address & (~(erase_size - 1));
	if (erase_addr >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", erase_addr);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (flash_is_area_write_disable(address)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	old_line_mode = bk_flash_get_line_mode();
	need_switch_line_mode = (old_line_mode != FLASH_LINE_MODE_TWO);
	if (need_switch_line_mode) {
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
	}

//CPU0 notfify CPU1 when operate flash, to fix LCD display issue while erasing
#if CONFIG_FLASH_MB && CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
	ret = ipc_send_flash_op_prepare();
	if(ret != BK_OK)
	{
		FLASH_LOGD("erase prepare ret = 0x%x\n", ret );
	}
#endif
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	flash_hal_wait_op_done(&s_flash.hal);
	flash_hal_write_enable(&s_flash.hal);
	flash_hal_erase_block(&s_flash.hal, erase_addr, type);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);

#if CONFIG_FLASH_MB && CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
	ret = ipc_send_flash_op_finish();
	if(ret != BK_OK)
	{
		FLASH_LOGD("erase op_finish ret = 0x%x\n", ret );
	}
#endif

	if (need_switch_line_mode) {
		bk_flash_set_line_mode(old_line_mode);
	}

	return BK_OK;
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
#if !CONFIG_BL2_UPGRADE_WITH_APP
	if (!is_address_erase_writable(address))
	{
		FLASH_LOGE("Current addr : (%x) is protected, cannot be erased.\r\n",address);
		return BK_FAIL;
	}
#endif

	flash_lock();
	bk_err_t ret = flash_erase_block(address, FLASH_OP_CMD_SE);
	flash_unlock();
	return ret;
}

bk_err_t bk_flash_erase_32k(uint32_t address)
{
#if !CONFIG_BL2_UPGRADE_WITH_APP
	if (!is_address_erase_writable(address))
	{
		FLASH_LOGE("Current addr : (%x) is protected, cannot be erased.\r\n",address);
		return BK_FAIL;
	}
#endif

	flash_lock();
	bk_err_t ret = flash_erase_block(address, FLASH_OP_CMD_BE1);
	flash_unlock();
	return ret;
}

bk_err_t bk_flash_erase_block(uint32_t address)
{
#if !CONFIG_BL2_UPGRADE_WITH_APP
	if (!is_address_erase_writable(address))
	{
		FLASH_LOGE("Current addr : (%x) is protected, cannot be erased.\r\n",address);
		return BK_FAIL;
	}
#endif

	flash_lock();
	bk_err_t ret = flash_erase_block(address, FLASH_OP_CMD_BE2);
	flash_unlock();
	return ret;
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

static bk_err_t flash_write_no_lock(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	flash_line_mode_t old_line_mode;
	bool need_switch_line_mode = false;

	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (flash_is_area_write_disable(address)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (!is_address_erase_writable(address))
	{
		FLASH_LOGE("Current addr : (%x) is protected, cannot be written.\r\n",address);
		return BK_FAIL;
	}

	old_line_mode = bk_flash_get_line_mode();
	need_switch_line_mode = (old_line_mode != FLASH_LINE_MODE_TWO);
	if (need_switch_line_mode) {
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
	}

	flash_write_common_inner(user_buf, address, size);
	flash_hal_wait_op_done(&s_flash.hal);
	if (need_switch_line_mode) {
		bk_flash_set_line_mode(old_line_mode);
	}

	return BK_OK;
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	bk_err_t ret = BK_OK;

	flash_lock();
	ret = flash_write_no_lock(address, user_buf, size);
	flash_unlock();

	return ret;
}

bool check_flash_crc_16(uint32_t vir_start, size_t size)
{
    flash_data_t enc_data;
    uint32_t phy_start = (vir_start >> 5) * 34;

    size = (size + 32 - 1) & ~(32 - 1); // 32 align_up
    size = (size >> 5) *34;

    for (size_t i = 0; i < size / 34; i++) {
        bk_flash_read_bytes(phy_start + i*34, (uint8_t*)&enc_data, 34);
        if (enc_data.crc16 != calculate_crc16(enc_data.data, 32)) {
            FLASH_LOGE("read crc = %#x,cal crc =%#x, addr=%#x\r\n",enc_data.crc16, calculate_crc16(enc_data.data, 32), phy_start + i*34);
            return false;
        }
    }
    return true;
}

bk_err_t bk_flash_write_bytes_ota(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	flash_line_mode_t old_line_mode;
	bool need_switch_line_mode = false;

	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (flash_is_area_write_disable(address)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	old_line_mode = bk_flash_get_line_mode();
	need_switch_line_mode = (old_line_mode != FLASH_LINE_MODE_TWO);
	if (need_switch_line_mode) {
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
	}

	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	flash_write_common_inner(user_buf, address, size);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);
	if (need_switch_line_mode) {
		bk_flash_set_line_mode(old_line_mode);
	}

	return BK_OK;
}

uint32_t bk_flash_get_id(void)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	s_flash.flash_id = flash_hal_get_id(&s_flash.hal);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);
	return s_flash.flash_id;
}

#if defined(CONFIG_SECURITY_OTA) && !defined(CONFIG_TFM_FWU)
__attribute__((section(".iram")))
#endif
flash_line_mode_t bk_flash_get_line_mode(void)
{
	return s_flash_runtime_line_mode;
}

bk_err_t bk_flash_set_clk_dpll(void)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	sys_drv_flash_set_dpll();
	flash_hal_set_clk_dpll(&s_flash.hal);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);
	return BK_OK;
}

bk_err_t bk_flash_set_clk_dco(void)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	sys_drv_flash_set_dco();
	bool ate_enabled = ate_is_enabled();
	flash_hal_set_clk_dco(&s_flash.hal, ate_enabled);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);

	return BK_OK;
}

bk_err_t bk_flash_set_clk_freq(uint32_t flash_clk_freq)
{
	switch (flash_clk_freq) {
	case FLASH_CLK_FREQ_26M_HZ:
	case FLASH_CLK_FREQ_48M_HZ:
	case FLASH_CLK_FREQ_40M_HZ:
	case FLASH_CLK_FREQ_60M_HZ:
	case FLASH_CLK_FREQ_80M_HZ:
	case FLASH_CLK_FREQ_120M_HZ:
		break;
	default:
		FLASH_LOGW("clk error:invalid freq %u\r\n", flash_clk_freq);
		return BK_ERR_FLASH_INVALID_CLK_PARAM;
	}

	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	sys_drv_flash_set_clk_freq(flash_clk_freq);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);

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

	uint32_t int_level = flash_enter_critical();
	if((sys_drv_flash_get_clk_sel() == flash_src_clk) && (sys_drv_flash_get_clk_div() == flash_dpll_div)) {
		flash_exit_critical(int_level);
		return BK_OK;
	}
	if (FLASH_CLK_DPLL == flash_src_clk) {
		sys_drv_flash_set_clk_div(flash_dpll_div);
	}
	sys_drv_flash_cksel(flash_src_clk);
	flash_exit_critical(int_level);

	return BK_OK;
}

bk_err_t bk_flash_clk_switch(uint32_t flash_speed_type, uint32_t modules)
{
	uint32_t int_level = flash_enter_critical();
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
	flash_exit_critical(int_level);

	return BK_OK;
}
#endif

bk_err_t bk_flash_write_enable(void)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	flash_hal_write_enable(&s_flash.hal);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);
	return BK_OK;
}

bk_err_t bk_flash_write_disable(void)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	flash_hal_write_disable(&s_flash.hal);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);
	return BK_OK;
}

uint16_t bk_flash_read_status_reg(void)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	uint16_t sr_data = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);
	return sr_data;
}

bk_err_t bk_flash_write_status_reg(uint16_t status_reg_data)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
	flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg_data);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);
	return BK_OK;
}

flash_protect_type_t bk_flash_get_protect_type(void)
{
	uint32_t type = 0;
	uint16_t protect_value = 0;

	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);
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

	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);

	return type;
}

bk_err_t bk_flash_set_protect_type(flash_protect_type_t type)
{
	uint32_t int_level = flash_enter_critical();
	flash_ps_suspend(NORMAL_PS);

	if (FLASH_PROTECT_NONE == type) {
		flash_unprotected();
	} else {
		flash_protected();
	}

	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);

	return BK_OK;
}

bk_err_t bk_flash_set_protect_none_full(void)
{
	uint32_t int_level = flash_enter_critical();

	flash_ps_suspend(NORMAL_PS);
	flash_set_protect_type(FLASH_PROTECT_NONE);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);

	return BK_OK;
}

bk_err_t bk_flash_set_protect_type_full(flash_protect_type_t type)
{
	uint32_t int_level = flash_enter_critical();

	flash_ps_suspend(NORMAL_PS);
	flash_set_protect_type(type);
	flash_ps_resume(NORMAL_PS);
	flash_exit_critical(int_level);

	return BK_OK;
}

/* This API is not used in bk7256xx */
void flash_ps_pm_init(void)
{
	PM_STATUS *flash_ps_status_ptr = &flash_ps_status;

	bk_flash_set_clk_dco();
	bk_flash_get_id();

	flash_ps_status_ptr->bits.unconditional_ps_support = 1;
	flash_ps_status_ptr->bits.unconditional_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.unconditional_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.unconditional_ps_sleeped = 0;
	flash_ps_status_ptr->bits.normal_ps_support = 1;
	flash_ps_status_ptr->bits.normal_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.normal_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.normal_ps_sleeped = 0;
	flash_ps_status_ptr->bits.lowvol_ps_support = 1;
	flash_ps_status_ptr->bits.lowvol_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.lowvol_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.lowvol_ps_sleeped = 0;
	flash_ps_status_ptr->bits.deep_ps_support = 1;
	flash_ps_status_ptr->bits.deep_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.deep_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.deep_ps_sleeped = 0;

	dev_pm_register(PM_ID_FLASH, "flash", &flash_ps_ops);
}

bool bk_flash_is_driver_inited()
{
	return s_flash_is_init;
}

uint32_t bk_flash_get_current_total_size(void)
{
	return s_flash.flash_cfg->flash_size;
}

bk_err_t bk_flash_register_ps_suspend_callback(flash_ps_callback_t ps_suspend_cb)
{
	s_flash_ps_suspend_cb = ps_suspend_cb;
	return BK_OK;
}

bk_err_t bk_flash_register_ps_resume_callback(flash_ps_callback_t ps_resume_cb)
{
	s_flash_ps_resume_cb = ps_resume_cb;
	return BK_OK;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_enter_deep_sleep(void)
{
#if CONFIG_SOC_BK7236XX
	int ret = 0;
	uint8_t op_code = FLASH_CMD_ENTER_DEEP_PWR_DW;

	// flash need to change 2 line when do flash operate except read
	// need to recover 4 line, please do it manually
	//if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
	//	bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);

	ret = flash_bypass_op_write_iram(&op_code, NULL, 0);
	if(ret == 0)// success
	{
		// delay T_dp: 3us
		//for(volatile int j=0; j<500; j++);
		return BK_OK;
	}
#endif
	return BK_FAIL;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_exit_deep_sleep(void)
{
#if CONFIG_SOC_BK7236XX
	int ret = 0;
	uint8_t op_code = FLASH_CMD_EXIT_DEEP_PWR_DW;

	// flash need to change 2 line when do flash operate except read
	// need to recover 4 line, please do it manually
	//if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
	//	bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);

	ret = flash_bypass_op_write_iram(&op_code, NULL, 0);
	if(ret == 0)// success
	{
		// delay T_res1: 20us
		//for(volatile int j=0; j<500; j++);
		return BK_OK;
	}
#endif
	return BK_FAIL;
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

void bk_flash_display_config_info(const char *point)
{
	uint32_t int_level = flash_enter_critical();
	uint32_t l_flash_protect_count = flash_protect_count;

	flash_ps_suspend(NORMAL_PS);
	uint16_t status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	flash_ps_resume(NORMAL_PS);

	flash_exit_critical(int_level);
	(void)point;
	(void)l_flash_protect_count;
	(void)status_reg;
}

#endif /* CONFIG_FLASH_V1P1 */
