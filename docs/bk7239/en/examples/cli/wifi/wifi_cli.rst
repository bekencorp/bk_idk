.. _wifi-cli:

**Wi-Fi Cli Commands**
=========================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *All functions on this page have been replaced by the WIFI-AT commands, IDK recommends using AT commands to operate the wifi.*

    *All functions on this page are exclusively for users who are still using CLI commands to operate WIFI.*

- :ref:`ap<wifi-ap>`             :Create SoftAP with SAP identity

- :ref:`ip/ipconfig<wifi-cliip>` : View the current mode status of the WiFi device or specific parameter information.

- :ref:`ping<wifi-cliping>`      :Perform PING operations with designated IP address and package quantity.

- :ref:`scan<wifi-cliscan>`      :Scan the surrounding AP information, or scan according to specified parameters.

- :ref:`sta<wifi-clista>`        :Connect to the router designated by the specified SSID/BSSID as a STA.

- :ref:`iperf<wifi-cliiperf>`      :Execute the iperf command in a certain identity.

.. _wifi-ap:

:ref:`ap<wifi-cli>` **:Create SoftAP with SAP identity**
------------------------------------------------------------------

**Command** ::

    ap ssid [password] [channel[1:14]]

**Params** ::
    
    ssid: The established softAP’s SSID (required).

    password：The password of the created softAP (optional)

    channel： The channels established for the softAP (optional)

**Usage Scenario** ::
    
    ap test123: Create a SoftAP with a mode set to OPEN and a name of test123.

    ap test123 12345678：Start to establish an encrypted softAP with the pattern name test123.

    ap test123 12345678  8：Establish an encrypted softAP with the pattern name "test123", and the channel number is 8.


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *If CONFIG_SOFTAP_WPA3 is enabled, the chip will default to setting up a Wi-Fi SoftAP with WPA3 encryption level; otherwise, it will establish a Wi-Fi SoftAP with WPA2 encryption level!*

    *If CONFIG_SOFTAP_WPA3 is enabled, it does not support setting up a SoftAP with WPA2 encryption level!*


.. _wifi-cliip:

:ref:`ip/ipconfig<wifi-cli>`  **:View the current mode status of the WiFi device or specific parameter information**
-------------------------------------------------------------------------------------------------------------------------

**Command** ::

    ip [sta|ap][{ip}{mask}{gate}{dns}]

    ipconfig [sta|ap][{ip}{mask}{gate}{dns}]

**Params** ::
    
    sta|ap: Indicate that the current operation identity is STA|AP identity.

    ip：ip address

    mask：mask address

    gate:gate address

    dns：dns address

**Usage Scenario** ::
    
    ip/ipconfig:None param
    
       Display network information showing the current board's STA and AP identities::

            netif(sta) ip4=192.168.1.229 mask=255.255.255.0 gate=192.168.1.1 dns=192.168.1.1
            netif(ap)  ip4=192.168.188.1 mask=255.255.255.0 gate=192.168.188.1 dns=0.0.0.0


    ip/ipconfig sta|ap ：1 param
    
        Display network information showing the current board's STA or AP identities::

            STA:
                netif(sta) ip4=192.168.1.229 mask=255.255.255.0 gate=192.168.1.1 dns=192.168.1.1
            AP:
                netif(ap)  ip4=192.168.188.1 mask=255.255.255.0 gate=192.168.188.1 dns=0.0.0.0


    ip/ipconfig sta|ap  ip gate mask dns ：5 params

        Set the network information for the current board to STA or AP identity::

            STA:
                set static ip,  netif(sta) ip4=ip mask=mask gate=gate dns=dns
            AP:
                set static ip,  netif(ap)  ip4=ip mask=mask gate=gate dns=dns

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect when CONFIG_BK_NETIF is enabled!*

.. _wifi-cliping:

:ref:`ping<wifi-cli>` **:Perform PING operations with designated IP address and package quantity**
----------------------------------------------------------------------------------------------------------

**Command** ::

    ping [ip|--stop] [cnt]

**Params** ::
    
    ip: The network address to ping, used for initiating the ping operation (required).

    --stop:Stop the current ping operation, used for ending the ping action (required).

    cnt：The number of packets to be pinged (optional)

**Usage Scenario** ::
    
    ping 192.168.0.1: Start the ping operation on the 192.168.0.1 address.

    ping 192.168.0.1 1000：Start the ping operation on the address 192.168.0.1, and a total of 1000 packets will be pinged.

    ping –stop：Terminate the ongoing ping operation.


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command requires enabling CONFIG_LWIP!*

.. _wifi-cliscan:

:ref:`scan<wifi-cli>` **:Scan the surrounding AP information, or scan according to specified parameters.**
--------------------------------------------------------------------------------------------------------------

**Command** ::

    scan [ssid]

**Params** ::
    
    ssid: The SSID of the router to be scanned

**Usage Scenario** ::
    
    scan:none param,start scanning all APs around and print the scan results.

    scan aclsemi ：1 param,Start scanning the surrounding APs named "aclsemi" and print the scan results.

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _wifi-clista:

:ref:`sta<wifi-cli>` **:Connect to the router designated by the specified SSID/BSSID as a STA**
----------------------------------------------------------------------------------------------------

**Command** ::

    sta ssid [password][bssid]

**Params** ::
    
    ssid: the SSID of connect router (required).

    password:the password of connect router(optional)

    bssid：When enabling BSSID connection, fill in the BSSID of the router you want to connect to (optional).

    channel：The channel of the router you want to connect to(optional).

**Usage Scenario** ::
    
    sta aclsemi:Begin to connect to the AP with the SSID of "aclsemi."

    sta aclsemi 12345678 ：Start encrypting the connection to the AP with the SSID of aclsemi.

    sta aclsemi 12345678 c758-7eac-a032  ：Start the encrypted connection to the AP with SSID as 'aclsemi' and BSSID as 'c758-7eac-a032'.

.. note::

        *When using ssid only, it is necessary to confirm that router is in OPEN mode; otherwise, it will continuously report ReasonCode 258 (wrong password)!*

        *No confirmation is needed for router to be in encrypted mode when using the password, and it can be omitted if the router connected is an OPEN router!*

        *When using the BSSID, it indicates that there are multiple identical SSID routers in the current environment. The module will be connecting to a certain specified AP,In this case, the SSID cannot be ignored!*

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _wifi-cliiperf:

:ref:`iperf<wifi-cli>` **:Execute the iperf command in a certain identity**
----------------------------------------------------------------------------------------------

**Command** ::

    iperf –c ip –i1 –t60(Time customization)
    
    iperf –s –i1
    
    iperf –c ip –u –b –i1 -t t60(Time customization)
    
    iperf –s –u –i1
    
    iperf --stop 


**Params** ::
    
    -c : Client mode, the host is the server's address.eg：iperf -c 222.35.11.23

    -s : server mode，eg：iperf -s

    ip ：The IP address obtained after the connection or the IP address of the router on the computer end.

    -i1：The report interval is displayed in seconds by the sec unit.eg：iperf -c 222.35.11.23 -i 2

    -u : Using the UDP protocol, it represents that the current running mode is UDP transmission.

    --stop :End the iperf execution program

    -t：Test duration, default 10 seconds,eg：iperf -c 222.35.11.23 -t 5

    -b：The client sends a specified bandwidth size, universal setting -b 40M.


**Usage Scenario** ::

    Firstly, confirm the STA status. Before executing iperf, you need to run the command through the serial port: sta ssid key. 
    
    If STA is supported, the serial port will return the IP address: 192.168.XX.XX. At this time, you can check the network status through the IP.
    
    TCP Method：

        Enter ：
            server ：iperf –s –i 1 

            client ：iperf -c 192.168.xx.xx -t30 -i1 
    
    UDP Method：

       UDP server operation method (computer end acting as the client side, sending; BK72xx board acting as the server side, receiving)：
        
        Enter ：

            server ：iperf –s -u -i 1 

            client ：iperf -c 192.168.xx.xx –u –b40M -t30 -i1 

        UDP client operation methods (Note: the BK72xx board is used as the client side and the sender):
        
        Enter ：
    
            server ：iperf –s -u -i 1 
    
            client ：iperf -c 192.168.xx.xx –u -t30 -i1 

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR