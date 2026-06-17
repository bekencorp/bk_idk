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
#include "sys_hal.h"
#include "sys_ll.h"
#include "aon_pmu_hal.h"
#include "gpio_hal.h"
#include "gpio_driver_base.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <driver/hal/hal_spi_types.h>
#include "gpio_hal.h"
#include "timer_hal.h"
#include <driver/pwr_clk.h>
#include "bk_misc.h"

static sys_hal_t s_sys_hal;

#if !CONFIG_BL2_REG_BASE
#if CONFIG_SPE
uint32_t SOC_SYS_REG_BASE = 0x44010000;
#else
uint32_t SOC_SYS_REG_BASE = 0x54010000;
#endif
#endif

#define PM_CLKSEL_CORE_320M                 (2)
#define PM_CLKSEL_CORE_480M                 (3)
#define PM_CLKSEL_FLASH_480M                (0x1)
#define PM_CLKDIV_CORE_0                    (0)
#define PM_CLKDIV_CORE_1                    (1)
#define PM_VDDD_H_VOL_1V                    (0x6)
#define PM_VDDDIG_H_VOL_0v9                 (0xC)
#define PM_CLKDV_CPU1_1                     (0x1)
#define PM_CLKDV_CPU0_0                     (0x0)
#define SYS_SWITCH_VDDDIG_VOL_DELAY_TIME    (2600)

/** Platform Misc Start **/
bk_err_t sys_hal_init()
{
	s_sys_hal.hw = (sys_hw_t *)SYS_LL_REG_BASE;
	return BK_OK;
}
/** Platform Misc End **/


void sys_hal_usb_enable_clk(bool en)
{
	sys_ll_set_cpu_device_clk_enable_usb_cken(en);
}

void sys_hal_flash_set_dco(void)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(FLASH_CLK_DPLL);
}

void sys_hal_flash_set_dpll(void)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(FLASH_CLK_APLL);
}

void sys_hal_flash_set_clk(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(value);
}

void sys_hal_flash_set_clk_div(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(value);
}

/* REG_0x09:cpu_clk_div_mode2->cksel_flash:0:XTAL  1:APLL  1x :clk_120M,R/W,0x9[25:24]*/
uint32_t sys_hal_flash_get_clk_sel(void)
{
	return sys_ll_get_cpu_clk_div_mode2_cksel_flash();
}

/* REG_0x09:cpu_clk_div_mode2->ckdiv_flash:0:/1  1:/2  2:/4  3:/8,R/W,0x9[27:26]*/
uint32_t sys_hal_flash_get_clk_div(void)
{
	return sys_ll_get_cpu_clk_div_mode2_ckdiv_flash();
}

/** Flash end **/

/*for low power function start*/
static void sys_hal_delay(volatile uint32_t times)
{
	while (times--);
}

bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value)
{
	if(sys_ll_get_ana_reg9_vcorehsel() != vol_value)
	{
		sys_ll_set_ana_reg9_spi_latch1v(1);
		sys_ll_set_ana_reg9_vcorehsel(vol_value);
		sys_ll_set_ana_reg9_spi_latch1v(0);
		sys_hal_delay(SYS_SWITCH_VDDDIG_VOL_DELAY_TIME);//when cpu0 max freq 240m ,it delay 10uS for voltage stability
	}

	return BK_OK;
}

void sys_hal_all_modules_clk_div_set(clk_div_reg_e reg, uint32_t value)
{
    clk_div_address_map_t clk_div_address_map_table[] = CLK_DIV_ADDRESS_MAP;
    clk_div_address_map_t *clk_div_addr = &clk_div_address_map_table[reg];

    uint32_t clk_div_reg_address = clk_div_addr->reg_address;

	REG_WRITE(clk_div_reg_address, value);
}
uint32_t sys_hal_all_modules_clk_div_get(clk_div_reg_e reg)
{
    clk_div_address_map_t clk_div_address_map_table[] = CLK_DIV_ADDRESS_MAP;
    clk_div_address_map_t *clk_div_addr = &clk_div_address_map_table[reg];

	 uint32_t clk_div_reg_address = clk_div_addr->reg_address;

	return REG_READ(clk_div_reg_address);
}

void sys_hal_cpu_clk_div_set(uint32_t core_index, uint32_t value)
{
	if(core_index == 0)//cpu0
	{
		sys_ll_set_cpu0_int_halt_clk_op_cpu0_speed(value);
	}
	else if(core_index == 1)//cpu1
	{
		sys_ll_set_cpu1_int_halt_clk_op_cpu1_speed(value);
	}
	else if(core_index == 2)//cpu2
	{
		sys_ll_set_cpu2_int_halt_clk_op_cpu2_speed(value);
	}
}
uint32 sys_hal_get_chip_id(void)
{
	return sys_ll_get_version_id_versionid();
}

uint32 sys_hal_get_device_id(void)
{
	return sys_ll_get_device_id_deviceid();
}


int32 sys_hal_int_disable(uint32 param) //CMD_ICU_INT_DISABLE
{
	uint32 reg = 0;

#if CONFIG_SYS_CPU0
	reg = sys_ll_get_cpu0_int_0_31_en_value();
	reg &= ~(param);
	sys_ll_set_cpu0_int_0_31_en_value(reg);
#else
	reg = sys_ll_get_cpu1_int_0_31_en_value();
	reg &= ~(param);
	sys_ll_set_cpu1_int_0_31_en_value(reg);
#endif

	return 0;
}

