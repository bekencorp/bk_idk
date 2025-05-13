工程目录结构简介
========================

:link_to_translation:`en:[English]`

新建工程目录结构
------------------------

新建工程的目录结构如下：以bk7258的platform工程为例;

  .. figure:: ../../../../common/_static/project.png
    :align: center
    :alt: project
    :figclass: align-center

    图2 project_struct

  - 在config/bk7258/config文件中需配CONFIG_OVERRIDE_FLASH_PARTITION=y；

  .. figure:: ../../../../common/_static/config.png
    :align: center
    :alt: config
    :figclass: align-center

    图3 project_config

  - projects/platform/main/CMakeLists.txt的basic内容需包含如下：

  .. figure:: ../../../../common/_static/cmakelist.png
    :align: center
    :alt: cmakelist
    :figclass: align-center

    图4 project_cmakelist

  - 其中vendor_flash.c和vendor_flash_partiiton.h可有可无，因为在make clean之后，会自动生成；
  -	Configuration.json是用于最后的固件打包，其中的数值在编译时根据分区表自动生成；
  - 新建工程的app_main.c中的main函数中需包含bk_init();已确保响应基本的命令；