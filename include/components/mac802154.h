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
 * @param enable Whether to enable the transmission, if enable is false, it will exit the transmission/continuous transmission mode.
 * @param channel The channel to transmit. channel range is 11~26. The channel should be the same as the channel used for receiving.
 * @param tx_cnt The number of times to transmit
 * @param tx_callback The callback function to be called when the transmission is complete
 *
 * @return
 *   - BK_OK: succeed
 *   - BK_ERR_PARAM: invalid channel
 */
bk_err_t mac802154_tx(bool enable, uint8_t channel, uint32_t tx_cnt, void *tx_callback);

/**
 * @brief  Receive a frame on the IEEE 802.15.4 radio
 *
 * This function is used to receive a frame on the IEEE 802.15.4 radio.
 *
 * @param enable Whether to enable the reception, if enable is false, it will exit the reception/continuous reception mode.
 * @param channel The channel to receive. channel range is 11~26. The channel should be the same as the channel used for transmission.
 *
 * @return
 *   - BK_OK: succeed
 *   - BK_ERR_PARAM: invalid channel
 */
bk_err_t mac802154_rx(bool enable, uint8_t channel);


#ifdef __cplusplus
}
#endif