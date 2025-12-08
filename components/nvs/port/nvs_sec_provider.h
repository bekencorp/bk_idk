/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 /*
* Change Logs:
* Date			 Author 	  Notes
* 2024-04-11	 Beken	  adapter to Beken sdk
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "nvs_internal.h"

/**
 * @brief Helper for populating the Flash encryption-based scheme specific configuration data
 */
#define NVS_KEY_PARTITION_LABEL      "nvs_key"
#define NVS_PARTITION_LABEL          "nvs"

bk_err_t nvs_sec_get_xts_key_te200(uint8_t *eky, uint8_t *tky);

const bk_logic_partition_t *bk_partition_find_first(esp_partition_type_t type,
        esp_partition_subtype_t subtype, const char *label);

#ifdef __cplusplus
}
#endif
// eof

