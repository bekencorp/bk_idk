#include <string.h>
#include "modules/https_ota.h"

#define TAG "HTTPS_OTA"

bk_err_t bk_https_ota_init(https_ota_t* ota_config)
{
	if (ota_config == NULL) {
		return BK_FAIL;
	}
	ota_config->ota_parse = bk_ota_download_init();
	if (ota_config->ota_parse == NULL) {
		return BK_FAIL;
	}
	ota_config->state = HTTPS_OTA_INIT;
	return BK_OK;
}

bk_err_t bk_https_ota_deinit(https_ota_t* ota_config)
{
	if (ota_config == NULL) {
		return BK_FAIL;
	}
	ota_config->state = HTTPS_OTA_UNINIT;
	return bk_ota_download_deinit(ota_config->ota_parse);
}

bk_err_t bk_https_ota_begin(https_ota_t* ota_config)
{
	if (ota_config == NULL || ota_config->ota_parse == NULL) {
		return BK_FAIL;
	}
	bk_err_t ret = BK_OK;
	ota_config->client = bk_http_client_init(ota_config->http_config);

	bk_http_client_set_method(ota_config->client, HTTP_METHOD_HEAD);
	bk_http_client_open(ota_config->client, 0);
	bk_http_client_fetch_headers(ota_config->client);

	bk_http_check_response(ota_config->client);

	if (ota_config->ETag == NULL && ota_config->client->etag != NULL) {
		ota_config->ETag = (char*)malloc(strlen(ota_config->client->etag) + 1);
		if (ota_config->ETag == NULL) {
			return BK_ERR_NO_MEM;
		} else {
			memcpy(ota_config->ETag, ota_config->client->etag, strlen(ota_config->client->etag) + 1);
		}
	}
	ota_config->state = HTTPS_OTA_READY;
	return ret;
}

bk_err_t bk_https_ota_progress_data(https_ota_t* ota_config)
{
	if (ota_config == NULL || ota_config->ota_parse == NULL) {
		return BK_FAIL;
	}
	bk_err_t err = BK_FAIL;
	uint32_t read_buf_len = 4096;
	char* read_buf;
	int32_t data_read;
	ota_config->state = HTTPS_OTA_PROGRESS_DATA;
	ota_config->client->cache_data_in_fetch_hdr = 1;
	bk_http_client_set_method(ota_config->client, HTTP_METHOD_GET);

	if (ota_config->ETag != NULL) {
		// TODO: use ETag to avoid duplicate download
		// bk_http_client_set_header(ota_config->client, "If-None-Match", ota_config->ETag);
	}

	if (ota_config->ota_download_resume == true && ota_config->ota_parse->total_rx_len > 0) {
		char resume_download[30];
		snprintf(resume_download,30,"bytes=%u-", ota_config->ota_parse->total_rx_len);
		bk_http_client_set_header(ota_config->client, "Range", resume_download);
	}

	char* post_data = NULL;
	uint32_t post_len = bk_http_client_get_post_field(ota_config->client, &post_data);
	bk_http_client_request_send(ota_config->client, post_len);
	if ((err = bk_http_check_response(ota_config->client)) != BK_OK) {
		BK_LOGE(TAG, "Error response\r\n");
		return err;
	}

	read_buf = (char*)os_malloc(read_buf_len * (sizeof(char)));
	if (read_buf == NULL) {
		return BK_ERR_NO_MEM;
	}
	while (ota_config->state == HTTPS_OTA_PROGRESS_DATA) {
		data_read = bk_http_client_read(ota_config->client, read_buf, read_buf_len);
		if (data_read == 0) {
			if (bk_http_client_is_complete_data_received(ota_config->client)) {
				BK_LOGI(TAG, "---------RECEIVED ALL DATA OVER---------\r\n");
				ota_config->state = HTTPS_OTA_FINISH;
				break;
			} else {
				BK_LOGE(TAG, "connection closed before receiving all the data.\r\n");
				goto fail;
			}
		} else if (data_read > 0) {
			if (bk_ota_download_parse_data(ota_config->ota_parse, read_buf, data_read) != BK_OK) {
				BK_LOGE(TAG, "ota download parse data fail.\r\n");
				goto fail;
			}
		} else {
			BK_LOGE(TAG, "http client read data error\r\n");
			goto fail;
		}
	}
	ota_config->client->response->buffer->raw_len = 0;
	os_free(read_buf);
	return BK_OK;
fail:
	ota_config->state = HTTPS_OTA_FAIL;
	os_free(read_buf);
	return BK_FAIL;
}

bk_err_t bk_https_ota_end(https_ota_t* ota_config)
{
	if (ota_config == NULL) {
		return BK_FAIL;
	}
	if (ota_config->client) {
		bk_http_client_close(ota_config->client);
		bk_http_client_cleanup(ota_config->client);
	}
	if (ota_config->ETag) {
		os_free(ota_config->ETag);
	}

	return BK_OK;
}

bk_err_t bk_https_ota_pause(https_ota_t* ota_config)
{
	if (ota_config == NULL) {
		return BK_FAIL;
	}
	ota_config->state = HTTPS_OTA_PAUSE;
	return BK_OK;
}

bk_err_t bk_https_ota_abort(https_ota_t* ota_config)
{
	if (ota_config == NULL) {
		return BK_FAIL;
	}
	ota_config->state = HTTPS_OTA_UNINIT;
	bk_https_ota_end(ota_config);
	bk_https_ota_deinit(ota_config);
	return BK_OK;
}

bk_err_t bk_https_ota_set_erase_method(https_ota_t* ota_config, bool is_entire_flash_erase)
{
	if (ota_config == NULL || ota_config->ota_parse == NULL) {
		return BK_FAIL;
	}
	ota_config->ota_parse->entire_flash_erase = is_entire_flash_erase;
	return BK_OK;
}

https_ota_state_t bk_https_ota_perform(https_ota_t* ota_config)
{
	while(1) {
		switch (ota_config->state) {
			case HTTPS_OTA_INIT:
				bk_https_ota_begin(ota_config);
				break;
			case HTTPS_OTA_READY:
			case HTTPS_OTA_PROGRESS_DATA:
				bk_https_ota_progress_data(ota_config);
				break;
			case HTTPS_OTA_PAUSE:
			case HTTPS_OTA_UNINIT:
				goto ret;
			case HTTPS_OTA_FINISH:
			case HTTPS_OTA_FAIL:
			default:
				goto end;
		}
	}

end:
	bk_https_ota_end(ota_config);
ret:
	return ota_config->state;
}