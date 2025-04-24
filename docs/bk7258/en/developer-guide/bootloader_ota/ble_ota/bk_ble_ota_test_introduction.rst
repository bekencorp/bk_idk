三.BLE_OTA Testing Flow
========================
:link_to_translation:`zh_CN:[中文]`

3.1.BLE_OTA Testing Steps Introduction
----------------------------------------

 The BLE_OTA testing flow mainly consists of the following steps:

  - 1) Download the BekenTool APK on your mobile phone by scanning the QR code below;
  - 2) Create a folder named "ble_ota" in your phone's file manager;
  - 3) Use differential upgrade, so you need to create a differential package; (How to create a differential package will be introduced below)
  - 4) Put the differential package into the "ble_ota" folder created in step 2;
  - 5) Burn the all-app.bin file on the board (using the scoote project as an example);
  - 6) Connect to Bluetooth;

      - a) Click on the BekenTool APK (Figure 1);
      - b) Select the corresponding BLE MAC address of the board (which can be obtained from the initialization log); 
      - c) Click on the corresponding MAC address (see Figure 2); 
      - d) Perform the upgrade (see Figure 3); 
      - e) Upgrade completed (see Figure 4);

  - 7) Upgrade success log;

      f_ota:E(300636):ota_do_check_crc:crc ok
      app_gatt:I(300638):bk_gatts_cb:BK_GATTS_CONF_EVT 0 5 57
      app_boar:W(300638):boarding_message_handle:ota success !
      sys:I(300638):bk_reboot

 .. figure:: ../../../../_static/ble_ota_test.png
    :align: center
    :alt: ble_ota_test
    :figclass: align-center

    Fig4 ble_ota test

3.2.BekenTool APK Acquisition Method
---------------------------------------

 .. figure:: ../../../../_static/Beken_tool.png
    :align: center
    :alt: Beken_tool
    :figclass: align-center

    Fig5 Beken_tool QR Code

3.3. Differential Package Creation Process
--------------------------------------------
 一、Precautions:
  - Differential upgrade, based on two versions (original version and new version (need to save backup well, next time make differential package with it as the original version));

 二、Differential Package Creation and Testing Steps:

  - 1、Use the following method to generate a differential package (compressed): ./diff-compress app_pack_ori.bin app_pack_new.bin diff1.bin app

    - a) Two source files are required: app_pack_ori.bin and app_pack_new.bin, (for example, the path for bk7258 is bk_idk/build/app/bk7258/encrypt/app_pack.bin);
    - b) Source file 1: app_pack_ori.bin;
    - c) Source file 2: app_pack_new.bin, (need to backup well, next time make differential package, app_pack_new.bin will be used as source file 1);
    - d) Generate target file: diff1.bin;
    - e) The first time burning uses all_app.bin, which is compiled with app_pack_ori.bin at the same time;
    - f) Subsequent boards do not need to be burned, and can be iteratively upgraded with differential upgrades, specific operations can be seen in the following image.

.. note::
    - 1.Generate in a Linux environment;
    - 2.The differential tool path is: bk_idk/tools/env_tools/diff_ota/diff-compress.o;


Differential Package Creation Steps Diagram
  .. figure:: ../../../../_static/diff_ota_make1.png
     :align: center
     :alt: diff_ota
     :figclass: align-center

     Fig6 diff_ota_package

  - 2、Put diff2.bin as the firmware to be upgraded on the server. (Same as normal upgrade)

  - 3、After downloading successfully, enter the bootloader, (can view the progress of differential restoration log through uart1, such as progress: 0%), where the normal restoration time is around 55s (still depends on the size of the image);
 
  - 4、After successful restoration in the bootloader, it will restart and enter the app to run the new firmware, can check the version to see if the upgrade was successful;

  .. figure:: ../../../../_static/diff_ota_result.png
     :align: center
     :alt: diff_Reduction
     :figclass: align-center

     Fig7 diff_ota_reduction


