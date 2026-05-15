Wi-Fi Low Voltage Keep-Alive Configuration
=======================================================

:link_to_translation:`zh_CN:[中文]`

Function call sequence for Wi-Fi low voltage keep-alive
--------------------------------------------------------------
- 1. If BLE is used for provisioning and no longer needed, BLE can be turned off to achieve lower power consumption;

    bk_bluetooth_deinit();

    Turn off BLE

- 2. bk_wifi_send_listen_interval_req(10);

    Set listen interval to 10;

- 3. bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

    Set sleep mode and enter low voltage sleep.

- 4. demo_sta_app_init("ssid", password");

    Connect to AP (ssid: AP name; password: AP password)

- 5. bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);

    Vote APP ticket to the system.

(Note: The device must connect to AP and obtain an IP address before calling bk_pm_module_vote_sleep_ctrl() to vote APP ticket, otherwise the mobile device cannot wake up the test device through network.)


Notes:

 - 1. BLE and WIFI tickets are voted by BLE and WIFI modules after entering sleep. This is handled inside SDK and applications do not need to care.

 - 2. When BLE or WIFI modules are not powered on in the system, the system automatically votes sleep tickets for these two modules.

Wi-Fi low voltage keep-alive reference data
--------------------------------------------------------------
With default configuration CONFIG_DPD_CALI=y, the table below lists reference keep-alive current data under different supply voltages and different Wi-Fi bands. Test conditions: DTIM=10, device connected to AP, BLE turned off, and low voltage sleep mode entered:

+----------------+-----------+----------------------+
| Supply Voltage | WiFi Band | Typical Current Value|
+================+===========+======================+
| 3.3V           | 2.4G      | about 118uA          |
+----------------+-----------+----------------------+
| 3.3V           | 5G        | about 130uA          |
+----------------+-----------+----------------------+
| 5V             | 2.4G      | about 115uA          |
+----------------+-----------+----------------------+
| 5V             | 5G        | about 118uA          |
+----------------+-----------+----------------------+

When CONFIG_DPD_CALI=n, code can run in RAM and lower limit power can be achieved. The following table shows limit power data measured under the same test conditions:

+----------------+-----------+----------------------+
| Supply Voltage | WiFi Band | Minimum Current Value|
+================+===========+======================+
| 3.3V           | 2.4G      | about 98uA           |
+----------------+-----------+----------------------+
| 3.3V           | 5G        | about 96uA           |
+----------------+-----------+----------------------+
| 5V             | 2.4G      | about 87uA           |
+----------------+-----------+----------------------+
| 5V             | 5G        | about 85uA           |
+----------------+-----------+----------------------+

.. note::
   1. Minimum power test conditions are the same as standard test conditions: DTIM=10, device connected to AP, BLE turned off
   2. When ``CONFIG_DPD_CALI=n`` is configured, key code runs in RAM, reducing power consumption caused by loading code from Flash during wakeup
   3. Actual power consumption may vary slightly due to specific application scenarios, hardware versions, and environmental factors

