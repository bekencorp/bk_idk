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

#include <string.h>
#include "os/os.h"
#include "os/mem.h"
#include "bk_ieee802154_ack.h"

static ieee802154_pending_table_t ieee802154_pending_table;
static bk_ieee802154_pending_mode_t s_pending_mode = IEEE802154_AUTO_PENDING_DISABLE;

#define GET_MASK_ITEM_FROM_TABLE(mask, pos) (mask[(pos) / IEEE802154_PENDING_TABLE_MASK_BITS])

#define BIT_SET(mask, pos) (GET_MASK_ITEM_FROM_TABLE(mask, pos) |= (1UL << (pos % IEEE802154_PENDING_TABLE_MASK_BITS)))
#define BIT_CLR(mask, pos) (GET_MASK_ITEM_FROM_TABLE(mask, pos) &= ~(1UL << (pos % IEEE802154_PENDING_TABLE_MASK_BITS)))
#define BIT_IST(mask, pos) (GET_MASK_ITEM_FROM_TABLE(mask, pos) & (1UL << (pos % IEEE802154_PENDING_TABLE_MASK_BITS)))

#define ACK_HEADER_WITH_PENDING 0x12
#define ACK_HEADER_WITHOUT_PENDING 0x02

static uint8_t m_ack_data[IEEE802154_IMM_ACK_LENGTH];
static bool ieee802154_addr_in_pending_table(const uint8_t *addr, bool is_short)
{
    bool ret = false;
    if (is_short) {
        for (uint8_t index = 0; index < CONFIG_IEEE802154_PENDING_TABLE_SIZE; index++) {
            if (BIT_IST(ieee802154_pending_table.short_addr_mask, index) &&
                    os_memcmp(addr, ieee802154_pending_table.short_addr[index], IEEE802154_FRAME_SHORT_ADDR_SIZE) == 0) {
                ret = true;
                break;
            }
        }
    } else {
        for (uint8_t index = 0; index < CONFIG_IEEE802154_PENDING_TABLE_SIZE; index++) {
            if (BIT_IST(ieee802154_pending_table.ext_addr_mask, index) &&
                    os_memcmp(addr, ieee802154_pending_table.ext_addr[index], IEEE802154_FRAME_EXT_ADDR_SIZE) == 0) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

bk_ieee802154_pending_mode_t ieee802154_get_pending_mode()
{
    return s_pending_mode;
}

void ieee802154_set_pending_mode(bool is_enable)
{
    if (is_enable) {
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    s_pending_mode = IEEE802154_AUTO_PENDING_ENHANCED;
#else
    s_pending_mode = IEEE802154_AUTO_PENDING_ENABLE;
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    } else {
        s_pending_mode = IEEE802154_AUTO_PENDING_DISABLE;
    }
}

bk_err_t ieee802154_add_pending_addr(const uint8_t *addr, bool is_short)
{
    bk_err_t ret = BK_FAIL;
    int8_t first_empty_index = -1;
    if (is_short) {
        for (uint8_t index = 0; index < CONFIG_IEEE802154_PENDING_TABLE_SIZE; index++) {
            if (!BIT_IST(ieee802154_pending_table.short_addr_mask, index)) {
                // record the first empty index
                first_empty_index = (first_empty_index == -1 ? index : first_empty_index);
                break;
            } else if (os_memcmp(addr, ieee802154_pending_table.short_addr[index], IEEE802154_FRAME_SHORT_ADDR_SIZE) == 0) {
                // The address is in the table already.
                ret = BK_OK;
                return ret;
            }
        }
        if (first_empty_index != -1) {
            os_memcpy(ieee802154_pending_table.short_addr[first_empty_index], addr, IEEE802154_FRAME_SHORT_ADDR_SIZE);
            BIT_SET(ieee802154_pending_table.short_addr_mask, first_empty_index);
            ret = BK_OK;
        }
    } else {
        for (uint8_t index = 0; index < CONFIG_IEEE802154_PENDING_TABLE_SIZE; index++) {
            if (!BIT_IST(ieee802154_pending_table.ext_addr_mask, index)) {
                first_empty_index = (first_empty_index == -1 ? index : first_empty_index);
                break;
            } else if (os_memcmp(addr, ieee802154_pending_table.ext_addr[index], IEEE802154_FRAME_EXT_ADDR_SIZE) == 0) {
                // The address is already in the pending table.
                ret = BK_OK;
                return ret;
            }
        }
        if (first_empty_index != -1) {
            os_memcpy(ieee802154_pending_table.ext_addr[first_empty_index], addr, IEEE802154_FRAME_EXT_ADDR_SIZE);
            BIT_SET(ieee802154_pending_table.ext_addr_mask, first_empty_index);
            ret = BK_OK;
        }
    }
    return ret;
}

bk_err_t ieee802154_clear_pending_addr(const uint8_t *addr, bool is_short)
{
    bk_err_t ret = BK_FAIL;
    // Consider this function may be called in ISR, only clear the mask bits for finishing the process quickly.
    if (is_short) {
        for (uint8_t index = 0; index < CONFIG_IEEE802154_PENDING_TABLE_SIZE; index++) {
            if (BIT_IST(ieee802154_pending_table.short_addr_mask, index) &&
                    os_memcmp(addr, ieee802154_pending_table.short_addr[index], IEEE802154_FRAME_SHORT_ADDR_SIZE) == 0) {
                BIT_CLR(ieee802154_pending_table.short_addr_mask, index);
                ret = BK_OK;
                break;
            }
        }
    } else {
        for (uint8_t index = 0; index < CONFIG_IEEE802154_PENDING_TABLE_SIZE; index++) {
            if (BIT_IST(ieee802154_pending_table.ext_addr_mask, index) &&
                    os_memcmp(addr, ieee802154_pending_table.ext_addr[index], IEEE802154_FRAME_EXT_ADDR_SIZE) == 0) {
                BIT_CLR(ieee802154_pending_table.ext_addr_mask, index);
                ret = BK_OK;
                break;
            }
        }
    }

    return ret;
}

void ieee802154_reset_pending_table(bool is_short)
{
    // Consider this function may be called in ISR, only clear the mask bits for finishing the process quickly.
    if (is_short) {
        os_memset(ieee802154_pending_table.short_addr_mask, 0, IEEE802154_PENDING_TABLE_MASK_SIZE);
    } else {
        os_memset(ieee802154_pending_table.ext_addr_mask, 0, IEEE802154_PENDING_TABLE_MASK_SIZE);
    }
}

bool ieee802154_ack_config_pending_bit(const uint8_t *frame)
{
    bool pending_bit = false;
    uint8_t addr[IEEE802154_FRAME_EXT_ADDR_SIZE] = {0};
    uint8_t src_mode = 0;

    bk_ieee802154_pending_mode_t pending_mode = ieee802154_get_pending_mode();
    switch (pending_mode) {
    case IEEE802154_AUTO_PENDING_DISABLE:
        if (is_data_request_frame(frame)) {
            pending_bit = true;
        }
        break;
    case IEEE802154_AUTO_PENDING_ENABLE:
    case IEEE802154_AUTO_PENDING_ENHANCED:
        src_mode = ieee802154_frame_get_src_addr(frame, addr);
        if (src_mode == IEEE802154_FRAME_SRC_MODE_SHORT || src_mode == IEEE802154_FRAME_SRC_MODE_EXT) {
            if (ieee802154_addr_in_pending_table(addr, src_mode == IEEE802154_FRAME_SRC_MODE_SHORT)) {
                pending_bit = true;
            }
        }
        break;
    case IEEE802154_AUTO_PENDING_ZIGBEE:
        // If the address type is short and in pending table, set 'pending_bit' false, otherwise set true.
        src_mode = ieee802154_frame_get_src_addr(frame, addr);
        pending_bit = true;
        if (src_mode == IEEE802154_FRAME_SRC_MODE_SHORT && ieee802154_addr_in_pending_table(addr, src_mode == IEEE802154_FRAME_SRC_MODE_SHORT)) {
            pending_bit = false;
        }
        break;
    default:
        break;
    }

    return pending_bit;
}

uint8_t* bk_ieee802154_imm_ack_generator_create(const uint8_t *frame, bool frame_pending)
{
    m_ack_data[2] = frame[2];
    if (frame_pending)
    {
        m_ack_data[0] = ACK_HEADER_WITH_PENDING;
    }
    else
    {
        m_ack_data[0] = ACK_HEADER_WITHOUT_PENDING;
    }
    return m_ack_data;
}
