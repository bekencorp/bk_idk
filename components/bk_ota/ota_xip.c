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
#include "cache.h"
#include "armstar.h"
#include "core_star.h"
#include "ota_verify.h"

#if (CONFIG_SUPPORT_MULTICORE)
#include "state.h"
#endif

extern void vPortEnableTimerInterrupt(void);
extern void vPortDisableTimerInterrupt(void);


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

__attribute__((section(".iram")))
bk_err_t xip_ota_write_cbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	OTA_LOGD(TAG, "xip write, off=%x len=%x\r\n", off, len);
	const bk_logic_partition_t *partition_a = xip_ota_get_a_partition(ota);
	BK_RETURN_ON_ERR(common_ota_partition_check(partition_a, off, buf, len));

	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_a->partition_start_addr));

	if((fa_off + off) & 0x1F) { // MUST align with 32-byte (check lower 5 bits)
		OTA_LOGE(TAG, "address not aligned to 32-byte boundary\r\n");
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

#if  !CONFIG_CACHE_ENABLE

	// SCB_CleanDCache_by_Addr((uint32_t *)write_addr, (int32_t)len);
	SCB_CleanDCache();
#endif

#if CONFIG_CACHE_ENABLE
	enable_dcache(1);
#endif
	rtos_enable_int(int_status);
	vPortEnableTimerInterrupt();
	return BK_OK;
}

bk_err_t xip_ota_read_dbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	const bk_logic_partition_t *partition_a = xip_ota_get_ota_partition(ota);
	uint32_t fa_addr = CEIL_ALIGN_34(partition_a->partition_start_addr);
	uint32_t addr = fa_addr + off;
	return bk_flash_read_bytes(addr, buf, len);
}


bk_err_t xip_ota_read_cbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	OTA_LOGD(TAG, "xip read, off=%x len=%x\r\n", off, len);
	const bk_logic_partition_t *partition_a = xip_ota_get_a_partition(ota);
	BK_RETURN_ON_ERR(common_ota_partition_check(partition_a, off, buf, len));

	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_a->partition_start_addr));

	if((fa_off + off) & 0x1F) { // MUST align with 32-byte (check lower 5 bits)
		OTA_LOGE(TAG, "address not aligned to 32-byte boundary\r\n");
		return BK_ERR_PARAM;
	}

	bk_flash_read_cbus(fa_off + off, buf, len);
	return 0;
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
	return ota_confirm(control, OTA_CONFIRM);
}

bk_err_t xip_ota_cancel(const bk_logic_partition_t *control_partition)
{
	OTA_LOGD(TAG, "xip cancel\r\n");
	if (control_partition == NULL) {
		return BK_FAIL;
	}

	uint32_t offset = ota_confirm_phy_offset(control_partition);
	uint32_t value = 0;
	bk_flash_read_bytes(offset, (uint8_t *)&value, 4);
	if (value == 0xFFFFFFFF) {
		return BK_OK;
	}

	uint8_t retry_cnt = 3;
	while (retry_cnt--) {
		flash_protect_type_t protect = bk_flash_get_protect_type();
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		bk_err_t ret = bk_flash_erase_sector(offset);
		bk_flash_set_protect_type(protect);
		if (ret != BK_OK) {
			continue;
		}
		bk_flash_read_bytes(offset, (uint8_t *)&value, 4);
		if (value == 0xFFFFFFFF) {
			return BK_OK;
		}
	}

	return BK_FAIL;
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

bk_err_t ota_verify_pubkey(const bk_logic_partition_t *app_partition,
    ota_verify_t *ota_verify, const uint8_t *ota_outer_pubkey, uint16_t ota_outer_pubkey_len)
{
	(void)ota_outer_pubkey;
	(void)ota_outer_pubkey_len;

	if (ota_verify == NULL || ota_verify->tlv_total == NULL) {
		return BK_ERR_PARAM;
	}

	uint8_t new_pubkey[MAX_PUBKEY_LEN];  // new_pubkey: new_img_sign_pubkey
	uint32_t new_pubkey_len = MAX_PUBKEY_LEN;
	uint8_t root_pubkey_tlv[MAX_PUBKEY_LEN];  // root_pubkey: img_sign_pubkey (root key)
	uint32_t root_pubkey_tlv_len = MAX_PUBKEY_LEN;
	uint8_t pubkey_sig[MAX_PUBKEY_LEN];  // pubkey_sig: signature of new_pubkey hash
	uint32_t pubkey_sig_len = MAX_PUBKEY_LEN;
	bk_err_t ret;

	// Read new_pubkey TLV from OTA data
	ret = ota_read_tlv_from_data(ota_verify->tlv_total, TLV_TOTAL_SIZE, IMAGE_TLV_CUSTOM, new_pubkey, &new_pubkey_len);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "missing required TLV: new_pubkey\r\n");
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	if (new_pubkey_len > MAX_PUBKEY_LEN) {
		BK_LOGE(TAG, "invalid new_pubkey TLV length\r\n");
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	// Try to read root_pubkey and pubkey_sig (new format)
	ret = ota_read_tlv_from_data(ota_verify->tlv_total, TLV_TOTAL_SIZE, IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY,
	                              root_pubkey_tlv, &root_pubkey_tlv_len);
	if (ret == BK_OK) {
		// Found root_pubkey, check for pubkey_sig
		ret = ota_read_tlv_from_data(ota_verify->tlv_total, TLV_TOTAL_SIZE, IMAGE_TLV_CUSTOM_PUBKEY_SIG,
		                              pubkey_sig, &pubkey_sig_len);
		if (ret == BK_OK) {
			BK_LOGI(TAG, "update pubkey");

			if (root_pubkey_tlv_len > MAX_PUBKEY_LEN) {
				BK_LOGE(TAG, "invalid root_pubkey TLV length\r\n");
				return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
			}

			// Step 1: Compare root_pubkey with primary partition root pubkey
			uint8_t root_pubkey_from_primary[MAX_PUBKEY_LEN];
			uint16_t root_pubkey_from_primary_len = MAX_PUBKEY_LEN;
			ret = ota_read_tlv_from_partition(app_partition, IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY,
			                                   root_pubkey_from_primary, &root_pubkey_from_primary_len);
			if (ret != BK_OK) {
				// Fallback: try to read new_pubkey as root pubkey (old format in primary)
				ret = ota_read_tlv_from_partition(app_partition, IMAGE_TLV_CUSTOM,
				                                   root_pubkey_from_primary, &root_pubkey_from_primary_len);
			}

			if (ret != BK_OK) {
				BK_LOGE(TAG, "failed to read root pubkey from primary, ret=0x%x\r\n", ret);
				return ret;
			}

			if (root_pubkey_tlv_len != root_pubkey_from_primary_len ||
			    memcmp(root_pubkey_tlv, root_pubkey_from_primary, root_pubkey_tlv_len) != 0) {
				BK_LOGE(TAG, "root pubkey mismatch\r\n");
				return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
			}

			// Step 2: Verify signature of new_pubkey using root pubkey
#if CONFIG_PSA_MBEDTLS
			// Calculate hash of new pubkey
			uint8_t new_pubkey_hash[32];
			mbedtls_sha256_context sha256_ctx;
			mbedtls_sha256_init(&sha256_ctx);
			mbedtls_sha256_starts(&sha256_ctx, 0);
			mbedtls_sha256_update(&sha256_ctx, new_pubkey, new_pubkey_len);
			mbedtls_sha256_finish(&sha256_ctx, new_pubkey_hash);
			mbedtls_sha256_free(&sha256_ctx);

			// Verify signature
			ret = ota_verify_signature(new_pubkey_hash, 32,
			                           root_pubkey_from_primary, root_pubkey_from_primary_len,
			                           pubkey_sig, pubkey_sig_len);
			if (ret != BK_OK) {
				return ret;
			}

#endif
			return BK_OK;
		}
	}

	uint8_t primary_new_pubkey[MAX_PUBKEY_LEN];
	uint16_t primary_new_pubkey_len = MAX_PUBKEY_LEN;
	ret = ota_read_tlv_from_partition(app_partition, IMAGE_TLV_CUSTOM,
	                                   primary_new_pubkey, &primary_new_pubkey_len);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "failed to read new_pubkey from primary, ret=0x%x\r\n", ret);
		return ret;
	}

	// Compare new_pubkey from OTA with new_pubkey from primary
	if (new_pubkey_len != primary_new_pubkey_len ||
	    memcmp(new_pubkey, primary_new_pubkey, new_pubkey_len) != 0) {
		BK_LOGE(TAG, "new_pubkey mismatch between OTA and primary\r\n");
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	return BK_OK;
}

