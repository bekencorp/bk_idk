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

#include <stdint.h>
#include <stdbool.h>

#include "common/bk_err.h"
#include "components/system.h"
#include "bk_ieee802154_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The frame pending mode types.
 */
typedef enum {
    IEEE802154_AUTO_PENDING_DISABLE,   /*!< Frame pending bit always set to 1 in the ack to Data Request */
    IEEE802154_AUTO_PENDING_ENABLE,    /*!< Frame pending bit set to 1 if src address matches, in the ack to Data Request */
    IEEE802154_AUTO_PENDING_ENHANCED,  /*!< Frame pending bit set to 1 if src address matches, in all ack frames */
    IEEE802154_AUTO_PENDING_ZIGBEE,    /*!< Frame pending bit set to 0 only if src address is short address and matches in table, in the ack to Data Request */
} bk_ieee802154_pending_mode_t;

/**
 * @brief The radio pending table, which is utilized to determine whether the received frame should be responded to with pending bit enabled.
 */

#define IEEE802154_PENDING_TABLE_MASK_BITS (8)
#define CONFIG_IEEE802154_PENDING_TABLE_SIZE 8
#define IEEE802154_PENDING_TABLE_MASK_SIZE (((CONFIG_IEEE802154_PENDING_TABLE_SIZE - 1) / IEEE802154_PENDING_TABLE_MASK_BITS) + 1)
typedef struct {
    uint8_t short_addr[CONFIG_IEEE802154_PENDING_TABLE_SIZE][IEEE802154_FRAME_SHORT_ADDR_SIZE]; /*!< Short address table */
    uint8_t ext_addr[CONFIG_IEEE802154_PENDING_TABLE_SIZE][IEEE802154_FRAME_EXT_ADDR_SIZE];     /*!< Extend address table */
    uint8_t short_addr_mask[IEEE802154_PENDING_TABLE_MASK_SIZE];                                /*!< The mask which the index of short address table is used */
    uint8_t ext_addr_mask[IEEE802154_PENDING_TABLE_MASK_SIZE];                                  /*!< The mask which the index of extended address table is used */
} ieee802154_pending_table_t;

/**
 * @brief  Add an address to the pending table.
 *
 * @param[in]  addr  The pointer to the address needs to be added.
 * @param[in]  is_short  The type of address, true for short address, false for extended.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_FAIL on failure due to the table is full.
 *
 */
bk_err_t ieee802154_add_pending_addr(const uint8_t *addr, bool is_short);

/**
 * @brief  Remove an address in pending table.
 *
 * @param[in]  addr  The pointer to the address needs to be cleared.
 * @param[in]  is_short  The type of address, true for short address, false for extended.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_FAIL on failure if the given address is not present in the pending table.
 *
 */
bk_err_t ieee802154_clear_pending_addr(const uint8_t *addr, bool is_short);

/**
 * @brief  Reset the pending table, only clear the mask bits for finishing the process quickly.
 *
 * @param[in]  is_short  The type of address, true for resetting short address table, false for extended.
 *
 */
void ieee802154_reset_pending_table(bool is_short);

void ieee802154_set_pending_mode(bool is_enable);
bk_ieee802154_pending_mode_t ieee802154_get_pending_mode();
uint8_t* bk_ieee802154_imm_ack_generator_create(const uint8_t *frame, bool frame_pending);
bool ieee802154_ack_config_pending_bit(const uint8_t *frame);


#ifdef __cplusplus
}
#endif
