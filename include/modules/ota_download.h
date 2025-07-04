// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "sdkconfig.h"
#include "CheckSumUtils.h"
#include "modules/ota.h"
#include "ota_verify.h"

#define BK_ERR_OTA_HDR_MAGIC   (BK_ERR_OTA_BASE - 1)
#define BK_ERR_OTA_OOM         (BK_ERR_OTA_BASE - 2)
#define BK_ERR_OTA_IMG_HDR_CRC (BK_ERR_OTA_BASE - 3)
#define BK_ERR_OTA_IMG_ID      (BK_ERR_OTA_BASE - 4)
#define BK_ERR_OTA_IMG_CRC     (BK_ERR_OTA_BASE - 5)

//TODO auto gen flags from partition.csv
#define OTA_IMG_FLAG_OW    (1<<16)
#define OTA_IMG_FLAG_XIP   (1<<17)

typedef struct ota_header_s {
	UINT64 magic;
	UINT32 crc;
	UINT32 version;
	UINT16 header_len;
	UINT16 image_num;
	UINT32 flags;
	UINT32 reserved[2];
} ota_header_t;

typedef struct ota_image_header_s {
	UINT32 image_len;
	UINT32 image_offset;
	UINT32 flash_offset;
	UINT32 checksum;
	UINT32 version;
	UINT32 flags;
	UINT32 reserved[2];
} ota_image_header_t;

typedef enum {
	OTA_PARSE_HEADER = 0,
	OTA_PARSE_IMG_HEADER,
	OTA_PARSE_IMG,
} ota_parse_type;

typedef struct ota_download_s {
	ota_parse_type phase;
	ota_header_t ota_header;
	ota_image_header_t *ota_image_header;
	uint32_t offset;
	uint32_t index;
	uint32_t write_offset;
	bool entire_flash_erase;
	uint32_t erase_offset;
	CRC32_Context ota_crc;
	uint32_t total_rx_len;
	uint32_t percent;
	uint8_t *wr_buf;
	uint32_t wr_last_len;
	bool ota_validated_flag;
	ota_verify_t* ota_verify;
	bk_ota_t *ota_handle;
} ota_download_t;

typedef enum {
	OTA_DOWNLOAD_START_EVENT,
	OTA_DOWNLOAD_ERROR_EVENT,
	OTA_DOWNLOAD_PAUSE_EVENT,
	OTA_DOWNLOAD_RESTART_EVENT,
} ota_download_event_id_t;

ota_download_t* bk_ota_download_init();
bk_err_t bk_ota_download_deinit(ota_download_t* ota_parse);
bk_err_t bk_ota_download_parse_data(ota_download_t* ota_parse, char *data, int len);
bk_err_t bk_ota_download_set_erase_method(ota_download_t* ota_parse, bool is_entire_flash_erase);
void ota_download_dispatch_event(ota_download_event_id_t event_id, void* event_data, int event_data_len);