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

#include <common/bk_include.h>
#include "sys_hal.h"
#include "sys_ll.h"
#include "sys_ana_ll.h"
#include "aon_pmu_hal.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <driver/hal/hal_spi_types.h>
#include "bk_arch.h"
#include "hal_port.h"
#include <os/os.h>
#include "sys_pm_hal.h"
#include "sys_pm_hal_ctrl.h"
#include "modules/pm.h"
#include <driver/pwr_clk.h>
#include "driver/flash.h"
#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include <bk_wdt.h>
#endif
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#include "FreeRTOS.h"
#include "armstar.h"
#if CONFIG_DEEP_LV
#include "deep_lv.h"
#endif
#if CONFIG_MPU
#include "mpu.h"
#endif
#include "sys_driver.h"

#if CONFIG_TEMP_DETECT
#include "components/sensor.h"
#endif
#include <os/mem.h>
#include "sys_persist_config.h"

#if CONFIG_DEEP_LV_FAST_BOOT
extern uint32_t __MSPTop;
void deep_lv_fast_boot(void);

#define BK7236N_DEEP_LV_FAST_BOOT_MSP_ADDR   (0x2807FFF8u)
#define BK7236N_DEEP_LV_FAST_BOOT_ENTRY_ADDR (0x2807FFFCu)

void sys_hal_deep_lv_fast_boot_vector_init(void)
{
	volatile uint32_t *const fast_boot_msp = (volatile uint32_t *)BK7236N_DEEP_LV_FAST_BOOT_MSP_ADDR;
	volatile uint32_t *const fast_boot_entry = (volatile uint32_t *)BK7236N_DEEP_LV_FAST_BOOT_ENTRY_ADDR;

	*fast_boot_msp = (uint32_t)&__MSPTop;
	*fast_boot_entry = (uint32_t)&deep_lv_fast_boot;
}
#endif

#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )

#define portNVIC_INT_CTRL_REG                 ( *( ( volatile uint32_t * ) 0xe000ed04 ) )

#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )

#define portNVIC_PENDSVSET_BIT                ( 1UL << 28UL )
#define portNVIC_PENDSVCLR_BIT                ( 1UL << 27UL )
#define portNVIC_SYSTICKSET_BIT               ( 1UL << 26UL )
#define portNVIC_SYSTICKCLR_BIT               ( 1UL << 25UL )

#define PM_EXIT_LOWVOL_SYSTICK_TIME           (32)      //1ms
#define PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME    (0xFFFFFF)//set max
#if CONFIG_SOFT_CTRL_VOLT
#define PM_LOW_VOL_VCOREHSEL_LDO_SEL          (6)       // 0.75V
#endif

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
#define FLASH_BASE_ADDRESS                    (0x44030000)
#define FLASH_OFFSET_ADDR_BEGIN               (0x16)
#define FLASH_OFFSET_ADDR_END                 (0x17)
#define FLASH_ADDR_OFFSET                     (0x18)
#define FLASH_OFFSET_ENABLE                   (0x19)

typedef struct
{
	uint32_t flash_offset_enable_val;
	uint32_t offset_addr_begin_val;
	uint32_t offset_addr_end_val ;
	uint32_t flash_addr_offset_val;
}flash_ab_reg_t;

#endif

uint64_t low_voltage_exit_tick = 0;
uint64_t low_voltage_sleep_duration_us = 0;
uint64_t low_voltage_wakeup_time_us = 0;
uint32_t bootup_restore_time_us = 0;
sys_persist_pm_t *persist_pm;
sys_persist_pm_t s_pm_config;
extern void bk_delay_us(UINT32 us);

#define TIMER0_REG_SET(reg_id, l, h, v) REG_SET((SOC_TIMER0_REG_BASE + ((reg_id) << 2)), (l), (h), (v))
#define TIMER0_PERIOD 0xFFFFFFFF

static uint32_t timer_hal_get_timer0_cnt(void)
{
	TIMER0_REG_SET(8, 2, 3, 0);
	TIMER0_REG_SET(8, 0, 0, 1);
	while (REG_READ((SOC_TIMER0_REG_BASE + (8 << 2))) & BIT(0));

	return REG_READ(SOC_TIMER0_REG_BASE + (9 << 2));
}

static uint32_t bootloader_to_wfi_time_cost_us(void)
{
	uint32_t tick_cnt = timer_hal_get_timer0_cnt();
	uint32_t time_cost_us = tick_cnt * 1000 / TIMER_CLOCK_FREQ_XTAL;
	return time_cost_us;
}

static inline bool is_lpo_src_26m32k(void)
{
	return (aon_pmu_ll_get_r41_lpo_config() == SYS_LPO_SRC_26M32K);
}

static inline bool is_lpo_src_ext32k(void)
{
	return (aon_pmu_ll_get_r41_lpo_config() == SYS_LPO_SRC_EXTERNAL_32K);
}

static inline bool is_wifi_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_WIFI);
}

static inline bool is_gpio_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_GPIO);
}

static inline bool is_rtc_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_RTC);
}

static inline bool is_bt_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_BT);
}

static inline bool is_usbplug_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_USBPLUG);
}

static inline bool is_touch_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_TOUCH);
}

static inline bool is_wifi_pd_poweron(uint32_t pd_bits)
{
	return !(pd_bits & PD_WIFI);
}

static inline bool is_btsp_pd_poweron(uint32_t pd_bits)
{
	return !(pd_bits & PD_BTSP);
}

static inline void sys_hal_restore_int(volatile uint32_t int_state1, volatile uint32_t int_state2)
{
	sys_ll_set_cpu0_int_0_31_en_value(int_state1);
	sys_ll_set_cpu0_int_32_63_en_value(int_state2);
}

static inline void sys_hal_enable_wakeup_int(void)
{
}

static inline void sys_hal_set_core_freq(volatile uint8_t cksel_core, volatile uint8_t clkdiv_core, volatile uint8_t clkdiv_bus)
{
    sys_ll_set_cpu_clk_div_mode1_cksel_core(cksel_core);
    sys_ll_set_cpu_clk_div_mode1_clkdiv_core(clkdiv_core);
    ;//sys_ll_set_cpu_clk_div_mode1_clkdiv_bus(clkdiv_bus);
}

static inline void sys_hal_set_core_26m(void)
{
	sys_hal_set_core_freq(0, 0, 0);
}

static inline void sys_hal_backup_set_core_26m(volatile uint8_t *cksel_core, volatile uint8_t *clkdiv_core, volatile uint8_t *clkdiv_bus)
{
	IF_LV_CTRL_CORE() {
		*cksel_core = sys_ll_get_cpu_clk_div_mode1_cksel_core();
		*clkdiv_core = sys_ll_get_cpu_clk_div_mode1_clkdiv_core();
		;//*clkdiv_bus = sys_ll_get_cpu_clk_div_mode1_clkdiv_bus();
		sys_hal_set_core_freq(0, 0, 0);
	}
}

static inline void sys_hal_restore_core_freq(volatile uint8_t cksel_core, volatile uint8_t clkdiv_core, volatile uint8_t clkdiv_bus)
{
	IF_LV_CTRL_CORE() {
		;//sys_ll_set_cpu_clk_div_mode1_clkdiv_bus(clkdiv_bus);
		sys_ll_set_cpu_clk_div_mode1_clkdiv_core(clkdiv_core);
		sys_ll_set_cpu_clk_div_mode1_cksel_core(cksel_core);
	}
}

static inline void sys_hal_set_flash_freq(volatile uint8_t cksel_flash, volatile uint8_t ckdiv_flash)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(cksel_flash);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(ckdiv_flash);
}

static inline void sys_hal_set_flash_26m(void)
{
	sys_hal_set_flash_freq(0, 0);
}

static inline void sys_hal_backup_set_flash_26m(volatile uint8_t *cksel_flash, volatile  uint8_t *ckdiv_flash)
{
	IF_LV_CTRL_FLASH() {
		*cksel_flash = sys_ll_get_cpu_clk_div_mode2_cksel_flash();
		*ckdiv_flash = sys_ll_get_cpu_clk_div_mode2_ckdiv_flash();
		sys_ll_set_cpu_clk_div_mode2_cksel_flash(0);//eg:from the 80m to 26m, it need select clk source first
		sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0);//then ckdiv
	}
}

static inline void sys_hal_set_flash_120m(void)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(2);
}

