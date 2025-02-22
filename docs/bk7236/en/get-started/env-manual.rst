Local Deployment
===================

:link_to_translation:`zh_CN:[中文]`

Setup Build Environment
--------------------------------------------------------------------

.. note::

      The Armino compilation environment requires Ubuntu 20.04 LTS version and above, CentOS 7 version or above, ArchLinux and Debian 11 version or above. This chapter will take Ubuntu 20.04 LTS version as an example to introduce the construction of the entire compilation environment.

The compilation environment can be installed using an environment installation script or manual installation. It is recommended to use the installation script method. If there is no installation script in the SDK directory, 
you can follow the manual installation method.


- Srcipt installation:

    If there is an installation script in the SDK, enter the SDK directory and execute the installation script. Follow the installation script to complete the build environment installation::

        cd ~/armino/bk_idk
        sudo bash tools/env_tools/setup/armino_env_setup.sh

    If the installation script does not exist in the SDK, it can be downloaded and executed from the `link <https://dl.bekencorp.com/d/tools/arminosdk/autoinstall_script/armino_env_setup.sh?sign=Xy2UU1MypajrqyIwyGPC_uW_6e4yqfKhJBuk14gtjx8=:0>`_ .

- Manual installation:

    Please refer to the :ref:`link <manual_install>` for manual installation.


.. note::

    The default path for the toolchain is defined in the middleware/doc/bk7258/bk7258.deficonfig. If you change the toolchain path, you will need to reconfigure it in the configuration file::

        CONFIG_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin"


Build The Project
------------------------------------

Run following commands to build BK7236 app project::

    cd ~/armino/bk_idk
    make bk7236

The app project startup:

 - WiFi/BLE
 - Common pheripheral drivers
 - Armino default Cli

You can build other projects with PROJECT parameter, e.g. run "make bk7236 PROJECT=security/secureboot" 
can build projects/security/secureboot etc.