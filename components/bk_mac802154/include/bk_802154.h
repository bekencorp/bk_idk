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
#include "stdint.h"
#include "common/bk_include.h"
#include "common/bk_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define US_PER_SYMBLE  16
#define BK_FRAME_BUFFER_MAX_SIZE            16 //Must be 2^n
#define BK_FRAME_LENGTH                     128
/**
 * @brief The radio state types.
 */
typedef enum {
    BK_802154_STATE_DISABLE = 0,
    BK_802154_STATE_IDLE,
    BK_802154_STATE_SLEEP,
    BK_802154_STATE_RECEIVE,
    BK_802154_STATE_RECEIVE_ACK,
    BK_802154_STATE_TRANSMIT,
    BK_802154_STATE_TRANSMIT_ACK,
    BK_802154_STATE_TRANSMIT_ENH_ACK,
    BK_802154_STATE_TRANSMIT_CCA,
    BK_802154_STATE_ED,
    BK_802154_STATE_CCA,
} bk_802154_state_t;

/**
 * @brief The possible errors during frame transmission.
 */
typedef enum {
    BK_802154_TX_ERR_NONE = 0,
    BK_802154_TX_ERR_CCA_BUSY,
    BK_802154_TX_ERR_ABORT,
    BK_802154_TX_ERR_NO_ACK,
    BK_802154_TX_ERR_INVALID_ACK,
    BK_802154_TX_ERR_NO_BUFFER,
} bk_802154_tx_err_t;

/**
 * @brief The possible errors during frame reception.
 */
typedef enum {
    BK_802154_RX_ERR_NONE = 0,
    BK_802154_RX_ERR_ABORT,
    BK_802154_RX_ERR_MALFORMED,
    BK_802154_RX_ERR_NO_BUFFER,
} bk_802154_rx_err_t;

/**
 * @brief Rx abort reasons.
 */
typedef enum {
    BK_802154_RX_ABORT_SFD_TIMEOUT     = 0b0001,
    BK_802154_RX_ABORT_CRC_ERR	       = 0b0010,
    BK_802154_RX_ABORT_INVALID_LEN     = 0b0011,
    BK_802154_RX_ABORT_ACK_TIMEOUT     = 0b0100,
    BK_802154_RX_ABORT_ACK_SN_MISMATCH = 0b0101,
    BK_802154_RX_ABORT_ED_STOP_CMD     = 0b0110,
    BK_802154_RX_ABORT_RX_STOP_CMD     = 0b0111,
    BK_802154_RX_ABORT_HW_FILTER_FAIL  = 0b1000,
    BK_802154_RX_ABORT_PHY_NO_ED       = 0b1001,
} bk_802154_rx_abort_reason_t;

/**
 * @brief Rx status
 */
enum {
    THREAD_MAC_CURRENT_RX_STATUS_P_START,
    THREAD_MAC_CURRENT_RX_STATUS_P_CHK_RF_IDLE,
    THREAD_MAC_CURRENT_RX_STATUS_P_HOP_PROG,
    THREAD_MAC_CURRENT_RX_STATUS_P_WAIT_RF_READY,
    THREAD_MAC_CURRENT_RX_STATUS_P_SFD,
    THREAD_MAC_CURRENT_RX_STATUS_P_LENGTH,
    THREAD_MAC_CURRENT_RX_STATUS_P_PAYLOAD,
    THREAD_MAC_CURRENT_RX_STATUS_P_CRC1,
    THREAD_MAC_CURRENT_RX_STATUS_P_CRC2,
    THREAD_MAC_CURRENT_RX_STATUS_P_RX_DONE,
    THREAD_MAC_CURRENT_RX_STATUS_P_WAIT_ED_START,
    THREAD_MAC_CURRENT_RX_STATUS_P_ED_SCAN_ON,
};

/**
 * @brief Tx Status
 */
 enum {
    THREAD_MAC_CURRENT_TX_STATUS_P_START,
    THREAD_MAC_CURRENT_TX_STATUS_P_CHK_RF_IDLE,
    THREAD_MAC_CURRENT_TX_STATUS_P_HOP_PROG,
    THREAD_MAC_CURRENT_TX_STATUS_P_WAIT_RF_READY,
    THREAD_MAC_CURRENT_TX_STATUS_P_PREAMBLE_1,
    THREAD_MAC_CURRENT_TX_STATUS_P_PREAMBLE_2,
    THREAD_MAC_CURRENT_TX_STATUS_P_PREAMBLE_3,
    THREAD_MAC_CURRENT_TX_STATUS_P_SFD,
    THREAD_MAC_CURRENT_TX_STATUS_P_PHR,
    THREAD_MAC_CURRENT_TX_STATUS_P_PAYLOAD_ST,
    THREAD_MAC_CURRENT_TX_STATUS_P_CRC_1,
    THREAD_MAC_CURRENT_TX_STATUS_P_CRC_2,
 };
