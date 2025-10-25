增加AB分区功能或建立AB工程的介绍
---------------------------------

:link_to_translation:`en:[English]`

  一、在新工程中增加AB功能或新建AB工程的修改主要分为以下几点：

  - 1、首先，规划AB分区的分区表，增加AB功能相关的宏配置，以及修改ab_position_independent.csv的值，确保开启AB功能；
  - 2、其次，确认该工程是否已加入到自动化例表中（在组件part_table添加）；
  - 3、然后，确认该工程是否已加入到ab分区相关脚本处理中（special_project_deal.py）；
  - 4、最后，配置pj_config.mk文件（配置bootloader的路径以及自动化编译所需的参数）
  - 5、具体步骤见下述描述。

  二、新增AB分区功能具体步骤如下，以bk_idk/projects/customization/config_ab/config/bk7258工程为例（可当作example）：

  - 1、AB分区的分区表、宏配置如下：
      - 1）需要有ota_rbl.config ，该配置文件是制作AB升级固件时所需的配置；（可直接复制该文件，且该文件位置放置在config目录文件下）；
      - 2）然后需要有ab_position_independent.csv，该配置文件用于打包时所需（可直接复制该.csv文件）;
      - 3）配置分区表，以bk7258_partitions.csv为例：
          - a)app、app1、app2的size均需为68k的倍数；
          - b)所需s_app的大小等于三个app分区和的大小；（若app只有一个，则s_app的大小等于该app的大小）；
          - c)所需ota_fina_executive分区，该分区为存储AB分区升级时的flag；
      - 4）将该config文件中的配置宏，拷贝到对应过程中的配置文件中；

  .. figure:: ../../../../../common/_static/ab_config.png
     :align: center
     :alt: ab_config
     :figclass: align-center

     图1 AB分区config配置

  .. figure:: ../../../../../common/_static/ab_partition.png
     :align: center
     :alt: ab_partition
     :figclass: align-center

     图2 AB的分区表配置

.. note::
    - 1.app、app1、app2的size均需为68k的倍数；
    - 2.s_app的大小等于三个app分区和的大小;

- 2、AB功能的工程需加入到自动化例表中 （customization/config_ab为例）：
    - 1）在 part_table/cmakelists中 bk7258xx_supported_projects例表中，增加 customization/config_ab；
    - 2）在 part_table/part_table.mk中 bk7258xx_supported_projects 中，增加 customization/config_ab；

  .. figure:: ../../../../../common/_static/add_into_autolist.png
     :align: center
     :alt: add_into_autolist
     :figclass: align-center

     图3 将AB工程加入到自动化例表

- 3、将AB工程加入到special_project_deal.py脚本处理中，路径：bk_idk/tools/build_tools/part_table_tools/otherScript

  .. figure:: ../../../../../common/_static/special_deal.png
     :align: center
     :alt: special_deal
     :figclass: align-center

     图4 加入到special_project_deal

- 4、需要pj_config.mk(路径是：bk_idk/projects/customization/config_ab) 配置所需的bootloaer路径，以及自动化例表所需的参数

  .. figure:: ../../../../../common/_static/project_config.png
     :align: center
     :alt: project_config
     :figclass: align-center

     图5 project_config配置