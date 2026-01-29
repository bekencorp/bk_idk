/* BK_NON_STANDARD Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <common/bk_include.h>
#include <components/system.h>
#include <components/log.h>
#include <modules/raw_link.h>
#include <os/mem.h>
#include <os/os.h>
#include "bk_cli.h"
#include "cli.h"
#include "wifi.h"
#include <driver/timer.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "timer_driver.h"
#include "bk_non_standard_demo.h"

#if CONFIG_TASK_WDT
extern void bk_task_wdt_feed(void);
#endif

static bool bk_non_standard_demo_started = false;

static uint8_t g_mac_hdr_type = 0xB0;
static uint8_t g_mac_hdr_oui[3]  = { 0xC8, 0x47, 0x8C }; 

//for broadcast and unicast test
static uint8_t g_broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t g_unicast_buffer[6] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 };
static uint8_t g_broadcast_buffer[6] = { 0x7, 0x8, 0x9, 0xa, 0xb, 0xc };

//for delay test
static bool recv_flag = false;

//for tput test
static bool trans_flag = true;
static uint8_t *trans_buf = NULL;
static uint64_t recv_cnt = 0;

static bk_err_t bk_non_standard_demo_send_cb(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status) {
	//BK_LOGI(NON_STANDARD_TAG, "send to "MACSTR", status:%d\r\n", MAC2STR(peer_mac_addr), status);

	//for delay test
	GPIO_DOWN(6);
	trans_flag = true;

	return BK_OK;
}

static bk_err_t bk_non_standard_demo_recv_cb(bk_rlk_recv_info_t *rx_info) {
	//BK_LOGI(NON_STANDARD_TAG, "recv from "MACSTR", rssi:%d, len:%u\r\n", MAC2STR(rx_info->src_addr), rx_info->rssi, rx_info->len);

	//for delay test
	if (recv_flag == false) {
		recv_flag = true;
		GPIO_UP(18);
	} else {
		recv_flag = false;
		GPIO_DOWN(18);
	}

	//for tput test
	recv_cnt += (BK_NON_STANDARD_HEADER_LEN + rx_info->len);
	return BK_OK;
}

static void bk_non_standard_demo_period_broadcast(uint8_t interval, uint16_t count) {
	bk_err_t err;
	for (int i = 0; i < count; i++) {
		GPIO_UP(6);
		if ((err = bk_rlk_send_by_oui(g_broadcast_mac, g_broadcast_buffer, 6, g_mac_hdr_type, g_mac_hdr_oui)) < BK_OK) {
			BK_LOGI(NON_STANDARD_TAG, "send error, err: %d\r\n", err);
			break;
		}
		bk_timer_delay_us(interval * 1000);
	}
	BK_LOGI(NON_STANDARD_TAG, "period broadcast done\r\n");
}

static uint8_t unicast_ack_cnt = 0;
static void bk_rlk_send_ex_cb(void *args, bool status) {
	//BK_LOGI(NON_STANDARD_TAG, "bk_rlk_send_ex_cb status:%u\n", status);
}
static void bk_non_standard_demo_unicast(uint8_t *mac_addr) {
	bk_err_t err;

	bk_rlk_config_info_t *rlk_tx = (bk_rlk_config_info_t *)os_zalloc(sizeof(bk_rlk_config_info_t));
	rlk_tx->len            = 6;
	rlk_tx->data           = g_unicast_buffer;
	rlk_tx->cb             = bk_rlk_send_ex_cb;
	rlk_tx->args           = (void *)&unicast_ack_cnt;
	rlk_tx->tx_rate        = LEGACY_RATE_24MBPS;
	rlk_tx->tx_power       = 0x7F;
	rlk_tx->tx_retry_cnt   = 3;
	/*rlk_tx->format       = BK_RLK_FORMAT_HT;
	rlk_tx->band_width     = BK_RLK_BW_20;
	rlk_tx->gi_type        = BK_RLK_LONG_GI;
	rlk_tx->ht_mcs         = BK_RLK_HT_MCS_3;*/

	if ((err = bk_rlk_send_ex(mac_addr, rlk_tx)) < BK_OK) {
		BK_LOGI(NON_STANDARD_TAG, "Send error, err: %d\n", err);
		return;
	}
	BK_LOGI(NON_STANDARD_TAG, "unicast done\r\n");
}

static void tput_tx_task(void *pvParameters) {
	bk_err_t err;
	while (1) {
		#if CONFIG_TASK_WDT
		bk_task_wdt_feed();
		#endif
		if (trans_flag) {
			trans_flag = false;
			if ((err = bk_rlk_send_by_oui(g_broadcast_mac, trans_buf, BK_NON_STANDARD_DEMO_TRANS_BUF_SIZE, g_mac_hdr_type, g_mac_hdr_oui)) < BK_OK) {
				BK_LOGI(NON_STANDARD_TAG, "send failed, err:%d\r\n", err);
				goto exit;
			}
		}
	}

exit:
	rtos_delete_thread(NULL);
}

