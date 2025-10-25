.. _pm-cli:

**Power Manager cli Command**
====================================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *Support platform power manager (normal sleep, low voltage, deep sleep)*

- :ref:`pm<pm-pm>`             :power manager for platform

- :ref:`pm_vote<pm-pm_vote>`   :Keep the WIFI alive to wake up low-voltage devices


.. _pm-pm:

:ref:`pm<pm-cli>` **:power manager for platform(normal sleep、low voltage 、deepsleep)**
----------------------------------------------------------------------------------------------

**Command** ::

    pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]

**Params** ::
    
    sleep_mode：sleep mode (parameter range: 0-3): 
                
                0:normal sleep; 
                
                1:low voltage; 
                
                2:deep sleep;3:none
    
    wake_source：Wakeup Source (Parameter Range: 0-5)： 
                
                0：GPIO；
                
                1：RTC；
                
                2：WIFI/BT；
                
                3：USB（BK72xx is not supported, other projects are supported);
                
                4：touch；
                
                5：none
    
    Vote1: vote2: vote3: module id for the vote to move to sleep:
                
                8:(BT)
                
                9(WIFI)
                
                12(APP)
    
    param1：The meaning varies depending on the source of the awakenings:
                
                GPIO：GPIO ID 
                
                RTC： Awake Time
                
                WIFI/BT： 
                
                touch：channel ID of touch
    
    param2：The meaning varies depending on the source of the awakenings:
                
                GPIO：Awake method:
                    
                    0：Low-power
                    
                    1：High-power
                    
                    2：Rising edge 
                    
                    3：Falling edge
                
                RTC和touch：No param,default is 0
    
    param3：BT sleep time;

            this parameter is compatible with the interface for sending commands. 
    
            It is recommended to set the time to 1000000000 (100s), and the reason for setting such a large value is to use the time set by the internal module of the BT module as the standard.


