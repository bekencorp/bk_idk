#include <components/log.h>
#include "bk_private/bk_init.h"
#if CONFIG_BLUETOOTH
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/bluetooth/bk_ble.h"
#include "ble_api_5_x.h"
#endif

#define TAG "example"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void){

}

extern int bk_cali_customer_init(void);

int main(void)
{
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7236)
	// STARTUP_PERF(14);
#endif
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
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
