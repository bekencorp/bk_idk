.. _plt-cli:

**Platform cli Command**
==============================

:link_to_translation:`zh_CN:[zh_CN]` 


.. important::

    *All functions of this interface are for the use of users who operate the platform via CLI commands*

- :ref:`backtrace<plt-backtrace>`              :Display task trace information

- :ref:`cpuload<plt-load>`                     :Display the load status and running time of each task on the current CPU

- :ref:`memdump<plt-memdump>`                  :Perform a dump operation on a specific length of memory.

- :ref:`memleak<plt-memleak>`                  :Statistics of memory heap allocation status

- :ref:`memshow<plt-memshow>`                  :Display the usage status of the current memory

- :ref:`memstack<plt-memstack>`                :Display the execution of stack usage on memory

- :ref:`reboot<plt-reboot>`                    :Reboot
 
- :ref:`tasklist <plt-taskl>`                  :Print the current task status list

- :ref:`flash <plt-flash>`                     :Perform operations related to flash.

.. _plt-backtrace:

:ref:`backtrace<plt-cli>` **:Display task trace information**
------------------------------------------------------------------

**Command** ::

    backtrace

**Params** ::
    
    None

**Usage Scenario** ::
    
    Input：

        backtrace
    
    Output：

        >>>>dump task backtrace begin.
        "task", "stack_addr", "top", "size", "overflow", "backtrace"
        <<<<dump task backtrace end
   

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-load:

:ref:`cpuload<plt-cli>`  **:Display the load status and running time of each task on the current CPU**
---------------------------------------------------------------------------------------------------------

**Command** ::

    cpuload

**Params** ::
    
    None

**Usage Scenario** ::
    
    Input：

        cpuload
    
    Output(example)：

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


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect under the condition that CONFIG_FREERTOS is enabled!*

.. _plt-memdump:

:ref:`memdump<plt-cli>` **:Perform a dump operation on a specific length of memory**
----------------------------------------------------------------------------------------------

**Command** ::

    memdump <addr> <length>

**Params** ::
    
    addr: the starting address of the memory to be dumped

    length:The length of the memory to be dumped


**Usage Scenario** ::

    Input：
    
        memdump 2806cd00 0x100

    Enter this command, and it will dump the current data in memory by bytes, and the return is:

        dump,address: 0x2806CD00 size: 0x00000100
        
        ……(Show dump DATA)

.. note::

    *This command requires enabling CONFIG_DEBUG_VERSION!*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR     

.. _plt-memleak:

:ref:`memleak<plt-cli>` **:Statistics of memory heap allocation status**
----------------------------------------------------------------------------------------------

**Command** ::

    memleak

**Params** ::
    
    None

**Usage Scenario** ::

    Input：
    
        memleak

    Enter this command to return the information about the heap locations applied under each task in the current system,
    including the number of ticks experienced, the size of the heap space applied, the command of the function applied, 
    and the position called by the malloc function, etc.

.. note::

    *This command requires enabling CONFIG_MEM_DEBUG and CONFIG_FREERTOS options!*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR         

.. _plt-memshow:

:ref:`memshow<plt-cli>` **:Display the usage status of the current memory**
----------------------------------------------------------------------------------------------

**Command** ::

    memshow 

**Params** ::
    
    None

**Usage Scenario** ::

    Input：
    
        memshow 

    Enter this command, it will return:
    
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

    Under the condition of enabling CONFIG_PSRAM_AS_SYS_MEMORY, memory status on PSRAM can also be displayed; it is shown under Dynamic memory,
    for example, as follows:

        ================Dynamic memory================
        name    total   free    minimum   peak 
        heap	391552	345248	332472	59080
        psram	3145728	3136968	3136848	8880

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-memstack:

:ref:`memstack<plt-cli>` **:Display the execution of stack usage on memory**
----------------------------------------------------------------------------------------------

**Command** ::

    memstack 

**Params** ::
    
    None

**Usage Scenario** ::

    Input：
    
        memstack 

    Enter this command, it will return：
    
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

    *This command requires the feature under CONFIG_FREERTOS to be enabled!*


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-reboot:

:ref:`reboot<plt-cli>` **:Reboot**
----------------------------------------------------------------------------------------------

**Command** ::

    reboot 

**Params** ::
    
    None

**Usage Scenario** ::

    Input：
    
        reboot 

    Enter this command to initiate the board's reboot procedure.

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _plt-taskl:

:ref:`tasklist<plt-cli>` **:Print the current task status list**
----------------------------------------------------------------------------------------------

**Command** ::

    tasklist

**Params** ::
    
    None

**Usage Scenario** ::

    Input:
    
       tasklist

    Output：

        Return the current task status

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

    *Under the setting CONFIG_FREERTOS_SMP, the printing will add a column called "affinity"*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR    

.. _plt-flash:

:ref:`flash<plt-cli>` **:Perform operations related to flash**
----------------------------------------------------------------------------------------------

**Command** ::

    flash {erase|read|write} [start_addr] [len]

**Params** ::
    
    erase：Erase flash data at the specified address and length.

    read:  Read flash data at the specified address and length.

    write: Write flash data to the specified address and length.

    start_addr:The starting address of the flash that you want to operate on.

    len:The desired data length to be operated on


**Usage Scenario** ::

    Input：
    
        flash  read  0xa5a5a5a5 0x1000

    Enter this command, if the configuration is successful, it will return:

        CMDRSP:OK

    If the command is not recognized after entering this command, return:
        
        CMDRSP:ERROR


.. note::

    *Effective when both CONFIG_FLASH and CONFIG_FLASH_TEST are enabled simultaneously.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR        