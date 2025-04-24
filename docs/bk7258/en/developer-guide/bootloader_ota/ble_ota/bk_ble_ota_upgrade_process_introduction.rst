二.BLE_OTA Process Introduction
=================================

:link_to_translation:`zh_CN:[中文]`

Place the differential upgrade firmware (considering the slower BLE download speed, differential upgrade is used) on the mobile phone's APK, and then click download. The APK will transmit the upgrade firmware data to the board end (such as BK7258) through the BLE channel. After the data download is complete, the APK sends a CRC command to verify the integrity of the transmitted data. If the CRC passes, the board end will reboot and enter the bootloader to restore the firmware. After the restoration is successful, the board end will restart and run with the new firmware.

2.1.BLE_OTA Upgrade Process Introduction
------------------------------------------

 .. figure:: ../../../../_static/ble_ota_process.png
    :align: center
    :alt: ble_ota_process
    :figclass: align-center

    Fig1 BLE_OTA Flowchart

2.2.BLE_OTA Data Interaction
------------------------------

The data interaction mainly occurs between the APK and the board end, as well as between the board end and the APK. Specifically:

.. important::
  - The first two bytes of the payload are the sequence number, representing the cumulative number of packets transmitted.
  - The payload of the first packet is magic+size (magic: OTA_START (9 bytes); size: the entire image size (4 bytes)).
  - The payload of the last packet is magic+crc (magic: OTA_DOWNLOADED (14 bytes); crc: the CRC value of the entire image size (4 bytes)).
  - The first and last packets' payloads do not contain the two-byte sequence number.

1、	APK to Board End Data Format:

 .. figure:: ../../../../_static/apk_board.png
    :align: center
    :alt: apk_board
    :figclass: align-center

    Fig2 apk_board interaction

2、 Board to APK End Data Format:

In the figures, status represents the result of the operation;0 represents success;Other values represent failure (explained in section 2.3)

 .. figure:: ../../../../_static/board_apk.png
    :align: center
    :alt: board_apk
    :figclass: align-center

    Fig3 board_apk interaction

2.3.BLE_OTA Data Structure Introduction
------------------------------------------

1、**The opcode values for APK and board interaction are as follows**

  - BLE_OTA is divided into three stages: start download, downloading, and download complete. The corresponding opcodes are:OP_OTA_START_DOWNLOAD,OP_OTA_DO_DOWNLOADING,OP_OTA_COMPLETE_DOWNLOAD

 ::

    typedef enum
    {
      BOARDING_OP_UNKNOWN = 0,
      BOARDING_OP_STATION_START = 1,
      BOARDING_OP_OTA_START_DOWNLOAD = 20,
      BOARDING_OP_OTA_DO_DOWNLOADING = 21,
      BOARDING_OP_OTA_COMPLETE_DOWNLOAD = 22,
    } boarding_opcode_t;

2、**The status values returned by the board to the APK have the following meanings**

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

2.4. BLE_OTA Exception Handling Mechanism
----------------------------------------------

- 1.If the board does not respond with an ACK within 6 seconds after sending a package, it is considered a failure.
- 2.If a failure occurs, the APK has a retry mechanism that sends the same sequence number and retries up to 3 times.
