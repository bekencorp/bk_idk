#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"

#define TAG "ota"

bk_err_t common_ota_partition_check(const bk_logic_partition_t *partition, uint32_t off, const uint8_t *buf, uint32_t len)
{
	if (!buf) {
		return BK_ERR_PARAM;
	}

	if (off > partition->partition_length) {
		return BK_ERR_PARAM;
	}

	if ((off + len) > partition->partition_length) {
		return BK_ERR_PARAM;
	}

	return BK_OK;
}

bk_err_t common_ota_confirm(const bk_logic_partition_t *partition, uint32_t status)
{
	uint8_t retry_cnt = 3;

	while (retry_cnt--) {
		uint32_t offset = partition->partition_start_addr;
		uint32_t check_value = 0;

		bk_flash_read_bytes(offset, (uint8_t*)&check_value, 4);
		if (check_value == status) {
			return BK_OK;
		}

		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		bk_flash_erase_sector(offset);
		bk_flash_write_bytes(offset, (uint8_t*)&status, 4);
		bk_flash_set_protect_type(FLASH_PROTECT_ALL);
	}

	BK_LOGE(TAG, "write image%d ota confirm=%x failed\r\n", index, status);
	return BK_FAIL;
}
