设置RTC作为唤醒源-系统进入深度,超深度睡眠以及低压睡眠配置说明
==============================================================

:link_to_translation:`en:[English]`

RTC-深度睡眠调用函数和调用顺序说明
--------------------------------------------
- 1. 配置RTC唤醒源的参数

    alarm_info_t low_valtage_alarm = {

                                      "low_vol",

                                      3000*RTC_TICKS_PER_1MS,//设置为3000ms后唤醒，唤醒时间可以根据需求更改

                                      1,

                                      cli_pm_rtc_callback,

                                      NULL

                                      };

    bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);


- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

    向系统中设置RTC方式作为唤醒源

- 3. bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

    设置睡眠模式，进入深度睡眠

RTC-低压睡眠调用函数和调用顺序说明
--------------------------------------------
- 1. 配置RTC唤醒源的参数;

    alarm_info_t low_valtage_alarm = {

                                      "low_vol",

                                      3000*RTC_TICKS_PER_1MS,//设置为3000ms后唤醒，唤醒时间可以根据需求更改

                                      1,

                                      cli_pm_rtc_callback,

                                      NULL

                                      };

    bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);


- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

    向系统中设置RTC方式作为唤醒源

- 3. bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

    设置睡眠模式，进入低压睡眠
    (备注：当前系统默认设置为低压睡眠，只需要满足投的票，就可以进入低压睡眠)

- 4. bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);

    向系统中投应用程序的票

.. note::

 - 1. BT和WIFI的票，BT和WIFI模块内部进入睡眠后自己投上，SDK内部做好，应用程序不用关注。应用程序需要做的事情是，进入低压和deepsleep休眠前，把上层业务关闭，再配置唤醒源，调用接口进入睡眠。

 - 2. 当系统中BT或WIFI模块没有上电时，系统会自动给这两个模块投上sleep的票。
