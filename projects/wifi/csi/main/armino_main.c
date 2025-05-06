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
#include "bk_csi_demo.h"
#include "bk_csi_ble_provision.h"
#include "bk_csi_common.h"
#include "cJSON.h"
#include <os/os.h>
#include <os/str.h>

#if (CONFIG_SYS_CPU0)
#if CONFIG_WIFI_CSI_DEMO
/*
 * GLOBAL VARIABLE
 ****************************************************************************************
 */
static csi_wifi_alg_t csi_wifi_alg_param;
static csi_wifi_config_t csi_wifi_config;
bool g_demo_is_app_ctrl = false;
bool g_demo_is_delay_resp = false;
uint32_t g_demo_is_delay_cnt = 0;
uint8_t g_static_cali_ctrl = 0;

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */
void csi_wifi_alg_result_handle(double result_thres1,double result_thres2);
extern bk_err_t bk_wifi_csi_alg_config_internal(double thres1,struct wifi_csi_cfg_dbg debug_param);
extern bk_err_t bk_wifi_csi_start_req_internal(uint8_t csi_work_type,uint8_t csi_work_mode,uint8_t csi_work_identity,uint8_t csi_data_format,
												uint32_t csi_data_interval,uint32_t delay,struct wifi_csi_start_dbg debug_param);

/*
 * FUNCTION DEFINITION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief csi wifi sta start
 *
 * @param[in] ssid        sta ssid
 * @param[in] password    sta password
 ****************************************************************************************
 */
static void csi_wifi_sta_connect(char* ssid,char*password)
{
	wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

	strncpy(sta_config.ssid, ssid, WIFI_SSID_STR_LEN);
	strncpy(sta_config.password, password, WIFI_PASSWORD_LEN);

	CSI_LOGI("STA ssid:%s password:%s\n", sta_config.ssid, sta_config.password);
	BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
	BK_LOG_ON_ERR(bk_wifi_sta_start());
}

/**
 ****************************************************************************************
 * @brief csi wifi ap start
 *
 * @param[in] ssid    ap ssid
 * @param[in] password    ap password
 ****************************************************************************************
 */
static void csi_wifi_ap_start(char* ssid,char*password)
{
	wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
	netif_ip4_config_t ip4_config = {0};

	strncpy(ip4_config.ip, WLAN_DEFAULT_IP, NETIF_IP4_STR_LEN);
	strncpy(ip4_config.mask, WLAN_DEFAULT_MASK, NETIF_IP4_STR_LEN);
	strncpy(ip4_config.gateway, WLAN_DEFAULT_GW, NETIF_IP4_STR_LEN);
	strncpy(ip4_config.dns, WLAN_DEFAULT_DNS, NETIF_IP4_STR_LEN);
	BK_LOG_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));

	strncpy(ap_config.ssid, ssid, WIFI_SSID_STR_LEN);
	strncpy(ap_config.password, password, WIFI_PASSWORD_LEN);

	CSI_LOGI("SAP ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
	BK_LOG_ON_ERR(bk_wifi_ap_set_config(&ap_config));
	BK_LOG_ON_ERR(bk_wifi_ap_start());
}

/**
 ****************************************************************************************
 * @brief csi init algorithm param
 *
 * @param[in] void
 ****************************************************************************************
 */
void csi_wifi_alg_param_init(void)
{
	memset(&csi_wifi_alg_param,0,sizeof(csi_wifi_alg_t));
	memset(&csi_wifi_config,0,sizeof(csi_wifi_config_t));

	//default param
	csi_wifi_config.thres1 = 5;
	csi_wifi_config.thres2 = 3;
	csi_wifi_config.thres3 = 8;
	csi_wifi_config.rate1 = 8;
	csi_wifi_config.rate2 = 16;
	csi_wifi_config.rate3 = 8;
	csi_wifi_config.static_update = 15000;
	csi_wifi_config.calibration_cnt = 200;
	csi_wifi_config.hold_time = 0;
}
#endif //CONFIG_WIFI_CSI_DEMO

/**
 ****************************************************************************************
 * @brief csi wifi netif event cb
 *
 * @param[in] arg            param
 * @param[in] event_module   event module
 * @param[in] event_id       event id
 * @param[in] event_data     event data
 ****************************************************************************************
 */
int csi_wifi_netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
	netif_event_got_ip4_t *got_ip;

	switch (event_id) {
		case EVENT_NETIF_GOT_IP4:{
			got_ip = (netif_event_got_ip4_t *)event_data;
			CSI_LOGI("%s got ip\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif");
			}break;

		default:{
			CSI_LOGI("rx event <%d %d>\n", event_module, event_id);
			}break;
	}

	return BK_OK;
}

/**
 ****************************************************************************************
 * @brief csi wifi wifi event cb
 *
 * @param[in] arg            param
 * @param[in] event_module   event module
 * @param[in] event_id       event id
 * @param[in] event_data     event data
 ****************************************************************************************
 */
