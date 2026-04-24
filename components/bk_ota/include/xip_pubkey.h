// Copyright 2020-2025 Beken
//
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>
#include "common/bk_include.h"
#include "driver/flash_partition.h"

bk_err_t xip_get_pubkey(const bk_logic_partition_t *app_partition,
			uint16_t tlv_type,
			uint8_t *pubkey_buf,
			uint16_t *pubkey_len_inout);
