:orphan:

.. _env_manual:

本地部署
===================

:link_to_translation:`en:[English]`


构建编译环境
------------------------------------

.. note::

    Armino 编译环境要求Ubuntu 20.04 LTS 版本及以上，Centos 7版本及以上，Archlinux或者Debian 11版本及以上，本章节将以 Ubuntu 20.04 LTS 版本为例，介绍整个编译环境的搭建。

编译环境可以使用环境安装脚本进行安装，也可以手动安装，这里推荐使用使用安装脚本方式。如果在sdk目录中没有安装脚本，可以从beken服务器上下载安装脚本。


- 脚本安装：

    如果sdk中存在安装脚本，那么进入sdk目录后执行安装脚本，按照安装脚本引导即可完成构建环境安装::

        cd ~/armino/bk_idk
        sudo bash tools/env_tools/setup/armino_env_setup.sh

    如果sdk中不存在安装脚本，可以从 `链接 <https://dl.bekencorp.com/d/tools/arminosdk/autoinstall_script/armino_env_setup.sh?sign=Xy2UU1MypajrqyIwyGPC_uW_6e4yqfKhJBuk14gtjx8=:0>`_  处下载并执行。

- 手动安装：

    手动安装请参考 :ref:`链接 <manual_install>`


.. note::

    工具链默认路径在 middleware/soc/bk7236/bk7236.defconfig 文件中定义，如果您更改了工具链路径，需要在配置文件中重新配置工具链路径::

        CONFIG_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin"


编译工程
------------------------------------

在终端下输入下述命令编译 Armino 默认工程，PROJECT为可选参数，默认为app，默认工程为启动 WiFi，BLE，初始化常见驱动，并启动 Armino 默认 Cli 程序::

    cd ~/armino/bk_idk
    make bk7236



您可以通过 PROJECT 参数来编译 projects 下其他工程，如 make bk7236 PROJECT=security/secureboot
可以编译 projects/security/secureboot 工程。
