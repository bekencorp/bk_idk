#include <components/log.h>
#include "bk_private/bk_init.h"
#if CONFIG_BLUETOOTH
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/bluetooth/bk_ble.h"
#include "ble_api_5_x.h"
#endif

#define TAG "example"

extern int bk_cali_customer_init(void);

int main(void)
{
	//BK_LOGI(TAG, "OTA successfully\r\n");
	bk_init();

#if CONFIG_CUSTOMER_INIT_CALI
	bk_cali_customer_init();
#endif
#if CONFIG_BLUETOOTH && CONFIG_CUSTOMER_INIT_BLE
	bk_bluetooth_init();
#endif
	return 0;
}
