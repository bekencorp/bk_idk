Configure GPIO as wakeup source for deep sleep and low voltage sleep
=====================================================================

:link_to_translation:`zh_CN:[中文]`

Configure GPIO as wakeup source for entering deep sleep: function call order
-----------------------------------------------------------------------------
- 1. bk_gpio_register_wakeup_source(GPIO_18,GPIO_INT_TYPE_FALLING_EDGE);

  a. Configure wakeup GPIO ID: GPIO18

  b. Configure GPIO wakeup mode:

::
    GPIO_INT_TYPE_LOW_LEVEL: wake up on low level;
    GPIO_INT_TYPE_HIGH_LEVEL: wake up on high level;
    GPIO_INT_TYPE_RISING_EDGE: wake up on rising edge;
    GPIO_INT_TYPE_FALLING_EDGE: wake up on falling edge;

- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);

    Set GPIO as wakeup source in the system.

- 3. bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

    Set sleep mode and enter deep sleep.

    (After wakeup from deep sleep, call bk_gpio_get_wakeup_gpio_id() to get which GPIO ID woke up the system.)


Configure GPIO as wakeup source for entering low voltage sleep: function call order
------------------------------------------------------------------------------------
- 1. bk_gpio_register_wakeup_source(GPIO_18,GPIO_INT_TYPE_FALLING_EDGE);

  a. Configure wakeup GPIO ID: GPIO18;

  b. Configure GPIO wakeup mode:

    GPIO_INT_TYPE_LOW_LEVEL: wake up on low level;
    GPIO_INT_TYPE_HIGH_LEVEL: wake up on high level;
    GPIO_INT_TYPE_RISING_EDGE: wake up on rising edge;
    GPIO_INT_TYPE_FALLING_EDGE: wake up on falling edge;

    GPIO_INT_TYPE_RISING_EDGE: wake up on rising edge;

- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);

    Set GPIO as wakeup source in the system.

- 3. bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

    Set sleep mode and enter low voltage sleep.
    (Note: low voltage sleep is currently the default sleep mode. The system can enter low voltage sleep when required votes are satisfied.)

- 4. bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);

    Vote APP ticket to the system.

Notes:

 - 1. BLE and WIFI votes are set by BLE/WIFI modules after entering sleep. This is handled internally by SDK and applications do not need to care.

 - 2. When BLE or WIFI modules are not powered on in the system, the system will automatically vote sleep tickets for these two modules.