static inline void sys_hal_set_anaspi_freq(uint8_t anaspi_freq)
{
	sys_ll_set_cpu_anaspi_freq_value(anaspi_freq);
}

static inline void sys_hal_backup_set_anaspi_freq_26m(uint8_t *anaspi_freq)
{
	*anaspi_freq = sys_ll_get_cpu_anaspi_freq_value();
	sys_hal_set_anaspi_freq(1); //26M/4
}

static inline void sys_hal_restore_anaspi_freq(uint8_t anaspi_freq)
{
	sys_hal_set_anaspi_freq(anaspi_freq);
}

static inline void sys_hal_restore_flash_freq(volatile uint8_t cksel_flash, volatile uint8_t ckdiv_flash)
{
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(ckdiv_flash);//eg:from the 26m to 80m, it need config clk div first
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(cksel_flash);//then clk source
}

static inline void sys_hal_mask_cpu0_int(void)
{
	sys_ll_set_cpu0_int_halt_clk_op_cpu0_int_mask(1);
}

static inline void sys_hal_enable_spi_latch(void)
{
	 sys_ll_set_ana_reg8_spi_latch1v_iram(1);
}

static inline void sys_hal_disable_spi_latch(void)
{
	 sys_ll_set_ana_reg8_spi_latch1v_iram(0);
}

static inline uint32_t sys_hal_disable_hf_clock(void)
{
	uint32_t val = sys_ll_get_ana_reg5_value();
	uint32_t ret_val = val;

	val &= ~EN_ALL;

	if (is_lpo_src_ext32k()) {
	val |= EN_XTAL;
	}

	sys_ll_set_ana_reg5_value(val);

	return ret_val;

}

static inline void sys_hal_restore_hf_clock(volatile uint32_t val)
{
	sys_ll_set_ana_reg5_value(val);
}

/**
 * buck power supply switch
 *
 * uint32_t type input:
 *   0: close buck
 *   other: open buck
 *
 * note: please declare this function as static when buck is stable
*/
void sys_hal_buck_switch(uint32_t flag)
{
	volatile uint8_t cksel_core = 0, clkdiv_core = 0, clkdiv_bus = 0;

	if (flag == 0) {
		os_printf("disable buckA.\r\n");
		sys_hal_enable_spi_latch();
		sys_ll_set_ana_reg10_aldosel(1);
		sys_hal_disable_spi_latch();
	} else if (flag == 1){
		os_printf("enable buckA.\r\n");
		//let the cpu frequency to 26m, in order to be successfully switch voltage provide from ldo to buck
		sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);

		sys_hal_enable_spi_latch();
		sys_ll_set_ana_reg10_aldosel(0);
		sys_hal_disable_spi_latch();

		sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);
	} else {
		os_printf("set buck power supply param %d must < 2 \r\n",flag);
	}
}


/**
 * high digital voltage
 *
 * uint32_t value input:
 *   voltage value
 *
 *
 * note: please declare this function
*/
void sys_hal_v_core_h_sel(uint32_t value)
{
	sys_hal_enable_spi_latch();
	if(sys_ll_get_ana_reg8_vcorehsel() != value)
	{
		sys_ll_set_ana_reg8_vcorehsel(value);
	}
	sys_hal_disable_spi_latch();
}

static inline void sys_hal_deep_sleep_set_buck(void)
{
	//TODO: fix me when bring up pm
	// sys_ll_set_ana_reg11_aldosel(1);
	// sys_ll_set_ana_reg12_dldosel(1);
}

static inline void sys_hal_deep_sleep_set_vldo(void)
{
	// sys_ll_set_ana_reg8_coreldo_hp(0);
	/*dldohp disabling causes OTP read failed!, so it can't set 0*/
	sys_ll_set_ana_reg7_dldohp(1);

	sys_ll_set_ana_reg7_aldohp(0); //20230423 tenglong
}

static inline void sys_hal_clear_wakeup_source(void)
{
	// aon_pmu_ll_set_r43_clr_wakeup(1);
	// aon_pmu_ll_set_r43_clr_wakeup(0);
}

