VND_CAL
================

:link_to_translation:`en:[English]`

1、概述
--------------------------
    为了方便客户理解vnd_cal.c里的参数，这里对vnd_cal.c里参数进行说明。



2、代码路径
--------------------------
    路径: ``\middleware\boards\bk7256(bk7235\bk7235m\bk7236\bk7258)\vnd_cal\vnd_cal.c``

3、参数简介
--------------------------
参数如下:

    1.校准数据的来源
		- #define CMTAG_FROM_FLASH                1
		- #define CMTAG_FROM_CALI                 0
		- 功能说明：用来配置DC，IQ校准值从easyflash读取，还是每次上电，校准获取。从easyflash获取校准数据是为了优化开机上电时间。

    2.默认晶体值
		- #define DEFAULT_TXID_XTAL               (0x3a)
		- 功能说明：用来配置晶体默认值，为了优化频偏。

    3.vnd_cal版本信息
		- vnd_cal_version = "24-04-10 00:00:00"
		- 功能说明：记录vnd_cal版本信息。

    4.11B gain基值与功率gain表
		- 1.const UINT32 pwr_gain_base_gain_b = 0x18ab7c00
		- 2.#define TPC_PAMAP_TAB_B_LEN			 (48)
		- 3.const PWR_REGS cfg_tab_b[TPC_PAMAP_TAB_B_LEN]
		- 功能说明：1是发包功率基值; 2是功率gain表长度; 3是11B发包pregain表。发包功率基值与pregain组合后为发包的具体功率值。11B为0.5dB一个step。

    5.11G gain基值与功率gain表
		- 1.const UINT32 pwr_gain_base_gain_g = 0x98ab7c00
		- 2.#define TPC_PAMAP_TAB_G_LEN			 (80)
		- 3.const PWR_REGS cfg_tab_g[TPC_PAMAP_TAB_G_LEN]
		- 功能说明：1是发包功率基值; 2是功率gain表长度; 3是11G发包pregain表。发包功率基值与pregain组合后为发包的具体功率值。11G为0.25dB一个step。

    6.BLE发包功率范围选择
		- 宏 CONFIG_BLE_USE_HIGH_POWER_LEVEL
		- 功能说明：选择BLE发包功率支持的范围，打开宏BLE支持高功率。

    7.BLE gain基值与功率gain表
		- 1.const UINT32 pwr_gain_base_gain_ble = 0x18a94c00
		- 2.#define TPC_PAMAP_TAB_BT_LEN                        (65)
		- 3.const PWR_REGS cfg_tab_bt[TPC_PAMAP_TAB_BT_LEN]
		- 功能说明：1是发包功率基值; 2是功率gain表长度; 3是BLE发包pregain表。发包功率基值与pregain组合后为发包的具体功率值。BLE为0.25dB一个step。

    8.发包功率index表
		- const TXPWR_ST gtxpwr_tab_def_b[WLAN_2_4_G_CHANNEL_NUM]
		- const TXPWR_ST gtxpwr_tab_def_g[WLAN_2_4_G_CHANNEL_NUM]
		- const TXPWR_ST gtxpwr_tab_def_n_40[WLAN_2_4_G_CHANNEL_NUM]
		- const TXPWR_ST gtxpwr_tab_def_ble[BLE_2_4_G_CHANNEL_NUM]
		- 功能说明：11B，11G，11N 40M带宽与BLE默认发包功率index表。开发板按照指定的发包功率校准后，会得出以上4张表。表中的值，为第4,5,7条pregain表中值的index。

    9.温补表
		- const TMP_PWR_ST tmp_pwr_tab[TMP_PWR_TAB_LEN]
		- 功能说明：温补表，根据温度调节发包功率与频偏。表的第2列为11B发包功率随温度变化的偏移值，第3列为11G、11N与11AX发包功率随温度变化的偏移值,第4列为BLE发包功率随温度变化的偏移值。第5列为频偏随温度变化的偏移值。

    10.自动功率校准配置
		- const static AUTO_PWR_CALI_CONTEXT auto_pwr
		- 功能说明：自动功率校准配置参数。第一个参数为打开与关闭自动功率校准的开关,0是打开,1是关闭。第2-4个参数为11B 1信道,7信道,13信道,自动功率校准的阈值。第5-7个参数为11G 1信道,7信道,13信道,自动功率校准的阈值。11N与11AX基于11G自动功率校准结果偏移而来(7236与7258不支持)。

    11.不同rate发包功率偏移表
		- const INT16 shift_tab_b[4]
		- const INT16 shift_tab_g[8]
		- const INT16 shift_tab_n20[10]
		- const INT16 shift_tab_n40[10]
		- 功能说明：不同rate发包功率偏移表。功率校准是以11B 11M,11G 54M,11N 40M NCS7进行校准,其他rate发包功率是在校准使用的rate功率进行偏移得出。11B 0.5dB一个step,其他为0.25dB一个step。例如11B希望整体降低1dB,const INT16 shift_tab_b[4] = {-1, -1, -1, -1};11N 40整体提升1dB,const INT16 shift_tab_n40[10] = {-6,  -2,  4,  4,  4,  4,  4,  4,  4, 4/*4*/}
	
    12.FCC与SRRC认证时提升或降低不同rate发包功率
		- const INT16 shift_tab_b_fcc[WLAN_2_4_G_CHANNEL_NUM]
		- const INT16 shift_tab_g_fcc[WLAN_2_4_G_CHANNEL_NUM]
		- const INT16 shift_tab_n20_fcc[WLAN_2_4_G_CHANNEL_NUM]
		- const INT16 shift_tab_n40_fcc[WLAN_2_4_G_CHANNEL_NUM]
		- const INT16 shift_tab_b_srrc[WLAN_2_4_G_CHANNEL_NUM]
		- const INT16 shift_tab_g_srrc[WLAN_2_4_G_CHANNEL_NUM]
		- const INT16 shift_tab_n20_srrc[WLAN_2_4_G_CHANNEL_NUM]
		- const INT16 shift_tab_n40_srrc[WLAN_2_4_G_CHANNEL_NUM]
		- 功能说明：客户在FCC与SRRC认证时调解不同rate发包功率偏移表。调解方式与第11条一致。

    13.配置自动功率校准参数到代码
		- vnd_cal_set_auto_pwr_thred(auto_pwr);
		- 功能说明：将第9条的参数配置到代码，7236与7258不支持。

    14.配置外部PA使用的GPIO
		- vnd_cal_set_epa_config(1, GPIO_28, GPIO_26, pwr_gain_base_gain_b, pwr_gain_base_gain_g);
		- vnd_cal_set_epa_config(0, GPIO_28, GPIO_26, pwr_gain_base_gain_b, pwr_gain_base_gain_g);
		- 功能说明：配置GPIO打开或关闭外PA功能,7236与7258不支持。

    15.开机打印vnd_cal版本信息
		- vnd_cal_version_log(vnd_cal_version);
		- 功能说明：打印vnd_cal.c版本信息。

    16.CCA门限
		- vnd_cal_set_cca_level(0);
		- 功能说明：SRRC自适应测试不同实验室要求不同,开放CCA门限。
