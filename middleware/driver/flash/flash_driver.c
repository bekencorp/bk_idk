// Copyright 2020-2026 Beken
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
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#include "flash_bypass.h"

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX)
#include "partitions_gen.h"
#endif

typedef struct {
    flash_hal_t            hal;
    uint32_t               flash_id;
    uint32_t               flash_status_reg_val;
    uint32_t               flash_line_mode;
    const flash_config_t * flash_cfg;
} flash_driver_t;

typedef struct {
    uint32_t     reg16;
    uint32_t     reg17;
} flash_ctrl_context_t;

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

__DRAM_SEC static const flash_config_t flash_config[] = {
    /* flash_id, flash_size,    status_reg_size, line_mode,            cmp_post, protect_post, protect_mask, protect_all, protect_none, unprotect_last_block. quad_en_post, quad_en_val, coutinuous_read_mode_bits_val   */
    {0x1C7016,   FLASH_SIZE_4M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x01B,                9,            1,           0xA5,                         }, //en_25qh32b
    {0x1C7015,   FLASH_SIZE_2M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x0d,                 9,            1,           0xA5,                         }, //en_25qh16b
    {0x0B4014,   FLASH_SIZE_1M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_25f08b
    {0x0B4015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_25f16b
    {0x0B4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_25f32b
    {0x0B4017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x05,        0x00,         0x109,                9,            1,           0xA0,                         }, //xtx_25f64b
    {0x0B6017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xt_25q64d
    {0x0B6018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xt_25q128d
    {0x0B4018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xt_25F128F-W
    {0x0E4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_FT25H32
    {0x1C4116,   FLASH_SIZE_4M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //en_25qe32a(not support 4 line)
    {0x5E5018,   FLASH_SIZE_16M,  1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //zb_25lq128c
    {0xC84015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //gd_25q16c
    {0xC84017,   FLASH_SIZE_8M,   1,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //gd_25q16c
    {0xC84016,   FLASH_SIZE_4M,   3,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //gd_25q32c
    {0xC86018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //gd_25lq128e
    {0xC86515,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //gd_25w16e
    {0xC86516,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //gd_25wq32e
    {0xEF4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //w_25q32(bfj)
    {0x204118,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xm_25qu128c
    {0x204017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xmc_25qh64d
    {0x204016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xmc_25qh32b
    {0xC22315,   FLASH_SIZE_2M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                6,            1,           0xA5,                         }, //mx_25v16b
    {0xEB6015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //zg_th25q16b
    {0xC86517,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //gd_25Q32E
    {0xCD6016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //th_25q32ub
    {0xCD6017,   FLASH_SIZE_8M,   3,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //th_25q64ha
    {0x000000,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_TWO,    0,        2,            0x1F,         0x00,        0x00,         0x000,                0,            0,           0x00,                         }, //default
};

static flash_driver_t s_flash = {0};
static bool s_flash_is_init = false;

extern bk_err_t mb_flash_ipc_init(void);
extern bk_err_t mb_flash_op_prepare(void);
extern bk_err_t mb_flash_op_finish(void);
extern bk_err_t bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code);
extern int xTaskResumeAll( void );
extern void vTaskSuspendAll( void );

static inline uint32_t flash_enter_critical()
{
    uint32_t flags = rtos_disable_int();

    return flags;
}

static inline void flash_exit_critical(uint32_t flags)
{
    rtos_enable_int(flags);
}

void flash_lock(void)
{
    BK_ASSERT(0 == rtos_is_in_interrupt_context());
    if(rtos_local_irq_disabled())
    {
        return;
    }

    /*disable scheduler for special device, contact with zili.guo/ling.zhou*/
    vTaskSuspendAll();

    mb_flash_op_prepare();
}

void flash_unlock(void)
{
    BK_ASSERT(0 == rtos_is_in_interrupt_context());
    if(rtos_local_irq_disabled())
    {
        return;
    }

    mb_flash_op_finish();

    xTaskResumeAll();
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

static uint32_t flash_read_status_reg(void)
{
    uint32_t status_reg;
    uint32_t int_level;

    int_level = flash_enter_critical();
    status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
    flash_exit_critical(int_level);

    return status_reg;
}

static void flash_write_status_reg(uint32_t status_reg_val)
{
    uint32_t int_level = flash_enter_critical();

#if CONFIG_FLASH_WRITE_STATUS_VOLATILE
    flash_hal_set_volatile_status_write(&s_flash.hal);
#endif

    s_flash.flash_status_reg_val = status_reg_val;

    flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg_val);

#if CONFIG_FLASH_WRITE_STATUS_VOLATILE
    flash_hal_clear_volatile_status_write(&s_flash.hal);
#endif

    flash_exit_critical(int_level);
}

static uint32_t flash_get_id(void)
{
    uint32_t flash_id;
    uint32_t int_level;

    int_level = flash_enter_critical();
    flash_id = flash_hal_get_id(&s_flash.hal);
    flash_exit_critical(int_level);

    return flash_id;
}

static uint32_t flash_get_protect_cfg(flash_protect_type_t type)
{
    switch (type) {
    case FLASH_PROTECT_NONE:
        return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_none);
    case FLASH_PROTECT_ALL:
        return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_all);
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

static flash_protect_type_t flash_get_protect_type(uint32_t sr_value)
{
    uint32_t type = 0;
    uint16_t protect_value = 0;
    uint16_t cmp;

    protect_value = sr_value >> s_flash.flash_cfg->protect_post;
    protect_value = protect_value & s_flash.flash_cfg->protect_mask;

    cmp = (sr_value >> s_flash.flash_cfg->cmp_post) & FLASH_CMP_MASK;
    protect_value |= cmp << FLASH_STATUS_REG_PROTECT_OFFSET;

    if (protect_value == s_flash.flash_cfg->protect_all)
        type = FLASH_PROTECT_ALL;
    else if (protect_value == s_flash.flash_cfg->protect_none)
        type = FLASH_PROTECT_NONE;
    else if (protect_value == s_flash.flash_cfg->unprotect_last_block)
        type = FLASH_UNPROTECT_LAST_BLOCK;
    else
        type = FLASH_PROTECT_ALL;  // FLASH_UNPROTECT_LAST_BLOCK ???

    return type;
}

void flash_set_protect_type(flash_protect_type_t type)
{
    uint32_t protect_cfg;
    uint32_t cmp_cfg;
    uint32_t status_reg = s_flash.flash_status_reg_val;

    protect_cfg = flash_get_protect_cfg(type);
    cmp_cfg = flash_get_cmp_cfg(type);

#if CONFIG_FLASH_SUPPORT_MULTI_PE  /* multiple process element. (multi-cores or SPE/NSPE) */
    status_reg = flash_read_status_reg();
#endif

    if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
        flash_set_protect_cfg(&status_reg, protect_cfg);
        flash_set_cmp_cfg(&status_reg, cmp_cfg);

        //FLASH_LOGD("write status reg:%x, status_reg_size:%d\r\n", status_reg, s_flash.flash_cfg->status_reg_size);
        flash_write_status_reg(status_reg);
    }
}

static void flash_set_qe(void)
{
    uint32_t status_reg = s_flash.flash_status_reg_val;

    #if CONFIG_FLASH_SUPPORT_MULTI_PE
    status_reg = flash_read_status_reg();
    #endif
    if (((status_reg >> s_flash.flash_cfg->quad_en_post) & 0x01) == s_flash.flash_cfg->quad_en_val) {
        return;
    }

    if (1 == s_flash.flash_cfg->quad_en_val)
        status_reg |= (1 << s_flash.flash_cfg->quad_en_post);
    else
        status_reg &= ~(1 << s_flash.flash_cfg->quad_en_post);

    flash_write_status_reg(status_reg);
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
        uint32_t int_level;

        /*disable interrupt for special device, contact with zili.guo/ling.zhou*/
        int_level = flash_enter_critical();
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
    uint32_t *pb = (uint32_t *)&buf[0];

    if (len == 0) {
        return;
    }

    while (len) {
        uint32_t int_level;

        /*disable interrupt for special device, contact with zili.guo/ling.zhou*/
        int_level = flash_enter_critical();
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

bk_err_t flash_write_common(const uint8_t *buffer, uint32_t address, uint32_t len)
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

        for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
            flash_hal_write_data(&s_flash.hal, buf[i]);
        }
        flash_hal_set_op_cmd_write(&s_flash.hal, addr);
        flash_exit_critical(int_level);

        addr += FLASH_BYTES_CNT;
    }
    return BK_OK;
}

bk_err_t flash_erase_block(uint32_t address, int type)
{
    uint32_t int_level = flash_enter_critical();

    flash_hal_erase_block(&s_flash.hal, address, type);

    flash_exit_critical(int_level);

    return BK_OK;
}

__IRAM_SEC flash_line_mode_t flash_set_line_mode(flash_line_mode_t line_mode)
{
    uint32_t int_level = flash_enter_critical();

    flash_line_mode_t  new_line_mode;
    flash_line_mode_t  old_line_mode = s_flash.flash_line_mode;

#if CONFIG_FLASH_QUAD_ENABLE
    if ( (FLASH_LINE_MODE_FOUR == line_mode)
        && (FLASH_LINE_MODE_FOUR == s_flash.flash_cfg->line_mode) )
    {
        new_line_mode = FLASH_LINE_MODE_FOUR;
    }
    else
#endif
    {
        new_line_mode = FLASH_LINE_MODE_TWO;
    }

    if(new_line_mode == old_line_mode)
    {
        flash_exit_critical(int_level);
        return old_line_mode;
    }

    flash_hal_clear_qwfr(&s_flash.hal);   // cmd CRMR (coutinuous_read_mode reset), quit QPI mode.

#if CONFIG_SOC_BK7236XX
    sys_drv_set_sys2flsh_2wire(0);
#endif

    if (FLASH_LINE_MODE_FOUR == new_line_mode)
    {
        flash_hal_set_quad_m_value(&s_flash.hal, s_flash.flash_cfg->coutinuous_read_mode_bits_val);
        flash_set_qe();
        flash_hal_set_mode(&s_flash.hal, FLASH_MODE_QUAD);  // enter QPI mode.
    }
    else
    {
        flash_hal_set_mode(&s_flash.hal, FLASH_MODE_DUAL);
    }

    s_flash.flash_line_mode = new_line_mode;

#if CONFIG_SOC_BK7236XX
    sys_drv_set_sys2flsh_2wire(1);
#endif

    flash_exit_critical(int_level);

    return old_line_mode;

}

flash_line_mode_t bk_flash_get_line_mode(void)
{
    return s_flash.flash_cfg->line_mode;
}

uint8_t bk_flash_get_coutinuous_read_mode(void)
{
    return s_flash.flash_cfg->coutinuous_read_mode_bits_val;
}

bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode)
{
    return BK_OK;

    (void)line_mode;
}

bk_err_t flash_clear_rd_residual_data(flash_hal_t *hal)
{
    uint32_t tmp_val;
    uint32_t residual_cnt, read_cnt;

    /*
     * data_sw_flash_sel (bits[21:19]): Software-to-flash data selector.
     * - Read-only field reporting the current data buffer segment being accessed.
     * - 3-bit selector that picks a 32-bit chunk out of the 256-bit buffer.
     * - Reset/default value: 0x7 (3'h7), held during reset and while IDLE.
     * - Selector mapping:
     *     0x0 → buf_sw_flash[31:0]  (most recent write)
     *     0x1 → buf_sw_flash[63:32]
     *     0x2 → buf_sw_flash[95:64]
     *     ...
     *     0x7 → buf_sw_flash[255:224] (oldest entry)
     */
    read_cnt = flash_hal_read_data_sw_flash_sel(hal);
    residual_cnt = FLASH_BYTE_SEL_RD_DEFAULT_VAL - read_cnt;

    if(FLASH_BYTE_SEL_RD_DEFAULT_VAL == read_cnt){
        goto clear_out;
    }
    FLASH_LOGE("flash_clear_rd_residual_data: residual_cnt=%d\r\n", residual_cnt);

    while(residual_cnt--){
        tmp_val = flash_hal_read_data(hal);
    }
    (void)tmp_val;

clear_out:
    return BK_OK;
}

bk_err_t bk_flash_driver_init(void)
{
    flash_hal_t *hal_ptr;

    if (s_flash_is_init) {
        return BK_OK;
    }

    bk_err_t ret_code = mb_flash_ipc_init();  /* used for projects with LCD. */
    if(ret_code != BK_OK)
        return ret_code;

#if (CONFIG_CPU_CNT > 1)
    extern bk_err_t bk_flash_svr_init(void);
    ret_code = bk_flash_svr_init();
    if(ret_code != BK_OK)
    {
        BK_LOGE("Flash", "flash svr create failed %d.\r\n", ret_code);
    }
#endif

    os_memset(&s_flash, 0, sizeof(s_flash));

    hal_ptr = &s_flash.hal;
    flash_hal_init(hal_ptr);

    flash_hal_disable_cpu_data_wr(hal_ptr);
    flash_set_line_mode(FLASH_LINE_MODE_TWO);

    s_flash.flash_id = flash_get_id();
    FLASH_LOGI("ID=0x%X\r\n", s_flash.flash_id);

    flash_get_current_config();

    flash_hal_set_quad_m_value(hal_ptr, s_flash.flash_cfg->coutinuous_read_mode_bits_val);

    s_flash.flash_status_reg_val = flash_read_status_reg();

    flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

    flash_set_line_mode(s_flash.flash_cfg->line_mode);

    flash_hal_set_default_clk(hal_ptr);

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX)
    #if (CONFIG_SOC_BK7236N) || (CONFIG_SOC_BK7239XX)
    #if CONFIG_ATE_TEST
    #if (CONFIG_EXT_FLASH_CLK_FREQ == 40000000)
    if((0 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
        sys_drv_flash_set_clk_div(0); // XTAL= 40M
        sys_drv_flash_cksel(0);
    }
    #elif (CONFIG_EXT_FLASH_CLK_FREQ == 80000000)
    if((2 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
        sys_drv_flash_set_clk_div(0); // 80M div 1 = 80M
        sys_drv_flash_cksel(2);
    }
    #elif (CONFIG_EXT_FLASH_CLK_FREQ == 120000000)
    if((3 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
        sys_drv_flash_set_clk_div(0); // 120M div 1 = 120M
        sys_drv_flash_cksel(3);
    }
    #else
    if((0 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
        sys_drv_flash_set_clk_div(0); // XTAL= 40M
        sys_drv_flash_cksel(0);
    }
    #endif
    #else
    if((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_GD || (s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_TH) {
        if((2 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
            sys_drv_flash_set_clk_div(0); // 80M div 1 = 80M
            sys_drv_flash_cksel(2);
        }
    } else {
        if((0 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
            sys_drv_flash_set_clk_div(0); // XTAL= 40M
            sys_drv_flash_cksel(0);
        }
    }
    #endif
    #else
    if((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_GD || (s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_TH) {
        if((1 != sys_drv_flash_get_clk_sel()) || (1 != sys_drv_flash_get_clk_div())) {
            #if (CONFIG_FLASH_CLK_120M)
            sys_drv_flash_set_clk_div(0); // dpll div 4 = 120M
            #else
            sys_drv_flash_set_clk_div(1); // dpll div 6 = 80M
            #endif
            sys_drv_flash_cksel(1);
        }
    }
    else
    {
        if((1 != sys_drv_flash_get_clk_sel()) || (3 != sys_drv_flash_get_clk_div())) {
            sys_drv_flash_set_clk_div(3); // dpll div 10 = 48M
            sys_drv_flash_cksel(1);
        }
    }
    #endif
    sys_drv_set_sys2flsh_2wire(1);
#endif

    /* If start flash read, and the 8-word data was not fully read out
     * from the flash fifo. and then a next flash read is initiated again
     * the data will no be read correctly for hw fifo pointer is wrong.
     */
    flash_clear_rd_residual_data(hal_ptr);

    s_flash_is_init = true;

    return BK_OK;
}

bk_err_t bk_flash_driver_deinit(void)
{
    if (!s_flash_is_init) {
        return BK_OK;
    }

    s_flash_is_init = false;

    return BK_OK;
}

static bk_err_t flash_erase_no_lock(uint32_t address, int cmd)
{
    if (address >= s_flash.flash_cfg->flash_size) {
        FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    uint32_t  erase_size = 0;

    if(cmd == FLASH_OP_CMD_SE)
        erase_size = FLASH_SECTOR_SIZE;
    else if(cmd == FLASH_OP_CMD_BE1)
        erase_size = FLASH_BLOCK32_SIZE;
    else if(cmd == FLASH_OP_CMD_BE2)
        erase_size = FLASH_BLOCK_SIZE;
    else
        return BK_FAIL;

    uint32_t erase_addr = address & (~(erase_size - 1));

    bk_err_t    ret_val = BK_FAIL;

    flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);

    uint32_t  status_reg = s_flash.flash_status_reg_val;
    #if CONFIG_FLASH_SUPPORT_MULTI_PE
    status_reg = flash_read_status_reg();
    #endif

    flash_protect_type_t partition_type = flash_get_protect_type(status_reg);

    if(bk_flash_partition_write_perm_check_by_addr(erase_addr, erase_size, FLASH_API_MAGIC_CODE) == BK_OK)
    {
        flash_set_protect_type(FLASH_PROTECT_NONE);

        if(bk_flash_partition_write_perm_check_by_addr(erase_addr, erase_size, FLASH_API_MAGIC_CODE) == BK_OK)
            ret_val = flash_erase_block(address, cmd);
    }

    flash_set_protect_type(partition_type);
    flash_set_line_mode(old_line_mode);

    return ret_val;
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
    if (address >= s_flash.flash_cfg->flash_size) {
        FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    flash_lock();
    bk_err_t ret_val = flash_erase_no_lock(address, FLASH_OP_CMD_SE);
    flash_unlock();

    return ret_val;
}

bk_err_t bk_flash_erase_32k(uint32_t address)
{
    if (address >= s_flash.flash_cfg->flash_size) {
        FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    flash_lock();
    bk_err_t ret_val = flash_erase_no_lock(address, FLASH_OP_CMD_BE1);
    flash_unlock();

    return ret_val;
}

bk_err_t bk_flash_erase_block(uint32_t address)
{
    if (address >= s_flash.flash_cfg->flash_size) {
        FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    flash_lock();
    bk_err_t ret_val = flash_erase_no_lock(address, FLASH_OP_CMD_BE2);
    flash_unlock();

    return ret_val;
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
    if (address >= s_flash.flash_cfg->flash_size) {
        FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    bk_err_t ret_val = BK_FAIL;

    flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);

    uint32_t  status_reg = s_flash.flash_status_reg_val;
    #if CONFIG_FLASH_SUPPORT_MULTI_PE
    status_reg = flash_read_status_reg();
    #endif

    flash_protect_type_t partition_type = flash_get_protect_type(status_reg);

    if(bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK)
    {
        flash_set_protect_type(FLASH_PROTECT_NONE);

        if(bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK)
            ret_val = flash_write_common(user_buf, address, size);
    }

    flash_set_protect_type(partition_type);
    flash_set_line_mode(old_line_mode);

    return ret_val;
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
    if (address >= s_flash.flash_cfg->flash_size) {
        FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    flash_lock();
    bk_err_t ret_val = flash_write_no_lock(address, user_buf, size);
    flash_unlock();

    return ret_val;
}

uint32_t bk_flash_get_id(void)
{
    return s_flash.flash_id;
}

bk_err_t bk_flash_set_clk_dpll(void)
{
    sys_drv_flash_set_dpll();
    flash_hal_set_clk_dpll(&s_flash.hal);

    return BK_OK;
}

bk_err_t bk_flash_set_clk_dco(void)
{
    sys_drv_flash_set_dco();
    bool ate_enabled = ate_is_enabled();
    flash_hal_set_clk_dco(&s_flash.hal, ate_enabled);

    return BK_OK;
}

#if CONFIG_FLASH_TEST
bk_err_t bk_flash_write_enable(void)
{
    return BK_OK;
}

bk_err_t bk_flash_write_disable(void)
{
    return BK_OK;
}

uint16_t bk_flash_read_status_reg(void)
{
    #if CONFIG_FLASH_SUPPORT_MULTI_PE
    flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);
    uint16_t sr_data = flash_read_status_reg();
    flash_set_line_mode(old_line_mode);
    return sr_data;
    #else
    return s_flash.flash_status_reg_val;
    #endif
}

bk_err_t bk_flash_write_status_reg(uint16_t status_reg_data)
{
    return BK_OK;
}

uint32_t bk_flash_get_crc_err_num(void)
{
    return flash_hal_get_crc_err_num(&s_flash.hal);
}
#endif

flash_protect_type_t bk_flash_get_protect_type(void)
{

    return FLASH_PROTECT_ALL;
}

bk_err_t bk_flash_set_protect_type(flash_protect_type_t type)
{
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

bk_err_t bk_flash_register_ps_suspend_callback(flash_ps_callback_t ps_suspend_cb)
{
    return BK_OK;
}

bk_err_t bk_flash_register_ps_resume_callback(flash_ps_callback_t ps_resume_cb)
{
    return BK_OK;
}

__IRAM_SEC bk_err_t bk_flash_power_saving_enter(void)
{
    // save flash ctrl setting to flash_ctrl_context;
    flash_set_line_mode(FLASH_LINE_MODE_TWO);

    return BK_OK;
}

__IRAM_SEC bk_err_t bk_flash_power_saving_exit(void)
{
    // restore flash ctrl setting from flash_ctrl_context;
    // the restore API must run in SRAM/ITCM.
    // don't access flash before restoring setting, especially for A/B image project.

    // update flash status register value, for volatile way reset flash status register value
    s_flash.flash_status_reg_val = flash_read_status_reg();

    s_flash.flash_line_mode = 0;
    flash_set_line_mode(s_flash.flash_cfg->line_mode);

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
    //    bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);

    ret = flash_bypass_op_write(&op_code, NULL, 0);
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
    //    bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);

    ret = flash_bypass_op_write(&op_code, NULL, 0);
    if(ret == 0)// success
    {
        // delay T_res1: 20us
        //for(volatile int j=0; j<500; j++);
        return BK_OK;
    }
#endif
    return BK_FAIL;
}

/* flash dump APIs are called in context of interrupt disabled. */
bk_err_t bk_flash_dump_erase_sector(uint32_t address)
{
    return flash_erase_no_lock(address, FLASH_OP_CMD_SE);
}

bk_err_t bk_flash_dump_write(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
    return flash_write_no_lock(address, user_buf, size);
}

static inline bool is_64k_aligned(uint32_t addr)
{
    return ((addr & (KB(64) - 1)) == 0);
}

static inline bool is_32k_aligned(uint32_t addr)
{
    return ((addr & (KB(32) - 1)) == 0);
}

/*make sure wdt is closed!*/
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

#if CONFIG_DIRECT_XIP
uint32_t flash_get_excute_enable(void)
{
    return flash_hal_read_offset_enable(&s_flash.hal);
}
#endif

__attribute__((section(".iram"))) void bk_flash_enable_cpu_data_wr(void)
{
    flash_hal_enable_cpu_data_wr(&s_flash.hal);
}

__attribute__((section(".iram"))) void bk_flash_disable_cpu_data_wr(void)
{
    flash_hal_disable_cpu_data_wr(&s_flash.hal);
}

#if CONFIG_FLASH_CBUS
#include "partitions.h"
#include "CheckSumUtils.h"
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include "bk_wdt.h"
#endif

#ifndef CEIL_ALIGN_34
#define CEIL_ALIGN_34(addr)           (((addr) + 34 - 1) / 34 * 34)
#endif

bk_err_t bk_flash_check_crc(uint32_t phy_offset, uint32_t phy_size);

__attribute__((section(".iram"))) static void bk_flash_wait_busy(void)
{
    while(flash_hal_is_busy(&s_flash.hal));
}

static inline void flash_set_qwfr(void)
{
    flash_hal_set_mode(&s_flash.hal, 2);
}

static inline void flash_enable_line_mode_switch(bool enable)
{
    if (enable) {
        sys_drv_set_sys2flsh_2wire(0);
    } else {
        sys_drv_set_sys2flsh_2wire(1);
    }
}

__attribute__((section(".iram")))
void cbus_flash_set_line_mode(flash_line_mode_t line_mode)
{
    flash_hal_clear_qwfr(&s_flash.hal);
    flash_enable_line_mode_switch(true);
    if (FLASH_LINE_MODE_TWO == line_mode) {
        flash_hal_set_dual_mode(&s_flash.hal);
    } else if (FLASH_LINE_MODE_FOUR == line_mode) {
        flash_hal_set_quad_m_value(&s_flash.hal, s_flash.flash_cfg->coutinuous_read_mode_bits_val);
        if (1 == s_flash.flash_cfg->quad_en_val) {
            flash_set_qe();
        }
        flash_set_qwfr();
    }
    flash_enable_line_mode_switch(false);
}

__attribute__((noinline, section(".iram")))
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

__attribute__((section(".iram")))
static void flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
    bk_flash_enable_cpu_data_wr();
    flash_memcpy((char*)(0x02000000+address), (const char*)user_buf,size);
    bk_flash_wait_busy();
    bk_flash_disable_cpu_data_wr();
}

__attribute__((section(".iram")))
void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
    uint32_t line_mode = bk_flash_get_line_mode();
    uint32_t int_status =  rtos_disable_int();
    cbus_flash_set_line_mode(2);
    uint32_t status_reg = flash_read_status_reg();
    flash_protect_type_t partition_type = flash_get_protect_type(status_reg);
    flash_set_protect_type(FLASH_PROTECT_NONE);

#if CONFIG_CACHE_ENABLE
    enable_dcache(0);
#endif
    flash_write_cbus(address, user_buf, size);
#if CONFIG_CACHE_ENABLE
    enable_dcache(1);
#endif
    flash_set_protect_type(partition_type);
    cbus_flash_set_line_mode(line_mode);
    rtos_enable_int(int_status);
}

__attribute__((section(".iram")))
void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
    flash_memcpy((char*)user_buf, (const char*)(0x02000000+address),size);
}

#if CONFIG_DIRECT_XIP
__attribute__((section(".iram")))
static void bk_flash_xip_write_cbus(uint32_t off, const void *src, uint32_t len)
{
    uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_XIP_A_PHY_PARTITION_OFFSET));

    if((fa_off+off) & 0x31)  // MUST aligh with 32-byte.
        return;

    uint32_t write_addr = (fa_off + off);
    write_addr |= 1 << 24;
    bk_flash_write_cbus(write_addr, src, len);
}
#endif

bk_err_t bk_flash_check_crc(uint32_t phy_offset, uint32_t phy_size)
{
    typedef struct flash_data {
        uint8_t data[32];
        uint16_t crc16;
    } flash_data_t;

    flash_data_t enc_data;
    uint32_t ff_buf[34];

    phy_offset = CEIL_ALIGN_34(phy_offset);

    for (size_t i = 0; i < phy_size / 34; i++) {
        bk_flash_read_bytes(phy_offset + i*34, (uint8_t *)&enc_data, 34);
        os_memset(ff_buf, 0xFF, 34);
        if (os_memcmp(ff_buf, (const void *)&enc_data, 34) == 0) {
            continue;
        }
        if (enc_data.crc16 != calculate_crc16(enc_data.data, 32)) {
            FLASH_LOGE("read crc = %#x,cal crc =%#x, addr=%#x",
                        enc_data.crc16, calculate_crc16(enc_data.data, 32), phy_offset + i*34);
            return BK_FAIL;
        }
    }
    return BK_OK;
}

#endif

#if CONFIG_FLASH_OTP
void bk_flash_lock(flash_line_mode_t* line_mode)
{
    flash_lock();
    * line_mode =  flash_set_line_mode(FLASH_LINE_MODE_TWO);
}

void bk_flash_unlock(flash_line_mode_t* line_mode)
{
    flash_set_line_mode(*line_mode);
    flash_unlock();
}

void bk_flash_lock_otp(uint8_t block_index)
{
    flash_lock();
    flash_line_mode_t line_mode =  flash_set_line_mode(FLASH_LINE_MODE_TWO);
    uint32_t status_reg = flash_read_status_reg();
    if (block_index == 1) {
        status_reg |= 1 << 11; //GD25WQ32E
    } else if (block_index == 2) {
        status_reg |= 1 << 12; //GD25WQ32E
    } else if (block_index == 3) {
        status_reg |= 1 << 13; //GD25WQ32E
    }
    flash_write_status_reg(status_reg);
    flash_set_line_mode(line_mode);
    flash_unlock();
    return;
}
#endif
