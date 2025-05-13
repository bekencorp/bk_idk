New Project Rule Introduction
================================

:link_to_translation:`zh_CN:[中文]`

.. note::
    Creating a new project rule mainly involves the following parts: partition table configuration, modifying macros in the config file, and adding automation lists. Taking the bk7258 platform project as an example, this will provide a detailed introduction on how to create a new project.

Partition Table Configuration
-------------------------------
The partition table configuration is a CSV file. For example, the partition table configuration file for the bk7258 platform engineering is bk7258_partitions.csv. The format is as follows:

  .. figure:: ../../../../common/_static/partition_layout.png
    :align: center
    :alt: partition_layout
    :figclass: align-center

    Fig1 partition_layout

  - ``Name`` - Must be named according to the SoC, such as bk7258, and the partition table name must be bk7258_partitions.csv. The partition table has 6 columns, and the Name column is required and must be unique, indicating the partition name.

    - During the build process, a unique macro definition will be generated based on the partition name, and the code will operate on this macro definition to access the corresponding partition.
    - The compiled binary file name is also named based on the partition name, such as bootloader.bin for the bootloader and app.bin for the app.
    - During packaging, the configuration.json file is updated based on the partition name for packaging.
    - Reserved partitions are partitions with specific purposes in Armino, such as:

      - bootloader - Type is configured as code, which corresponds to bootloader.bin compiled by the build system. Size is 34/32 times the actual size of bootloader.bin.
      - app - Type is configured as code, which corresponds to app.bin compiled by the build system. Size is 34/32 times the actual size of app.bin.
      - app1 - Type is configured as code, corresponding to app1.bin compiled by the build system, Size is 34/32 times the actual size of app1.bin
      - app2 - Type is configured as code, which corresponds to app2.bin compiled by the build system.Size is 34/32 times the actual size of app2.bin.
      - download - A unique partition in Armino. This partition will be used by ota. When the ota function is not needed, the partition size can be reserved and configured as 4K.
      - usr_config - A unique partition in Armino. This partition will be used by the file system. The size of this partition can be increased or decreased appropriately in units of 4K.
      - easyflash - a unique partition in Armino, Size and Offset are fixed and cannot be customized.
      - rf_firmware, net_param - unique partitions in Armino, RF calibration, Wi-Fi/TCPIP will use these two partitions.Size and Offset are fixed and cannot be customized.
  - ``Offset`` - Optional, represents the physical offset position in the flash memory; if not specified, it will default to being appended after the previous partition.
  - ``Size`` - Required, represents the physical size of the partition, in units of k/K (bytes). For data type regions, it must be 4K aligned, while for code regions, it must be 68K aligned.
  - ``Type`` -  Required, represents the type of region. code: represents the region for executable code (68K aligned); data: represents the region for storing data.
  - ``Read`` -  Optional, represents the read permission for this region. If not configured, the default is FALSE. It is recommended to set it to TRUE to make the region readable.
  - ``Write`` - Optional, represents the write permission for this region. Generally, for code regions, it should be set to FALSE to prevent accidental modifications, while for data regions, it should be set to TRUE.
  - The cumulative size of all partitions cannot exceed the size of the flash memory, which is currently 8M/bytes for BK7258.