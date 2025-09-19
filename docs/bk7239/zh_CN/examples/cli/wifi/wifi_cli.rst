.. _wifi-cli:

**Wi-Fi cli命令**
=======================

:link_to_translation:`en:[English]`


.. important::

    *本界面所有功能均以被WIFI-AT命令替代，IDK推荐使用AT命令来操作wifi*

    *本界面所有功能仅面向一些仍在使用cli命令操作WIFI的用户使用*

- :ref:`ap<wifi-ap>`             :以SAP身份创建SoftAP

- :ref:`ip/ipconfig<wifi-cliip>` :用来查看wifi设备当前的模式状态或指定参数信息

- :ref:`ping<wifi-cliping>`      :进行指定IP地址和指定时长和每包大小的PING包操作

- :ref:`scan<wifi-cliscan>`      :扫描周围AP信息,或者按照指定参数进行扫描

- :ref:`sta<wifi-clista>`        :以STA身份连接至指定SSID/BSSID的路由器上

- :ref:`iperf<wifi-cliiperf>`      :执行iperf 命令


.. _wifi-ap:

:ref:`ap<wifi-cli>` **:以SAP身份创建SoftAP**
------------------------------------------------------------------

**命令** ::

    ap ssid [password] [channel[1:14]]

**参数** ::
    
    ssid: 所建立热点的ssid(必选)

    password：所建立热点的密码(可选)

    channel：所建立热点的信道(可选)

**使用场景** ::
    
    ap test123: 建立模式为OPEN的SoftAP，名字为test123

    ap test123 12345678：开始建立模式名字为test123的加密softAP

    ap test123 12345678  8：建立模式名字为test123的加密softAP，该信道为8


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *如果CONFIG_SOFTAP_WPA3开启，则芯片默认建立WPA3级别加密热点,否则建立WPA2级加密热点!*

    *如果CONFIG_SOFTAP_WPA3开启，不支持建立WPA2级别加密热点!*


.. _wifi-cliip:

:ref:`ip/ipconfig<wifi-cli>`  **:用来查看或设置wifi设备当前的模式状态或指定参数信息**
-------------------------------------------------------------------------------------

**命令** ::

    ip [sta|ap][{ip}{mask}{gate}{dns}]

    ipconfig [sta|ap][{ip}{mask}{gate}{dns}]

**参数** ::
    
    sta|ap: 指示当前操作身份为STA|AP身份

    ip：ip地址

    mask：mask地址

    gate:gate地址

    dns：dns地址

**使用场景** ::
    
    ip/ipconfig:无参数
    
        显示当前板子STA和AP身份的网络信息::

            netif(sta) ip4=192.168.1.229 mask=255.255.255.0 gate=192.168.1.1 dns=192.168.1.1
            netif(ap)  ip4=192.168.188.1 mask=255.255.255.0 gate=192.168.188.1 dns=0.0.0.0


    ip/ipconfig sta|ap ：1个参数
    
        显示当前板子STA或AP身份的网络信息::

            STA:
                netif(sta) ip4=192.168.1.229 mask=255.255.255.0 gate=192.168.1.1 dns=192.168.1.1
            AP:
                netif(ap)  ip4=192.168.188.1 mask=255.255.255.0 gate=192.168.188.1 dns=0.0.0.0

    ip/ipconfig sta|ap  ip gate mask dns ：5个参数

        设置当前板子STA或AP身份的网络信息::

            STA:
                set static ip,  netif(sta) ip4=ip mask=mask gate=gate dns=dns
            AP:
                set static ip,  netif(ap)  ip4=ip mask=mask gate=gate dns=dns

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *本命令在CONFIG_BK_NETIF开启下生效!*

.. _wifi-cliping:

:ref:`ping<wifi-cli>` **:进行指定IP地址和指定时长和每包大小的PING包操作**
----------------------------------------------------------------------------------------------

**命令** ::

    ping [ip|--stop] [cnt]

**参数** ::
    
    ip: 想要ping的网络地址,用于ping开始操作(必选)

    --stop:停止目前正在ping的操作，用于ping结束操作(必选)

    cnt：想要ping的包的个数(可选)

