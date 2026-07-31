// Copyright 2020-2025 Beken
//
// SPDX-License-Identifier: Apache-2.0

#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "driver/flash.h"
#include "partitions_gen.h"
#include "ow_pubkey.h"
#include "ota_verify.h"
#include "ota_common.h"

#define TAG "ota"

bk_err_t ow_get_pubkey(const bk_logic_partition_t *app_partition,
		       uint8_t *pubkey_buf,
		       uint16_t *pubkey_len_inout)
{
	if (!app_partition || !pubkey_buf || !pubkey_len_inout) {
		return BK_ERR_PARAM;
	}

#if CONFIG_OTA_VERIFY_PUBKEY
	bk_err_t ret = load_pubkey_from_flash_backup(pubkey_buf, pubkey_len_inout);

	if (ret == BK_OK) {
		return BK_OK;
	}
#endif

	return read_pubkey_from_primary(app_partition, pubkey_buf, pubkey_len_inout);
}
