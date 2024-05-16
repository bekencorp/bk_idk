OS 抽象层
=======================

:link_to_translation:`en:[English]`



OS 抽象层介绍
------------------------

 - OS抽象层主要为了适配Armino平台不同的操作系统
 - 对于不同的操作系统，OS抽象层对外提供一套统一的接口
 - 目前Armino平台OS抽象层支持的操作系统有FreeRTOS，鸿蒙OS，RTThread.
 - 目前Armino平台posix接口仅支持FreeRTOS V10.4操作系统，默认关闭，若使用，需要打开CONFIG_FREERTOS_POSIX配置开关
 
.. note::

 - 在使用FreeRTOS的posix功能的时候，在引用posix相关头文件之前，需要先引用FreeRTOS_POSIX.h头文件；
 - 可以在components/bk_rtos/freertos/posix/freertos_impl/include/portable/bk/FreeRTOS_POSIX_portable.h中自定义相关配置。

OS API详细信息
------------------------

.. toctree::
    :maxdepth: 1

        OS API <../../api-reference/system/os>
        Posix API <../../api-reference/system/posix/index>



