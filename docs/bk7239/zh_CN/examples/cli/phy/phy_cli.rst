.. _phy-cli:

**PHY cli命令**
=======================

:link_to_translation:`en:[English]`


.. important::

    *本界面所有功能仅面向一些仍在使用cli命令操作PHY的用户使用*

- :ref:`rfcali_show_data<phy-rfcal>` :显示校准信息

- :ref:`tempd<phy-temp>`             :获取当前芯片温度

- :ref:`rxsens<phy-rxsens>`          :WIFI射频接收测试命令

- :ref:`txevm<phy-txevm>`            :WIFI各信道发射测试命令

.. _phy-rfcal:

:ref:`rfcali_show_data<phy-cli>` **:显示校准信息**
------------------------------------------------------------------

**命令** ::

    rfcali_show_data

**参数** ::
    
    无

**使用场景** ::
    
    rfcali_show_data: 输入这条命令，则开始显示板子的校准信息

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *本命令在CONFIG_WIFI_CLI_ENABLE开启下生效*


.. _phy-temp:

:ref:`tempd<phy-cli>` **:获取当前芯片温度**
------------------------------------------------------------------

**命令** ::

    tempd [stop]

**参数** ::
    
    stop:停止温补

**使用场景** ::
    
    1)	输入：tempd,返回当前温度

            get temperature 581

            current temperature is 581

    2)	输入: tempd stop 关闭温补


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *本命令在CONFIG_WIFI_CLI_ENABLE开启下生效*

.. _phy-rxsens:

:ref:`rxsens<phy-cli>`  **:WIFI射频接收测试命令**
----------------------------------------------------------------

**命令** ::

    rxsens [-b] [-c] [-d]

**参数** ::
    
    -d: Timer interval (单位:s)

    -c: Channel

    -b: Bandwidth 0:20M, 1:40M


**使用场景** ::
    
    rxsens -e 1	进入接收测试模式

    rxsens -b 0 -c 1 -d 1	1信道20M带宽接收

    rxsens -b 0 -c 7 -d 1	7信道20M带宽1s接收

    rxsens -b 0 -c 13 -d 1	13信道20M带宽1s接收

    rxsens -b 0 -c 14 -d 1	14信道20M带宽1s接收

    rxsens -b 0 -c 14 -d 1	14信道20M连续1s接收(仅用于手动测试)

    rxsens -b 1 -c 3 -d 1	3信道40M带宽1s接收

    rxsens -b 1 -c 7 -d 1	7信道40M带宽1s接收

    rxsens -b 1 -c 11 -d 1	11信道40M带宽1s接收

    rxsens -b 1 -c 14 -d 1	14信道40M带宽1s接收

    rxsens -b 1 -c 14 -d 1	14信道40M连续1s接收(仅用于手动测试)

    rxsens -g 0	包计数清零

    rxsens -s 1	开始计算包

    rxsens -s 0	停止计算包

    rxsens -g 1	得到20M带宽正确包(正确包数/1000,测试通过的要求：11b：>92%;其它：>90%。)

    rxsens -g 2	得到40M带宽正确包(正确包数/1000,测试通过的要求：>90%。)

    rxsens -e 0	退出接收模式

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *本命令在CONFIG_WIFI_CLI_ENABLE开启下生效!*
     	

.. _phy-txevm:

:ref:`txevm<phy-cli>` **:WIFI各信道发射测试命令**
----------------------------------------------------------------------------------------------

**命令** ::

    txevm [-b] [-r] [-c] [-f]

**参数** ::
    
    -b: Bandwidth 0:20M; 1:40M

    -r: Rate

    -c: Channel

    -f : 0x0: Non-HT; 0x1:Non-HT-DUP; 0x2: HT-MM;  0x3: HT-GF


**使用场景** ::
    
    //11M功率测试	
    txevm -b 0 -r 11 -c 1	11M信道1发射
    txevm -b 0 -r 11 -c 7	11M信道7发射
    txevm -b 0 -r 11 -c 13	11M信道13发射
        
    //54M功率测试	
    txevm -b 0 -r 54 -c 1	54M信道1发射
    txevm -b 0 -r 54 -c 7	54M信道7发射
    txevm -b 0 -r 54 -c 13	54M信道13发射
        
    //HT20 65M功率测试	
    txevm -b 0 -r 135 -c 1 -f 2	    65M信道1发射
    txevm -b 0 -r 135 -c 7 -f 2	    65M信道7发射
    txevm -b 0 -r 135 -c 13 -f 2	65M信道13发射
        
    //HT40 135M功率测试	
    txevm -b 1 -r 135 -c 3 -f 2	    135M信道3发射
    txevm -b 1 -r 135 -c 7 -f 2	    135M信道7发射
    txevm -b 1 -r 135 -c 11 -f 2	135M信道11发射


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *本命令在CONFIG_WIFI_CLI_ENABLE开启下生效!*
