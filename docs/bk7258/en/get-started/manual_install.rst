:orphan:

.. _manual_install:


Manual installation
===================

:link_to_translation:`zh_CN:[中文]`

Installation steps
------------------------

 - Tool Chain Installation
 - Document Compilation Dependent Library Installation
 - Document Compilation Dependent Library Installation

Tool Chain Installation
----------------------------------------------------------------

Click `Donwload <https://dl.bekencorp.com/tools/toolchain/arm/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2>`_ to download the BK7236 toolchain.

After downloading the tool kit, decompress it to '/opt/'::

    $ sudo tar -xvjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/


.. note::

    Tool chain the default path is configured in the middleware/soc/bk7258/bk7258.defconfig, you can modify ``CONFIG_TOOLCHAIN_PATH`` to set to your owner toolchain path:

        CONFIG_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin"

Depended libraries installation
-----------------------------------------------------------------

If using the Ubuntu or Debian operating system, Enter the following command in the terminal to install::

    sudo apt update
    sudo apt install make cmake python3 python3-pip ninja-build -y

If using the CentOS operating system, you need to enter the following command::

    sudo yum update
    sudo yum install -y python38 make git cmake3 ninja-build bzip2 wget

If using the Archlinux operating system, you need to enter the following command::

    sudo pacman -Syyu
    sudo pacman -S --noconfirm make wget git cmake python3 python-pip python-setuptools ninja

Enter the following command to install python modules::

    sudo pip3 install pycryptodome click future click_option_group cryptography jinja2 PyYAML cbor2 intelhex

If you default Python is Python2, please set it to Python3. The minimum required version for Python is Python 3.8::

    sudo ln -s /usr/bin/python3 /usr/bin/python


.. note::

    If Ubuntu version is 23.04 or above, Python's third-party libraries will use external environment management. When installing third-party libraries using pip, you need to pass in --break-system-packages.
    CentOS 7 cannot install Python 3.8 from the software source and needs to be installed using source code compilation.


Document Compilation Dependent Library Installation
------------------------------------------------------------------------------

.. note::

    Usually you don't need to build the document youself. Click `Doc <https://docs.bekencorp.com/arminodoc/bk_idk/bk7258/zh_CN/v2.0.1/index.html>`_ you can get the latest Armino documents.


If you really need to build the documents, following python3 packages are required::

    sudo pip3 install Sphinx sphinx-rtd-theme breathe blockdiag sphinxcontrib-blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag sphinxcontrib-nwdiag Pillow


And the necessary software tool doxygen needs to be installed. When using Ubuntu, enter the following command to install it::

    sudo apt install doxygen -y


.. note::

    With the upgrade of doxygen software and the update of Python third-party libraries, there are incompatible syntax and feature calls for documents before February 2025. 
    You can refer to articles on the `Armino forum <https://armino.bekencorp.com/article/25.html>`_ to solve the compilation problems of previous documents.
    