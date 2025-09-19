.. _plt-cli:

**Platform cli命令**
=======================

:link_to_translation:`en:[English]`


.. important::

    *本界面所有功能面向使用cli命令操作平台的用户使用*

- :ref:`backtrace<plt-backtrace>`              :显示任务回溯信息

- :ref:`cpuload<plt-load>`                     :用来展示当前CPU上每个task的负载情况和已运行时间等情况

- :ref:`memdump<plt-memdump>`                  :执行对内存上某段长度的dump操作

- :ref:`memleak<plt-memleak>`                  :执行对内存heap申请情况的统计

- :ref:`memshow<plt-memshow>`                  :执行对当前内存所使用情况的显示

- :ref:`memstack<plt-memstack>`                :执行对内存上stack使用情况的显示

- :ref:`reboot<plt-reboot>`                    :执行重启

- :ref:`tasklist <plt-taskl>`                 :打印当前的task状态列表

- :ref:`flash <plt-flash>`                     :执行对flash的相关操作

.. _plt-backtrace:

:ref:`backtrace<plt-cli>` **:显示任务回溯信息**
------------------------------------------------------------------

**命令** ::

    backtrace

**参数** ::
    
    无

**使用场景** ::
    
    输入：

        backtrace
    
    显示：

        >>>>dump task backtrace begin.
        "task", "stack_addr", "top", "size", "overflow", "backtrace"
        <<<<dump task backtrace end
   

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-load:

:ref:`cpuload<plt-cli>`  **:用来展示当前CPU上每个task的负载情况和已运行时间等情况**
-------------------------------------------------------------------------------------

**命令** ::

    cpuload

**参数** ::
    
    无

**使用场景** ::
    
    输入：

        cpuload
    
    显示(仅举例)：

        >>>>dump task runtime begin.
        shell_handle   	0		   <1%
        IDLE           	356432	   99%
        Tmr Svc        	0		   <1%
        wpas_thread    	606		   <1%
        tcp/ip         	0		   <1%
        atsvr-handler  	0		   <1%
        event          	0		   <1%
        kmsgbk          2		   <1%
        dhcp-server    	0		   <1%
        tempd           2		   <1%
        ble            	2		   <1%
        core_thread    	131		   <1%
        cli            	0		   <1%
        syswq          	0		   <1%
        <<<<dump task runtime end.


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *本命令在CONFIG_FREERTOS开启下生效!*

.. _plt-memdump:

:ref:`memdump<plt-cli>` **:执行对内存上某段长度的dump操作**
----------------------------------------------------------------------------------------------

**命令** ::

    memdump <addr> <length>

**参数** ::
    
    addr:想要dump的内存的起始地址

    length:想要dump的内存的长度


**使用场景** ::

    输入：
    
        memdump 2806cd00 0x100

    输入这条命令，则按字节dump当前内存中的数据，则返回：

        dump,address: 0x2806CD00 size: 0x00000100
        
        ……(随后显示dump数据)

.. note::

    *此命令需要开启CONFIG_DEBUG_VERSION!*

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR     

.. _plt-memleak:

:ref:`memleak<plt-cli>` **:执行对内存heap申请情况的统计**
----------------------------------------------------------------------------------------------

**命令** ::

    memleak

**参数** ::
    
    无

**使用场景** ::

    输入：
    
        memleak

    输入这条命令，则返回当前系统中每个task下申请的heap的位置，所经历的tick数，所申请的heap空间大小，所申请函数命令及其malloc函数所调用的位置等信息。

.. note::

    *此命令需要开启CONFIG_MEM_DEBUG和CONFIG_FREERTOS下使用!*

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR         

.. _plt-memshow:

:ref:`memshow<plt-cli>` **:执行对当前内存所使用情况的显示**
----------------------------------------------------------------------------------------------

**命令** ::

    memshow 

**参数** ::
    
    无

**使用场景** ::

    输入：
    
        memshow 

    输入这条命令，则返回：
    
        ================Static memory================
        mem_type start    end      size    
        -------- -------- -------- --------
        itcm     0x0      0x3e00   15872   
        dtcm     0x20000000 0x20001bd8 7128    
        ram      0x28037400 0x2809eff8 424952  
        non_heap 0x28037400 0x2806aec0 211648  
        iram     0x8000000 0x8037258 225880  
        data     0x28037400 0x280389e8 5608    
        bss      0x2803d5c0 0x2806aebc 186620  
        heap     0x2806aec0 0x2809eff8 213304  
        ================Dynamic memory================
        name    total   free    minimum   peak 
        heap	213304	161032	154520	58784

    在开启CONFIG_PSRAM_AS_SYS_MEMORY下，还可以显示psram上的内存情况；其显示在Dynamic memory下，例如如下情况：

        ================Dynamic memory================
        name    total   free    minimum   peak 
        heap	391552	345248	332472	59080
        psram	3145728	3136968	3136848	8880

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-memstack:

