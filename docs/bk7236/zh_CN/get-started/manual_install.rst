:orphan:

.. _manual_install:


手动安装
===================

:link_to_translation:`en:[English]`

安装步骤
------------------------

 - 工具链下载与安装
 - 程序编译依赖库安装
 - 文档编译依赖库安装

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

使用Ubuntu或Debian操作系统，需要输入以下命令::

    sudo apt update
    sudo apt install make cmake python3 python3-pip ninja-build -y

使用Centos操作系统，需要输入以下命令::

    sudo yum update
    sudo yum install -y python38 make git cmake3 ninja-build bzip2 wget

使用Archlinux操作系统，需要输入以下命令::

    sudo pacman -Syyu
    sudo pacman -S --noconfirm make wget git cmake python3 python-pip python-setuptools ninja

在终端输入下述命令安装python依赖库::

    sudo pip3 install pycryptodome click future click_option_group cryptography jinja2 PyYAML cbor2 intelhex

如果您的 Python 默认为 Python2，请使用下述命令更改成 Python3，Python最低要求版本为Python3.8::

    sudo ln -s /usr/bin/python3 /usr/bin/python


.. note::

    如果Ubuntu版本在23.04以上，python的三方库会使用外部环境管理，使用pip安装三方库时需要传入--break-system-packages。
    Centos7 从软件源无法安装python3.8，需要使用源码编译方式安装。


文档编译依赖库安装
------------------------------------


.. note::

    通常您不需要自己编译 Armino 文档，点击进入 `文档 <https://docs.bekencorp.com/arminodoc/bk_idk/bk7236/zh_CN/v2.0.1/index.html>`_ 您就可以查看 Armino 最近或者最新的文档。


但如果您希望自己生成 Armino 文档，则需要安装下述 Python 依赖::

    sudo pip3 install Sphinx sphinx-rtd-theme breathe blockdiag sphinxcontrib-blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag sphinxcontrib-nwdiag Pillow


并且需要安装必要的软件工具doxygen，当使用ubuntu时输入以下命令安装::

    sudo apt install doxygen -y


.. note::

    随着doxygen软件升级和python三方库更新，对于2025年2月之前的文档，存在不兼容的语法和特性调用，可以参考 `armino论坛 <https://armino.bekencorp.com/article/25.html>`_ 的文章，来解决以前文档编译问题。
