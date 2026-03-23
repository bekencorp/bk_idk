#include "https_ota.h"
#include "cli.h"
#include "modules/bk_ota.h"
#include "driver/flash.h"
#include "driver/flash_partition.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#if CONFIG_OTA_UPDATE_PUBKEY
#if CONFIG_PSA_MBEDTLS
#include <mbedtls/sha256.h>
#endif
#endif

#define TAG "HTTPS_OTA_TEST"

static beken_thread_t ota_thread_handle = NULL;
// static ota_download_t* s_ota_handle = NULL;
https_ota_t* s_ota_config = NULL;
char *https_url = NULL;

/* this crt for url https://docs.bekencorp.com , support test*/
/* CA cert chain from bekencorp.com.pem (DigiCert: leaf + EE DV TLS CA - G2 + root) */
const char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"MIIGBTCCBO2gAwIBAgIQAsqntZ25WVsTdidRS+2NGjANBgkqhkiG9w0BAQsFADBu\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMS0wKwYDVQQDEyRFbmNyeXB0aW9uIEV2ZXJ5d2hlcmUg\r\n"
"RFYgVExTIENBIC0gRzIwHhcNMjYwMjI3MDAwMDAwWhcNMjYwODI4MjM1OTU5WjAa\r\n"
"MRgwFgYDVQQDDA8qLmJla2VuY29ycC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IB\r\n"
"DwAwggEKAoIBAQC2p4/5I10rf3hlm8ODDD63l11uHUDUjbg3+OonbZyKO2M1v4un\r\n"
"uzKuJG/scMyG539sZzgd+zBbWLYhY/LE2VmSERL8qD/8Gk8olczoBV3jt0yp1ejO\r\n"
"HSoyiNHcJDi/q3NfNTiyLUuxrEdWkrhX1A2a2d3UFH1ZZJxT46cvDldWgN6Cgp8Z\r\n"
"rwHaaT+47l9JnqZwNgWqZL4Fu2i6bi2VXGnB+mghsv1J84+jMj2x8Bs6w1dzc+o+\r\n"
"iLxUntTkELz++MdsMo0lqazVcMfcP3V67d+lgSBAl+o/lY2dkrKAzMbF3WIUG21b\r\n"
"OgHGJjYb+ZZ3ETpUe/dhcEMG+T/W/nbctEV3AgMBAAGjggLxMIIC7TAfBgNVHSME\r\n"
"GDAWgBR435GQX+7erPbFdevVTFVT7yRKtjAdBgNVHQ4EFgQU2XUdTK+JaPeB8RaQ\r\n"
"CrUV0FcA5lMwKQYDVR0RBCIwIIIPKi5iZWtlbmNvcnAuY29tgg1iZWtlbmNvcnAu\r\n"
"Y29tMD4GA1UdIAQ3MDUwMwYGZ4EMAQIBMCkwJwYIKwYBBQUHAgEWG2h0dHA6Ly93\r\n"
"d3cuZGlnaWNlcnQuY29tL0NQUzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYI\r\n"
"KwYBBQUHAwEGCCsGAQUFBwMCMIGABggrBgEFBQcBAQR0MHIwJAYIKwYBBQUHMAGG\r\n"
"GGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBKBggrBgEFBQcwAoY+aHR0cDovL2Nh\r\n"
"Y2VydHMuZGlnaWNlcnQuY29tL0VuY3J5cHRpb25FdmVyeXdoZXJlRFZUTFNDQS1H\r\n"
"Mi5jcnQwDAYDVR0TAQH/BAIwADCCAX4GCisGAQQB1nkCBAIEggFuBIIBagFoAHUA\r\n"
"wjF+V0UZo0XufzjespBB68fCIVoiv3/Vta12mtkOUs0AAAGcnSWS3AAABAMARjBE\r\n"
"AiAIkAZsG62e3UmSsXVdd4WB7KmTeFj8yClww8D+PofqvgIgR6mR7qyNkjHelmoC\r\n"
"r9T/Uz+iOhdbswNielmDW40kwqMAdwDXbX0Q0af1d8LH6V/XAL/5gskzWmXh0LMB\r\n"
"cxfAyMVpdwAAAZydJZLkAAAEAwBIMEYCIQCmN2JyB26N0MbVvXZ8XUM6WYFdftLl\r\n"
"ng1SoDJzQyxSuwIhAOEM7VoyCy5WCCs7YtqL1daehCc3pP6rhcLEmcqr/uvqAHYA\r\n"
"lE5Dh/rswe+B8xkkJqgYZQHH0184AgE/cmd9VTcuGdgAAAGcnSWS7gAABAMARzBF\r\n"
"AiEA+EQrmc7m7zmZGhNRj0wY59GpSINoywLl+jFezOPD+5ACIHQXwZFPf5mcgf+L\r\n"
"9zIcSR/vvDjYmUWqH23FJ7AIITtjMA0GCSqGSIb3DQEBCwUAA4IBAQDcIEmp+BR+\r\n"
"vA3kYBgProQtAjVJgpdrrVgNgV8Vthn13cFwdS9WZO/F0Q9kdGiDYL1J0OSzs39K\r\n"
"FVvC0nbKR65x1uczMInIPRv9P81JrhV+RAOGA8OgFsJiyrI3QV0BgA2EAvhiW98Q\r\n"
"079UviXgc1a9Lp6pvnFKxCkULDN9/vvjY8oQsGLYYrOoVolYFr7rFkyqLp/CO9TL\r\n"
"5CRCD+Ex3A2pUR7nuqwNXM3VJPI+CWSd8ecEQCYlAvjcXx42fJtOyooAhygHt4hR\r\n"
"80/cXtU/hpV9JSGtvDfIvwNjoI7CfgSf9Km3h6BbCnWxPesA9Ult7OUi/JT2PHWk\r\n"
"vRSQZ00pyMup\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN CERTIFICATE-----\r\n"
"MIIEqjCCA5KgAwIBAgIQDeD/te5iy2EQn2CMnO1e0zANBgkqhkiG9w0BAQsFADBh\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"
"MjAeFw0xNzExMjcxMjQ2NDBaFw0yNzExMjcxMjQ2NDBaMG4xCzAJBgNVBAYTAlVT\r\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
"b20xLTArBgNVBAMTJEVuY3J5cHRpb24gRXZlcnl3aGVyZSBEViBUTFMgQ0EgLSBH\r\n"
"MjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAO8Uf46i/nr7pkgTDqnE\r\n"
"eSIfCFqvPnUq3aF1tMJ5hh9MnO6Lmt5UdHfBGwC9Si+XjK12cjZgxObsL6Rg1njv\r\n"
"NhAMJ4JunN0JGGRJGSevbJsA3sc68nbPQzuKp5Jc8vpryp2mts38pSCXorPR+sch\r\n"
"QisKA7OSQ1MjcFN0d7tbrceWFNbzgL2csJVQeogOBGSe/KZEIZw6gXLKeFe7mupn\r\n"
"NYJROi2iC11+HuF79iAttMc32Cv6UOxixY/3ZV+LzpLnklFq98XORgwkIJL1HuvP\r\n"
"ha8yvb+W6JislZJL+HLFtidoxmI7Qm3ZyIV66W533DsGFimFJkz3y0GeHWuSVMbI\r\n"
"lfsCAwEAAaOCAU8wggFLMB0GA1UdDgQWBBR435GQX+7erPbFdevVTFVT7yRKtjAf\r\n"
"BgNVHSMEGDAWgBROIlQgGJXm427mD/r6uRLtBhePOTAOBgNVHQ8BAf8EBAMCAYYw\r\n"
"HQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8C\r\n"
"AQAwNAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdp\r\n"
"Y2VydC5jb20wQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQu\r\n"
"Y29tL0RpZ2lDZXJ0R2xvYmFsUm9vdEcyLmNybDBMBgNVHSAERTBDMDcGCWCGSAGG\r\n"
"/WwBAjAqMCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BT\r\n"
"MAgGBmeBDAECATANBgkqhkiG9w0BAQsFAAOCAQEAoBs1eCLKakLtVRPFRjBIJ9LJ\r\n"
"L0s8ZWum8U8/1TMVkQMBn+CPb5xnCD0GSA6L/V0ZFrMNqBirrr5B241OesECvxIi\r\n"
"98bZ90h9+q/X5eMyOD35f8YTaEMpdnQCnawIwiHx06/0BfiTj+b/XQih+mqt3ZXe\r\n"
"xNCJqKexdiB2IWGSKcgahPacWkk/BAQFisKIFYEqHzV974S3FAz/8LIfD58xnsEN\r\n"
"GfzyIDkH3JrwYZ8caPTf6ZX9M1GrISN8HnWTtdNCH2xEajRa/h9ZBXjUyFKQrGk2\r\n"
"n2hcLrfZSbynEC/pSw/ET7H5nWwckjmAJ1l9fcnbqkU/pf6uMQmnfl0JQjJNSg==\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN CERTIFICATE-----\r\n"
"MIIEfjCCA2agAwIBAgIQD+Ayq4RNAzEGxQyOE8iwaDANBgkqhkiG9w0BAQsFADBh\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
"QTAeFw0yNDAxMTgwMDAwMDBaFw0zMTExMDkyMzU5NTlaMGExCzAJBgNVBAYTAlVT\r\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo4IBMDCC\r\n"
"ASwwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUTiJUIBiV5uNu5g/6+rkS7QYX\r\n"
"jzkwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUwDgYDVR0PAQH/BAQD\r\n"
"AgGGMHQGCCsGAQUFBwEBBGgwZjAjBggrBgEFBQcwAYYXaHR0cDovL29jc3AuZGln\r\n"
"aWNlcnQuY24wPwYIKwYBBQUHMAKGM2h0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNu\r\n"
"L0RpZ2lDZXJ0R2xvYmFsUm9vdENBLmNydDBABgNVHR8EOTA3MDWgM6Axhi9odHRw\r\n"
"Oi8vY3JsLmRpZ2ljZXJ0LmNuL0RpZ2lDZXJ0R2xvYmFsUm9vdENBLmNybDARBgNV\r\n"
"HSAECjAIMAYGBFUdIAAwDQYJKoZIhvcNAQELBQADggEBAHRBl3jN7+XHBUK0dZnu\r\n"
"hMdoNwD1nCROU3BTIh1TNzRI0bQ0m5+C/dCRzzlqoSAFHUlOi+OiDltWkXTzmQn6\r\n"
"Z8bH5PFBy5sYpc/8cNPoSzhyqcpvvEZvv/Ivc0Up+dzma7vBDJC9WrMRUUlSFSQp\r\n"
"kdXSmphDNkXJsgARmxzc18IN6LYMRiOWlY7RE2F900pPW60BvJHHNCX0bbSRj/Ql\r\n"
"bmVq8wuftBD++D+RS8K++ujpMjFBROyWfBX+woQDGsMazkmgulQdnZrdj476elOL\r\n"
"axRvrSgEorju1kJM7M65z2RUZrfzQYW/1rs8mRUXin6iEtad/Rv1ZI1WGYmWPyBm\r\n"
"pbo=\r\n"
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
	BK_LOGI(TAG,"ota cancel current/ow/xip\r\n");
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
#if CONFIG_OTA_UPDATE_PUBKEY && CONFIG_OTA_OVERWRITE
		if (s_ota_config && s_ota_config->ota_parse && s_ota_config->ota_parse->ota_handle) {
			if (bk_ota_update_public_key(s_ota_config->ota_parse->ota_handle) != BK_OK) {
				BK_LOGE(TAG, "Back pubkey fail!\r\n");
				break;
			}
		}
