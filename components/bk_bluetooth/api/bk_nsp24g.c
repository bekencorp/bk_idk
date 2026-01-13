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

#include "os/os.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include <components/log.h>
#include "nsp24g_internal.h"
#include <string.h>

#define TAG       "nsp24g"
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static uint8_t nsp24g_already_init;

ble_err_t bk_bluetooth_nsp24g_init(void)
{
    if ((bk_bluetooth_get_status() == BK_BLUETOOTH_STATUS_UNINITIALIZED) || nsp24g_already_init)
    {
        return -1;
    }

    ble_err_t ret = nsp24g_init();
    if (ret) {
        LOGE("%s failed status:0x%x\r\n", __func__,ret);
        return ret;
    }

    nsp24g_already_init = 1;

    return 0;
}

ble_err_t bk_bluetooth_nsp24g_deinit(void)
{
    if (!nsp24g_already_init) {
        return -1;
    }

    ble_err_t ret = nsp24g_deinit();
    if (ret) {
        LOGE("%s failed status:0x%x\r\n", __func__,ret);
        return ret;
    }

    nsp24g_already_init = 0;

    return 0;
}

ble_err_t bk_bluetooth_nsp24g_start(void)
{
    if (!nsp24g_already_init) {
        return -1;
    }

    bk_nsp24g_param_t param = {0};

    uint8_t addr[4] = {0x71, 0x72, 0x73, 0x74};
    uint8_t crc_init[3] = {0x55, 0x55, 0x55};

    memcpy(param.aa.addr, addr, 4);
    memcpy(param.crcinit.crc, crc_init, 3);

    ble_err_t ret;
    ret = nsp24g_start(&param);

    if (ret) {
        LOGE("%s failed status:0x%x\r\n", __func__,ret);
        return ret;
    }

    return 0;
}

ble_err_t bk_bluetooth_nsp24g_stop(void)
{
    if (!nsp24g_already_init) {
        return -1;
    }

    ble_err_t ret;
    ret = nsp24g_stop();

    if (ret) {
        LOGE("%s failed status:0x%x\r\n", __func__,ret);
        return ret;
    }

    return 0;
}

ble_err_t bk_bluetooth_nsp24g_send_frame(nsp24g_packet_t *tx_frame,uint16_t ack_timeout,ble_nsp24g_ack_cb_t ack_cb)
{
    if (!nsp24g_already_init) {
        return -1;
    }

    ble_err_t ret;
    ret = nsp24g_send_start(tx_frame,ack_timeout,ack_cb);

    if (ret) {
        LOGE("%s failed status:0x%x\r\n", __func__,ret);
        return ret;
    }

    return ret;
}

ble_err_t bk_bluetooth_nsp24g_rx_start(ble_nsp24g_rx_cb_t rx_cb)
{
    if (!nsp24g_already_init) {
        return -1;
    }

    ble_err_t ret;
    ret = nsp24g_rx_start(rx_cb);

    if (ret) {
        LOGE("%s failed status:0x%x\r\n", __func__,ret);
        return ret;
    }

    return 0;
}

ble_err_t bk_bluetooth_nsp24g_rx_stop(void)
{
    if (!nsp24g_already_init) {
        return -1;
    }

    ble_err_t ret;
    ret = nsp24g_rx_stop();

    if (ret) {
        LOGE("%s failed status:0x%x\r\n", __func__,ret);
        return ret;
    }

    return 0;
}
