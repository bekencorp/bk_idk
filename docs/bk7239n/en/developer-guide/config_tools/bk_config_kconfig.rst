.. _bk_config_kconfig:

Kconfig Configuration
==========================

:link_to_translation:`zh_CN:[中文]`

This section introduces frequency ask questions of Armino Kconfig.

Kconfig configuration examples can be found in the project codebase.

Kconfig Files
----------------------------------------------------------

The Armino Kconfig organization is shown in the following diagram:

::

     -armino/
         - components/
             - c1/
                 -Kconfig
         - middleware/
             - soc/
                 - bk7239n/
                     -bk7239n.defconfig
                 - bk7258/
                     -bk7258.defconfig
         - projects/
             - my_project/
                - config/
                    - common/
                        - config
                    - bk7239n/
                        - config
                    - bk7258/
                        - config
                - Kconfig.projbuild
                - main/
                    - Kconfig
                - components/
                    - c2/
                        - Kconfig

These files can be grouped into two categories:

  - Configuration definition file:

    - Kconfig - usually defined in components.
    - Kconfig.projbuild - defined in the component or project directory.
  - Configuration modification file:

    - middleware/soc/bk7239n.defconfig - Default configuration of BK7239.
    - my_projects/config/common/config - configuration shared by different BK72xx in the project.
    - my_projects/config/bk7239n/config - BK7239 project configuration.

.. important::

   The configuration items can only be modified through configuration modification files after they are defined in definition files.
   If you try to modify a configuration that is not defined, no definitions will be generated in sdkconfig.h.

.. note::

   All the items defined in Kconfig.projbuild will be show in the top menu of menuconfig. BK7239 does not support menuconfig, so Kconfig.projbuild
   works same as Kconfig.

.. note::

   The configuration item names defined in the Kconfig file need to be prefixed with ``CONFIG_`` when used in config, defconfig, and C code.

In the Armino project, the priority of Kconfig configuration is::

 my_projects/config/bk7239n/config > my_projects/config/common/config > bk7239n.defconfig > configuration definition files

Menuconfig Supporting
----------------------------------------------------------

Armino Kconfig does not support menuconfig.

Configuration Dependencies
----------------------------------------------------------

Interdependent configuration items are automatically changed to correct values, only when using menuconfig configuration.
Armino does not support menuconfig, so when changing a configuration via configuration modification file,
the configurations that depend on this configuration will not be changed automatically.

.. important::

  All configuration items in Armino need to be specified explicitly, or use the default value.

Using Kconfig in CMakeLists.txt
----------------------------------------------------------

The configuration items defined in Kconfig can be directly used in CMakeLists.txt, but you should NOT put
armino_component_register() in any configuration condition. More information about disabling components via Kconfig can be found in the project codebase.