#endif
#if CONFIG_OTA_CONFIRM_UPDATE
		if (s_ota_config && s_ota_config->ota_parse && s_ota_config->ota_parse->ota_handle) {
			if (bk_ota_confirm(s_ota_config->ota_parse->ota_handle) != BK_OK) {
				BK_LOGE(TAG,"set confirm fail!\r\n");
				break;
			}
		}
#endif
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
		BK_LOGI(TAG, "HTTPS_EVENT_ON_CONNECTED\r\n");
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
		BK_LOGI(TAG, "HTTPS_EVENT_ON_FINISH\r\n");
		break;
	case HTTP_EVENT_DISCONNECTED:
		BK_LOGI(TAG, "HTTPS_EVENT_DISCONNECTED\r\n");
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
	bool ota_ok = false;

	s_ota_config = generate_ota_config(https_url);
	bk_https_ota_init(s_ota_config);
	bk_https_ota_set_erase_method(s_ota_config, true);

	state = bk_https_ota_perform(s_ota_config);

	if (state == HTTPS_OTA_FINISH) {
		bk_err_t verify_ret = BK_FAIL;
		bk_err_t confirm_ret = BK_FAIL;
		if (s_ota_config && s_ota_config->ota_parse && s_ota_config->ota_parse->ota_handle) {
			verify_ret = bk_ota_verify(s_ota_config->ota_parse->ota_handle, s_ota_config->ota_parse->ota_verify);
			if (s_ota_config->ota_parse) {
				s_ota_config->ota_parse->ota_verify = NULL;
			}
			if (verify_ret == BK_OK) {
				confirm_ret = bk_ota_confirm(s_ota_config->ota_parse->ota_handle);
			} else {
				BK_LOGE(TAG, "HTTPS OTA verify fail, verify=0x%x\r\n", verify_ret);
			}
		}
		if (confirm_ret == BK_OK) {
			ota_ok = true;
			if (s_ota_config && s_ota_config->ota_event_handler) {
				https_ota_event_t success_evt = {
					.event_id = HTTPS_OTA_SUCCESS_EVENT,
					.data = NULL,
					.data_len = 0,
				};
				s_ota_config->ota_event_handler(success_evt);
			}
			BK_LOGI(TAG, "HTTPS OTA SUCCESS");
		} else {
			BK_LOGE(TAG, "HTTPS OTA confirm fail, confirm=0x%x\r\n", confirm_ret);
		}
	} else {
		BK_LOGE(TAG, "HTTPS OTA perform fail, state=%d\r\n", state);
	}

	if (!ota_ok) {
		BK_LOGE(TAG, "OTA FAIL\r\n");
	}

	bk_https_ota_deinit(s_ota_config);
	release_ota_config(s_ota_config);
	s_ota_config = NULL;
	ota_thread_handle = NULL;
	rtos_delete_thread(NULL);
}

