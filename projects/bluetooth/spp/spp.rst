SPP
=======================================

1. Project Overview
--------------------------

    This project show how to use Bluetooth SPP protocol to interact between two development boards. 

2. Hardware Requirements
----------------------------

    Beken development boards.


3. Build
----------------------------

    Enter the following command in the Armino root directory to compile:

    > make bk72xx PROJECT=bluetooth/spp

4. Connect UART
----------------------------

    UART0 is used for:

    - BKFIL to download images
    - Logging and Cli commands.

5. Flash all-app.bin
----------------------------

    Flash Armino/build/spp/bk72xx/all-app.bin to the board by the tool BKFIL.

6. Work flow chart
----------------------------------
 spp work flow chart:

 .. figure:: ../../../../_static/bt_spp_flow_chart.png
    :align: center
    :alt: bt spp demo work flow chart
    :figclass: align-center

    Figure 1. spp demo work flow

7. Cli Command
----------------------------------
    | The project spp supports the following commands through uart0:
    | ``spp help`` : get the list of commands.
    | ``spp init`` : init spp protocol.
    | ``spp start_server`` : start as a spp server.
    | ``spp stop_server [local_channel]`` : stop a spp server.
    | ``spp conn [xx:xx:xx:xx:xx:xx]`` : connect to the device, the parameter is the remote’s address.
    | ``spp disconn [xx:xx:xx:xx:xx:xx] [spp_handle]`` : disconnect to the connected device.
    | ``spp write [handle][data]`` : send data.
    | ``spp rate [handle][length]`` : spp throughput test cmd.

7.1 Command Log
***********************************
- spp init
    init spp protocol

::

    [13:58:03.240]收←◆spp init
    os:I(4358):create shell_handle, tcb=2802b758, stack=[28029f30-2802b730:6144], prio=5
    CLI BT RSP:OK

    $cli:I(4358):bk_cli_bt_spp_callback spp init status:0

- spp start_server
    spp work as server after registed in sdp database. 

::

    [14:10:10.816]发→◇spp start_server
    □
    [14:10:10.820]收←◆spp start_server
    os:I(731944):create shell_handle, tcb=2802a160, stack=[28041fb0-280437b0:6144], prio=5
    CLI BT RSP:OK

    $bt:I(731946):SPP_INT | bt_spp_start_srv get_db_chl:1, rec_hand:0x90001, SPP_HANDLE:0x00
    cli:I(731946):bk_cli_bt_spp_callback, spp start_server success, chnl:1, spp_handle:0x00 

- spp stop server [local_channel]
    | stop server, unregist from sdp database, the parameter local_channel got from start_server command.
    | 'bk_cli_bt_spp_callback, spp start_server success, ``chnl:1``, spp_handle:0x00'

::

    [14:49:28.313]发→◇spp stop_server 1
    □
    [14:49:28.316]收←◆spp stop_server 1
    os:I(501332):create shell_handle, tcb=2802a160, stack=[28041fb0-280437b0:6144], prio=5
    CLI BT RSP:OK

    $bt:I(501334):SPP_INT | bt_spp_stop_srv spp_stop res:0
    bt:E(501334):SPP_INT | bt_spp_stop_auto_cnf is_server:1
    cli:I(501334):bk_cli_bt_spp_callback, spp stop_server success, handle: 0x0, chnl:1 

- spp conn [xx:xx:xx:xx:xx:xx]
    connect to a spp server

