.. _phy-cli:

**PHY cli Command**
=======================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *All functionalities on this page are designed for users who are still using CLI commands to operate PHY.*

- :ref:`rfcali_show_data<phy-rfcal>` :Display calibration information

- :ref:`tempd<phy-temp>`             :Get the current chip temperature

- :ref:`rxsens<phy-rxsens>`          :WIFI radio frequency receiver test command

- :ref:`txevm<phy-txevm>`            :WiFi channel transmission test command

.. _phy-rfcal:

:ref:`rfcali_show_data<phy-cli>` **:Display calibration information**
--------------------------------------------------------------------------------

**Command** ::

    rfcali_show_data

**Params** ::
    
    none

**Usage Scenario** ::
    
    rfcali_show_data: Input this command to start displaying the calibration information of the board.

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect under the condition that CONFIG_WIFI_CLI_ENABLE is enabled.*


.. _phy-temp:

:ref:`tempd<phy-cli>` **:Get the current chip temperature**
------------------------------------------------------------------

**Command** ::

    tempd [stop]

**Params** ::
    
    stop:Turn off temperature compensation

**Usage Scenario** ::
    
    1)  input：
            tempd:Return the current temperature

               get temperature 581

               current temperature is 581

    2)	input: 
            tempd stop :Turn off temperature compensation


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect under the condition that CONFIG_WIFI_CLI_ENABLE is enabled*

.. _phy-rxsens:

:ref:`rxsens<phy-cli>`  **:WIFI radio frequency receiver test command**
------------------------------------------------------------------------------------

**Command** ::

    rxsens [-b] [-c] [-d]

**Params** ::
    
    -d: Timer interval (unit:s)

    -c: Channel

    -b: Bandwidth 0:20M, 1:40M


**Usage Scenario** ::
    
    rxsens -e 1	Enter rx test mode.

    rxsens -b 0 -c 1 -d 1	Channel 1 with a 20M bandwidth receiver

    rxsens -b 0 -c 7 -d 1	Channel 7 with a 20M bandwidth receiver

    rxsens -b 0 -c 13 -d 1	Channel 13 with a 20M bandwidth receiver

    rxsens -b 0 -c 14 -d 1	Channel 14 with a 20M bandwidth receiver

    rxsens -b 0 -c 14 -d 1	Channel 14 with a 20M bandwidth receiver(manual testing only)

    rxsens -b 1 -c 3 -d 1	Channel 3 with a 40M bandwidth receiver

    rxsens -b 1 -c 7 -d 1	Channel 7 with a 40M bandwidth receiver

    rxsens -b 1 -c 11 -d 1	Channel 11 with a 40M bandwidth receiver

    rxsens -b 1 -c 14 -d 1	Channel 14 with a 40M bandwidth receiver

    rxsens -b 1 -c 14 -d 1	Channel 14 with a 40M bandwidth receiver(manual testing only)

    rxsens -g 0	Reset count of the packages to zero.

    rxsens -s 1	Start calculating the package

    rxsens -s 0	Stop calculating the package

    rxsens -g 1	Obtain 20M bandwidth correctly packet(correctly packets/1000,The requirements for passing the test：11b：>92%;others:>90%。)

    rxsens -g 2	Obtain 40M bandwidth correctly packet(correctly packets/1000,The requirements for passing the test：>90%。)

    rxsens -e 0	Exit rx test mode.

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect under the condition that CONFIG_WIFI_CLI_ENABLE is enabled*
     	

.. _phy-txevm:

:ref:`txevm<phy-cli>` **:WiFi channel transmission test command**
----------------------------------------------------------------------------------------------

**Command** ::

    txevm [-b] [-r] [-c] [-f]

**Params** ::
    
    -b: Bandwidth 0:20M; 1:40M

    -r: Rate

    -c: Channel

    -f : 0x0: Non-HT; 0x1:Non-HT-DUP; 0x2: HT-MM;  0x3: HT-GF


**Usage Scenario** ::
    
    //11M power test
    txevm -b 0 -r 11 -c 1	11M Channel 1 transmission
    txevm -b 0 -r 11 -c 7	11M Channel 7 transmission
    txevm -b 0 -r 11 -c 13	11M Channel 13 transmission
        
    //54M power test
    txevm -b 0 -r 54 -c 1	54M Channel 1 transmission
    txevm -b 0 -r 54 -c 7	54M Channel 7 transmission
    txevm -b 0 -r 54 -c 13	54M Channel 13 transmission
        
    //HT20 65M power test
    txevm -b 0 -r 135 -c 1 -f 2	    65M Channel 1 transmission
    txevm -b 0 -r 135 -c 7 -f 2	    65M Channel 7 transmission
    txevm -b 0 -r 135 -c 13 -f 2	65M Channel 13 transmission
        
    //HT40 135M power test
    txevm -b 1 -r 135 -c 3 -f 2	    135M Channel 3 transmission
    txevm -b 1 -r 135 -c 7 -f 2	    135M Channel 7 transmission
    txevm -b 1 -r 135 -c 11 -f 2	135M Channel 11 transmission


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect under the condition that CONFIG_WIFI_CLI_ENABLE is enabled*
