自动化列表的简介
========================

:link_to_translation:`en:[English]`

增加工程到自动化列表
------------------------
将新建的工程名 platform添加到 components/part_table/CMakeLists.txt 和 components/part_table/part_table.mk 文件所定义的支持列表中，如下图所示：

  - components/part_table/CMakeLists.txt文件修改如下：

  .. figure:: ../../../../common/_static/part_table1.png
    :align: center
    :alt: part_table1
    :figclass: align-center

    图5 part_table_cmakelists

  - components/part_table/part_table.mk文件修改如下：

  .. figure:: ../../../../common/_static/part_table2.png
    :align: center
    :alt: part_table2
    :figclass: align-center

    图6 part_table_mk

  
  - 工程新建完成之后，需要先执行make clean;然后再进行make；
  - 分区表中size的总和大小，不得超过所用flash的实际Size；