:ref:`memstack<plt-cli>` **:执行对当前内存所使用情况的显示**
----------------------------------------------------------------------------------------------

**命令** ::

    memstack 

**参数** ::
    
    无

**使用场景** ::

    输入：
    
        memstack 

    输入这条命令，则返回：
    
    task            stack_size              address            peak_used    current_used    water      
    shell_handle       6144      	 0x2806b5b0- 0x2806cdb0	      412       	80          5732       

    IDLE               1536      	 0x200003dc- 0x200009dc	      232       	148         1304       

    Tmr Svc            3072      	 0x20000a44- 0x20001644	      344       	140         2728
        
    tcp/ip             2048      	 0x2806e2d0- 0x2806ead0	      200       	200         1848       

    kmsgbk             4096          0x28071670- 0x28072670	      168       	168         3928    
    
    core_thread        2048      	 0x28072da0- 0x280735a0	      200       	200         1848       

    wpas_thread        5120          0x28073ad0- 0x28074ed0	      240       	240         4880       

    atsvr-handler      3072      	 0x28078640- 0x28079240       184       	184         2888       

    event              2048      	 0x2806d788- 0x2806df88	      292       	216         1756       

    tempd              1536          0x28070ba8- 0x280711a8	      900       	328          636        

    ble                3072      	 0x280755a0- 0x280761a0	      844       	336         2228       

    cli                3072      	 0x28077898- 0x28078498	      948       	416         2124       

    syswq              1024      	 0x2806ecf0- 0x2806f0f0	      176       	176          848        

    thread_stack:37888, used:35056, the rest:2832
    arm_mode_stack:0, used:0, the rest:0
    total_stack_space:37888, used:35056, the rest:2832

.. note::

    *此命令需要开启CONFIG_FREERTOS下使用!*


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-reboot:

:ref:`reboot<plt-cli>` **:执行重启**
----------------------------------------------------------------------------------------------

**命令** ::

    reboot 

**参数** ::
    
    无

**使用场景** ::

    输入：
    
        reboot 

    输入这条命令，则执行板子重启流程

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-taskl:

:ref:`tasklist<plt-cli>` **:打印当前的task状态列表**
----------------------------------------------------------------------------------------------

**命令** ::

    tasklist

**参数** ::
    
    无

**使用场景** ::

    输入:
    
       tasklist

    返回：

        返回当前task状态

        >>>>dump task list begin.
        task           state   pri     water   no   
        shell_handle   	X	5	5804	23
        IDLE           	R	0	1304	3
        wpas_thread    	B	4	3168	11
        Tmr Svc        	B	5	2728	4
        tcp/ip         	B	7	1092	6
        atsvr-handler  	B	3	2888	16
        event          	B	1	1076	5
        kmsgbk         	B	6	3372	9
        ble            	B	7	2184	12
        tempd          	B	5	636	8
        core_thread    	B	7	1044	10
        cli            	B	5	2124	15
        syswq          	B	6	848	7
        <<<<dump task list end.

.. note::

    *开启CONFIG_FREERTOS_SMP下，打印将增加一列”affinity”*

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR    

.. _plt-flash:

:ref:`flash<plt-cli>` **:执行对flash的相关操作**
----------------------------------------------------------------------------------------------

**命令** ::

    flash {erase|read|write} [start_addr] [len]

**参数** ::
    
    erase：擦除指定地址和长度的flash数据

    read:  读取指定地址和长度的flash数据

    write:  写入指定地址和长度的flash数据

    start_addr:想要操作的flash起始地址

    len:想要操作的数据长度


**使用场景** ::

    输入：
    
        flash  read  0xa5a5a5a5 0x1000

    输入这条命令，配置成功，则返回：

        CMDRSP:OK

    如果在输入这条命令后，命令不被识别，则返回：
        
        CMDRSP:ERROR


.. note::

    *在CONFIG_FLASH和CONFIG_FLASH_TEST同时开启下生效*

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR        