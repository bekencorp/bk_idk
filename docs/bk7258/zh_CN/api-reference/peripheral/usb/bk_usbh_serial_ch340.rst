USB Host Serial CH340
================================================================================================================================

:link_to_translation:`en:[English]`

    BK7258的 USB HOST 可以加载CH340的驱动。打开USB驱动后，连接识别到CH340设备，相关宏开关在CPU0的config中打开。

USB MSD的宏定义配置
---------------------------------------------------------------------------------------------

CONFIG_USB=y  打开USB的总开关宏定义 使能USB的代码功能

CONFIG_USB_VBAT_CONTROL_GPIO_ID=0x1C  默认使用的GPIO是GPIO_28进行USB Vbus电的控制，用户根据自己的实际情况进行配置

CONFIG_CHERRY_USB=y  默认使用的CherryUSB的开源代码，用户可根据实际情况进行版本的更新

CONFIG_USB_HOST=y    主程序作为HOST 对接口连接的设备进行枚举

CONFIG_TASK_USB_PRIO=2 USB的处理程序的TASK优先级配置为2

CONFIG_USBH_SERIAL_CH340=y 打开Serial ch340驱动的编译，能够做到打开USB后自动识别 CH340串口


Serial ch340 主机驱动CLI测试示例
------------------------------------------------------------------------------

参考代码在cli_usb.c中，API的调用及示例请参考由CONFIG_USBH_SERIAL_CH340宏范围的code。
集合使用的demo，参考cli_simulate_uart_demo init/deinit操作。

注册回调函数，设备连接成功后上报回调函数cli示例：
	输入命令：
	usbh_to_uart reg_cb 0 注册回调函数
	usbh_to_uart unreg_cb 0 注销回调函数

.. figure:: ../../../../_static/usb/usbh_serial_ch340_callback_init_deinit.png
    :align: center
    :alt: cb init deinit
    :figclass: align-center

    Serial CH340 Register callback

打开USB驱动对CH340 串口打开进行枚举：
	输入命令：
	usbh_to_uart init 0

.. figure:: ../../../../_static/usb/usbh_serial_ch340_enumeration.png
    :align: center
    :alt: open USB for enumeration 
    :figclass: align-center

    USB HOST for enumeration

关闭CH340的驱动，先触发断开中断，然后再关闭USB驱动:
	usbh_to_uart deinit 0

.. figure:: ../../../../_static/usb/usbh_serial_ch340_deinit.png
    :align: center
    :alt: mount_udisk
    :figclass: align-center

    USB HOST CH340 Driver Deinit

调用接口发送数据,log中显示发送成功后的回调函数。测试命令如下:
	usbh_to_uart write 0 128 100

    id:0     cnt:128 bytes    timeout_ms:100ms

.. figure:: ../../../../_static/usb/usbh_serial_ch340_send.png
    :align: center
    :alt: send data
    :figclass: align-center

    Send Data

调用接口接收数据,接收到数据后，触发回调函数。测试命令如下:
	usbh_to_uart read 0 32 1000

    id:0     cnt:32 bytes    timeout_ms:1000ms

.. figure:: ../../../../_static/usb/usbh_serial_ch340_receive.png
    :align: center
    :alt: receive data
    :figclass: align-center

    Receive Data


基本API 调用流程图
------------------------------------------------------------------------------------

.. figure:: ../../../../_static/usb/usbh_serial_ch340_flow.png
    :align: center
    :alt: flow
    :figclass: align-center

    Flow chart

注:
1.回调函数注册是一次性将tx、rx、connect、disconnect回调函数都注册。

2.初始化的配置，除了波特率后续都不能更改。

3.发送数据，timeout大于0时，表示将一致等待数据发送直到结束。timeout等于0时，将把数据拷贝到fifo中，再发送，注意返回值错误时可能fifo不足。

4.接收数据时，可通过rx回调函数通知后再读出数据。或使用timeout等待数据的接收。

5.2种方式检测是否连接成功，①connect回调函数上报，②check检测是否有设备连接。

6.关闭时，将先上报disconnect，在对USB驱动进行关闭。


USB HOST Serial CH340
------------------------------------------------------------------------------------
    更多详细开发可参考CherryUSB相关文档， https://github.com/cherry-embedded/CherryUSB/blob/master/README_zh.md