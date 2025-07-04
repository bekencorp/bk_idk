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
#include "sys_ana_ll.h"
#include "aon_pmu_hal.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include "timer_hal.h"
#include <driver/pwr_clk.h>
#include "bk_misc.h"

#if CONFIG_SOC_BK7236_SMP_TEMP
#include "cpu_id.h"
#endif

#define __IRAM_SEC    __attribute__((section(".iram")))
extern void bk_delay_us(UINT32 us);
static void sys_hal_delay(volatile uint32_t times)
{
    while(times--);
}
#define CLKSEL_CORE_480M                 (1)
#define CLKSEL_FLASH_80M                (0x2)

static sys_hal_t s_sys_hal;

/**  Platform Start **/
/** Platform Misc Start **/
bk_err_t sys_hal_init()
{
	s_sys_hal.hw = (sys_hw_t *)SYS_LL_REG_BASE;
	return BK_OK;
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

void sys_hal_set_sys2flsh_2wire(uint32_t value)
{
	//TODO
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

uint32_t sys_hal_mclk_mux_get(void)
{
	UINT32 ret = 0;

	ret = sys_ll_get_cpu_clk_div_mode1_cksel_core();

	return ret;
}

void sys_hal_mclk_mux_set(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_core(value);
}

uint32_t sys_hal_mclk_div_get(void)
{
	UINT32 ret = 0;

	ret = sys_ll_get_cpu_clk_div_mode1_clkdiv_core();

	return ret;
}

void sys_hal_mclk_div_set(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_clkdiv_core(value);
}

/*for low power function end*/
/*sleep feature end*/

uint32 sys_hal_get_chip_id(void)
{
	return sys_ll_get_version_id_versionid();
}

uint32 sys_hal_get_device_id(void)
{
	return sys_ll_get_device_id_deviceid();
}

/* NOTICE: NOTICE: NOTICE: NOTICE: NOTICE: NOTICE: NOTICE
 * BK7256 clock, power is different with previous products(2022-01-10).
 * Previous products peripheral devices use only one signal of clock enable.
 * BK7256 uses clock and power signal to control one device,
 * This function only enable clock signal, we needs to enable power signal also
 * if we want to enable one device.
 */
__IRAM_SEC void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up)
{
	uint32_t v = 0;

    BK_ASSERT(CLK_PWR_ID_NONE > dev);

	v = sys_ll_get_cpu_device_clk_enable_value();

	if(CLK_PWR_CTRL_PWR_UP == power_up)
		v |= (1 << dev);
	else
		v &= ~(1 << dev);

	sys_ll_set_cpu_device_clk_enable_value(v);
}

uint32_t sys_hal_uart_select_clock_get(uart_id_t id)
{
	return UART_SCLK_XTAL_26M;
}

/* UART select clock **/
void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode)
{
	switch(id)
	{
		case UART_ID_0:
			{
				if(mode == UART_SCLK_APLL)
					;//sys_ll_set_cpu_clk_div_mode1_clksel_uart0(sel_appl);
				else
					;//sys_ll_set_cpu_clk_div_mode1_clksel_uart0(sel_xtal);
				break;
			}
		case UART_ID_1:
			{
				if(mode == UART_SCLK_APLL)
					;//sys_ll_set_cpu_clk_div_mode1_cksel_uart1(sel_appl);
				else
					;//sys_ll_set_cpu_clk_div_mode1_cksel_uart1(sel_xtal);
				break;
			}
		case UART_ID_2:
			{
				if(mode == UART_SCLK_APLL)
					;//sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_appl);
				else
					;//sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_xtal);
				break;
			}
#if 0//(SOC_UART_ID_NUM_PER_UNIT  >= 4) //TODO
		case UART_ID_3:
			{
				if(mode == UART_SCLK_APLL)
					;//sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_appl);
				else
					;//sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_xtal);
				break;
			}
#endif
		default:
			break;
	}

}

void sys_hal_pwm_select_clock(sys_sel_pwm_t num, pwm_src_clk_t mode)
{
	uint32_t sel_clk32 = 0;
	uint32_t sel_xtal = 1;

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
				sys_ll_set_cpu_clk_div_mode1_cksel_pwm0(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode1_cksel_pwm0(sel_clk32);
			break;

		default:
			break;
	}
}

void sys_hal_timer_select_clock(sys_sel_timer_t num, timer_src_clk_t mode)
{
	uint32_t sel_clk32 = 0;
	uint32_t sel_xtal = 1;

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

	ret = (ret) ? TIMER_SCLK_XTAL : TIMER_SCLK_CLK32;

	return ret;
}