static inline void sys_hal_set_halt_config(void)
{
	uint32_t v = aon_pmu_ll_get_r41();

	//halt_lpo = 1
	//halt_busrst = 0
	//halt_busiso = 1, halt_buspwd = 1
	//halt_blpiso = 1, halt_blppwd = 1
	//halt_wlpiso = 1, halt_wlppwd = 1
	v |= (0xFD << 24);
	aon_pmu_ll_set_r41(v);
}
static inline void sys_hal_power_on_and_select_rosc(pm_lpo_src_e lpo_src)
{
	volatile uint32_t count = PM_POWER_ON_ROSC_STABILITY_TIME;
	if((lpo_src == PM_LPO_SRC_ROSC)||(lpo_src == PM_LPO_SRC_DIVD))
	{
		if(sys_ll_get_ana_reg5_pwd_rosc_spi() != 0x0)
		{
			sys_ll_set_ana_reg5_pwd_rosc_spi(0x0);//power on rosc
			while(count--)//delay time for stability when power on rosc
			{
			}
		}

		if(aon_pmu_ll_get_r41_lpo_config() != PM_LPO_SRC_ROSC)
		{
			aon_pmu_ll_set_r41_lpo_config(PM_LPO_SRC_ROSC);
		}
	}
}
static inline void sys_hal_set_power_parameter(uint8_t sleep_mode)
{
	/*  r40[3:0]   wake1_delay = 0x1;
 	 *  r40[7:4]   wake2_delay = 0x1;
 	 *  r40[11:8]  wake3_delay = 0x1;
 	 *  r40[15:12] halt1_delay = 0x1;
 	 *  r40[19:16] halt2_delay = 0x1;
 	 *  r40[23:20] halt3_delay = 0x1;
 	 *  r40[24] halt_volt: deep = 0, lv = 1
 	 *  r40[25] halt_xtal = 1 //26M32K unable to wakeup system, disable during lv sleep
 	 *  r40[26] halt_core: deep = 1, lv = 0
 	 *  r40[27] halt_flash = 1
 	 *  r40[28] halt_rosc = 0
 	 *  r40[29] halt_resten: deep = 0, lv = 1
 	 *  r40[30] halt_isolat = 1
 	 *  r40[31] halt_clkena = 0
 	 **/
	if (sleep_mode == PM_MODE_DEEP_SLEEP)
	{
#if CONFIG_SPE
		aon_pmu_ll_set_r40(0x5E111111);//using the external 32k , it need more wake delay time
#else
		// OTP need more delay to recovery voltage
		aon_pmu_ll_set_r40(0x5E111111);
#endif
	}
	else
	{
		uint32_t val;
		#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
		val = (0x63111000
			|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
			|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
			|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
		#else
		val = (0x6B111000
			|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
			|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
			|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
		#endif

		aon_pmu_ll_set_r40(val);
	}
}

static inline void sys_hal_set_sleep_condition(void)
{
	uint32_t v = sys_ll_get_cpu_power_sleep_wakeup_value();

	//sleep_bus_idle_bypass =  0
	//sleep_en_global = 1
	//sleep_en_need_cpu0_wfi = 0
	//sleep_en_need_flash_idle = 0
	v &= (~0xF0000);
	v |= (0x8 << 16);

	sys_ll_set_cpu_power_sleep_wakeup_value(v);
}

static inline void sys_hal_power_down_pd(volatile uint32_t *pd_reg_v)
{
	uint32_t value = 0;
	IF_LV_CTRL_PD() {
		uint32_t v = sys_ll_get_cpu_power_sleep_wakeup_value();
		*pd_reg_v = v;

		/*config power domain*/
		v |= PD_DOWN_DOMAIN;
#if 0
		/*config r41 bus wl,bl pw and iso*/
		sys_hal_set_halt_config();

		if (is_wifi_pd_poweron(v)) {
			aon_pmu_ll_set_r41_halt_wlpiso(1);
			aon_pmu_ll_set_r41_halt_wlppwd(0);
		}
		else {
			aon_pmu_ll_set_r41_halt_wlpiso(1);
			aon_pmu_ll_set_r41_halt_wlppwd(1);
		}

		if (is_btsp_pd_poweron(v)) {
			aon_pmu_ll_set_r41_halt_blpiso(1);
			aon_pmu_ll_set_r41_halt_blppwd(0);
		}
		else {
			aon_pmu_ll_set_r41_halt_blpiso(1);
			aon_pmu_ll_set_r41_halt_blppwd(1);
		}
#endif
		sys_ll_set_cpu_power_sleep_wakeup_value(v);

		/*config cpu0 subpwdm*/
		sys_ll_set_cpu_power_sleep_wakeup_cpu0_subpwdm_en(1);
		value = sys_ll_get_cpu0_lv_sleep_cfg_value();
		/*bit2(1: fpu powerdown  when lv sleep & cpu0_subpwdm_en==1'b1)*/
		/*bit0(1: cache retension when lv sleep & cpu0_subpwdm_en==1'b1)*/
		// value |= (0x1 << 0);
		sys_ll_set_cpu0_lv_sleep_cfg_value(value);

#if CONFIG_DEEP_LV
		/* cpu is off during lv-sleep */
		aon_pmu_hal_set_halt_cpu(1);
		/* active when cpu off*/
		aon_pmu_ll_set_r41_halt_anareg(0);//must be 0, if set to 1 will reset ,analog bug
		/* PMU_REG0x40: BIT[29]=0, BIT[30]=1 */
		aon_pmu_ll_set_r40_halt_resten(0);
		aon_pmu_ll_set_r40_halt_isolat(1);
#endif
	}
}

static inline void sys_hal_power_on_pd(volatile uint32_t v_sys_r10)
{
	IF_LV_CTRL_PD() {
		sys_ll_set_cpu_power_sleep_wakeup_value(v_sys_r10);
	}
}

#if !CONFIG_AON_PMU_REG0_REFACTOR_DEV
void sys_hal_gpio_state_switch(bool lock)
{
	/*pass aon_pmu_r0 to ana*/
	if (lock) {
		aon_pmu_ll_set_r0_gpio_sleep(1);
	} else {
		aon_pmu_ll_set_r0_gpio_sleep(0);
	}
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}
#endif

__attribute__((section(".itcm_sec_code"))) void sys_hal_enter_deep_sleep(void *param)
{
	volatile uint32_t int_state1, int_state2;
	uint32_t systick_ctrl_value = 0;
	pm_lpo_src_e lpo_src        = PM_LPO_SRC_ROSC;
	bk_err_t sleep_allowd       = BK_OK;
	uint64_t deep_ps_expired_tick = 0;

	portNVIC_INT_CTRL_REG |= portNVIC_SYSTICKCLR_BIT;
	systick_ctrl_value = portNVIC_SYSTICK_CTRL_REG;
	portNVIC_SYSTICK_CTRL_REG = 0;//disable the systick, avoid it affect the enter deepsleep

	int_state1 = sys_ll_get_cpu0_int_0_31_en_value();
	int_state2 = sys_ll_get_cpu0_int_32_63_en_value();
	sys_ll_set_cpu0_int_0_31_en_value(0x0);
	sys_ll_set_cpu0_int_32_63_en_value(0x0);
	extern bk_err_t ana_rtc_deep_ps_wakeup_check(uint64_t *p_expired_tick);
	extern bk_err_t ana_rtc_deep_ps_wakeup_apply(uint64_t expired_tick);
	sleep_allowd = ana_rtc_deep_ps_wakeup_check(&deep_ps_expired_tick);
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );

	/*confirm here hasn't external interrupt*/
	if ((check_IRQ_pending())
	 || (portNVIC_INT_CTRL_REG&portNVIC_SYSTICKSET_BIT)
	 || (sleep_allowd != BK_OK))
	{
		sys_ll_set_cpu0_int_0_31_en_value(int_state1);
		sys_ll_set_cpu0_int_32_63_en_value(int_state2);
		portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
		return;
	}

	/* If "deep_ps" alarm exists, *p_expired_tick is set and apply reprograms RTC compare.
	 * If not, *p_expired_tick stays 0: keep existing HW RTC/timer compare registers (legacy path). */
	if (deep_ps_expired_tick != 0)
	{
		sleep_allowd = ana_rtc_deep_ps_wakeup_apply(deep_ps_expired_tick);
		if (sleep_allowd != BK_OK)
		{
			sys_ll_set_cpu0_int_0_31_en_value(int_state1);
			sys_ll_set_cpu0_int_32_63_en_value(int_state2);
			portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
			return;
		}
	}

	sys_hal_mask_cpu0_int();

/*----------config wakeup source  start--------------*/
	sys_drv_wakeup_source_gpio_clear();
	sys_drv_wakeup_source_rtc_clear();
#if CONFIG_GPIO_WAKEUP_SUPPORT
	extern bk_err_t gpio_enable_interrupt_mult_for_wake(void);
	gpio_enable_interrupt_mult_for_wake();
#endif
	sys_hal_enable_ana_rtc_int();
	sys_hal_enable_ana_gpio_int();
/*-----------config wakeup source  end --------------*/

	sys_hal_set_core_26m();
	sys_hal_set_flash_26m();

#if CONFIG_INT_WDT
	bk_wdt_stop();
	#if CONFIG_TASK_WDT
	bk_task_wdt_stop();
	#endif
#endif

#if CONFIG_AON_PMU_POR_TIMING_SUPPORT
	// disable it or it will cause error
	sys_ll_set_ana_reg7_vporsel(0);
	sys_ana_ll_set_ana_reg10_vbg_rstrtc_en(1);
	// aon_pmu_ll_set_r0_saved_time(0);
#endif

	lpo_src = aon_pmu_ll_get_r41_lpo_config();
	/*set enter deep sleep mode flag*/
	bk_misc_set_reset_reason(RESET_SOURCE_SUPER_DEEP);

	sys_hal_set_sleep_condition();

	sys_hal_set_power_parameter(PM_MODE_DEEP_SLEEP);

	sys_hal_enable_spi_latch();

	sys_hal_disable_hf_clock();
	sys_ll_set_ana_reg5_en_cb(0);

	if((lpo_src == PM_LPO_SRC_ROSC)||(lpo_src == PM_LPO_SRC_DIVD))
	{
		sys_hal_power_on_and_select_rosc(lpo_src);
		// levelshift latch for rc32k (keep current freq)
		sys_ll_set_ana_reg5_spilatchb_rc32k(0);
		// spi_byp32pwd must be set to 1
		sys_ll_set_ana_reg9_spi_byp32pwd(1);
	}

	sys_hal_disable_spi_latch();

#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
	aon_pmu_hal_set_gpio_sleep(1, false);
#else
	sys_hal_gpio_state_switch(true);
#endif

	sys_ll_set_cpu_power_sleep_wakeup_cpu0_subpwdm_en(1);

	aon_pmu_ll_set_r0_fast_boot(1); // set fast boot flag to 1
	
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
	aon_pmu_hal_r0_latch_to_r7b();
#endif

	/*-----enter deep sleep-------*/
	arch_deep_sleep();
}

static inline void sys_hal_set_low_voltage(volatile uint32_t *ana_r0, volatile uint32_t *ana_r3, volatile uint32_t *ana_r7, volatile uint32_t *ana_r8, volatile uint32_t *ana_r9, volatile uint32_t *ana_r10)
{
	sys_hal_enable_spi_latch();

	*ana_r0 = sys_ll_get_ana_reg0_value();
	*ana_r3 = sys_ll_get_ana_reg3_value();
	*ana_r7 = sys_ll_get_ana_reg7_value();
	// *ana_r8 = sys_ll_get_ana_reg8_value();
	*ana_r9 = sys_ll_get_ana_reg9_value();
	*ana_r10 = sys_ll_get_ana_reg10_value();
#if !CONFIG_DEEP_LV
	sys_ll_set_ana_reg3_en_xtalh_sleep(1);
#endif
	sys_ll_set_ana_reg7_vporsel(1);
	sys_ll_set_ana_reg7_vbspbuf_lp(1);//bit4
	sys_ll_set_ana_reg7_envrefh1v(0);//bit12
	sys_ll_set_ana_reg7_vanaldosel((s_pm_config.lvsleep_vanaldosel == 0xff) ? 2 : s_pm_config.lvsleep_vanaldosel); // 0: 0.9V, 2: 1V
#if CONFIG_TEMP_DETECT
	float temp;
	bk_err_t err = bk_sensor_get_current_temperature(&temp);

	if((err != BK_OK) || (temp < 0)) // get temperature failed or temperature is less than 0
	{
		#if CONFIG_LDO_SELF_LOW_POWER_MODE_ENA
		sys_ll_set_ana_reg7_dldohp((s_pm_config.lvsleep_dldohp_low_temp == 0xff) ? 1 : s_pm_config.lvsleep_dldohp_low_temp); //will increase current 15uA
		sys_ll_set_ana_reg7_aldohp((s_pm_config.lvsleep_aldohp_low_temp == 0xff) ? 0 : s_pm_config.lvsleep_aldohp_low_temp);
		#endif
	}
	else
	{
		#if CONFIG_LDO_SELF_LOW_POWER_MODE_ENA
		sys_ll_set_ana_reg7_dldohp((s_pm_config.lvsleep_dldohp_high_temp == 0xff) ? 0 : s_pm_config.lvsleep_dldohp_high_temp);
		sys_ll_set_ana_reg7_aldohp((s_pm_config.lvsleep_aldohp_high_temp == 0xff) ? 0 : s_pm_config.lvsleep_aldohp_high_temp);
		#endif
	}
#else
	#if CONFIG_LDO_SELF_LOW_POWER_MODE_ENA
	sys_ll_set_ana_reg7_dldohp((s_pm_config.lvsleep_dldohp_high_temp == 0xff) ? 0 : s_pm_config.lvsleep_dldohp_high_temp);
	sys_ll_set_ana_reg7_aldohp((s_pm_config.lvsleep_aldohp_high_temp == 0xff) ? 0 : s_pm_config.lvsleep_aldohp_high_temp);
	#endif
#endif

	sys_ll_set_ana_reg7_bypassen((s_pm_config.lvsleep_bypassen == 0xff) ? 1 : s_pm_config.lvsleep_bypassen);//bit20
	sys_ll_set_ana_reg7_ioldolp((s_pm_config.lvsleep_ioldolp == 0xff) ? 1 : s_pm_config.lvsleep_ioldolp);//bit21

	// sys_ll_set_ana_reg9_azcd_manu(0x20);
	// sys_ll_set_ana_reg9_enzcdcalib(0);
	// sys_ll_set_ana_reg9_zcdmsel(1);

	sys_ll_set_ana_reg9_lburstsel((s_pm_config.lvsleep_iburstsel == 0xff) ? 3 : s_pm_config.lvsleep_iburstsel);//bit[19:18]
	sys_ll_set_ana_reg9_mrosci_cal((s_pm_config.lvsleep_mrosci_cal == 0xff) ? 0 : s_pm_config.lvsleep_mrosci_cal);//bit[27:25]
	sys_ll_set_ana_reg10_avea_sel((s_pm_config.lvsleep_avea_sel == 0xff) ? 3 : s_pm_config.lvsleep_avea_sel);
	// sys_ll_set_ana_reg10_arampc(0);//suggest by long.teng,20251105

	sys_hal_disable_spi_latch();
}

static inline void sys_hal_restore_voltage(volatile uint32_t ana_r8, volatile uint32_t core_low_voltage)
{
	sys_hal_enable_spi_latch();
	sys_ll_set_ana_reg8_value(ana_r8);
	sys_hal_disable_spi_latch();
}

static void sys_hal_delay(volatile uint32_t times)
{
        while(times--);
}

void sys_hal_exit_low_voltage(void)
{
}

uint64_t sys_hal_get_exit_low_voltage_tick(void)
{
	return low_voltage_exit_tick;
}

void sys_hal_set_exit_low_voltage_tick(uint64_t tick)
{
	low_voltage_exit_tick= tick;

}

uint64_t sys_hal_get_low_voltage_sleep_duration_us(void)
{
	return low_voltage_sleep_duration_us;
}

void sys_hal_set_low_voltage_sleep_duration_us(uint64_t sleep_duration)
{
	low_voltage_sleep_duration_us= sleep_duration;
}

inline uint64_t sys_hal_get_low_voltage_wakeup_time_us(void)
{
	return low_voltage_wakeup_time_us;
}

inline void sys_hal_set_low_voltage_wakeup_time_us(uint64_t wakeup_time)
{
	low_voltage_wakeup_time_us = wakeup_time;
}

uint32_t sys_hal_get_bootup_restore_time_us(void)
{
	return bootup_restore_time_us;
}

static inline void sys_hal_lv_set_buck(void)
{
	// fix it when bring up pm
#if 0
	if (sys_ll_get_ana_reg11_aldosel() == 0) {
		sys_ll_set_ana_reg11_aforcepfm(1);//tenglong20230417 modify buck mode(increase buck effect)
		sys_ll_set_ana_reg12_dforcepfm(1);//tenglong20230417
	}
#endif
}

static inline void sys_hal_lv_restore_buck(void)
{
	// fix it when bring up pm
#if 0
	if (sys_ll_get_ana_reg11_aldosel() == 0) {
		sys_ll_set_ana_reg11_aforcepfm(0);//tenglong20230417
		sys_ll_set_ana_reg12_dforcepfm(0);//tenglong20230417
	}
#endif
}

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
//when enter lowvoltage need backup related flash information.
static inline void flash_ab_info_backup(flash_ab_reg_t *p_flash_ab_reg)
{
	if(!p_flash_ab_reg)
	{
		return;
	}

	p_flash_ab_reg->offset_addr_begin_val = REG_READ(FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_BEGIN*4);
	p_flash_ab_reg->offset_addr_end_val = REG_READ(FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_END*4);
	p_flash_ab_reg->flash_addr_offset_val = REG_READ(FLASH_BASE_ADDRESS + FLASH_ADDR_OFFSET*4);
	p_flash_ab_reg->flash_offset_enable_val = (REG_READ(FLASH_BASE_ADDRESS + FLASH_OFFSET_ENABLE*4) & 0x1);
}

//when exit lowvoltage need restore related flash information.
static inline void flash_ab_info_restore(flash_ab_reg_t *p_flash_ab_reg)
{
	if(!p_flash_ab_reg)
	{
		return;
	}

	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_BEGIN*4), p_flash_ab_reg->offset_addr_begin_val);
	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_END*4), p_flash_ab_reg->offset_addr_end_val);
	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_ADDR_OFFSET*4), p_flash_ab_reg->flash_addr_offset_val);
	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_OFFSET_ENABLE*4), p_flash_ab_reg->flash_offset_enable_val);
}
#endif

