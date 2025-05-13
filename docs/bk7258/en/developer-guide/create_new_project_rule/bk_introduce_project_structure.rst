Project Directory Structure Overview
======================================

:link_to_translation:`zh_CN:[中文]`

Creating a New Project Directory Structure
---------------------------------------------

The directory structure for a new project is as follows: Taking the bk7258 platform project as an example:

  .. figure:: ../../../../common/_static/project.png
    :align: center
    :alt: project
    :figclass: align-center

    Fig2 project_struct

  - In the config/bk7258/config file, you need to set CONFIG_OVERRIDE_FLASH_PARTITION=y;

  .. figure:: ../../../../common/_static/config.png
    :align: center
    :alt: config
    :figclass: align-center

    Fig3 project_config

  - The basic content of projects/platform/main/CMakeLists.txt should include the following:

  .. figure:: ../../../../common/_static/cmakelist.png
    :align: center
    :alt: cmakelist
    :figclass: align-center

    Fig4 project_cmakelist

  - vendor_flash.c and vendor_flash_partiiton.h are optional, as they will be automatically generated after running make clean;
  -	Configuration.json is used for final firmware packaging, and the values are automatically generated during compilation based on the partition table;
  - In the app_main.c file of the new project, the main function should include bk_init(); to ensure basic command responses.