void https_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 ret = 0;
	if (argc != 2 || argv[1] == NULL) {
		BK_LOGE(TAG, "Usage: https_ota <url>\r\n");
		return;
	}
	if (https_url != NULL) {
		free(https_url);
		https_url = NULL;
	}
	https_url = (char*)malloc(strlen(argv[1]) + 1);
	if (https_url == NULL) {
		BK_LOGE(TAG, "malloc https_url failed\r\n");
		return;
	}
	strcpy(https_url, argv[1]);
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
		if (argc < 3 || argv[2] == NULL) {
			BK_LOGE(TAG, "Usage: ota init <url>\r\n");
			return;
		}
		if (https_url != NULL) {
			free(https_url);
			https_url = NULL;
		}
		https_url = (char*)malloc(strlen(argv[2]) + 1);
		if (https_url == NULL) {
			BK_LOGE(TAG, "malloc https_url failed\r\n");
			return;
		}
		strcpy(https_url, argv[2]);
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
		if (argc < 3) {
			BK_LOGE(TAG, "Usage: ota confirm <current/ow/xip>\r\n");
			BK_LOGE(TAG, "  current - confirm current OTA upgrade\r\n");
			BK_LOGE(TAG, "  ow      - confirm OW mode upgrade\r\n");
			BK_LOGE(TAG, "  xip     - confirm XIP mode upgrade\r\n");
			return;
		}
		if (os_strcmp(argv[2], "ow") == 0) {
			bk_ota_t *ow_ota_handle = NULL;
			ow_ota_handle = bk_ota_get_ow_handle();
			if (ow_ota_handle != NULL) {
				bk_err_t ret = bk_ota_confirm(ow_ota_handle);
				if (ret == BK_OK) {
					BK_LOGI(TAG, "OTA confirm (ow) success\r\n");
				} else {
					BK_LOGE(TAG, "OTA confirm (ow) failed, ret=0x%x\r\n", ret);
				}
			} else {
				BK_LOGE(TAG, "Failed to get OW OTA handle\r\n");
			}
		} else if (os_strcmp(argv[2], "xip") == 0) {
			bk_ota_t *xip_ota_handle = NULL;
			xip_ota_handle = bk_ota_get_xip_handle();
			if (xip_ota_handle != NULL) {
				bk_err_t ret = bk_ota_confirm(xip_ota_handle);
				if (ret == BK_OK) {
					BK_LOGI(TAG, "OTA confirm (xip) success\r\n");
				} else {
					BK_LOGE(TAG, "OTA confirm (xip) failed, ret=0x%x\r\n", ret);
				}
			} else {
				BK_LOGE(TAG, "Failed to get XIP OTA handle\r\n");
			}
		} else if (os_strcmp(argv[2], "current") == 0) {
			if (s_ota_config != NULL && s_ota_config->ota_parse != NULL && s_ota_config->ota_parse->ota_handle != NULL) {
				BK_LOGI(TAG, "OTA confirm (current): using ota_handle from config\r\n");
				if (s_ota_config->ota_parse->ota_handle->control_partition != NULL) {
					BK_LOGI(TAG, "control_partition: %s, addr=0x%x\r\n", 
						s_ota_config->ota_parse->ota_handle->control_partition->partition_description ? 
						s_ota_config->ota_parse->ota_handle->control_partition->partition_description : "NULL",
						s_ota_config->ota_parse->ota_handle->control_partition->partition_start_addr);
				} else {
					BK_LOGE(TAG, "control_partition is NULL!\r\n");
				}
				bk_err_t ret = bk_ota_confirm(s_ota_config->ota_parse->ota_handle);
				if (ret == BK_OK) {
					BK_LOGI(TAG, "OTA confirm (current) success\r\n");
				} else {
					BK_LOGE(TAG, "OTA confirm (current) failed, ret=0x%x\r\n", ret);
					BK_LOGE(TAG, "Try: ota confirm xip (for DIRECT_XIP mode)\r\n");
				}
			} else {
				BK_LOGE(TAG, "config or ota_handle is NULL\r\n");
				BK_LOGE(TAG, "Try: ota confirm <ow/xip>\r\n");
			}
		} else {
			BK_LOGE(TAG, "Invalid confirm parameter: %s\r\n", argv[2]);
			BK_LOGE(TAG, "Usage: ota confirm <current/ow/xip>\r\n");
		}
	} else if (os_strcmp(argv[1], "cancel") == 0) {
		if (argc < 3) {
			BK_LOGE(TAG, "Usage: ota cancel <current/ow/xip>\r\n");
			BK_LOGE(TAG, "  current - cancel current OTA upgrade\r\n");
			BK_LOGE(TAG, "  ow      - cancel OW mode upgrade\r\n");
			BK_LOGE(TAG, "  xip     - cancel XIP mode upgrade\r\n");
			return;
		}
		if (os_strcmp(argv[2], "ow") == 0) {
			bk_ota_t *ow_ota_handle = NULL;
			ow_ota_handle = bk_ota_get_ow_handle();
			if (ow_ota_handle != NULL) {
				bk_err_t ret = bk_ota_cancel(ow_ota_handle);
				if (ret == BK_OK) {
					BK_LOGI(TAG, "OTA cancel (ow) success\r\n");
				} else {
					BK_LOGE(TAG, "OTA cancel (ow) failed, ret=0x%x\r\n", ret);
				}
			} else {
				BK_LOGE(TAG, "Failed to get OW OTA handle\r\n");
			}
		} else if (os_strcmp(argv[2], "xip") == 0) {
			bk_ota_t *xip_ota_handle = NULL;
			xip_ota_handle = bk_ota_get_xip_handle();
			if (xip_ota_handle != NULL) {
				bk_err_t ret = bk_ota_cancel(xip_ota_handle);
				if (ret == BK_OK) {
					BK_LOGI(TAG, "OTA cancel (xip) success\r\n");
				} else {
					BK_LOGE(TAG, "OTA cancel (xip) failed, ret=0x%x\r\n", ret);
				}
			} else {
				BK_LOGE(TAG, "Failed to get XIP OTA handle\r\n");
			}
		} else if (os_strcmp(argv[2], "current") == 0) {
			if (s_ota_config != NULL && s_ota_config->ota_parse != NULL && s_ota_config->ota_parse->ota_handle != NULL) {
				BK_LOGI(TAG, "OTA cancel (current): using ota_handle from config\r\n");
				if (s_ota_config->ota_parse->ota_handle->control_partition != NULL) {
					BK_LOGI(TAG, "control_partition: %s, addr=0x%x\r\n", 
						s_ota_config->ota_parse->ota_handle->control_partition->partition_description ? 
						s_ota_config->ota_parse->ota_handle->control_partition->partition_description : "NULL",
						s_ota_config->ota_parse->ota_handle->control_partition->partition_start_addr);
				} else {
					BK_LOGE(TAG, "control_partition is NULL!\r\n");
				}
				bk_err_t ret = bk_ota_cancel(s_ota_config->ota_parse->ota_handle);
				if (ret == BK_OK) {
					BK_LOGI(TAG, "OTA cancel (current) success\r\n");
				} else {
					BK_LOGE(TAG, "OTA cancel (current) failed, ret=0x%x\r\n", ret);
					BK_LOGE(TAG, "Try: ota cancel xip (for DIRECT_XIP mode)\r\n");
				}
			} else {
				BK_LOGE(TAG, "config or ota_handle is NULL\r\n");
				BK_LOGE(TAG, "Try: ota cancel <ow/xip>\r\n");
			}
		} else {
			BK_LOGE(TAG, "Invalid cancel parameter: %s\r\n", argv[2]);
			BK_LOGE(TAG, "Usage: ota cancel <current/ow/xip>\r\n");
		}
	} else if (os_strcmp(argv[1], "verify") == 0) {
		if (s_ota_config != NULL) {
			if (bk_ota_verify(s_ota_config->ota_parse->ota_handle, 0) != BK_OK) {
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
