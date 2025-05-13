三.BLE_OTA测试流程
========================
:link_to_translation:`en:[English]`

3.1.BLE_OTA测试步骤介绍
----------------------------

 BLE_OTA的测试流程主要分为以下步骤：

  - 1）需要在手机上，下载一个BekenTool的APK，可以通过扫描以下的二维码下载；
  - 2）在手机文件管理建立一个文件夹，命名为ble_ota;
  - 3）需使用差分升级，所以需要制作差分包；（如何制作差分包，下面会介绍）
  - 4）把差分包放进步骤2 中ble_ota文件夹中；
  - 5）板子端烧录 all-app.bin；（以scoote工程为例）
  - 6）连上蓝牙；

      - a）点击步骤1中的BekenTool APK（图1）；
      - b）选择板子端对应的ble mac（可以从初始化log中获取）；
      - c）点击相应的mac地址（见图2）；
      - d）然后进行升级升级（见图3）；
      - e）升级结束（见图4）；

  - 7）	升级成功的log;

      f_ota:E(300636):ota_do_check_crc:crc ok
      app_gatt:I(300638):bk_gatts_cb:BK_GATTS_CONF_EVT 0 5 57
      app_boar:W(300638):boarding_message_handle:ota success !
      sys:I(300638):bk_reboot

 .. figure:: ../../../../../common/_static/ble_ota_test.png
    :align: center
    :alt: ble_ota_test
    :figclass: align-center

    图4 ble_ota测试

3.2.BekenTool APK获取方式
----------------------------

 .. figure:: ../../../../../common/_static/Beken_tool.png
    :align: center
    :alt: Beken_tool
    :figclass: align-center

    图5 Beken_tool二维码

3.3.差分包的制作流程
----------------------------
 一、注意点：
  - 差分升级，基于两个版本（原始版本和新的版本(需保存备份好，下次做差分包以它为原始版本)）来制作差分包；

 二、	差分包制作及测试步骤：

  - 1、用如下方法生成 差分包（压缩）： ./diff-compress  app_pack_ori.bin  app_pack_new.bin  diff1.bin  app

    - a） 其中需要两个源文件 app_pack_ori.bin、 app_pack_new.bin，（以bk7258为例 路径是bk_idk/build/app/bk7258/encrypt/app_pack.bin）；
    - b）	源文件1：app_pack_ori.bin；
    - c）	源文件2：app_pack_new.bin，（需备份好，后续做差分包，app_pack_new.bin会作为源文件1）；
    - d）	生成目标文件 ：diff1.bin；
    - e）	其中第一次烧录使用all_app.bin 是和app_pack_ori.bin一同编译出来时的版本；
    - f）	后续的板子，应该不需要烧录，一直迭代差分升级上区，具体操作可见下述图片。

.. note::
    - 1.在linux环境下生成；
    - 2.差分工具路径是：bk_idk/tools/env_tools/diff_ota/diff-compress.o ;


差分包制作步骤图
  .. figure:: ../../../../../common/_static/diff_ota_make1.png
     :align: center
     :alt: diff_ota
     :figclass: align-center

     图6 diff_ota_package图

  - 2、将diff2.bin 作为待升级的固件，放到服务器上。（和正常升级一样）

  - 3、下载成功之后，进入bootloader里面，（可以通过uart1看出差分还原的log进度 如 progress :0%  ），其中正常的还原时间在55s左右（还是要镜像的大小）；
 
  - 4、bootloader中还原成功之后，会重新启动，然后进入app中运行新的固件，可查看version 看是否升成功；

  .. figure:: ../../../../../common/_static/diff_ota_result.png
     :align: center
     :alt: diff_Reduction
     :figclass: align-center

     图7 diff_ota_reduction