int32 sys_hal_int_enable(uint32 param) //CMD_ICU_INT_ENABLE
{
	uint32 reg = 0;

#if CONFIG_SYS_CPU0
	reg = sys_ll_get_cpu0_int_0_31_en_value();
	reg |= (param);
	sys_ll_set_cpu0_int_0_31_en_value(reg);
#else
	reg = sys_ll_get_cpu1_int_0_31_en_value();
	reg |= (param);
	sys_ll_set_cpu1_int_0_31_en_value(reg);
#endif

	return 0;
}

//NOTICE:Temp add for BK7256 product which has more then 32 Interrupt sources
int32 sys_hal_int_group2_disable(uint32 param)
{
	uint32 reg = 0;

#if CONFIG_SYS_CPU0
	reg = sys_ll_get_cpu0_int_32_63_en_value();
	reg &= ~(param);
	sys_ll_set_cpu0_int_32_63_en_value(reg);
#else
	reg = sys_ll_get_cpu1_int_32_63_en_value();
	reg &= ~(param);
	sys_ll_set_cpu1_int_32_63_en_value(reg);
#endif

	return 0;
}

//NOTICE:Temp add for BK7256 product which has more then 32 Interrupt sources
int32 sys_hal_int_group2_enable(uint32 param)
{
	uint32 reg = 0;

#if CONFIG_SYS_CPU0
	reg = sys_ll_get_cpu0_int_32_63_en_value();
	reg |= (param);
	sys_ll_set_cpu0_int_32_63_en_value(reg);
#else
	reg = sys_ll_get_cpu1_int_32_63_en_value();
	reg |= (param);
	sys_ll_set_cpu1_int_32_63_en_value(reg);
#endif

	return 0;
}

int32 sys_hal_fiq_disable(uint32 param)
{
	uint32 reg = 0;

#if CONFIG_SYS_CPU0
	reg = sys_ll_get_cpu0_int_32_63_en_value();
	reg &= ~(param);
	sys_ll_set_cpu0_int_32_63_en_value(reg);
#else
	reg = sys_ll_get_cpu1_int_32_63_en_value();
	reg &= ~(param);
	sys_ll_set_cpu1_int_32_63_en_value(reg);
#endif

	return 0;
}

int32 sys_hal_fiq_enable(uint32 param)
{
	uint32 reg = 0;

#if CONFIG_SYS_CPU0
	reg = sys_ll_get_cpu0_int_32_63_en_value();
	reg |= (param);
	sys_ll_set_cpu0_int_32_63_en_value(reg);
#else
	reg = sys_ll_get_cpu1_int_32_63_en_value();
	reg |= (param);
	sys_ll_set_cpu1_int_32_63_en_value(reg);
#endif

	return 0;
}

//	GLOBAL_INT_DECLARATION();	GLOBAL_INT_DISABLE();
int32 sys_hal_global_int_enable(uint32 param)
{
	int32 ret = 0;

	return ret;
}

uint32 sys_hal_get_int_status(void)
{
#if CONFIG_SYS_CPU0
	return sys_ll_get_cpu0_int_0_31_status_value();
#else
	return sys_ll_get_cpu1_int_0_31_status_value();
#endif
}

uint32_t sys_hal_get_cpu0_gpio_int_st(void)
{
    return sys_ll_get_cpu0_int_32_63_status_cpu0_gpio_int_st();
}

//NOTICE:INT source status is read only and can't be set, other projects is error, we'll delete them.
int32 sys_hal_set_int_status(uint32 param)
{
	return 0;
}

uint32 sys_hal_get_fiq_reg_status(void)
{
	uint32 reg = 0;

#if CONFIG_SYS_CPU0
	reg = sys_ll_get_cpu0_int_32_63_status_value();
#else
	reg = sys_ll_get_cpu1_int_32_63_status_value();
#endif

	return reg;
}

uint32 sys_hal_set_fiq_reg_status(uint32 param)
{
	uint32 reg = 0;

	///TODO:this reg is read only

	return reg;
}

uint32 sys_hal_get_intr_raw_status(void)
{
	uint32 reg = 0;

	///TODO:
	reg = sys_ll_get_cpu0_int_0_31_status_value();

	return reg;
}

uint32 sys_hal_set_intr_raw_status(uint32 param)
{
	uint32 reg = 0;

	///TODO:this reg is read only

	return reg;
}

int32 sys_hal_set_jtag_mode(uint32 param)
{
	int32 ret = 0;
	//sys_ll_set_cpu_storage_connect_op_select_jtag_core_sel(param);
	return ret;
}

uint32 sys_hal_get_jtag_mode(void)
{
	uint32 reg = 0;
	//reg = sys_ll_get_cpu_storage_connect_op_select_jtag_core_sel();
	return reg;
}

/* NOTICE: NOTICE: NOTICE: NOTICE: NOTICE: NOTICE: NOTICE
 * BK7256 clock, power is different with previous products(2022-01-10).
 * Previous products peripheral devices use only one signal of clock enable.
 * BK7256 uses clock and power signal to control one device,
 * This function only enable clock signal, we needs to enable power signal also
 * if we want to enable one device.
 */
