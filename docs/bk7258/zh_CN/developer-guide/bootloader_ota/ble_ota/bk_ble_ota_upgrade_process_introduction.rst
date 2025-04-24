二.BLE_OTA流程简介
========================

:link_to_translation:`en:[English]`

将制作好的差分升级固件（考虑到BLE下载速度较慢，所以使用差分升级），放置到手机的APK上，然后点击下载，即APK通过BLE通道将升级固件数据下传到板端（如BK7258）,当数据下载完成之后，APK下发CRC命令来校验下发数据的完整性，若CRC通过，则板端进行reboot并进入bootloader进行还原，还原成功之后，即板端重启到新的固件上进行运行。

2.1.BLE_OTA的升级流程图介绍
----------------------------

 .. figure:: ../../../../_static/ble_ota_process.png
    :align: center
    :alt: ble_ota_process
    :figclass: align-center

    图1 ble_ota流程图片

2.2.BLE_OTA数据交互介绍
----------------------------

BLE_OTA的数据交互，主要体现在APK到板端以及板端到APK端的数据交互。其中、

.. important::
  - payload 中的前两个字节为sequence号。代表，累计下发的包数。
  - 第一包packet的payload为 magic+size（magic：OTA_START（9个字节）；size:镜像整个大小（4个字节））；
  - 最后一包packet的payload为magic+crc（magic: OTA_DOWNLOADED（14个字节）；crc:整个镜像大小的crc值（4个字节））；
  - 第一包和最后一包packet的payload中不含有两字节sequence号；

1、	APK到板端的数据格式：

 .. figure:: ../../../../_static/apk_board.png
    :align: center
    :alt: apk_board
    :figclass: align-center

    图2 apk_board图片

2、板端到APK端的数据格式：

其中，staus: 0代表成功；其他值，代表失败.（2.3节有介绍，各值含义）；

 .. figure:: ../../../../_static/board_apk.png
    :align: center
    :alt: board_apk
    :figclass: align-center

    图3 board_apk图片

2.3.BLE_OTA的数据结构介绍
----------------------------

1、**APK和板端的交互opcode对应值如下**

  - 其中，BLE_OTA细分为三个阶段，分为为开始下载、下载中、下载完成。对应OP_OTA_START_DOWNLOAD、OP_OTA_DO_DOWNLOADING、OP_OTA_COMPLETE_DOWNLOAD；

 ::

    typedef enum
    {
      BOARDING_OP_UNKNOWN = 0,
      BOARDING_OP_STATION_START = 1,
      BOARDING_OP_OTA_START_DOWNLOAD = 20,
      BOARDING_OP_OTA_DO_DOWNLOADING = 21,
      BOARDING_OP_OTA_COMPLETE_DOWNLOAD = 22,
    } boarding_opcode_t;

2、**板端回APK的status对应的值含义如下**

 ::

    typedef enum
    {
      F_OTA_COMM_OK            = 0,
      F_OTA_START_MAGIC_ERROR  = 1,
      F_OTA_FINISH_MAGIC_ERROR = 2,
      F_OTA_COMM_DATA_ERROR    = 3,
      F_OTA_COMM_LENGTH_ERROR  = 4,
      F_OTA_COMM_CRC_ERROR     = 5,
    }f_ota_status_t;

2.4.BLE_OTA异常处理机制介绍
----------------------------
- 1.每包6秒之内，Board->APK没有回ack，即视为失败。；
- 2.若失败，APK会有重试机制，会发送相同的sepence号，重试次数为3次。
