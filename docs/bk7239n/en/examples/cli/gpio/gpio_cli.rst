.. _gpio-cli:

**GPIO cli Command**
=======================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *The test cases related to gpio require the macros CONFIG_GPIO and CONFIG_DRIVER_TEST to be enabled.*

- :ref:`gpio set_config<gpio-gpioset>`          :Configure GPIO status

- :ref:`gpio output<gpio-gpiooutput>`           :Configure GPIO output status

- :ref:`gpio output<gpio-gpioinput>`            :Configure the input state of GPIO

- :ref:`gpio output_high<gpio-gpioouth>`        :Configure the GPIO output level

- :ref:`gpio input_get<gpio-gpioinpg>`          :Get the GPIO input level

- :ref:`gpio_int<gpio-gpioint>`                 :Configure the GPIO interrupt type and enable or disable interrupts.

- :ref:`gpio_wake<gpio-gpiowake>`               :Before entering the low-power mode, the corresponding GPIO is registered as a wake-up source.

- :ref:`gpio_kpsta<gpio-gpiokpsta>`             :GPIO that needs to maintain its state under the low-voltage registration mode

- :ref:`gpio_driver<gpio-gpiodrv>`              :Used to initialize (de-initialize) resources related to GPIO


.. _gpio-gpioset:

:ref:`gpio set_config<gpio-cli>` **:Configure GPIO status**
----------------------------------------------------------------------------------------------

**Command** ::

    gpio set_config <id> <io_mode> <pull_mode>

**Params** ::
    
    id: GPIO pins to be operated on

    io_mode:GPIO input/output enable
    
            0: High-resistance state
            
            1:Output Enable
            
            2:Input Enable
            
            3:Invalid

    pull_mode:Set GPIO pull-up and pull-down resistors.；
    
            0:disable；
            
            1:Enable by pull-down；
            
            2:Enable by pulling up
            
            3:Invalid

**Usage Scenario** ::
    
    Input:

        gpio set_config 5 2 1

    Output:

        gpio io(output/disable/input): 2 ,  pull(disable/down/up) : 1


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command requires the CONFIG_GPIO and CONFIG_DRIVER_TEST options to be enabled!*

    *The GPIO that has already enabled the second function must first disable the second function before proceeding with this setting.*

.. _gpio-gpiooutput:

:ref:`gpio output<gpio-cli>` **:Configure GPIO output status**
----------------------------------------------------------------------------------------------

**Command** ::

    gpio output <id> <pull_up|pull_down>

**Params** ::
    
    id: GPIO pins to be operated on

    pull_up/pull_down:pull up|pull down
    

**Usage Scenario** ::
    
    Input：

        gpio output 5 pull_down

    Output：

        gpio output test: 5

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _gpio-gpioinput:

:ref:`gpio input<gpio-cli>` **:Configure the input state of GPIO**
----------------------------------------------------------------------------------------------

**Command** ::

    gpio input <id> <pull_up|pull_down>

**Params** ::
    
    id: GPIO pins to be operated on

    pull_up/pull_down:pull up|pull down
    

**Usage Scenario** ::
    
    Input：

        gpio input 5 pull_down

    Output：

        gpio input test: 5

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _gpio-gpioouth:

:ref:`gpio output_high<gpio-cli>` **:Configure the GPIO output level**
----------------------------------------------------------------------------------------------

**Command** ::

    gpio <output_high|output_low> <id>

**Params** ::
    
    id: GPIO pins to be operated on

    pull_up/pull_down:pull up|pull down

**Usage Scenario** ::

    Input：gpio output_high 5

        Enter this command, if the configuration is successful, then return:

            $cli:I(42274):gpio output high
        
        If the command is not recognized after entering this command, return:
        
            $err:E(5890):cli_gpio_cmd 114: ret=-0x2007
            
            cli:I(5890):gpio output high

     Input：gpio output_low 5

        Enter this command, if the configuration is successful, then return:
        
            $cli:I(42274):gpio output low
        
        If the command is not recognized after entering this command, return:
        
            $err:E(5890):cli_gpio_cmd 114: ret=-0x2007
            
            cli:I(5890):gpio output low


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _gpio-gpioinpg:

:ref:`gpio intput_get<gpio-cli>` **:Get the GPIO input level**
----------------------------------------------------------------------------------------------

**Command** ::

    gpio intput_get <id>

**Params** ::
    
    id: GPIO pins to be operated on

**Usage Scenario** ::

    Input：gpio input_get 5
    
        Enter this command, and if the configuration is successful, return the input level of the GPIO.

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR  