**使用场景** ::
    
    ping 192.168.0.1: 开始对192.168.0.1地址进行ping的操作

    ping 192.168.0.1 1000：开始对192.168.0.1地址进行ping的操作，一共ping 1000个包

    ping –stop：终止正在进行的ping的操作


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *此命令需要开启CONFIG_LWIP!*

.. _wifi-cliscan:

:ref:`scan<wifi-cli>` **:用来查看wifi设备当前的模式状态或指定参数信息**
----------------------------------------------------------------------------------------------

**命令** ::

    scan [ssid]

**参数** ::
    
    ssid: 想要扫描的路由器的ssid

**使用场景** ::
    
    scan:无参数,开始对周围所有AP进行扫描并打印扫描结果

    scan aclsemi ：1个参数,开始对周围名字为aclsemi的AP进行扫描并打印扫描结果

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _wifi-clista:

:ref:`sta<wifi-cli>` **:以STA身份连接至指定SSID/BSSID的路由器上**
----------------------------------------------------------------------------------------------

**命令** ::

    sta ssid [password][bssid]

**参数** ::
    
    ssid: 想要连接的路由器的ssid(必选)

    password:想要连接的路由器密码(可选)

    bssid：当支持bssid连接时填入所要连接的路由器BSSID(可选)

    channel：想要连接的路由器所在的信道(可选)

**使用场景** ::
    
    sta aclsemi:开始对ssid为aclsemi的AP进行连接
  
    sta aclsemi 12345678 ：开始对ssid为aclsemi的AP进行连接

    sta aclsemi 12345678 c758-7eac-a032  ：开始对ssid为aclsemi且bssid为c758-7eac-a032的AP进行连接

.. note::

        *使用单ssid时需确认路由器是OPEN模式，否则会持续汇报258错误(wrong password)!*

        *使用password时无需确认路由器是否为加密模式，如果连接的是OPEN路由器则省略!*

        *使用bssid时则代表当前环境有多个同名ssid路由器,指定模组连接某一个指定的AP上,此情况下ssid不可忽略!*

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. _wifi-cliiperf:

:ref:`iperf<wifi-cli>` **:以某个身份执行iperf命令**
----------------------------------------------------------------------------------------------

**命令** ::

    iperf –c ip –i1 –t60(时间自定义)
    
    iperf –s –i1
    
    iperf –c ip –u –b –i1 -t t60(时间自定义)
    
    iperf –s –u –i1
    
    iperf --stop 


**参数** ::
    
    -c : client 模式启动，host 是 server 端地址，eg：iperf -c 222.35.11.23

    -s : server 模式启动，eg：iperf -s

    ip ：连接后获取的ip 地址或者是电脑端路由器ip地址

    -i1：sec 以秒为单位显示报告间隔，eg：iperf -c 222.35.11.23 -i 2

    -u : 使用 udp 协议，代表当前运行的是udp 传输模式

    --stop :结束 iperf 执行程序

    -t：测试时间，默认 10 秒,eg：iperf -c 222.35.11.23 -t 5

    -b：客户端发送指定带宽大小，通用设置-b 40M

**使用场景** ::

    首先确认STA状态，iperf 执行前，需要串口执行命令：sta ssid key，若支持 sta,串口会返回 ip addr：192.168.XX.XX。此时可以通过 ip 查看网络状态。
    
    TCP 测试方法：

        分别输入：
            server 端：iperf –s –i 1 

            client 端：iperf -c 192.168.xx.xx -t30 -i1 
    
    UDP 测试方法：

        UDP server 端操作方法（电脑端作为client端，发送，BK72xx 板子作为server端，接收）：
        
        分别输入：

            server 端：iperf –s -u -i 1 

            client 端：iperf -c 192.168.xx.xx –u –b40M -t30 -i1 

        UDP client 端操作方法（注意：BK72xx 板子作为client端，发送）:
        
        分别输入：
    
            server 端：iperf –s -u -i 1 
    
            client 端：iperf -c 192.168.xx.xx –u -t30 -i1 

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR