#include "sdkconfig.h"
#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "ota_confirm.h"

#define TAG "ota"

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