.. _gpio-gpioint:

:ref:`gpio_int<gpio-cli>` **:Configure the GPIO interrupt type and enable or disable interrupts**
------------------------------------------------------------------------------------------------------

**Command** ::

    gpio_int <id> <inttype|start|stop> [mode]

**Params** ::
    
    id: GPIO pins to be operated on

    inttype:Set the interrupt type, combined with the mode parameter.

        start:Enable interruption
        
        stop: Disable interruption

    mode:Interrupt Trigger Mode

        low_level:Low-level trigger
       
        high_level:High-level trigger
       
        rising_edge:Rising edge trigger
       
        falling_edge:Falling edge trigger

**Usage Scenario** ::

    Input：
    
        gpio_int 5 inttype low_level：Enter this command to set the GPIO5 to trigger an interrupt at a low level.

    Input：
    
        gpio_int 5 start：Enter this command to enable interruption

    输入：
    
        gpio_int 5 stop：Input this command, disable interrupts.

.. note::

    *The GPIO that has already been activated for the second function must first deactivate the second function before proceeding with this setting.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR    

.. _gpio-gpiowake:

:ref:`gpio wake<gpio-cli>` **:Before entering the low-power mode, the corresponding GPIO is registered as a wake-up source.**
------------------------------------------------------------------------------------------------------------------------------------

**Command** ::

    gpio_wake <register|unregister|get_id> [id] [mode]

**Params** ::
    
    register:Register wake source.
    
    unregister:Unsubscribe from the registration wake source (combined with id and mode parameters)
    
    get_id: Query which gpio_id is currently waking up the system.

    id: GPIO pins to be operated on

    mode:Waking mode

        0:Low-level Wake-Up
        
        1:High-level wake-up
        
        2:Rising-edge wake-up.
        
        3:Falling-edge wake-up.

**Usage Scenario** ::

    Input：
    
        gpio_wake register 5 0:Enter this command to set GPIO5 as the wake-up source with a low-level wake-up.

    Input：
    
        gpio_wake unregister 5:Enter this command to cancel registering gpio5 as a wake-up source.

    Input：
    
        gpio_wake get_id: Enter this command to find out which gpio_id is currently waking up the system.

.. note::

    *This command requires enabling CONFIG_GPIO_WAKEUP_SUPPORT and CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT!*

    *To set the GPIO as a wake-up source for the MCU, it can be verified under the low-voltage mode.*
    

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR        


.. _gpio-gpiokpsta:

:ref:`gpio_kpsta<gpio-cli>` **:GPIO that needs to maintain its state under the low-voltage registration mode**
-------------------------------------------------------------------------------------------------------------------------

**Command** ::

    gpio_kpsta <register|unregister> <id> [io_mode|pull_mode|func_mode]

**Params** ::
    
    register:register requires GPIO pins to be kept.
    
    unregister:deregister requires GPIO pins to be kept.
    
    id: GPIO pins to be operated on

    io_mode:GPIO input/output enable
    
        0: High-resistance state
        
        1:Output Enable
        
        2:Input Enable
        
        3:Invalid

    pull_mode:Set GPIO pull-up and pull-down resistors.；
    
        0:disable；
        
        1:Enable by pull-down；
        
        2:Enable by pulling up
        
        3:Invalid


    func_mode:Enable the second function
    
        0:Disable
        
        1:Enable

**Usage Scenario** ::

    Input:
    
        gpio_wake register 5:Enter this command to register gpio5 to maintain the state in low-voltage mode.

    Output:
    
        gpio_wake unregister 5:Input this command to cancel the registration of gpio5 requiring to maintain the state under low-voltage mode.

.. note::

    *This command requires to enable CONFIG_GPIO_WAKEUP_SUPPORT!*

    *To set GPIO as a wake-up source for the MCU, it can be verified under the low-voltage mode*
    

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR   


.. _gpio-gpiodrv:

:ref:`gpio_driver<gpio-cli>` **:Used to initialize (de-initialize) resources related to GPIO**
-----------------------------------------------------------------------------------------------------------------------

**Command** ::

    gpio_driver [init/deinit]

**Params** ::
    
    init:initialize gpio

    deinit:deinitialize gpio


**Usage Scenario** ::

    Input:
    
        gpio_driver init
        
        Input this command, if the configuration is successful, it will return:

        gpio init
    
    Input:
    
        gpio_driver deinit
    
        Input this command, if the configuration is successful, it will return:
   
        gpio deinit

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR     
