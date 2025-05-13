Automated List Introduction
=============================

:link_to_translation:`zh_CN:[中文]`

Adding New Project to the Automated List
-----------------------------------------------
To add a newly created project named "platform" to the automated list, modify the components/part_table/CMakeLists.txt and components/part_table/part_table.mk files as shown below:

  - Modify components/part_table/CMakeLists.txt as follows:

  .. figure:: ../../../../common/_static/part_table1.png
    :align: center
    :alt: part_table1
    :figclass: align-center

    Fig5 part_table_cmakelists

  - Modify components/part_table/part_table.mk as follows:

  .. figure:: ../../../../common/_static/part_table2.png
    :align: center
    :alt: part_table2
    :figclass: align-center

    Fig6 part_table_mk

  
  - After completing the new project, execute make clean first, then make;
  - The total size of the size fields in the partition table must not exceed the actual size of the used flash.