void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up)
{
	uint32_t v = 0;
	uint32_t offset = 0;

	if (dev >= CLK_PWR_ID_H264) {
		offset += CLK_PWR_ID_H264;
		v = sys_ll_get_reserver_reg0xd_value();
	} else {
		offset = 0;
		v = sys_ll_get_cpu_device_clk_enable_value();
	}

	if(CLK_PWR_CTRL_PWR_UP == power_up)
		v |= (1 << (dev - offset));
	else
		v &= ~(1 << (dev - offset));

	if (dev >= CLK_PWR_ID_H264) {
		sys_ll_set_reserver_reg0xd_value(v);
	} else {
		sys_ll_set_cpu_device_clk_enable_value(v);
	}
}

/* UART select clock **/
void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode)
{
	int sel_xtal = 0;
	int sel_appl = 1;

	switch(id)
	{
		case UART_ID_0:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode1_clksel_uart0(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode1_clksel_uart0(sel_xtal);
				break;
			}
		case UART_ID_1:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode1_cksel_uart1(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode1_cksel_uart1(sel_xtal);
				break;
			}
		case UART_ID_2:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_xtal);
				break;
			}
		default:
			break;
	}

}

uint32_t sys_hal_uart_select_clock_get(uart_id_t id)
{
	(void)id;
	return UART_SCLK_XTAL_26M;
}

