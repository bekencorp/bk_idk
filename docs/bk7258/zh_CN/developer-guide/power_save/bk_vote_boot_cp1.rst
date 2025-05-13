如何启动和关闭CPU1
==================================================

启动和关闭CPU1的策略
-----------------------------------------------------

- 1.启动CPU1，有一个模块投票启动，则启动CPU1。

- 2.关闭CPU1时，需要所有投启动CPU1的模块，都投关闭的票，才能关闭。


当前预置启动CPU1的模块名称
------------------------------------------------------

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


启动CPU1的接口
-----------------------------------------------------

 - bk_err_t bk_pm_module_vote_boot_cp1_ctrl(pm_boot_cp1_module_name_e module,pm_power_module_state_e power_state)
 
    - module：模块名字（选择一个没有应用程序使用的模块名字。应用程序根据需求，可以自己添加一个模块名字）

    - power_state：PM_POWER_MODULE_STATE_ON:启动CPU1; PM_POWER_MODULE_STATE_OFF:关闭CPU1


获取当前启动CPU1是否成功的接口
-----------------------------------------------------

 - pm_mailbox_communication_state_e bk_pm_cp1_work_state_get()
    - 返回值：
        - 0x1:表示启动CPU1成功了
        - 0x0:表示启动CPU1失败或者没有启动CPU1
 

示例：启动CPU1和关闭CPU1
-----------------------------------------------------

 - 启动CPU1： MULTIMEDIA模块：
   - bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_MULTIMEDIA,PM_POWER_MODULE_STATE_ON);

 - 关闭CPU1： MULTIMEDIA模块：
   - bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_MULTIMEDIA,PM_POWER_MODULE_STATE_OFF);
   

如何查看启动CPU1是否成功或者关闭CPU1哪些模块投下电的票
----------------------------------------------------------------

  - 可以发送pm_debug 8命令，通过log分析“pm_cp1_ctrl_state:0x0”字段，详细参考 :doc:`bk_Lowpower_problem_analysis` 。

.. note::
   启动CPU1使用的module名字和使用完关闭CPU1的module名字需要一样，如果不一样，启动和关闭模块不对称，会出现不符合预期的。

:link_to_translation:`en:[English]`

