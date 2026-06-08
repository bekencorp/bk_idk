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
#include <os/mem.h>
#include <os/os.h>
#include "mac802154_adapter.h"

extern struct mac802154_osi_funcs_t g_mac802154_os_funcs;

static uint8_t mac802154_is_inited = 0;

bk_err_t mac802154_init(void)
{
    if (mac802154_is_inited) {
        return BK_FAIL;
    }

    mac802154_osi_init((void *)&g_mac802154_os_funcs);
    mac802154_mac_init();

    mac802154_is_inited = 1;

    return BK_OK;
}

void mac802154_deinit(void)
{
    //TODO
}


bk_err_t mac802154_tx(bool enable, uint8_t channel, uint32_t tx_cnt, void *tx_callback)
{
    if (enable && ((channel < 11) || (channel > 26)))
    {
        return BK_ERR_PARAM;
    }

    mac802154_mac_tx(enable, channel, tx_cnt, tx_callback);

    return BK_OK;
}

bk_err_t mac802154_rx(bool enable, uint8_t channel)
{
    if (enable && ((channel < 11) || (channel > 26)))
    {
        return BK_ERR_PARAM;
    }

    mac802154_mac_rx(enable, channel);

    return BK_OK;
}