**Usage Scenario** ::
    
    Example 1: Using GPIO as a awaken source to wake up the deepsleep device:

        1）After booting and starting for 5 seconds, (after the boot-up, BT, WIFI, and CPU perform initialization).
 
        2）Input：

            A. PM 2 0 8 9 12 9 0 1000000000 :Let the system enter deepsleep, use gpio for wake-up, 9 indicates the gpio ID, and 0 represents low-level wake-up.
        
            B. If you want a certain GPIO to be woken up by a high level signal, connect it to a low level for its initial state before the test, and to a high level for waking up.
        
            C. If you wish to wake up a GPIO from a low level, then connect the initial state of the GPIO to high before the test, and connect it to low when waking up.
        
            D. If you want a specific GPIO to wake up on the rising edge, set the initial state of that GPIO to low before the test, and connect it to high voltage when it wakes up.
            
            E. If you want a certain GPIO to wake up from a falling edge, do not set the initial state of that GPIO to high before the test; connect it to low when it wakes up.
        
        3）During the deepsleep process, measuring the voltage of VDDDIG will become 0; after waking up, the voltage of VDDDIG will become 1.1V；
        
        4）Simultaneously, by using a precise power supply, the power consumption of the entire process from deep sleep to wake-up is measured.

        Result：

            1）During the deepsleep process, measuring the voltage of VDDDIG will become 0; after waking up, the voltage of VDDDIG will become 1.1V.
            
            2）Simultaneously, the power consumption of the entire process from deep sleep to wake-up is measured by using a precise power supply.
            
            3）The power consumption after deepsleep is as expected to be around 15ua to 20ua.

   
    Example 2: Use RTC to set the wake-up time to wake up a deepsleep device:

        1）After booting up for 5 seconds (after the boot-up, BT, WIFI, and CPU perform initialization);

        2）Input command: 
            pm 2 1 8 9 12 3000 0 1000000000 (to put the system into deepsleep, using RTC for wake-up, where 3000 represents the set wake-up time, which can be modified as needed, with the unit being ms).
        
        3）After the system reaches the set wake-up time, the system will be activated.
        
        4）During the deepsleep process, measuring the voltage of VDDDIG will become 0; after waking up, the voltage of VDDDIG will become 1.1 volts.
        
        5）Simultaneously, the power consumption throughout the entire process from deep sleep to wake-up is measured by using precision power supplies.

        Result：

            1）During the deepsleep process, measuring the voltage of VDDDIG will become 0; after waking up, the voltage of VDDDIG will become 1.1V.
        
            2）Simultaneously, by using precise power supplies to measure the energy consumption throughout the entire deepsleep to wake-up process.
        
            3）The power consumption after deepsleep is as expected to be around 15ua-20ua.


    Example 3: Waking up a deepsleep device using touch (touch) as the wake source:

        1）After booting up for 5 seconds (after the boot-up, BT, WIFI, and CPU perform initialization);
        
        2）Input 2 command：
            
            A. touch_single_channel_calib_mode_test 2 1

                The number 2 represents the touch channel number; the default test version is 2,and the channel should be determined based on actual usage;\
                
                The number 1 indicates the strength setting of the touch channel, with a value range of 0 to 3
        
            B. pm 2 4 8 9 12 2 0 1000000000
            
                Put the system into deep sleep, and wake it up with touch.
        
            C. Manual touch can wake up the system from the deep sleep state.
        
        3）During the deepsleep process, measuring the voltage of VDDDIG will become 0; after waking up, the voltage of VDDDIG will become 1.1 volts.
        
        4）Simultaneously, the power consumption throughout the entire process from deep sleep to wake-up is measured by using precision power supplies.

        Result：

            1）During the deepsleep process, measuring the voltage of VDDDIG will turn to 0; after wake-up (using touch), the voltage of VDDDIG will become 1.1v.
            
            2）Simultaneously, by using a precise power supply to measure the power consumption throughout the process from deep sleep to wake-up.
            
            3）The power consumption after deepsleep is as expected to be around 15ua-20ua.


    Example 4: Using GPIO to wake up a low-voltage device:

        1）After booting up for 5 seconds (after the boot-up, BT, WIFI, and CPU perform initialization);

        2）Input：

            A. PM 1 0 8 9 12 9 2 1000000000 
            
                this command makes the system enter low voltage mode, wakes it up using GPIO, with 9 representing the GPIO ID and 2 indicating an edge trigger on rising
            
            B. If you wish to wake up a GPIO from a high level, connect the initial state of the GPIO to a low level before testing, and connect it to a high level when waking up.
            
            C. If you wish to wake up a GPIO from a low level, set the initial state of the GPIO to high before testing, and connect it to a low level when waking up.
            
            D. If you wish to wake up a certain GPIO from the rising edge, set the initial state of that GPIO to low before testing, and connect it to high when waking up.
            
            E. If you want a GPIO to wake up on a falling edge, do not connect the initial state of that GPIO to a high level before the test; instead, connect it to a low level when waking up.
            
        3）During the low voltage process, measuring the voltage of VDDDIG will become the set voltage value (e.g., 0.9 or 1.0V); after waking up, the voltage of VDDDIG will become 1.1V.
        
        4）Simultaneously, the power consumption throughout the process from the low voltage to wake-up is measured using a precision power supply.


        Result：

        1）During the low voltage process, measuring the VDDDIG voltage will become the set voltage value (e.g., 0.9V or 1.0V); after waking up, the VDDDIG voltage will become 1.1V.
        
        2）Simultaneously, the power consumption throughout the entire process from low voltage to wake-up is measured by using precision power supplies.


        For optimal power consumption, please turn off BT after use, the shutdown command is: AT+BLEPOWER=0!!!


    Example 5: Use RTC as a wake-up source to wake up a low-power device:

        1）After booting up for 5 seconds (after the boot-up, BT, WIFI, and CPU perform initialization);
        
        2）Input：
        
            A. When there is nothing to do with WIFI, it defaults to entering a sleep state.
        
            B. Bluetooth defaults to entering sleep mode (for optimal power consumption, please turn off Bluetooth after use, and the shutdown command is: AT+BLEPOWER=0).
        
            C. pm 1 1 0 0 12 10000 0 1 
                
                Set the system to enter low voltage mode, use RTC for wake-up, 10000 represents the set wake-up time, which can be modified as needed, and the unit is milliseconds.
        
        3）After the system reaches the specified wake-up time, the system will be woken up from a low-power state.
        
        4）During the low voltage process, measuring the voltage of VDDDIG will become the set voltage value (e.g., 0.6V or 0.9V); after waking up, the voltage of VDDDIG will become 1.1V.
        
        5）Simultaneously, measure the power consumption throughout the entire low voltage to wake-up process using a precision power supply.


        Result：

        1）During the low voltage process, measuring the VDDDIG voltage will be adjusted to the set voltage value (e.g., 0.6V or 0.9V); after waking up, the VDDDIG voltage will then become 1.1V.

        2）Simultaneously measuring the power consumption throughout the entire low voltage to wake-up process using a precise power supply.


    Example 6: Use RTC as an wake-up source to wake up a low-power device:

        1）After booting up for 5 seconds (after the boot-up, BT, WIFI, and CPU perform initialization);
        
        2）Input：

            A. touch_single_channel_calib_mode_test 2 1 
            
                the number 2 represents the touch channel number, the default test version is 2, and the channel should be determined based on actual use; 
                
                the number 1 indicates the intensity setting of the touch channel, with a range of 0 to 3
    
            B. pm 1 4 8 9 12 2 0 1,000,000,000 
            
                Set the system to enter low voltage, wake up using touch, with 2 as the touch channel number
        
            C. Manual touch can wake the system from the low voltage state.
        
        3）During the low voltage process, measuring the voltage of VDDDIG will become the set voltage value (e.g., 0.6V or 0.9V); after waking up, the voltage of VDDDIG will become 1.1V.
        
        4）Simultaneously, measure the power consumption throughout the entire low voltage to wake-up process using precision power supplies.

        Result：

        1）During the low voltage process, measuring the voltage of VDDDIG will change to the set voltage value (e.g., 0.9 or 1.0V); after being awakened, the voltage of VDDDIG will become 1.1V.
        
        2）Simultaneously, measure the power consumption throughout the entire low voltage to wake-up process by using a precision power supply.


        For optimal power consumption, please turn off BT after use, and the shutdown command is: AT+BLEPOWER=0!!!

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect when CONFIG_MCU_PS is enabled!*


