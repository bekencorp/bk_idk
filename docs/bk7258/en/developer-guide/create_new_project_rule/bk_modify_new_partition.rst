Modifying Partitions
========================

:link_to_translation:`zh_CN:[中文]`

Modifying Partitions
------------------------

  - 1）	It is recommended to insert the new partition between the usr_config and rf_firmware partitions;
  - 2）	The name cannot be the same as an existing partition (Note that app, app1, and download partitions have aliases,the relationship is app->application,app1->application1,download->ota);
  - 3）	The required Size is suggested to be obtained by reducing the Size of the user_config partition. The total Size of all partitions must not exceed the actual Size of the flash used;
  - 4）	An example of adding a new partition to the default partition csv table in the SDK is shown below.

  .. figure:: ../../../../common/_static/add_partition.png
    :align: center
    :alt: add_partition
    :figclass: align-center

    Fig7 add_partition

  - 5）	Check if the new partition takes effect. You can check if the partition exists in vendor_flash.c (path: projects/platform/main/vendor_flash.c) or in partitions.csv (path: projects/platform/config/bk7258/partitions.csv).

  .. figure:: ../../../../common/_static/new_part.png
    :align: center
    :alt: new_part
    :figclass: align-center

    Fig8 vendor_flash

  .. figure:: ../../../../common/_static/new_part1.png
    :align: center
    :alt: new_part1
    :figclass: align-center

    Fig9 partitions_csv

  
  - After modifying the partition, you need to execute make clean first;
