#include "sdkconfig.h"
#include <string.h>
#include <components/system.h>
#include "partitions.h"
#include "modules/ota_download.h"
#include <driver/wdt.h>
#include <components/log.h>

/* TODO
 * 1. This file only supports OTA downloading/parse state machine in download module
 * 2. Move tfm related low-level ops to ota
 * 3. Move pubkey related ops to ota
 * 4. Move validation to better place
 * */

#define TAG "ota"

#define OTA_MAGIC_WORD "\x42\x4B\x37\x32\x33\x36\x35\x38"
#define OTA_FLASH_WR_BUF_MAX 4096

ota_download_t* bk_ota_download_init()
{
	ota_download_t* ota_parse = (ota_download_t*)malloc(sizeof(ota_download_t));
	if (ota_parse == NULL) {
		return NULL;
	}
	memset(ota_parse, 0, sizeof(ota_download_t));
	CRC32_Init(&(ota_parse->ota_crc));
	ota_parse->wr_buf = (uint8_t*)malloc(OTA_FLASH_WR_BUF_MAX * sizeof(uint8_t));
	if (ota_parse->wr_buf == NULL) {
		free(ota_parse);
		return NULL;
	}
	ota_parse->wr_last_len = 0;

	return ota_parse;
}

bk_err_t bk_ota_download_deinit(ota_download_t* ota_parse)
{
	if (ota_parse == NULL) {
		return BK_OK;
	}

	if (ota_parse->ota_verify != NULL) {
		ota_verify_deinit(ota_parse->ota_verify);
		ota_parse->ota_verify = NULL;
	}

	if (ota_parse->ota_image_header) {
		free(ota_parse->ota_image_header);
		ota_parse->ota_image_header = NULL;
	}

	if (ota_parse->wr_buf) {
		free(ota_parse->wr_buf);
		ota_parse->wr_buf = NULL;
	}

	free(ota_parse);

	return BK_OK;
}


static bk_err_t ota_download_parse_header(ota_download_t* ota_parse, uint8_t **data, int *len)
{
	uint32_t data_len, offset;
	uint8_t *tmp;

	if (*len == 0) return 0;

	if (ota_parse->offset == 0) {
		BK_LOGI(TAG, "downloading OTA global header...\r\n");
	}

	tmp = (uint8_t *)&ota_parse->ota_header;
	data_len = sizeof(ota_header_t) - ota_parse->offset;
	if (*len < data_len) {
		memcpy(tmp + ota_parse->offset, *data, *len);
		ota_parse->offset += *len;
		return BK_OK;
	} else {
		memcpy(tmp + ota_parse->offset, *data, data_len);
		*data += data_len;
		*len -= data_len;
		//check global header magic code!
		if(memcmp(OTA_MAGIC_WORD,tmp,8) != 0){
			BK_LOGE(TAG, "magic error\r\n");
			return BK_ERR_OTA_HDR_MAGIC;
		}

		/*calculate global header crc*/
		offset = sizeof(ota_parse->ota_header.magic) + sizeof(ota_parse->ota_header.crc);
		tmp += offset;
		CRC32_Update(&ota_parse->ota_crc, tmp, sizeof(ota_header_t) - offset);

		/*to next parse*/
		ota_parse->phase = OTA_PARSE_IMG_HEADER;
		ota_parse->offset = 0;
		if (ota_parse->ota_image_header) {
			free(ota_parse->ota_image_header);
		}
		offset = ota_parse->ota_header.image_num * sizeof(ota_image_header_t);
		ota_parse->ota_image_header = (ota_image_header_t *)malloc(offset);
		if (!ota_parse->ota_image_header) {
			BK_LOGE(TAG, "ota parse image header: oom\r\n");
			return BK_ERR_OTA_OOM;
		}
		BK_LOGI(TAG, "crc %x, version %x, header_len %x, image_num %x\r\n",
			ota_parse->ota_header.crc, ota_parse->ota_header.version, ota_parse->ota_header.header_len, ota_parse->ota_header.image_num);
	}

	return BK_OK;
}

