.. _wifi-AT:

**Wi-Fi AT Commands**
=======================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *By default, the Armino supports all the AT commands described in this section*


- :ref:`AT+WLMAC<wifi-mac>` :Get the MAC address of STA identity and SAP identity, you can also configure the MAC address

- :ref:`AT+STASTART<wifi-stastart>` :Connect to the router with the specified SSID as STA

- :ref:`AT+STASTOP<wifi-stastop>` :Disconnect from the router that is currently connected as STA 

- :ref:`AT+WIFISTATUS<wifi-getstatus>` :It is used to view the current mode status of the wifi device or specify parameter information

- :ref:`AT+WIFISCAN<wifi-scan>` :Scan for neighbor AP information

- :ref:`AT+SAPSTART<wifi-sapstart>` :Create SoftAP as SAP

- :ref:`AT+SAPSTOP<wifi-stastop>` :Stop current SoftAP

- :ref:`AT+STAPING<wifi-pingstart>` :PING packets with a specified IP address, a specified duration, and a specified packet size

- :ref:`AT+STAPINGSTOP<wifi-pingstop>` :Stop the PING packet that is being pinged


.. _wifi-mac:

:ref:`AT+WLMAC<wifi-AT>` **:Get the MAC address of STA identity and SAP identity,also configure the MAC address**
--------------------------------------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+WLMAC 
    AT+WLMAC = mac_addr(Optional)

**Params** ::

    No param：Gets the STA/SAP MAC address of the current device
    
    mac_addr(Optional):configure the MAC address of this device

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |     None       |        None         |       
    +---------------+----------------+---------------------+

.. _wifi-stastart:

:ref:`AT+STASTART<wifi-AT>`  **:Connect to the router with the specified SSID as STA**
----------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

     AT+STASTART = ssid, pwd(Optional)

**Params** ::

    SSID：SSID of the AP router to be connected as STA
    
    pwd:  The AP password is ignored if the AP is OPEN

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      20s       |        None         |       
    +---------------+----------------+---------------------+

**AT Event** ::

    EVT:WLAN STA CONNECTED

    CONNECTED SSID: xxxx(AP SSID)

    EVT:GOT-IP

    sta got ip:192.168.xx.xx


.. _wifi-stastop:

:ref:`AT+STASTOP<wifi-at>` **:Disconnect from the router that is currently connected as STA**
-----------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+STASTOP

**Params** ::

    None

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      None      |        None         |       
    +---------------+----------------+---------------------+

**AT Event** ::

    EVT:WLAN STA DISCONNECTED

    EVT:DISCONNECT REASON:xxx(REASON CODE:xx)(Reasons in 802.11 protocol)

    local generated:yes/no(Indicate whether the disconnection was initiated by STA itself)

.. _wifi-getstatus:

:ref:`AT+WIFISTATUS<wifi-at>` **:view the current mode status of the wifi device or specify parameter information**
----------------------------------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

   AT+WIFISTATUS
   AT+WIFISTATUS = STA/AP
   AT+WIFISTATUS = STA/AP,param

**Params** ::

    No param：View the current mode status of the wifi device
    
    RSSI\BSSID\SSID\IP\CHANNEL:Query current status by a specified type.

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      None      |        None         |       
    +---------------+----------------+---------------------+

**AT Event** ::

    No Param:
            EVT:'sta: x, ap: x (1:The corresponding identity has been enabled；0:The corresponding identity has not been enabled)
            [KW:]sta:rssi=xxx,aid=xx,ssid=xxxx,bssid=xx:xx:xx:xx:xx:xx,channel=xx,cipher_type=xxx(STA is started)

        One Param：
            STA Status：
            CMDRSP::STA_WIFI_CONNECT
            CMDRSP::STA_WIFI_DISCONNECT

            AP Status：
            CMDRSP::AP_WIFI_START
            CMDRSP::AP_WIFI_CLOSE

         Two Params：
            STA：
                RSSI:    CMDRSP:sta_rssi:xxx
                CHANNEL: CMDRSP:sta_chnl:xx
                BSSID：  CMDRSP:sta_bssid:xx:xx:xx:xx:xx:xx
                SSID：   CMDRSP:sta_ssid::xxx
                IP：     CMDRSP:STA_IP=192.168.xx.xx,GATE=192.168.xx.xx,MASK=xxx.xxx.xxx.x,DNS=xxx.xx.xx.

            AP：
                SSID:    CMDRSP:ap_ssid:xxx
                CHANNEL: CMDRSP:ap_chnl:xx
                SECURITY:CMDRSP:ap_security:xxx


.. _wifi-scan:

:ref:`AT+WIFISCAN<wifi-at>` **:Scan for neighbor AP information**
----------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+WIFISCAN
    AT+WIFISCAN = param

**Params** ::

    No param：The device scans all surrounding AP information and prints it
    
    param:ssid scan by specified SSID。

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      7ms       |        None         |       
    +---------------+----------------+---------------------+

**AT Event** ::

    EVT:WLAN STA SCAN_DONE


.. _wifi-sapstart:

:ref:`AT+SAPSTART<wifi-at>` **:Create SoftAP as SAP**
--------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+SAPSTART=param1
    AT+SAPSTART=param1,param2(Optional)
    AT+SAPSTART=param1,param2,param3

**Params** ::

    param1：The SSID string you want to create SoftAP
    
    param2 :pwd,     The password for the SoftAP you want to create, ignore this option if you want to set SAP to OPEN
            Channel, when you do not set a password or set a password length of less than 3 characters,
                     will be considered as a channel parameter processing.

    param3:channel,  when you set the SSID and password, the third parameter is the set channel value.

.. note::

    SSID length should not exceed 32 characters, exceeding 32 characters will result in a failure response!

    The password should be set between 8 and 64 bytes in length. If it is less than 8 bytes, it will be treated as OPEN. If it exceeds 64 bytes, the creation will fail!


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      None      |        None         |       
    +---------------+----------------+---------------------+

**AT消息** ::

    Print when a STA is connected to this SAP
    EVT:WLAN SAP CONNECTED + MAC Address which belongs to STA


.. _wifi-sapstop:

:ref:`AT+SAPSTOP<wifi-at>` **:Stop current SoftAP**
--------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+SAPSTOP

**Params** ::

    None

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      None      |        None         |       
    +---------------+----------------+---------------------+


.. _wifi-pingstart:

:ref:`AT+STAPING<wifi-at>` **:PING packets with a specified IP address, a specified duration, and a specified packet size**
------------------------------------------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+STAPING=param1,param2,param3

**Params** ::

    param1:Target IP Address
    param2:Packet numbers
    param3:size of each packet

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      None      |        None         |       
    +---------------+----------------+---------------------+

.. _wifi-pingstop:

:ref:`AT+STAPINGSTOP<wifi-at>` **:Stop the PING packet that is being pinged**
-----------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+STAPINGSTOP

**Params** ::

    None

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+---------------------+
    |  Timeout      |  Time-out time |  Time-out callback  | 
    +===============+================+=====================+
    |     None      |      None      |        None         |       
    +---------------+----------------+---------------------+
