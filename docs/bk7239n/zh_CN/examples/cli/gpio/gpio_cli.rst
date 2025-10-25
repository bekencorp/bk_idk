.. _gpio-cli:

**GPIO cli命令**
=======================

:link_to_translation:`en:[English]`


.. important::

    *与gpio相关的测试用例需要打开CONFIG_GPIO和CONFIG_DRIVER_TEST宏*

- :ref:`gpio set_config<gpio-gpioset>`          :用来配置gpio的相关状态

- :ref:`gpio output<gpio-gpiooutput>`           :用来配置gpio的输出状态

- :ref:`gpio output<gpio-gpioinput>`            :用来配置gpio的输入状态

- :ref:`gpio output_high<gpio-gpioouth>`        :用来配置gpio的输出电平

- :ref:`gpio input_get<gpio-gpioinpg>`          :用来获取gpio的输入电平

- :ref:`gpio_int<gpio-gpioint>`                 :用来配置gpio的中断类型以及开启关闭中断

- :ref:`gpio_wake<gpio-gpiowake>`               :在进入低压模式之前，用来注册对应的gpio为唤醒源

- :ref:`gpio_kpsta<gpio-gpiokpsta>`             :用来注册在低压模式下需要保持状态的gpio

- :ref:`gpio_driver<gpio-gpiodrv>`              :用来初始化（取消初始化）与gpio相关的资源


.. _gpio-gpioset:

:ref:`gpio set_config<gpio-cli>` **:用来配置gpio的相关状态**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio set_config <id> <io_mode> <pull_mode>

**参数** ::
    
    id: 想要操作的gpio引脚

    io_mode:gpio的输入输出使能；
    
            0:高组态；
            
            1:输出使能；
            
            2:输入使能；
            
            3:无效

    pull_mode:设置gpio上下拉电阻；
    
            0:disable；
            
            1:下拉使能；
            
            2:上拉使能；
            
            3:无效

**使用场景** ::
    
    输入：

        gpio set_config 5 2 1

    输出：

        gpio io(output/disable/input): 2 ,  pull(disable/down/up) : 1


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *此命令需要开启CONFIG_GPIO和CONFIG_DRIVER_TEST!*

    *已经开启第二功能的gpio必须先取消第二功能才能进行此设置*

.. _gpio-gpiooutput:

:ref:`gpio output<gpio-cli>` **:用来配置gpio的输出状态**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio output <id> <pull_up|pull_down>

**参数** ::
    
    id: 想要操作的gpio引脚

    pull_up/pull_down:上拉或下拉
    

**使用场景** ::
    
    输入：

        gpio output 5 pull_down

    输出：

        gpio output test: 5

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _gpio-gpioinput:

:ref:`gpio input<gpio-cli>` **:用来配置gpio的输出状态**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio input <id> <pull_up|pull_down>

**参数** ::
    
    id: 想要操作的gpio引脚

    pull_up/pull_down:上拉或下拉
    

**使用场景** ::
    
    输入：

        gpio input 5 pull_down

    输出：

        gpio input test: 5

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _gpio-gpioouth:

:ref:`gpio output_high<gpio-cli>` **:用来配置gpio的输出电平**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio <output_high|output_low> <id>

**参数** ::
    
    id: 想要操作的gpio引脚

    pull_up/pull_down:上拉或下拉

**使用场景** ::

    输入：gpio output_high 5

        输入这条命令，配置成功，则返回：
    
            $cli:I(42274):gpio output high
        
        如果在输入这条命令后，命令不被识别，则返回：
        
            $err:E(5890):cli_gpio_cmd 114: ret=-0x2007
            
            cli:I(5890):gpio output high

    输入：gpio output_low 5
        输入这条命令，配置成功，则返回：
        
            $cli:I(42274):gpio output low
        
        如果在输入这条命令后，命令不被识别，则返回：
        
            $err:E(5890):cli_gpio_cmd 114: ret=-0x2007
            
            cli:I(5890):gpio output low


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _gpio-gpioinpg:

:ref:`gpio intput_get<gpio-cli>` **:用来获取gpio的输入电平**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio intput_get <id>

**参数** ::
    
    id: 想要操作的gpio引脚

**使用场景** ::

    输入：gpio input_get 5
    
        输入这条命令，配置成功，则返回gpio的输入电平。

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR  


.. _gpio-gpioint:

:ref:`gpio_int<gpio-cli>` **:用来配置gpio的中断类型以及开启关闭中断**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio_int <id> <inttype|start|stop> [mode]

**参数** ::
    
    id: 想要操作的gpio引脚

    inttype:设置中断类型，和mode参数结合使用

        start:使能中断
        
        stop:失能中断

    mode:中断触发模式

        low_level:低电平触发
       
        high_level:高电平触发
       
        rising_edge:上升沿触发
       
        falling_edge:下降沿触发


**使用场景** ::

    输入：
    
        gpio_int 5 inttype low_level：输入这条命令，设置gpio5低电平触发中断。

    输入：
    
        gpio_int 5 start：输入这条命令，使能中断。

    输入：
    
        gpio_int 5 stop：输入这条命令，失能中断。

.. note::

    *已经开启第二功能的gpio必须先取消第二功能才能进行此设置*

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR    

.. _gpio-gpiowake:

:ref:`gpio wake<gpio-cli>` **:在进入低压模式之前，用来注册对应的gpio为唤醒源**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio_wake <register|unregister|get_id> [id] [mode]

**参数** ::
    
    register:注册唤醒源
    
    unregister:取消注册唤醒源（与id和mode参数结合使用）
    
    get_id: 获取当前哪个gpio_id唤醒系统

    id:想要操作的gpio引脚

    mode:唤醒模式

        0:低电平唤醒
        
        1:高电平唤醒
        
        2:上升沿唤醒
        
        3:下降沿唤醒

**使用场景** ::

    输入：
    
        gpio_wake register 5 0:输入这条命令，设置gpio5为唤醒源，低电平唤醒。
        
    输入：
    
        gpio_wake unregister 5:输入这条命令，取消注册gpio5为唤醒源。

    输入：
    
        gpio_wake get_id:输入这条命令，获取当前是哪个gpio_id唤醒系统。

.. note::

    *此命令需要开启CONFIG_GPIO_WAKEUP_SUPPORT和CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT!*

    *要向MCU设置GPIO作为唤醒源，低压模式下可验证*
    

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR        


.. _gpio-gpiokpsta:

:ref:`gpio_kpsta<gpio-cli>` **:用来注册在低压模式下需要保持状态的gpio**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio_kpsta <register|unregister> <id> [io_mode|pull_mode|func_mode]

**参数** ::
    
    register:注册需要保持的gpio引脚
    
    unregister:取消注册需要保持的gpio引脚
    
    id:想要操作的gpio引脚

    io_mode:gpio的输入输出使能

        0:高组态；
        
        1:输出使能；
        
        2:输入使能；
        
        3:无效;

    pull_mode:设置gpio上下拉电阻

        0:disable；
        
        1:下拉使能；
        
        2:上拉使能；
        
        3:无效;

    func_mode:使能第二功能；
    
        0:失能；
        
        1:使能

**使用场景** ::

    输入：
    
        gpio_wake register 5:输入这条命令，注册gpio5在低压模式下需要保持状态。

    输入：
    
        gpio_wake unregister 5:输入这条命令，取消注册gpio5在低压模式下需要保持状态。


.. note::

    *此命令需要开启CONFIG_GPIO_WAKEUP_SUPPORT!*

    *要向MCU设置GPIO作为唤醒源，低压模式下可验证*
    

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR   


.. _gpio-gpiodrv:

:ref:`gpio_driver<gpio-cli>` **:用来初始化（取消初始化）与gpio相关的资源**
----------------------------------------------------------------------------------------------

**命令** ::

    gpio_driver [init/deinit]

**参数** ::
    
    init:初始化gpio

    deinit:取消gpio初始化


**使用场景** ::

    输入：
    
        gpio_driver init
        
        输入这条命令，配置成功，则返回：
    
        gpio init
    
    输入：
    
        gpio_driver deinit
    
        输入这条命令，配置成功，则返回：
    
        gpio deinit

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR     