int csi_wifi_wifi_event_cb(void *arg, event_module_t event_module,
					  int event_id, void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_ap_disconnected_t *ap_disconnected;
	wifi_event_ap_connected_t *ap_connected;
	struct wifi_csi_alg_ind *csi_data_info;

	switch (event_id) {
		case EVENT_WIFI_CSI_ALG_IND:{
			csi_data_info = (struct wifi_csi_alg_ind *)event_data;

			#if CONFIG_WIFI_CSI_DEMO
			csi_wifi_alg_result_handle(csi_data_info->move_change_rate, csi_data_info->state_change_rate);
			#else
			CSI_LOGI("move_change_rate %d state_change_rate %d\n", csi_data_info->move_change_rate, csi_data_info->state_change_rate);
			#endif

			}break;

		case EVENT_WIFI_STA_CONNECTED:{
			sta_connected = (wifi_event_sta_connected_t *)event_data;
			CSI_LOGI("STA connected to %s\n", sta_connected->ssid);
			}break;

		case EVENT_WIFI_STA_DISCONNECTED:{
			sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
			CSI_LOGI("STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
			}break;

		case EVENT_WIFI_AP_CONNECTED:{
			ap_connected = (wifi_event_ap_connected_t *)event_data;
			CSI_LOGI(BK_MAC_FORMAT" connected to AP\n", BK_MAC_STR(ap_connected->mac));
			}break;

		case EVENT_WIFI_AP_DISCONNECTED:{
			ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
			CSI_LOGI(BK_MAC_FORMAT" disconnected from AP\n", BK_MAC_STR(ap_disconnected->mac));
			}break;

		default:{
			CSI_LOGI("rx event <%d %d>\n", event_module, event_id);
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
	if(info != NULL)
	{
		if(info->data_type == 0)// buf
		{
			BK_LOG_RAW("SPCSIINFO %d \r\n",info->len);
			for(int i=0;i<info->len;i++)
			{
				BK_LOG_RAW("%x \r\n",info->data.buf[i]);
			}
			BK_LOG_RAW("EPCSIINFO \r\n");
		}
		else if(info->data_type == 1)//iq
		{
			BK_LOG_RAW("SPCSIINFO iq:%d,[",info->len);
			for(int i=0;i<info->len;i++)
			{
				BK_LOG_RAW("%f + %f i,",info->data.iq[i].real,info->data.iq[i].imag);
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

	// initializate demo param
	#if CONFIG_WIFI_CSI_DEMO
	csi_wifi_alg_param_init();
	#endif
}
#if CONFIG_WIFI_CSI_DEMO
/**
 ****************************************************************************************
 * @brief csi send result to app by ble, DEMO
 *
 * @param[in] flag       result type
 * @param[in] result       result value
 ****************************************************************************************
 */
void csi_ind_send_to_app(uint8_t flag, uint32_t result)
{
	char payload[128] = {0};
	uint16 len = 0;

	len = os_snprintf(payload+5, 128-5, "{\"csi_ind\":%d,\"csi_result\":%d}", flag, result);
	//csi_code(2)|status(1)|length(2)|string
	payload[0] = CSI_BOARDING_OP_CSI_DATA_IND;
	payload[1] = 0;
	payload[2] = 0;
	payload[3] = len&0xFF;
	payload[4] = len>>8;
	bk_wifi_csi_ble_data_send((uint8_t *)payload, len+5);
}

/**
 ****************************************************************************************
 * @brief csi send result to app by ble
 *
 * @param[in] flag       result type, 0: response OK, 1: response fail with error reason
 * @param[in] result       result value,  error reason
 ****************************************************************************************
 */
void csi_resp_send_to_app(uint8_t flag, char* result)
{
	char payload[128] = {0};
	uint16 len = 0;

	if(flag == 0)
	{
		CSI_LOGI("resp ok\r\n");
		len = os_snprintf(payload+5, 128-5, "resp ok");
	}
	else
	{
		CSI_LOGI("resp fail, err param:%s.\r\n",result);
		len = os_snprintf(payload+5, 128-5, "resp fail,err param:%s",result);
	}
	//csi_code(2)|status(1)|length(2)|string
	payload[0] = CSI_BOARDING_OP_START_CSI_CONFIG;
	payload[1] = 0;
	payload[2] = flag;//status
	payload[3] = len&0xFF;
	payload[4] = len>>8;
	bk_wifi_csi_ble_data_send((uint8_t *)payload, len+5);
}

/**
 ****************************************************************************************
 * @brief handle csi demo config data from APP by BLE
 *
 * @param[in] param     data frame from ble
 ****************************************************************************************
 */
int csi_data_rcv_data_handle(char * param)
{
	cJSON *json = NULL;
	char ssid_ptr[WIFI_SSID_STR_LEN];
	char password_ptr[WIFI_PASSWORD_LEN];
	bool ssid_is_valid = false;
	uint8_t code_int = 0;
	uint8_t temp8 = 0;
	uint16_t temp16 = 0;
	uint32_t temp32 = 0;
	#define CSI_ERR_CODE_LEN   (20)
	char err_code[CSI_ERR_CODE_LEN] = {0};

	memset(err_code,0,CSI_ERR_CODE_LEN);
	memset(ssid_ptr,0,WIFI_SSID_STR_LEN);
	memset(password_ptr,0,WIFI_PASSWORD_LEN);

	// param : csi code(2) | csi string length(2) | json payload string
	uint16_t csi_code = param[0]|(((uint16_t)param[1])<<8);
	uint16_t csi_str_len = param[2]|(((uint16_t)param[3])<<8);
	CSI_LOGI("csi_data_rcv_data_handle csi_code %d,csi_str_len %d\r\n",csi_code,csi_str_len);

	// check csi code
	if(csi_code != CSI_BOARDING_OP_START_CSI_CONFIG)
	{
		CSI_LOGI("Error csi_code: [%d]\n", csi_code);
		os_snprintf(err_code, CSI_ERR_CODE_LEN, "csi_code %d",csi_code);
		goto csi_start_fail;
	}

	json = cJSON_Parse(param+4);
	if (!json)
	{
		CSI_LOGI("Error before: [%s]\n", cJSON_GetErrorPtr());
		os_snprintf(err_code, CSI_ERR_CODE_LEN, "json");
		goto csi_start_fail;
	}

	//code
	cJSON *code = cJSON_GetObjectItem(json, "code");
	if (code && ((code->type & 0xFF) == cJSON_Number))
	{
		code_int = code->valueint;
		CSI_LOGI("code :%d\r\n",code->valueint);
	}
	else
	{
		CSI_LOGI("[Error] not find msg code\n");
		goto csi_start_fail;
	}
#if 0
	//is_csi_start
	cJSON *is_csi_start = cJSON_GetObjectItem(json, "is_csi_start");
	if (is_csi_start && ((is_csi_start->type & 0xFF) == cJSON_String))
	{
		uint8_t csi_enable = os_strtoul(is_csi_start->valuestring,NULL,10);
		CSI_LOGI("is_csi_start :%s\r\n",is_csi_start->valuestring);
		if(csi_enable == 0)
		{
			CSI_LOGI("csi stop\r\n");
			bk_wifi_csi_stop_req();
			csi_resp_send_to_app(0,err_code);
			return 0;
		}
		else
		{
			CSI_LOGI("csi start\r\n");
		}
	}
	else
	{
		CSI_LOGE("[Error] not find msg is_csi_start\n");

		// TODO 
		//goto csi_start_fail;
	}
#endif
	// csi start config
	if(1 == code_int)
	{
		// clear original config
		bk_wifi_csi_stop_req();
		bk_wifi_ap_stop();
		bk_wifi_sta_stop();
		
		//ssid
		cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
		if (ssid && ((ssid->type & 0xFF) == cJSON_String))
		{
			CSI_LOGI("ssid :%s\r\n",ssid->valuestring);
			ssid_is_valid = true;
			strncpy(ssid_ptr, ssid->valuestring, WIFI_SSID_STR_LEN);
		}
		else
		{
			CSI_LOGI("[Error] not find msg ssid\n");
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "ssid");
			goto csi_start_fail;
		}
		//password
		cJSON *password = cJSON_GetObjectItem(json, "password");
		if (password && ((password->type & 0xFF) == cJSON_String))
		{
			CSI_LOGI("password :%s\r\n",password->valuestring);
			strncpy(password_ptr, password->valuestring, WIFI_PASSWORD_LEN);
		}
		else
		{
			CSI_LOGI("[Warning] not find msg password\n");
		}
		//tx_type
		cJSON *tx_type = cJSON_GetObjectItem(json, "tx_type");
		if (tx_type && ((tx_type->type & 0xFF) == cJSON_Number))
		{
			CSI_LOGI("tx_type :%d\r\n",tx_type->valueint);
			temp8 = tx_type->valueint;
			// check the parameter setting range
			if(temp8 < 3)
			{
				csi_wifi_config.tx_type = temp8;
			}
			else
			{
				CSI_LOGI("[Error]tx_type :%d\r\n",temp8);
				os_snprintf(err_code, CSI_ERR_CODE_LEN, "tx_type");
				goto csi_start_fail;
			}
		}
		else
		{
			CSI_LOGI("[Warning] not find msg tx_type\n");
		}
		//rx_mode
		cJSON *rx_mode = cJSON_GetObjectItem(json, "rx_mode");
		if (rx_mode && ((rx_mode->type & 0xFF) == cJSON_Number))
		{
			CSI_LOGI("rx_mode :%d\r\n",rx_mode->valueint);
			temp8 = rx_mode->valueint;
			// check the parameter setting range
			if(temp8 < 3)
			{
				csi_wifi_config.rx_mode = temp8;
			}
			else
			{
				CSI_LOGI("[Error]rx_mode :%d\r\n",temp8);
				os_snprintf(err_code, CSI_ERR_CODE_LEN, "rx_mode");
				goto csi_start_fail;
			}
		}
		else
		{
			CSI_LOGI("[Warning] not find msg rx_mode\n");
		}
		//format
		cJSON *format = cJSON_GetObjectItem(json, "format");
		if (format && ((format->type & 0xFF) == cJSON_Number))
		{
			CSI_LOGI("format :%d\r\n",format->valueint);
			temp8 = format->valueint;
			// check the parameter setting range
			if(temp8 < 8)
			{
				csi_wifi_config.format = temp8;
			}
			else
			{
				CSI_LOGI("[Error]format :%d\r\n",temp8);
				os_snprintf(err_code, CSI_ERR_CODE_LEN, "format");
				goto csi_start_fail;
			}
		}
		else
		{
			CSI_LOGI("[Warning] not find msg format\n");
		}
		//is_resp_null
		cJSON *csi_work_type = cJSON_GetObjectItem(json, "is_resp_null");
		if (csi_work_type && ((csi_work_type->type & 0xFF) == cJSON_Number))
		{
			CSI_LOGI("is_resp_null :%d\r\n",csi_work_type->valueint);
			temp8 = csi_work_type->valueint;
			// check the parameter setting range
			if(temp8 < 3)
			{
				csi_wifi_config.csi_work_type = temp8;
			}
			else
			{
				CSI_LOGI("[Error]csi_work_type :%d\r\n",temp8);
				os_snprintf(err_code, CSI_ERR_CODE_LEN, "csi_work_type");
				goto csi_start_fail;
			}
		}
		else
		{
			CSI_LOGI("[Warning] not find msg csi_work_type\n");
		}
		//interval
		cJSON *interval = cJSON_GetObjectItem(json, "interval");
		if (interval && ((interval->type & 0xFF) == cJSON_Number))
		{
			CSI_LOGI("interval :%d\r\n",interval->valueint);
			temp32 = interval->valueint;
			// check the parameter setting range
			if((temp32 < 10000)&&(temp32 > 10))
			{
				csi_wifi_config.interval = temp32;
			}
			else
			{
				CSI_LOGI("[Error]interval :%d\r\n",temp32);
				os_snprintf(err_code, CSI_ERR_CODE_LEN, "interval");
				goto csi_start_fail;
			}
		}
		else
		{
			CSI_LOGI("[Warning] not find msg interval\n");
		}
		//mode
		cJSON *mode = cJSON_GetObjectItem(json, "mode");
		if (mode && ((mode->type & 0xFF) == cJSON_Number))
		{
			CSI_LOGI("mode :%d\r\n",mode->valueint);
			temp8 = mode->valueint;
			// check the parameter setting range
			if(temp8 < 16)
			{
				csi_wifi_config.mode = temp8;
			}
			else
			{
				CSI_LOGI("[Error]mode :%d\r\n",temp8);
				os_snprintf(err_code, CSI_ERR_CODE_LEN, "mode");
				goto csi_start_fail;
			}
		}
		else
		{
			CSI_LOGI("[Warning] not find msg mode\n");
		}
		//type
		cJSON *csi_work_identity = cJSON_GetObjectItem(json, "type");
		if (csi_work_identity && ((csi_work_identity->type & 0xFF) == cJSON_Number))
		{
			CSI_LOGI("type :%d\r\n",csi_work_identity->valueint);
			temp8 = csi_work_identity->valueint;
			// check the parameter setting range
			if(temp8 < 3)
			{
				csi_wifi_config.csi_work_identity = temp8;
			}
			else
			{
				CSI_LOGI("[Error]csi_work_identity :%d\r\n",temp8);
				os_snprintf(err_code, CSI_ERR_CODE_LEN, "csi_work_identity");
				goto csi_start_fail;
			}
		}
		else
		{
			CSI_LOGI("[Error] not find msg csi_work_identity\n");
		}
	}
	//rate1
	cJSON *rate1 = cJSON_GetObjectItem(json, "rate1");
	if (rate1 && ((rate1->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("rate1 :%d\r\n",rate1->valueint);
		temp16 = rate1->valueint;
			// check the parameter setting range
		if((temp16 < 1024)&&(temp16 > 0))
		{
			csi_wifi_config.rate1 = temp16;
		}
		else
		{
			CSI_LOGI("[Error]rate1 :%d\r\n",temp16);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "rate1");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg rate1\n");
	}
	//rate2
	cJSON *rate2 = cJSON_GetObjectItem(json, "rate2");
	if (rate2 && ((rate2->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("rate2 :%d\r\n",rate2->valueint);
		temp16 = rate2->valueint;
		// check the parameter setting range
		if((temp16 < 1024)&&(temp16 > 0))
		{
			csi_wifi_config.rate2 = temp16;
		}
		else
		{
			CSI_LOGI("[Error]rate2 :%d\r\n",temp16);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "rate2");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg rate2\n");
	}
	//rate3
	cJSON *rate3 = cJSON_GetObjectItem(json, "rate3");
	if (rate3 && ((rate3->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("rate3 :%d\r\n",rate3->valueint);
		temp16 = rate3->valueint;
		// check the parameter setting range
		if((temp16 < 1024)&&(temp16 > 0))
		{
			csi_wifi_config.rate3 = temp16;
		}
		else
		{
			CSI_LOGI("[Error]rate3 :%d\r\n",temp16);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "rate3");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg rate3\n");
	}
	//thres1
	cJSON *thres1 = cJSON_GetObjectItem(json, "thres1");
	if (thres1 && ((thres1->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("thres1 :%d\r\n",thres1->valueint);
		temp16 = thres1->valueint;
		// check the parameter setting range
		if((temp16 < 100)&&(temp16 > 0))
		{
			csi_wifi_config.thres1 = temp16;
		}
		else
		{
			CSI_LOGI("[Error]thres1 :%d\r\n",temp16);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "thres1");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg thres1\n");
	}
	//thres2
	cJSON *thres2 = cJSON_GetObjectItem(json, "thres2");
	if (thres2 && ((thres2->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("thres2 :%d\r\n",thres2->valueint);
		temp16 = thres2->valueint;
		// check the parameter setting range
		if((temp16 < 100)&&(temp16 > 0))
		{
			csi_wifi_config.thres2 = temp16;
		}
		else
		{
			CSI_LOGI("[Error]thres2 :%d\r\n",temp16);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "thres2");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg thres2\n");
	}
	//thres3
	cJSON *thres3 = cJSON_GetObjectItem(json, "thres3");
	if (thres3 && ((thres3->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("thres3 :%d\r\n",thres3->valueint);
		temp16 = thres3->valueint;
		// check the parameter setting range
		if((temp16 < 100)&&(temp16 > 0))
		{
			csi_wifi_config.thres3 = temp16;
		}
		else
		{
			CSI_LOGI("[Error]thres3 :%d\r\n",temp16);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "thres3");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg thres3\n");
	}
	//static_update [time = static_update*interval(ms)]
	cJSON *static_update = cJSON_GetObjectItem(json, "static_update");
	if (static_update && ((static_update->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("static_update :%d\r\n",static_update->valueint);
		temp32 = static_update->valueint;
		// check the parameter setting range
		if(temp32 < 3600000)
		{
			csi_wifi_config.static_update = temp32;
		}
		else
		{
			CSI_LOGI("[Error]static_update :%d\r\n",temp32);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "static_update");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg static_update\n");
	}
	//hold_time [time = hold_time*interval(ms)]
	cJSON *hold_time = cJSON_GetObjectItem(json, "hold_time");
	if (hold_time && ((hold_time->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("hold_time :%d\r\n",hold_time->valueint);
		temp32 = hold_time->valueint;
		// check the parameter setting range
		if(temp32 < 3600000000)
		{
			csi_wifi_config.hold_time = temp32;
		}
		else
		{
			CSI_LOGI("[Error]hold_time :%d\r\n",temp32);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "hold_time");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg hold_time\n");
	}
	//calibration_cnt
	cJSON *calibration_cnt = cJSON_GetObjectItem(json, "calibration_cnt");
	if (calibration_cnt && ((calibration_cnt->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("calibration_cnt :%d\r\n",calibration_cnt->valueint);
		temp32 = calibration_cnt->valueint;
		// check the parameter setting range
		if(temp32 < 3600000)
		{
			csi_wifi_config.calibration_cnt = temp32;
		}
		else
		{
			CSI_LOGI("[Error]calibration_cnt :%d\r\n",temp32);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "calibration_cnt");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg calibration_cnt\n");
	}
	//cali_mode
	cJSON *cali_mode = cJSON_GetObjectItem(json, "cali_mode");
	if (cali_mode && ((cali_mode->type & 0xFF) == cJSON_Number))
	{
		CSI_LOGI("calibration_cnt :%d\r\n",cali_mode->valueint);
		temp8 = cali_mode->valueint;
		// check the parameter setting range
		if(temp8 < 4)
		{
			csi_wifi_config.cali_mode = temp8;
			if(csi_wifi_config.cali_mode == 1)
			{
				csi_wifi_alg_param.state_static = STATIC_EMPTY;
				CSI_LOGI("clear change flag\r\n");
			}
		}
		else
		{
			CSI_LOGI("[Error]calibration_cnt :%d\r\n",temp8);
			os_snprintf(err_code, CSI_ERR_CODE_LEN, "calibration_cnt");
			goto csi_start_fail;
		}
	}
	else
	{
		CSI_LOGI("[Warning] not find msg cali_mode\n");
	}

	cJSON_Delete(json);

	// send resp OK to APP by ble
	if((code_int != 1)&&(csi_wifi_config.cali_mode == 1))
	{
		g_demo_is_delay_resp = true;
		g_demo_is_delay_cnt = 0;
	}
	else
	{
		g_demo_is_delay_resp = false;
		csi_resp_send_to_app(0,err_code);
	}

	// csi start config
	if(1 == code_int)
	{
		g_demo_is_app_ctrl = true;
		// start STA
		if(csi_wifi_config.csi_work_identity == 1)
		{
			if(ssid_is_valid)
			{
				// connnect router
				csi_wifi_sta_connect(ssid_ptr,password_ptr);

				struct wifi_csi_start_dbg debug_param;
				debug_param.tx_type = csi_wifi_config.tx_type;
				debug_param.rx_mode = csi_wifi_config.rx_mode;
				debug_param.gap_num = csi_wifi_config.gap_num;
				debug_param.gap = csi_wifi_config.gap;
				debug_param.data_cnt = csi_wifi_config.data_cnt;
				debug_param.filter_mac_num = csi_wifi_config.filter_mac_num;
				debug_param.mac = csi_wifi_config.mac;
				// start csi
				// open interface :bk_wifi_csi_start_req
				// internal interface with debugging function
				
				bk_wifi_csi_start_req_internal(csi_wifi_config.csi_work_type,csi_wifi_config.mode,csi_wifi_config.csi_work_identity,csi_wifi_config.format,
						csi_wifi_config.interval,csi_wifi_config.delay,debug_param);
			}
			else
				CSI_LOGI("[Error] not find ssid\n");
		}
		// start AP
		else if(csi_wifi_config.csi_work_identity == 2)
		{
			if(ssid_is_valid)
			{
				// start sap
				csi_wifi_ap_start(ssid_ptr,password_ptr);

				struct wifi_csi_start_dbg debug_param;
				debug_param.tx_type = csi_wifi_config.tx_type;
				debug_param.rx_mode = csi_wifi_config.rx_mode;
				debug_param.gap_num = csi_wifi_config.gap_num;
				debug_param.gap = csi_wifi_config.gap;
				debug_param.data_cnt = csi_wifi_config.data_cnt;
				debug_param.filter_mac_num = csi_wifi_config.filter_mac_num;
				debug_param.mac = csi_wifi_config.mac;
				// start csi
				// open interface :bk_wifi_csi_start_req
				// internal interface with debugging function
				bk_wifi_csi_start_req_internal(csi_wifi_config.csi_work_type,csi_wifi_config.mode,csi_wifi_config.csi_work_identity,csi_wifi_config.format,
						csi_wifi_config.interval,csi_wifi_config.delay,debug_param);
			}
			else
				CSI_LOGI("[Error] not find ssid\n");
		}
	}

	// csi alg param config
	struct wifi_csi_cfg_dbg debug_cfg_param;
	debug_cfg_param.rate1 = csi_wifi_config.rate1;
	debug_cfg_param.rate2 = csi_wifi_config.rate2;
	debug_cfg_param.rate3 = csi_wifi_config.rate3;
	// thres2 must small than thres1
	if(csi_wifi_config.thres2 >= csi_wifi_config.thres1)
		csi_wifi_config.thres2 = csi_wifi_config.thres1 - 0.1;
	debug_cfg_param.thres2 = csi_wifi_config.thres2;
	// config algorithm parameters
	// open interface :bk_wifi_csi_alg_config
	// internal interface with debugging function
	bk_wifi_csi_alg_config_internal(csi_wifi_config.thres1,debug_cfg_param);

	// set calibration start
	if(csi_wifi_config.calibration_cnt != 0)
		bk_wifi_csi_static_param_reset_req(csi_wifi_config.cali_mode,csi_wifi_config.calibration_cnt);

	return 0;

csi_start_fail:
	// resp fail
	csi_resp_send_to_app(1,err_code);
	return -1;
}

/**
 ****************************************************************************************
 * @brief csi demo algorithm result handle
 *
 * @param[in] result_thres1     Dynamic Check Ratio
 * @param[in] result_thres2     State Change Ratio
 ****************************************************************************************
 */
void csi_wifi_alg_result_handle(double result_thres1,double result_thres2)
{
	if(g_demo_is_delay_resp)
	{
		g_demo_is_delay_cnt++;
		if(g_demo_is_delay_cnt > csi_wifi_config.calibration_cnt)
		{
			g_demo_is_delay_resp = false;
			g_demo_is_delay_cnt = 0;
			csi_resp_send_to_app(0,NULL);
		}
	}
	if(csi_wifi_alg_param.state == STATE_NO_MOTION)
	{
		csi_wifi_alg_param.empty_cnt++;
		// calibration start
		// only config static_update and calibration_cnt, it will auto calibration
		if((csi_wifi_config.static_update > 0)&&(csi_wifi_config.calibration_cnt > 0))
		{
			if(csi_wifi_config.static_update > csi_wifi_config.calibration_cnt)
			{
				if((g_static_cali_ctrl == 0)&&
					(csi_wifi_alg_param.empty_cnt >= (csi_wifi_config.static_update - csi_wifi_config.calibration_cnt)))
				{
					// start calibration but not update
					bk_wifi_csi_static_param_reset_req(2,csi_wifi_config.calibration_cnt);
					g_static_cali_ctrl = 1;
				}
				else
				{
					if(csi_wifi_alg_param.empty_cnt >= csi_wifi_config.static_update)
					{
						// update calibration result
						bk_wifi_csi_static_param_reset_req(3,csi_wifi_config.calibration_cnt);
						csi_wifi_alg_param.state_static = STATIC_EMPTY;
						CSI_LOGI("clear change flag\r\n");
						csi_wifi_alg_param.empty_cnt = 0;
						g_static_cali_ctrl = 0;
					}
				}
			}
			else
			{
				if(g_static_cali_ctrl == 0)
				{
					// start calibration but not update
					bk_wifi_csi_static_param_reset_req(2,csi_wifi_config.static_update);
					g_static_cali_ctrl = 1;
				}
				else
				{
					if(csi_wifi_alg_param.empty_cnt >= csi_wifi_config.static_update)
					{
						// update calibration result
						bk_wifi_csi_static_param_reset_req(3,csi_wifi_config.static_update);
						csi_wifi_alg_param.state_static = STATIC_EMPTY;
						CSI_LOGI("clear change flag\r\n");
						csi_wifi_alg_param.empty_cnt = 0;
						g_static_cali_ctrl = 0;
					}
				}
			}
		}
		// calibration end

		// check exercise
		if(result_thres1 > csi_wifi_config.thres1)
		{
			csi_wifi_alg_param.motion_cnt++;
			// continuous detection of motion is considered motion
			if(csi_wifi_alg_param.motion_cnt >= 4)
			{
				csi_wifi_alg_param.empty_cnt = 0;
				csi_wifi_alg_param.state1_num = 0;
				csi_wifi_alg_param.state2_num = 0;
				csi_wifi_alg_param.state = STATE_MOTION;
				g_static_cali_ctrl = 0;
			}
		}
		else
		{// clear motion count
			
			csi_wifi_alg_param.motion_cnt = 0;
		}
	}
	else if(csi_wifi_alg_param.state >= STATE_MOTION)
	{
		csi_wifi_alg_param.motion_cnt = 0;
		csi_wifi_alg_param.empty_cnt++;
		// if discovering exercise midway clear empty cnt
		if(result_thres1 > csi_wifi_config.thres1)
		{
			csi_wifi_alg_param.empty_cnt = 0;
			csi_wifi_alg_param.state1_num = 0;
			csi_wifi_alg_param.state2_num = 0;
			csi_wifi_alg_param.state = STATE_MOTION;
		}
		else
		{
			uint32_t check_num = 0;
			// already calibrated
			if(result_thres2 != 0)
			{
				if(csi_wifi_config.hold_time > STATIC_DETECTION)
					check_num = csi_wifi_config.hold_time - STATIC_DETECTION;

				if(csi_wifi_alg_param.empty_cnt >= check_num)
				{
					csi_wifi_alg_param.state1_num++;
					// check if static change
					if(result_thres2 > csi_wifi_config.thres3)
					{
						csi_wifi_alg_param.state2_num++;
					}
					if(csi_wifi_alg_param.state1_num >= STATIC_DETECTION)
					{
						if(csi_wifi_alg_param.state2_num >= STATIC_DETECTION_THRESHOLD)
							csi_wifi_alg_param.state_static = STATIC_CHANGE;
						else
							csi_wifi_alg_param.state_static = STATIC_EMPTY;

						csi_wifi_alg_param.empty_cnt = 0;
						csi_wifi_alg_param.state1_num = 0;
						csi_wifi_alg_param.state2_num = 0;
						csi_wifi_alg_param.state = STATE_NO_MOTION;
					}
				}
			}
			else
			{
				if(csi_wifi_config.hold_time > 0)
					check_num = csi_wifi_config.hold_time;
				else
					check_num = 4;//continuous detection of empty is considered empty

				// continuous detection of empty is considered empty or check times more than hold times
				if(csi_wifi_alg_param.empty_cnt >= check_num)
				{
					csi_wifi_alg_param.empty_cnt = 0;
					csi_wifi_alg_param.state1_num = 0;
					csi_wifi_alg_param.state2_num = 0;
					csi_wifi_alg_param.state = STATE_NO_MOTION;
				}
			}
		}
	}

	// print state, change light, and send result to app
	if((csi_wifi_alg_param.state == STATE_NO_MOTION)&&((csi_wifi_alg_param.state_static == STATIC_EMPTY)||(result_thres2 == 0)))
	{
		if(csi_wifi_alg_param.state_keep != STATE_OUT_EMPTY)
		{
#if BK_CSI_DEMO_DEBUG
			CSI_LOGI("state %d to 1, keep cnt = %d. \r\n",csi_wifi_alg_param.state_keep,csi_wifi_alg_param.skeep_cnt);
			csi_wifi_alg_param.skeep_cnt = 0;
#endif //BK_CSI_DEMO_DEBUG
			CSI_LOGI("-------------empty \r\n");
			// change light to green
			bk_wifi_csi_demo_turn_on_light(3,false);
			csi_wifi_alg_param.state_keep = STATE_OUT_EMPTY;

			// if app config csi,send csi check result to app
			if(g_demo_is_app_ctrl)
				csi_ind_send_to_app(0,0);
		}
#if BK_CSI_DEMO_DEBUG
		else
		{
			//BK_LOG_RAW("=");
			csi_wifi_alg_param.skeep_cnt++;
			if(csi_wifi_alg_param.skeep_cnt%100 == 0)
			{
				BK_LOG_RAW("= %f, %f",result_thres1,result_thres2);
				if(g_demo_is_app_ctrl)
					csi_ind_send_to_app(0,0);
			}
		}
#endif //BK_CSI_DEMO_DEBUG
	}
	else if((csi_wifi_alg_param.state == STATE_NO_MOTION)&&(csi_wifi_alg_param.state_static == STATIC_CHANGE))
	{
		if(csi_wifi_alg_param.state_keep != STATE_OUT_STATIC_CHANGE)
		{
#if BK_CSI_DEMO_DEBUG
			CSI_LOGI("state %d to 3, keep cnt = %d. \r\n",csi_wifi_alg_param.state_keep,csi_wifi_alg_param.skeep_cnt);
			csi_wifi_alg_param.skeep_cnt = 0;
#endif //BK_CSI_DEMO_DEBUG
			CSI_LOGI("-------------change \r\n");
			// change light to blue
			bk_wifi_csi_demo_turn_on_light(2,false);
			csi_wifi_alg_param.state_keep = STATE_OUT_STATIC_CHANGE;

			// if app config csi,send csi check result to app
			if(g_demo_is_app_ctrl)
				csi_ind_send_to_app(0,2);
		}
#if BK_CSI_DEMO_DEBUG
		else
		{
			//BK_LOG_RAW("%");
			csi_wifi_alg_param.skeep_cnt++;
			if(csi_wifi_alg_param.skeep_cnt%100 == 0)
			{
				BK_LOG_RAW("# %f, %f",result_thres1,result_thres2);
				if(g_demo_is_app_ctrl)
					csi_ind_send_to_app(0,2);
			}
		}
#endif //BK_CSI_DEMO_DEBUG
	}
	else if(csi_wifi_alg_param.state >= STATE_MOTION)
	{
		if(csi_wifi_alg_param.state_keep != STATE_OUT_MOTION)
		{
#if BK_CSI_DEMO_DEBUG
			CSI_LOGI("state %d to 2, keep cnt = %d. \r\n",csi_wifi_alg_param.state_keep,csi_wifi_alg_param.skeep_cnt);
			csi_wifi_alg_param.skeep_cnt = 0;
#endif //BK_CSI_DEMO_DEBUG
			CSI_LOGI("-------------man \r\n");
			// change light to pink
			bk_wifi_csi_demo_turn_on_light(5,false);
			csi_wifi_alg_param.state_keep = STATE_OUT_MOTION;

			// if app config csi,send csi check result to app
			if(g_demo_is_app_ctrl)
				csi_ind_send_to_app(0,1);
		}
#if BK_CSI_DEMO_DEBUG
		else
		{
			//BK_LOG_RAW("&");
			csi_wifi_alg_param.skeep_cnt++;
			if(csi_wifi_alg_param.skeep_cnt%100 == 0)
			{
				BK_LOG_RAW("& %f, %f",result_thres1,result_thres2);
				if(g_demo_is_app_ctrl)
					csi_ind_send_to_app(0,1);
			}
		}
#endif //BK_CSI_DEMO_DEBUG
	}
}

/**
 ****************************************************************************************
 * @brief csi demo algorithm param set
 *
 * @param[in] thres1     Dynamic Check Ratio
 * @param[in] thres3     State Change Ratio
 * @param[in] hold_time     Dynamic state keep count
 ****************************************************************************************
 */
void csi_wifi_demo_param_set(double thres1,double thres3,uint32_t static_update,uint32_t hold_time,uint32_t calibration_cnt)
{
	csi_wifi_config.thres1 = thres1;
	csi_wifi_config.thres3 = thres3;
	csi_wifi_config.static_update = static_update;
	csi_wifi_config.hold_time = hold_time;
	csi_wifi_config.calibration_cnt = calibration_cnt;
	CSI_LOGI("thres1 %f, thres3 %f ,csi_wifi_alg_hold_time %d.\r\n",thres1,thres3,hold_time);
}

#endif //CONFIG_WIFI_CSI_DEMO
#endif //CONFIG_SYS_CPU0
int main(void)
{
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
	#if CONFIG_WIFI_CSI_DEMO
	// init light demo
	bk_csi_demo_main();
	// init ble
	bk_wifi_csi_ble_provisioning_init();
	#endif
	// register event cb
	csi_wifi_event_handler_init();
	#endif

	return 0;
}
