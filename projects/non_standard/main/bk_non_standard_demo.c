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
#include "FreeRTOS.h"
#include "timers.h"
#include "timer_driver.h"
#include "bk_non_standard_demo.h"

static bool bk_non_standard_demo_started = false;

static uint8_t g_mac_hdr_type = 0xB0;
static uint8_t g_mac_hdr_oui[3]  = { 0xC8, 0x47, 0x8C }; 
static uint8_t g_broadcast_mac[BK_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t g_unicast_buffer[6] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 };
__maybe_unused static uint8_t g_broadcast_buffer[6] = { 0x7, 0x8, 0x9, 0xa, 0xb, 0xc };
__maybe_unused static uint8_t *trans_buf;

static TimerHandle_t td_timer = NULL;
static bool td_timer_started = false;
static uint64_t total_recv_cnt = 0;
static bool throughput_test_started = false;

static bool running_mtmr_test = false;

static bk_err_t bk_non_standard_demo_send_cb(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status) {
	if (!throughput_test_started && !running_mtmr_test) {
		BK_LOGI(NON_STANDARD_TAG, "send to "MACSTR", status:%d\r\n", MAC2STR(peer_mac_addr), status);
	}
	return BK_OK;
}

static bk_err_t bk_non_standard_demo_recv_cb(bk_rlk_recv_info_t *rx_info) {
	GPIO_DOWN(6);
	GPIO_DOWN(7);
	if (!td_timer_started && !running_mtmr_test) {
		BK_LOGI(NON_STANDARD_TAG, "recv from "MACSTR", rssi:%d, len:%u\r\n", MAC2STR(rx_info->src_addr), rx_info->rssi, rx_info->len);
	} else {
		total_recv_cnt += (BK_NON_STANDARD_HEADER_LEN + rx_info->len);
	}

	return BK_OK;
}

static void bk_non_standard_demo_slave_cb(bool sosp) {
	if (sosp)  {
		BK_LOGI(NON_STANDARD_TAG, "slave cb, recv SOSP\r\n");
		//start stransmit
		bk_non_standard_slave_send(g_unicast_buffer, 6);
	} else {
		BK_LOGI(NON_STANDARD_TAG, "slave cb, recv EOSP\r\n");
		//stop stransmit
	}
}