void sys_hal_spi_select_clock(spi_id_t num, spi_src_clk_t mode)
{
	uint32_t sel_xtal = 0;
	uint32_t sel_60m = 1;
	uint32_t sel_80m = 3;

	switch(num)
	{
		case SPI_ID_0:
			if (mode == SPI_CLK_XTAL) {
				sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi0(sel_xtal);
			} else if (mode == SPI_CLK_APLL) {
				sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi0(sel_60m);
			} else {
				sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi0(sel_80m);
			}
			break;
#if (SOC_SPI_UNIT_NUM > 1)
		case SPI_ID_1:
			if (mode == SPI_CLK_XTAL) {
				sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi1(sel_xtal);
			} else if (mode == SPI_CLK_APLL) {
				sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi1(sel_60m);
			} else {
				sys_ll_set_cpu_26m_wdt_clk_div_clksel_spi1(sel_80m);
			}
			break;
#endif
		default:
			break;
	}
}

void sys_hal_cali_dpll_spi_detect_disable(void)
{
    sys_ll_set_ana_reg1_osccal_trig(0);
}

void sys_hal_cali_dpll_spi_detect_enable(void)
{
    sys_ll_set_ana_reg1_osccal_trig(1);
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

void sys_hal_analog_set(analog_reg_t reg, uint32_t value)
{
    uint32_t analog_reg_address;

    if ((reg < ANALOG_REG0) || (reg >= ANALOG_MAX)) {
        return;
    }

    analog_reg_address = SYS_ANA_REG0_ADDR + (reg - ANALOG_REG0) * 4;

	sys_ll_set_analog_reg_value(analog_reg_address, value);
}

/**  Platform End **/

uint32_t sys_hal_cali_dpll(uint32_t first_time)
{
    //liupeng20250519: disable unlock detecter and delay 20us when cali dpll
    sys_ll_set_ana_reg0_cben(1);
    sys_ll_set_ana_reg1_manual(0);
    sys_hal_delay(340);

    sys_hal_cali_dpll_spi_detect_disable();

    if (first_time)
    {
        sys_hal_delay(3400);
    }
    else
    {
        sys_hal_delay(340);
    }

	sys_hal_cali_dpll_spi_detect_enable();

    if (first_time)
    {
        sys_hal_delay(3400);
    }
    else
    {
        sys_hal_delay(340);
    }

    //ronghui20250604: toggle twice to avoid wrong value after 0x40<29:28>=1
    sys_hal_cali_dpll_spi_detect_disable();

    if (first_time)
    {
        sys_hal_delay(3400);
    }
    else
    {
        sys_hal_delay(340);
    }

	sys_hal_cali_dpll_spi_detect_enable();

    if (first_time)
    {
        sys_hal_delay(3400);
    }
    else
    {
        sys_hal_delay(340);
    }

    sys_ll_set_ana_reg1_bandmanual(aon_pmu_hal_get_dpll_band());
    sys_ll_set_ana_reg1_manual(1);
    sys_ll_set_ana_reg0_cben(0);

    return BK_OK;
}

static void sys_hal_dpll_cpu_flash_time_early_init(uint32_t chip_id)
{
	uint32_t coresel = 0;
	uint32_t corediv = 0;
	//zhangheng20231018: calibrate dpll before use
	/*0x2:320M, 0X3:480M*/
	coresel = sys_ll_get_cpu_clk_div_mode1_cksel_core();
	corediv = sys_ll_get_cpu_clk_div_mode1_clkdiv_core();
	if((coresel != CLKSEL_CORE_480M) || !sys_ll_get_ana_reg1_manual())
	{
		sys_hal_cali_dpll(1);
	}

	/*default of MP flash 80M = 80/1*/
	if(sys_ll_get_cpu_clk_div_mode2_cksel_flash() != CLKSEL_FLASH_80M)
	{
		if(sys_ll_get_cpu_clk_div_mode2_ckdiv_flash() != 0x0)// 80/1
		{
			sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0x0);
		}
		sys_hal_flash_set_clk(CLKSEL_FLASH_80M);
	}

	/* clk_divd 120MHz,
	 * 1, the core clock is depended on CONFIG_CPU_FREQ_HZ and configSYSTICK_CLOCK_HZ.
	 *    Pay attention to bk_pm_module_vote_cpu_freq,and the routine will switch core
	 *    clock;
	 * 2, sysTick module's clock source is processor clock now;
	 */
	if((coresel != CLKSEL_CORE_480M) && (corediv != 0x3))//160M
	{
		sys_hal_mclk_div_set(480000000/CONFIG_CPU_FREQ_HZ - 1);
		sys_hal_delay(10000);

		sys_hal_mclk_mux_set(CLKSEL_CORE_480M);/*clock source: DPLL, 480M*/
	}
}

