#include "sdkconfig.h"
#include <string.h>
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include "ota_common.h"
#include "ota_verify.h"
#include "ota_confirm.h"
#include "partitions_gen.h"

#define TAG "ota"

#if CONFIG_OTA_OVERWRITE
bk_err_t ow_ota_get_info(const bk_ota_t *ota, uint32_t *pad_offset, uint32_t *length)
{
	*pad_offset = 0;
	*length = ota->ota_partition->partition_length;
	return BK_OK;
}

bk_err_t ow_ota_write_dbus(const bk_ota_t *ota, uint32_t off, const uint8_t *buf, uint32_t len)
{
	OTA_LOGD(TAG, "ow write, off=%x len=%x\r\n", off, len);
	BK_RETURN_ON_ERR(common_ota_partition_check(ota->ota_partition, off, buf, len));
	return bk_flash_write_bytes(ota->ota_partition->partition_start_addr + off, buf, len);
}

bk_err_t ow_ota_read_dbus(const bk_ota_t *ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_ERR(common_ota_partition_check(ota->ota_partition, off, buf, len));
	return bk_flash_read_bytes(ota->ota_partition->partition_start_addr + off, buf, len);
}

bk_err_t ow_ota_erase(const bk_ota_t *ota)
{
	OTA_LOGD(TAG, "ow erase\r\n");
	return bk_flash_erase_fast(ota->ota_partition->partition_start_addr, ota->ota_partition->partition_length);
}

bk_err_t ow_ota_erase_sector(const bk_ota_t *ota, uint32_t off)
{
	OTA_LOGD(TAG, "ow sector erase\r\n");
	return bk_flash_erase_sector(ota->ota_partition->partition_start_addr + off);
}

bk_err_t ow_ota_confirm(const bk_logic_partition_t *control_partition)
{
	OTA_LOGD(TAG, "ow confirm\r\n");
	return ota_confirm(control_partition, OTA_CONFIRM);
}

bk_err_t ow_ota_cancel(const bk_logic_partition_t *control_partition)
{
	OTA_LOGD(TAG, "ow cancel\r\n");
	return bk_flash_erase_fast(control_partition->partition_start_addr, control_partition->partition_length);
}

bk_err_t ow_ota_accept(const bk_ota_t *ota)
{
	OTA_LOGD(TAG, "ow accept, erase confirm\r\n");
	if (ota == NULL || ota->control_partition == NULL) {
		return BK_FAIL;
	}
#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
	(void)ota_control_backup_fail_stage_if_dirty(ota->control_partition);
#endif
	uint32_t offset = ota_confirm_phy_offset(ota->control_partition);
	uint32_t value = 0;
	bk_flash_read_bytes(offset, (uint8_t*)&value, 4);
	if (value == 0xFFFFFFFF) {
		return BK_OK;
	}
	uint8_t retry_cnt = 3;
	while (retry_cnt--) {
		/* Save and restore original protect status, only disable during erase. */
		flash_protect_type_t protect = bk_flash_get_protect_type();
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		bk_err_t ret = bk_flash_erase_sector(offset);
		bk_flash_set_protect_type(protect);
		if (ret != BK_OK) {
			continue;
		}

		bk_flash_read_bytes(offset, (uint8_t*)&value, 4);
		if (value == 0xFFFFFFFF) {
			return BK_OK;
		}
	}

	return BK_FAIL;
}

bk_err_t ow_ota_verify(const bk_ota_t *ota, void* metadata)
{
	(void)ota;
	(void)metadata;
	return BK_OK;
}

bk_err_t ow_ota_reboot(void)
{
	OTA_LOGD(TAG, "ow reboot\r\n");
	bk_reboot();
	return BK_OK;
}

void ow_bl2_ota_get_pack_addrs(uint32_t *bl2_address, uint32_t *partition_address)
{
	*bl2_address = CONFIG_OVERWRITE_VIRTUAL_CODE_SIZE +
	              FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_OVERWRITE_PHY_PARTITION_OFFSET)) +
	              0x1000 - 80 * 1024;
	*partition_address = CONFIG_OVERWRITE_VIRTUAL_CODE_SIZE +
	                    FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_OVERWRITE_PHY_PARTITION_OFFSET)) +
	                    0x1000 - 8 * 1024;
}

#if CONFIG_OTA_UPDATE_PUBKEY
bk_err_t ota_verify_pubkey(const bk_logic_partition_t *app_partition,
    ota_verify_t *ota_verify, const uint8_t *ota_outer_pubkey, uint16_t ota_outer_pubkey_len)
{
	(void)ota_verify;
	uint8_t primary_pubkey[MAX_PUBKEY_LEN];

	if (ota_outer_pubkey_len > MAX_PUBKEY_LEN) {
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	bk_err_t ret = read_pubkey_from_primary(app_partition, primary_pubkey, NULL);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "primary read public key fail!\r\n");
		return ret;
	}

	if (memcmp(primary_pubkey, ota_outer_pubkey, ota_outer_pubkey_len) != 0) {
		BK_LOGE(TAG, "public key of OTA-outer and primary not equal!\r\n");
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}
	return BK_OK;
}
#endif
#endif
