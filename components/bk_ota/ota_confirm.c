#include "sdkconfig.h"
#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "ota_confirm.h"
#include "partitions_gen.h"

#define TAG "ota"

#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
#define OTA_CTRL_FAIL_BACKUP_LEN    (36)

#define OTA_CHECK_IS_ALL_FF(buf, start, end) \
	({ \
		int _ota_all_ff = 1; \
		int _ota_i; \
		for (_ota_i = (start); _ota_i < (end); _ota_i++) { \
			if ((buf)[_ota_i] != 0xFF) { \
				_ota_all_ff = 0; \
				break; \
			} \
		} \
		_ota_all_ff; \
	})

static bk_err_t ota_flash_backup_boot_fail_stage(uint32_t src_addr, uint8_t *data, uint32_t len)
{
	bk_err_t ret = BK_OK;
#if defined(CONFIG_PUBLIC_KEY_PHY_PARTITION_OFFSET) && defined(CONFIG_PUBLIC_KEY_PHY_PARTITION_SIZE)
	uint32_t dst_addr = CONFIG_PUBLIC_KEY_PHY_PARTITION_OFFSET + CONFIG_PUBLIC_KEY_PHY_PARTITION_SIZE - len;

	BK_LOGD(TAG, "backup boot fail stage: 0x%x bytes from 0x%x to 0x%x\r\n", len, src_addr, dst_addr);
	flash_protect_type_t protect = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	ret = bk_flash_write_bytes(dst_addr, data, len);
	bk_flash_set_protect_type(protect);
#endif
	return ret;

}

bk_err_t ota_control_backup_fail_stage_if_dirty(const bk_logic_partition_t *control_partition)
{
	if (control_partition == NULL) {
		return BK_FAIL;
	}

	if (control_partition->partition_length < OTA_CTRL_FAIL_BACKUP_LEN) {
		return BK_OK;
	}

	uint32_t ota_confirm_offset = ota_confirm_phy_offset(control_partition);
	uint32_t ota_fail_flag_offset = ota_confirm_offset - 32;
	uint8_t check_buf[OTA_CTRL_FAIL_BACKUP_LEN] = {0};

	bk_flash_read_bytes(ota_fail_flag_offset, check_buf, OTA_CTRL_FAIL_BACKUP_LEN);

	if (!OTA_CHECK_IS_ALL_FF(check_buf, 0, OTA_CTRL_FAIL_BACKUP_LEN)) {
		return ota_flash_backup_boot_fail_stage(ota_fail_flag_offset, check_buf, OTA_CTRL_FAIL_BACKUP_LEN);
	}

	return BK_OK;
}
#endif /* CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE */

uint32_t ota_confirm_phy_offset(const bk_logic_partition_t *partition)
{
	if (partition == NULL) {
		return 0;
	}
	return OTA_CONFIRM_PHY_OFFSET(partition->partition_start_addr, partition->partition_length);
}

bk_err_t ota_confirm(const bk_logic_partition_t *partition, uint32_t status)
{
	uint8_t retry_cnt = 3;
	uint32_t check_value = 0;

	if (partition == NULL) {
		BK_LOGE(TAG, "ota_confirm: partition is NULL\r\n");
		return BK_FAIL;
	}

	uint32_t offset = ota_confirm_phy_offset(partition);
	BK_LOGD(TAG, "ota_confirm: partition=%s, addr=0x%x, status=0x%x\r\n",
		partition->partition_description ? partition->partition_description : "NULL",
		offset, status);

	while (retry_cnt--) {
		bk_flash_read_bytes(offset, (uint8_t*)&check_value, 4);
		BK_LOGD(TAG, "ota_confirm: read from 0x%x, value=0x%x, expected=0x%x\r\n", 
			offset, check_value, status);
		if (check_value == status) {
			BK_LOGD(TAG, "ota_confirm: already confirmed, return OK\r\n");
			return BK_OK;
		}

		BK_LOGD(TAG, "ota_confirm: writing status 0x%x to 0x%x\r\n", status, offset);
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		bk_flash_erase_sector(offset);
		bk_flash_write_bytes(offset, (uint8_t*)&status, 4);
		
		/* Verify write */
		uint32_t verify_value = 0;
		bk_flash_read_bytes(offset, (uint8_t*)&verify_value, 4);
		BK_LOGD(TAG, "ota_confirm: verify read from 0x%x, value=0x%x\r\n", offset, verify_value);
		if (verify_value == status) {
			bk_flash_set_protect_type(FLASH_PROTECT_ALL);
			BK_LOGD(TAG, "ota_confirm: write and verify success\r\n");
			return BK_OK;
		}
		bk_flash_set_protect_type(FLASH_PROTECT_ALL);
		BK_LOGW(TAG, "ota_confirm: verify failed, retry=%d\r\n", retry_cnt);
	}

	BK_LOGE(TAG, "write image %s ota confirm failed, value in flash %#x\r\n", 
		partition->partition_description ? partition->partition_description : "NULL", check_value);
	return BK_FAIL;
}

bk_err_t ota_cancel(const bk_logic_partition_t *partition)
{
	return BK_OK;
}