// Copyright 2022-2023 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#include "hal_config.h"
#include "sys_hw.h"
#include "sys_hal.h"

#if CONFIG_HAL_DEBUG_SYS
typedef void (*sys_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	sys_dump_fn_t fn;
} sys_reg_fn_map_t;

static void system_dump_device_id(void)
{
	SOC_LOGI("device_id: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x0 << 2)));
}

static void system_dump_version_id(void)
{
	SOC_LOGI("version_id: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x1 << 2)));
}

static void system_dump_cpu_storage_connect_op_select(void)
{
	system_cpu_storage_connect_op_select_t *r = (system_cpu_storage_connect_op_select_t *)(SOC_SYSTEM_REG_BASE + (0x2 << 2));

	SOC_LOGI("cpu_storage_connect_op_select: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	boot_mode: %8x\r\n", r->boot_mode);
	SOC_LOGI("	reserved0: %8x\r\n", r->reserved0);
	SOC_LOGI("	rf_switch_manual_en: %8x\r\n", r->rf_switch_manual_en);
	SOC_LOGI("	rf_source: %8x\r\n", r->rf_source);
	SOC_LOGI("	reserved1: %8x\r\n", r->reserved1);
	SOC_LOGI("	flash_sel: %8x\r\n", r->flash_sel);
	SOC_LOGI("	fem_bps_txen: %8x\r\n", r->fem_bps_txen);
	SOC_LOGI("	reserved2: %8x\r\n", r->reserved2);
}

static void system_dump_cpu_current_run_status(void)
{
	system_cpu_current_run_status_t *r = (system_cpu_current_run_status_t *)(SOC_SYSTEM_REG_BASE + (0x3 << 2));

	SOC_LOGI("cpu_current_run_status: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x3 << 2)));
	SOC_LOGI("	core0_halted: %8x\r\n", r->core0_halted);
	SOC_LOGI("	reserved_bit_1_1: %8x\r\n", r->reserved_bit_1_1);
	SOC_LOGI("	reserved0: %8x\r\n", r->reserved0);
	SOC_LOGI("	cpu0_sw_reset: %8x\r\n", r->cpu0_sw_reset);
	SOC_LOGI("	reserved1: %8x\r\n", r->reserved1);
	SOC_LOGI("	cpu0_pwr_dw_state: %8x\r\n", r->cpu0_pwr_dw_state);
	SOC_LOGI("	reserved2: %8x\r\n", r->reserved2);
}

static void system_dump_cpu0_int_halt_clk_op(void)
{
	system_cpu0_int_halt_clk_op_t *r = (system_cpu0_int_halt_clk_op_t *)(SOC_SYSTEM_REG_BASE + (0x4 << 2));

	SOC_LOGI("cpu0_int_halt_clk_op: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	cpu0_sw_rst: %8x\r\n", r->cpu0_sw_rst);
	SOC_LOGI("	cpu0_pwr_dw: %8x\r\n", r->cpu0_pwr_dw);
	SOC_LOGI("	cpu0_int_mask: %8x\r\n", r->cpu0_int_mask);
	SOC_LOGI("	cpu0_halt: %8x\r\n", r->cpu0_halt);
	SOC_LOGI("	cpu0_speed: %8x\r\n", r->cpu0_speed);
	SOC_LOGI("	cpu0_rxevt_sel: %8x\r\n", r->cpu0_rxevt_sel);
	SOC_LOGI("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGI("	cpu0_offset: %8x\r\n", r->cpu0_offset);
}

static void system_dump_rsv_5_7(void)
{
	for (uint32_t idx = 0; idx < 3; idx++) {
		SOC_LOGI("rsv_5_7: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + ((0x5 + idx) << 2)));
	}
}

static void system_dump_cpu_clk_div_mode1(void)
{
	system_cpu_clk_div_mode1_t *r = (system_cpu_clk_div_mode1_t *)(SOC_SYSTEM_REG_BASE + (0x8 << 2));

	SOC_LOGI("cpu_clk_div_mode1: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x8 << 2)));
	SOC_LOGI("	clkdiv_core: %8x\r\n", r->clkdiv_core);
	SOC_LOGI("	cksel_core: %8x\r\n", r->cksel_core);
	SOC_LOGI("	cksel_pwm0: %8x\r\n", r->cksel_pwm0);
	SOC_LOGI("	cksel_tim0: %8x\r\n", r->cksel_tim0);
	SOC_LOGI("	cksel_tim1: %8x\r\n", r->cksel_tim1);
}

static void system_dump_cpu_clk_div_mode2(void)
{
	system_cpu_clk_div_mode2_t *r = (system_cpu_clk_div_mode2_t *)(SOC_SYSTEM_REG_BASE + (0x9 << 2));

	SOC_LOGI("cpu_clk_div_mode2: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x9 << 2)));
	SOC_LOGI("	reserved_0_3: %8x\r\n", r->reserved_0_3);
	SOC_LOGI("	ckdiv_psram: %8x\r\n", r->ckdiv_psram);
	SOC_LOGI("	cksel_psram: %8x\r\n", r->cksel_psram);
	SOC_LOGI("	reserved_bit_6_13: %8x\r\n", r->reserved_bit_6_13);
	SOC_LOGI("	ckdiv_sdio: %8x\r\n", r->ckdiv_sdio);
	SOC_LOGI("	cksel_sdio: %8x\r\n", r->cksel_sdio);
	SOC_LOGI("	ckdiv_auxs: %8x\r\n", r->ckdiv_auxs);
	SOC_LOGI("	cksel_auxs: %8x\r\n", r->cksel_auxs);
	SOC_LOGI("	cksel_flash: %8x\r\n", r->cksel_flash);
	SOC_LOGI("	ckdiv_flash: %8x\r\n", r->ckdiv_flash);
	SOC_LOGI("	reserved_bit_28_30: %8x\r\n", r->reserved_bit_28_30);
	SOC_LOGI("	clkdiv_auxs: %8x\r\n", r->clkdiv_auxs);
}

static void system_dump_cpu_26m_wdt_clk_div(void)
{
	system_cpu_26m_wdt_clk_div_t *r = (system_cpu_26m_wdt_clk_div_t *)(SOC_SYSTEM_REG_BASE + (0xa << 2));

	SOC_LOGI("cpu_26m_wdt_clk_div: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0xa << 2)));
	SOC_LOGI("	ckdiv_xtal: %8x\r\n", r->ckdiv_xtal);
	SOC_LOGI("	ckdiv_wdt: %8x\r\n", r->ckdiv_wdt);
	SOC_LOGI("	clksel_spi0: %8x\r\n", r->clksel_spi0);
	SOC_LOGI("	clksel_spi1: %8x\r\n", r->clksel_spi1);
	SOC_LOGI("	cksel_qspi1: %8x\r\n", r->cksel_qspi1);
	SOC_LOGI("	reserved_9_9: %8x\r\n", r->reserved_9_9);
	SOC_LOGI("	cksel_i2c0: %8x\r\n", r->cksel_i2c0);
	SOC_LOGI("	cksel_i2c1: %8x\r\n", r->cksel_i2c1);
	SOC_LOGI("	cksel_uart0: %8x\r\n", r->cksel_uart0);
	SOC_LOGI("	cksel_uart1: %8x\r\n", r->cksel_uart1);
	SOC_LOGI("	cksel_uart2: %8x\r\n", r->cksel_uart2);
	SOC_LOGI("	cksel_uart3: %8x\r\n", r->cksel_uart3);
	SOC_LOGI("	ckdiv_qspi1: %8x\r\n", r->ckdiv_qspi1);
	SOC_LOGI("	reserved_18_31: %8x\r\n", r->reserved_18_31);
}

static void system_dump_cpu_anaspi_freq(void)
{
	system_cpu_anaspi_freq_t *r = (system_cpu_anaspi_freq_t *)(SOC_SYSTEM_REG_BASE + (0xb << 2));

	SOC_LOGI("cpu_anaspi_freq: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0xb << 2)));
	SOC_LOGI("	anaspi_freq: %8x\r\n", r->anaspi_freq);
	SOC_LOGI("	reserved_6_31: %8x\r\n", r->reserved_6_31);
}

static void system_dump_cpu_device_clk_enable(void)
{
	system_cpu_device_clk_enable_t *r = (system_cpu_device_clk_enable_t *)(SOC_SYSTEM_REG_BASE + (0xc << 2));

	SOC_LOGI("cpu_device_clk_enable: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0xc << 2)));
	SOC_LOGI("	i2c0_cken: %8x\r\n", r->i2c0_cken);
	SOC_LOGI("	spi0_cken: %8x\r\n", r->spi0_cken);
	SOC_LOGI("	uart0_cken: %8x\r\n", r->uart0_cken);
	SOC_LOGI("	pwm0_cken: %8x\r\n", r->pwm0_cken);
	SOC_LOGI("	tim0_cken: %8x\r\n", r->tim0_cken);
	SOC_LOGI("	sadc_cken: %8x\r\n", r->sadc_cken);
	SOC_LOGI("	uart3_cken: %8x\r\n", r->uart3_cken);
	SOC_LOGI("	gpio_cken: %8x\r\n", r->gpio_cken);
	SOC_LOGI("	i2c1_cken: %8x\r\n", r->i2c1_cken);
	SOC_LOGI("	spi1_cken: %8x\r\n", r->spi1_cken);
	SOC_LOGI("	uart1_cken: %8x\r\n", r->uart1_cken);
	SOC_LOGI("	uart2_cken: %8x\r\n", r->uart2_cken);
	SOC_LOGI("	reserved_bit_12_12: %8x\r\n", r->reserved_bit_12_12);
	SOC_LOGI("	tim1_cken: %8x\r\n", r->tim1_cken);
	SOC_LOGI("	reserved_bit_14_14: %8x\r\n", r->reserved_bit_14_14);
	SOC_LOGI("	otp_cken: %8x\r\n", r->otp_cken);
	SOC_LOGI("	i2s_cken: %8x\r\n", r->i2s_cken);
	SOC_LOGI("	led_cken: %8x\r\n", r->led_cken);
	SOC_LOGI("	tamp_cken: %8x\r\n", r->tamp_cken);
	SOC_LOGI("	psram_cken: %8x\r\n", r->psram_cken);
	SOC_LOGI("	dplldiv_cken: %8x\r\n", r->dplldiv_cken);
	SOC_LOGI("	qspi1_cken: %8x\r\n", r->qspi1_cken);
	SOC_LOGI("	sdio_cken: %8x\r\n", r->sdio_cken);
	SOC_LOGI("	auxs_cken: %8x\r\n", r->auxs_cken);
	SOC_LOGI("	btdm_cken: %8x\r\n", r->btdm_cken);
	SOC_LOGI("	xvr_cken: %8x\r\n", r->xvr_cken);
	SOC_LOGI("	mac_cken: %8x\r\n", r->mac_cken);
	SOC_LOGI("	phy_cken: %8x\r\n", r->phy_cken);
	SOC_LOGI("	thread_cken: %8x\r\n", r->thread_cken);
	SOC_LOGI("	rf_cken: %8x\r\n", r->rf_cken);
	SOC_LOGI("	reserved_bit_30_30: %8x\r\n", r->reserved_bit_30_30);
	SOC_LOGI("	wdt_cken: %8x\r\n", r->wdt_cken);
}

static void system_dump_rsv_d_e(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGI("rsv_d_e: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + ((0xd + idx) << 2)));
	}
}

static void system_dump_cpu_device_mem_ctrl2(void)
{
	system_cpu_device_mem_ctrl2_t *r = (system_cpu_device_mem_ctrl2_t *)(SOC_SYSTEM_REG_BASE + (0xf << 2));

	SOC_LOGI("cpu_device_mem_ctrl2: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0xf << 2)));
	SOC_LOGI("	uart1_ds: %8x\r\n", r->uart1_ds);
	SOC_LOGI("	uart2_ds: %8x\r\n", r->uart2_ds);
	SOC_LOGI("	spi1_ds: %8x\r\n", r->spi1_ds);
	SOC_LOGI("	sdio_ds: %8x\r\n", r->sdio_ds);
	SOC_LOGI("	uart3_ds: %8x\r\n", r->uart3_ds);
	SOC_LOGI("	led_ds: %8x\r\n", r->led_ds);
	SOC_LOGI("	reserved_6_6: %8x\r\n", r->reserved_6_6);
	SOC_LOGI("	qspi1_ds: %8x\r\n", r->qspi1_ds);
	SOC_LOGI("	pram_ds: %8x\r\n", r->pram_ds);
	SOC_LOGI("	reserved0: %8x\r\n", r->reserved0);
	SOC_LOGI("	i2s0_ds: %8x\r\n", r->i2s0_ds);
	SOC_LOGI("	reserved_13_20: %8x\r\n", r->reserved_13_20);
	SOC_LOGI("	mac_ds: %8x\r\n", r->mac_ds);
	SOC_LOGI("	phy_ds: %8x\r\n", r->phy_ds);
	SOC_LOGI("	xvr_ds: %8x\r\n", r->xvr_ds);
	SOC_LOGI("	reserved_bit_24_24: %8x\r\n", r->reserved_bit_24_24);
	SOC_LOGI("	la_ds: %8x\r\n", r->la_ds);
	SOC_LOGI("	flsh_ds: %8x\r\n", r->flsh_ds);
	SOC_LOGI("	uart_ds: %8x\r\n", r->uart_ds);
	SOC_LOGI("	spi0_ds: %8x\r\n", r->spi0_ds);
	SOC_LOGI("	enc_ds: %8x\r\n", r->enc_ds);
	SOC_LOGI("	dma0_ds: %8x\r\n", r->dma0_ds);
	SOC_LOGI("	reserved1: %8x\r\n", r->reserved1);
}

static void system_dump_cpu_power_sleep_wakeup(void)
{
	system_cpu_power_sleep_wakeup_t *r = (system_cpu_power_sleep_wakeup_t *)(SOC_SYSTEM_REG_BASE + (0x10 << 2));

	SOC_LOGI("cpu_power_sleep_wakeup: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x10 << 2)));
	SOC_LOGI("	pwd_mem1: %8x\r\n", r->pwd_mem1);
	SOC_LOGI("	pwd_mem2: %8x\r\n", r->pwd_mem2);
	SOC_LOGI("	pwd_mem3: %8x\r\n", r->pwd_mem3);
	SOC_LOGI("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
	SOC_LOGI("	pwd_bakp: %8x\r\n", r->pwd_bakp);
	SOC_LOGI("	reserved_bit_5_7: %8x\r\n", r->reserved_bit_5_7);
	SOC_LOGI("	pwd_btsp: %8x\r\n", r->pwd_btsp);
	SOC_LOGI("	pwd_wifp_mac: %8x\r\n", r->pwd_wifp_mac);
	SOC_LOGI("	pwd_wifp_phy: %8x\r\n", r->pwd_wifp_phy);
	SOC_LOGI("	pwd_mem0: %8x\r\n", r->pwd_mem0);
	SOC_LOGI("	pwd_mem4: %8x\r\n", r->pwd_mem4);
	SOC_LOGI("	pwd_ofdm: %8x\r\n", r->pwd_ofdm);
	SOC_LOGI("	reserved_bit_14_14: %8x\r\n", r->reserved_bit_14_14);
	SOC_LOGI("	rom_pgen: %8x\r\n", r->rom_pgen);
	SOC_LOGI("	sleep_en_need_flash_idle: %8x\r\n", r->sleep_en_need_flash_idle);
	SOC_LOGI("	reserved_bit_17_17: %8x\r\n", r->reserved_bit_17_17);
	SOC_LOGI("	sleep_en_need_cpu0_wfi: %8x\r\n", r->sleep_en_need_cpu0_wfi);
	SOC_LOGI("	sleep_en_global: %8x\r\n", r->sleep_en_global);
	SOC_LOGI("	sleep_bus_idle_bypass: %8x\r\n", r->sleep_bus_idle_bypass);
	SOC_LOGI("	reserved_bit_21_21: %8x\r\n", r->reserved_bit_21_21);
	SOC_LOGI("	bts_soft_wakeup_req: %8x\r\n", r->bts_soft_wakeup_req);
	SOC_LOGI("	rom_rd_disable: %8x\r\n", r->rom_rd_disable);
	SOC_LOGI("	otp_rd_disable: %8x\r\n", r->otp_rd_disable);
	SOC_LOGI("	tcm0_pgen: %8x\r\n", r->tcm0_pgen);
	SOC_LOGI("	cpu0_subpwdm_en: %8x\r\n", r->cpu0_subpwdm_en);
	SOC_LOGI("	te200_clkgat_bps: %8x\r\n", r->te200_clkgat_bps);
	SOC_LOGI("	share_mem_clkgating_disable: %8x\r\n", r->share_mem_clkgating_disable);
	SOC_LOGI("	cpu0_ticktimer_32k_enable: %8x\r\n", r->cpu0_ticktimer_32k_enable);
	SOC_LOGI("	reserved_30_30: %8x\r\n", r->reserved_30_30);
	SOC_LOGI("	busmatrix_busy: %8x\r\n", r->busmatrix_busy);
}

static void system_dump_cpu0_lv_sleep_cfg(void)
{
	system_cpu0_lv_sleep_cfg_t *r = (system_cpu0_lv_sleep_cfg_t *)(SOC_SYSTEM_REG_BASE + (0x11 << 2));

	SOC_LOGI("cpu0_lv_sleep_cfg: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x11 << 2)));
	SOC_LOGI("	cpu0_cache_ret_en: %8x\r\n", r->cpu0_cache_ret_en);
	SOC_LOGI("	cpu0_cache_sleeppwd_en: %8x\r\n", r->cpu0_cache_sleeppwd_en);
	SOC_LOGI("	reserved_2_31: %8x\r\n", r->reserved_2_31);
}

static void system_dump_cpu_device_mem_ctrl3(void)
{
	system_cpu_device_mem_ctrl3_t *r = (system_cpu_device_mem_ctrl3_t *)(SOC_SYSTEM_REG_BASE + (0x12 << 2));

	SOC_LOGI("cpu_device_mem_ctrl3: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x12 << 2)));
	SOC_LOGI("	ram0_ds: %8x\r\n", r->ram0_ds);
	SOC_LOGI("	ram1_ds: %8x\r\n", r->ram1_ds);
	SOC_LOGI("	ram2_ds: %8x\r\n", r->ram2_ds);
	SOC_LOGI("	ram3_ds: %8x\r\n", r->ram3_ds);
	SOC_LOGI("	ram4_ds: %8x\r\n", r->ram4_ds);
	SOC_LOGI("	reserved0: %8x\r\n", r->reserved0);
	SOC_LOGI("	ram_config: %8x\r\n", r->ram_config);
	SOC_LOGI("	reserved1: %8x\r\n", r->reserved1);
}

static void system_dump_rsv_13_1f(void)
{
	for (uint32_t idx = 0; idx < 13; idx++) {
		SOC_LOGI("rsv_13_1f: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + ((0x13 + idx) << 2)));
	}
}

static void system_dump_cpu0_int_0_31_en(void)
{
	system_cpu0_int_0_31_en_t *r = (system_cpu0_int_0_31_en_t *)(SOC_SYSTEM_REG_BASE + (0x20 << 2));

	SOC_LOGI("cpu0_int_0_31_en: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x20 << 2)));
	SOC_LOGI("	cpu0_dma_nsec_intr_en: %8x\r\n", r->cpu0_dma_nsec_intr_en);
	SOC_LOGI("	cpu0_encp_sec_intr_en: %8x\r\n", r->cpu0_encp_sec_intr_en);
	SOC_LOGI("	cpu0_encp_nsec_intr_en: %8x\r\n", r->cpu0_encp_nsec_intr_en);
	SOC_LOGI("	cpu0_timer_int_en: %8x\r\n", r->cpu0_timer_int_en);
	SOC_LOGI("	cpu0_uart0_int_en: %8x\r\n", r->cpu0_uart0_int_en);
	SOC_LOGI("	cpu0_pwm0_int_en: %8x\r\n", r->cpu0_pwm0_int_en);
	SOC_LOGI("	cpu0_i2c0_int_en: %8x\r\n", r->cpu0_i2c0_int_en);
	SOC_LOGI("	cpu0_spi0_int_en: %8x\r\n", r->cpu0_spi0_int_en);
	SOC_LOGI("	reserved_bit_8_8: %8x\r\n", r->cpu0_sadc_int_en);
	SOC_LOGI("	cpu0_uart3_int_en: %8x\r\n", r->cpu0_uart3_int_en);
	SOC_LOGI("	cpu0_sdio_int_en: %8x\r\n", r->cpu0_sdio_int_en);
	SOC_LOGI("	cpu0_gdma_int_en: %8x\r\n", r->cpu0_gdma_int_en);
	SOC_LOGI("	cpu0_la_int_en: %8x\r\n", r->cpu0_la_int_en);
	SOC_LOGI("	cpu0_timer1_int_en: %8x\r\n", r->cpu0_timer1_int_en);
	SOC_LOGI("	cpu0_i2c1_int_en: %8x\r\n", r->cpu0_i2c1_int_en);
	SOC_LOGI("	cpu0_uart1_int_en: %8x\r\n", r->cpu0_uart1_int_en);
	SOC_LOGI("	cpu0_uart2_int_en: %8x\r\n", r->cpu0_uart2_int_en);
	SOC_LOGI("	cpu0_spi1_int_en: %8x\r\n", r->cpu0_spi1_int_en);
	SOC_LOGI("	cpu0_led_int_en: %8x\r\n", r->cpu0_led_int_en);
	SOC_LOGI("	cpu0_resv0_int_en: %8x\r\n", r->cpu0_resv0_int_en);
	SOC_LOGI("	cpu0_resv1_int_en: %8x\r\n", r->cpu0_resv1_int_en);
	SOC_LOGI("	cpu0_ckmn_int_en: %8x\r\n", r->cpu0_ckmn_int_en);
	SOC_LOGI("	cpu0_resv2_int_en: %8x\r\n", r->cpu0_resv2_int_en);
	SOC_LOGI("	cpu0_resv3_int_en: %8x\r\n", r->cpu0_resv3_int_en);
	SOC_LOGI("	cpu0_i2s0_int_en: %8x\r\n", r->cpu0_i2s0_int_en);
	SOC_LOGI("	cpu0_resv4_int_en: %8x\r\n", r->cpu0_resv4_int_en);
	SOC_LOGI("	cpu0_resv5_int_en: %8x\r\n", r->cpu0_resv5_int_en);
	SOC_LOGI("	cpu0_resv6_int_en: %8x\r\n", r->cpu0_resv6_int_en);
	SOC_LOGI("	cpu0_resv7_int_en: %8x\r\n", r->cpu0_resv7_int_en);
	SOC_LOGI("	cpu0_int_mac_wakeup_int_en: %8x\r\n", r->cpu0_int_mac_wakeup_int_en);
	SOC_LOGI("	cpu0_hsu_irq_int_en: %8x\r\n", r->cpu0_hsu_irq_int_en);
	SOC_LOGI("	cpu0_mac_int_gen_int_en: %8x\r\n", r->cpu0_mac_int_gen_int_en);
}

static void system_dump_cpu0_int_32_63_en(void)
{
	system_cpu0_int_32_63_en_t *r = (system_cpu0_int_32_63_en_t *)(SOC_SYSTEM_REG_BASE + (0x21 << 2));

	SOC_LOGI("cpu0_int_32_63_en: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x21 << 2)));
	SOC_LOGI("	cpu0_mac_int_port_trigger_int_en: %8x\r\n", r->cpu0_mac_int_port_trigger_int_en);
	SOC_LOGI("	cpu0_mac_int_tx_trigger_int_en: %8x\r\n", r->cpu0_mac_int_tx_trigger_int_en);
	SOC_LOGI("	cpu0_mac_int_rx_trigger_int_en: %8x\r\n", r->cpu0_mac_int_rx_trigger_int_en);
	SOC_LOGI("	cpu0_mac_int_tx_rx_misc_int_en: %8x\r\n", r->cpu0_mac_int_tx_rx_misc_int_en);
	SOC_LOGI("	cpu0_mac_int_tx_rx_timer_int_en: %8x\r\n", r->cpu0_mac_int_tx_rx_timer_int_en);
	SOC_LOGI("	cpu0_phy_riu_int_int_en: %8x\r\n", r->cpu0_phy_riu_int_int_en);
	SOC_LOGI("	cpu0_phy_mbp_int_int_en: %8x\r\n", r->cpu0_phy_mbp_int_int_en);
	SOC_LOGI("	cpu0_bt_irq_int_en: %8x\r\n", r->cpu0_bt_irq_int_en);
	SOC_LOGI("	cpu0_ble_irq_int_en: %8x\r\n", r->cpu0_ble_irq_int_en);
	SOC_LOGI("	cpu0_dm_irq_int_en: %8x\r\n", r->cpu0_dm_irq_int_en);
	SOC_LOGI("	cpu0_qspi0_int_en: %8x\r\n", r->cpu0_qspi0_int_en);
	SOC_LOGI("	cpu0_resv8_int_en: %8x\r\n", r->cpu0_resv8_int_en);
	SOC_LOGI("	cpu0_resv9_int_en: %8x\r\n", r->cpu0_resv9_int_en);
	SOC_LOGI("	cpu0_resv10_int_en: %8x\r\n", r->cpu0_resv10_int_en);
	SOC_LOGI("	cpu0_resv11_int_en: %8x\r\n", r->cpu0_resv11_int_en);
	SOC_LOGI("	cpu0_resv12_int_en: %8x\r\n", r->cpu0_resv12_int_en);
	SOC_LOGI("	cpu0_thread_int_en: %8x\r\n", r->cpu0_thread_int_en);
	SOC_LOGI("	cpu0_resv13_int_en: %8x\r\n", r->cpu0_resv13_int_en);
	SOC_LOGI("	cpu0_otp_int_en: %8x\r\n", r->cpu0_otp_int_en);
	SOC_LOGI("	cpu0_dpllunlock_int_en: %8x\r\n", r->cpu0_dpllunlock_int_en);
	SOC_LOGI("	cpu0_resv14_int_en: %8x\r\n", r->cpu0_resv14_int_en);
	SOC_LOGI("	cpu0_resv15_int_en: %8x\r\n", r->cpu0_resv15_int_en);
	SOC_LOGI("	cpu0_resv16_int_en: %8x\r\n", r->cpu0_resv16_int_en);
	SOC_LOGI("	cpu0_gpio_s_int_en: %8x\r\n", r->cpu0_gpio_s_int_en);
	SOC_LOGI("	cpu0_gpio_ns_int_en: %8x\r\n", r->cpu0_gpio_ns_int_en);
	SOC_LOGI("	cpu0_resv17_int_en: %8x\r\n", r->cpu0_resv17_int_en);
	SOC_LOGI("	cpu0_ana_gpio_int_en: %8x\r\n", r->cpu0_ana_gpio_int_en);
	SOC_LOGI("	cpu0_ana_rtc_int_en: %8x\r\n", r->cpu0_ana_rtc_int_en);
	SOC_LOGI("	cpu0_abnormal_gpio_int_en: %8x\r\n", r->cpu0_abnormal_gpio_int_en);
	SOC_LOGI("	cpu0_abnormal_rtc_int_en: %8x\r\n", r->cpu0_abnormal_rtc_int_en);
	SOC_LOGI("	cpu0_resv18_int_en: %8x\r\n", r->cpu0_resv18_int_en);
	SOC_LOGI("	cpu0_resv19_int_en: %8x\r\n", r->cpu0_resv19_int_en);
}

static void system_dump_rsv_22_27(void)
{
	for (uint32_t idx = 0; idx < 6; idx++) {
		SOC_LOGI("rsv_22_27: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + ((0x22 + idx) << 2)));
	}
}

static void system_dump_cpu0_int_0_31_status(void)
{
	system_cpu0_int_0_31_status_t *r = (system_cpu0_int_0_31_status_t *)(SOC_SYSTEM_REG_BASE + (0x28 << 2));

	SOC_LOGI("cpu0_int_0_31_status: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x28 << 2)));
	SOC_LOGI("	cpu0_dma_nsec_intr_st: %8x\r\n", r->cpu0_dma_nsec_intr_st);
	SOC_LOGI("	cpu0_stcp_sec_intr_st: %8x\r\n", r->cpu0_stcp_sec_intr_st);
	SOC_LOGI("	cpu0_stcp_nsec_intr_st: %8x\r\n", r->cpu0_stcp_nsec_intr_st);
	SOC_LOGI("	cpu0_timer_int_st: %8x\r\n", r->cpu0_timer_int_st);
	SOC_LOGI("	cpu0_uart0_int_st: %8x\r\n", r->cpu0_uart0_int_st);
	SOC_LOGI("	cpu0_pwm0_int_st: %8x\r\n", r->cpu0_pwm0_int_st);
	SOC_LOGI("	cpu0_i2c0_int_st: %8x\r\n", r->cpu0_i2c0_int_st);
	SOC_LOGI("	cpu0_spi0_int_st: %8x\r\n", r->cpu0_spi0_int_st);
	SOC_LOGI("	cpu0_sadc_int_st: %8x\r\n", r->cpu0_sadc_int_st);
	SOC_LOGI("	cpu0_uart3_int_st: %8x\r\n", r->cpu0_uart3_int_st);
	SOC_LOGI("	cpu0_sdio_int_st: %8x\r\n", r->cpu0_sdio_int_st);
	SOC_LOGI("	cpu0_gdma_int_st: %8x\r\n", r->cpu0_gdma_int_st);
	SOC_LOGI("	cpu0_la_int_st: %8x\r\n", r->cpu0_la_int_st);
	SOC_LOGI("	cpu0_timer1_int_st: %8x\r\n", r->cpu0_timer1_int_st);
	SOC_LOGI("	cpu0_i2c1_int_st: %8x\r\n", r->cpu0_i2c1_int_st);
	SOC_LOGI("	cpu0_uart1_int_st: %8x\r\n", r->cpu0_uart1_int_st);
	SOC_LOGI("	cpu0_uart2_int_st: %8x\r\n", r->cpu0_uart2_int_st);
	SOC_LOGI("	cpu0_spi1_int_st: %8x\r\n", r->cpu0_spi1_int_st);
	SOC_LOGI("	cpu0_led_int_st: %8x\r\n", r->cpu0_led_int_st);
	SOC_LOGI("	cpu0_resv0_int_st: %8x\r\n", r->cpu0_resv0_int_st);
	SOC_LOGI("	cpu0_resv1_int_st: %8x\r\n", r->cpu0_resv1_int_st);
	SOC_LOGI("	cpu0_ckmn_int_st: %8x\r\n", r->cpu0_ckmn_int_st);
	SOC_LOGI("	cpu0_resv2_int_st: %8x\r\n", r->cpu0_resv2_int_st);
	SOC_LOGI("	cpu0_resv3_int_st: %8x\r\n", r->cpu0_resv3_int_st);
	SOC_LOGI("	cpu0_i2s0_int_st: %8x\r\n", r->cpu0_i2s0_int_st);
	SOC_LOGI("	cpu0_resv4_int_st: %8x\r\n", r->cpu0_resv4_int_st);
	SOC_LOGI("	cpu0_resv5_int_st: %8x\r\n", r->cpu0_resv5_int_st);
	SOC_LOGI("	cpu0_resv6_int_st: %8x\r\n", r->cpu0_resv6_int_st);
	SOC_LOGI("	cpu0_resv7_int_st: %8x\r\n", r->cpu0_resv7_int_st);
	SOC_LOGI("	cpu0_int_mac_wakeup_int_st: %8x\r\n", r->cpu0_int_mac_wakeup_int_st);
	SOC_LOGI("	cpu0_hsu_irq_int_st: %8x\r\n", r->cpu0_hsu_irq_int_st);
	SOC_LOGI("	cpu0_mac_int_gst_int_st: %8x\r\n", r->cpu0_mac_int_gst_int_st);
}

static void system_dump_cpu0_int_32_63_status(void)
{
	system_cpu0_int_32_63_status_t *r = (system_cpu0_int_32_63_status_t *)(SOC_SYSTEM_REG_BASE + (0x29 << 2));

	SOC_LOGI("cpu0_int_32_63_status: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x29 << 2)));
	SOC_LOGI("	cpu0_mac_int_port_trigger_int_st: %8x\r\n", r->cpu0_mac_int_port_trigger_int_st);
	SOC_LOGI("	cpu0_mac_int_tx_trigger_int_st: %8x\r\n", r->cpu0_mac_int_tx_trigger_int_st);
	SOC_LOGI("	cpu0_mac_int_rx_trigger_int_st: %8x\r\n", r->cpu0_mac_int_rx_trigger_int_st);
	SOC_LOGI("	cpu0_mac_int_tx_rx_misc_int_st: %8x\r\n", r->cpu0_mac_int_tx_rx_misc_int_st);
	SOC_LOGI("	cpu0_mac_int_tx_rx_timer_int_st: %8x\r\n", r->cpu0_mac_int_tx_rx_timer_int_st);
	SOC_LOGI("	cpu0_phy_riu_int_int_st: %8x\r\n", r->cpu0_phy_riu_int_int_st);
	SOC_LOGI("	cpu0_phy_mbp_int_int_st: %8x\r\n", r->cpu0_phy_mbp_int_int_st);
	SOC_LOGI("	cpu0_bt_irq_int_st: %8x\r\n", r->cpu0_bt_irq_int_st);
	SOC_LOGI("	cpu0_ble_irq_int_st: %8x\r\n", r->cpu0_ble_irq_int_st);
	SOC_LOGI("	cpu0_dm_irq_int_st: %8x\r\n", r->cpu0_dm_irq_int_st);
	SOC_LOGI("	cpu0_qspi0_int_st: %8x\r\n", r->cpu0_qspi0_int_st);
	SOC_LOGI("	cpu0_resv8_int_st: %8x\r\n", r->cpu0_resv8_int_st);
	SOC_LOGI("	cpu0_resv9_int_st: %8x\r\n", r->cpu0_resv9_int_st);
	SOC_LOGI("	cpu0_resv10_int_st: %8x\r\n", r->cpu0_resv10_int_st);
	SOC_LOGI("	cpu0_resv11_int_st: %8x\r\n", r->cpu0_resv11_int_st);
	SOC_LOGI("	cpu0_resv12_int_st: %8x\r\n", r->cpu0_resv12_int_st);
	SOC_LOGI("	cpu0_thread_int_st: %8x\r\n", r->cpu0_thread_int_st);
	SOC_LOGI("	cpu0_resv13_int_st: %8x\r\n", r->cpu0_resv13_int_st);
	SOC_LOGI("	cpu0_otp_int_st: %8x\r\n", r->cpu0_otp_int_st);
	SOC_LOGI("	cpu0_dpllunlock_int_st: %8x\r\n", r->cpu0_dpllunlock_int_st);
	SOC_LOGI("	cpu0_resv14_int_st: %8x\r\n", r->cpu0_resv14_int_st);
	SOC_LOGI("	cpu0_resv15_int_st: %8x\r\n", r->cpu0_resv15_int_st);
	SOC_LOGI("	cpu0_resv16_int_st: %8x\r\n", r->cpu0_resv16_int_st);
	SOC_LOGI("	cpu0_gpio_s_int_st: %8x\r\n", r->cpu0_gpio_s_int_st);
	SOC_LOGI("	cpu0_gpio_ns_int_st: %8x\r\n", r->cpu0_gpio_ns_int_st);
	SOC_LOGI("	cpu0_resv17_int_st: %8x\r\n", r->cpu0_resv17_int_st);
	SOC_LOGI("	cpu0_ana_gpio_int_st: %8x\r\n", r->cpu0_ana_gpio_int_st);
	SOC_LOGI("	cpu0_ana_rtc_int_st: %8x\r\n", r->cpu0_ana_rtc_int_st);
	SOC_LOGI("	cpu0_abnormal_gpio_int_st: %8x\r\n", r->cpu0_abnormal_gpio_int_st);
	SOC_LOGI("	cpu0_abnormal_rtc_int_st: %8x\r\n", r->cpu0_abnormal_rtc_int_st);
	SOC_LOGI("	cpu0_resv18_int_st: %8x\r\n", r->cpu0_resv18_int_st);
	SOC_LOGI("	cpu0_resv19_int_st: %8x\r\n", r->cpu0_resv19_int_st);
}

static void system_dump_rsv_2a_2f(void)
{
	for (uint32_t idx = 0; idx < 6; idx++) {
		SOC_LOGI("rsv_2a_2f: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + ((0x2a + idx) << 2)));
	}
}

static void system_dump_gpio_config0(void)
{
	SOC_LOGI("gpio_config0: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x30 << 2)));
}

static void system_dump_gpio_config1(void)
{
	SOC_LOGI("gpio_config1: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x31 << 2)));
}

static void system_dump_gpio_config2(void)
{
	SOC_LOGI("gpio_config2: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x32 << 2)));
}

static void system_dump_rsv_33_37(void)
{
	for (uint32_t idx = 0; idx < 5; idx++) {
		SOC_LOGI("rsv_33_37: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + ((0x33 + idx) << 2)));
	}
}

static void system_dump_dbug_config0(void)
{
	system_dbug_config0_t *r = (system_dbug_config0_t *)(SOC_SYSTEM_REG_BASE + (0x38 << 2));

	SOC_LOGI("dbug_config0: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x38 << 2)));
	SOC_LOGI("	dbug_msg: %8x\r\n", r->dbug_msg);
	SOC_LOGI("	dbug_mux: %8x\r\n", r->dbug_mux);
	SOC_LOGI("	reserved_20_31: %8x\r\n", r->reserved_20_31);
}

static void system_dump_rsv_39_39(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGI("rsv_39_39: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + ((0x39 + idx) << 2)));
	}
}

static void system_dump_anareg_stat(void)
{
	SOC_LOGI("anareg_stat: %8x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + (0x3a << 2)));
}

static void sys_dump_rsv_3b_3f(void)
{
	for (uint32_t idx = 0; idx < 6; idx++) {
		SOC_LOGI("rsv_3a_3f: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + ((0x3b + idx) << 2)));
	}
}

static void sys_dump_ana_reg0(void)
{
	sys_ana_reg0_t *r = (sys_ana_reg0_t *)(SOC_SYS_REG_BASE + (0x40 << 2));

	SOC_LOGI("ana_reg0: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x40 << 2)));
	SOC_LOGI("	nc_0_6: %8x\r\n", r->nc_0_6);
	SOC_LOGI("	cp: %8x\r\n", r->cp);
	SOC_LOGI("	spideten: %8x\r\n", r->spideten);
	SOC_LOGI("	cben: %8x\r\n", r->cben);
	SOC_LOGI("	hvref: %8x\r\n", r->hvref);
	SOC_LOGI("	lvref: %8x\r\n", r->lvref);
	SOC_LOGI("	rzctrl26m: %8x\r\n", r->rzctrl26m);
	SOC_LOGI("	lpf_rz: %8x\r\n", r->lpf_rz);
	SOC_LOGI("	rpc: %8x\r\n", r->rpc);
	SOC_LOGI("	dpll_tsten: %8x\r\n", r->dpll_tsten);
	SOC_LOGI("	divsel: %8x\r\n", r->divsel);
	SOC_LOGI("	kctrl: %8x\r\n", r->kctrl);
	SOC_LOGI("	vsel_ldo: %8x\r\n", r->vsel_ldo);
	SOC_LOGI("	bp_caldone: %8x\r\n", r->bp_caldone);
	SOC_LOGI("	nc_31_31: %8x\r\n", r->nc_31_31);
}

static void sys_dump_ana_reg1(void)
{
	sys_ana_reg1_t *r = (sys_ana_reg1_t *)(SOC_SYS_REG_BASE + (0x41 << 2));

	SOC_LOGI("ana_reg1: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x41 << 2)));
	SOC_LOGI("	nc_0_0: %8x\r\n", r->nc_0_0);
	SOC_LOGI("	bandmanual: %8x\r\n", r->bandmanual);
	SOC_LOGI("	mode: %8x\r\n", r->mode);
	SOC_LOGI("	manual_ictrl: %8x\r\n", r->manual_ictrl);
	SOC_LOGI("	enclk: %8x\r\n", r->enclk);
	SOC_LOGI("	closeloop_en: %8x\r\n", r->closeloop_en);
	SOC_LOGI("	nc_13_13: %8x\r\n", r->nc_13_13);
	SOC_LOGI("	spi_rstn: %8x\r\n", r->spi_rstn);
	SOC_LOGI("	osccal_trig: %8x\r\n", r->osccal_trig);
	SOC_LOGI("	manual: %8x\r\n", r->manual);
	SOC_LOGI("	diffl: %8x\r\n", r->diffl);
	SOC_LOGI("	ictrl: %8x\r\n", r->ictrl);
	SOC_LOGI("	cnti: %8x\r\n", r->cnti);
}

static void sys_dump_ana_reg2(void)
{
	sys_ana_reg2_t *r = (sys_ana_reg2_t *)(SOC_SYS_REG_BASE + (0x42 << 2));

	SOC_LOGI("ana_reg2: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x42 << 2)));
	SOC_LOGI("	xtalh_ctune: %8x\r\n", r->xtalh_ctune);
	SOC_LOGI("	gadc_inbufsel: %8x\r\n", r->gadc_inbufsel);
	SOC_LOGI("	nc_10_10: %8x\r\n", r->nc_10_10);
	SOC_LOGI("	gadc_compisel: %8x\r\n", r->gadc_compisel);
	SOC_LOGI("	gadc_bscalsaw: %8x\r\n", r->gadc_bscalsaw);
	SOC_LOGI("	gadc_vncalsaw: %8x\r\n", r->gadc_vncalsaw);
	SOC_LOGI("	gadc_vpcalsaw: %8x\r\n", r->gadc_vpcalsaw);
	SOC_LOGI("	gadc_vbg_sel: %8x\r\n", r->gadc_vbg_sel);
	SOC_LOGI("	gadc_clk_rlten: %8x\r\n", r->gadc_clk_rlten);
	SOC_LOGI("	gadc_calintsaw_en: %8x\r\n", r->gadc_calintsaw_en);
	SOC_LOGI("	gadc_clk_sel: %8x\r\n", r->gadc_clk_sel);
	SOC_LOGI("	gadc_clk_inv: %8x\r\n", r->gadc_clk_inv);
	SOC_LOGI("	gadc_calcap_ch: %8x\r\n", r->gadc_calcap_ch);
	SOC_LOGI("	inbufen: %8x\r\n", r->inbufen);
	SOC_LOGI("	sar_enspi: %8x\r\n", r->sar_enspi);
}

static void sys_dump_ana_reg3(void)
{
	sys_ana_reg3_t *r = (sys_ana_reg3_t *)(SOC_SYS_REG_BASE + (0x43 << 2));

	SOC_LOGI("ana_reg3: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x43 << 2)));
	SOC_LOGI("	preamp_isel: %8x\r\n", r->preamp_isel);
	SOC_LOGI("	refbuff_isel: %8x\r\n", r->refbuff_isel);
	SOC_LOGI("	vref_sel: %8x\r\n", r->vref_sel);
	SOC_LOGI("	offset_en: %8x\r\n", r->offset_en);
	SOC_LOGI("	hpssren: %8x\r\n", r->hpssren);
	SOC_LOGI("	ck_sel: %8x\r\n", r->ck_sel);
	SOC_LOGI("	anabuf_sel_tx: %8x\r\n", r->anabuf_sel_tx);
	SOC_LOGI("	ampbias_outen: %8x\r\n", r->ampbias_outen);
	SOC_LOGI("	iamp: %8x\r\n", r->iamp);
	SOC_LOGI("	vddren: %8x\r\n", r->vddren);
	SOC_LOGI("	xamp: %8x\r\n", r->xamp);
	SOC_LOGI("	vosel: %8x\r\n", r->vosel);
	SOC_LOGI("	en_xtalh_sleep: %8x\r\n", r->en_xtalh_sleep);
	SOC_LOGI("	digbufb_bpen: %8x\r\n", r->digbufb_bpen);
	SOC_LOGI("	bufictrl: %8x\r\n", r->bufictrl);
	SOC_LOGI("	ibias_ctrl: %8x\r\n", r->ibias_ctrl);
	SOC_LOGI("	icore_ctrl: %8x\r\n", r->icore_ctrl);
}

static void sys_dump_ana_reg4(void)
{
	sys_ana_reg4_t *r = (sys_ana_reg4_t *)(SOC_SYS_REG_BASE + (0x44 << 2));

	SOC_LOGI("ana_reg4: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x44 << 2)));
	SOC_LOGI("	td_clk_en: %8x\r\n", r->td_clk_en);
	SOC_LOGI("	td_cks: %8x\r\n", r->td_cks);
	SOC_LOGI("	td_en: %8x\r\n", r->td_en);
	SOC_LOGI("	td_chn: %8x\r\n", r->td_chn);
	SOC_LOGI("	td_enscm1v: %8x\r\n", r->td_enscm1v);
	SOC_LOGI("	tdldo_sel: %8x\r\n", r->tdldo_sel);
	SOC_LOGI("	tdldo_bypassen: %8x\r\n", r->tdldo_bypassen);
	SOC_LOGI("	nc_24_25: %8x\r\n", r->nc_24_25);
	SOC_LOGI("	en_vddflashtest: %8x\r\n", r->en_vddflashtest);
	SOC_LOGI("	vddgpio_sel: %8x\r\n", r->vddgpio_sel);
	SOC_LOGI("	vpsramsel: %8x\r\n", r->vpsramsel);
	SOC_LOGI("	swb: %8x\r\n", r->swb);
	SOC_LOGI("	en_psramldo: %8x\r\n", r->en_psramldo);
}

static void sys_dump_ana_reg5(void)
{
	sys_ana_reg5_t *r = (sys_ana_reg5_t *)(SOC_SYS_REG_BASE + (0x45 << 2));

	SOC_LOGI("ana_reg5: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x45 << 2)));
	SOC_LOGI("	anabufsel_rx: %8x\r\n", r->anabufsel_rx);
	SOC_LOGI("	en_xtall: %8x\r\n", r->en_xtall);
	SOC_LOGI("	nc_2_2: %8x\r\n", r->nc_2_2);
	SOC_LOGI("	nc_3_3: %8x\r\n", r->nc_3_3);
	SOC_LOGI("	en_temp: %8x\r\n", r->en_temp);
	SOC_LOGI("	en_dpll: %8x\r\n", r->en_dpll);
	SOC_LOGI("	en_cb: %8x\r\n", r->en_cb);
	SOC_LOGI("	pmu_test: %8x\r\n", r->pmu_test);
	SOC_LOGI("	pwd_anabuf_lownoise: %8x\r\n", r->pwd_anabuf_lownoise);
	SOC_LOGI("	cktst_sel: %8x\r\n", r->cktst_sel);
	SOC_LOGI("	ck_tst_enable: %8x\r\n", r->ck_tst_enable);
	SOC_LOGI("	rosc_disable: %8x\r\n", r->rosc_disable);
	SOC_LOGI("	spilatchb_rc32k: %8x\r\n", r->spilatchb_rc32k);
	SOC_LOGI("	pwd_rosc_spi: %8x\r\n", r->pwd_rosc_spi);
	SOC_LOGI("	rc32k_refclken: %8x\r\n", r->rc32k_refclken);
	SOC_LOGI("	itune_xtall: %8x\r\n", r->itune_xtall);
	SOC_LOGI("	xtall_ten: %8x\r\n", r->xtall_ten);
	SOC_LOGI("	rosc_tsten: %8x\r\n", r->rosc_tsten);
	SOC_LOGI("	bcal_start: %8x\r\n", r->bcal_start);
	SOC_LOGI("	bcal_en: %8x\r\n", r->bcal_en);
	SOC_LOGI("	bcal_sel: %8x\r\n", r->bcal_sel);
	SOC_LOGI("	vbias: %8x\r\n", r->vbias);
}

static void sys_dump_ana_reg6(void)
{
	sys_ana_reg6_t *r = (sys_ana_reg6_t *)(SOC_SYS_REG_BASE + (0x46 << 2));

	SOC_LOGI("ana_reg6: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x46 << 2)));
	SOC_LOGI("	calib_interval: %8x\r\n", r->calib_interval);
	SOC_LOGI("	modify_interval: %8x\r\n", r->modify_interval);
	SOC_LOGI("	xtal_wakeup_time: %8x\r\n", r->xtal_wakeup_time);
	SOC_LOGI("	spi_trig: %8x\r\n", r->spi_trig);
	SOC_LOGI("	modifi_auto: %8x\r\n", r->modifi_auto);
	SOC_LOGI("	calib_auto: %8x\r\n", r->calib_auto);
	SOC_LOGI("	cal_mode: %8x\r\n", r->cal_mode);
	SOC_LOGI("	manu_ena: %8x\r\n", r->manu_ena);
	SOC_LOGI("	manu_cin: %8x\r\n", r->manu_cin);
}

static void sys_dump_ana_reg7(void)
{
	sys_ana_reg7_t *r = (sys_ana_reg7_t *)(SOC_SYS_REG_BASE + (0x47 << 2));

	SOC_LOGI("ana_reg7: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x47 << 2)));
	SOC_LOGI("	clk_sel: %8x\r\n", r->clk_sel);
	SOC_LOGI("	timer_wkrstn: %8x\r\n", r->timer_wkrstn);
	SOC_LOGI("	dldohp: %8x\r\n", r->dldohp);
	SOC_LOGI("	vporsel: %8x\r\n", r->vporsel);
	SOC_LOGI("	vbspbuf_lp: %8x\r\n", r->vbspbuf_lp);
	SOC_LOGI("	iocurlim: %8x\r\n", r->iocurlim);
	SOC_LOGI("	vanaldosel: %8x\r\n", r->vanaldosel);
	SOC_LOGI("	aldohp: %8x\r\n", r->aldohp);
	SOC_LOGI("	anacurlim: %8x\r\n", r->anacurlim);
	SOC_LOGI("	envrefh1v: %8x\r\n", r->envrefh1v);
	SOC_LOGI("	violdosel: %8x\r\n", r->violdosel);
	SOC_LOGI("	vbatdetsel: %8x\r\n", r->vbatdetsel);
	SOC_LOGI("	nc_18_18: %8x\r\n", r->nc_18_18);
	SOC_LOGI("	spi_pwd_regpow: %8x\r\n", r->spi_pwd_regpow);
	SOC_LOGI("	bypassen: %8x\r\n", r->bypassen);
	SOC_LOGI("	ioldolp: %8x\r\n", r->ioldolp);
	SOC_LOGI("	bgcal: %8x\r\n", r->bgcal);
	SOC_LOGI("	vbgcalmode: %8x\r\n", r->vbgcalmode);
	SOC_LOGI("	vbgcalstart: %8x\r\n", r->vbgcalstart);
	SOC_LOGI("	pwd_bgcal: %8x\r\n", r->pwd_bgcal);
	SOC_LOGI("	spi_envbg: %8x\r\n", r->spi_envbg);
}

static void sys_dump_ana_reg8(void)
{
	sys_ana_reg8_t *r = (sys_ana_reg8_t *)(SOC_SYS_REG_BASE + (0x48 << 2));

	SOC_LOGI("ana_reg8: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x48 << 2)));
	SOC_LOGI("	asoft_stc: %8x\r\n", r->asoft_stc);
	SOC_LOGI("	rst_timerwks1v: %8x\r\n", r->rst_timerwks1v);
	SOC_LOGI("	dldo_czsel: %8x\r\n", r->dldo_czsel);
	SOC_LOGI("	digcurlim: %8x\r\n", r->digcurlim);
	SOC_LOGI("	spi_latch1v: %8x\r\n", r->spi_latch1v);
	SOC_LOGI("	dldo_rzsel: %8x\r\n", r->dldo_rzsel);
	SOC_LOGI("	lvsleep_wkrst: %8x\r\n", r->lvsleep_wkrst);
	SOC_LOGI("	gpiowk_rstn: %8x\r\n", r->gpiowk_rstn);
	SOC_LOGI("	rtcwk_rstn: %8x\r\n", r->rtcwk_rstn);
	SOC_LOGI("	ensfsdd: %8x\r\n", r->ensfsdd);
	SOC_LOGI("	vcorehsel: %8x\r\n", r->vcorehsel);
	SOC_LOGI("	vcorelsel: %8x\r\n", r->vcorelsel);
	SOC_LOGI("	vlden: %8x\r\n", r->vlden);
	SOC_LOGI("	rst_gpiowks: %8x\r\n", r->rst_gpiowks);
	SOC_LOGI("	aldo_rzsel: %8x\r\n", r->aldo_rzsel);
	SOC_LOGI("	aldo_czsel: %8x\r\n", r->aldo_czsel);
	SOC_LOGI("	vtrxspisel: %8x\r\n", r->vtrxspisel);
}

static void sys_dump_ana_reg9(void)
{
	sys_ana_reg9_t *r = (sys_ana_reg9_t *)(SOC_SYS_REG_BASE + (0x49 << 2));

	SOC_LOGI("ana_reg9: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x49 << 2)));
	SOC_LOGI("	azcd_manu: %8x\r\n", r->azcd_manu);
	SOC_LOGI("	zcdswvs: %8x\r\n", r->zcdswvs);
	SOC_LOGI("	zcdsrefsel: %8x\r\n", r->zcdsrefsel);
	SOC_LOGI("	enzcdcalib: %8x\r\n", r->enzcdcalib);
	SOC_LOGI("	enzcddy: %8x\r\n", r->enzcddy);
	SOC_LOGI("	zcdmsel: %8x\r\n", r->zcdmsel);
	SOC_LOGI("	spi_timerwken: %8x\r\n", r->spi_timerwken);
	SOC_LOGI("	spi_byp32pwd: %8x\r\n", r->spi_byp32pwd);
	SOC_LOGI("	sd: %8x\r\n", r->sd);
	SOC_LOGI("	lburstsel: %8x\r\n", r->lburstsel);
	SOC_LOGI("	ckfs: %8x\r\n", r->ckfs);
	SOC_LOGI("	ckintsel: %8x\r\n", r->ckintsel);
	SOC_LOGI("	osccaltrig: %8x\r\n", r->osccaltrig);
	SOC_LOGI("	mroscsel: %8x\r\n", r->mroscsel);
	SOC_LOGI("	mrosci_cal: %8x\r\n", r->mrosci_cal);
	SOC_LOGI("	mrosccap_cal: %8x\r\n", r->mrosccap_cal);
}

static void sys_dump_ana_reg10(void)
{
	sys_ana_reg10_t *r = (sys_ana_reg10_t *)(SOC_SYS_REG_BASE + (0x4a << 2));

	SOC_LOGI("ana_reg10: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4a << 2)));
	SOC_LOGI("	sfsr: %8x\r\n", r->sfsr);
	SOC_LOGI("	ensfsaa: %8x\r\n", r->ensfsaa);
	SOC_LOGI("	apfms: %8x\r\n", r->apfms);
	SOC_LOGI("	atmpo_sel: %8x\r\n", r->atmpo_sel);
	SOC_LOGI("	ampoen: %8x\r\n", r->ampoen);
	SOC_LOGI("	nc_13_13: %8x\r\n", r->nc_13_13);
	SOC_LOGI("	avea_sel: %8x\r\n", r->avea_sel);
	SOC_LOGI("	aforcepfm: %8x\r\n", r->aforcepfm);
	SOC_LOGI("	acls: %8x\r\n", r->acls);
	SOC_LOGI("	aswrsten: %8x\r\n", r->aswrsten);
	SOC_LOGI("	aripc: %8x\r\n", r->aripc);
	SOC_LOGI("	arampc: %8x\r\n", r->arampc);
	SOC_LOGI("	arampcen: %8x\r\n", r->arampcen);
	SOC_LOGI("	aenburst: %8x\r\n", r->aenburst);
	SOC_LOGI("	apfmen: %8x\r\n", r->apfmen);
	SOC_LOGI("	aldosel: %8x\r\n", r->aldosel);
}

static void sys_dump_ana_reg11(void)
{
	sys_ana_reg11_t *r = (sys_ana_reg11_t *)(SOC_SYS_REG_BASE + (0x4b << 2));

	SOC_LOGI("ana_reg11: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4b << 2)));
	SOC_LOGI("	gpiowk: %8x\r\n", r->gpiowk);
	SOC_LOGI("	rtcsel: %8x\r\n", r->rtcsel);
	SOC_LOGI("	timersel: %8x\r\n", r->timersel);
}

static void sys_dump_ana_reg12(void)
{
	SOC_LOGI("ana_reg12: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4c << 2)));
}

static void sys_dump_ana_reg13(void)
{
	SOC_LOGI("ana_reg13: %8x\r\n", REG_READ(SOC_SYS_REG_BASE + (0x4d << 2)));
}

static sys_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, system_dump_device_id},
	{0x1, 0x1, system_dump_version_id},
	{0x2, 0x2, system_dump_cpu_storage_connect_op_select},
	{0x3, 0x3, system_dump_cpu_current_run_status},
	{0x4, 0x4, system_dump_cpu0_int_halt_clk_op},
	{0x5, 0x8, system_dump_rsv_5_7},
	{0x8, 0x8, system_dump_cpu_clk_div_mode1},
	{0x9, 0x9, system_dump_cpu_clk_div_mode2},
	{0xa, 0xa, system_dump_cpu_26m_wdt_clk_div},
	{0xb, 0xb, system_dump_cpu_anaspi_freq},
	{0xc, 0xc, system_dump_cpu_device_clk_enable},
	{0xd, 0xf, system_dump_rsv_d_e},
	{0xf, 0xf, system_dump_cpu_device_mem_ctrl2},
	{0x10, 0x10, system_dump_cpu_power_sleep_wakeup},
	{0x11, 0x11, system_dump_cpu0_lv_sleep_cfg},
	{0x12, 0x12, system_dump_cpu_device_mem_ctrl3},
	{0x13, 0x20, system_dump_rsv_13_1f},
	{0x20, 0x20, system_dump_cpu0_int_0_31_en},
	{0x21, 0x21, system_dump_cpu0_int_32_63_en},
	{0x22, 0x28, system_dump_rsv_22_27},
	{0x28, 0x28, system_dump_cpu0_int_0_31_status},
	{0x29, 0x29, system_dump_cpu0_int_32_63_status},
	{0x2a, 0x30, system_dump_rsv_2a_2f},
	{0x30, 0x30, system_dump_gpio_config0},
	{0x31, 0x31, system_dump_gpio_config1},
	{0x32, 0x32, system_dump_gpio_config2},
	{0x33, 0x38, system_dump_rsv_33_37},
	{0x38, 0x38, system_dump_dbug_config0},
	{0x39, 0x3a, system_dump_rsv_39_39},
	{0x3a, 0x3a, system_dump_anareg_stat},
	{0x3b, 0x40, sys_dump_rsv_3b_3f},
	{0x40, 0x40, sys_dump_ana_reg0},
	{0x41, 0x41, sys_dump_ana_reg1},
	{0x42, 0x42, sys_dump_ana_reg2},
	{0x43, 0x43, sys_dump_ana_reg3},
	{0x44, 0x44, sys_dump_ana_reg4},
	{0x45, 0x45, sys_dump_ana_reg5},
	{0x46, 0x46, sys_dump_ana_reg6},
	{0x47, 0x47, sys_dump_ana_reg7},
	{0x48, 0x48, sys_dump_ana_reg8},
	{0x49, 0x49, sys_dump_ana_reg9},
	{0x4a, 0x4a, sys_dump_ana_reg10},
	{0x4b, 0x4b, sys_dump_ana_reg11},
	{0x4c, 0x4c, sys_dump_ana_reg12},
	{0x4d, 0x4d, sys_dump_ana_reg13},
	{-1, -1, 0}
};

void sys_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
#endif