bk_err_t xip_ota_verify(const bk_ota_t *ota, void* metadata)
{
	ota_verify_t* ota_verify = (ota_verify_t*)metadata;
	int ret = BK_OK;

	if (ota_verify != NULL) {
		const bk_logic_partition_t *app_partition = ota->app_partition;
		if (app_partition == NULL) {
			ota_verify_deinit(ota_verify);
			return BK_ERR_PARAM;
		}

		ret = ota_verify_image(app_partition, ota_verify);
		ota_verify_deinit(ota_verify);
		ota_verify = NULL;
	}
	return ret;
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

	uint32_t current_image_ok_value = 0;
	bk_flash_read_bytes(image_ok_offset, (uint8_t *)&current_image_ok_value, 4);
	if (current_image_ok_value != image_ok_value) {
		bk_flash_write_bytes(image_ok_offset, (const uint8_t *)&image_ok_value, 4);
	}

	// Save current running slot information for bootloader
	uint32_t running_slot_offset = CONFIG_XIP_OTA_CONTROL_PHY_PARTITION_OFFSET;
	uint32_t current_slot = flash_get_excute_enable(); // 0 = slot A, 1 = slot B

	uint32_t current_slot_value = 0;
	bk_flash_read_bytes(running_slot_offset, (uint8_t *)&current_slot_value, 4);

	if (current_slot == 0) {
		if (current_slot_value != 0xFFFFFFFF) {
			OTA_LOGI(TAG, "current slot is 0, erasing 4k at offset 0x%x\r\n", running_slot_offset);
			bk_flash_erase_sector(running_slot_offset);
		}
	} else {
		uint32_t running_slot_value = 1;
		if (current_slot_value != running_slot_value) {
			OTA_LOGI(TAG, "save running slot: %d, value: 0x%x\r\n", current_slot, running_slot_value);
			bk_flash_write_bytes(running_slot_offset, (const uint8_t *)&running_slot_value, 4);
		}
	}

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

void xip_bl2_ota_get_pack_addrs(uint32_t *bl2_address, uint32_t *partition_address)
{
	*bl2_address = CONFIG_XIP_A_VIRTUAL_CODE_SIZE +
	               FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_XIP_A_PHY_PARTITION_OFFSET)) +
	               0x1000 - 80 * 1024;
	*partition_address = CONFIG_XIP_A_VIRTUAL_CODE_SIZE +
	                     FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_XIP_A_PHY_PARTITION_OFFSET)) +
	                     0x1000 - 8 * 1024;
}
#endif