#if CONFIG_DEEP_LV
static uint32_t s_sys_saved_regs[12] = {0};
static uint32_t s_sys_ana_regs[16] = {0};

static void _deep_lv_enter_(void)
{
	__asm volatile
	(
		" .syntax unified           \n"
		" cpsie i                   \n" /* Globally enable interrupts. */
		" cpsie f                   \n"
		" dsb                       \n"
		" isb                       \n"
		" svc %0                    \n"
		" nop                       \n"
		"                           \n"
		::"i"(portSVC_DEEP_LV_ENTER):"memory"
	);
}

void sys_hal_deep_lv_enter(void)
{
	_deep_lv_enter_();
	__asm volatile
	(
		" .syntax unified           \n"
		" nop                       \n"
	);
}

void sys_hal_regs_save(void)
{
	s_sys_saved_regs[0] = sys_ll_get_cpu_clk_div_mode1_value(); // reg_0x8
	s_sys_saved_regs[1] = sys_ll_get_cpu_clk_div_mode2_value(); // reg_0x9
	s_sys_saved_regs[2] = sys_ll_get_cpu_26m_wdt_clk_div_value(); // reg_0xa
	s_sys_saved_regs[3] = sys_ll_get_cpu_anaspi_freq_value(); // reg_0xb
	s_sys_saved_regs[4] = sys_ll_get_cpu_device_clk_enable_value(); // reg_0xc
	s_sys_saved_regs[5] = sys_ll_get_cpu_device_mem_ctrl2_value(); // reg_0xf
	s_sys_saved_regs[6] = sys_ll_get_cpu_power_sleep_wakeup_value(); // reg_0x10
	s_sys_saved_regs[7] = sys_ll_get_cpu0_lv_sleep_cfg_value(); // reg_0x11
	s_sys_saved_regs[8] = sys_ll_get_cpu_device_mem_ctrl3_value(); // reg_0x12
	s_sys_saved_regs[9] = sys_ll_get_cpu0_int_0_31_en_value(); // reg_0x20
	s_sys_saved_regs[10] = sys_ll_get_cpu0_int_32_63_en_value(); // reg_0x21

	for (uint32_t i = 0; i < 16; i++) {
		s_sys_ana_regs[i] = sys_hal_analog_get_iram(ANALOG_REG0 + i);
		REG_WRITE(0x2807E400 + (i << 2), s_sys_ana_regs[i]);//save analog regs to bootloader stack
	}
	// Used to check if memory (contents) have changed.
	REG_WRITE(0x2807E400 + (16 << 2), 0x5A5A5A5A);
	REG_WRITE(0x2807E400 + (17 << 2), 0xA5A5A5A5);	

}

