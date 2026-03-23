#include "bk_feature.h"
#include "sdkconfig.h"


int bk_feature_temp_detect_enable(void) {
#if CONFIG_TEMP_DETECT
    return 1;
#else
    return 0;
#endif
}

int bk_feature_get_cpu_id(void) {
#if CONFIG_SYS_CPU0
    return BK_CPU0;
#elif CONFIG_SYS_CPU1
    return BK_CPU1;
#elif CONFIG_SYS_CPU2
    return BK_CPU2;
#else
    return 0;
#endif
}


int bk_feature_get_cpu_cnt(void) {
#if CONFIG_CPU_CNT
    return CONFIG_CPU_CNT;
#else
    return 1;
#endif
}

int bk_feature_auto_garp_enable(void) {
#if CONFIG_AUTO_GARP
	return 1;
#else
	return 0;
#endif
}

int bk_feature_get_auto_garp_period(void) {
#if CONFIG_AUTO_GARP
	return CONFIG_AUTO_GARP_PERIOD;
#else
	return 0;
#endif
}

int bk_feature_correct_listen_interval_ie_enable(void) {
#if CONFIG_CORRECT_LISTEN_INTERVAL_IE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_config_sleep_mode_enable(void) {
#if CONFIG_WIFI_SLEEP_MODE_SUPPORT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_dtim_event_enable(void) {
#if CONFIG_DTIM_EVENT_ENABLE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_retry_config_enable(void) {
#if CONFIG_WIFI_RETRY_CONFIG_SUPPORT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_config_wireless_mode_enable(void) {
#if CONFIG_WIFI_WIRELESS_MODE_SUPPORT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_receive_bcmc_enable(void) {
#if CONFIG_RECEIVE_BCMC_IN_DTIM10
    return 1;
#else
    return 0;
#endif
}

int bk_feature_not_check_ssid_enable(void) {
#if CONFIG_NOT_CHECK_SSID_CHANGE
    return 1;
#else
    return 0;
#endif
}

int bk_feature_config_cache_enable(void) {
#if CONFIG_CACHE_ENABLE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_ckmn_enable(void) {
#if CONFIG_CKMN
	return 1;
#else
	return 0;
#endif
}

int bk_feature_send_deauth_before_connect(void) {
#if CONFIG_DEAUTH_BEFORE_CONNECT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_close_coexist_csa(void) {
#if CONFIG_CLOSE_COEXIST_CSA
	return 1;
#else
	return 0;
#endif
}

int bk_feature_ap_ps_enable(void) {
#if CONFIG_AP_PS
	return 1;
#else
	return 0;
#endif
}

int bk_feature_tx_tim_enable(void) {
#if CONFIG_AP_TX_TIM
    return 1;
#else
    return 0;
#endif
}

int bk_feature_set_relay_ap_inactivity(void) {
#if CONFIG_BRIDGE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_config_add_chan_ctxt(void) {
#if CONFIG_ADD_CHAN_CTXT
    return 1;
#else
    return 0;
#endif
}

int bk_feature_send_sta_csa_ind(void) {
#if CONFIG_BRIDGE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_get_scan_speed_level(void) {
	return CONFIG_SCAN_SPEED_LEVEL;
}

int bk_feature_tcp_protect_enable(void) {
#if CONFIG_TCP_PROTECT_EN
	return 1;
#else
	return 0;
#endif
}

int bk_feature_get_mac_sup_sta_max_num(void) {
#if CONFIG_WIFI_MAC_SUPPORT_STAS_MAX_NUM
    return CONFIG_WIFI_MAC_SUPPORT_STAS_MAX_NUM;
#else
    return 2;
#endif
}

int bk_feature_save_rfcali_to_otp_enable(void) {
#if (CONFIG_OTP && CONFIG_PHY_RFCALI_TO_OTP)
    return 1;
#else
    return 0;
#endif
}

int bk_feature_phy_log_enable(void) {
#if CONFIG_PHY_LOG_ENABLE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_mac_pwd_enable(void)
{
#if CONFIG_MAC_PWD
	return 1;
#else
	return 0;
#endif
}

int bk_feature_wrls_pwd_enable(void)
{
#if CONFIG_WRLS_PWD
	return 1;
#else
	return 0;
#endif
}

int bk_feature_cpu_pwd_enable(void)
{
#if CONFIG_DEEP_LV
	return 1;
#else
	return 0;
#endif
}

int bk_feature_non_standard_enable(void)
{
#if CONFIG_WIFI_NON_STANDARD
	return 1;
#else
	return 0;
#endif
}

unsigned int bk_feature_get_default_td_win(void) {
	return CONFIG_DEFAULT_TD_WIN;
}

unsigned int bk_feature_get_reduce_td_win(void) {
	return CONFIG_TD_REDUCE_INTV;
}

unsigned int bk_feature_get_td_learn_cnt(void) {
	return CONFIG_DYNAMIC_TD_LEARN;
}

unsigned int bk_feature_get_td_run_cnt(void) {
	return CONFIG_DYNAMIC_TD_RUN;
}

unsigned int bk_feature_get_td_level2(void) {
	return CONFIG_DYNAMIC_TD_LEVEL2;
}

int bk_feature_arp_fail_trig_null(void) {
#if CONFIG_ARP_FAIL_TRIG_NULL
	return 1;
#else
	return 0;
#endif
}

unsigned int bk_feature_get_ap_drift_statistic_cnt(void) {
	return CONFIG_AP_DRIFT_STATISTIC_CNT;
}

unsigned int bk_feature_get_keep_alive_retry_cnt(void) {
	return CONFIG_KEEP_ALIVE_RETRY_CNT;
}

unsigned int bk_feature_get_listen_interval_threshold(void) {
	return CONFIG_LISTEN_INTERVAL_THRESHOLD;
}

unsigned int bk_feature_get_keep_alive_to_dur(void) {
	return CONFIG_VIF_MGMT_KEEP_ALIVE_TO_DUR;
}

int bk_feature_compute_listen_interval_enable(void) {
#if CONFIG_COMPUTE_LISTEN_INTERVAL
	return 1;
#else
	return 0;
#endif
}

int bk_feature_wifi_null_frame_with_sn(void)
{
#if CONFIG_WIFI_NULL_FRAME_WITH_SN
	return 1;
#else
	return 0;
#endif
}

int bk_feature_wifi_dsss_only(void)
{
#if CONFIG_PM_DSSS_ONLY
	return 1;
#else
	return 0;
#endif
}

int bk_feature_coex_enable(void) {
#if (CONFIG_COEX)
	return 1;
#else
	return 0;
#endif
}

int bk_feature_update_power_with_rssi(void) {
#if (CONFIG_UPDATE_POWER_WITH_RSSI)
	return 1;
#else
	return 0;
#endif
}

int bk_feature_wifi6_enable(void)
{
#ifdef CONFIG_WIFI6
	return 1;
#else
	return 0;
#endif
}
