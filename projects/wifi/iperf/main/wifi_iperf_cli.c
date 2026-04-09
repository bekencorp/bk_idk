#include "conv_utf8_pub.h"
#include "wifi_iperf_cli.h"


beken_semaphore_t wifi_iperf_cmd_sema = NULL;
int wifi_iperf_cmd_status = 0;

extern void iperf(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);


static int wifi_iperf_hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int wifi_iperf_hex2byte(const char *hex)
{
	int a, b;
	a = wifi_iperf_hex2num(*hex++);
	if (a < 0)
		return -1;
	b = wifi_iperf_hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

int wifi_iperf_hexstr2bin_cli(const char *hex, u8 *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	u8 *opos = buf;

	for (i = 0; i < len; i++) {
		a = wifi_iperf_hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}

void wifi_iperf_scan_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;

	if (argc < 2) {
		demo_scan_app_init();
	} else {
		uint8_t *ap_ssid;

		ap_ssid = (uint8_t *)argv[1];
		demo_scan_adv_app_init(ap_ssid);
	}

	if(ret == 0)
		msg = WIFI_IPERF_CMD_RSP_SUCCEED;
	else
		msg = WIFI_IPERF_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

int wifi_iperf_cli_find_id(int argc, char **argv, char *param)
{
	int i;
	int index;

	index = -1;
	if (NULL == param)
		goto find_over;

	for (i = 1; i < argc; i ++) {
		if (os_strcmp(argv[i], param) == 0) {
			index = i;
			break;
		}
	}

find_over:
	return index;
}

void wifi_iperf_ap_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *ap_ssid = NULL;
	char *ap_key = "";
	char *ap_channel = NULL;
	int ret = 0;
	char *msg = NULL;

	if (argc == 2)
		ap_ssid = argv[1];
	else if (argc == 3) {
		ap_ssid = argv[1];
		if (os_strlen(argv[2]) <= 2)
			ap_channel = argv[2];
		else
			ap_key = argv[2];
	} else if (argc == 4) {
		ap_ssid = argv[1];
		ap_key = argv[2];
		ap_channel = argv[3];
	}else{
		IPERF_LOGD("Invalid parameters\n");
		return;
	}

	if (ap_ssid) {
		ret = demo_softap_app_init(ap_ssid, ap_key, ap_channel);
	}
	if(ret == 0)
		msg = WIFI_IPERF_CMD_RSP_SUCCEED;
	else
		msg = WIFI_IPERF_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#include "conv_utf8_pub.h"
void wifi_iperf_sta_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *ssid = NULL;
	char *password = "";
#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
	u8 *psk = 0;
#endif
	char *msg = NULL;
	int ret;

	if ((argc < 2) || (argc > 6)) {
		IPERF_LOGD("invalid argc number\n");
		goto error;
	}

	int err = rtos_init_semaphore(&wifi_iperf_cmd_sema, 1);
	if (err != kNoErr) {
		goto error;
	}

#ifdef CONFIG_BSSID_CONNECT
	uint8_t bssid[6] = {0};
	if (os_strcmp(argv[1], "bssid") == 0) {
		if(argc >= 3) {
			wifi_iperf_hexstr2bin_cli(argv[2], bssid, 6);
		}
		if(argc >= 4) {
			password = argv[3];
		}
		demo_sta_bssid_app_init(bssid, password);
		if (wifi_iperf_cmd_sema != NULL)
		{
			err = rtos_get_semaphore(&wifi_iperf_cmd_sema, 10000);
			if (err != kNoErr)
			{
				goto error;
			}
			else
			{
				if (wifi_iperf_cmd_status == 1)
				{
					msg = WIFI_IPERF_EVT_GOT_IP;
					os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
					rtos_deinit_semaphore(&wifi_iperf_cmd_sema);
					wifi_iperf_cmd_status = 0;
					return;
				}
				else
				{
					goto error;
				}
			}
		}

	}
#endif

	if (argc >= 2)
		ssid = argv[1];

	if (argc >= 3)
		password = argv[2];

#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
	if (argc >= 4) {
		psk = (u8 *)argv[2];
		password = argv[3];
	}
#endif

#if 0 //TODO support BSSID/Channel configuration
	if (argc >= 4)
		bssid = argv[3];

	if (argc >= 5)
		channel = argv[4];
#endif

	char *oob_ssid_tp = ssid;
#if CONFIG_USE_CONV_UTF8
	oob_ssid_tp = (char *)conv_utf8((uint8_t *)ssid);
#endif

	if (oob_ssid_tp) {
#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
	ret = demo_sta_app_init((char *)oob_ssid_tp, psk, password);
#else
	ret = demo_sta_app_init((char *)oob_ssid_tp, password);
#endif
	if (ret == -1)
		goto error;

#if CONFIG_USE_CONV_UTF8
		os_free(oob_ssid_tp);
#endif
	} else {
		IPERF_LOGD("not buf for utf8\r\n");
		goto error;
	}

	goto exit;

error:
	msg = WIFI_IPERF_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
exit:
	if (wifi_iperf_cmd_sema != NULL)
		rtos_deinit_semaphore(&wifi_iperf_cmd_sema);
	return;
}

void wifi_iperf_stop_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;

	if (argc == 2) {
		if (os_strcmp(argv[1], "sta") == 0) {
			ret = bk_wifi_sta_stop();
		} else if (os_strcmp(argv[1], "ap") == 0)
			ret = bk_wifi_ap_stop();
		else {
			IPERF_LOGD("unknown WiFi interface\n");
			goto error;
		}
	} else {
		IPERF_LOGD("bad parameters\r\n");
		goto error;
	}

	if (!ret) {
		msg = WIFI_IPERF_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}
	else {
		goto error;
	}

error:
	msg = WIFI_IPERF_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}


