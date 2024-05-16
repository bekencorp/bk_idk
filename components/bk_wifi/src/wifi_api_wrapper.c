#include <common/bk_include.h>
#include "common.h"
#include <common/bk_kernel_err.h>
#include "bk_wifi.h"
#include "bk_wifi_private.h"
#include <common/bk_err.h>
#include <modules/wifi.h>
#include "components/event.h"
#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>

//TODO should finally delete this file!!!

/* This file defines some WiFi API wrappers, it's used for internal CLI modules only.
 **/
#define TAG "bk_wifi"

static int wlan_scan_done_handler(void *arg, event_module_t event_module,
								  int event_id, void *event_data)
{
	wifi_scan_result_t scan_result = {0};

	BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
	BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result));
	bk_wifi_scan_free_result(&scan_result);

	return BK_OK;
}

void demo_scan_app_init(void)
{
	demo_scan_adv_app_init(NULL);
}

void demo_scan_adv_app_init(uint8_t *oob_ssid)
{
	wifi_scan_config_t scan_config = {0};

	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,
							   wlan_scan_done_handler, NULL);

	if (oob_ssid) {
		os_strncpy(scan_config.ssid, (char *)oob_ssid, WIFI_SSID_STR_LEN);
		BK_LOG_ON_ERR(bk_wifi_scan_start(&scan_config));
	} else
		BK_LOG_ON_ERR(bk_wifi_scan_start(NULL));
}

int demo_softap_app_init(char *ap_ssid, char *ap_key, char *ap_channel)
{
	wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
	netif_ip4_config_t ip4_config = {0};
	int len, key_len;
	len = os_strlen(ap_ssid);
	key_len = os_strlen(ap_key);
	if (SSID_MAX_LEN < len) {
		BK_LOGE(TAG, "ssid name more than 32 Bytes\r\n");
		return BK_FAIL;
	}
	if (0 == len) {
		BK_LOGE(TAG, "ssid name must not be null\r\n");
		return BK_FAIL;
	}

	if (8 > key_len)
		BK_LOGE(TAG, "key less than 8 Bytes, the security will be set NONE\r\n");

	if (64 < key_len) {
		BK_LOGE(TAG, "key more than 64 Bytes\r\n");
		return BK_FAIL;
	}

	os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
	os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
	os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
	os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);
	BK_RETURN_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));

	os_strcpy(ap_config.ssid, ap_ssid);
	os_strcpy(ap_config.password, ap_key);

	if (ap_channel) {
		int channel;
		char *end;

		channel = strtol(ap_channel, &end, 0);
		if (*end) {
			BK_LOGE(TAG, "Invalid number '%s'", ap_channel);
			return BK_FAIL;
		}
		ap_config.channel = channel;
	}

	BK_LOGI(TAG, "ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
	BK_RETURN_ON_ERR(bk_wifi_ap_set_config(&ap_config));
	BK_RETURN_ON_ERR(bk_wifi_ap_start());
	return BK_OK;
}

void demo_sta_bssid_app_init(uint8_t *oob_bssid, char *connect_key)
{
	wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

	os_memset(sta_config.ssid, 0, sizeof(sta_config.ssid));
	os_memcpy(sta_config.bssid, oob_bssid, 6);
	os_strcpy(sta_config.password, connect_key);

	BK_LOGI(TAG, "Scan specified BSSID " MACSTR  "\r\n", MAC2STR(sta_config.bssid));
	BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
	BK_LOG_ON_ERR(bk_wifi_sta_start());
}

#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
int demo_sta_app_init(char *oob_ssid, u8* psk, char *connect_key)
#else
int demo_sta_app_init(char *oob_ssid, char *connect_key)
#endif
{
	wifi_sta_config_t sta_config = {0};
	int len;

	len = os_strlen(oob_ssid);
	if (SSID_MAX_LEN < len) {
		BK_LOGI(TAG, "ssid name more than 32 Bytes\r\n");
		return BK_FAIL;
	}
#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
	if (psk) {
		sta_config.psk_len = PMK_LEN * 2;
		sta_config.psk_calculated = true;
		os_strlcpy((char *)sta_config.psk, (char *)psk, sizeof(sta_config.psk));
	}
#endif
	os_strcpy(sta_config.ssid, oob_ssid);
	os_strcpy(sta_config.password, connect_key);

	BK_LOGI(TAG, "ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
	BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
	BK_LOG_ON_ERR(bk_wifi_sta_start());
	return BK_OK;
}

void demo_sta_adv_app_init(char *oob_ssid, char *connect_key)
{
	wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
	int len;

	len = os_strlen(oob_ssid);
	if (SSID_MAX_LEN < len) {
		BK_LOGE(TAG, "ssid name more than 32 Bytes\r\n");
		return;
	}

	os_strcpy(sta_config.ssid, oob_ssid);
	os_strcpy(sta_config.password, connect_key);

	//TODO should NOT use hard-coded BSSID and channel
	hwaddr_aton("48:ee:0c:48:93:12", (u8 *)sta_config.bssid);
	sta_config.security = BK_SECURITY_TYPE_WPA2_MIXED;
	sta_config.channel = 11;

	BK_LOGI(TAG, "ssid:%s  key:%s\r\n", sta_config.ssid, sta_config.password);
	BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
	BK_LOG_ON_ERR(bk_wifi_sta_start());
}

void demo_wlan_app_init(VIF_ADDCFG_PTR cfg)
{
	network_InitTypeDef_st network_cfg;

	if (cfg->wlan_role == BK_STATION) {
		if (cfg->adv == 1) {
			demo_sta_adv_app_init(cfg->ssid, cfg->key);
			return;
		} else {
#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
			demo_sta_app_init(cfg->ssid, NULL, cfg->key);
#else
			demo_sta_app_init(cfg->ssid, cfg->key);
#endif
			BK_LOGI(TAG, "ssid:%s key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);
		}
	} else if (cfg->wlan_role == BK_SOFT_AP) {
		demo_softap_app_init(cfg->ssid, cfg->key, NULL);
		BK_LOGI(TAG, "ssid:%s  key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);
	}
}

extern const char *wifi_sec_type_string(wifi_security_t security);

int demo_state_app_init(void)
{
#if CONFIG_LWIP
	wifi_link_status_t link_status = {0};
	wifi_ap_config_t ap_info = {0};
	netif_ip4_config_t ap_ip4_info = {0};
	char ssid[33] = {0};
#if CONFIG_WIFI4
	BK_LOGI(TAG, "[KW:]sta: %d, ap: %d, b/g/n\r\n", wifi_netif_sta_is_got_ip(), uap_ip_is_start());
#else
	BK_LOGI(TAG, "[KW:]sta: %d, ap: %d, b/g\r\n", wifi_netif_sta_is_got_ip(), uap_ip_is_start());
#endif

	if (sta_ip_is_start()) {
		os_memset(&link_status, 0x0, sizeof(link_status));
		BK_RETURN_ON_ERR(bk_wifi_sta_get_link_status(&link_status));
		os_memcpy(ssid, link_status.ssid, 32);

		BK_LOGI(TAG, "[KW:]sta:rssi=%d,aid=%d,ssid=%s,bssid=%pm,channel=%d,cipher_type=%s\r\n",
				   link_status.rssi, link_status.aid, ssid, link_status.bssid,
				   link_status.channel, wifi_sec_type_string(link_status.security));
	}

	if (uap_ip_is_start()) {
		os_memset(&ap_info, 0x0, sizeof(ap_info));
		BK_RETURN_ON_ERR(bk_wifi_ap_get_config(&ap_info));
		os_memcpy(ssid, ap_info.ssid, 32);
		BK_LOGI(TAG, "[KW:]softap: ssid=%s, channel=%d, cipher_type=%s\r\n",
				   ssid, ap_info.channel, wifi_sec_type_string(ap_info.security));

		BK_RETURN_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info));
		BK_LOGI(TAG, "[KW:]ip=%s,gate=%s,mask=%s,dns=%s\r\n",
				   ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
	}
	return BK_OK;
#endif
}

bk_err_t demo_monitor_cb(const uint8_t *frame, uint32_t len, const wifi_frame_info_t *info)
{
	BK_LOGD(TAG, "rx frame=%p len=%d rssi=%d\n", frame, len, info ? info->rssi : 0);
	return BK_OK;
}

int wifi_demo(int argc, char **argv)
{
	char *oob_ssid = NULL;
	char *connect_key;

	if (strcmp(argv[1], "sta") == 0) {
		BK_LOGI(TAG, "sta_Command\r\n");
		if (argc == 3) {
			oob_ssid = argv[2];
			connect_key = "1";
		} else if (argc == 4) {
			oob_ssid = argv[2];
			connect_key = argv[3];
		} else {
			BK_LOGI(TAG, "parameter invalid\r\n");
			return -1;
		}

		if (oob_ssid)
#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
			demo_sta_app_init(oob_ssid, NULL, connect_key);
#else
			demo_sta_app_init(oob_ssid, connect_key);
#endif

		return 0;
	}

	if (strcmp(argv[1], "adv") == 0) {
		BK_LOGI(TAG, "sta_adv_Command\r\n");
		if (argc == 3) {
			oob_ssid = argv[2];
			connect_key = "1";
		} else if (argc == 4) {
			oob_ssid = argv[2];
			connect_key = argv[3];
		} else {
			BK_LOGI(TAG, "parameter invalid\r\n");
			return -1;
		}

		if (oob_ssid)
			demo_sta_adv_app_init(oob_ssid, connect_key);
		return 0;
	}

	if (strcmp(argv[1], "softap") == 0) {

		BK_LOGI(TAG, "SOFTAP_COMMAND\r\n\r\n");
		if (argc == 3) {
			oob_ssid = argv[2];
			connect_key = "1";
		} else if (argc == 4) {
			oob_ssid = argv[2];
			connect_key = argv[3];
		} else {
			BK_LOGI(TAG, "parameter invalid\r\n");
			return -1;
		}

		if (oob_ssid)
			demo_softap_app_init(oob_ssid, connect_key, NULL);
		return 0;
	}

	if (strcmp(argv[1], "status") == 0) {
		if (argc != 3)
			BK_LOGI(TAG, "parameter invalid\r\n");

		if (strcmp(argv[2], "net") == 0)
			demo_ip_app_init();
		else if (strcmp(argv[2], "link") == 0)
			demo_state_app_init();
		else
			BK_LOGI(TAG, "parameter invalid\r\n");
	}

	if (strcmp(argv[1], "scan") == 0) {
		if (argc == 2)
			demo_scan_app_init();
		else if (argc == 3)
			demo_scan_adv_app_init((uint8_t *)argv[2]);
		else
			BK_LOGI(TAG, "parameter invalid\r\n");
	}

	if (strcmp(argv[1], "monitor") == 0) {
		if (argc != 3)
			BK_LOGI(TAG, "parameter invalid\r\n");

		if (strcmp(argv[2], "start") == 0) {
			BK_LOG_ON_ERR(bk_wifi_monitor_register_cb(demo_monitor_cb));
			BK_LOG_ON_ERR(bk_wifi_monitor_start());
		} else if (strcmp(argv[2], "stop") == 0)
			BK_LOG_ON_ERR(bk_wifi_monitor_stop());
		else
			BK_LOGI(TAG, "parameter invalid\r\n");
	}

	return 0;
}