static void bk_non_standard_demo_tput_tx() {
	trans_buf = (uint8_t *)os_zalloc(BK_NON_STANDARD_DEMO_TRANS_BUF_SIZE * sizeof(uint8_t));
	if (trans_buf == NULL) {
		BK_LOGI(NON_STANDARD_TAG, "trans_buf malloc failed\r\n");
		return;
	}

	beken_thread_t thd;
	rtos_create_thread(&thd, BEKEN_DEFAULT_WORKER_PRIORITY, "tput_tx_task",
		(beken_thread_function_t)tput_tx_task, 2048, NULL);
}

static void td_timer_cb(timer_id_t id) {
	if (recv_cnt != 0) {
		double throughput = (double)((double)recv_cnt * 8) / 1000000;
		BK_LOGI(NON_STANDARD_TAG, "recv_cnt:%llu, throughput:%fMbps\r\n", recv_cnt, throughput);
		recv_cnt = 0;
	}
}

static void bk_non_standard_demo_tput_rx() {
	bk_timer_delay_with_callback(TIMER_ID1, 1 * 1000 * 1000, td_timer_cb);
}

static void cli_non_standard_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv) {
	uint8_t interval;
	uint16_t count;
	uint8_t mac_addr[6] = {0};
	uint8_t rate_idx = 0;
	char *msg = NULL;
	if (argc <= 1)
		goto error;

	if(strcmp(argv[1], "broadcast") == 0) {
		if (argc < 4) {
			BK_LOGI(NON_STANDARD_TAG, "broadcast require interval and count\r\n");
			goto error;
		}
		interval = strtoul(argv[2], NULL, 10) & 0xFFFF;
		count    = strtoul(argv[3], NULL, 10) & 0xFFFF;
		bk_non_standard_demo_period_broadcast(interval, count);
	}
	else if(strcmp(argv[1], "unicast") == 0) {
		if (argc < 3) {
			BK_LOGI(NON_STANDARD_TAG, "unicast require mac addr\r\n");
			goto error;
		}
		hexstr2bin_cli(argv[2], mac_addr, 6);
		bk_non_standard_demo_unicast(mac_addr);
	}
	else if(strcmp(argv[1], "tput") == 0) {
		if (strcmp(argv[2], "rx") == 0) {
			bk_non_standard_demo_tput_rx();
		} else if (strcmp(argv[2], "tx") == 0) {
			rate_idx = strtoul(argv[3], NULL, 10) & 0xFF;
			bk_rlk_set_tx_rate(rate_idx);
			bk_non_standard_demo_tput_tx();
		} else {
			BK_LOGI(NON_STANDARD_TAG, "invalid paramter\r\n");
			goto error;
		}
	}
	else {
		BK_LOGI(NON_STANDARD_TAG, "invalid paramter\r\n");
		goto error;
	}

	msg = WIFI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

static const struct cli_command s_non_standard_commands[] = {
	{"non_standard", "non_standard test", cli_non_standard_cmd},
};

static void bk_non_standard_demo_init() {
	bk_rlk_init();
	bk_rlk_set_channel(BK_NON_STANDARD_DEMO_DEFAULT_CHANNEL, 0);
	//bk_rlk_set_channel(BK_NON_STANDARD_DEMO_DEFAULT_PRIM_CHANNEL, BK_NON_STANDARD_DEMO_DEFAULT_SECOND_CHANNEL);
	bk_rlk_register_send_cb(bk_non_standard_demo_send_cb);
	bk_rlk_register_recv_cb(bk_non_standard_demo_recv_cb);
	bk_rlk_add_white_list(g_mac_hdr_type, g_mac_hdr_oui);

	cli_register_commands(s_non_standard_commands, 1);
	bk_non_standard_demo_started = true;
}

static void bk_non_standard_demo_deinit() {
	cli_unregister_commands(s_non_standard_commands, 1);
	bk_rlk_deinit();
	bk_non_standard_demo_started = false;
}

void bk_non_standard_demo_main() {
	if (bk_non_standard_demo_started) {
		BK_LOGI(NON_STANDARD_TAG, "demo already started\r\n");
		return;
	}
    bk_non_standard_demo_init();
}

void bk_non_standard_demo_stop() {
	if (!bk_non_standard_demo_started) {
		BK_LOGI(NON_STANDARD_TAG, "demo already stopped\r\n");
		return;
	}  
	bk_non_standard_demo_deinit();
}