static int ota_download_parse_image_header(ota_download_t* ota_parse, uint8_t **data, int *len)
{
	int i;
	uint32_t data_len, offset, crc_control;
	uint8_t *tmp;

	if (*len == 0) return 0;

	if (ota_parse->offset == 0) {
		BK_LOGI(TAG, "downloading OTA image header...\r\n");
	}

	tmp = (uint8_t *)ota_parse->ota_image_header;
	data_len = ota_parse->ota_header.image_num * sizeof(ota_image_header_t) - ota_parse->offset;
	if (*len < data_len) {
		memcpy(tmp + ota_parse->offset, *data, *len);
		ota_parse->offset += *len;
		return 0;
	} else {
		memcpy(tmp + ota_parse->offset, *data, data_len);
		*data += data_len;
		*len -= data_len;

		/*calculate header crc*/
		offset = ota_parse->ota_header.image_num * sizeof(ota_image_header_t);
		CRC32_Update(&ota_parse->ota_crc, tmp, offset);

		//TODO check image CRC!
		CRC32_Final(&ota_parse->ota_crc,&crc_control);
		if(crc_control != ota_parse->ota_header.crc){
			BK_LOGE(TAG, "crc error\r\n");
			return BK_ERR_OTA_IMG_HDR_CRC;
		}

		/*to next parse*/
		ota_parse->phase = OTA_PARSE_IMG;
		ota_parse->offset = 0;
		for (i = 0; i < ota_parse->ota_header.image_num; i++) {
			BK_LOGI(TAG, "image[%d], image_len=%x, image_offset=%x, flash_offset=%x\r\n", i,
				ota_parse->ota_image_header[i].image_len,
				ota_parse->ota_image_header[i].image_offset,
				ota_parse->ota_image_header[i].flash_offset);
		}
	}

	return 0;
}

static void ota_download_write_flash(ota_download_t* ota_parse, uint8_t **data, uint32_t len, bool need_flush)
{
	uint32_t copy_len;
	uint32_t data_idx = 0;
	uint32_t http_flash_wr_buf_max = OTA_FLASH_WR_BUF_MAX;
	uint32_t recover_len;
	uint32_t erase_size;
	bk_ota_get_info(ota_parse->ota_handle, &recover_len, &erase_size);

	if (ota_parse->entire_flash_erase == false && ota_parse->erase_offset <= ota_parse->write_offset && ota_parse->erase_offset < erase_size) {
		if (recover_len != 0) {
			uint8_t recover[34];
			memset(recover, 0xFF, 34);
			bk_ota_read(ota_parse->ota_handle, ota_parse->erase_offset - recover_len, recover, recover_len);
			bk_ota_erase_sector(ota_parse->ota_handle, ota_parse->erase_offset);
			bk_ota_write(ota_parse->ota_handle, ota_parse->erase_offset - recover_len, recover, recover_len);
		} else {
			bk_ota_erase_sector(ota_parse->ota_handle, ota_parse->erase_offset);
		}
		ota_parse->erase_offset += 4096;
	}

	while(data_idx < len) {
		copy_len = min(len-(data_idx),http_flash_wr_buf_max - ota_parse->wr_last_len);
		memcpy(ota_parse->wr_buf + ota_parse->wr_last_len, *data + data_idx, copy_len);
		data_idx += copy_len;
		ota_parse->wr_last_len += copy_len;
		if(ota_parse->wr_last_len >= http_flash_wr_buf_max) {
			bk_ota_write(ota_parse->ota_handle, ota_parse->write_offset, (const void *)ota_parse->wr_buf, http_flash_wr_buf_max);
			ota_parse->write_offset += http_flash_wr_buf_max;
			ota_parse->wr_last_len = 0;
		}
	}
	if (need_flush == true) {
		bk_ota_write(ota_parse->ota_handle, ota_parse->write_offset, (const void *)ota_parse->wr_buf, ota_parse->wr_last_len); //TODO check wr_last_en for TFM
		ota_parse->wr_last_len = 0;
		ota_parse->write_offset += ota_parse->wr_last_len;
	}
}

