// Copyright 2020-2022 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* @brief Overview about this API header
 *
 */

/**
 * @brief     Interface for customer to Enable/Disable beacon tx tim
 *
 * @attention 1 Controled by middleware/soc/bk72xx.defconfig
 *
 * @return
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_tx_tim_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable AP_PS
 *
 * @attention 1 Controled by middleware/soc/bk72xx.defconfig
 *
 * @return
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_ap_ps_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable temp detect functionality
 *
 * @attention 1 Controled by middleware/soc/bk72xx.defconfig
 *
 * @return
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_temp_detect_enable(void);


typedef enum {
    BK_CPU0 = 0, /**< cpu0 id */
    BK_CPU1 = 1, /**< cpu1 id */
    BK_CPU2 = 2 /**< cpu2 id */
} BK_CPU_ID;


/**
 * @brief     Interface to get if current system is cpu0
 *
 *
 * @return
 *    - BK_CPU0: current system is cpu0
 *    - BK_CPU1: current system is cpu1
 *    - BK_CPU2: current system is cpu2
 *
 */
int bk_feature_get_cpu_id(void);


/**
 * @brief     Interface to get the cpu count of system
 *
 *
 * @return
 *    - 1: current system is single core
 *    - 2: current system is dual core
 *    - 3: current system is triple core
 *
 */
int bk_feature_get_cpu_cnt(void);

/**
 * @brief     Interface for customer to Enable/Disable Auto Gratuitous ARP
 *
 *
 * @return
 *    - 0: Disable Auto Gratuitous ARP
 *    - 1: Enable Auto Gratuitous ARP
 *
 */
int bk_feature_auto_garp_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable correct listen interval ie in assoc req
 *
 *
 * @return
 *    - 0: Disable correct listen interval ie
 *    - 1: Enable correct listen interval ie
 *
 */
int bk_feature_correct_listen_interval_ie_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable config sleep mode
 *
 *
 * @return
 *    - 0: Disable config sleep mode
 *    - 1: Enable config sleep mode
 *
 */
int bk_feature_config_sleep_mode_enable(void);

/**
 * @brief     Interface for customer to get Auto Gratuitous ARP period
 *
 */
int bk_feature_get_auto_garp_period(void);

/**
 * @brief     Interface for customer to Enable/Disable config retry parameters
 *
 *
 * @return
 *    - 0: Disable retry config
 *    - 1: Enable retry config
 *
 */
int bk_feature_retry_config_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable config wireless mode
 *
 *
 * @return
 *    - 0: Disable config wireless mode
 *    - 1: Enable config wireless mode
 *
 */
int bk_feature_config_wireless_mode_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable dtim event
 *
 *
 * @return
 *    - 0: Disable dtim event
 *    - 1: Enable dtim event
 *
 */
int bk_feature_dtim_event_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable Station receiving BCMC frame in dtim10
 *
 *
 * @return
 *    - 0: Disable Station to receive BCMC frame in dtim10
 *    - 1: Enable Station to receive BCMC frame in dtim10
 *
 */
int bk_feature_receive_bcmc_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable not check beacon ssid changes
 *
 *
 * @return
 *    - 0: Disable not check beacon ssid changes
 *    - 1: Enable not check beacon ssid changes
 *
 */
int bk_feature_not_check_ssid_enable(void);

/**
 * @brief     Interface for customer to close ap csa in coexist mode
 *
 *
 * @return
 *    - 0: Open coexist csa
 *    - 1: Close coexist csa
 *
 */
int bk_feature_close_coexist_csa(void);
/**
 * @brief     Interface for customer to set the MAX number of stations supported by Mac
 *
 *
 * @return
 *    - the MAX number of stations supported by Mac
 */
int bk_feature_get_mac_sup_sta_max_num(void);

int bk_feature_config_cache_enable(void);
int bk_feature_ckmn_enable(void);
int bk_feature_send_deauth_before_connect(void);
int bk_feature_send_sta_csa_ind(void);
int bk_feature_get_scan_speed_level(void);
int bk_feature_tcp_protect_enable(void);
int bk_feature_set_relay_ap_inactivity(void);
int bk_feature_config_add_chan_ctxt(void);

/**
 * @brief     Interface for customer to save cali data to OTP2
 *
 *
 * @return
 *    - 0: Disable save cali data to OTP2
 *    - 1: Enable save cali data to OTP2
 *
 */
int bk_feature_save_rfcali_to_otp_enable(void);

/**
 * @brief     Interface for customer to close or open phy log
 *
 *
 * @return
 *    - 0: close phy log
 *    - 1: open phy log
 *
 */
int bk_feature_phy_log_enable(void);

/**
 * @brief     Interface for customer to close or open mac pd during lv sleep
 *
 *
 * @return
 *    - 0: mac always on
 *    - 1: mac power down during lv
 *
 */
int bk_feature_mac_pwd_enable(void);

/**
 * @brief     Interface for customer to close or open wrls pd during lv sleep
 *
 *
 * @return
 *    - 0: wrls always on
 *    - 1: wrls power down during lv
 *
 */
int bk_feature_wrls_pwd_enable(void);

/**
 * @brief     Interface for customer to close or open cpu pd during lv sleep
 *
 *
 * @return
 *    - 0: cpu always on
 *    - 1: cpu power down during lv
 *
 */
int bk_feature_cpu_pwd_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable non standard functionality
 *
 * @return
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_non_standard_enable(void);

/**
 * @brief     Interface to get default td window
 *
 *
 * @return
 *    - default td window
 *
 */
unsigned int bk_feature_get_default_td_win(void);

/**
 * @brief     Interface to get reduce td window
 *
 *
 * @return
 *    - reduce td window
 *
 */
unsigned int bk_feature_get_reduce_td_win(void);

/**
 * @brief     Interface to get td learn count
 *
 *
 * @return
 *    - td learn count
 *
 */
unsigned int bk_feature_get_td_learn_cnt(void);

/**
 * @brief     Interface to get td run count
 *
 *
 * @return
 *    - td run count
 *
 */
unsigned int bk_feature_get_td_run_cnt(void);

/**
 * @brief     Interface to get td level2
 *
 *
 * @return
 *    - td level2
 *
 */
unsigned int bk_feature_get_td_level2(void);

/**
 * @brief     Interface to get arp fail trig null feature
 *
 *
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_arp_fail_trig_null(void);

/**
 * @brief     Interface to get ap drift statistic cnt
 *
 *
 * @return
 *    - ap drift statistic cnt
 *
 */
unsigned int bk_feature_get_ap_drift_statistic_cnt(void);

/**
 * @brief     Interface to get keep alive retry cnt
 *
 *
 * @return
 *    - keep alive retry cnt
 *
 */
unsigned int bk_feature_get_keep_alive_retry_cnt(void);

/**
 * @brief     Interface to get keep alive listen interval threshold
 *
 *
 * @return
 *    - keep alive listen interval threshold
 *
 */
unsigned int bk_feature_get_listen_interval_threshold(void);

/**
 * @brief     Interface to get keep alive timeout duration
 *
 *
 * @return
 *    - keep alive timeout duration
 *
 */
unsigned int bk_feature_get_keep_alive_to_dur(void);

/**
 * @brief     Interface to get compute listen interval enable
 *
 *
 * @return
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_compute_listen_interval_enable(void);

/**
 * @brief     Interface for customer to Enable/Disable [Qos] Null frames with Sequence number
 *
 * @return
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_wifi_null_frame_with_sn(void);

/**
 * @brief     Interface for customer to Enable/Disable wifi dsss only
 *
 * @return
 *    - 1: enable
 *    - 0: disable
 */
int bk_feature_wifi_dsss_only(void);

/**
 * @brief     Interface for wi-fi and ble coex
 *
 *
 * @return
 *    - 0: disable wi-fi and ble coex
 *    - 1: enable wi-fi and ble coex
 *
 */
int bk_feature_coex_enable(void);

/**
 * @brief     Interface for update power with RSSI
 *
 *
 * @return
 *    - 0: disable update power with RSSI
 *    - 1: enable update power with RSSI
 *
 */
int bk_feature_update_power_with_rssi(void);

/**
 * @brief     Interface for customer to enable or disable wifi 6.
 *
 *
 * @return
 *    - 0: wifi 6 is disabled
 *    - 1: wifi 6 is enabled
 *
 */
int bk_feature_wifi6_enable(void);

