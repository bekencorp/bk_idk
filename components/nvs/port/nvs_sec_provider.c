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
#include <string.h>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "nvs_sec_provider.h"
#include "os/mem.h"

const bk_logic_partition_t *bk_partition_find_first(esp_partition_type_t type,
        esp_partition_subtype_t subtype, const char *label);

bk_err_t nvs_sec_get_xts_key_te200(uint8_t *eky, uint8_t *tky)
{
    /*TODO, wangzhilei*/
#if CONFIG_SHANHAI_KEY_LADDER
    /* derived key from key ladder*/
#else
#endif
    return BK_OK;
}

// eof