int wifi_iperf_event_cb(void *arg, event_module_t event_module,
					  int event_id, void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_ap_disconnected_t *ap_disconnected;
	wifi_event_ap_connected_t *ap_connected;

	switch (event_id) {
	case EVENT_WIFI_STA_CONNECTED:
		sta_connected = (wifi_event_sta_connected_t *)event_data;
		IPERF_LOGD("BK STA connected %s\n", sta_connected->ssid);
		break;

	case EVENT_WIFI_STA_DISCONNECTED:
		sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
		IPERF_LOGD("BK STA disconnected, reason(%d)%s\n", sta_disconnected->disconnect_reason,
			sta_disconnected->local_generated ? ", local_generated" : "");
		break;

	case EVENT_WIFI_AP_CONNECTED:
		ap_connected = (wifi_event_ap_connected_t *)event_data;
		IPERF_LOGD(BK_MAC_FORMAT" connected to BK AP\n", BK_MAC_STR(ap_connected->mac));
		break;

	case EVENT_WIFI_AP_DISCONNECTED:
		ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
		IPERF_LOGD(BK_MAC_FORMAT" disconnected from BK AP\n", BK_MAC_STR(ap_disconnected->mac));
		break;

	default:
		IPERF_LOGD("rx event <%d %d>\n", event_module, event_id);
		break;
	}

	return BK_OK;
}

int wifi_iperf_netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
	netif_event_got_ip4_t *got_ip;

	switch (event_id) {
	case EVENT_NETIF_GOT_IP4:
		if (wifi_iperf_cmd_sema != NULL) {
			wifi_iperf_cmd_status = 1;
			rtos_set_semaphore(&wifi_iperf_cmd_sema);
		}
		got_ip = (netif_event_got_ip4_t *)event_data;
		IPERF_LOGD("%s got ip\n", got_ip->netif_if == NETIF_IF_STA ? "BK STA" : "unknown netif");
		break;
	default:
		IPERF_LOGD("rx event <%d %d>\n", event_module, event_id);
		break;
	}

	return BK_OK;
}


#define WIFI_IPERF_CMD_CNT (sizeof(s_wifi_iperf_commands) / sizeof(struct cli_command))
static const struct cli_command s_wifi_iperf_commands[] = {
	{"scan", "scan [ssid]", wifi_iperf_scan_cmd},
	{"ap", "ap ssid [password] [channel[1:14]]", wifi_iperf_ap_cmd},
	{"sta", "sta ssid [password][bssid][channel]", wifi_iperf_sta_cmd}, //TODO support connect speicific BSSID
	{"stop", "stop {sta|ap}", wifi_iperf_stop_cmd},
	{"iperf", "iperf help", iperf },
};

int wifi_cli_iperf_init(void)
{
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_iperf_event_cb, NULL));
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_iperf_netif_event_cb, NULL));

	return cli_register_commands(s_wifi_iperf_commands, WIFI_IPERF_CMD_CNT);
}


