.. _sd-cli:

**SD Card cli Command**
============================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *The sd_card feature requires testing using the fatfstest command. The sd_card test cases need to enable the CONFIG_FATFS macro.*

- :ref:`fatfstest M<sd-sdM>`                  :Mount operation on the SD card.

- :ref:`fatfstest U<sd-sdU>`                  :Unplug the SD card.

- :ref:`fatfstest G<sd-sdG>`                  :Check the current remaining space on the SD card.

- :ref:`fatfstest S<sd-sdS>`                  :Scan SD card files

- :ref:`fatfstest F<sd-sdF>`                  :Format SD card device

- :ref:`fatfstest R<sd-sdR>`                  :Read files from the SD card

- :ref:`fatfstest W<sd-sdW>`                  :Write content to a file on the SD card.

- :ref:`fatfstest D<sd-sdD>`                  :dump Transfer the contents of the SD card to a space.

- :ref:`fatfstest A<sd-sdA>`                  :Automatic test file writing

.. _sd-sdM:

:ref:`fatfstest M<sd-cli>` **:Mount operation on the SD card.**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest M {DISK_NUMBER} 

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

**Usage Scenario** ::

    Input：
    
        fatfstest M 1：Mount SD card

    Success of mounting will display "f_mount OK", the printed content is as follows:

    (68861838):error file name,use defaultfilename.txt
    
    sdio_hos:I(68861838):dst start addr=0x458d003c
    
    sdio_hos:I(68861838):s_sdio_host.dma_tx_id = 0.

    sdio_hos:I(68861838):s_sdio_host.dma_tx_id = 12.

    sdio_hos:I(68861838):s_sdio_host.dma_rx_id = 13.

    sd_card:I(68861840):sdio host init ok, clock_freq:7

    sd_card:I(68861922):sd card support voltage 2.7-3.6 V

    sd_card:I(68861990):card capacity SDHC_SDXC

    sd_card:I(68861998):sd card class:0x5b5

    sd_card:I(68862010):sdio clock freq:7->11

    (68862014):fmt=2

    (68862016):fmt2=1

    Fatfs:I(68862016):f_mount OK!

    Fatfs:I(68862016):----- test_mount 1 over  -----

.. note::

    *Print error: the file name, use defaultfilename.txt, is due to the test command scanning the defaultfilename.txt file; if this file does not exist, it will print an error*

.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _sd-sdU:

:ref:`fatfstest U<sd-cli>` **:Unplug the SD card**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest U {DISK_NUMBER}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

**Usage Scenario** ::

    Input：
    
        fatfstest U 1：Unplug the SD card

    The successful unmount will display "f_unmount OK," and the printed content will be as follows:
    
    fatfstest U 1
    
    $(404554):error file name,use defaultfilename.txt
    
    (404554):f_unmount OK!
    
    (404554):----- test_unmount 1 over  -----
    
    (404554):unmount:sdio_sd

.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _sd-sdG:

:ref:`fatfstest G<sd-cli>` **:Check the current remaining space on the SD card**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest G {DISK_NUMBER}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

**Usage Scenario** ::

    Input：
    
        fatfstest G 1：Check the current remaining space on the SD card

    A successful query will display "f_getfree OK!," and the printed content will be as follows:

    sdio_hos:I(69876174):dst start addr=0x458d003c
    
    sdio_hos:I(69876174):s_sdio_host.dma_tx_id = 12.
    
    sdio_hos:I(69876174):s_sdio_host.dma_rx_id = 13.
    
    sd_card:I(69876174):sdio host init ok, clock_freq:7
    
    sd_card:I(69876254):sd card support voltage 2.7-3.6 V
    
    sd_card:I(69876324):card capacity SDHC_SDXC
    
    sd_card:I(69876334):sd card class:0x5b5
    
    sd_card:I(69876344):sdio clock freq:7->11
    
    (69876538):test_getfree getnclst:DEC 949435 free space: 59339MB
    
    (69876540):f_getfree OK!
    
    (69876540):----- test_getfree 1 over  -----
    
    (69876540):getfree:sdio_sd


.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _sd-sdS:

:ref:`fatfstest S<sd-cli>` **:Scan SD card files**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest S {DISK_NUMBER}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

