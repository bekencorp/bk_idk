// Copyright 2020-2025 Beken
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

#include "wifi_test.h"
#include "statis.h"

#include <string.h>
#include <components/log.h>
#include <components/event.h>
#include <components/netif_types.h>
#include <modules/wifi.h>
#include <modules/wifi_types.h>
#include <os/os.h>
#include <os/mem.h>
#include <common/sys_config.h>

#define WIFI_REL_TAG "wifi"

static beken_thread_t s_wifi_reliability_thread = NULL;
static int s_cached_temperature_code;

void update_wifi_test_result(int temp_code, wifi_statis_event_t event)
{
    int idx = 0;

    if (event == WIFI_STATIS_EVENT_TICK) {
        g_reliability_stats.wifi.total_test_time++;
        return;
    }

    if (temp_code < -STATIS_LOW_TEMP_NUM) {
        idx = 0;
    } else if (temp_code > STATIS_HIGH_TEMP_NUM) {
        idx = STATIS_TEMP_RESULT_LEN - 1;
    } else {
        idx = temp_code + (STATIS_LOW_TEMP_NUM + 1);
    }

    g_reliability_stats.wifi.temp_count_tab[idx][event]++;
}

/* Same pattern as cli_wifi_event_cb() */
static bk_err_t wifi_reliability_wifi_event_cb(void *arg, event_module_t event_module, int event_id,
					     void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_network_found_t *network_found;

	(void)arg;
	if (event_module != EVENT_MOD_WIFI) {
		return BK_OK;
	}

	switch (event_id) {
	case EVENT_WIFI_STA_CONNECTED:
		sta_connected = (wifi_event_sta_connected_t *)event_data;
		if (sta_connected) {
			BK_RAW_LOGI(WIFI_REL_TAG, "BK STA connected %s, temp_code: %d\r\n", sta_connected->ssid,
				s_cached_temperature_code);
			update_wifi_test_result(s_cached_temperature_code, WIFI_STATIS_EVENT_CONNECT);
		}
		break;

	case EVENT_WIFI_STA_DISCONNECTED:
		sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
		if (sta_disconnected) {
			BK_RAW_LOGI(WIFI_REL_TAG, "BK STA disconnected, reason(%d)%s, temp_code: %d\r\n",
				sta_disconnected->disconnect_reason,
				sta_disconnected->local_generated ? ", local_generated" : "",
				s_cached_temperature_code);
			update_wifi_test_result(s_cached_temperature_code, WIFI_STATIS_EVENT_DISCONNECT);
		}
		break;

	case EVENT_WIFI_NETWORK_FOUND:
		network_found = (wifi_event_network_found_t *)event_data;
		if (network_found) {
			BK_RAW_LOGI(WIFI_REL_TAG, "target AP: %s, bssid " BK_MAC_FORMAT ", temp_code: %d\r\n",
				network_found->ssid, BK_MAC_STR(network_found->bssid), s_cached_temperature_code);
		}
		break;

#if CONFIG_WIFI_REGDOMAIN
	case EVENT_WIFI_REGDOMAIN_CHANGED: {
		wifi_event_reg_change_t *reg_change = (wifi_event_reg_change_t *)event_data;

		if (reg_change) {
			BK_RAW_LOGI(WIFI_REL_TAG, "regulatory domain changed to %c%c, temp_code: %d\r\n",
				reg_change->alpha2[0], reg_change->alpha2[1], s_cached_temperature_code);
		}
		break;
	}
#endif

	default:
		break;
	}

	return BK_OK;
}

