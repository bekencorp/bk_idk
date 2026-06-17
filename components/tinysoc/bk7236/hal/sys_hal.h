
#pragma once

#include <stdbool.h>
#include "sys_types.h"
#include <driver/hal/hal_uart_types.h>
#include <driver/hal/hal_pwm_types.h>
#include <driver/hal/hal_timer_types.h>
#include <driver/hal/hal_spi_types.h>
#include <driver/sys_pm_types.h>
#include <modules/pm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	sys_hw_t *hw;
} sys_hal_t;

/** Platform flash **/
void sys_hal_flash_set_dco(void);
void sys_hal_flash_set_dpll(void);
void sys_hal_flash_set_clk(uint32_t value);
void sys_hal_flash_set_clk_div(uint32_t value);
uint32_t sys_hal_flash_get_clk_sel(void);
uint32_t sys_hal_flash_get_clk_div(void);

bk_err_t sys_hal_init(void);
void sys_hal_set_base_addr(uint32_t addr);
uint32 sys_hal_get_chip_id(void);
uint32 sys_hal_get_device_id(void);

void sys_hal_all_modules_clk_div_set(clk_div_reg_e reg, uint32_t value);
uint32_t sys_hal_all_modules_clk_div_get(clk_div_reg_e reg);
void sys_hal_cpu_clk_div_set(uint32_t core_index, uint32_t value);
void sys_hal_switch_freq(uint32_t cksel_core, uint32_t ckdiv_core, uint32_t ckdiv_cpu0);
bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value);

#if CONFIG_SOC_BK7236XX
#define sys_hal_set_cpu0_rxevt_sel(param)     sys_ll_set_cpu0_int_halt_clk_op_cpu0_rxevt_sel(param)
#define sys_hal_set_cpu1_rxevt_sel(param)     sys_ll_set_cpu1_int_halt_clk_op_cpu1_rxevt_sel(param)
#define sys_hal_set_cpu2_rxevt_sel(param)     sys_ll_set_cpu2_int_halt_clk_op_cpu2_rxevt_sel(param)
#endif

int32 sys_hal_set_jtag_mode(uint32 param);
uint32 sys_hal_get_jtag_mode(void);

/* clock power control */
void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up);
void sys_hal_set_clk_select(dev_clk_select_id_t dev, dev_clk_select_t clk_sel);
void sys_hal_set_clk_div(dev_clk_select_id_t dev, uint32_t clk_div);
dev_clk_select_t sys_hal_get_clk_select(dev_clk_select_id_t dev);
void sys_hal_set_dco_div(dev_clk_dco_div_t div);
dev_clk_dco_div_t sys_hal_get_dco_div(void);

void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode);
uint32_t sys_hal_uart_select_clock_get(uart_id_t id);
void sys_hal_pwm_set_clock(uint32_t mode, uint32_t param);
void sys_hal_pwm_select_clock(sys_sel_pwm_t num, pwm_src_clk_t mode);
void sys_hal_timer_select_clock(sys_sel_timer_t num, timer_src_clk_t mode);
uint32_t sys_hal_timer_select_clock_get(sys_sel_timer_t id);
void sys_hal_usb_enable_clk(bool en);
void sys_hal_qspi_clk_sel(uint32_t id, uint32_t param);
void sys_hal_qspi_set_src_clk_div(uint32_t id, uint32_t value);
void sys_hal_psram_clk_sel(uint32_t value);
void sys_hal_psram_set_clkdiv(uint32_t value);
void sys_hal_i2s_select_clock(uint32_t value);
void sys_hal_aud_clock_en(uint32_t value);
void sys_hal_i2s_clock_en(uint32_t value);
void sys_hal_i2s1_clock_en(uint32_t value);
void sys_hal_i2s2_clock_en(uint32_t value);
void sys_hal_fft_disckg_set(uint32_t value);
void sys_hal_i2s_disckg_set(uint32_t value);
void sys_hal_set_yuv_buf_clock_en(uint32_t value);
void sys_hal_set_h264_clock_en(uint32_t value);
uint32_t sys_hal_nmi_wdt_get_clk_div(void);
void sys_hal_nmi_wdt_set_clk_div(uint32_t value);
void sys_hal_trng_disckg_set(uint32_t value);

void sys_hal_sadc_pwr_up(void);
void sys_hal_sadc_pwr_down(void);
void sys_hal_set_sdio_clk_en(uint32_t value);
void sys_hal_set_sdio_clk_div(uint32_t value);
uint32_t sys_hal_get_sdio_clk_div(void);
void sys_hal_set_sdio_clk_sel(uint32_t value);
uint32_t sys_hal_get_sdio_clk_sel(void);