void sys_hal_pwm_select_clock(sys_sel_pwm_t num, pwm_src_clk_t mode)
{
	int sel_clk32 = 0;
	int sel_xtal = 1;

	switch(num)
	{
		case SYS_SEL_PWM0:
			if(mode == PWM_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode1_cksel_pwm0(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode1_cksel_pwm0(sel_clk32);
			break;
		case SYS_SEL_PWM1:
			if(mode == PWM_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode1_cksel_pwm1(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode1_cksel_pwm1(sel_clk32);
			break;

		default:
			break;
	}
}

void sys_hal_timer_select_clock(sys_sel_timer_t num, timer_src_clk_t mode)
{
	int sel_clk32 = 0;
	int sel_xtal = 1;

	switch(num)
	{
		case SYS_SEL_TIMER0:
			if(mode == TIMER_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode1_cksel_tim0(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode1_cksel_tim0(sel_clk32);
			break;
		case SYS_SEL_TIMER1:
			if(mode == TIMER_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode1_cksel_tim1(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode1_cksel_tim1(sel_clk32);
			break;

		default:
			break;
	}
}

uint32_t sys_hal_timer_select_clock_get(sys_sel_timer_t id)
{
    uint32_t ret = 0;

    switch(id)
    {
        case SYS_SEL_TIMER0:
        {
            ret = sys_ll_get_cpu_clk_div_mode1_cksel_tim0();
            break;
        }
        case SYS_SEL_TIMER1:
        {
            ret = sys_ll_get_cpu_clk_div_mode1_cksel_tim1();
            break;
        }
        default:
            break;
    }

    ret = (ret)?TIMER_SCLK_XTAL:TIMER_SCLK_CLK32;

    return ret;
}

void sys_hal_qspi_clk_sel(uint32_t id, uint32_t param)
{
	switch (id) {
	case 0: /* QSPI_ID_0 */
		sys_ll_set_cpu_clk_div_mode2_cksel_qspi0(param);
		break;
#if (SOC_QSPI_UNIT_NUM > 1)
	case 1: /* QSPI_ID_1 */
		sys_ll_set_cpu_26m_wdt_clk_div_cksel_qspi1(param);
		break;
#endif
	default:
		break;
	}
}

void sys_hal_qspi_set_src_clk_div(uint32_t id, uint32_t value)
{
	switch (id) {
	case 0: /* QSPI_ID_0 */
		sys_ll_set_cpu_clk_div_mode2_ckdiv_qspi0(value);
		break;
#if (SOC_QSPI_UNIT_NUM > 1)
	case 1: /* QSPI_ID_1 */
		sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_qspi1(value);
		break;
#endif
	default:
		break;
	}
}

#if 1	//tmp build
void sys_hal_set_clk_select(dev_clk_select_id_t dev, dev_clk_select_t clk_sel)
{
	switch (dev) {
	case CLK_SEL_ID_SPI0:
		if (clk_sel == CLK_SEL_APLL) {
			sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi0(SPI_CLK_APLL);
		} else {
			sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi0(SPI_CLK_XTAL);
		}
		break;
	case CLK_SEL_ID_SPI1:
		if (clk_sel == CLK_SEL_APLL) {
			sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi1(SPI_CLK_APLL);
		} else {
			sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi1(SPI_CLK_XTAL);
		}
		break;
	case CLK_SEL_ID_QSPI0:
		if (clk_sel == CLK_SEL_320M) {
			sys_ll_set_cpu_clk_div_mode2_cksel_qspi0(QSPI_CLK_320M);
		} else {
			sys_ll_set_cpu_clk_div_mode2_cksel_qspi0(QSPI_CLK_480M);
		}
		break;
	case CLK_SEL_ID_QSPI1:
		if (clk_sel == CLK_SEL_320M) {
			sys_ll_set_cpu_26m_wdt_clk_div_cksel_qspi1(QSPI_CLK_320M);
		} else {
			sys_ll_set_cpu_26m_wdt_clk_div_cksel_qspi1(QSPI_CLK_480M);
		}
		break;
	case CLK_SEL_ID_DISP:
		if (clk_sel == CLK_SEL_320M) {
			sys_ll_set_cpu_clk_div_mode2_cksel_disp(0);
		} else {
			sys_ll_set_cpu_clk_div_mode2_cksel_disp(1);
		}
		break;
	case CLK_SEL_ID_PSRAM:
		if (clk_sel == CLK_SEL_320M) {
			sys_ll_set_cpu_clk_div_mode2_cksel_psram(0);
		} else {
			sys_ll_set_cpu_clk_div_mode2_cksel_psram(1);
		}
		break;
	case CLK_SEL_ID_SDIO:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode2_cksel_sdio(0);
		} else {
			sys_ll_set_cpu_clk_div_mode2_cksel_sdio(1);
		}
		break;
	case CLK_SEL_ID_AUXS:
		if (clk_sel == CLK_SEL_DCO) {
			sys_ll_set_cpu_clk_div_mode2_cksel_auxs(0);
		} else if (clk_sel == CLK_SEL_APLL) {
			sys_ll_set_cpu_clk_div_mode2_cksel_auxs(1);
		} else if (clk_sel == CLK_SEL_320M) {
			sys_ll_set_cpu_clk_div_mode2_cksel_auxs(2);
		} else {
			sys_ll_set_cpu_clk_div_mode2_cksel_auxs(3);
		}
		break;
	case CLK_SEL_ID_FLASH:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode2_cksel_flash(0);
		} else if (clk_sel == CLK_SEL_DPLL) {
			sys_ll_set_cpu_clk_div_mode2_cksel_flash(1);
		} else {
			sys_ll_set_cpu_clk_div_mode2_cksel_flash(2);
		}
		break;
	case CLK_SEL_ID_I2S:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_i2s(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_i2s(1);
		}
		break;
	case CLK_SEL_ID_CORE:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_core(0);
		} else if (clk_sel == CLK_SEL_DCO) {
			sys_ll_set_cpu_clk_div_mode1_cksel_core(1);
		} else if (clk_sel == CLK_SEL_320M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_core(2);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_core(3);
		}
		break;
	case CLK_SEL_ID_UART0:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode1_clksel_uart0(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_clksel_uart0(1);
		}
		break;
	case CLK_SEL_ID_UART1:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_uart1(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_uart1(1);
		}
		break;
	case CLK_SEL_ID_UART2:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_uart2(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_uart2(1);
		}
		break;
	case CLK_SEL_ID_SADC:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_sadc(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_sadc(1);
		}
		break;
	case CLK_SEL_ID_PWM0:
		if (clk_sel == CLK_SEL_32K) {
			sys_ll_set_cpu_clk_div_mode1_cksel_pwm0(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_pwm0(1);
		}
		break;
	case CLK_SEL_ID_PWM1:
		if (clk_sel == CLK_SEL_32K) {
			sys_ll_set_cpu_clk_div_mode1_cksel_pwm1(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_pwm1(1);
		}
		break;
	case CLK_SEL_ID_TIMER0:
		if (clk_sel == CLK_SEL_32K) {
			sys_ll_set_cpu_clk_div_mode1_cksel_tim0(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_tim0(1);
		}
		break;
	case CLK_SEL_ID_TIMER1:
		if (clk_sel == CLK_SEL_32K) {
			sys_ll_set_cpu_clk_div_mode1_cksel_tim1(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_tim1(1);
		}
		break;
	case CLK_SEL_ID_TIMER2:
		if (clk_sel == CLK_SEL_32K) {
			sys_ll_set_cpu_clk_div_mode1_cksel_tim2(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_tim2(1);
		}
		break;
	case CLK_SEL_ID_AUDIO:
		if (clk_sel == CLK_SEL_XTL_26M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_aud(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_aud(1);
		}
		break;
	case CLK_SEL_ID_JPEG:
		if (clk_sel == CLK_SEL_320M) {
			sys_ll_set_cpu_clk_div_mode1_cksel_jpeg(0);
		} else {
			sys_ll_set_cpu_clk_div_mode1_cksel_jpeg(1);
		}
		break;
	default:
		break;
	}
}

dev_clk_select_t sys_hal_get_clk_select(dev_clk_select_id_t dev)
{
	//tmp build
	return 0;
}

void sys_hal_set_clk_div(dev_clk_select_id_t dev, uint32_t clk_div)
{
	switch (dev) {
	case CLK_SEL_ID_26M:
		if (clk_div == CLK_DIV_2) {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_26m(1);
		} else if (clk_div == CLK_DIV_4) {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_26m(2);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_26m(3);
		} else {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_26m(0);
		}
		break;
	case CLK_SEL_ID_WDT:
		if (clk_div == CLK_DIV_4) {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_wdt(1);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_wdt(2);
		} else if (clk_div == CLK_DIV_16) {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_wdt(3);
		} else {
			sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_wdt(0);
		}
		break;
	case CLK_SEL_ID_QSPI0:
		sys_ll_set_cpu_clk_div_mode2_ckdiv_qspi0(clk_div - 1);
		break;
	case CLK_SEL_ID_QSPI1:
		sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_qspi1(clk_div - 1);
		break;
	case CLK_SEL_ID_PSRAM:
		sys_ll_set_cpu_clk_div_mode2_ckdiv_psram(clk_div - 1);
		break;
	case CLK_SEL_ID_SDIO:
		if (clk_div == CLK_DIV_4) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(1);
		} else if (clk_div == CLK_DIV_6) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(2);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(3);
		} else if (clk_div == CLK_DIV_10) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(4);
		} else if (clk_div == CLK_DIV_12) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(5);
		} else if (clk_div == CLK_DIV_16) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(6);
		} else if (clk_div == CLK_DIV_256) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(7);
		} else {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(0);
		}
		break;
	case CLK_SEL_ID_AUXS:
		sys_ll_set_cpu_clk_div_mode2_ckdiv_auxs(clk_div - 1);
		break;
	case CLK_SEL_ID_FLASH:
		if (clk_div == CLK_DIV_6) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(1);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(2);
		} else if (clk_div == CLK_DIV_10) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(3);
		} else {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0);
		}
		break;
	case CLK_SEL_ID_I2S:
		if (clk_div == CLK_DIV_2) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(1);
		} else if (clk_div == CLK_DIV_4) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(2);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(3);
		} else if (clk_div == CLK_DIV_16) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(4);
		} else if (clk_div == CLK_DIV_32) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(5);
		} else if (clk_div == CLK_DIV_64) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(6);
		} else if (clk_div == CLK_DIV_256) {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(7);
		} else {
			sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(0);
		}
		break;
	case CLK_SEL_ID_CORE:
		sys_ll_set_cpu_clk_div_mode1_clkdiv_core(clk_div - 1);
		break;
	case CLK_SEL_ID_UART0:
		if (clk_div == CLK_DIV_2) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart0(1);
		} else if (clk_div == CLK_DIV_4) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart0(2);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart0(3);
		} else {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart0(0);
		}
		break;
	case CLK_SEL_ID_UART1:
		if (clk_div == CLK_DIV_2) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart1(1);
		} else if (clk_div == CLK_DIV_4) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart1(2);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart1(3);
		} else {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart1(0);
		}
		break;
	case CLK_SEL_ID_UART2:
		if (clk_div == CLK_DIV_2) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart2(1);
		} else if (clk_div == CLK_DIV_4) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart2(2);
		} else if (clk_div == CLK_DIV_8) {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart2(3);
		} else {
			sys_ll_set_cpu_clk_div_mode1_clkdiv_uart2(0);
		}
		break;
	case CLK_SEL_ID_JPEG:
		sys_ll_set_cpu_clk_div_mode1_clkdiv_jpeg(clk_div - 1);
		break;
	case CLK_SEL_ID_DISP:
		break;
	default:
		break;
	}
}

