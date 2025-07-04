#include "https_ota.h"
#include "cli.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#define TAG "HTTPS_OTA_TEST"

static beken_thread_t ota_thread_handle = NULL;
// static ota_download_t* s_ota_handle = NULL;
https_ota_t* s_ota_config = NULL;
char *https_url = NULL;

/* this crt for url https://docs.bekencorp.com , support test*/
const char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"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\r\n"
"clib3BcxKZX9qK4A6FAATghuSQ==\r\n"
"-----END CERTIFICATE-----\r\n"
};

static void cli_ota_help(void)
{
	BK_LOGI(TAG,"Two example:\n");
	BK_LOGI(TAG,"https_ota + url\n");
	BK_LOGI(TAG,"OR\n");
	BK_LOGI(TAG,"ota init + url\n");
	BK_LOGI(TAG,"ota begin/download/pause/end/deinit/verify\n");
	BK_LOGI(TAG,"ota confirm current/ow/xip\r\n");
	BK_LOGI(TAG,"ota entire_erase 1/0\r\n");
}

void ota_event_cb(https_ota_event_t evt)
{
	switch (evt.event_id) {
	case HTTPS_OTA_START_EVENT:
		break;
	case HTTPS_OTA_ERROR_EVENT:
		BK_LOGE(TAG,"OTA ERROR CODE %d\r\n",*(int*)(evt.data));
		break;
	case HTTPS_OTA_SUCCESS_EVENT:
		BK_LOGI(TAG,"OTA SUCCESS\r\n");
		break;	
	case HTTPS_OTA_RESTART_EVENT:
		break;
	case HTTPS_OTA_PAUSE_EVENT:
		break;
	}
}

bk_err_t https_event_cb(bk_http_client_event_t *evt)
{
	switch (evt->event_id) {
	case HTTP_EVENT_ERROR:
		BK_LOGE(TAG, "HTTPS_EVENT_ERROR\r\n");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		BK_LOGD(TAG, "HTTPS_EVENT_ON_CONNECTED\r\n");
		break;
	case HTTP_EVENT_HEADER_SENT:
		BK_LOGD(TAG, "HTTPS_EVENT_HEADER_SENT\r\n");
		break;
	case HTTP_EVENT_ON_HEADER:
		BK_LOGD(TAG, "HTTPS_EVENT_ON_HEADER\r\n");
		break;
	case HTTP_EVENT_ON_DATA:
		BK_LOGD(TAG, "HTTP_EVENT_ON_DATA, length:%d\r\n", evt->data_len);
		break;
	case HTTP_EVENT_ON_FINISH:
		BK_LOGD(TAG, "HTTPS_EVENT_ON_FINISH\r\n");
		break;
	case HTTP_EVENT_DISCONNECTED:
		BK_LOGD(TAG, "HTTPS_EVENT_DISCONNECTED\r\n");
		break;

	}
	return BK_OK;
}

https_ota_t* generate_ota_config(char * https_url)
{
	https_ota_t* ota_config = (https_ota_t*)os_malloc(sizeof(https_ota_t));
	if (ota_config == NULL) {
		return NULL;
	}
	os_memset(ota_config, 0, sizeof(https_ota_t));
	ota_config->http_config = (bk_http_input_t*)os_malloc(sizeof(bk_http_input_t));
	if (ota_config->http_config == NULL) {
		os_free(ota_config);
		return NULL;
	}
	os_memset(ota_config->http_config, 0, sizeof(bk_http_input_t));
	

	ota_config->http_config ->url = https_url;
	ota_config->http_config ->cert_pem = ca_crt_rsa;
	ota_config->http_config ->event_handler = https_event_cb;
	ota_config->http_config ->buffer_size = 4096;
	ota_config->http_config ->user_agent = "TEST HTTP Client/1.1";
	ota_config->http_config ->timeout_ms = 15000;
	ota_config->http_config ->keep_alive_enable = true,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
	ota_config->http_config ->crt_bundle_attach = esp_crt_bundle_attach,
#endif
	ota_config->ota_download_resume = true;
	ota_config->ota_event_handler = ota_event_cb;
	return ota_config;
}

void release_ota_config(https_ota_t* ota_config)
{
	if (ota_config->http_config != NULL) {
		os_free(ota_config->http_config);
	}
	if (ota_config != NULL) {
		os_free(ota_config);
	}
}

