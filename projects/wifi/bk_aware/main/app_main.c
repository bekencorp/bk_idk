#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/shell_task.h>
#include <components/event.h>
#include <components/netif.h>
#include <modules/wifi.h>
#include "raw_link.h"
#include "bk_aware_demo.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);

#define APP_TAG "APP_DEMO"
#define APP_WIFI_CONNECT   0

static beken_semaphore_t wifi_app_sema = NULL;

static int app_wifi_event_cb(void *arg, event_module_t event_module,
						int event_id, void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_network_found_t *network_found;

	switch (event_id) {
		case EVENT_WIFI_STA_CONNECTED:
			sta_connected = (wifi_event_sta_connected_t *)event_data;
			BK_LOGI(APP_TAG, "BK STA connected %s\n", sta_connected->ssid);
			break;

		case EVENT_WIFI_STA_DISCONNECTED:
			sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
			BK_LOGI(APP_TAG, "BK STA disconnected, reason(%d)%s\n", sta_disconnected->disconnect_reason,
				sta_disconnected->local_generated ? ", local_generated" : "");
			break;

		case EVENT_WIFI_NETWORK_FOUND:
			network_found = (wifi_event_network_found_t *)event_data;
			BK_LOGI(APP_TAG, "target AP: %s, bssid %pm found\n", network_found->ssid, network_found->bssid);
			break;

		default:
			BK_LOGI(APP_TAG, "rx event <%d %d>\n", event_module, event_id);
			break;
	}

	return BK_OK;
}

static int app_netif_event_cb(void *arg, event_module_t event_module,
						int event_id, void *event_data)
{
	netif_event_got_ip4_t *got_ip;

	switch (event_id) {
		case EVENT_NETIF_GOT_IP4:
			got_ip = (netif_event_got_ip4_t *)event_data;
			BK_LOGI(APP_TAG, "%s got ip: %s\n", got_ip->netif_if == NETIF_IF_STA ? "BK STA" : "unknown netif", got_ip->ip);
			if (wifi_app_sema) {
				rtos_set_semaphore(&wifi_app_sema);
			}
			break;

		default:
			BK_LOGI(APP_TAG, "rx event <%d %d>\n", event_module, event_id);
			break;
	}

	return BK_OK;
}

void user_app_main(void){
#if APP_WIFI_CONNECT
	wifi_sta_config_t sta_config = {0};

	rtos_init_semaphore(&wifi_app_sema, 1);
	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, app_wifi_event_cb, NULL);
	bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, app_netif_event_cb, NULL);
	os_strcpy(sta_config.ssid, "beken");
	os_strcpy(sta_config.password, "12345678");
	bk_wifi_sta_split_init();
	bk_wifi_sta_split_start();
	bk_wifi_sta_set_config(&sta_config);
	bk_wifi_sta_split_connect();
	rtos_get_semaphore(&wifi_app_sema, BEKEN_WAIT_FOREVER);
#endif
	bk_aware_demo_main();
#if APP_WIFI_CONNECT
	rtos_deinit_semaphore(&wifi_app_sema);
#endif
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
#endif
	bk_init();

	return 0;
}