//DCO divider is valid for all of the peri-devices.
void sys_hal_set_dco_div(dev_clk_dco_div_t div)
{
	//tmp build
}

//DCO divider is valid for all of the peri-devices.
dev_clk_dco_div_t sys_hal_get_dco_div(void)
{
	//tmp build
	return 0;
}
#endif	//temp build

/*clock power control end*/

void sys_hal_sadc_int_enable(void)
{
    sys_hal_int_enable(SADC_INTERRUPT_CTRL_BIT);
}

void sys_hal_sadc_int_disable(void)
{
    sys_hal_int_disable(SADC_INTERRUPT_CTRL_BIT);
}

void sys_hal_sadc_pwr_up(void)
{
	sys_ll_set_cpu_device_clk_enable_sadc_cken(1);
}

void sys_hal_sadc_pwr_down(void)
{
	sys_ll_set_cpu_device_clk_enable_sadc_cken(0);
}

void sys_hal_en_tempdet(uint32_t value)
{
    sys_ll_set_ana_reg5_en_temp(value);
}

void sys_hal_mclk_mux_set(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_core(value);
}

void sys_hal_mclk_div_set(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_clkdiv_core(value);
}

/**  Platform End **/




/**  BT Start **/
//BT
void sys_hal_cali_dpll_spi_trig_disable(void)
{
    sys_ll_set_ana_reg0_spitrig(0);
}

void sys_hal_cali_dpll_spi_trig_enable(void)
{
    sys_ll_set_ana_reg0_spitrig(1);
}

void sys_hal_cali_dpll_spi_detect_disable(void)
{
    sys_ll_set_ana_reg0_spideten(0);
}

void sys_hal_cali_dpll_spi_detect_enable(void)
{
    sys_ll_set_ana_reg0_spideten(1);
}

void sys_hal_analog_set(analog_reg_t reg, uint32_t value)
{
    uint32_t analog_reg_address;

    if ((reg < ANALOG_REG0) || (reg >= ANALOG_MAX)) {
        return;
    }

    analog_reg_address = SYS_ANA_REG0_ADDR + (reg - ANALOG_REG0) * 4;

	sys_ll_set_analog_reg_value(analog_reg_address, value);
}
uint32_t sys_hal_analog_get(analog_reg_t reg)
{
    uint32_t analog_reg_address;

    if ((reg < ANALOG_REG0) || (reg >= ANALOG_MAX)) {
        return 0;
    }

    analog_reg_address = SYS_ANA_REG0_ADDR + (reg - ANALOG_REG0) * 4;

	return sys_ll_get_analog_reg_value(analog_reg_address);
}
void sys_hal_aud_clock_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_aud_cken(value);
}

void sys_hal_fft_disckg_set(uint32_t value)
{
	//sys_ll_set_cpu_mode_disckg1_fft_disckg(value);
}

void sys_hal_cpu_fft_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_fft_int_en(value);
}

