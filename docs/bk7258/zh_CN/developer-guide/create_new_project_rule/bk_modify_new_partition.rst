修改分区简介
========================

:link_to_translation:`en:[English]`

修改分区
------------------------

  - 1）	建议从usr_config和rf_firmware分区之间插入新增分区；
  - 2）	name不可和已有的分区重名（注意，app、app1和download 分区还拥有别名，其对应关系为app->application,app1->application1,download->ota）；
  - 3）	需要的Size建议通过缩减user_config分区Size获得，所有分区的Size总和不得超过所用flash的实际Size；
  - 4）	在sdk默认分区csv表中新增分区示例如下。

  .. figure:: ../../../../common/_static/add_partition.png
    :align: center
    :alt: add_partition
    :figclass: align-center

    图7 add_partition

  - 5）	看新增分区是否生效，可以在vendor_flash.c中查看是否有该分区（路径是：projects/platform/main/vendor_flash.c）；或者在partitions.csv中查看（路径是：projects/platform/config/bk7258/partitions.csv）

  .. figure:: ../../../../common/_static/new_part.png
    :align: center
    :alt: new_part
    :figclass: align-center

    图8 vendor_flash

  .. figure:: ../../../../common/_static/new_part1.png
    :align: center
    :alt: new_part1
    :figclass: align-center

    图9 partitions_csv

  
  - 分区修改之后，需要先执行make clean;
