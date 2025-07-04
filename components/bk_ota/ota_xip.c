#include <common/bk_include.h>
#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include "ota_common.h"
#include "ota_confirm.h"
#include "partitions_gen.h"
#include <components/bluetooth/bk_dm_bluetooth.h>
#include "aon_pmu_hal.h"
#include "driver/wdt.h"

#if (CONFIG_SUPPORT_MULTICORE)
#include "state.h"
#endif

#define TAG "ota"

#if CONFIG_DIRECT_XIP
static const bk_logic_partition_t *xip_ota_get_a_partition(const bk_ota_t *ota)
{
	return ota->app_partition;
}

static const bk_logic_partition_t *xip_ota_get_ota_partition(const bk_ota_t *ota)
{
	if (flash_get_excute_enable()) {//APP is partition B
		return ota->app_partition;
	} else {
		return ota->ota_partition;
	}
}

static const bk_logic_partition_t *xip_ota_get_app_partition(const bk_ota_t *ota)
{
	if (flash_get_excute_enable()) {//APP is partition B
		return ota->ota_partition;
	} else {
		return ota->app_partition;
	}
}

bk_err_t xip_ota_get_info(const bk_ota_t *ota, uint32_t* pad_size, uint32_t* length)
{
	const bk_logic_partition_t *partition_a = xip_ota_get_ota_partition(ota);
	*pad_size = CEIL_ALIGN_34(partition_a->partition_start_addr) - partition_a->partition_start_addr;
	*length = partition_a->partition_length;
	return BK_OK;
}

__attribute__((section(".iram")))
bk_err_t xip_ota_write_dbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	const bk_logic_partition_t *partition_a = xip_ota_get_ota_partition(ota);
	uint32_t fa_addr = CEIL_ALIGN_34(partition_a->partition_start_addr);
	uint32_t addr = fa_addr + off;
	return bk_flash_write_bytes(addr, buf, len);
}

#if CONFIG_XIP_OTA_WRITE_CBUS
__attribute__((section(".iram")))
bk_err_t xip_ota_write_cbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	OTA_LOGD(TAG, "xip write, off=%x len=%x\r\n", off, len);
	const bk_logic_partition_t *partition_a = xip_ota_get_a_partition(ota);
	BK_RETURN_ON_ERR(common_ota_partition_check(partition_a, off, buf, len));

	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_a->partition_start_addr));

	if((fa_off + off) & 0x31) { // MUST aligh with 32-byte.
		OTA_LOGE(TAG, "align\r\n");
		return BK_ERR_PARAM;
	}
	vPortDisableTimerInterrupt();
	uint32_t int_status =  rtos_disable_int();

#if CONFIG_CACHE_ENABLE
	enable_dcache(0);
#endif

	uint32_t write_addr = fa_off + off;
	write_addr |= 1 << 24;
	bk_flash_write_cbus(write_addr, buf, len);

#if CONFIG_CACHE_ENABLE
	enable_dcache(1);
#endif
	rtos_enable_int(int_status);
	vPortEnableTimerInterrupt();
	return BK_OK;
}
#endif

bk_err_t xip_ota_read_dbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	const bk_logic_partition_t *partition_a = xip_ota_get_ota_partition(ota);
	uint32_t fa_addr = CEIL_ALIGN_34(partition_a->partition_start_addr);
	uint32_t addr = fa_addr + off;
	return bk_flash_read_bytes(addr, buf, len);
}


bk_err_t xip_ota_read_cbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	return BK_ERR_NOT_SUPPORT;
}

bk_err_t xip_ota_erase_sector(const bk_ota_t *ota, uint32_t off)
{
	OTA_LOGD(TAG, "xip erase sector, off=%#x\r\n",off);
	const bk_logic_partition_t *ota_partition = xip_ota_get_ota_partition(ota);
	return bk_flash_erase_sector(ota_partition->partition_start_addr + off);
}

bk_err_t xip_ota_erase(const bk_ota_t *ota)
{
	OTA_LOGD(TAG, "xip erase\r\n");
	const bk_logic_partition_t *ota_partition = xip_ota_get_ota_partition(ota);

#if CONFIG_INT_WDT
	bk_wdt_stop();
#endif

	int ret = bk_flash_erase_fast(ota_partition->partition_start_addr, ota->ota_partition->partition_length);

#if CONFIG_INT_WDT
	bk_wdt_start(CONFIG_INT_WDT_PERIOD_MS);
#endif

	return ret;
}

bk_err_t xip_ota_confirm(const bk_logic_partition_t *control)
{
	OTA_LOGD(TAG, "xip confirm\r\n");
	return ota_confirm(control, OVERWRITE_CONFIRM);
}

static bool xip_ota_partition_is_valid(const bk_logic_partition_t *ota)
{
	uint8_t ff_buf[64];
	uint8_t read_buf[64];

	os_memset(ff_buf, 0xFF, 64);
	os_memset(read_buf, 0x0, 64);

	bk_flash_read_bytes(ota->partition_start_addr, read_buf, 64);

	if (os_memcmp(ff_buf, read_buf, 64) == 0) {
		return false;
	} else {
		return true;
	}
}

bk_err_t xip_ota_verify(const bk_ota_t *ota)
{
	#if 0
	const bk_logic_partition_t *ota_partition = xip_ota_get_ota_partition(ota);

	OTA_LOGD(TAG, "xip verify\r\n");
	if (xip_ota_partition_is_valid(ota_partition)) {
		return bk_flash_check_crc(ota_partition->partition_start_addr, ota_partition->partition_length);
	} else {
		OTA_LOGD(TAG, "invalid\r\n");
		return BK_FAIL;
	}
	#endif
	return BK_OK;
}

static uint32_t xip_ota_get_app_magic_off(const bk_ota_t *ota)
{
	const bk_logic_partition_t *app_partition = xip_ota_get_app_partition(ota);
	uint32_t vir_offset = 0;
	uint32_t phy_offset = 0;

	vir_offset = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(app_partition->partition_start_addr)) +
		(FLASH_PHY2VIRTUAL(app_partition->partition_length) & (~(4096 - 1))) - 48;
	phy_offset = FLASH_VIRTUAL2PHY(vir_offset);

	return phy_offset;
}

bk_err_t xip_ota_accept(const bk_ota_t *ota)
{
	OTA_LOGD(TAG, "xip accept\r\n");
	uint32_t image_ok_offset =  xip_ota_get_app_magic_off(ota) - (XIP_IMAGE_OK_TYPE - 1) * 8;
	uint32_t image_ok_value = XIP_IMAGE_OK;

	bk_flash_write_bytes(image_ok_offset, (const uint8_t *)&image_ok_value, 4);
	return BK_OK;
}

static void xip_ota_reboot_prepare(void)
{
	bk_wifi_sta_stop();
	bk_wifi_ap_stop();
	bk_bluetooth_deinit();
	rtos_delay_milliseconds(100);
}

static void xip_ota_set_reboot_status(void)
{
	rtos_disable_int(); /* disable interrupt, and scheduling is close*/

	#if CONFIG_SPINLOCK
	void bk_gpio_spinlock_try_unlock(void);
	void bk_sys_spinlock_try_unlock(void);
	void bk_flash_spinlock_try_unlock(void);

	bk_gpio_spinlock_try_unlock();
	bk_sys_spinlock_try_unlock();
	bk_flash_spinlock_try_unlock();
	#endif

	bk_misc_set_reset_reason(RESET_SOURCE_REBOOT);
	#if (CONFIG_SUPPORT_MULTICORE)
	bk_core_set_system_status(SYS_OTA_REBOOT);
	#endif

	while(7236){
		; /*waiting for reboot of system/cpu*/
	}
}

bk_err_t xip_ota_reboot(void)
{
	OTA_LOGD(TAG, "xip reboot\r\n");
	xip_ota_reboot_prepare();
	// aon_pmu_hal_set_xip_ota_finish(1);
	xip_ota_set_reboot_status();
	return BK_OK;
}
#endif