/**  FFT End  **/

/**  I2S Start  **/
void sys_hal_i2s_select_clock(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_i2s(value);
}

void sys_hal_i2s_clock_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_i2s_cken(value);
}

void sys_hal_i2s1_clock_en(uint32_t value)
{
	sys_ll_set_reserver_reg0xd_i2s1_cken(value);
}

void sys_hal_i2s2_clock_en(uint32_t value)
{
	sys_ll_set_reserver_reg0xd_i2s2_cken(value);
}

void sys_hal_i2s_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_i2s0_int_en(value);
}

void sys_hal_i2s1_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_i2s1_int_en(value);
}

void sys_hal_i2s2_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_i2s2_int_en(value);
}

void sys_hal_i2s_disckg_set(uint32_t value)
{
	//NOT SUPPORT
}

void sys_hal_psram_clk_sel(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_psram(value);
}

void sys_hal_psram_set_clkdiv(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_ckdiv_psram(value);
}

/**  psram End **/

/* REG_0x03:cpu_storage_connect_op_select->flash_sel:0: normal flash operation 1:flash download by spi,R/W,0x3[9]*/
uint32_t sys_hal_get_cpu_storage_connect_op_select_flash_sel(void)
{
    return sys_ll_get_cpu_storage_connect_op_select_flash_sel();
}

void sys_hal_set_cpu_storage_connect_op_select_flash_sel(uint32_t value)
{
    sys_ll_set_cpu_storage_connect_op_select_flash_sel(value);
}

void sys_hal_set_sys2flsh_2wire(uint32_t value)
{   
        sys_ll_set_sys2flsh_2wire(value);
}

/**  Misc Start **/
//Misc
/**  Misc End **/

void sys_hal_set_ana_trxt_tst_enable(uint32_t value)
{
	//sys_ll_set_ana_reg5_trxt_tst_enable(value);
}


void sys_hal_set_ana_scal_en(uint32_t value)
{
	//sys_ll_set_ana_reg7_scal_en(value);
}


void sys_hal_set_ana_gadc_buf_ictrl(uint32_t value)
{
   //sys_ll_set_ana_reg7_gadc_buf_ictrl(value);
}

void sys_hal_set_ana_gadc_cmp_ictrl(uint32_t value)
{
    //sys_ll_set_ana_reg7_gadc_cmp_ictrl(value);
}

void sys_hal_set_ana_pwd_gadc_buf(uint32_t value)
{
	//sys_ll_set_ana_reg6_pwd_gadc_buf(value);
}

void sys_hal_set_ana_vref_sel(uint32_t value)
{
	//sys_ll_set_ana_reg7_vref_sel(value);
}
void sys_hal_set_ana_cb_cal_manu(uint32_t value)
{
    sys_ll_set_ana_reg5_bcal_en(value);
}

void sys_hal_set_ana_cb_cal_trig(uint32_t value)
{
    sys_ll_set_ana_reg5_bcal_start(value);
}
void sys_hal_set_ana_vlsel_ldodig(uint32_t value)
{
    //sys_ll_set_ana_reg3_vlsel_ldodig(value);
}
void sys_hal_set_ana_vhsel_ldodig(uint32_t value)
{
    //sys_ll_set_ana_reg3_vhsel_ldodig(value);
}

void sys_hal_set_sdio_clk_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_sdio_cken(value);
}

void sys_hal_set_cpu0_sdio_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_sdio_int_en(value);
}

void sys_hal_set_cpu1_sdio_int_en(uint32_t value)
{
	sys_ll_set_cpu1_int_0_31_en_cpu1_sdio_int_en(value);
}

void sys_hal_set_sdio_clk_div(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_ckdiv_sdio(value);
}

uint32_t sys_hal_get_sdio_clk_div()
{
	return sys_ll_get_cpu_clk_div_mode2_ckdiv_sdio();
}

void sys_hal_set_sdio_clk_sel(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_sdio(value);
}

uint32_t sys_hal_get_sdio_clk_sel()
{
	return sys_ll_get_cpu_clk_div_mode2_cksel_sdio();
}


void sys_hal_set_ana_vctrl_sysldo(uint32_t value)
{
    //sys_ll_set_ana_reg5_vctrl_sysldo(value);
}

void sys_hal_set_yuv_buf_clock_en(uint32_t value)
{
	sys_ll_set_reserver_reg0xd_yuv_cken(value);
}

void sys_hal_set_h264_clock_en(uint32_t value)
{
	sys_ll_set_reserver_reg0xd_h264_cken(value);
}

void sys_hal_nmi_wdt_set_clk_div(uint32_t value)
{
	sys_ll_set_cpu_26m_wdt_clk_div_ckdiv_wdt(value);
}

uint32_t sys_hal_nmi_wdt_get_clk_div(void)
{
	return sys_ll_get_cpu_26m_wdt_clk_div_ckdiv_wdt();
}


void sys_hal_set_ana_cb_cal_manu_val(uint32_t value)
{
    sys_ll_set_ana_reg5_vbias(value);
}

