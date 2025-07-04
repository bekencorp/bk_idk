#include "sdkconfig.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include "ota_common.h"
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
	return ota_confirm(control_partition, OVERWRITE_CONFIRM);
}

bk_err_t ow_ota_cancel(const bk_logic_partition_t *control_partition)
{
	OTA_LOGD(TAG, "ow cancel\r\n");
	return bk_flash_erase_fast(control_partition->partition_start_addr, control_partition->partition_length);
}

bk_err_t ow_ota_reboot(void)
{
	OTA_LOGD(TAG, "ow reboot\r\n");
	bk_reboot();
	return BK_OK;
}
#endif