void sys_hal_regs_restore(void)
{
	s_sys_saved_regs[0] |= (0x1 << 4);
	s_sys_saved_regs[0] |= (0x3 << 0);
	sys_ll_set_cpu_clk_div_mode1_value(s_sys_saved_regs[0]); // reg_0x8
//	sys_ll_set_cpu_clk_div_mode2_value(s_sys_saved_regs[1]); // reg_0x9
	sys_ll_set_cpu_26m_wdt_clk_div_value(s_sys_saved_regs[2]); // reg_0xa
	sys_ll_set_cpu_anaspi_freq_value(s_sys_saved_regs[3]); // reg_0xb
	sys_ll_set_cpu_device_clk_enable_value(s_sys_saved_regs[4]); // reg_0xc
	sys_ll_set_cpu_device_mem_ctrl2_value(s_sys_saved_regs[5]); // reg_0xf
	sys_ll_set_cpu_power_sleep_wakeup_value(s_sys_saved_regs[6]); // reg_0x10
	sys_ll_set_cpu0_lv_sleep_cfg_value(s_sys_saved_regs[7]); // reg_0x11
	sys_ll_set_cpu_device_mem_ctrl3_value(s_sys_saved_regs[8]); // reg_0x12
	sys_ll_set_cpu0_int_0_31_en_value(s_sys_saved_regs[9]); // reg_0x20
	sys_ll_set_cpu0_int_32_63_en_value(s_sys_saved_regs[10]); // reg_0x21

	sys_ll_set_ana_reg8_spi_latch1v(1);
	/* restore analog regs */
	for (uint32_t i = 0; i < 9; i++) {
		if(( i == 0)||( i == 5)||( i == 7)||( i == 8))
			continue;
		sys_hal_analog_set_iram(ANALOG_REG0 + i, s_sys_ana_regs[i]);
	}
	/**
	 * attention:
	 * SPI latch must be enable before ana_reg[8~13] modification
	 * and don't forget disable it after that.
	 */
	for (uint32_t i = 9; i < 16; i++) {
		sys_hal_analog_set_iram(ANALOG_REG0 + i, s_sys_ana_regs[i]);
	}
	sys_ll_set_ana_reg8_spi_latch1v(0);
}
#endif

void sys_hal_enable_ana_rtc_int(void)
{
	sys_hal_int_group2_enable(ANA_RTC_INTERRUPT_CTRL_BIT);

	sys_hal_enable_spi_latch();
	#if CONFIG_ANA_RTC_WAKUP_BY_RTC
	sys_hal_set_ana_reg8_rtcwk_rstn(1);
	#else
	sys_hal_set_ana_reg8_rtcwk_rstn(1);
	sys_hal_set_ana_reg7_timer_wkrstn(1);
	#endif
	sys_hal_disable_spi_latch();
}

void sys_hal_disable_ana_rtc_int(void)
{
	sys_hal_int_group2_disable(ANA_RTC_INTERRUPT_CTRL_BIT);

	sys_hal_enable_spi_latch();
	#if CONFIG_ANA_RTC_WAKUP_BY_RTC
	sys_hal_set_ana_reg8_rtcwk_rstn(0);
	#else
	sys_hal_set_ana_reg7_timer_wkrstn(0);
	sys_hal_set_ana_reg8_rtcwk_rstn(0);
	#endif
	/*clear ana int*/
	sys_hal_set_ana_reg8_rst_timerwks1v(1);
	sys_hal_set_ana_reg8_rst_timerwks1v(0);
	sys_hal_disable_spi_latch();
}

void sys_hal_enable_ana_gpio_int(void)
{
	sys_hal_int_group2_enable(ANA_GPIO_INTERRUPT_CTRL_BIT);

	sys_hal_enable_spi_latch();
	sys_hal_set_ana_reg8_gpiowk_rstn(1);
	sys_hal_disable_spi_latch();
}

void sys_hal_disable_ana_gpio_int(void)
{
	sys_hal_int_group2_disable(ANA_GPIO_INTERRUPT_CTRL_BIT);

	sys_hal_enable_spi_latch();
	sys_hal_set_ana_reg8_gpiowk_rstn(0);
	/*clear ana int, ask long.teng provide a new bit for gpio in next version*/
	sys_hal_set_ana_reg8_rst_gpiowks(1);
	sys_hal_set_ana_reg8_rst_gpiowks(0);
	sys_hal_disable_spi_latch();
}

void sys_drv_wakeup_source_gpio_clear(void)
{
	//reset source flag
	sys_ll_set_ana_reg8_spi_latch1v(1);
	sys_ll_set_ana_reg8_rst_gpiowks(1);
	sys_ll_set_ana_reg8_rst_gpiowks(0);
	sys_ll_set_ana_reg8_spi_latch1v(0);
}

void sys_drv_wakeup_source_rtc_clear(void)
{
	//reset source flag
	sys_ll_set_ana_reg8_spi_latch1v(1);
	sys_ll_set_ana_reg8_rst_timerwks1v(1);
	sys_ll_set_ana_reg8_rst_timerwks1v(0);
	sys_ll_set_ana_reg8_spi_latch1v(0);
}

void sys_hal_enter_low_voltage(void)
{
	//TODO: fix it when bring up pm
#if 1
	volatile uint32_t int_state1, int_state2;
	volatile uint8_t cksel_core = 0, clkdiv_core = 0, clkdiv_bus = 0;
	volatile uint8_t cksel_flash = 0, clkdiv_flash = 0;
	volatile uint32_t v_ana_r0,v_ana_r3, v_ana_r7, v_ana_r8,v_ana_r9, v_ana_r10;
	volatile uint32_t v_sys_r10    = 0;
	volatile uint32_t pmu_val               = 0;
	volatile uint32_t systick_ctrl_value    = 0;
	volatile uint32_t hf_reg_v = 0;
	bk_err_t sleep_allowd       = BK_OK;

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	flash_ab_reg_t  ab_flash_reg   = {0};
#endif
	portNVIC_INT_CTRL_REG |= portNVIC_SYSTICKCLR_BIT;
	systick_ctrl_value = portNVIC_SYSTICK_CTRL_REG;
	portNVIC_SYSTICK_LOAD_REG = PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME;
	portNVIC_SYSTICK_CTRL_REG = 0;//disable the systick, avoid it affect the enter low voltage sleep

	int_state1 = sys_ll_get_cpu0_int_0_31_en_value();
	int_state2 = sys_ll_get_cpu0_int_32_63_en_value();
	sys_ll_set_cpu0_int_0_31_en_value(0x0);
	sys_ll_set_cpu0_int_32_63_en_value(0x0);
	extern bk_err_t ana_rtc_wakeup_config(void *args);
	sleep_allowd = ana_rtc_wakeup_config(NULL);

	if ((check_IRQ_pending())
	 || (sys_ll_get_cpu0_int_0_31_status_value() || sys_ll_get_cpu0_int_32_63_status_value())
	 || (portNVIC_INT_CTRL_REG&portNVIC_SYSTICKSET_BIT)
	 || (sleep_allowd != BK_OK))
	{
		sys_ll_set_cpu0_int_0_31_en_value(int_state1);
		sys_ll_set_cpu0_int_32_63_en_value(int_state2);
		portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
		return;
	}

	portNVIC_SYSTICK_LOAD_REG = PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME;

	sys_hal_mask_cpu0_int();
	aon_pmu_ll_set_r0_fast_boot(1); // set fast boot flag to 1
	sys_drv_wakeup_source_gpio_clear();
	sys_drv_wakeup_source_rtc_clear();
/*----------config wakeup source  start--------------*/
#if CONFIG_GPIO_WAKEUP_SUPPORT
	extern bk_err_t gpio_enable_interrupt_mult_for_wake(void);
	gpio_enable_interrupt_mult_for_wake();
#endif
	sys_hal_enable_ana_rtc_int();
	sys_hal_enable_ana_gpio_int();
/*-----------config wakeup source  end --------------*/

	pmu_val =  aon_pmu_ll_get_r2();
	pmu_val |= BIT(BIT_SLEEP_FLAG_LOW_VOLTAGE);
	aon_pmu_ll_set_r2(pmu_val);

	bk_pm_module_lv_sleep_state_set();

	sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);
	sys_hal_backup_set_flash_26m(&cksel_flash, &clkdiv_flash);

