

#pragma once

#include "sys_types.h"
#include <driver/hal/hal_uart_types.h>
#include <driver/hal/hal_pwm_types.h>
#include <driver/hal/hal_timer_types.h>
#include <driver/hal/hal_spi_types.h>
#include <driver/sys_pm_types.h>
#include <modules/pm.h>

#if CONFIG_GPIO_CLOCK_PIN_SUPPORT
#include <driver/hal/hal_clock_types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	sys_hw_t *hw;
} sys_hal_t;


/**  Platform Start **/

//Platform

/** Platform flash Start **/
void sys_hal_flash_set_dco(void);

void sys_hal_flash_set_dpll(void);

void sys_hal_flash_set_clk(uint32_t value);

void sys_hal_flash_set_clk_div(uint32_t value);

uint32_t sys_hal_flash_get_clk_sel(void);

uint32_t sys_hal_flash_get_clk_div(void);

/** Platform flash End **/

void sys_hal_cpu0_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state);
void sys_hal_cpu1_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state);
void sys_hal_set_cpu1_boot_address_offset(uint32_t address_offset);
void sys_hal_set_cpu1_reset(uint32_t reset_value);
void sys_hal_set_cpu2_boot_address_offset(uint32_t address_offset);
void sys_hal_set_cpu2_reset(uint32_t reset_value);

void sys_hal_cpu_clk_div_set(uint32_t core_index, uint32_t value);
uint32_t sys_hal_cpu_clk_div_get(uint32_t core_index);
bk_err_t sys_hal_switch_cpu_bus_freq(pm_cpu_freq_e cpu_bus_freq);
bk_err_t sys_hal_core_bus_clock_ctrl(uint32_t cksel_core, uint32_t ckdiv_core,uint32_t ckdiv_bus, uint32_t ckdiv_cpu0,uint32_t ckdiv_cpu1);

#if CONFIG_SOC_BK7236XX
#define sys_hal_set_cpu0_rxevt_sel(param)     sys_ll_set_cpu0_int_halt_clk_op_cpu0_rxevt_sel(param)
#define sys_hal_set_cpu1_rxevt_sel(param)     sys_ll_set_cpu1_int_halt_clk_op_cpu1_rxevt_sel(param)
#define sys_hal_set_cpu2_rxevt_sel(param)     sys_ll_set_cpu2_int_halt_clk_op_cpu2_rxevt_sel(param)
#endif

int32 sys_hal_set_jtag_mode(uint32 param);
uint32 sys_hal_get_jtag_mode(void);

/*clock power control start*/
void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up);
void sys_hal_set_clk_select(dev_clk_select_id_t dev, dev_clk_select_t clk_sel);
dev_clk_select_t sys_hal_get_clk_select(dev_clk_select_id_t dev);
//DCO divider is valid for all of the peri-devices.
void sys_hal_set_dco_div(dev_clk_dco_div_t div);
//DCO divider is valid for all of the peri-devices.
dev_clk_dco_div_t sys_hal_get_dco_div(void);
/*clock power control end*/

/* UART select clock    DIRTY **/
void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode);
/* UART select clock    DIRTY **/

uint32_t sys_hal_nmi_wdt_get_clk_div(void);
void sys_hal_nmi_wdt_set_clk_div(uint32_t value);
void sys_hal_trng_disckg_set(uint32_t value);
/**  Platform End **/

void sys_hal_enable_pll_iram(void);
void sys_hal_enable_high_freq_iram(void);

#ifdef __cplusplus
}
#endif


