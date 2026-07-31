// Copyright 2020-2025 Beken
//
// SPDX-License-Identifier: Apache-2.0

#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "driver/flash.h"
#include "partitions_gen.h"
#include "xip_pubkey.h"
#include "ota_verify.h"
#include "ota_common.h"

#define TAG "ota"

/* Declared in ota_verify.h only when CONFIG_PSA_MBEDTLS; implementation is always linked. */
bk_err_t ota_read_tlv_from_partition(const bk_logic_partition_t *partition, uint16_t tlv_type,
				      uint8_t *tlv_value, uint16_t *tlv_value_len);

bk_err_t xip_get_pubkey(const bk_logic_partition_t *app_partition,
			uint16_t tlv_type,
			uint8_t *pubkey_buf,
			uint16_t *pubkey_len_inout)
{
	if (!app_partition || !pubkey_buf || !pubkey_len_inout) {
		return BK_ERR_PARAM;
	}

#if CONFIG_OTA_VERIFY_PUBKEY
	bk_err_t bkret = load_pubkey_from_flash_backup(pubkey_buf, pubkey_len_inout);

	if (bkret == BK_OK) {
		return BK_OK;
	}
#endif

	return ota_read_tlv_from_partition(app_partition, tlv_type, pubkey_buf, pubkey_len_inout);
}
