**Support Board:BK7236**

Softap
=======================================

Project Overview
-------------------------------
This project is used for demonstrating [Wi-Fi AP Mode](https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/wifi/bk_wifi_mode.html).

Hardware Requirements
-------------------------------
BK7236 development board

Configure and Build
-------------------------------

Configure
***************************

Build
***************************
Execute the following command in the Armino root directory to compile:
make bk7236 PROJECT=softap APP_VERSION=”v1.0” or make bk7236 PROJECT=softap APP_VERSION="v2.0"

Connect UART
***************************
Uart0 is used for Logging and CLI

Flash
***************************
Burn all-app.bin to the board using BKFIL.

Running and Output
-------------------------------
The entry of SoftAP mode is cmd_wlan_ap_exec in V1 version or cli_wifi_ap_cmd in V2 version,
the source code of softap is same as STA mode. The main configuration parameters are SSID and password.

The common ones of V1 as follows:
  - net ap set <field> <value>
  - net ap set ssid ssid_example
  - net ap set psk psk_example

  - net ap enable
  - net ap reload
  - net ap disable

cmd example:
  - net ap enable
  - net ap set ssid BEKEN_TEST
  - net ap set psk 12345678
  - net ap start

The common ones of V2 as follows:
  - {"ap", "ap ssid [password]", cli_wifi_ap_cmd}
  - {"stop", "stop {sta|ap}", cli_wifi_stop_cmd}
  - {"state", "state - show STA/AP state", cli_wifi_state_cmd}

cmd example:
  - ap BEKEN_TEST 12345678

case 1
***************************
Operate
***************************
Execute cmd ap BEKEN_TEST 12345678 and observe the process

Output
***************************
::

 os:I(9134):create shell_handle, tcb=28035740, stack=[28034318-28035718:5120], prio=5
 bk_wifi:I(9134):ssid:BEKEN_TEST  key:12345678
 wifi W:(9134): the max conn num is zero, set it is default
 wifid I:(9136): phy_s_ch_w_r err,band:0,cf:255,pf:255,type:5
 wifid I:(9144): mm_add_if_req: vif_idx=0, type=2, p2p=0, status=0
 wifid I:(9144): chan_ctxt_add: CTXT0,freq2462MHz,bw20MHz,pwr0dBm
 wifid I:(9144): chan_reg_fix:VIF0,CTXT0,type2,ctxt_s0,nb_vif0
 wifid I:(9146): ps OFF
 wifid I:(9148): [KW:]mm_set_vif_state,vif=0,vif_type=2,is_active=1, aid=0x0
 hitf I:(9148): start apm success, vif0, channel0, bcmc2
 hitf I:(11996): add CCMP
 lwip:I(11998):mac c8:47:8c:da:bb:28
 lwip:I(11998):configuring interface ap (with Static IP)
 $os:I(12000):create dhcp-server, tcb=28040ae8, stack=[280404c0-28040ac0:1536], prio=2
 wifi I:(12002): ap started

case 2
***************************
Operate
***************************
Execute the cmd stop ap and observe the process

Output
***************************

::

 os:I(165502):create shell_handle, tcb=280343e0, stack=[28040b68-28041f68:5120], prio=5
 lwip:I(165522):uap ip down
 lwip:I(165522):remove vif0
 rwnx I:(165524): send apm stop req, vif=0
 wifid I:(165524): [KW:]mm_set_vif_state,vif=0,vif_type=2,is_active=0, aid=0x0
 wifid I:(165524): chan_unreg_fix:VIF0,CTXT0,nb_vif1
 wifid I:(165526): [KW:]mm_remove_if: vif0
 wifid I:(165526): ps ON
 cal I:(165566): temp_code:32 - adc_code:446 - adc_trend:[15]:434->[14]:454
 wifi I:(165726): ap stopped

case 3
**************************
Operate
**************************
Execute the cmd state and observe the process

Output
**************************
::

 os:I(112526):create shell_handle, tcb=280343e0, stack=[28040b68-28041f68:5120], prio=5
 bk_wifi:I(112526):[KW:]sta: 0, ap: 1, b/g/n
 bk_wifi:I(112526):[KW:]softap: ssid=BEKEN_TEST, channel=11, cipher_type=WPA2-AES
 bk_wifi:I(112528):[KW:]ip=192.168.0.1,gate=192.168.0.1,mask=255.255.255.0,dns=0.0.0.0