#if CONFIG_INT_WDT
	bk_wdt_stop();
	#if CONFIG_TASK_WDT
	bk_task_wdt_stop();
	#endif
#endif

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	;//flash_ab_info_backup(&ab_flash_reg);
#endif
#if 0

	sys_ll_set_cpu0_int_32_63_en_cpu0_wifi_mac_int_gen_en(0x1);
#if CONFIG_SPE
	sys_ll_set_cpu0_int_32_63_en_cpu0_gpio_int_en(0x1);
#else
	//enable gpio ns interrupt
	sys_ll_set_cpu0_int_32_63_en_cpu0_wifi_hsu_irq_en(0x1);
#endif
	sys_ll_set_cpu0_int_32_63_en_cpu0_rtc_int_en(0x1);
	sys_ll_set_cpu0_int_32_63_en_cpu0_touched_int_en(0x1);
	sys_ll_set_cpu0_int_32_63_en_cpu0_dm_irq_en(0x1);
#endif
	hf_reg_v = sys_hal_disable_hf_clock();

#if 0
	psram_state = sys_ll_get_ana_reg13_enpsram();
	chip_id = aon_pmu_hal_get_chipid();
	if(psram_state != 0x0)//when psram ldo enable(0x1:psram ldo enable)
	{
		if ((chip_id & PM_CHIP_ID_MASK) != (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK))
		{
			sys_ll_set_ana_reg10_vbspbuflp1v(0x0);
		}
	}
	else//when psram ldo disable
	{
		if ((chip_id & PM_CHIP_ID_MASK) != (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK))
		{
			sys_ll_set_ana_reg10_vbspbuflp1v(0x1);
		}
	}

	/*low voltage power optimization*/
	if(sys_ll_get_ana_reg11_enpowa() != 0x1)
	{
		sys_ll_set_ana_reg11_enpowa(0x1);
	}

	#if CONFIG_PSRAM_POWER_DOMAIN_LV_DISABLE
	if(psram_state != 0x0)
	{
		sys_ll_set_ana_reg13_enpsram(0x0);//the psram power domain need the app enable again
	}
	#endif

	if(sys_ll_get_ana_reg12_denburst() != 0x1)
	{
		sys_ll_set_ana_reg12_denburst(0x1);//buckD burst enable
	}

	if(sys_ll_get_ana_reg11_aenburst() != 0x1)
	{
		sys_ll_set_ana_reg11_aenburst(0x1);//buckA burst enable
	}
#endif
	sys_ll_set_ana_reg5_en_cb(0);

	sys_hal_set_power_parameter(PM_MODE_LOW_VOLTAGE);
	sys_hal_set_sleep_condition();
	sys_hal_power_down_pd(&v_sys_r10);
	v_ana_r8 = sys_ll_get_ana_reg8_value();
	/*step 1: set dig voltage to 0.7V*/
#if CONFIG_SOFT_CTRL_VOLT
	uint32_t vcorehsel             = 0;
	uint8_t  ustep                 = 0;
	sys_hal_enable_spi_latch();
	vcorehsel = sys_ll_get_ana_reg8_vcorehsel();
	sys_ll_set_ana_reg8_vcorehsel((s_pm_config.lvsleep_vcorehsel == 0xff) ? PM_LOW_VOL_VCOREHSEL_LDO_SEL : s_pm_config.lvsleep_vcorehsel); //0x0:0.7V vcore
	sys_hal_disable_spi_latch();
#endif
	/*step 2: set ana voltage to 0.9V */
	sys_hal_set_low_voltage(&v_ana_r0, &v_ana_r3, &v_ana_r7, &v_ana_r8, &v_ana_r9, &v_ana_r10);

#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
	bk_flash_enter_deep_sleep();
#endif

#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
	aon_pmu_hal_set_gpio_sleep(1, true);
#else
	sys_hal_gpio_state_switch(true);
#endif

	volatile uint64_t before = bk_aon_rtc_get_us();

/*----enter low voltage sleep-------*/
#if CONFIG_DEEP_LV
	sys_hal_regs_save();
	aon_pmu_hal_backup();
	sys_hal_deep_lv_enter();
	__NOP();
	if (dlv_is_startup_iram()) {
		arch_deep_sleep();
	}
	bootup_restore_time_us = bootloader_to_wfi_time_cost_us();
#else
	arch_deep_sleep();
	bk_delay_us(40); //must delay 40us to sampling rtc value
#endif

	uint64_t current = bk_aon_rtc_get_us();
	sys_hal_set_low_voltage_wakeup_time_us(current);
	sys_hal_set_low_voltage_sleep_duration_us(current - before);
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
	aon_pmu_hal_set_gpio_sleep(0, true);
#else
	sys_hal_gpio_state_switch(false);
#endif

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	;//flash_ab_info_restore(&ab_flash_reg);
#endif
	#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
	bk_flash_exit_deep_sleep();
	#endif

/*--------------------wake up---------------------*/
#if CONFIG_DEEP_LV
	sys_hal_regs_restore();
	aon_pmu_hal_restore();
#endif

/*-----------restore voltage  start----------------*/
	sys_ll_set_ana_reg8_spi_latch1v(1);

	sys_ll_set_ana_reg0_value(v_ana_r0);
	sys_ll_set_ana_reg3_value(v_ana_r3);
	/*step 1: restore ana voltage to 1.1V*/
	sys_ll_set_ana_reg7_value(v_ana_r7);
	/*step 2: restore dig voltage to 0.875V by step */
#if CONFIG_SOFT_CTRL_VOLT
	/*vdddig voltage*/
	for(ustep = ((s_pm_config.lvsleep_vcorehsel == 0xff) ? PM_LOW_VOL_VCOREHSEL_LDO_SEL : s_pm_config.lvsleep_vcorehsel)+1; ustep <= vcorehsel; ustep++)
	{
		sys_ll_set_ana_reg8_vcorehsel(ustep); //restore vdddig
	}
#endif
	sys_ll_set_ana_reg8_value(v_ana_r8);
	sys_ll_set_ana_reg9_value(v_ana_r9);
	sys_ll_set_ana_reg10_value(v_ana_r10);

/*-------------restore voltage  end-----------------*/


/*----------restore analog clock  start--------------*/
	sys_ll_set_ana_reg5_en_cb(1);

	sys_hal_restore_hf_clock(hf_reg_v);

/*-----------restore analog clock  end --------------*/

	sys_hal_power_on_pd(v_sys_r10);

	volatile uint64_t previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	sys_hal_set_exit_low_voltage_tick(previous_tick);

	//Use a function instead of delay
	void pm_low_voltage_bsp_restore(void);
	pm_low_voltage_bsp_restore();

/*----------reset wakeup source  start--------------*/
	#if CONFIG_ANA_RTC_WAKUP_BY_RTC
	sys_ll_set_ana_reg8_rtcwk_rstn(0);
	#else
	sys_ll_set_ana_reg7_timer_wkrstn(0);
	#endif
	sys_ll_set_ana_reg8_gpiowk_rstn(0);
	sys_ll_set_ana_reg8_lvsleep_wkrst(1);
	sys_ll_set_ana_reg8_spi_latch1v(0);
/*-----------reset wakeup source  end --------------*/
	aon_pmu_ll_set_r0_fast_boot(0); // set fast boot flag to 0
#if !CONFIG_DEEP_LV
/*---------------at least delay 190us-----------------*/
	volatile uint64_t current_tick  = 0;
	current_tick = previous_tick;
	while(((current_tick - previous_tick)) < (LOW_POWER_DPLL_STABILITY_DELAY_TIME*AON_RTC_MS_TICK_CNT))
	{
		current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	}
#endif
/*---------------at least delay 190us end -----------------*/
	sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);

	sys_hal_restore_flash_freq(cksel_flash, clkdiv_flash);

	sys_hal_restore_int(int_state1, int_state2);

	portNVIC_SYSTICK_LOAD_REG = PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME;

	portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
#endif
}

void sys_hal_touch_wakeup_enable(uint8_t index)
{
}

void sys_hal_usb_wakeup_enable(uint8_t index)
{
}

void sys_hal_rtc_wakeup_enable(uint32_t value)
{
}

void sys_hal_enter_cpu_wfi()
{
	// TODO - find out the reason:
	// When sys_ll_set_cpu0_int_halt_clk_op_cpu0_int_mask() is called, the normal sleep can't wakedup,
	// need to find out!!!
	// sys_ll_set_cpu0_int_halt_clk_op_cpu0_halt(1);
	arch_sleep();
}

void sys_hal_enter_normal_sleep(uint32_t peri_clk)
{
	volatile uint32_t clken_peri = 0;
	volatile uint8_t cksel_core = 0, clkdiv_core = 0;
	volatile uint8_t cksel_flash = 0, clkdiv_flash = 0;
	volatile uint8_t vcoresel = 0;
	volatile bool en_dpll_dig = false;
	volatile bool en_dpll_ana = false;
	volatile bool xtal_sleep = false;

	clken_peri = sys_ll_get_cpu_device_clk_enable_value();
	sys_ll_set_cpu_device_clk_enable_mac_cken(0);
	sys_ll_set_cpu_device_clk_enable_thread_cken(0);
	sys_ll_set_cpu_device_clk_enable_phy_cken(0);
	sys_ll_set_cpu_device_clk_enable_rf_cken(0);
	sys_ll_set_cpu_device_clk_enable_xvr_cken(0);
	sys_ll_set_cpu_device_clk_enable_btdm_cken(0);
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );

	/*confirm here hasn't external interrupt*/
	if (check_IRQ_pending())
	{
		sys_ll_set_cpu_device_clk_enable_value(clken_peri);
		// os_printf("%x\r\n", clken_peri);
		return;
	}

	/* cpu freq use 10M*/
	cksel_core = sys_ll_get_cpu_clk_div_mode1_cksel_core();
	clkdiv_core = sys_ll_get_cpu_clk_div_mode1_clkdiv_core();
	sys_ll_set_cpu_clk_div_mode1_cksel_core(0);
	sys_ll_set_cpu_clk_div_mode1_clkdiv_core(3);

	/*Flash use 26M */
	cksel_flash = sys_ll_get_cpu_clk_div_mode2_cksel_flash();
	clkdiv_flash = sys_ll_get_cpu_clk_div_mode2_ckdiv_flash();
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(0);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0);

	/*DPLL_DIV disable*/
	en_dpll_dig = sys_ll_get_cpu_device_clk_enable_dplldiv_cken();
	sys_ll_set_cpu_device_clk_enable_dplldiv_cken(0);

	/*latch start*/
	sys_hal_enable_spi_latch();

	/*vcore 0.75v*/
	vcoresel = sys_ll_get_ana_reg8_vcorehsel();
	sys_ll_set_ana_reg8_vcorehsel(0x6);
	bk_delay_us(100);

	/*DPLL disable*/
	en_dpll_ana = sys_ll_get_ana_reg5_en_dpll();
	sys_ll_set_ana_reg5_en_dpll(0);
	bk_delay_us(100);

	/*xtal sleep*/
	xtal_sleep = sys_ll_get_ana_reg3_en_xtalh_sleep();
	sys_ll_set_ana_reg3_en_xtalh_sleep(1);

	/*latch end*/
	sys_hal_disable_spi_latch();

	// sys_ll_set_cpu0_int_halt_clk_op_cpu0_halt(1);
	arch_sleep();

	/*latch start*/
	sys_hal_enable_spi_latch();

	/*xtal sleep disable*/
	sys_ll_set_ana_reg3_en_xtalh_sleep(xtal_sleep);

	/*DPLL enable*/
	sys_ll_set_ana_reg5_en_dpll(en_dpll_ana);
	/*delay for dpll stability*/
	bk_delay_us(200);

	/*vcore 0.875v*/
	sys_ll_set_ana_reg8_vcorehsel(vcoresel);
	/*delay for vddcore stability*/
	bk_delay_us(100);

	/*latch end*/
	sys_hal_disable_spi_latch();

	/*DPLL_DIV enable*/
	sys_ll_set_cpu_device_clk_enable_dplldiv_cken(en_dpll_dig);

	/*cpu freq restore*/
	sys_ll_set_cpu_clk_div_mode1_cksel_core(cksel_core);
	sys_ll_set_cpu_clk_div_mode1_clkdiv_core(clkdiv_core);

	/*Flash freq restore */
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(cksel_flash);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(clkdiv_flash);

	/*delay for flash stability*/
	bk_delay_us(100);

	sys_ll_set_cpu_device_clk_enable_value(clken_peri);
	// os_printf("%x %x\r\n", sys_ll_get_cpu0_int_0_31_status_value(), sys_ll_get_cpu0_int_32_63_status_value());
}

void sys_hal_enter_normal_wakeup()
{
}

void sys_hal_enable_mac_wakeup_source()
{
}

void sys_hal_enable_bt_wakeup_source()
{
}

void sys_hal_wakeup_interrupt_clear(wakeup_source_t interrupt_source)
{
}

int sys_hal_set_lpo_src(sys_lpo_src_t src)
{
	//TODO
	return BK_OK;
}

#define VANALDOSEL_STEP_UP_DELAY_US  (10)
#define VANALDOSEL_STEP_SIZE         (2)
int32_t sys_hal_set_vanaldosel(uint32_t value)
{
	if(value > 0xF) {
		return 0;
	}

	uint32_t cur = 0;
	cur = sys_ll_get_ana_reg7_vanaldosel();
	if (cur == value) {
		return 0;
	}

	sys_ll_set_ana_reg8_spi_latch1v(1);
	if (value < cur) {
		/* Target is lower: set directly */
		sys_ll_set_ana_reg7_vanaldosel(value);
	} else {
		/* Target is higher: step up in increments of VANALDOSEL_STEP_SIZE with delay each step */
		uint32_t v = cur;
		while (v < value) {
			v += VANALDOSEL_STEP_SIZE;
			if (v > value)
				v = value;
			sys_ll_set_ana_reg7_vanaldosel(v);
			bk_delay_us(VANALDOSEL_STEP_UP_DELAY_US);
		}
	}

	sys_ll_set_ana_reg8_spi_latch1v(0);
	return 0;
}

void sys_hal_enter_low_analog(void)
{
	#if CONFIG_TEMP_DETECT
	float temp;
	bk_err_t err = bk_sensor_get_current_temperature(&temp);
	if((err != BK_OK) || (temp < -10))
	{
		sys_hal_set_vanaldosel((s_pm_config.btpll_vanaldosel_high == 0xff) ? 0xa : s_pm_config.btpll_vanaldosel_high);
	}
	else if (temp < 20)
	{
		sys_hal_set_vanaldosel((s_pm_config.btpll_vanaldosel_mid == 0xff) ? 0x7 : s_pm_config.btpll_vanaldosel_mid);
	}
	else if (temp < 60)
	{
		sys_hal_set_vanaldosel((s_pm_config.btpll_vanaldosel_low == 0xff) ? 0x4 : s_pm_config.btpll_vanaldosel_low);
	}
	else
	{
		sys_hal_set_vanaldosel((s_pm_config.btpll_vanaldosel_high == 0xff) ? 0xa : s_pm_config.btpll_vanaldosel_high);
	}
	#else
	sys_hal_set_vanaldosel((s_pm_config.btpll_vanaldosel_high == 0xff) ? 0xa : s_pm_config.btpll_vanaldosel_high);
	#endif

	sys_ll_set_ana_reg3_hpssren((s_pm_config.btpll_hpssren == 0xff) ? 0 : s_pm_config.btpll_hpssren);
	sys_ll_set_ana_reg5_pwd_anabuf_lownoise(1);
	sys_ll_set_ana_reg3_anabuf_sel_tx(1);
	sys_ll_set_ana_reg5_anabufsel_rx(1);
}

