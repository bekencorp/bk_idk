#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <os/os.h>
#include <os/mem.h>

#include "ble_boarding.h"

#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#define TAG "prov"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define ADV_MAX_SIZE (31)
#define ADV_NAME_HEAD "BKPROV"

#define ADV_TYPE_FLAGS                      (0x01)
#define ADV_TYPE_LOCAL_NAME                 (0x09)
#define ADV_TYPE_SERVICE_UUIDS_16BIT        (0x14)
#define ADV_TYPE_SERVICE_DATA               (0x16)
#define ADV_TYPE_MANUFACTURER_SPECIFIC      (0xFF)

#define BEKEN_COMPANY_ID                    (0x05F0)

#define PROV_UUID                       (0xFA00)

static ble_boarding_info_t wifi_prov_info = {0};

void wifi_prov_over_ble_init(void)
{
    uint8_t adv_data[ADV_MAX_SIZE] = {0};
    uint8_t adv_index = 0;
    uint8_t len_index = 0;
    uint8_t mac[6];
    int ret;


    LOGI("%s\n", __func__);

    /* flags */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_FLAGS;
    adv_data[adv_index++] = 0x06;
    adv_data[len_index] = 2;

    /* local name */
    bk_bluetooth_get_address(mac);

    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_LOCAL_NAME;

    ret = sprintf((char *)&adv_data[adv_index], "%s_%02X%02X%02X",
                  ADV_NAME_HEAD, mac[2], mac[1], mac[0]);

    LOGI("Provisioning started with device name:%s\n", (char *)&adv_data[adv_index]);

    adv_index += ret;
    adv_data[len_index] = ret + 1;

    /* 16bit uuid */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_SERVICE_DATA;
    adv_data[adv_index++] = PROV_UUID & 0xFF;
    adv_data[adv_index++] = PROV_UUID >> 8;
    adv_data[len_index] = 3;

    /* manufacturer */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_MANUFACTURER_SPECIFIC;
    adv_data[adv_index++] = BEKEN_COMPANY_ID & 0xFF;
    adv_data[adv_index++] = BEKEN_COMPANY_ID >> 8;
    adv_data[len_index] = 3;

    os_memset(&wifi_prov_info, 0, sizeof(ble_boarding_info_t));

    ble_boarding_init(&wifi_prov_info);

    if (adv_index > ADV_MAX_SIZE)
    {
        LOGE("%s, adv len(%d) exceeds the limit(%d)\n", __func__,adv_index,ADV_MAX_SIZE);
    }
    else
    {
        ble_boarding_adv_start(adv_data, adv_index);
    }
}

