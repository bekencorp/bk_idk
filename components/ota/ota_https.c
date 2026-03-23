#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include "components/system.h"
#include "components/log.h"
#include "driver/flash.h"
#include "modules/ota.h"
#include "utils_httpc.h"
#include "modules/wifi.h"
#include "bk_https.h"

#if CONFIG_PSA_MBEDTLS
#include "psa/crypto.h"
#endif


#define TAG "HTTPS_OTA"

extern UINT8 ota_flag;
#ifdef CONFIG_HTTP_AB_PARTITION
extern part_flag update_part_flag;
#endif

#define HTTPS_INPUT_SIZE   (5120)

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
	
bk_http_client_handle_t bk_https_client_flash_init(bk_http_input_t config)
{
	bk_http_client_handle_t client = NULL;

	ota_flag = 1;
#if CONFIG_SYSTEM_CTRL
	bk_wifi_ota_dtim(1);
#endif
#if HTTP_WR_TO_FLASH
	if (!bk_http_ptr) {
		BK_LOGE(TAG, "[OTA_HTTPS] ERROR: bk_http_ptr is NULL!\r\n");
		return NULL;
	}
	http_flash_init();
#endif
	client = bk_http_client_init(&config);

	return client;

}

bk_err_t bk_https_client_flash_deinit(bk_http_client_handle_t client)
{
	int err;

	ota_flag = 0;
#if CONFIG_SYSTEM_CTRL
	bk_wifi_ota_dtim(0);
#endif
#if HTTP_WR_TO_FLASH
	http_flash_deinit();
#endif
	if(client)
	{
		err = bk_http_client_cleanup(client);
	}
	else
	{
		BK_LOGE(TAG, "[OTA_HTTPS] ERROR: client is NULL, return BK_FAIL\r\n");
		return BK_FAIL;
	}

	return err;

}

bk_err_t https_ota_event_cb(bk_http_client_event_t *evt)
{
    if(!evt)
    {
        return BK_FAIL;
    }

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
	BK_LOGE(TAG, "HTTPS_EVENT_ERROR\r\n");
	break;
    case HTTP_EVENT_ON_CONNECTED:
	BK_LOGE(TAG, "HTTPS_EVENT_ON_CONNECTED\r\n");
#ifdef CONFIG_HTTP_OTA_WITH_BLE
#if CONFIG_BLUETOOTH
        bk_ble_register_sleep_state_callback(ble_sleep_cb);
#endif
#endif
	break;
    case HTTP_EVENT_HEADER_SENT:
	BK_LOGE(TAG, "HTTPS_EVENT_HEADER_SENT\r\n");
	break;
    case HTTP_EVENT_ON_HEADER:
	BK_LOGE(TAG, "HTTPS_EVENT_ON_HEADER\r\n");
	break;
    case HTTP_EVENT_ON_DATA:
	//do something: evt->data, evt->data_len
#if HTTP_WR_TO_FLASH
	http_wr_to_flash((char *)evt->data,evt->data_len);
#endif
	BK_LOGD(TAG, "HTTP_EVENT_ON_DATA, length:%d\r\n", evt->data_len);
	break;
    case HTTP_EVENT_ON_FINISH:
#if HTTP_WR_TO_FLASH
	http_flash_wr(bk_http_ptr->wr_buf, bk_http_ptr->wr_last_len);
#endif
	bk_https_client_flash_deinit(evt->client);
	BK_LOGI(TAG, "HTTPS_EVENT_ON_FINISH\r\n");
	break;
    case HTTP_EVENT_DISCONNECTED:
	BK_LOGE(TAG, "HTTPS_EVENT_DISCONNECTED\r\n");
	break;

    }
    return BK_OK;
}


int bk_https_ota_download(const char *url)
{
	int err;

      if(!url)
      {
          err = BK_FAIL;
          BK_LOGE(TAG, "[OTA_HTTPS] ERROR: url is NULL, return BK_FAIL\r\n");
          return err;
      }

	/* Validate URL format */
	if (os_strncmp(url, "https://", 8) != 0 && os_strncmp(url, "http://", 7) != 0) {
		BK_LOGW(TAG, "[OTA_HTTPS] WARNING: URL does not start with http:// or https://\r\n");
	}
	
	bk_http_input_t config = {
	    .url = url,
	    .cert_pem = ca_crt_rsa,
	    .event_handler = https_ota_event_cb,
	    .buffer_size = HTTPS_INPUT_SIZE,
	    .timeout_ms = 15000
	};

#ifdef CONFIG_HTTP_AB_PARTITION
	ota_temp_exec_flag temp_exec_flag = 6;
	exec_flag exec_temp_part = 6;
	uint8 current_partition;
	current_partition = bk_ota_get_current_partition();
	if(current_partition == EXEX_A_PART ||current_partition == 0xFF)
	{
		update_part_flag = UPDATE_B_PART;
	}
	else if(current_partition == EXEC_B_PART)
	{
		update_part_flag = UPDATE_A_PART;
	}
	else
	{
		BK_LOGE(TAG, "[OTA_HTTPS] ERROR: Invalid partition 0x%x, return -1\r\n", current_partition);
		return -1;
	}
#endif

	bk_http_client_handle_t client = bk_https_client_flash_init(config);
	if (client == NULL) {
		BK_LOGE(TAG, "[OTA_HTTPS] ERROR: client is NULL after bk_https_client_flash_init\r\n");
		err = BK_FAIL;
		return err;
	}

	err = bk_http_client_perform(client);
	
	if(err == BK_OK){
#ifdef CONFIG_HTTP_AB_PARTITION
        #ifndef CONFIG_OTA_UPDATE_DEFAULT_PARTITION
            temp_exec_flag = ota_temp_execute_partition(err); //temp_exec_flag :3 :A ,4:B
        #else
            #ifdef CONFIG_OTA_UPDATE_B_PARTITION
                temp_exec_flag = CONFIRM_EXEC_B; //update B Partition;
            #else
                temp_exec_flag = CONFIRM_EXEC_A; //update A Partition;
            #endif
        #endif

        if(temp_exec_flag == CONFIRM_EXEC_A){
	        exec_temp_part = EXEX_A_PART;
        }
        else if(temp_exec_flag == CONFIRM_EXEC_B){
		exec_temp_part = EXEC_B_PART;
        }
        else
        {
		BK_LOGE(TAG, "[OTA_HTTPS] ERROR: Invalid temp_exec_flag: 0x%x\r\n", temp_exec_flag);
        }

        ota_write_flash(BK_PARTITION_OTA_FINA_EXECUTIVE, exec_temp_part, 4);
	bk_reboot();
#else
        bk_reboot();
#endif
	}
	else{
		BK_LOGE(TAG, "[OTA_HTTPS] bk_http_client_perform FAILED (err=0x%x)\r\n", err);
		bk_https_client_flash_deinit(client);
	}

	return err;
}
