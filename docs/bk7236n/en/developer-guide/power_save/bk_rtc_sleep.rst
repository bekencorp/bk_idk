Use RTC as wakeup source for deep sleep and low voltage sleep
=============================================================

:link_to_translation:`zh_CN:[中文]`

Function call sequence for using RTC as wakeup source and entering deep sleep
-----------------------------------------------------------------------------
- 1. Configure RTC wakeup parameters

  ::

      alarm_info_t deep_sleep_alarm = {
                                        "deep_ps",
                                        3000*RTC_TICKS_PER_1MS,//wake up after 3000 ms, configurable by requirement
                                        1,
                                        cli_pm_rtc_callback,
                                        NULL
                                      };

      bk_alarm_register(AON_RTC_ID_1, &deep_sleep_alarm);


- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

    Set RTC as wakeup source in the system

- 3. bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

    Set sleep mode and enter deep sleep

Function call sequence for using RTC as wakeup source and entering low voltage sleep
-------------------------------------------------------------------------------------
- 1. Configure RTC wakeup parameters;

  ::

      alarm_info_t low_valtage_alarm = {
                                        "low_vol",
                                        3000*RTC_TICKS_PER_1MS,//wake up after 3000 ms, configurable by requirement
                                        1,
                                        cli_pm_rtc_callback,
                                        NULL
                                        };
      bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);


- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

    Set RTC as wakeup source in the system

- 3. bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

    Set sleep mode and enter low voltage sleep
    (Note: low voltage sleep is currently the default sleep mode. The system can enter low voltage sleep once required votes are satisfied)

- 4. bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);

    Vote APP ticket to the system

Notes:

 - 1. BLE and WIFI tickets are voted internally by BLE/WIFI modules after entering sleep. This is handled by SDK and applications do not need to care;

 - 2. When BLE or WIFI modules are not powered on, the system automatically votes sleep tickets for these two modules;

