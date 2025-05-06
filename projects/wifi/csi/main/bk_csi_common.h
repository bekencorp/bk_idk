/* BK_CSI Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef BK_CSI_COMMON_H
#define BK_CSI_COMMON_H

#define CSI_TAG "csi_demo"
#define CSI_LOGD(...)       BK_LOGD(CSI_TAG, ##__VA_ARGS__)
#define CSI_LOGI(...)       BK_LOGI(CSI_TAG, ##__VA_ARGS__)
#define CSI_LOGW(...)       BK_LOGW(CSI_TAG, ##__VA_ARGS__)
#define CSI_LOGE(...)       BK_LOGE(CSI_TAG, ##__VA_ARGS__)

#define BK_CSI_DEMO_DEBUG           (1)


#define WLAN_DEFAULT_IP         "192.168.188.1"
#define WLAN_DEFAULT_GW         "192.168.188.1"
#define WLAN_DEFAULT_MASK       "255.255.255.0"
#define WLAN_DEFAULT_DNS         "192.168.188.1"


#define STATE_NO_MOTION                       (0)
#define STATE_MOTION                          (1)

#define STATIC_EMPTY                          (0)
#define STATIC_CHANGE                         (1)

#define STATE_OUT_EMPTY                       (1)
#define STATE_OUT_MOTION                      (2)
#define STATE_OUT_STATIC_CHANGE               (3)


#define STATIC_DETECTION                      (100)
#define STATIC_DETECTION_THRESHOLD            (50)


typedef struct {
	uint8_t state;
	uint8_t state_static;
	uint8_t state_keep;
	uint32_t state1_num;
	uint32_t state2_num;
	uint32_t motion_cnt;
	uint32_t empty_cnt;
#if BK_CSI_DEMO_DEBUG
	uint32_t skeep_cnt;
#endif
} csi_wifi_alg_t;

typedef struct {
	uint8_t tx_type;
	uint8_t rx_mode;
	uint8_t format;
	uint8_t csi_work_type;
	uint8_t mode;
	uint8_t csi_work_identity;
	uint8_t gap_num;
	uint32_t interval;
	uint32_t gap;
	uint32_t data_cnt;
	uint32_t delay;
	uint8_t filter_mac_num;
	uint8_t mac[6*4];

	uint16_t rate1;
	uint16_t rate2;
	uint16_t rate3;
	double thres1;
	double thres2;
	double thres3;
	uint32_t static_update;
	uint32_t hold_time;
	uint32_t calibration_cnt;
	uint8_t cali_mode;
} csi_wifi_config_t;


int csi_data_rcv_data_handle(char * param);


#endif /* BK_CSI_COMMON_H */