**Usage Scenario** ::

    Input：
    
        fatfstest G 1：Scan SD card files

    Successful scanning will display "scan_files OK!", and the printed content is as follows:
    
    Fatfs:I(70912942):
    
    ----- scan_file_system 1 start -----
    
    Fatfs:I(70912942):1:/
    
    sdio_hos:I(70912942):dst start addr=0x458d003c
    
    sdio_hos:I(70912942):s_sdio_host.dma_tx_id = 12.
    
    sdio_hos:I(70912944):s_sdio_host.dma_rx_id = 13.
    
    sd_card:I(70912944):sdio host init ok, clock_freq:7
    
    sd_card:I(70913024):sd card support voltage 2.7-3.6 V
    
    sd_card:I(70913094):card capacity SDHC_SDXC
    
    sd_card:I(70913104):sd card class:0x5b5
    
    sd_card:I(70913112):sdio clock freq:7->11
    
    Fatfs:I(70913118):1:/test.txt
    
    Fatfs:I(70913118):1:/autotest_1.txt
    
    Fatfs:I(70913118):1:/autotest_2.txt
    
    Fatfs:I(70913118):1:/autotest_3.txt
    
    Fatfs:I(70913118):1:/dump_psram.txt
    
    $Fatfs:I(70913118):1:/dump_2.txt
    
    Fatfs:I(70913118):scan_files OK!
    
    Fatfs:I(70913118):----- scan_file_system 1 over  -----
    
    (70913118):scan



.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _sd-sdF:

:ref:`fatfstest F<sd-cli>` **:Format SD card device**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest F {DISK_NUMBER}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

**Usage Scenario** ::

    Input：
    
        fatfstest F 1：Format SD card device

    The formatting was successful and displayed "f_mkfs OK!", the output content is as follows:

    sdio_hos:I(439300):dst start addr=0x458d003c

    sdio_hos:I(439300):s_sdio_host.dma_tx_id = 12.

    sdio_hos:I(439300):s_sdio_host.dma_rx_id = 13.

    sd_card:I(439302):sdio host init ok, clock_freq:7

    sd_card:I(439384):sd card support voltage 2.7-3.6 V

    sd_card:I(439472):card capacity SDHC_SDXC

    sd_card:I(439484):sd card class:0x5b5

    sd_card:I(439492):sdio clock freq:7->11

    (439496):part=0

    sd_card:I(439496):card ver=2.0,size:0x073e8000 sector(sector=512bytes)

    (439496):sdcard sector cnt=121536512

    Fatfs:I(441532):f_mkfs OK!

    (441532):format :1



.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR


.. _sd-sdR:

:ref:`fatfstest R<sd-cli>` **:Read files from the SD card**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest R {DISK_NUMBER} {file_name} {length}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

    file_name: file name

    length: read length

**Usage Scenario** ::

    In the case of the SD card mounted, input:

       fatfstest R 1 test.txt 10

   On successful read, the file content will be output, and the printed content is as follows:
    
    ----- test_fatfs 1 start -----
    
    Fatfs:I(71732504):f_open "1:/test.txt"
    
    sdio_hos:I(71732504):dst start addr=0x458d003c
    
    sdio_hos:I(71732506):s_sdio_host.dma_tx_id = 12.
    
    sdio_hos:I(71732506):s_sdio_host.dma_rx_id = 13.
    
    sd_card:I(71732506):sdio host init ok, clock_freq:7
    
    sd_card:I(71732588):sd card support voltage 2.7-3.6 V
    
    sd_card:I(71732656):card capacity SDHC_SDXC
    
    sd_card:I(71732666):sd card class:0x5b5
    
    sd_card:I(71732676):sdio clock freq:7->11
    
    Fatfs:I(71732680):will read left_len = 10 
    
    ====== f_read one cycle - dump(len=10) begin ======
    
    61 63 6c 5f 62 6b 37 32 35 38 
    
    ====== f_read one cycle - dump(len=10)   end ======
    
    $Fatfs:I(71732680):f_read start:10 bytes 
    
    Fatfs:I(71732688):f_read one cycle finish:left_len = 0
    
    Fatfs:I(71732688):f_read: read total byte = 10
    
    Fatfs:I(71732688):f_close OK
    
    Fatfs:I(71732688):----- test_fatfs 1 over  -----

.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _sd-sdW:

:ref:`fatfstest W<sd-cli>` **:Write content to a file on the SD card**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest W {DISK_NUMBER} {file_name} {content} {length}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

    file_name: File name

    content: Write content

    length: Content length to be written, up to 64 bytes at a time

**Usage Scenario** ::

    In the case of SD card mounted, input:
    
       fatfstest W 1 autotest.txt Aclsemi 7:Write the file content "Aclsemi" to autotest.txt

    A successful write will output "append and write:autotest.txt,Aclsemi," with the following printed content:

    Fatfs:I(82556592):
    
    ----- test_fatfs 1 start -----
    
    Fatfs:I(82556592):f_open "1:/autotest.txt"
    
    sdio_hos:I(82556594):dst start addr=0x458d003c
    
    sdio_hos:I(82556594):s_sdio_host.dma_tx_id = 12.
    
    sdio_hos:I(82556594):s_sdio_host.dma_rx_id = 13.
    
    sd_card:I(82556594):sdio host init ok, clock_freq:7
    
    sd_card:I(82556676):sd card support voltage 2.7-3.6 V
    
    sd_card:I(82556752):card capacity SDHC_SDXC
    
    sd_card:I(82556764):sd card class:0x5b5
    
    sd_card:I(82556772):sdio clock freq:7->11
    
    Fatfs:I(82556778):.Fatfs:I(82556794):f_close OK
    
    Fatfs:I(82556794):----- test_fatfs 1 over  -----
    
    $(82556794):append and write:autotest.txt,Aclsemi


.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR    


.. _sd-sdD:

:ref:`fatfstest D<sd-cli>` **:dump Transfer the contents of the SD card to a space**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest D {DISK_NUMBER} {file_name} {start_addr} {length}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card

    file_name: file name related

    start_addr: dump start address

    length:content length

**Usage Scenario** ::

    In the case of SD card mounted, input:
    
       fatfstest D 1 autotest.txt 0x23da000 7

    Successful dump of the following content:
    
    Fatfs:I(116672):

    ----- test_fatfs_dump 1 start -----
    
    Fatfs:I(116672):file_name=test.txt,start_addr=0x23da000,len=16 
    
    Fatfs:I(116672):f_open start "1:/test.txt"
    
    sdio_hos:I(116674):dst start addr=0x458d003c
    
    sdio_hos:I(116674):s_sdio_host.dma_tx_id = 12.
    
    sdio_hos:I(116674):s_sdio_host.dma_rx_id = 13.
    
    sd_card:I(116674):sdio host init ok, clock_freq:7
    
    sd_card:I(116756):sd card support voltage 2.7-3.6 V
    
    sd_card:I(116770):card capacity SDHC_SDXC
    
    sd_card:I(116780):sd card class:0x5b5
    
    sd_card:I(116788):sdio clock freq:7->11
    
    Fatfs:I(116794):f_write start
    
    Fatfs:I(116794):f_write end len = 16
    
    Fatfs:I(116794):f_lseek start
    
    Fatfs:I(116794):f_close start



.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR    


.. _sd-sdA:

:ref:`fatfstest A<sd-cli>` **:Automatic test file writing**
----------------------------------------------------------------------------------------------

**Command** ::

    fatfstest A {DISK_NUMBER} {file_name} {length} {test_cnt} {start_addr}

**Params** ::
    
    DISK_NUMBER：Disk number, 1 indicates SD card 

    file_name: file name to be written

    length:content length

    test_cnt:Auto-loop writing times

    start_addr:start address

**Usage Scenario** ::

    In the case of SD card mounted, input:
    
       fatfstest A 1 autotest1.txt 1024 1 0x23da000

    Test successful, output the following content:

    sdio_hos:I(28686):dst start addr=0x458d003c
    
    sdio_hos:I(28686):s_sdio_host.dma_tx_id = 12.
    
    sdio_hos:I(28686):s_sdio_host.dma_rx_id = 13.
    
    sd_card:I(28686):sdio host init ok, clock_freq:7
    
    sd_card:I(28768):sd card support voltage 2.7-3.6 V
    
    sd_card:I(28782):card capacity SDHC_SDXC
    
    sd_card:I(28794):sd card class:0x5b5
    
    sd_card:I(28802):sdio clock freq:7->11
    
    Fatfs:I(28814):auto test succ




.. note::

    *The sd_card test cases require the CONFIG_FATFS macro to be enabled.*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR    