void bk_https_start_download(beken_thread_arg_t arg) 
{
	https_ota_state_t state;
	s_ota_config = generate_ota_config(https_url);
	bk_https_ota_init(s_ota_config);
	bk_https_ota_set_erase_method(s_ota_config, true);
	state = bk_https_ota_perform(s_ota_config);
	if (state == HTTPS_OTA_FINISH || state == HTTPS_OTA_FAIL) {
		bk_https_ota_deinit(s_ota_config);
		release_ota_config(s_ota_config);
		s_ota_config = NULL;
	}
	ota_thread_handle = NULL;
	rtos_delete_thread(NULL);
}

void https_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 ret = 0;
	if (https_url != NULL) {
		free(https_url);
		https_url = NULL;
		https_url = (char*)malloc(strlen(argv[1]));
		strcpy(https_url, argv[1]);
	} else {
		https_url = (char*)malloc(strlen(argv[1]));
		strcpy(https_url, argv[1]);
	}
	BK_LOGI(TAG, "https_ota_start\r\n");
	if (ota_thread_handle == NULL) {
		ret = rtos_create_thread(&ota_thread_handle, 4,
								"https_ota",
								(beken_thread_function_t)bk_https_start_download,
								5120,
								0);
	}
	if (kNoErr != ret)
		BK_LOGE(TAG, "https_ota_start failed\r\n");
}

void ota_steps_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "init") == 0) {
		if (https_url != NULL) {
			free(https_url);
			https_url = NULL;
			https_url = (char*)malloc(strlen(argv[2]));
			strcpy(https_url, argv[2]);
		} else {
			https_url = (char*)malloc(strlen(argv[2]));
			strcpy(https_url, argv[2]);
		}
		s_ota_config = generate_ota_config(https_url);
		bk_https_ota_init(s_ota_config);
	} else if (os_strcmp(argv[1], "begin") == 0) {
		bk_https_ota_begin(s_ota_config);
	} else if (os_strcmp(argv[1], "download") == 0) {
		bk_https_ota_progress_data(s_ota_config);
	} else if (os_strcmp(argv[1], "pause") == 0) {
		bk_https_ota_pause(s_ota_config);
		BK_LOGI(TAG, "OTA PAUSED\r\n");
	} else if (os_strcmp(argv[1], "end") == 0) {
		bk_https_ota_end(s_ota_config);
	} else if (os_strcmp(argv[1], "confirm") == 0) {
		if (os_strcmp(argv[2], "ow") == 0) {
			bk_ota_t *ow_ota_handle = NULL;
			ow_ota_handle = bk_ota_get_ow_handle();
			bk_ota_confirm(ow_ota_handle);
		} else if (os_strcmp(argv[2], "xip") == 0) {
			bk_ota_t *wifi_ota_handle = NULL;
			wifi_ota_handle = bk_ota_get_xip_handle();
			bk_ota_confirm(wifi_ota_handle);
		} else if (os_strcmp(argv[2], "current") == 0) {
			if (s_ota_config != NULL) {
				bk_ota_confirm(s_ota_config->ota_parse->ota_handle);
			} else {
				BK_LOGE(TAG,"config has been released,try other confirm commands!\r\n");
			}
		}
	} else if (os_strcmp(argv[1], "verify") == 0) {
		if (s_ota_config != NULL) {
			if (bk_ota_verify(s_ota_config->ota_parse->ota_handle) != BK_OK) {
				BK_LOGE(TAG,"ota partition verify error!\r\n");
			} else {
				BK_LOGI(TAG,"verify pass\r\n");
			}
		} else {
			BK_LOGE(TAG,"config has been released,try other verify commands!\r\n");
		}
	} else if (os_strcmp(argv[1], "entire_erase") == 0) {
		bool erase = os_strtoul(argv[2], NULL, 10);
		bk_https_ota_set_erase_method(s_ota_config, erase);
		BK_LOGI(TAG,"set erase method:%d\r\n",s_ota_config->ota_parse->entire_flash_erase);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		bk_https_ota_deinit(s_ota_config);
		release_ota_config(s_ota_config);
		s_ota_config = NULL;
#if CONFIG_OTA_UPDATE_PUBKEY
	} else if (os_strcmp(argv[1], "update_pubkey") == 0) {
		bk_ota_t *ow_ota_handle = NULL;
		ow_ota_handle = bk_ota_get_ow_handle();
		bk_ota_update_public_key(ow_ota_handle);
#endif
	} else {
		cli_ota_help();
	}
}

#define OTA_CMD_CNT (sizeof(s_ota_commands) / sizeof(struct cli_command))
static const struct cli_command s_ota_commands[] = {
	{"https_ota","",https_ota_Command},
	{"ota","",ota_steps_Command},
};

int cli_https_ota_init(void)
{
	return cli_register_commands(s_ota_commands, OTA_CMD_CNT);
}