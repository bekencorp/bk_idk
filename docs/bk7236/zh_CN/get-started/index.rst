快速入门
=======================

:link_to_translation:`en:[English]`

本文档以 BK7236 开发板为例，通过一个简单的示例项目向您展示:

 - 代码，工具链下载；
 - 编译环境搭建；
 - 工程配置；
 - 固件编译与烧录；

概述
------------------------

BK7236 SoC 芯片支持以下功能:

 - 2.4G Wi-Fi 6
 - 低功耗蓝牙 5.4
 - ARMv8-M Star (M33F) MCU
 - 丰富的外设
 - 内置 TrustEngine 安全引擎

BK7236 采用 22 nm 工艺制成，具有最佳的功耗性能，射频性能，稳定性，通用性，可靠性，
和最高级别的安全（PSA Level 2），适用于各种应用场景和不同功耗，安全需求。

博通集成为用户提供完整的软，硬件资源，进行 BK7236 硬件设备开发。其中，软件开发环境
Armino 旨在协助用户快速开发物联网(IoT)应用，可满足用户对 Wi-Fi，蓝牙，低功耗，安全
等方面的要求。

准备工作
------------------------

硬件：

 - BK7236 开发板；
 - 串口烧录工具；
 - 电脑（Windows 与 Ubuntu）；

.. note::

  Armino 支持在 Linux 平台编译，也支持在 Windows 平台的 Linux 虚拟机上编译;
  Armino 支持在 Windows/Linux 平台进行固件烧录(参考烧录工具中指导文档)。

软件：

 - ARM GCC 工具链，用于编译 BK7236 版本；
 - 构建工具，包含 CMake；
 - Armino 源代码；
 - 串口烧录软件；


开发板简介
------------------------

点击下列链接了解 Armino 支持的开发板详细信息：

.. toctree::
    :maxdepth: 1

        BK7236 <bk7236>


Armino SDK 代码下载
------------------------------------

您可从 gitlab 上下载 Armino::

    mkdir -p ~/armino
    cd ~/armino
    git clone https://gitlab.bekencorp.com/armino/bk_idk.git


您也可从 github 上下载 Armino::

	mkdir -p ~/armino
	cd ~/armino
	git clone https://github.com/bekencorp/bk_idk.git


然后切换到稳定分支Tag节点, 如v2.0.1.12::

    git checkout -B your_branch_name v2.0.1.12

.. note::

    从官网的gitlab下载的为最近的SDK代码，相关账号找项目上审核申请。


构建编译环境
------------------------------------

.. note::

    Armino 编译环境要求Ubuntu 20.04 LTS 版本及以上，本章节将以 Ubuntu 20.04 LTS 版本为例，介绍整个编译环境的搭建。


工具链下载与安装
------------------------------------

点击 `下载 <https://dl.bekencorp.com/tools/toolchain/arm/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2>`_ 下载 BK7236 工具链。

工具包下载后，通过如下操作命令解压至 /opt/ 目录下::

    $ sudo tar -xvjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/


.. note::

    工具链默认路径在 middleware/soc/bk7236/bk7236.defconfig 文件中定义，您也可以在项目配置文件中更改工具链路径::

        CONFIG_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin"

程序编译依赖库安装
------------------------------------

在终端输入下述命令安装 python3，CMake，Ninja，Crypto以及依赖库::

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install build-essential cmake python3 python3-pip doxygen ninja-build libc6:i386 libstdc++6:i386 libncurses5-dev lib32z1 -y
    sudo pip3 install pycryptodome click

文档编译依赖库安装
------------------------------------

点击进入 `文档 <https://docs.bekencorp.com/arminodoc/bk_idk/bk7236/zh_CN/v2.0.1/index.html>`_ 您就可以查看 Armino 最近或者最新的文档，因此，
通常您不需要自己编译 Armino 文档。但如果您希望自己生成 Armino 文档，则需要安装下述 Python 依赖::

    sudo pip3 install sphinx_rtd_theme future breathe blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag sphinxcontrib-nwdiag sphinxcontrib.blockdiag

如果您的 Python 默认为 Python2，请使用下述命令更改成 Python3::

    sudo ln -s /usr/bin/python3 /usr/bin/python


编译工程
------------------------------------

在终端下输入下述命令编译 Armino 默认工程，PROJECT为可选参数，默认为app，默认工程为启动 WiFi，BLE，初始化常见驱动，并启动 Armino 默认 Cli 程序::

    cd ~/armino/bk_idk
    make bk7236



您可以通过 PROJECT 参数来编译 projects 下其他工程，如 make bk7236 PROJECT=security/secureboot
可以编译 projects/security/secureboot 工程。


配置工程
------------------------------------

您可以通过工程配置文件来进行更改 Armino 默认配置或者针对不同芯片进行差异化配置::

    工程配置文件 Override 芯片配置文件 Override 默认配置
    如： bk7236/config >> bk7236.defconfig >> KConfig
    + 工程配置文件示例：
        projects/app/config/bk7236/config
    + 芯片配置文件示例：
        middleware/soc/bk7236/bk7236.defconfig
    + KConfig 配置文件示例：
        middleware/arch/cm33/Kconfig
        components/bk_cli/Kconfig

点击 :ref:`Kconfig 配置 <bk_config_kconfig>` 进一步了解 Armino 配置。

新建工程
------------------------------------

BK7236 默认工程为 projects/app，新建工程可参考 projects/app工程


烧录代码
------------------------------------

Armino 支持在 Windows/Linux 平台进行固件烧录, 烧录方法参考烧录工具中指导文档。
以Windows 平台为例， Armino 目前支持 UART 烧录。



通过串口烧录
********************

.. note::

    Armino 支持 UART 烧录，推荐使用 CH340 串口工具小板进行下载。

串口烧录工具如下图所示:

.. figure:: ../../_static/download_tool_uart.png
    :align: center
    :alt: Uart
    :figclass: align-center

    UART

烧录工具（BKFIL）获取：

	https://dl.bekencorp.com/tools/flash/
	在此目录下获取最新版本，如：BEKEN_BKFIL_V2.1.6.0_20231123.zip

BKFIL.exe 界面及相关配置如下图所示：

.. figure:: ../../_static/download_uart_bk7236.png
    :align: center
    :alt: BKFIL GUI
    :figclass: align-center

    BKFIL GUI

选择烧录串口 DL_UART0，点击 ``烧录`` 进行版本烧录, 烧录完成之后掉电重启设备。

点击 :ref:`BKFIL <bk_tool_bkfil>` 进一步了解 Armino 烧录工具。

串口 Log 及 Command Line
------------------------------------

目前 BK7236 平台，串口 Log 及 Cli 命令输入在 DL_UART0 口；可通过 help 命令查看支持命令列表。
