USB Host MSD
================================

:link_to_translation:`en:[English]`

    BK7258的 USB Device MSC 默认是识别和使用SDcard或SDNAND，所以若使用U盘功能需要将SDcard的驱动相关宏也需要打开。

USB Device MSC的宏定义配置
------------------------------------------------------------------------

CONFIG_USB=y  打开USB的总开关宏定义 使能USB的代码功能

CONFIG_USB_VBAT_CONTROL_GPIO_ID=0x1C  默认使用的GPIO是GPIO_28进行USB Vbus电的控制，用户根据自己的实际情况进行配置

CONFIG_CHERRY_USB=y  默认使用的CherryUSB的开源代码，用户可根据实际情况进行版本的更新

CONFIG_USB_HOST=n    主程序作为DEVICE 连接到HOST被枚举到为U盘
CONFIG_USB_DEVICE=y

CONFIG_USBD_MSC=y 打开MSC驱动的编译，作为U盘被HOST识别到


MSD 主机驱动CLI测试示例
------------------------------------

注：作为DEVICE，若需要使用U盘功能时，才将驱动初始化。若需要U盘功能实时待命识别，请将驱动在main函数中初始化

打开USB U盘驱动，被PC识别到：
	输入命令：usbd msc_init 初始化U盘驱动

.. figure:: ../../../../_static/usb/usbd_msc_init.png
    :align: center
    :alt: USBD msc init
    :figclass: align-center

    USB DEVICE UDISK INIT

	输入命令：usbd msc_deinit 卸载U盘驱动

.. figure:: ../../../../_static/usb/usbd_msc_uninit.png
    :align: center
    :alt: USBD msc uninit 
    :figclass: align-center

    USB DEVICE UDISK UNINIT

USB DEVICE MSC
---------------------
    更多详细开发可参考CherryUSB相关文档， https://github.com/cherry-embedded/CherryUSB/blob/master/README_zh.md