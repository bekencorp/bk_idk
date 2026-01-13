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

#include "components/bluetooth/bk_ble_types.h"

int nsp24g_init(void);
int nsp24g_deinit(void);
int nsp24g_start(bk_nsp24g_param_t *nsp24g_info);
int nsp24g_stop(void);
int nsp24g_rx_start(ble_nsp24g_rx_cb_t rx_cb);
int nsp24g_rx_stop(void);
int nsp24g_send_start(nsp24g_packet_t *tx_fame,uint16_t ack_timeout,ble_nsp24g_ack_cb_t ack_cb);