static void bk_non_standard_demo_period_broadcast(uint8_t interval, uint16_t count) {
	bk_err_t err;
	for (int i = 0; i < count; i++) {
		GPIO_DOWN(6);
		GPIO_DOWN(7);
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
	BK_LOGI(NON_STANDARD_TAG, "bk_rlk_send_ex_cb status:%u\n", status);
}

static void bk_non_standard_demo_unicast(uint8_t *mac_addr) {
	bk_err_t err;

	bk_rlk_config_info_t rlk_tx;
	rlk_tx.len            = 6;
	rlk_tx.data           = g_unicast_buffer;
	rlk_tx.cb             = bk_rlk_send_ex_cb;
	rlk_tx.args           = (void *)&unicast_ack_cnt;
	//rlk_tx.tx_rate        = LEGACY_RATE_24MBPS;
	rlk_tx.tx_power       = 0x7F;
	rlk_tx.tx_retry_cnt   = 0;
	rlk_tx.format         = BK_RLK_FORMAT_HT;
	rlk_tx.band_width     = BK_RLK_BW_20;
	rlk_tx.gi_type        = BK_RLK_LONG_GI;
	rlk_tx.ht_mcs         = BK_RLK_HT_MCS_3;

	for (int i = 0; i < 100; i++) {
		if ((err = bk_rlk_send_ex(mac_addr, &rlk_tx)) < BK_OK) {
			BK_LOGI(NON_STANDARD_TAG, "Send error, err: %d\n", err);
			return;
		}
		bk_timer_delay_us(10 * 1000);
	}
	BK_LOGI(NON_STANDARD_TAG, "unicast done\r\n");
}

#if CONFIG_TASK_WDT
extern void bk_task_wdt_feed(void);
#endif

static void throughput_test_task(void *pvParameter) {
	trans_buf = (uint8_t*)os_malloc(BK_NON_STANDARD_DEMO_TRANS_BUF_SIZE * sizeof(uint8_t));
	if (trans_buf == NULL) {
		BK_LOGI(NON_STANDARD_TAG, "alloc trans_buf fail\r\n");
		goto exit;
	}
	memset(trans_buf, 0, BK_NON_STANDARD_DEMO_TRANS_BUF_SIZE * sizeof(uint8_t));
	while (1) {
		if (throughput_test_started == false) {
			goto exit;
		}
		#if CONFIG_TASK_WDT
		bk_task_wdt_feed();
		#endif
		bk_err_t err;
		//bk_timer_delay_us(1000);
		if ((err = bk_rlk_send_by_oui(g_broadcast_mac, trans_buf, BK_NON_STANDARD_DEMO_TRANS_BUF_SIZE, g_mac_hdr_type, g_mac_hdr_oui)) < BK_OK) {
			BK_LOGI(NON_STANDARD_TAG, "send error, err: %d\r\n", err);
			goto exit;
		}
	}

exit:
	BK_LOGI(NON_STANDARD_TAG, "exit throughput test\r\n");
	os_free(trans_buf);
	trans_buf = NULL;
	rtos_delete_thread(NULL);
}

static void bk_non_standard_demo_throughput_test(uint8_t start) {
	if (start == 0) {
		BK_LOGI(NON_STANDARD_TAG, "stop throughput test\r\n");
		throughput_test_started = false;
	} else {
		if (throughput_test_started) {
			BK_LOGI(NON_STANDARD_TAG, "throughput test ongoing\r\n");
			return;
		}
		BK_LOGI(NON_STANDARD_TAG, "start throughput test\r\n");
		throughput_test_started = true;
		beken_thread_t thd;
		rtos_create_thread(&thd, BEKEN_DEFAULT_WORKER_PRIORITY, "throughput_test_task",
					(beken_thread_function_t)throughput_test_task,
					2048, NULL);
	}
}

static void traffic_detection(TimerHandle_t xTimer) {
	if (total_recv_cnt != 0) {
		double throughput = (double)((double)total_recv_cnt * 8) / 1000000;
		BK_LOGI(NON_STANDARD_TAG, "recv_cnt:%llu, throughput:%fMbps\r\n", total_recv_cnt, throughput);
		total_recv_cnt = 0;
	}
	if (xTimerReset(td_timer, pdMS_TO_TICKS(1000)) != pdPASS) {
		BK_LOGI(NON_STANDARD_TAG, "reset timer fail\r\n");
	}
}

static void bk_non_standard_demo_traffic_detection(uint8_t start) {
	if (start == 0) {
		BK_LOGI(NON_STANDARD_TAG, "stop traffic detection\r\n");
		td_timer_started = false;
		if (xTimerStop(td_timer, 0) != pdPASS) {
			BK_LOGI(NON_STANDARD_TAG, "stop td_timer fail\r\n");
		}
		return;
	} else {
		if (td_timer_started) {
			BK_LOGI(NON_STANDARD_TAG, "traffifc detection ongoing\r\n");
			return;
		}
		BK_LOGI(NON_STANDARD_TAG, "start traffic detection\r\n");
		td_timer_started = true;
		total_recv_cnt = 0;
		if (xTimerStop(td_timer, 0) != pdPASS) {
			BK_LOGI(NON_STANDARD_TAG, "stop td_timer fail\r\n");
			return;
		}
		if (td_timer != NULL) {
			if (xTimerStart(td_timer, 0) != pdPASS) {
				BK_LOGI(NON_STANDARD_TAG, "start td_timer fail\r\n");
				xTimerDelete(td_timer, 0);
				return;
			}
		} else {
			BK_LOGI(NON_STANDARD_TAG, "td_timer is NULL\r\n");
			return;
		}
	}
}

static void cli_non_standard_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv) {
	uint8_t interval;
	uint16_t count;
	uint8_t mac_addr[6] = {0};
	uint8_t start = 0;
	uint8_t max_tx_num = 0;
	uint8_t max_rx_num = 0;
	uint8_t slave_mode = 0;
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
	else if(strcmp(argv[1], "throughput") == 0) {
		start = strtoul(argv[2], NULL, 10) & 0xFFFF;
		bk_non_standard_demo_throughput_test(start);
	}
	else if(strcmp(argv[1], "td") == 0) {
		start = strtoul(argv[2], NULL, 10) & 0xFFFF;
		bk_non_standard_demo_traffic_detection(start);
	}
	else if(strcmp(argv[1], "master") == 0) {
		running_mtmr_test = true;
		if (strcmp(argv[2], "init") == 0) {
			max_tx_num = strtoul(argv[3], NULL, 10) & 0xFF;
			max_rx_num = strtoul(argv[4], NULL, 10) & 0xFF;
			bk_non_standard_master_init(max_tx_num, max_rx_num);
		}
	}
	else if(strcmp(argv[1], "slave") == 0) {
		running_mtmr_test = true;
		if (strcmp(argv[2], "init") == 0) {
			slave_mode = strtoul(argv[3], NULL, 10) & 0xFF;
			bk_non_standard_slave_init(slave_mode, bk_non_standard_demo_slave_cb);
		} else if (strcmp(argv[2], "connect") == 0) {
			bk_non_standard_slave_connect();
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
	GPIO_UP(6);
	GPIO_UP(7);

	bk_rlk_init();
	//bk_rlk_set_channel(BK_NON_STANDARD_DEMO_DEFAULT_CHANNEL);
	bk_rlk_set_channel(BK_NON_STANDARD_DEMO_DEFAULT_PRIM_CHANNEL, BK_NON_STANDARD_DEMO_DEFAULT_SECOND_CHANNEL);
	bk_rlk_register_send_cb(bk_non_standard_demo_send_cb);
	bk_rlk_register_recv_cb(bk_non_standard_demo_recv_cb);
	bk_rlk_add_white_list(g_mac_hdr_type, g_mac_hdr_oui);

	td_timer = xTimerCreate(
		"td_timer",
		pdMS_TO_TICKS(1000),
		pdFALSE,
		(void*)0,
		traffic_detection
	);
	if (td_timer == NULL) {
		BK_LOGI(NON_STANDARD_TAG, "create td_timer fail\r\n");
		return;
	}

	cli_register_commands(s_non_standard_commands, 1);

	bk_non_standard_demo_started = true;
}

static void bk_non_standard_demo_deinit() {
	cli_unregister_commands(s_non_standard_commands, 1);

	xTimerDelete(td_timer, 0);

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
