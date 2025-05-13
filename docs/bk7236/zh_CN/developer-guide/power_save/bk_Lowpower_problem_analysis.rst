低功耗问题分析
=======================================================

一、低压问题分析指导
--------------------------------------------------------------

1.进低压失败
##############################################################

如果进低压失败，首先确认是否满足进入低压的条件：

- 1. 业务线程可能有类似while(1)的操作,业务线程一直在运行，导致系统无法进入IDLE task。

该问题debug方法：

  a)  异常后，通过串口发送测试命令：pm_debug 1
  观察是否有以下log打印：

   ::

    “lowvol1 0x3 0xFFFFEFFF 0xFFFFFFFF”
    “lowvol2 0x0 0x0”

  如果没有以上打印，则有可能是这个问题。

  b) 以上问题的解决方案：
  线程运行不能一直while(1)进行运行或者等待，应该使用信号量或者事件标识，没有业务工作时，线程进入sleep。

- 2. 相应的模块没有投sleep的票，导致无法进入低压。

该问题debug方法：

  a) 异常后，通过串口发送测试命令：pm_debug 1
  有以下log打印：

   ::

    “lowvol1 0x3 0xFFFFEFFF  0xFFFFFFFF”
    “lowvol2 0x0 0x0”

   当前投的票的值：0xFFFFEFFF(bit位为1，表示该位的票投上了票)
   需要满足的票值：0xFFFFFFFF

  从以上两个值对比来看，bit12的值为0，表示该位置的票没有投上，
  查看pm_sleep_module_name_e枚举类型，值为12的票
  为PM_SLEEP_MODULE_NAME_APP，则表示该APP票没有投上。

  b) 以上问题的解决方案：
  需要在业务线程处理完响应的工作后，调用以下接口把票投上：

   ::

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x1);
    typedef enum
    {
        PM_SLEEP_MODULE_NAME_I2C1 = 0, // 0
        PM_SLEEP_MODULE_NAME_SPI_1,    // 1
        PM_SLEEP_MODULE_NAME_UART1,    // 2
        PM_SLEEP_MODULE_NAME_PWM_1,    // 3
        PM_SLEEP_MODULE_NAME_TIMER_1,  // 4
        PM_SLEEP_MODULE_NAME_SARADC,   // 5
        PM_SLEEP_MODULE_NAME_AUDP,     // 6
        PM_SLEEP_MODULE_NAME_VIDP,     // 7
        PM_SLEEP_MODULE_NAME_BTSP,     // 8
        PM_SLEEP_MODULE_NAME_WIFIP_MAC,// 9
        PM_SLEEP_MODULE_NAME_WIFI_PHY, // 10
        PM_SLEEP_MODULE_NAME_TIMER_2,  // 11
        PM_SLEEP_MODULE_NAME_APP,      // 12
        PM_SLEEP_MODULE_NAME_OTP,      // 13
        PM_SLEEP_MODULE_NAME_I2S_1,    // 14
        PM_SLEEP_MODULE_NAME_USB_1,    // 15
        PM_SLEEP_MODULE_NAME_CAN,      // 16
        PM_SLEEP_MODULE_NAME_PSRAM,    // 17
        PM_SLEEP_MODULE_NAME_QSPI_1,   // 18
        PM_SLEEP_MODULE_NAME_QSPI_2,   // 19
        PM_SLEEP_MODULE_NAME_SDIO,     // 20
        PM_SLEEP_MODULE_NAME_AUXS,     // 21
        PM_SLEEP_MODULE_NAME_CPU,      // 22
        PM_SLEEP_MODULE_NAME_AT,       // 23
        PM_SLEEP_MODULE_NAME_I2C2,     // 24
        PM_SLEEP_MODULE_NAME_UART2,    // 25
        PM_SLEEP_MODULE_NAME_UART3,    // 26
        PM_SLEEP_MODULE_NAME_WDG,      // 27
        PM_SLEEP_MODULE_NAME_TIMER_3,  // 28
        PM_SLEEP_MODULE_NAME_APP1,     // 29
        PM_SLEEP_MODULE_NAME_APP2,     // 30
        PM_SLEEP_MODULE_NAME_APP3,     // 31

        PM_SLEEP_MODULE_NAME_MAX
    }pm_sleep_module_name_e;