::

    [14:55:56.968]发→◇spp conn C8:47:8C:0B:DC:08

    [14:55:56.971]收←◆spp conn C8:47:8C:0B:DC:08
    os:I(16236):create shell_handle, tcb=2802b6a8, stack=[28029e08-2802b608:6144], prio=5
    CLI BT RSP:OK

    $bt:E(16238):hci_alloc_remote_device:[HCI] Allocating new Remote Device. Index = 0 c8:47:8c:0b:dc:08 0


    $
    [14:55:57.463]收←◆bt:E(16730):hci_connection_complete_event:[HCI] Received Connection Complete 0x03. Link 0x01, Status 0x00 c8:47:8c:0b:dc:08

    bt:I(16732):HCI_CONNECTION_COMPLETE_EVENT conn_handle: 80 

    [14:55:57.566]收←◆bt:I(16832):SDP_INT | bt_sdp_internal_common_cb, addr 8 dc b 8c 47 c8 
    bt:E(16832):sdp_request_bh:[SDP] Connection Entity Exists for BD_ADDR


    [14:55:57.599]收←◆bt:I(16866):SDP_INT | bt_sdp_internal_common_cb, addr 8 dc b 8c 47 c8 
    bt:I(16866):SDP_INT | SDP CB: response -> sta:0, len:21 
    bt:I(16866):SPP_INT | Remote Server Channel 0x01 
    bt:E(16866):BT_sdp_get_Pointer_to_next_AttributeList:[SDP] FAILED to Extract Data Sequence for Next Record

    cli:I(16866):bk_cli_bt_spp_callback, spp discover success, chnl0:1, cnt:1 !!
    bt:I(16868):SPP_INT | bt_spp_conn get_db_chl:1, rec_hand:0x90001
    bt:E(16868):sdp_request_bh:[SDP] Connection Entity Exists for BD_ADDR

    bt:E(16868):spp_rfcomm_callback:[SPP] FAILED to Find SPP Instance from RFCOMM Handle

    bt:E(16872):sm_device_link_key_and_type:[SM] Link Key is NOT Available for the specified Device

    bt:I(16876):SDP_INT | bt_sdp_internal_common_cb, addr 8 dc b 8c 47 c8 
    bt:I(16876):SDP_INT | SDP CB : SDP PROCEDURE DONE... 

    bt:I(17436):SPP_INT | SPP_CONNECT_CNF -> 0x0000
    bt:I(17436):SPP_INT | SPP Instance Connected : 0
    bt:I(17436):SPP_INT | Remote device ADDR: C8:47:8C:0B:DC:08 
    cli:I(17436):bk_cli_bt_spp_callback, spp conn success to 0x08:0xdc:0x0b:0x8c:0x47:0xc8
    cli:I(17438):HANDLE: 0x00 

- spp disconn [xx:xx:xx:xx:xx:xx] [spp_handle] 
    | disconnect to a spp device, parameter spp_handle got from connected command's log.
    | 'cli:I(17438): ``HANDLE: 0x00``'

::

    [15:09:50.365]发→◇spp disconn C8:47:8C:0B:DC:08 0
    [15:09:50.368]收←◆spp disconn C8:47:8C:0B:DC:08 0
    os:I(849626):create shell_handle, tcb=2802a2f8, stack=[28041cd0-280434d0:6144], prio=5
    CLI BT RSP:OK
    [15:09:50.457]收←◆bt:I(849716):SPP_INT | SPP_DISCONNECT_CNF -> Disconnection Successful
    bt:I(849716):SPP_INT | Remote device ADDR: C8:47:8C:0B:DC:08 
    cli:I(849716):bk_cli_bt_spp_callback, spp disconn success 

- spp write [handle][data]
    | spp send data, parameter handle got from connected command's log.
    | 'cli:I(17438): ``HANDLE: 0x00``'

::

    //client log:
    [15:17:25.648]发→◇spp write 00 111122221111
    □
    [15:17:25.652]收←◆spp write 00 111122221111
    os:I(1304902):create shell_handle, tcb=2802a720, stack=[28041cd0-280434d0:6144], prio=5
    CLI BT RSP:OK

    $cli:I(1304904):bk_cli_bt_spp_callback, spp write cnf, handle 0x00

    $
    [15:17:51.204]收←◆app_time:I(1330458):
    Time timeout obtained from the network

    [15:18:18.798]收←◆cli:I(1358050):===========DATA IND===========
    cli:I(1358050):bk_cli_bt_spp_callback, spp data ind, handle:0x00, len:7 
    cli:I(1358050):aaaabbb 
    cli:I(1358050):==============================

    //server log:
    [15:17:25.655]收←◆cli:I(1302824):===========DATA IND===========
    cli:I(1302824):bk_cli_bt_spp_callback, spp data ind, handle:0x00, len:12 
    cli:I(1302824):111122221111 
    cli:I(1302824):==============================

    [15:17:53.256]收←◆app_time:I(1330424):
    Time timeout obtained from the network

    [15:18:18.785]发→◇spp write 00 aaaabbb
    □
    [15:18:18.788]收←◆spp write 00 aaaabbb
    os:I(1355954):create shell_handle, tcb=28029ef8, stack=[28041cd0-280434d0:6144], prio=5
    CLI BT RSP:OK

    $cli:I(1355956):bk_cli_bt_spp_callback, spp write cnf, handle 0x00

- spp rate [length]
    spp throughput test command, parameter length is the data length to be send, the data is randomly generated.

::

    //tx side log
    $cli:I(19382):========spp tx start total_length: 524287 ========

    [19:33:06.024]收←◆cli:I(22038):spp tx length: 524287, speed: 192.771KB/s 
    cli:I(22038):========spp tx finish tx_length: 524287, crc:0x5c512cd ========

    //rx side log
    [19:33:03.381]收←◆cli:I(48944):======== spp rx start ========
    [19:33:06.088]收←◆cli:I(51650):========spp rx finish tx_length: 524287, speed: 189.209KB/s, crc:0x5c512cd========


- cli command source code please go to ``projects/bluetooth/spp``