/* Same pattern as cli_netif_event_cb(): EVENT_NETIF_GOT_IP4 / EVENT_NETIF_GOT_IP6 */
static bk_err_t wifi_reliability_netif_event_cb(void *arg, event_module_t event_module, int event_id,
						void *event_data)
{
	netif_event_got_ip4_t *got_ip;

	(void)arg;
	if (event_module != EVENT_MOD_NETIF) {
		return BK_OK;
	}

	switch (event_id) {
	case EVENT_NETIF_GOT_IP4:
	case EVENT_NETIF_GOT_IP6:
		got_ip = (netif_event_got_ip4_t *)event_data;
		if (got_ip) {
			BK_RAW_LOGI(WIFI_REL_TAG, "%s got ip%d:%s, temp_code: %d\r\n",
				got_ip->netif_if == NETIF_IF_STA ? "BK STA" : "unknown netif",
				event_id == EVENT_NETIF_GOT_IP4 ? 4 : 6, got_ip->ip, s_cached_temperature_code);
			update_wifi_test_result(s_cached_temperature_code, WIFI_STATIS_EVENT_GOT_IP4);
		} else {
			BK_RAW_LOGI(WIFI_REL_TAG, "GOT_IP event with NULL data (id=%d)\r\n", event_id);
		}
		break;
	case EVENT_NETIF_DHCP_TIMEOUT:
		BK_RAW_LOGI(WIFI_REL_TAG, "DHCP timeout, temp_code: %d\r\n", s_cached_temperature_code);
		update_wifi_test_result(s_cached_temperature_code, WIFI_STATIS_EVENT_DHCP_TIMEOUT);
		break;
	default:
		break;
	}

	return BK_OK;
}

static void wifi_reliability_sta_connect(void)
{
	wifi_sta_config_t sta_config;

	os_memset(&sta_config, 0, sizeof(sta_config));

	strncpy(sta_config.ssid, CONFIG_WIFI_RELIABILITY_STA_SSID, sizeof(sta_config.ssid) - 1);
	sta_config.ssid[sizeof(sta_config.ssid) - 1] = '\0';

	strncpy(sta_config.password, CONFIG_WIFI_RELIABILITY_STA_PASSPHRASE, sizeof(sta_config.password) - 1);
	sta_config.password[sizeof(sta_config.password) - 1] = '\0';

	sta_config.security = (wifi_security_t)CONFIG_WIFI_RELIABILITY_STA_SECURITY;

	if (bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_reliability_wifi_event_cb, NULL) !=
	    BK_OK) {
		BK_RAW_LOGI(WIFI_REL_TAG, "register WiFi event cb failed\r\n");
	}

	if (bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_reliability_netif_event_cb,
				 NULL) != BK_OK) {
		BK_RAW_LOGI(WIFI_REL_TAG, "register EVENT_NETIF_GOT_IP4 failed\r\n");
	}

	if (bk_wifi_sta_set_config(&sta_config) != BK_OK) {
		BK_RAW_LOGI(WIFI_REL_TAG, "failed to set STA config\r\n");
	} else if (bk_wifi_sta_start() != BK_OK) {
		BK_RAW_LOGI(WIFI_REL_TAG, "failed to start STA\r\n");
	}
}

static void wifi_reliability_sta_monitor_thread(beken_thread_arg_t arg)
{
	(void)arg;

	rtos_delay_milliseconds(5000);

	s_cached_temperature_code = get_current_temperature_code();

	wifi_reliability_sta_connect();

	while (1) {
		rtos_delay_milliseconds(1000);
		s_cached_temperature_code = get_current_temperature_code();
		g_reliability_stats.wifi.total_test_time++;
		if (g_reliability_stats.wifi.total_test_time % CONFIG_WIFI_RELIABILITY_SAVE_INTERVAL == 0) {
			statis_save_to_flash();
			statis_print_all_results();
		}

		if (g_reliability_stats.wifi.total_test_time % CONFIG_WIFI_RELIABILITY_SHOW_TEMP_INTERVAL == 0) {
			BK_RAW_LOGI(WIFI_REL_TAG, "Current temperature code: %d\r\n", s_cached_temperature_code);
		}

	}
}

void wifi_reliability_start(void)
{
	if (s_wifi_reliability_thread == NULL) {
		BK_RAW_LOGI(WIFI_REL_TAG, "start STA monitor thread\r\n");
		rtos_create_thread(&s_wifi_reliability_thread, 4, "wifi_rel",
				   (beken_thread_function_t)wifi_reliability_sta_monitor_thread, 8192, NULL);
	}
}