2.低压唤醒后系统自动复位
##############################################################

低压唤醒后系统自动复位，可能的原因如下：

- 1. wdt复位:低压唤醒后，有中断没有清除掉，导致中断不断的上报，WDT无法喂狗导致

该问题debug方法：

  a)  该类问题，一般会进行dump，通过查看LR的地址，地址是否在pm.c出开中断后或者interrupts.c文件中的中断函数中，如果在，一般是这类问题。
  在接收中断的函数中通过逻辑分析仪拉GPIO信号或者打印log，确定哪个中断，再结合代码和流程找出问题。

  b) 以上问题的解决方案：
  中断响应完后，需要把中断清除，防止中断不断往上报。


二、深度睡眠问题分析指导
--------------------------------------------------------------

1.进深度睡眠失败
##############################################################

如果进深度睡眠失败，首先确认是否满足进入深度睡眠的条件：

- 1.  业务线程可能有类似while(1)的操作,业务线程一直在运行，导致系统无法进入IDLE task。

该问题debug方法：

  a)  异常后，通过串口发送测试命令：pm_debug 2

观察是否有以下log打印：

   ::

    deepsleep1 0x3C4 0x3C0
    deepsleep2 0x0 0x0

如果没有以上打印，则有可能是这个问题。

  b)  以上问题的解决方案：

线程运行不能一直while(1)进行运行或者等待，应该使用信号量或者事件标识，没有业务工作时，线程进入sleep。

- 2.  没有满足了进入深度睡眠的票（BT和WIFI进入sleep并下电,多媒体关闭并下电）

.. important::
    1)BT和WIFI进入SLEEP并下电，是有SDK内部代码完成，客户或者应用层代码不需要调用关闭和下电的接口
    2)进入deepsleep的前提之一是多媒体(audio,video,lcd需要关闭和下电，需要调用他们模块的接口)

该问题debug方法：

  a)异常后，通过串口发送测试命令：pm_debug 1

有以下log打印：

   :: 

    deepsleep1 0x300 0x3C0
    deepsleep2 0x0 0x0

   当前投的票的值：0x300 (bit位为1，表示该位的票投上了票)
   需要满足的票值：0x3C0

从以上两个值对比来看，bit6和bit7的值为0，表示该位置的票没有投上，
查看PM_ENTER_DEEP_SLEEP_MODULES_CONFIG定义的值，bit6为AUDIO,
Bit7为VIDEO，则表示以上两个模块的票没有投上。

   :: 

    #define PM_ENTER_DEEP_SLEEP_MODULES_CONFIG \
    {\
        PM_POWER_MODULE_NAME_AUDP,\      /*6*/
        PM_POWER_MODULE_NAME_VIDP,\      /*7*/
        PM_POWER_MODULE_NAME_BTSP,\      /*8*/
        PM_POWER_MODULE_NAME_WIFIP_MAC,\ /*9*/
    }

  b)以上问题的解决方案：
  Audio,video模块需要在进deepsleep之前，调用各自模块的接口关闭audio,video。