void sys_hal_en_tempdet(uint32_t value);
uint32_t sys_hal_get_cpu_storage_connect_op_select_flash_sel(void);
void sys_hal_set_cpu_storage_connect_op_select_flash_sel(uint32_t value);
void sys_hal_set_bts_wakeup_platform_en(uint32_t value);
uint32_t sys_hal_get_bts_wakeup_platform_en(void);
void sys_hal_set_bts_sleep_exit_req(uint32_t value);
uint32_t sys_hal_get_bts_sleep_exit_req(void);
void sys_hal_set_sys2flsh_2wire(uint32_t value);

void sys_hal_set_ana_trxt_tst_enable(uint32_t value);
void sys_hal_set_ana_scal_en(uint32_t value);
void sys_hal_set_ana_gadc_buf_ictrl(uint32_t value);
void sys_hal_set_ana_gadc_cmp_ictrl(uint32_t value);
void sys_hal_set_ana_pwd_gadc_buf(uint32_t value);
void sys_hal_set_ana_hres_sel0v9(uint32_t value);
void sys_hal_set_ana_vref_sel(uint32_t value);
void sys_hal_set_ana_cb_cal_manu(uint32_t value);
void sys_hal_set_ana_cb_cal_trig(uint32_t value);
uint32_t sys_hal_get_ana_cb_cal_manu_val(void);
void sys_hal_set_ana_cb_cal_manu_val(uint32_t value);
void sys_hal_set_ana_reg11_apfms(uint32_t value);
void sys_hal_set_ana_reg12_dpfms(uint32_t value);
void sys_hal_set_ana_vlsel_ldodig(uint32_t value);
void sys_hal_set_ana_vhsel_ldodig(uint32_t value);
void sys_hal_set_ana_vctrl_sysldo(uint32_t value);
void sys_hal_set_ana_vtempsel(uint32_t value);
void sys_hal_set_ioldo_lp(uint32_t value);
void sys_hal_set_slcd_com_enable(uint32_t value);
void sys_hal_set_slcd_seg_enable(uint32_t value);
uint32_t sys_hal_get_slcd_seg_enable_status(void);
void sys_hal_set_slcd_crb(uint32_t value);
void sys_hal_set_slcd_sw_bias(uint32_t value);
void sys_hal_set_slcd_enable(uint32_t value);

void sys_hal_analog_set(analog_reg_t reg, uint32_t value);
uint32_t sys_hal_analog_get(analog_reg_t reg);
void sys_hal_cali_dpll_spi_trig_disable(void);
void sys_hal_cali_dpll_spi_trig_enable(void);
void sys_hal_cali_dpll_spi_detect_disable(void);
void sys_hal_cali_dpll_spi_detect_enable(void);
uint32_t sys_hal_cali_dpll(uint32_t param);
void sys_hal_mclk_mux_set(uint32_t value);
void sys_hal_mclk_div_set(uint32_t value);

int32 sys_hal_int_disable(uint32 param);
int32 sys_hal_int_enable(uint32 param);
int32 sys_hal_int_group2_disable(uint32 param);
int32 sys_hal_int_group2_enable(uint32 param);
int32 sys_hal_fiq_disable(uint32 param);
int32 sys_hal_fiq_enable(uint32 param);
int32 sys_hal_global_int_enable(uint32 param);
uint32 sys_hal_get_int_status(void);
uint32_t sys_hal_get_cpu0_gpio_int_st(void);
int32 sys_hal_set_int_status(uint32 param);
uint32 sys_hal_get_fiq_reg_status(void);
uint32 sys_hal_set_fiq_reg_status(uint32 param);
uint32 sys_hal_get_intr_raw_status(void);
uint32 sys_hal_set_intr_raw_status(uint32 param);
void sys_hal_sadc_int_enable(void);
void sys_hal_sadc_int_disable(void);
void sys_hal_cpu_fft_int_en(uint32_t value);
void sys_hal_set_cpu0_sdio_int_en(uint32_t value);
void sys_hal_set_cpu1_sdio_int_en(uint32_t value);
void sys_hal_i2s_int_en(uint32_t value);
void sys_hal_i2s1_int_en(uint32_t value);
void sys_hal_i2s2_int_en(uint32_t value);

void sys_hal_early_init(void);
void sys_hal_early_deinit(void);
void sys_hal_early_init_tfm(void);
void sys_hal_dpll_cpu_flash_time_early_init(void);

#ifdef __cplusplus
}
#endif