/**
 * @brief The data structure of tx/rx/ack frame.
 */
typedef struct bk802154_frame{
    uint8_t frame[BK_FRAME_LENGTH];
    uint8_t used;  //True:used and not available, False:available
    bool pending;
    bool enh_ack;
    int8_t rssi;
    uint8_t lqi;
    uint8_t channel; // current packet channel
    uint64_t time;
} bk_802154_frame_t;

/**
 * @brief The msg type
 */
typedef enum {
    BK_802154_EVENT_TX_DONE_NO_ACK = 0,
    BK_802154_EVENT_TX_DONE_WITH_ACK = 1,
    BK_802154_EVENT_RX_DONE = 2,
    BK_802154_EVENT_TX_FAIL = 3,
    BK_802154_EVENT_RX_FAIL = 4,
    BK_802154_EVENT_ED_DONE = 5,
    BK_802154_EVENT_MAX,
} bk_802154_event_type_t;


/**
 * @brief  Initialize the IEEE 802.15.4 subsystem.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 *
 */
bk_err_t bk_ieee802154_enable(void);

/**
 * @brief  Deinitialize the IEEE 802.15.4 subsystem.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_disable(void);

/**
 * @brief  Get the operational channel.
 *
 * @return The channel number (11~26).
 *
 */
uint8_t bk_ieee802154_channel_get(void);

/**
 * @brief  Set the operational channel.
 *
 * @param[in]  channel  The channel number (11-26).
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_channel_set(uint8_t channel);

/**
 * @brief  Get the transmit power.
 *
 * @return The transmit power in dBm.
 *
 */
int8_t bk_ieee802154_get_txpower(void);

/**
 * @brief  Set the transmit power.
 *
 * @param[in]  power  The transmit power in dBm.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_txpower(int8_t power);

/**
 * @brief  Get the promiscuous mode.
 *
 * @return
 *      - True   The promiscuous mode is enabled.
 *      - False  The promiscuous mode is disabled.
 *
 */
bool bk_ieee802154_get_promiscuous(void);

/**
 * @brief  Set the promiscuous mode.
 *
 * @param[in]  enable  The promiscuous mode to be set.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_promiscuous(bool enable);

/**
 * @brief  Get the IEEE 802.15.4 Radio state.
 *
 * @return  The IEEE 802.15.4 Radio state, refer to bk_ieee802154_state_t.
 *
 */
bk_802154_state_t bk_ieee802154_state_get(void);

/**
 * @brief  Set the IEEE 802.15.4 Radio to sleep state.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_sleep(void);

/**
 * @brief  Set the IEEE 802.15.4 Radio to receive state.
 *
 * @note Radio will continue receiving until it receives a valid frame.
 *       Refer to `bk_ieee802154_receive_done()`.
 *
 * @return
 *      - BK_OK on success
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_receive(void);

/**
 * @brief  Transmit the given frame.
 *         The transmit result will be reported via `bk_ieee802154_transmit_done()`
 *         or `bk_ieee802154_transmit_failed()`.
 *
 * @param[in]  frame  The pointer to the frame, the frame format:
 *                    |-----------------------------------------------------------------------|
 *                    | Len | MHR |              MAC Payload                          |  FCS  |
 *                    |-----------------------------------------------------------------------|
 * @param[in]  cca    Perform CCA before transmission if it's true, otherwise transmit the frame directly.
 *
 * @note During transmission, the hardware calculates the FCS, and send it over the air right after the MAC payload,
 *       so you just need to prepare the length, mac header and mac payload content.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_INVALID_ARG on an invalid frame.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_transmit(const uint8_t *frame, bool cca);

/**
 * @brief  Set the IEEE 802.15.4 Radio to receive state at a specific time.
 *
 * @note   Radio will start receiving after the timestamp, and continue receiving until it receives a valid frame.
 *         Refer to `bk_ieee802154_receive_done()`.
 *
 * @param[in]  time  A specific timestamp for starting receiving.
 * @return
 *      - BK_OK on success
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_receive_at(uint32_t time);

/**
 * @brief  Transmit the given frame at a specific time.
 *         The transmit result will be reported via `bk_ieee802154_transmit_done()`
 *         or `bk_ieee802154_transmit_failed()`.
 *
 * @param[in]  frame  The pointer to the frame. Refer to `bk_ieee802154_transmit()`.
 * @param[in]  cca    Perform CCA before transmission if it's true, otherwise transmit the frame directly.
 * @param[in]  time  A specific timestamp for starting transmission.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_INVALID_ARG on an invalid frame.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_transmit_at(const uint8_t *frame, bool cca, uint32_t time);

/**
 * @brief  Get the device PAN ID.
 *
 * @return  The device PAN ID.
 *
 */
