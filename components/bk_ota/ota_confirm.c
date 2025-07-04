#include "sdkconfig.h"
#include "driver/flash.h"
#include "driver/flash_partition.h"

#define TAG "ota"

bk_err_t ota_confirm(const bk_logic_partition_t *partition, uint32_t status)
{
	uint8_t retry_cnt = 3;
	uint32_t check_value = 0;

	while (retry_cnt--) {
		uint32_t offset = partition->partition_start_addr;

		bk_flash_read_bytes(offset, (uint8_t*)&check_value, 4);
		if (check_value == status) {
			return BK_OK;
		}

		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		bk_flash_erase_sector(offset);
		bk_flash_write_bytes(offset, (uint8_t*)&status, 4);
		bk_flash_set_protect_type(FLASH_PROTECT_ALL);
	}

	BK_LOGE(TAG, "write image %s ota confirm failed, value in flash %#x\r\n", partition->partition_description, check_value);
	return BK_FAIL;
}

bk_err_t ota_cancel(const bk_logic_partition_t *partition)
{
	return BK_OK;
}