uint32_t sys_hal_cali_dpll(uint32_t param)
{
    sys_hal_cali_dpll_spi_trig_disable();

    if (!param)
    {
        sys_hal_delay(120);
    }
    else
    {
        sys_hal_delay(60);
    }

    sys_hal_cali_dpll_spi_trig_enable();
    sys_hal_cali_dpll_spi_detect_disable();

    if (!param)
    {
        sys_hal_delay(3400);
    }
    else
    {
        sys_hal_delay(340);
    }

	sys_hal_cali_dpll_spi_detect_enable();

    if (!param)
    {
        sys_hal_delay(3400);
    }
    else
    {
        sys_hal_delay(340);
    }

    return BK_OK;
}

void sys_hal_switch_freq(uint32_t cksel_core, uint32_t ckdiv_core, uint32_t ckdiv_cpu0)
{

	uint32_t clk_param  = 0;
	clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
	if(((clk_param >> 0x4)&0x3) > cksel_core)//when it from the higher frequency to lower frequency
	{
		/*1.core clk select*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x3 << 4);
		clk_param |=  cksel_core << 4;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);

		/*2.config bus and core clk div*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0xF << 0);
		clk_param |=  ckdiv_core << 0;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);

		/*3.config cpu clk div*/
		sys_hal_cpu_clk_div_set(0,ckdiv_cpu0);

	}
	else//when it from the lower frequency to higher frequency
	{
		/*1.config bus and core clk div*/
		if(ckdiv_core == 0)
		{
			sys_hal_cpu_clk_div_set(0,ckdiv_cpu0);//avoid the bus freq > 240m
		}

		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0xF << 0);
		clk_param |=  ckdiv_core << 0;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);

		/*2.config cpu clk div*/
		if(ckdiv_core != 0)
		{
			sys_hal_cpu_clk_div_set(0,ckdiv_cpu0);
		}

		/*3.core clk select*/

		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x3 << 4);
		clk_param |=  cksel_core << 4;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);
	}

}

void sys_hal_dpll_cpu_flash_time_early_init(void)
{
	uint32_t coresel = 0;
	uint32_t corediv = 0;
	//zhangheng20231018: calibrate dpll before use
	/*0x2:320M, 0X3:480M*/
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	coresel = sys_ll_get_cpu_clk_div_mode1_cksel_core();
	corediv = sys_ll_get_cpu_clk_div_mode1_clkdiv_core();
	if((coresel != PM_CLKSEL_CORE_320M) || (coresel != PM_CLKSEL_CORE_480M))
	{
		sys_hal_cali_dpll(0);
	}

	if (((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MPW_V2_3 & PM_CHIP_ID_MASK))
	|| ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MPW_V4 & PM_CHIP_ID_MASK))) {
		sys_hal_flash_set_clk(0x2);
	} else {
		/*default of MP flash 80M = 480/6*/
		if(sys_ll_get_cpu_clk_div_mode2_cksel_flash() != PM_CLKSEL_FLASH_480M)
		{
			if(sys_ll_get_cpu_clk_div_mode2_ckdiv_flash() != 0x1)// 480/6
			{
				sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0x2);
			}
			sys_hal_flash_set_clk(PM_CLKSEL_FLASH_480M);
		}
	}

	/* clk_divd 120MHz,
	 * 1, the core clock is depended on CONFIG_CPU_FREQ_HZ and configSYSTICK_CLOCK_HZ.
	 *    Pay attention to bk_pm_module_vote_cpu_freq,and the routine will switch core
	 *    clock;
	 * 2, sysTick module's clock source is processor clock now;
	 */
	if((coresel != PM_CLKSEL_CORE_320M) && (corediv != 0x1))//160M
	{
		#if 0
		// sys_hal_mclk_div_set(480000000/CONFIG_CPU_FREQ_HZ - 1);
		sys_hal_mclk_div_set(7);
		sys_hal_delay(10000);

		// sys_hal_mclk_mux_set(0x3);/*clock source: DPLL, 480M*/
		sys_hal_mclk_mux_set(0x2);
		#else
		sys_hal_switch_freq(2, 7, 1);
		// sys_hal_ctrl_vdddig_h_vol(0xc);
		#endif
	}
	// timer_hal_us_init();

	/*increase the anaspi_freq, current it only use for lpo: external 32k, 26m/32k*/
	//if(bk_clk_32k_customer_config_get() != PM_LPO_SRC_ROSC)
	{
		sys_ll_set_cpu_anaspi_freq_anaspi_freq(0x2);//ana_sck = bus_clk/((divd+1)*2))
	}

}

