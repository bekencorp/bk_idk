Quick Start Guide
==============================================

:link_to_translation:`zh_CN:[中文]`

This article demonstrates:

 - Armino code and toolchain download
 - Setup software development environment
 - How to configure Armino project
 - How to build the project and flash the bin to board

Preparation
--------------------------------------------------------

Hardware：

 - BK7239 Demo board
 - BKFIL.exe
 - PC（Windows & Ubuntu）

.. note::

  Armino only supports compiling on Linux platform and firmware burning on Windows platform.
  Therefore, we need two computers, one Linux platform and one Windows platform.
  Of course, we can also install Linux virtual machine on Windows platform only

Software：

 - GCC ARM tool chain, used to compile BK7239
 - Build tools, including CMake
 - Armino source code
 - BKFIL.exe


Introduction to Development Board
--------------------------------------------------------

Click the following link to learn more about the development boards supported by Armino:

.. toctree::
    :maxdepth: 1

        BK7239 <bk7239>

Armino SDK Code download
--------------------------------------------------------------------

We can download Armino from gitlab::

    mkdir -p ~/armino
    cd ~/armino
    git clone https://gitlab.bekencorp.com/armino/bk_idk.git -b release/v2.0.1


.. note::

    1. The gitlab always has the latest Armino code, only authorised account can download the code. Please contact the your BK7239 project owner to get relevant accounts.
    2. On the other hand, Github only has the stable Armino code.


Setup Build Environment
--------------------------------------------------------------------

.. note::

    Armino, currently supports compiling in Linux environment. This chapter willtake Ubuntu 20.04 LTS
	as an example to introduce the construction of the entire compiling environment.


Tool Chain Installation
----------------------------------------------------------------

Click `Donwload <https://dl.bekencorp.com/tools/toolchain/arm/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2>`_ to download the BK7239 toolchain.

After downloading the tool kit, decompress it to '/opt/'::

    $ sudo tar -xvjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/


.. note::

    Tool chain the default path is configured in the middleware/soc/bk7239/bk7239.defconfig, you can modify ``CONFIG_TOOLCHAIN_PATH`` to set to your owner toolchain path:

    CONFIG_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin"


Depended libraries installation
-----------------------------------------------------------------

Enter the following command in the terminal to install python\CMake\Ninja::

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install build-essential cmake python3 python3-pip doxygen ninja-build libc6:i386 libstdc++6:i386 libncurses5-dev lib32z1 -y
    sudo pip3 install pycryptodome click

Document Compilation Dependent Library Installation
------------------------------------------------------------------------------

Click `Doc <http://docs.bekencorp.com/armino/bk7256/zh_CN/latest/index.html>`_ you can get the latest Armino documents. So you don't need to build the document youself.

If you really need to build the documents, following python3 packages are required::

    sudo pip3 install sphinx_rtd_theme future breathe blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag sphinxcontrib-nwdiag sphinxcontrib.blockdiag


If you default Python is Python2, please set it to Python3::

    sudo ln -s /usr/bin/python3 /usr/bin/python


Build The Project
------------------------------------

Run following commands to build BK7239 verify project::

    cd ~/armino/bk_idk
    make bk7239 PROJECT=verify/spe

Another way to build BK7239 verify project is::

    cd ~/armino/bk_idk/project/verify
    make s

The above build method are equal. 

The verify project startup:

 - WiFi/BLE
 - Common pheripheral driver
 - Armino default Cli

.. note::

    BK7239 is in developing stage, currently it only supports verify project. Please use verify project to demo BK7239.

Configuration project
------------------------------------

We can also use the project configuration file for differentiated configuration::

    Project Profile Override Chip Profile Override Default Configuration
    Example： bk7239.config >> bk7239.defconfig >> KConfig
    + Example of project configuration file：
        projects/app/config/bk7239.config
    + Sample chip configuration file：
        middleware/soc/bk7239/bk7239.defconfig
    + Sample KConfig configuration file：
        middleware/arch/cm33/Kconfig
        components/bk_cli/Kconfig

New project
------------------------------------

The default project is projects/app. For new projects, please refer to projects/harmony project


Burn Code
------------------------------------

On the Windows platform, Armino currently supports UART burning.



Burn through serial port
****************************************

.. note::

    Armino supports UART burning. It is recommended to use the CH340 serial port tool board to download.

Serial port burning tool is shown in the figure below:

.. figure:: ../../_static/download_tool_uart.png
    :align: center
    :alt: Uart
    :figclass: align-center

    UART

Download burning tools (BKFILL.exe)：

	http://dl.bekencorp.com/tools/flash/
	Get the latest version in this directory. Ex：BEKEN_BKFIL_V2.1.4.0_20231011.zip

The snapshot of BKFILL.exe downloading.

.. figure:: ../../_static/download_uart_bk7239_en.png
    :align: center
    :alt: BKFIL GUI
    :figclass: align-center

    BKFIL GUI


Burn the serial port DL_UART0, click ``Download`` to burn the image, and then power down and restart the device after burning.


Serial port Log and Command Line
------------------------------------

Currently the BK7239 use the DL_UART0 as the Log output and Cli input; You can get the supported command list through the help command.
