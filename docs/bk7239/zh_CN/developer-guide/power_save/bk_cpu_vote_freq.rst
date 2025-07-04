如何动态切换CPU主频
==================================================

切换时钟的策略
-----------------------------------------------------
- 1.配置CPU主频统一在CPU0上(避免多核操作相同的寄存器不同步而出现异常)。
- 2.CPU选择投频最高的模块的CPU主频。
- 3.支持CPU0和CPU1调用bk_pm_module_vote_cpu_freq()接口投频，不支持CPU2上调用该接口。
- 4.启动CPU1时，当前SDK默认把主频升到最高，应用程序可以不用额外再调接口投频。
- 5.CPU0或CPU1上调接口投频之后，CPU0,CPU1,CPU2的主频和BUS频率都会变化。因为他们的频率是联动的，不是独立的。


支持配置的CPU主频
-----------------------------------------------------
投频后CPU和BUS的频率对应关系：
    - 0. PM_CPU_FRQ_26M = 0,  // 0:CPU1:26M,CPU0:26M,BUS:26M
    - 1. PM_CPU_FRQ_60M,      // 1:CPU1:60M,CPU0:60M,BUS:60M
    - 2. PM_CPU_FRQ_80M,      // 2:CPU1:80M,CPU0:80M,BUS:80M
    - 3. PM_CPU_FRQ_120M,     // 3:CPU1:120M,CPU0:120M,BUS:120M
    - 4. PM_CPU_FRQ_240M,     // 4:CPU1:240M,CPU0:120m,BUS:120M
    - 5. PM_CPU_FRQ_320M,     // 5:CPU1:320M,CPU0:160M,BUS:160M
    - 6. PM_CPU_FRQ_480M,     // 6:CPU1:480M,CPU0:240M,BUS:240M
    - 7. PM_CPU_FRQ_DEFAULT   // default cpu frequency which control by pm module


动态投CPU主频的接口
-----------------------------------------------------
 - bk_err_t bk_pm_module_vote_cpu_freq(pm_dev_id_e module,pm_cpu_freq_e cpu_freq);
    - module：模块名字（选择一个使用的模块，如果自己新的模块，可以添加）
    - cpu_freq：需要投的主频


获取当前投的最高CPU主频的接口
-----------------------------------------------------
pm_cpu_freq_e bk_pm_current_max_cpu_freq_get()

示例：参考以下步骤1和步骤2
-----------------------------------------------------
 - 步骤1： DISP模块投320M:
   - bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_320M);

 - 步骤2： DISP模块使用完高平后，把自己的模块主频恢复默认：
   - bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);


.. note::
   投频使用的dev_id和使用完之后的dev_id需要一样，不然会引起错误。使用前可以搜索,确认DEV_ID是其他模块没有其他业务使用

:link_to_translation:`en:[English]`

