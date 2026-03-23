/**
 ****************************************************************************************
 *
 * @file armino_main.c
 *
 * @brief CSI main.
 *
 * Copyright (C) RivieraWaves 2011-2025
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <common/sys_config.h>
#include <components/log.h>
#include <modules/wifi.h>
#include <components/event.h>
#include <components/netif.h>
#include <string.h>
#include "bk_private/bk_init.h"
#include "bk_private/bk_wifi.h"
#include "modules/wifi_types.h"
#include <os/os.h>
#include <os/str.h>

#define CSI_WIFI_CONNECT   1
#define APP_TAG "CSI_DEMO"

#if (CONFIG_SYS_CPU0)
static beken_semaphore_t wifi_conn_sema = NULL;

int csi_wifi_netif_event_cb(void *arg, event_module_t event_module,
					int event_id, void *event_data)
{
	netif_event_got_ip4_t *got_ip;

	switch (event_id) {
		case EVENT_NETIF_GOT_IP4:{
			got_ip = (netif_event_got_ip4_t *)event_data;
			BK_LOGI(APP_TAG, "%s got ip\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif");
			if (wifi_conn_sema) {
				rtos_set_semaphore(&wifi_conn_sema);
			}
			}break;

		default:{
			BK_LOGI(APP_TAG, "rx event <%d %d>\n", event_module, event_id);
			}break;
	}

	return BK_OK;
}

int csi_wifi_wifi_event_cb(void *arg, event_module_t event_module,
					int event_id, void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_ap_disconnected_t *ap_disconnected;
	wifi_event_ap_connected_t *ap_connected;

	switch (event_id) {
		case EVENT_WIFI_STA_CONNECTED:{
			sta_connected = (wifi_event_sta_connected_t *)event_data;
			BK_LOGI(APP_TAG, "STA connected to %s\n", sta_connected->ssid);
			}break;

		case EVENT_WIFI_STA_DISCONNECTED:{
			sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
			BK_LOGI(APP_TAG, "STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
			}break;

		case EVENT_WIFI_AP_CONNECTED:{
			ap_connected = (wifi_event_ap_connected_t *)event_data;
			BK_LOGI(APP_TAG, BK_MAC_FORMAT" connected to AP\n", BK_MAC_STR(ap_connected->mac));
			}break;

		case EVENT_WIFI_AP_DISCONNECTED:{
			ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
			BK_LOGI(APP_TAG, BK_MAC_FORMAT" disconnected from AP\n", BK_MAC_STR(ap_disconnected->mac));
			}break;

		default:{
			BK_LOGI(APP_TAG, "rx event <%d %d>\n", event_module, event_id);
			}break;
	}

	return BK_OK;
}

/**
 ****************************************************************************************
 * @brief csi wifi event cb
 *
 * @param[in] void
 ****************************************************************************************
 */
static void csi_wifi_event_handler_init(void)
{
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, csi_wifi_wifi_event_cb, NULL));
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, csi_wifi_netif_event_cb, NULL));
}

/**
 ****************************************************************************************
 * @brief wifi_csi_rx_cb_demo, output CSI data
 *
 * @param[in] info     Pointer to the csi info buffer
 ****************************************************************************************
 */
void wifi_csi_rx_cb_demo(struct wifi_csi_info_t *info)
{
	if (info != NULL) {
		struct csi_info_t *ci = info->csi_info;

		for (int i = 0; i < MAX_NUM; i++, ci++) {
			if (!ci->sc_num || !ci->csi_data)
				continue;

			BK_LOG_RAW("SPCSIINFO type:%d iq:%d,[", ci->ltf_type, ci->sc_num);
			for (int j = 0; j < ci->sc_num; j++) {
				BK_LOG_RAW("%f + %f i,", ci->csi_data[j].real, ci->csi_data[j].imag);
			}
			BK_LOG_RAW("]\r\n");
		}
	}
}

/**
 ****************************************************************************************
 * @brief csi init
 *
 * @param[in] void
 ****************************************************************************************
 */
void wifi_csi_init(void)
{
	// register csi data out callback function
	bk_wifi_csi_info_cb_register(wifi_csi_rx_cb_demo);
}
#endif //CONFIG_SYS_CPU0

int main(void)
{
	#if CSI_WIFI_CONNECT
	wifi_sta_config_t sta_config = {0};
	wifi_csi_active_mode_config_t active_mode_config = {0};
	#endif

	bk_init();

	#if (CONFIG_SYS_CPU0)
	// PS CLOSE
	bk_wifi_ps_config(WIFI_PS_CMD_CLOSE,0,0);
	// capa tx_ampdu 0
	bk_wifi_capa_config(WIFI_CAPA_ID_TX_AMPDU_EN, 0);
	// capa rx_ampdu 0
	bk_wifi_capa_config(WIFI_CAPA_ID_RX_AMPDU_EN, 0);
	// csi init
	wifi_csi_init();

	#if CSI_WIFI_CONNECT
	rtos_init_semaphore(&wifi_conn_sema, 1);
	csi_wifi_event_handler_init();
	os_strcpy(sta_config.ssid, "beken");
	os_strcpy(sta_config.password, "12345678");
	bk_wifi_sta_split_init();
	bk_wifi_sta_split_start();
	bk_wifi_sta_set_config(&sta_config);
	bk_wifi_sta_split_connect();
	rtos_get_semaphore(&wifi_conn_sema, BEKEN_WAIT_FOREVER);
	active_mode_config.enable = true;
	active_mode_config.interval = 200;
	bk_wifi_csi_active_mode_req(&active_mode_config);
	rtos_deinit_semaphore(&wifi_conn_sema);
	#endif
	#endif

	return 0;
}
