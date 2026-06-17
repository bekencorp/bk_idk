#include "atsvr_unite.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "components/system.h"
#include "bk_system_at.h"

#define TAG "sys_at"

#if CONFIG_PM_ENABLE
#include <modules/pm.h>
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
#include <driver/gpio.h>
#endif

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#include <driver/aon_rtc.h>
#endif

void at_system_reboot(void)
{
	bk_reboot();
}

static int at_misc_reset_system(int sync,int argc, char **argv)
{
    if(argc != 0){
		atsvr_cmd_rsp_error();
        return -1;
    }
	atsvr_cmd_rsp_ok();
	at_system_reboot();
    return 0 ;
}

#if CONFIG_PM_ENABLE
static UINT32 s_at_sleep_mode = 0;
#if CONFIG_AON_RTC || CONFIG_ANA_RTC
static void at_pm_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
	if(s_at_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		os_printf("Attention: unable to enter deepsleep, it's not in a full function state now, please reboot !!!\r\n");
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_at_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	os_printf("at_pm_rtc_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}
#endif //CONFIG_AON_RTC || CONFIG_ANA_RTC

static void at_pm_gpio_callback(gpio_id_t gpio_id)
{
	if(s_at_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_at_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	os_printf("at_pm_gpio_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}

static int at_pwr_cmd_gslp(int sync, int argc, char **argv)
{
	uint32_t timeouts_ms;
	bk_err_t ret = BK_FAIL;

	if (argc != 1) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	timeouts_ms = (int)os_strtoul(argv[0], NULL, 10);
	if (timeouts_ms <= 500) {
		BK_LOGE(TAG, "param %d invalid ! must > 500ms.\r\n", timeouts_ms);
		atsvr_cmd_rsp_error();
		return -1;
	}

	#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	alarm_info_t deep_sleep_alarm = {
		"deep_ps",
		(rtc_tick_t)((uint32_t)timeouts_ms * AON_RTC_MS_TICK_CNT),
		1,
		at_pm_rtc_callback,
		NULL
	};

	bk_alarm_unregister(AON_RTC_ID_1, deep_sleep_alarm.name);
	ret = bk_alarm_register(AON_RTC_ID_1, &deep_sleep_alarm);
	#endif //CONFIG_AON_RTC || CONFIG_ANA_RTC
	if (ret != BK_OK) {
		atsvr_cmd_rsp_error();
		return -1;
	}
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
	bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);
	s_at_sleep_mode = PM_MODE_DEEP_SLEEP;
	atsvr_cmd_rsp_ok();
	return 0;
}

static int at_pwr_cmd_lv_sleep_cfg(int sync, int argc, char **argv)
{
	(void)sync;
	(void)argc;
	(void)argv;

	uint32_t wakeup_source = (int)os_strtoul(argv[0], NULL, 10);
	uint32_t sleep_param1 = (int)os_strtoul(argv[1], NULL, 10);

	if(wakeup_source == 0) //lv sleep wakeup by rtc, sleep_param1 s
	{
		if(argc != 2)
		{
			BK_LOGE(TAG, "invalid arguments count\r\n");
			atsvr_cmd_rsp_error();
			return -1;
		}
		#if CONFIG_AON_RTC || CONFIG_ANA_RTC
		alarm_info_t low_valtage_alarm = {
										"low_vol",
										sleep_param1*1000*AON_RTC_MS_TICK_CNT,
										1,
										at_pm_rtc_callback,
										NULL
										};
		//force unregister previous if doesn't finish.
		bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
		bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);
		#endif //CONFIG_AON_RTC
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0);
		bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);
	} else if (wakeup_source == 2) { //lv sleep wakeup by GPIO, sleep_param1 is GPIO index, sleep_param2 is GPIO interrupt type
		if(argc != 3)
		{
			BK_LOGE(TAG, "invalid arguments count\r\n");
			atsvr_cmd_rsp_error();
			return -1;
		}
		if(sleep_param1 > 28)
		{
			BK_LOGE(TAG, "invalid GPIO index\r\n");
			atsvr_cmd_rsp_error();
			return -1;
		}
		uint32_t sleep_param2 = (int)os_strtoul(argv[2], NULL, 10);
		if(sleep_param2 >= GPIO_INT_TYPE_MAX)
		{
			BK_LOGE(TAG, "invalid GPIO interrupt type\r\n");
			atsvr_cmd_rsp_error();
			return -1;
		}
		#if CONFIG_GPIO_WAKEUP_SUPPORT
		bk_gpio_register_isr(sleep_param1, at_pm_gpio_callback);
		#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
		bk_gpio_register_wakeup_source(sleep_param1,sleep_param2);
		#endif
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
		#endif //CONFIG_GPIO_WAKEUP_SUPPORT
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0);
		bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);
	} else {
		BK_LOGE(TAG, "invalid wakeup source %d\r\n", wakeup_source);
		atsvr_cmd_rsp_error();
		return -1;
	}
	s_at_sleep_mode = PM_MODE_LOW_VOLTAGE;

	atsvr_cmd_rsp_ok();
	return 0;
}
#endif /* CONFIG_PM_ENABLE */

const struct _atsvr_command misc_cmds_table[] = {
	ATSVR_CMD_HADLER("AT+RST","AT+RST",NULL,at_misc_reset_system,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+GSLP","AT+GSLP=<time_ms>",NULL,at_pwr_cmd_gslp,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+SLEEPWCFG","AT+SLEEPWCFG=<wakeup source><param1>[,<param2>]",NULL,at_pwr_cmd_lv_sleep_cfg,false,0,0,NULL,false),
};

void at_misc_cmd_init(void)
{
	atsvr_register_commands(misc_cmds_table, sizeof(misc_cmds_table) / sizeof(struct _atsvr_command),"misc",NULL);

}

