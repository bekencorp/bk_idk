Add AB Partition Function or Create AB Project
------------------------------------------------

:link_to_translation:`zh_CN:[中文]`

一、Key Modifications to Add AB Functionality in a New Project or Create an AB Project:

  - 1、Planning the AB Partition Table, Adding Macro Configurations, and Modifying the ab_position_independent.csv File:
  - 2、Adding the Project to the Automation list: (Verify that the project has been added to the automation list in component part_table):
  - 3、Ensure the project is added to the script handling in special_project_deal.py):
  - 4、Configuring the pj_config.mk File(Set the boot loader path and parameters required for automated compilation)
  - 5、Detailed steps are described below.

二、Specific Steps to Add AB Partition Functionality,Using the example of the bk_idk/projects/customization/config_ab/config/bk7258 project (which can be used as a reference):

  - 1、AB Partition Table and Macro Configuration:
      - 1) The ota_rbl.config file is required for creating AB upgrade firmware. Place this file directly in the config directory;
      - 2) The ab_position_independent.csv file is needed for packing purposes. Copy this .csv file directly;
      - 3) Configure the partition table, using bk7258_partitions.csv as an example:

          - The sizes of the app, app1, and app2 partitions must be multiples of 68k.
          - The size of the s_app partition must equal the sum of the sizes of the three app partitions. (If there is only one app partition, the s_app size equals the size of that single partition.)
          - The ota_fina_executive partition stores the flag for AB partition upgrades.
      - 4) Copy the macro configurations from the config file to the corresponding configuration files.

  .. figure:: ../../../../../common/_static/ab_config.png
     :align: center
     :alt: ab_config
     :figclass: align-center

     Figure 1: AB Partition Configuration

  .. figure:: ../../../../../common/_static/ab_partition.png
     :align: center
     :alt: ab_partition
     :figclass: align-center

     Figure 2: AB Partition Table Configuration

.. note::
    - 1.The sizes of app, app1, and app2 partitions must be multiples of 68k.
    - 2.The size of s_app must equal the sum of the sizes of the three app partitions.

- 2、Adding the AB Project to the Automation List (e.g., customization/config_ab)
    - 1) Add customization/config_ab to the bk7258xx_supported_projects list in part_table/CMakeLists.txt.
    - 2) Add customization/config_ab to the bk7258xx_supported_projects list in part_table/part_table.mk.

  .. figure:: ../../../../../common/_static/add_into_autolist.png
     :align: center
     :alt: add_into_autolist
     :figclass: align-center

     Figure 3: Adding AB Project to Automation List

- 3、Adding the AB Project to special_project_deal.py Script Handling,Location: bk_idk/tools/build_tools/part_table_tools/otherScript

  .. figure:: ../../../../../common/_static/special_deal.png
     :align: center
     :alt: special_deal
     :figclass: align-center

     Figure 4: Adding AB Project to Script Handling

- 4、Configuring pj_config.mk (Path: bk_idk/projects/customization/config_ab),Set the boot loader path and parameters required for automation.

  .. figure:: ../../../../../common/_static/project_config.png
     :align: center
     :alt: project_config
     :figclass: align-center

     Figure 5: Project Configuration