.. _pm-pm_vote:

:ref:`pm_vote<pm-cli>`  **:Keep the WIFI alive to wake up low-voltage devices**
-------------------------------------------------------------------------------------

**Command** ::

    pm_vote [sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]  

**Params** ::
    
    sleep_mode：Sleep Mode (parameter range: 0-3):
    
        0:normal sleep;
        
        1:low voltage; 
        
        2:deep sleep;
        
        3:none
    
    pm_vote:Vote module ID:
            
        8:(BT)
        
        9(WIFI)
        
        12(APP)

    pm_vote_value：vote value
    
        0：MODULE_STATE_ON   
        
        1：MODULE_STATE_OFF）
    
    pm_sleep_time：BT sleep time, this parameter is compatible with the interface for sending commands, and the recommended setting time is 1000000000 (100s). 
                   The reason for setting such a large value is to use the time set by the internal module of the BT module as the standard.

**Usage Scenario** ::
    
    Example 1: Use RTC as a wake-up source to wake up low-power devices:
    
        1）After booting up for 5 seconds (after the boot-up, BT, WIFI, and CPU perform initialization);
    
        2）Input：
       
            A. set_interval 10 (This command sets DTIM10) 
            
                Note: To configure other values: DTIM1 command: set_interval 1; DTIM3 command: set_interval 3

            B. sta 123 (Router Name) 12345678 (Router Password) 
            
                Upon connecting to the router, the WIFI will enter the sleep-wakeup process
    
            C. Bluetooth defaults to entering sleep mode (for optimal power consumption, please turn off Bluetooth after use, and use the following command to shut down: AT+BLEPOWER=0).
    
            D. pm_vote 1 12 1 0（vote  to app）
    
        3）During the low voltage process, measuring the voltage of VDDDIG will become the set voltage value (e.g., 0.6V or 0.9V); after waking up, the voltage of VDDDIG will become 1.1V.
    
        4）Simultaneously, measure the power consumption throughout the entire low voltage to wake-up process using precision power supplies.


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

     *This command takes effect when CONFIG_MCU_PS is enabled!*