static void sys_hal_dpll_cpu_flash_time_deinit()
{
	//set cpu clk and flash clk to default value 0
	sys_hal_mclk_mux_set(0);
	sys_hal_mclk_div_set(0);
	sys_hal_flash_set_clk(0);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0);

}

//TODO the module owner can put the sys init to better place
void sys_hal_early_init(void)
{
	uint32_t chip_id = aon_pmu_hal_get_chipid();

	uint32_t val = sys_hal_analog_get(ANALOG_REG5);
	// power up dpll first
	val |= (0x1 << 5);
	sys_hal_analog_set(ANALOG_REG5,val);

	sys_hal_analog_set(ANALOG_REG0, 0x19219780);//liupeng202400807:<9:7>=7;zhiyin20250429<11>=0 enable dpll_unlock flag;zhiyin20250604<29:28>=1
	sys_hal_analog_set(ANALOG_REG1, 0xC02EFA00);
	sys_hal_analog_set(ANALOG_REG2, 0x0600D436);
	sys_hal_analog_set(ANALOG_REG3, 0xF5F00BC8);//tenglong20240806:high psrr as default, low when sleep
	sys_hal_analog_set(ANALOG_REG4, 0x40000000);
	sys_hal_analog_set(ANALOG_REG5, 0x84072160);
	sys_hal_analog_set(ANALOG_REG6, 0x80088100);//liupeng20250427
	sys_hal_analog_set(ANALOG_REG7, 0x57E69706);//tenglong20250430:ldo current limit for EVM;//shuguang20250512:vio=3.3V
	sys_hal_analog_set(ANALOG_REG8, 0xF97C72E4);//tenglong20250519:<31:30>=3(0.9V), same with BK7236_SYS_ANA_4D<13:12>

	/**
	 * attention:
	 * SPI latch must be enable before ana_reg[8~13] modification
	 * and don't forget disable it after that.
	 */
	sys_ll_set_ana_reg8_spi_latch1v(1);
	sys_hal_analog_set(ANALOG_REG9, 0xB7553327);//tenglong20240805:2M buck for EVM
	sys_hal_analog_set(ANALOG_REG10, 0xF7EEB89A);
	sys_hal_analog_set(ANALOG_REG11, 0x00000000);
	sys_hal_analog_set(ANALOG_REG12, 0x00100000);
	sys_hal_analog_set(ANALOG_REG13, 0x00080000);
	sys_hal_analog_set(ANALOG_REG14, 0x00A80008);
	sys_hal_analog_set(ANALOG_REG15, 0x528809F6);
	sys_ll_set_ana_reg8_spi_latch1v(0);

	/*early init cpu flash time*/
	sys_hal_dpll_cpu_flash_time_early_init(chip_id);

	//TODO: should be done with wifi_init->bk_pm_clock_ctrl
	sys_hal_clk_pwr_ctrl(CLK_PWR_ID_RF, CLK_PWR_CTRL_PWR_UP);
}

void sys_hal_early_deinit(void)
{
	sys_hal_dpll_cpu_flash_time_deinit();

	sys_hal_analog_set(ANALOG_REG0, 0x9219D00);
	sys_hal_analog_set(ANALOG_REG1, 0xC02E3A00);
	sys_hal_analog_set(ANALOG_REG2, 0x7E003430);
	sys_hal_analog_set(ANALOG_REG3, 0xC4500A88);
	sys_hal_analog_set(ANALOG_REG4, 0x1A7F0);
	sys_hal_analog_set(ANALOG_REG5, 0x8407A340);
	sys_hal_analog_set(ANALOG_REG6, 0x80088200);
	sys_hal_analog_set(ANALOG_REG7, 0x57E62726);
	sys_hal_analog_set(ANALOG_REG8, 0xF87C70E4);

	sys_ll_set_ana_reg8_spi_latch1v(1);
	sys_hal_analog_set(ANALOG_REG9, 0x8255C3A7);//tenglong20240805:2M buck for EVM
	sys_hal_analog_set(ANALOG_REG10, 0xF47AB0FA);
	sys_hal_analog_set(ANALOG_REG11, 0x00000000);
	sys_hal_analog_set(ANALOG_REG12, 0x00100000);
	sys_hal_analog_set(ANALOG_REG13, 0x00080000);
	sys_hal_analog_set(ANALOG_REG14, 0x00A80008);
	sys_hal_analog_set(ANALOG_REG15, 0x528809F6);
	sys_ll_set_ana_reg8_spi_latch1v(0);
}
/**  SCR End  */