/*TODO: ota receive data len > partition len*/
static bk_err_t ota_download_handle_image(ota_download_t* ota_parse, uint8_t **data, int *len)
{
	uint32_t image_crc;
	bk_err_t ret = BK_OK;

#if (CONFIG_TASK_WDT)
	extern void bk_task_wdt_feed(void);
	bk_task_wdt_feed();
#endif
#if (CONFIG_INT_WDT)
	bk_wdt_feed();
#endif

	do {
		if (ota_parse->offset == 0) {
			BK_LOGI(TAG, "downloading OTA image%d, expected data len=%x...\r\n", ota_parse->index, ota_parse->ota_image_header[ota_parse->index].image_len);
			ota_parse->ota_handle = bk_ota_addr_to_handle(ota_parse->ota_image_header[ota_parse->index].flash_offset);
			if (ota_parse->ota_handle == NULL) {
				BK_LOGE(TAG, "image flash offset error\r\n");
				return BK_ERR_OTA_HDR_MAGIC;
			}
			if (ota_parse->entire_flash_erase == true) {
				bk_ota_erase(ota_parse->ota_handle);
			}

			ota_parse->percent = 0;
			CRC32_Init(&(ota_parse->ota_crc));
#if CONFIG_VALIDATE_DURING_DOWNLOADING || CONFIG_OTA_UPDATE_PUBKEY
			ota_parse->ota_verify = ota_verify_init();
			if (ota_parse->ota_verify == NULL) {
				return BK_ERR_NO_MEM;
			}
#endif
		}
		uint32_t image_len = ota_parse->ota_image_header[ota_parse->index].image_len;
		uint32_t data_len = image_len - ota_parse->offset;

		if (ota_parse->offset >= (image_len/10 + image_len*ota_parse->percent/100)) {
			ota_parse->percent += 10;
			if (ota_parse->percent < 100) {
				BK_LOGI(TAG, "downloading %d%%\r\n", ota_parse->percent);
			}
		}
#if CONFIG_VALIDATE_DURING_DOWNLOADING || CONFIG_OTA_UPDATE_PUBKEY
		ret = ota_verify_receive_data(ota_parse->ota_verify, *data, *len);
#endif
		if (*len < data_len) {
			ota_download_write_flash(ota_parse, data, *len, false);
			CRC32_Update(&ota_parse->ota_crc,*data,*len);
			ota_parse->offset += *len;
			*len = 0;
		} else {
			ota_download_write_flash(ota_parse, data, data_len, true);
			CRC32_Update(&ota_parse->ota_crc,*data,data_len);
			*data += data_len;
			*len -= data_len;

			BK_LOGI(TAG, "downloaded OTA image%d\r\n", ota_parse->index);
			//check image CRC, then we can abort quickly!
			CRC32_Final(&ota_parse->ota_crc,&image_crc);
			if(image_crc !=  ota_parse->ota_image_header[ota_parse->index].checksum){
				BK_LOGE(TAG, "image crc error!\r\n");
				return BK_ERR_OTA_IMG_CRC;
			}
#if CONFIG_VALIDATE_DURING_DOWNLOADING
			if (ret == BK_OK) {
				ret = ota_verify_image(ota_parse->ota_handle->app_partition, ota_parse->ota_verify);
				if (ret != BK_OK) {
					ota_verify_deinit(ota_parse->ota_verify);
					ota_parse->ota_verify = NULL;
					return ret;
				}
			}
			ota_verify_deinit(ota_parse->ota_verify);
			ota_parse->ota_verify = NULL;
#elif CONFIG_OTA_UPDATE_PUBKEY
			// Keep ota_verify for trust chain verification after download completes
			// ota_verify will be deinitialized in cli_https_ota.c after trust chain verification
#endif
			/*to next image*/
			BK_LOGI(TAG, "\r\n");
			ota_parse->index++;
			ota_parse->offset = 0;
			ota_parse->write_offset = 0;
			ota_parse->erase_offset = 0;
		}
	} while(*len);

	return ret;
}

bk_err_t bk_ota_download_parse_data(ota_download_t* ota_parse, char *data, int len)
{
	int ret = BK_OK;

	ota_parse->total_rx_len += len;
	if (ota_parse->phase == OTA_PARSE_HEADER) {
		ret = ota_download_parse_header(ota_parse, (uint8_t **)&data, &len);
		if (ret != BK_OK) {
			goto end;
		}
	}

	if (ota_parse->phase == OTA_PARSE_IMG_HEADER) {
		ret = ota_download_parse_image_header(ota_parse, (uint8_t **)&data, &len);
		if (ret != BK_OK) {
			goto end;
		}
	}

	if (ota_parse->phase == OTA_PARSE_IMG) {
		if (len == 0) return BK_OK;

		ret = ota_download_handle_image(ota_parse, (uint8_t **)&data, &len);
		if (ret != BK_OK) {
			goto end;
		}
	}

	return BK_OK;

end:
	ota_parse->ota_validated_flag = false;
	return ret;
}

void bk_ota_download_set_flash_erase(ota_download_t* ota_parse, bool is_entire_flash_erase)
{
	ota_parse->entire_flash_erase = is_entire_flash_erase;
}
