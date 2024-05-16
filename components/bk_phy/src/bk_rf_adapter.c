#include <common/bk_include.h>
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "bk_rf_adapter.h"
#include "adc_driver.h"
#include "bk_phy_internal.h"
#include "sys_driver.h"
#include "sys_ll.h"
#include <modules/pm.h>
#include "bk_wifi.h"

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif // CONFIG_FREERTOS_SMP

uint32_t sys_drv_modem_bus_clk_ctrl_ptr(bool clk_en)
{
	return sys_drv_modem_bus_clk_ctrl(clk_en);
}

uint32_t sys_drv_modem_clk_ctrl_ptr(bool clk_en)
{
	return sys_drv_modem_clk_ctrl(clk_en);
}

void phy_exit_dsss_only_ptr(void)
{
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX)
	phy_exit_dsss_only();
#else
#endif

}

void phy_enter_dsss_only_ptr(void)
{
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX)
	phy_enter_dsss_only();
#else
#endif
}

#ifdef CONFIG_FREERTOS_SMP
static volatile spinlock_t rf_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP
static uint32_t rtos_disable_int_ptr(void)
{
	uint32_t int_level = rtos_disable_int();
	#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&rf_spin_lock);
	#endif // CONFIG_FREERTOS_SMP
	return int_level;
}

static void rtos_enable_int_ptr(uint32_t int_level)
{
	#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&rf_spin_lock);
	#endif // CONFIG_FREERTOS_SMP
	rtos_enable_int(int_level);
}

const rf_control_funcs_t g_rf_control_funcs = {
    ._sys_drv_modem_bus_clk_ctrl  = sys_drv_modem_bus_clk_ctrl_ptr,
    ._sys_drv_modem_clk_ctrl  = sys_drv_modem_clk_ctrl_ptr,
    ._phy_exit_dsss_only = phy_exit_dsss_only_ptr,
    ._phy_enter_dsss_only = phy_enter_dsss_only_ptr,
    ._rtos_disable_int = rtos_disable_int_ptr,
    ._rtos_enable_int = rtos_enable_int_ptr,
    ._rwnx_cal_mac_sleep_rc_recover = rwnx_cal_mac_sleep_rc_recover,
};


void bk_rf_adapter_init(void)
{
    rf_adapter_init(&g_rf_control_funcs);
    rf_cntrl_init();
}

