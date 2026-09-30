// Copyright 2020-2021 Beken
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

#ifdef __cplusplus
extern"C" {
#endif

#include <stdbool.h>
#include "mac802154_types.h"

bk_err_t mac802154_init(void);

/**
 * @brief  Transmit a frame on the IEEE 802.15.4 radio
 *
 * This function is used to transmit a frame on the IEEE 802.15.4 radio.
 *
 * @param enable Whether to enable the transmission, 
 *               if enable is false, it will exit the transmission/continuous transmission mode.
 * @param channel The channel to transmit. channel range is 11~26. 
 *                The channel should be the same as the channel used for rx channel of DUT.
 * @param tx_cnt The number of packets to transmit
 * @param pass_cnt_thre The number of packets to pass the test
 *
 * @return
 *   - BK_OK: TX started or stopped successfully; for finite TX, all packets completed before timeout
 *   - BK_ERR_PARAM: invalid channel, valid range is 11~26
 *   - BK_ERR_BUSY: another TX/RX/DUT test is already running, stop it before starting TX
 *   - BK_ERR_TIMEOUT: finite TX did not complete within the expected wait time
 */
bk_err_t mac802154_tx(bool enable, uint8_t channel, uint8_t tx_cnt, uint8_t pass_cnt_thre);

/**
 * @brief  Receive a frame on the IEEE 802.15.4 radio
 *
 * This function is used to receive a frame on the IEEE 802.15.4 radio.
 *
 * @param enable Whether to enable the reception, 
 *               if enable is false, it will exit the reception/continuous reception mode.
 * @param channel The channel to receive. channel range is 11~26. 
 *                The channel should be the same as the channel used for tx channel of DUT.
 *
 * @return
 *   - BK_OK: RX started or stopped successfully
 *   - BK_ERR_PARAM: invalid channel, valid range is 11~26
 *   - BK_ERR_BUSY: another TX/RX/DUT test is already running, stop it before starting RX
 */
bk_err_t mac802154_rx(bool enable, uint8_t channel);

/**
 * @brief  Enter the DUT mode
 *
 * This function is used to enter the DUT mode.
 *
 * @param enable Whether to enable the DUT mode, if enable is false, it will exit the DUT mode.
 * @param dut_mode The DUT mode to use. dut_mode range is 1~2.
 *                MAC802154_DUT_MODE_RX_ONLY: 1, RX only mode
 *                MAC802154_DUT_MODE_RX_AND_TX: 2, RX and TX mode
 * @param rx_channel The channel to use for the DUT mode. channel range is 11~26.
 *                The channel should be the same as the channel used for transmission.
 * @param tx_channel The channel to use for the DUT mode. channel range is 11~26.
 *                The channel should be the same as the channel used for reception.
 * @param dut_cb The callback function to be called when the DUT mode receives the packet number 
 *                     reaches the pass count threshold or the test is completed.
 *
 * @return
 *   - BK_OK: DUT mode started or stopped successfully
 *   - BK_ERR_PARAM: invalid rx_channel or tx_channel, valid range is 11~26
 *   - BK_ERR_BUSY: another TX/RX/DUT test is already running, stop it before starting DUT
 *   - BK_ERR_TRY_AGAIN: DUT start failed due to internal resource or task creation failure, try again later
 */
bk_err_t mac802154_dut(bool enable, uint8_t dut_mode, uint8_t rx_channel, uint8_t tx_channel, void *dut_cb);

#ifdef __cplusplus
}
#endif