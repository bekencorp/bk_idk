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
#include "bk_rf_internal.h"
#include "bk_feature.h"

#if CONFIG_MAC802154_ENABLE
#include "lw_mac802154_interface.h"
#endif
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif // CONFIG_FREERTOS_SMP

#if CONFIG_MAC802154_ENABLE
#if CONFIG_OPENTHREAD
extern void bk_ieee802154_check_ed_scan_stop(void);
extern bool bk_ieee802154_check_ed_scan_start(void);
#endif
#endif

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
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX) && CONFIG_WIFI_ENABLE
	phy_exit_dsss_only();
#else
#endif

}

void phy_enter_dsss_only_ptr(void)
{
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX) && CONFIG_WIFI_ENABLE
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

void sys_drv_module_power_ctrl_ptr(unsigned int module, uint32_t power_state)
{
	sys_drv_module_power_ctrl(module,power_state);
}

void sys_drv_set_ana_reg11_apfms_ptr(uint32_t value)
{
	sys_drv_set_ana_reg11_apfms(value);
}

void sys_drv_set_ana_reg12_dpfms_ptr(uint32_t value)
{
	sys_drv_set_ana_reg12_dpfms(value);
}

bk_err_t bk_pm_module_vote_power_ctrl_ptr(unsigned int module, uint32_t power_state)
{
	return bk_pm_module_vote_power_ctrl((pm_power_module_name_e)module, (pm_power_module_state_e)power_state);
}

void sys_hal_low_analog_set(uint32_t en)
{
    if(en)
    {
        sys_hal_enter_low_analog();
    }else
    {
        sys_hal_exit_low_analog();
    }
}

void bk_thread_rf_coex_leave_for_wifi_ble(void)
{
#if CONFIG_RF_COEX_JUDGE && CONFIG_MAC802154_ENABLE
	if (get_current_rf_path() == RF_PATH_THREAD_IQ)
	{
		if (lw_mac802154_lw_macl_is_tx_ongoing_pl() || lw_mac802154_lw_macl_is_rx_ongoing_pl())
		{
			BK_LOGI("rf", "thread is trxing %u %u\r\n",
				(unsigned int)lw_mac802154_lw_macl_is_tx_ongoing_pl(),
				(unsigned int)lw_mac802154_lw_macl_is_rx_ongoing_pl());
		}
	}
	lw_mac802154_thread_tx_stop();
	lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
	#if CONFIG_OPENTHREAD
	bk_ieee802154_check_ed_scan_stop();
	#endif
#endif
}

void bk_thread_rf_coex_rx_start_on_enter_thread(void)
{
#if CONFIG_RF_COEX_JUDGE && CONFIG_MAC802154_ENABLE
	#if CONFIG_OPENTHREAD
	if(bk_ieee802154_check_ed_scan_start())
	#endif
	{
		lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
		lw_mac802154_lw_macl_rx_config_pl_ext(LW_TRUE);
	}
#endif
}

uint8_t sys_hal_rf_ctrl_type_get_ptr(void)
{
	return sys_hal_rf_ctrl_type_get();
}

int bk_feature_rf_coex_judge_enable(void)
{
    return bk_feature_rf_coex_judge();
}

const rf_control_funcs_t g_rf_control_funcs = {
    ._sys_drv_modem_bus_clk_ctrl  = sys_drv_modem_bus_clk_ctrl_ptr,
    ._sys_drv_modem_clk_ctrl  = sys_drv_modem_clk_ctrl_ptr,
    ._phy_exit_dsss_only = phy_exit_dsss_only_ptr,
    ._phy_enter_dsss_only = phy_enter_dsss_only_ptr,
    ._rtos_disable_int = rtos_disable_int_ptr,
    ._rtos_enable_int = rtos_enable_int_ptr,
    ._rwnx_cal_mac_sleep_rc_recover = rwnx_cal_mac_sleep_rc_recover,
    ._sys_drv_module_power_ctrl = sys_drv_module_power_ctrl_ptr,
    ._sys_drv_set_ana_reg11_apfms = sys_drv_set_ana_reg11_apfms_ptr,
    ._sys_drv_set_ana_reg12_dpfms = sys_drv_set_ana_reg12_dpfms_ptr,
    ._bk_pm_module_vote_power_ctrl = bk_pm_module_vote_power_ctrl_ptr,
    ._sys_hal_low_analog_set = sys_hal_low_analog_set,
    ._thread_rf_coex_leave_for_wifi_ble = bk_thread_rf_coex_leave_for_wifi_ble,
    ._thread_rf_coex_rx_start_on_enter_thread = bk_thread_rf_coex_rx_start_on_enter_thread,
    ._sys_hal_rf_ctrl_type_get = sys_hal_rf_ctrl_type_get_ptr,
    ._bk_feature_rf_coex_judge_enable = bk_feature_rf_coex_judge_enable,
};

const rf_variable_t g_rf_variable = {
    ._pm_power_module_state_off = PM_POWER_MODULE_STATE_OFF,
    ._pm_power_module_state_on = PM_POWER_MODULE_STATE_ON,
    ._pm_power_module_name_phy = PM_POWER_MODULE_NAME_PHY,
    ._pm_power_module_name_rf = PM_POWER_SUB_MODULE_NAME_PHY_RF,
    ._pm_power_module_name_mac = PM_POWER_MODULE_NAME_WIFIP_MAC,
    ._pm_power_module_name_ofdm = PM_POWER_MODULE_NAME_OFDM,
};

void bk_rf_adapter_init(void)
{
    rf_adapter_init(&g_rf_control_funcs, &g_rf_variable);
    rf_cntrl_init();
}

