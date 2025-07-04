低功耗问题分析Checklist
==================================================

-  进入低压(sleep mode)或者deep sleep失败分析步骤

-----------------------------------------------------

   - 1.异常后，发送pm_debug 8命令抓取log,根据前面log分析步骤 :doc:`bk_Lowpower_problem_analysis` ，分析哪几个模块没有投票等,如果能快速找出问题，则解决。
       
       备注：如果不好发pm_debug 8命令，可以在进入低压失败的时候，调用以下函数，抓取log:
         - 1) pm_debug_ctrl(8);
         - 2) pm_debug_pwr_clk_state();
         - 3) pm_debug_lv_state();

   - 2.如果不好找出问题，则可以先下载原版SDK，通过发命令的方式进入低压睡眠和deepsleep，确认是否成功。如果是失败的，则可以提问题单，让我们分析确认。

   - 3.如果成功，那么失败的原因可能是自己添加代码导致的，可以采用一点一点添加代码的方式确认，哪块有问题。

   - 4.确认进入低压或者deepsleep前，应用层代码结束了业务，再进入低压或者deepsleep。

   - 5.如果该投票的票，都投了，在睡眠过程中异常了，则需要抓取发生问题前后的电流图。通过电流图查看，发生问题的时机；可以通过执行部分代码发方式，找出是哪块代码执行异常。

   - 6.分析时可以抓取正常和异常时的log和电流图对比，分析差异，找出问题。

   - 7.以上排查完，找不出原因，如果需要协助，可以提供发生问题的步骤，抓取pm_debug 8的log，抓取发生问题前后的电流图，SDK版本等信息提问题单给我们。


-  进入低压(sleep mode)或者deep sleep功耗高分析步骤

-------------------------------------------------------

   - 1.首先确认是否是当前PCB上额外挂载的外设模块，没有下电导致功耗高。

   - 2.首先下载原版SDK，通过选择RTC唤醒方式，发命令的方式进入低压睡眠或者deepsleep(命令可以参考文档中样例演示->平台-> power manager中示例)，对比功耗差异。

   - 3.通过通过发送pm_debug 8,查看log中“pm power and pmu state[0x202a25e8][0x7d000272] ”和根据log分析 :doc:`bk_Lowpower_problem_analysis` 。

     解析0x202a25e8：以下为每bit对应的电源(0:表示该power domain是开启的; 1:表示该power domain是关闭的)。
     0x7d000272：以上值的最后2bit表示32K时钟源,0:表示26M时钟分下来的32k; 1:表示外部32k时钟; 2:表示内部的ROSC时钟。

     以下为各个bit对应的power domain:: 

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

    .. note::
            进入低压的预期值为：pm power and pmu state[0x202a25e8][0x7d000272]
            默认32K时钟源为ROSC

   - 4.同时通过电流波形图，查看功耗差异，是否存在异常电流。

   - 5.另外排查下系统会不会误入了ATE模式(通过查看log，由提示ate enabled is 1,值预期为0:关闭)或者DTIM参数设置和预期的不一样等。



-  进入低压(sleep mode)或者deep sleep唤醒失败分析步骤

---------------------------------------------------------

   - 1.需要使用唤醒源唤醒处于睡眠中(低压(sleep mode)或者deep sleep)的系统, 唤醒的前提是系统成功进入了低压或者deep sleep睡眠状态
   - 2.如果唤醒失败，可以通过测量电流的方式确认是否成功进入了低压或者deep sleep睡眠状态。低压睡眠时底电流小于100uA左右,deep sleep时底电流小于25uA左右。

:link_to_translation:`en:[English]`