void sys_hal_exit_low_analog(void)
{
	sys_hal_set_vanaldosel((s_pm_config.wifipll_vanaldosel == 0xff) ? 0xf : s_pm_config.wifipll_vanaldosel);

	sys_ll_set_ana_reg3_hpssren((s_pm_config.wifipll_hpssren == 0xff) ? 1 : s_pm_config.wifipll_hpssren);
	sys_ll_set_ana_reg5_pwd_anabuf_lownoise(0);
	sys_ll_set_ana_reg3_anabuf_sel_tx(0);
	sys_ll_set_ana_reg5_anabufsel_rx(0);
}

/**
 * set io ldo power mode
 *
 * uint32_t type input:
 *   0: high power mode
 *   1: low power mode
 *   other: undefine
*/
void sys_hal_set_ioldo_lp(uint32_t val)
{
	//TODO: fix it when bring up pm
}

void sys_hal_dco_switch_freq(dco_cali_speed_e speed)
{
	//TODO: fix it when bring up pm
#if 0
	//dco: default calibration is 240M
	switch (speed)
	{
		case DCO_CALIB_SPEED_480M:
		case DCO_CALIB_SPEED_320M:
			break;
		case DCO_CALIB_SPEED_240M:
			sys_ll_set_cpu_clk_div_mode1_cksel_core(0x1);// cpu clock source select dco
			break;
		case DCO_CALIB_SPEED_120M:
			sys_ll_set_ana_reg1_divctrl(0x1);
			sys_ll_set_cpu_clk_div_mode1_cksel_core(0x1);
			break;
		case DCO_CALIB_SPEED_80M:
			sys_ll_set_ana_reg1_divctrl(0x2);
			sys_ll_set_cpu_clk_div_mode1_cksel_core(0x1);
			;// cpu use dco
			break;
		case DCO_CALIB_SPEED_60M:
			sys_ll_set_ana_reg1_divctrl(0x3);
			sys_ll_set_cpu_clk_div_mode1_cksel_core(0x1);
			break;
		default:
			break;
	}
#endif
}

static int sys_hal_config_32k_source_default()
{
	pm_lpo_src_e lpo_src = PM_LPO_SRC_ROSC;

	lpo_src = bk_clk_32k_customer_config_get();
	if(lpo_src == PM_LPO_SRC_X32K)
	{
		sys_ll_set_ana_reg5_en_xtall(0x1);
		if(sys_ll_get_ana_reg5_itune_xtall() != 0xF)
		{
			sys_ll_set_ana_reg5_itune_xtall(0xF);
		}

		sys_ll_set_ana_reg5_itune_xtall(0xA);//0x0 provide highest current for external 32k,because the signal path long
		sys_ll_set_ana_reg5_itune_xtall(0x4);

		aon_pmu_hal_lpo_src_set(PM_LPO_SRC_X32K);

		sys_ll_set_ana_reg9_ckintsel(1);//select buck clock source(0x1: extern 32k)
	}
	else if(lpo_src == PM_LPO_SRC_DIVD)
	{
		aon_pmu_hal_lpo_src_set(PM_LPO_SRC_DIVD);
	}
	else
	{
		aon_pmu_hal_lpo_src_set(PM_LPO_SRC_ROSC);
	}

	return 0;
}

static int sys_hal_enable_buck()
{
	volatile uint8_t cksel_core = 0, clkdiv_core = 0, clkdiv_bus = 0;
	sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);//let the cpu frequency to 26m, in order to be successfully switch voltage provide from ldo to buck

	sys_hal_enable_spi_latch();
	sys_ll_set_ana_reg10_aldosel(0);
	bk_delay_us(1);
	sys_hal_disable_spi_latch();

	sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);
	return 0;
}
static int sys_hal_power_config_default()
{
	//TODO: fix it when bring up pm
	/*config the power domain*/
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_THREAD,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_ENCP,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_BTSP,POWER_MODULE_STATE_OFF);
#if 0
#if CONFIG_SPE
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_ENCP,POWER_MODULE_STATE_OFF);
#endif
	//sys_hal_module_power_ctrl(POWER_MODULE_NAME_BAKP,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_AUDP,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_BTSP,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_WIFIP_MAC,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_WIFI_PHY,POWER_MODULE_STATE_OFF);
	//sys_hal_module_power_ctrl(POWER_MODULE_NAME_MEM5,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_CPU1,POWER_MODULE_STATE_OFF);
	sys_hal_module_power_ctrl(POWER_MODULE_NAME_CPU2,POWER_MODULE_STATE_OFF);

	/*config the analog power*/
	/*1.disable audio test mode save 2ma*/
	sys_ll_set_ana_reg25_test_ckaudio_en(0x0);
	sys_ll_set_ana_reg25_audioen(0x0);

	sys_ll_set_ana_reg6_manu_cin(0x0);

	/*2.psram sel mode to save power consumption*/
	sys_ll_set_ana_reg13_vpsramsel(0x1);
	sys_ll_set_ana_reg13_pwdovp1v(0x1);

#if CONFIG_VBSPBUFLP1V_ENABLE
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	if ((chip_id & PM_CHIP_ID_MASK) != (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK)){
		sys_ll_set_ana_reg10_vbspbuflp1v(0x1);
	}
#endif

	/*decrease the buck ripple wave,which good for wifi evm from hardware and analog reply */
	sys_ll_set_ana_reg9_spi_latch1v(1);
	sys_ll_set_ana_reg12_dswrsten(0x0);
	sys_ll_set_ana_reg10_aswrsten(0x0);
	sys_ll_set_ana_reg9_spi_latch1v(0);
#endif
	return 0;
}
static void sys_hal_wakeup_source_init(void)
{
	sys_hal_set_ana_reg5_pwd_rosc_spi(1);//bit14
	sys_hal_set_ana_reg8_spi_latch1v(1);//bit9
	sys_hal_set_ana_reg8_lvsleep_wkrst(0);//bit12
	sys_hal_set_ana_reg8_gpiowk_rstn(0);//bit13
	sys_hal_set_ana_reg8_rtcwk_rstn(0);//bit14
	sys_hal_set_ana_reg7_timer_wkrstn(0);
	sys_hal_set_ana_reg5_pwd_rosc_spi(0);//bit14
	sys_hal_set_ana_reg5_rosc_disable(0);//bit12
	sys_hal_set_ana_reg8_pwdovp1v(1);//bit24
	#if CONFIG_KEEP_VDDDIG_VOLT_IN_LV
	sys_hal_set_ana_reg8_vlden(0);//bit23
	#else
	sys_hal_set_ana_reg8_vlden(1);//bit23
	#endif
	sys_hal_set_ana_reg8_spi_latch1v(0);
}

void sys_hal_low_power_hardware_init()
{
	persist_pm = get_sys_persist_pm();
	if (NULL != persist_pm) {
		os_memcpy(&s_pm_config, persist_pm, sizeof(sys_persist_pm_t));
	} else {
		os_memset(&s_pm_config, 0xff, sizeof(sys_persist_pm_t));
	}
	#if !CONFIG_AON_PMU_REG0_REFACTOR_DEV
	/*recover aon pmu reg0*/
	uint32_t reg = aon_pmu_ll_get_r7b();
	aon_pmu_ll_set_r0(reg);
	#endif

	/*set GPIO retetnion function continue*/
#if CONFIG_GPIO_RETENTION_SUPPORT
	extern void sys_hal_gpio_retention_reset(void);
	sys_hal_gpio_retention_reset();
#endif

	/*gpio state unlock for shutdown wakeup*/
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
	aon_pmu_hal_set_gpio_sleep(0, true);
#else
	sys_hal_gpio_state_switch(false);
#endif

	/*set memery bypass*/
	aon_pmu_ll_set_r0_memchk_bps(1);

	/*set wakeup source*/
	sys_hal_wakeup_source_init();

	/*enable the buck*/
	#if CONFIG_BUCK_ENABLE
	sys_hal_enable_buck();
	#endif

	/*select lowpower lpo clk source*/
	sys_hal_config_32k_source_default();

	/*default to config the power */
	sys_hal_power_config_default();

	/*set the lp voltage*/
	sys_hal_lp_vol_set((s_pm_config.vcorelsel == 0xff) ? CONFIG_LP_VOL : s_pm_config.vcorelsel);

	/*set rosc modify 2s interval*/
	#if !CONFIG_ROSC_CALIB_SW
	sys_hal_rosc_calibration(2, 0x8);
	#endif

	/*rtc on bk7236N and bk7239n requires spi_timerwken*/
	sys_hal_set_ana_reg9_spi_timerwken(1);

}