void sys_hal_early_init(void)
{
	/*power down rosc, temp modify*/
	// sys_hal_pwd_rosc();

	uint32_t chip_id = aon_pmu_hal_get_chipid();

	uint32_t val = sys_hal_analog_get(ANALOG_REG5);
	val |= (0x1 << 5) | (0x1 << 3) | (0x1 << 2);

	sys_hal_analog_set(ANALOG_REG5,val);

	//donghui20230504: 0:1/7 1:1/5 2:1/3 3:1/1
	sys_ll_set_ana_reg5_adc_div(1); //tenglong20230627 adc_div=1/5 since volt of GPIO <= 3.3V

	val = sys_hal_analog_get(ANALOG_REG0);
	val |= (0x13 << 20) ;
	sys_hal_analog_set(ANALOG_REG0,val);

	sys_hal_analog_set(ANALOG_REG0, 0xF1305B56);  //zhangheng20231020 <0>=0 pdll test disalbe for safe
	sys_ll_set_ana_reg0_dsptrig(1);
	sys_ll_set_ana_reg0_dsptrig(0);

	sys_hal_analog_set(ANALOG_REG2, 0x7E003450); //wangjian20221110 xtal=0x50
	//sys_ll_set_cpu_device_clk_enable_value(0x0c008084);
	sys_hal_analog_set(ANALOG_REG3, 0xC5F00B88);

	/**
	 * attention:
	 * SPI latch must be enable before ana_reg[8~13] modification
	 * and don't forget disable it after that.
	 */
	sys_ll_set_ana_reg9_spi_latch1v(1);
	if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MPW_V2_3 & PM_CHIP_ID_MASK)) {
		sys_hal_analog_set(ANALOG_REG8, 0x57E62FFE);
	} else {
		sys_hal_analog_set(ANALOG_REG8, 0x57E62F26);//shuguang20230414[8:3]7->4: for evm
	}
	//tenglong20231113: <8:5>=5 <3:0>=4 keep GPIO5/4 pullup when powerup
	sys_hal_analog_set(ANALOG_REG9, 0x787BC8A4); //tenglong20231017: revert <15> to 1 for softstart, should not change it
	if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MPW_V2_3 & PM_CHIP_ID_MASK)) {
		sys_hal_analog_set(ANALOG_REG10, 0xC35543C7);//tenglong20230417:rosc config for buck in lowpower
		sys_hal_analog_set(ANALOG_REG11, 0x9FEF31F7);
		sys_hal_analog_set(ANALOG_REG12, 0x9F03EF6F);
		sys_hal_analog_set(ANALOG_REG13, 0x1F6FB3FF);
	} else if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MPW_V4 & PM_CHIP_ID_MASK)) {
		sys_hal_analog_set(ANALOG_REG10, 0xC35543C7);//tenglong20230417:rosc config for buck in lowpower
		sys_hal_analog_set(ANALOG_REG11, 0x977EB9FF);
		sys_hal_analog_set(ANALOG_REG12, 0x977ECA4A);
		sys_hal_analog_set(ANALOG_REG13, 0x547AB0F5);
	} else if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK)){
		sys_hal_analog_set(ANALOG_REG10, 0xC35543C7);//tenglong20230417:rosc config for buck in lowpower
		//default of MP
		//tenglong20231017: SYS_reg0x4B<3:0>=8,SYS_reg0x4C<3:0>=0,SYS_reg0x4D<4:1>=7 for softstart
		sys_hal_analog_set(ANALOG_REG11, 0x907EB878);
		sys_hal_analog_set(ANALOG_REG12, 0x907ECA40);
		sys_hal_analog_set(ANALOG_REG13, 0x727070EE);//tenglong20231020 disable psram/update volt for safe
		sys_hal_analog_set(ANALOG_REG25, 0x961FAA4);

		sys_ll_set_ana_reg3_inbufen0v9(1);
	} else {
		sys_hal_analog_set(ANALOG_REG10, 0xC3D543A7);//tenglong20240123
		//default of MP
		//tenglong20231017: SYS_reg0x4B<3:0>=8,SYS_reg0x4C<3:0>=0,SYS_reg0x4D<4:1>=7 for softstart
		sys_hal_analog_set(ANALOG_REG11, 0xB47E99F8);//tenglong20240418
		sys_hal_analog_set(ANALOG_REG12, 0xB47ECF20);//tenglong20240418
		sys_hal_analog_set(ANALOG_REG13, 0x727070EE);//tenglong20231020 disable psram/update volt for safe
		sys_hal_analog_set(ANALOG_REG25, 0x961FAA4);

		sys_ll_set_ana_reg3_inbufen0v9(1);

	}

	sys_ll_set_ana_reg9_spi_latch1v(0);

	/*early init cpu flash time*/
	// sys_hal_dpll_cpu_flash_time_early_init(chip_id);
}

void sys_hal_early_init_tfm(void)
{
	uint32_t val = sys_hal_analog_get(0x5);
	val |=  0x1 << 5; //DPLL
	sys_hal_analog_set(0x5,val);

        val = sys_hal_analog_get(ANALOG_REG0);
        val |= (0x13 << 20) ;
        sys_hal_analog_set(ANALOG_REG0,val);

        sys_hal_analog_set(ANALOG_REG0, 0xF1305B56);  // triger dpll
        sys_ll_set_ana_reg0_dsptrig(1);
        sys_ll_set_ana_reg0_dsptrig(0);

	sys_hal_flash_set_clk(0x1);
	sys_hal_flash_set_clk_div(0x1);

	/* clk_divd 120MHz,
	 * 1, the core clock is depended on CONFIG_CPU_FREQ_HZ and configSYSTICK_CLOCK_HZ.
	 *    Pay attention to bk_pm_module_vote_cpu_freq,and the routine will switch core
	 *    clock;
	 * 2, sysTick module's clock source is processor clock now;
	 */
	sys_hal_mclk_div_set(480000000/CONFIG_CPU_FREQ_HZ - 1);
	sys_hal_delay(10000);
	sys_hal_mclk_mux_set(0x3);/*clock source: DPLL, 480M*/
}

void sys_hal_early_deinit(void)
{
}

void sys_hal_set_base_addr(uint32_t addr)
{
	s_sys_hal.hw = (sys_hw_t *)addr;
#if !CONFIG_BL2_REG_BASE
	SOC_SYS_REG_BASE = addr;
#endif
	return BK_OK;
}