三、获取低功耗相关调试信息
--------------------------------------------------------------

    通过串口发送测试命令：pm_debug 8，获取低功耗相关调试信息（模块未使用则不输出相关调试信息）

   :: 

    1)pm video and audio state:0x0 0x0
    2)pm ahpb and bakp state:0x0 0x10001
    3)pm low vol[module:0xffbfffff] [need module:0xffffffff]
    4)pm deepsleep[module:0x5e0][need module:0x3c0]
    5)pm power and pmu state[0x202a25e8][0x7d000271]
    6)Attention the bakp not power down[modulue:0x10001]
    7)pm_psram_ctrl_state:0x0 0x0
    8)pm_cp1_ctrl_state:0x0
    9)pm_cp1_boot_ready:0x0
    10)pm_module_lv_sleep_state:0xffffffffffffffff

  - pm video and audio state:0x0 0x0 ： 记录video和audio电源域投票状态
  - pm ahpb and bakp state:0x0 0x10001 : 记录ahpb和bakp电源域投票状态
  - pm low vol[module:0xffbfffff] [need module:0xffffffff]： 记录进入低压需要投票的模块和已经投票的模块
  - pm deepsleep[module:0x5e0][need module:0x3c0] ：记录进入deepsleep需要投票的模块和已经投票的模块
  - pm power and pmu state[0x202a25e8][0x7d000271] ：输出所有电源域的状态和lpo时钟源选择状态
  - Attention the bakp not power down[modulue:0x10001] ：输出bakp电源域下的模块状态
  - pm_psram_ctrl_state:0x0 0x0 ：记录使用psram的子模块和psram作为内存使用的初始化完成标志
  - pm_cp1_ctrl_state:0x0 ： 记录启动cpu1的子模块
  - pm_cp1_boot_ready:0x0 ： 记录cpu1的启动状态
  - pm_module_lv_sleep_state:0xffffffffffffffff ： 记录退出低压时需要恢复的模块状态

  - 1)pm video and audio state:0x0 0x0
      
      记录video和audio电源域电源域投票模块状态。

      -  第一个0x0记录video投票上下电情况。如果值是0x0表示投的票都下电了;如果值是0x1表示bit0所对应的模块JPEG_EN模块没有投下电的票,各个bit对应的模块。 

        :: 

         typedef enum
         {
         POWER_SUB_MODULE_NAME_VIDP_JPEG_EN = POWER_MODULE_NAME_VIDP*PM_MODULE_SUB_POWER_DOMAIN_MAX, //0
         POWER_SUB_MODULE_NAME_VIDP_JPEG_DE , //1
         POWER_SUB_MODULE_NAME_VIDP_DMA2D ,   //2
         POWER_SUB_MODULE_NAME_VIDP_LCD ,     //3
         POWER_SUB_MODULE_NAME_VIDP_YUVBUF ,  //4
         POWER_SUB_MODULE_NAME_VIDP_ROTT ,    //5
         POWER_SUB_MODULE_NAME_VIDP_SCAL0 ,   //6
         POWER_SUB_MODULE_NAME_VIDP_SCAL1 ,   //7
         POWER_SUB_MODULE_NAME_VIDP_H264 ,    //8
         POWER_SUB_MODULE_NAME_VIDP_NONE
         }power_sub_module_name_vidp_e;

      - 第二个0x0记录audio投票上下电情况。如果值是0x0表示投的票都下电了；如果值是0x1表示bit0所对应的模块FFT模块没有投下电的票,各个bit对应的模块。

       :: 

        typedef enum
        {
        POWER_SUB_MODULE_NAME_AUDP_FFT  = POWER_MODULE_NAME_AUDP*PM_MODULE_SUB_POWER_DOMAIN_MAX , //0
        POWER_SUB_MODULE_NAME_AUDP_SBC ,  //1
        POWER_SUB_MODULE_NAME_AUDP_AUDIO ,//2   
        POWER_SUB_MODULE_NAME_AUDP_I2S ,  //3
        POWER_SUB_MODULE_NAME_AUDP_NONE
        }power_sub_module_name_audp_e;

  - 2)pm ahpb and bakp state:0x0 0x10001
    记录ahpb和bakp电源域上下电投票状态。

   备注：ahpb和bakp比较特殊，由SDK底层进行管理，一般使用场景，应用程序不需要关注。进入低压时会备份相关寄存器，出低压时会恢复相关寄存器。


 - 3)pm low vol[module:0xffbfffff] [need module:0xffffffff]

  记录进入低压需要投票的模块和已经投票的模块
  当系统无法进入低压时，确认该项：
  need module:0xffffffff  表示进入低压需要32张票都投1，则表示都进入睡眠状态。
  module:0xffbfffff  其中为1的bit位，表示已经进入了睡眠的模块；其中为0的bit位，表示还没有进入睡眠的模块。
  各个bit为对应的模块，请参考pm_sleep_module_name_e中定义的票。

 - 4)pm deepsleep[module:0x5e0][need module:0x3c0]

  记录进入deepsleep需要投票的模块和已经投票的模块
  need module:0x3c0 表示进入深度睡眠，以下模块需要关闭状态。值为1表示该模块已经关闭了。
  当前进入deepsleep，需要投以下票后，才能进入deepsleep.

  module:0x5e0 表示当前投票进入deepsleep的模块。
  其中bit6,bit7为1了，则表示AUDP,VIDP模块已经关闭了。
  Bit8为1了，则表示BTSP模块进入睡眠了。
  Bit9为0， 则表示WIFI_MAC还没有进入睡眠。
  以上值不满足进入deepsleep的条件。

   :: 

        #define PM_ENTER_DEEP_SLEEP_MODULES_CONFIG \
        {\
        PM_POWER_MODULE_NAME_BTSP,\       // bit8
        PM_POWER_MODULE_NAME_WIFIP_MAC,\  //bit9
        PM_POWER_MODULE_NAME_AUDP,\       //bit6
        PM_POWER_MODULE_NAME_VIDP,\       //bit7
        }


 - 5)pm power and pmu state[0x202a25e8][0x7d000272]

  输出所有电源域的状态和lpo时钟源选择状态。
  红色第一项表示电源关闭状态,关注bit[0]到bit[15],各个bit位代表的含义如下图。
  其中值为1, 表示该电源域关闭，值为0，表示该电源域开启。

  例如:
  a)0x202a25e8(进入低功耗预期以下模块都关闭了)

  其中8:表示pwd_encp模块电源关闭了。
  e:表示pwd_vidp,pwd,audp,pwd_ ahbp模块的电源关闭了
  5:表示pwd_wifip_phy,pwd,btsp关闭了
  2:表示pwd_ofdm关闭了。

  以下为每bit对应的电源(0:表示开; 1:表示关闭)：
  :: 

        bit0:mem1
        bit1:mem2
        bit2:mem3
        bit3:encp
        bit4:bakp
        bit5:ahbp
        bit6:audp
        bit7:vidp
        bit8:btsp
        bit9:wifi_mac
        bit10:wifi_phy
        bit11:mem0
        bit12:mem4
        bit13:ofdm
        bit14:mem5
        bit15:rom

  b)0x7d000272
  以上值的最后2bit表示32K时钟源,
  0:表示26M时钟分下来的32k; 1:表示外部32k时钟; 2:表示内部的ROSC时钟

    .. note::

         进入低压的预期值为：pm power and pmu state[0x202a25e8][0x7d000272]
         默认32K时钟源为ROSC

 - 6)Attention the bakp not power down[modulue:0x10001]

  输出bakp电源域下的模块状态。
  Bakp模块比较特殊，底层SDK做好适配。当前的方案是进入低压前备份相关的寄存器的值，低压唤醒后恢复备份的值。

 - 7)pm_psram_ctrl_state:0x0 0x0

      记录使用psram的子模块和psram作为内存使用的初始化完成标志。
      a)第一个红色参数表示投票开启PSRAM的模块是否都关闭.
      0x0：表示投票开启PSRAM的模块都关闭了
      如果非0，例如0x4，则表示bit2对应的模块（PM_POWER_PSRAM_MODULE_NAME_AUDP_AUDIO），
      没有投关闭PSRAM的票。

   :: 

        typedef enum
        {
        PM_POWER_PSRAM_MODULE_NAME_FFT       = 0,
        PM_POWER_PSRAM_MODULE_NAME_AUDP_SBC     ,// 1
        PM_POWER_PSRAM_MODULE_NAME_AUDP_AUDIO   ,// 2
        PM_POWER_PSRAM_MODULE_NAME_AUDP_I2S     ,// 3
        PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN ,// 4
        PM_POWER_PSRAM_MODULE_NAME_VIDP_H264_EN ,// 5
        PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_DE ,// 6
        PM_POWER_PSRAM_MODULE_NAME_VIDP_DMA2D   ,// 7
        PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD     ,// 8
        PM_POWER_PSRAM_MODULE_NAME_APP          ,// 9
        PM_POWER_PSRAM_MODULE_NAME_AS_MEM       ,// 10
        PM_POWER_PSRAM_MODULE_NAME_CPU1         ,// 11
        PM_POWER_PSRAM_MODULE_NAME_MEDIA        ,// 12
        PM_POWER_PSRAM_MODULE_NAME_LVGL_CODE_RUN,// 13
        PM_POWER_PSRAM_MODULE_NAME_MAX          ,// attention: MAX value can not exceed 31.
        }pm_power_psram_module_name_e;


  b)当应用程序没有把malloc psram作为SRAM用，进入电压前没有是否，pm_debug 8命令后，会打印以下log，并会告诉哪个地方没有释放：
  ”Attention the CPU1 psram malloc count”

  c)如果PSRAM不关闭，能够进入低压，但是低压的底电流会变高。

 - 8)pm_cp1_ctrl_state:0x0

  记录启动cpu1的子模块。
  当值为0x0时，表示投CPU1启动的模块，都关闭了。

  当值不为0x0时，表示投CPU1启动的模块，还有没有关闭的。
  例如当值为0x4时，表示PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO模块没有投关闭CPU1的票。

  各个bit 位对应的模块，如下:

   :: 

        typedef enum
        {
        PM_BOOT_CP1_MODULE_NAME_FFT          = 0,
        PM_BOOT_CP1_MODULE_NAME_AUDP_SBC        ,// 1
        PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO      ,// 2
        PM_BOOT_CP1_MODULE_NAME_AUDP_I2S        ,// 3
        PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN    ,// 4
        PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_DE    ,// 5
        PM_BOOT_CP1_MODULE_NAME_VIDP_DMA2D      ,// 6
        PM_BOOT_CP1_MODULE_NAME_VIDP_LCD        ,// 7
        PM_BOOT_CP1_MODULE_NAME_MULTIMEDIA      ,// 8
        PM_BOOT_CP1_MODULE_NAME_APP             ,// 9
        PM_BOOT_CP1_MODULE_NAME_VIDP_ROTATE     ,// 10
        PM_BOOT_CP1_MODULE_NAME_VIDP_SCALE      ,// 11
        PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG   ,// 12
        PM_BOOT_CP1_MODULE_NAME_LVGL            ,// 13
        PM_BOOT_CP1_MODULE_NAME_MAX             ,// attention: MAX value can not exceed 31.
        }pm_boot_cp1_module_name_e;



 - 9)pm_cp1_boot_ready:0x0

     记录cpu1的启动状态。
     确认CPU1是否关闭。
     当值为0x0时，表示CPU1已经关闭了
     当值为0x1时，表示CPU1启动了，还没有关闭。

 - 10)pm_module_lv_sleep_state:0xffffffffffffffff
      记录退出低压时需要恢复的模块状态。
      该state主要是给SDK内部的bakp模块使用。

四、低功耗问题问答
--------------------------------------------------------------

 - 1.进入深度睡眠后功耗偏高问题确认
     - a.进入深度睡眠成功后，VDDDIG的电压为0v.电流在20uA左右.
     - b.如果VDDDIG的电压为0v, 电流超过了25Ua则需要确认是否外围电路漏电。

 - 2.进入低压后功耗偏高问题确认
     - a.通过pm_debug 8确认该关闭的模块都关闭了，功耗还偏高，则确认外围电路是否由漏电。通过万用表确认各个GPIO是否有高电平。
     - b.确认测试环境，温度高低压功耗也会变高。
     - c.WIFI保活功耗高，则首先确认底电流是否符合预期，然后确认WIFI开窗时长是否符合预期，WIFI接收电流是否符合预期。WIFI发包周期是否频繁，DTIM的值是否符合预期。

:link_to_translation:`en:[English]`