uint16_t bk_ieee802154_get_panid(void);

/**
 * @brief  Set the device PAN ID.
 *
 * @param[in]  panid  The device PAN ID.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_panid(uint16_t panid);

/**
 * @brief  Get the device short address.
 *
 * @return  The device short address.
 *
 */
uint16_t bk_ieee802154_get_short_address(void);

/**
 * @brief  Set the device short address.
 *
 * @param[in]  short_address  The device short address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_short_address(uint16_t short_address);

/**
 * @brief  Get the device extended address.
 *
 * @param[out]  ext_addr  The pointer to the device extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_get_extended_address(uint8_t *ext_addr);

/**
 * @brief  Set the device extended address.
 *
 * @param[in]  ext_addr  The pointer to the device extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_extended_address(const uint8_t *ext_addr);

/**
 * @brief  Set the auto frame pending mode.
 *
 * @param[in]  pending_mode  The auto frame pending mode, refer to bk_ieee802154_pending_mode_t.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_pending_mode(bool pending_mode);

/**
 * @brief  Add address to the source matching table.
 *
 * @param[in]  addr      The pointer to the address.
 * @param[in]  is_short  Short address or Extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_NO_MEM if the pending table is full.
 *
 */
bk_err_t bk_ieee802154_add_pending_addr(const uint8_t *addr, bool is_short);

/**
 * @brief  Remove address from the source matching table.
 *
 * @param[in]  addr      The pointer to the address.
 * @param[in]  is_short  Short address or Extended address.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_ERR_NOT_FOUND if the address was not found from the source matching table.
 *
 */
bk_err_t bk_ieee802154_clear_pending_addr(const uint8_t *addr, bool is_short);

/**
 * @brief  Clear the source matching table to empty.
 *
 * @param[in]  is_short  Clear Short address table or Extended address table.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_reset_pending_table(bool is_short);

/**
 * @brief  Get the CCA threshold.
 *
 * @return  The CCA threshold in dBm.
 *
 */
int8_t bk_ieee802154_get_cca_threshold(void);

/**
 * @brief  Set the CCA threshold.
 *
 * @param[in]  cca_threshold  The CCA threshold in dBm.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_cca_threshold(int8_t cca_threshold);

/**
 * @brief  Enable rx_on_when_idle mode, radio will receive during idle.
 *
 * @param[in]  enable  Enable/Disable rx_on_when_idle mode.
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure.
 */
bk_err_t bk_ieee802154_set_rx_when_idle(bool enable);

/**
 * @brief  Get the rx_on_when_idle mode.
 *
 * @return  rx_on_when_idle mode.
 *
 */
bool bk_ieee802154_get_rx_when_idle(void);

/**
 * @brief  Perform energy detection.
 *
 * @param[in]  duration  The duration of energy detection, in symbol unit (16 us).
 *                       The result will be reported via bk_ieee802154_energy_detect_done().
 *
 * @return
 *      - BK_OK on success.
 *      - BK_FAIL on failure due to invalid state.
 *
 */
bk_err_t bk_ieee802154_energy_detect(uint32_t duration);

/**
 * @brief  The energy detection done. Refer to `bk_ieee802154_energy_detect()`.
 *
 * @param[in]  power  The detected power level, in dBm.
 *
 */
void bk_ieee802154_energy_detect_done(int8_t power);

/**
 * @brief  Get the RSSI of the most recent received frame.
 *
 * @return The value of RSSI.
 *
 */
int8_t bk_ieee802154_get_recent_rssi(void);

/**
 * @brief  Tx mode trxconfig add in 20251016.
 *
 * @return NULL.
 *
 */
void bk_ieee802154_tx_mode_trxconfig(void);

bk_802154_tx_err_t bk_ieee802154_get_tx_err();
bk_802154_frame_t* bk_ieee802154_get_rx_read_buffer();
bk_err_t bk_ieee802154_transmit_ack(const uint8_t *frame);
bk_802154_frame_t* bk_ieee802154_get_ack_buffer();
bk_err_t bk_ieee802154_receive_handle_done(const uint8_t *data);
bk_err_t bk_802154_enter_sleep(void);

#define MAC802154_TAG "MAC802154"
#define MAC802154_LOGI(...) BK_LOGI(MAC802154_TAG, ##__VA_ARGS__)
#define MAC802154_LOGW(...) BK_LOGW(MAC802154_TAG, ##__VA_ARGS__)
#define MAC802154_LOGE(...) BK_LOGE(MAC802154_TAG, ##__VA_ARGS__)
#define MAC802154_LOGD(...) BK_LOGD(MAC802154_TAG, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif


