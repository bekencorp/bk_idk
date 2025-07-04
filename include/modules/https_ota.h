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
#include "bk_https.h"
#include "ota_download.h"

typedef enum {
	HTTPS_OTA_UNINIT,
	HTTPS_OTA_INIT,
	HTTPS_OTA_READY,
	HTTPS_OTA_PROGRESS_DATA,
	HTTPS_OTA_FINISH,
	HTTPS_OTA_PAUSE,
	HTTPS_OTA_FAIL,
} https_ota_state_t;

typedef enum {
	HTTPS_OTA_START_EVENT,
	HTTPS_OTA_ERROR_EVENT,
	HTTPS_OTA_PAUSE_EVENT,
	HTTPS_OTA_SUCCESS_EVENT,
	HTTPS_OTA_RESTART_EVENT,
} https_ota_event_id_t;

typedef struct {
	https_ota_event_id_t event_id;
	void *data;
	int data_len;
} https_ota_event_t;

typedef void (*https_ota_cb_t)(https_ota_event_t);

typedef struct {
	bk_http_input_t* http_config;
	bk_http_client_handle_t client;
	https_ota_state_t state;
	bool ota_download_resume;
	uint32_t max_http_request_size;
	https_ota_cb_t ota_event_handler;
	char* ETag;
	ota_download_t* ota_parse;
} https_ota_t;

/*
 * @brief    Https ota firmware upgrade init download config
 * 
 * This function initializes download config in the https ota config.
 * This function must be invoked first. It also represents the initalization
 * of the entire OTA firmware upgrade.
 * 
 * @param[in] ota_config          pointer to https_ota_t struct which has been assigned values
 * 
 * @return
 *     - BK_FAIL:                 parameter pointer is empty
 *     - BK_ERR_NO_MEM:           no enough heap space
 *     - BK_OK:                   initialize succeed
*/
bk_err_t bk_https_ota_init(https_ota_t* ota_config);

/*
 * @brief    Establish a connection to the HTTP server and retrieve firmware information
 * 
 * This function tries to establish a HTTPS connection. 
 * If succeed, use GET method to request firmware from server.
 * 
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 connect fail or firmware resource unavailable
 *     - BK_OK:                   succeed
*/
bk_err_t bk_https_ota_begin(https_ota_t* ota_config);

/*
 * @brief    Read data from HTTP stream and write data into flash
 * 
 * This function starts a loop to receive data from HTTP stream.
 * Then it hands data over to the OTA download state machine for processing.
 * 
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 connect fail or other failure
 *     - BK_OK:                   finish processing all the data
*/
bk_err_t bk_https_ota_progress_data(https_ota_t* ota_config);

/*
 * @brief    Close HTTPS connection
 * 
 * This function closes the HTTP connection and free http context.
 * But it does not free OTA process context. There can be multiple HTTP
 * connections during once OTA process. 
 * 
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 param error
 *     - BK_OK:                   finish processing all the data
*/
bk_err_t bk_https_ota_end(https_ota_t* ota_config);

/*
 * @brief    https ota firmware upgrade release download config
 * 
 * This function releases download config in the https ota config.
 * This function should be called after the OTA firmware upgrade process is completed. 
 * It will not release the ota config.
 * 
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 parameter pointer is empty
 *     - BK_OK:                   deinit succeed
*/
bk_err_t bk_https_ota_deinit(https_ota_t* ota_config);

/*
 * @brief    pause ota download but keep the connection open
 * 
 * This function releases download config in the https ota config.
 * This function should be called after the OTA firmware upgrade process is completed. 
 * It will not release the ota config. If want to recover the download, call 
 * bk_https_ota_progress_data() or change ota_state to HTTPS_OTA_PROGRESS_DATA
 * then call bk_https_ota_perform()
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 parameter pointer is empty
 *     - BK_OK:                   deinit succeed
*/
bk_err_t bk_https_ota_pause(https_ota_t* ota_config);

/*
 * @brief    abort ota process and release resoureces
 * 
 * This function terminates download but will not erase ota partition.
 * It will not release ota_config struct.
 * 
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 parameter pointer is empty
 *     - BK_OK:                   abort succeed
*/
bk_err_t bk_https_ota_abort(https_ota_t* ota_config);

/*
 * @brief    Https ota firmware upgrade 
 * 
 * This function upgrade context, establishes HTTPS connection,
 * reads image data from HTTP stream and writes it to OTA partition.
 * It must be called after calling bk_https_ota_init().
 * 
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 parameter pointer is empty
 *     - BK_OK:                   succeed
*/
https_ota_state_t bk_https_ota_perform(https_ota_t* ota_config);

/*
 * @brief    Change the flash erase method
 * 
 * This function changes the flash erase method.
 * There are two ways to erase OTA partiton. One is to erase
 * the entire flash before downloading, which takes less time
 * to erase. The other is to erase sectors while writing, which 
 * the duration with interrupts disabled is shorter.
 * It must be called after calling bk_https_ota_init().
 * 
 * @param[in] ota_config          pointer to https_ota_t struct
 * 
 * @return
 *     - BK_FAIL:                 parameter pointer is empty
 *     - BK_OK:                   succeed
*/
bk_err_t bk_https_ota_set_erase_method(https_ota_t* ota_config, bool is_entire_flash_erase);