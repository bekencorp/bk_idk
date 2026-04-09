#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include "ota_ow.h"
#include "ota_xip.h"
#include "ota_bl2.h"
#include "ota_common.h"
#if CONFIG_OTA_BACKUP_PUBKEY
#include "ota_verify.h"
#endif
#include "partitions_gen.h"
#include "aon_pmu_hal.h"

#define TAG "ota"

static bk_ota_t* s_xip_ota_handle = NULL;
static bk_ota_t* s_ow_ota_handle = NULL;

static bk_ota_t* bk_ota_init(const char* app_partition_name, const char* ota_partition_name, const char* control_partition_name)
{
	bk_ota_t *ota = (bk_ota_t*)os_malloc(sizeof(bk_ota_t));

	OTA_LOGD(TAG, "init %s %s %s\r\n", app_partition_name, ota_partition_name, control_partition_name);

	if (!ota) {
		OTA_LOGE(TAG, "init oom\r\n");
		return NULL;
	}

	os_memset(ota, 0, sizeof(*ota));
	ota->app_partition = bk_flash_partition_get_info_by_name(app_partition_name);
	ota->ota_partition = bk_flash_partition_get_info_by_name(ota_partition_name);
	ota->control_partition = bk_flash_partition_get_info_by_name(control_partition_name);
#if CONFIG_OTA_UPDATE_PUBKEY
	ota->key_partition = bk_flash_partition_get_info_by_name("public_key");
#endif

	if ((!ota->app_partition) || (!ota->ota_partition) || (!ota->control_partition)) {
		OTA_LOGD(TAG, "invalid ota partition: app=%s ota=%s control=%s\r\n", 
			app_partition_name, ota_partition_name, control_partition_name);
		os_free(ota);
		return NULL;
	}

	uint32_t partition_type = bk_flash_partition_get_type(ota->control_partition);
	uint32_t partition_options = bk_flash_partition_get_type(ota->control_partition);

	//TODO auto generate macro in partitions_gen.h to save code size!
	if (partition_type == PARTITION_SUBTYPE_OW_OTA_CONTROL) {
#if CONFIG_OTA_OVERWRITE
		OTA_LOGD(TAG, "init ow\r\n");
		if (partition_options & PARTITION_FLAGS_OTA_WRITE_CBUS) {
		} else {
			ota->ops.info = ow_ota_get_info;
			ota->ops.write = ow_ota_write_dbus;
		}

		if (partition_options & PARTITION_FLAGS_OTA_READ_CBUS) {
		} else {
			ota->ops.read = ow_ota_read_dbus;
		}

		ota->ops.erase = ow_ota_erase;
		ota->ops.erase_sector = ow_ota_erase_sector;
		ota->ops.confirm = ow_ota_confirm;
		ota->ops.verify = ow_ota_verify;
		ota->ops.cancel = ow_ota_cancel;
		ota->ops.accept = ow_ota_accept;
		ota->ops.reboot = ow_ota_reboot;
#endif
	} else if (partition_type == PARTITION_SUBTYPE_XIP_OTA_CONTROL) {
#if CONFIG_DIRECT_XIP
		OTA_LOGD(TAG, "init xip\r\n");
		if (partition_options & PARTITION_FLAGS_OTA_WRITE_CBUS) {
		} else {
			ota->ops.info = xip_ota_get_info;
			ota->ops.write = xip_ota_write_cbus;
		}

		if (partition_options & PARTITION_FLAGS_OTA_READ_CBUS) {
			ota->ops.read = xip_ota_read_cbus;
		} else {
			ota->ops.read = xip_ota_read_dbus;
		}

		ota->ops.erase = xip_ota_erase;
		ota->ops.erase_sector = xip_ota_erase_sector;
		ota->ops.confirm = xip_ota_confirm;
		ota->ops.cancel = xip_ota_cancel;
		ota->ops.verify = xip_ota_verify;
		ota->ops.reboot = xip_ota_reboot;
		ota->ops.accept = xip_ota_accept;
#endif
	} else {
		OTA_LOGE(TAG, "unknown partition_type=%d\r\n", partition_type);
		return NULL;
	}

#if CONFIG_OTA_BACKUP_PUBKEY
	ota->ops.updatekey = ota_update_back_pubkey_from_primary;
#endif
	return ota;
}

static bk_err_t bk_ota_deinit(bk_ota_t* ota)
{
	OTA_LOGD(TAG, "deinit\r\n");
	if (ota) {
		os_free(ota);
	}
	return BK_OK;
}

bk_err_t bk_ota_get_info(bk_ota_t* ota, uint32_t *pad_size, uint32_t *length)
{
	if (ota && ota->ops.info) {
		return ota->ops.info(ota, pad_size, length);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_write(bk_ota_t* ota, uint32_t off, const uint8_t *buf, uint32_t len)
{
	OTA_LOGD(TAG, "write, off=%x len=%x\r\n", off, len);
	if (ota && ota->ops.write) {
		return ota->ops.write(ota, off, buf, len);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_read(bk_ota_t* ota, uint32_t off, uint8_t *buf, uint32_t len)
{
	OTA_LOGD(TAG, "read, off=%x len=%x\r\n", off, len);
	if (ota && ota->ops.read) {
		return ota->ops.read(ota, off, buf, len);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_erase(bk_ota_t* ota)
{
	OTA_LOGD(TAG, "erase\r\n");
	if (ota && ota->ops.erase) {
		return ota->ops.erase(ota);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_erase_sector(bk_ota_t* ota, uint32_t off)
{
	OTA_LOGD(TAG, "erase sector, off=%#x\r\n", off);
	if (ota && ota->ops.erase) {
		return ota->ops.erase_sector(ota, off);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_verify(bk_ota_t* ota, void* metadata)
{
	OTA_LOGD(TAG, "verify\r\n");
	if (ota && ota->ops.verify) {
		return ota->ops.verify(ota, metadata);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_confirm(bk_ota_t* ota)
{
	OTA_LOGD(TAG, "confirm\r\n");
	if (ota == NULL) {
		OTA_LOGE(TAG, "bk_ota_confirm: ota is NULL\r\n");
		return BK_ERR_NOT_SUPPORT;
	}
	if (ota->ops.confirm == NULL) {
		OTA_LOGE(TAG, "bk_ota_confirm: ops.confirm is NULL\r\n");
		return BK_ERR_NOT_SUPPORT;
	}
	if (ota->control_partition == NULL) {
		OTA_LOGE(TAG, "bk_ota_confirm: control_partition is NULL\r\n");
		return BK_FAIL;
	}
	OTA_LOGD(TAG, "bk_ota_confirm: calling ops.confirm for partition %s\r\n", 
		ota->control_partition->partition_description ? ota->control_partition->partition_description : "NULL");
	return ota->ops.confirm(ota->control_partition);
}

bk_err_t bk_ota_cancel(bk_ota_t* ota)
{
	OTA_LOGD(TAG, "cancel\r\n");
	if (ota && ota->ops.cancel) {
		return ota->ops.cancel(ota->control_partition);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_accept(bk_ota_t* ota)
{
	OTA_LOGD(TAG, "accept\r\n");
	if (ota && ota->ops.accept) {
		return ota->ops.accept(ota);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}

bk_err_t bk_ota_reboot(bk_ota_t* ota)
{
	OTA_LOGD(TAG, "reboot\r\n");
	if (ota && ota->ops.reboot) {
		return ota->ops.reboot();
	} else {
		return BK_ERR_NOT_SUPPORT;
	}

	return BK_OK;
}

#if CONFIG_OTA_CONFIRM_UPDATE
bk_err_t bk_ota_confirm_update(void)
{
	bk_err_t ret = BK_ERR_NOT_SUPPORT;

#if CONFIG_OTA_OVERWRITE
	{
		bk_ota_t *ow = bk_ota_get_ow_handle();
		if (ow) {
			ret = bk_ota_confirm(ow);
		}
	}
#endif

#if CONFIG_DIRECT_XIP
	if (ret != BK_OK) {
		bk_ota_t *xip = bk_ota_get_xip_handle();
		if (xip) {
			ret = bk_ota_confirm(xip);
		}
	}
#endif

	if (ret == BK_ERR_NOT_SUPPORT) {
		ret = BK_FAIL;
	}

	return ret;
}

bk_err_t bk_ota_cancel_update(uint8_t erase_ota)
{
	bk_err_t ret = BK_ERR_NOT_SUPPORT;

	(void)erase_ota;

#if CONFIG_OTA_OVERWRITE
	{
		bk_ota_t *ow = bk_ota_get_ow_handle();
		if (ow) {
			ret = bk_ota_cancel(ow);
		}
	}
#endif

#if CONFIG_DIRECT_XIP
	if (ret != BK_OK) {
		bk_ota_t *xip = bk_ota_get_xip_handle();
		if (xip) {
			ret = bk_ota_cancel(xip);
		}
	}
#endif

	if (ret == BK_ERR_NOT_SUPPORT) {
		ret = BK_FAIL;
	}

	return ret;
}
#endif

void bk_ota_accept_image(void)
{
#if CONFIG_OTA_CONFIRM_UPDATE
#if CONFIG_OTA_OVERWRITE
	bk_ota_t *ow = bk_ota_get_ow_handle();
	if (ow) {
		bk_ota_accept(ow);
	}
#endif
#if CONFIG_DIRECT_XIP
	bk_ota_t *xip = bk_ota_get_xip_handle();
	if (xip) {
		bk_ota_accept(xip);
	}
#endif
	aon_pmu_hal_set_app_boot_validate_pending(0);
#endif
}

const bk_logic_partition_t* bk_ota_get_partition(bk_ota_t* ota)
{
	return ota->ota_partition;
}

bk_err_t bk_ota_xip_deinit(void)
{
	OTA_LOGD(TAG, "ota xip deinit\r\n");
	if (s_xip_ota_handle) {
		bk_ota_deinit(s_xip_ota_handle);
		s_xip_ota_handle = NULL;
	}
	return BK_OK;
}

bk_ota_t* bk_ota_get_xip_handle(void)
{
	if (!s_xip_ota_handle) {
		s_xip_ota_handle = bk_ota_init("xip_a", "xip_b", "xip_ota_control");
	}

	return s_xip_ota_handle;
}

bk_ota_t* bk_ota_get_ow_handle(void)
{
	if (!s_ow_ota_handle) {
		s_ow_ota_handle = bk_ota_init("overwrite", "ota", "ow_ota_control");
	}

	return s_ow_ota_handle;
}

#if CONFIG_BL2_UPGRADE_WITH_APP
void bk_ota_register_bl2_pack_addrs(void)
{
#if CONFIG_OTA_OVERWRITE
	bl2_ota_register_get_pack_addrs(ow_bl2_ota_get_pack_addrs);
#elif CONFIG_DIRECT_XIP
	bl2_ota_register_get_pack_addrs(xip_bl2_ota_get_pack_addrs);
#endif
}
#endif

bk_ota_t* bk_ota_addr_to_handle(uint32_t addr)
{
	if (!s_ow_ota_handle) {
		s_ow_ota_handle = bk_ota_get_ow_handle();
	}

	if (!s_xip_ota_handle) {
		s_xip_ota_handle = bk_ota_get_xip_handle();
	}

	if (s_ow_ota_handle != NULL && s_ow_ota_handle->ota_partition != NULL) {
		if (addr == s_ow_ota_handle->ota_partition->partition_start_addr) {
			return s_ow_ota_handle;
		}
	}

	// xip ota_partition always xip_a
	if (s_xip_ota_handle != NULL && s_xip_ota_handle->app_partition != NULL) {
		if (addr == s_xip_ota_handle->app_partition->partition_start_addr) {
			return s_xip_ota_handle;
		}
	}

	return NULL;

}

bk_err_t bk_ota_ow_deinit(void)
{
	OTA_LOGD(TAG, "ota ow deinit\r\n");
	if (s_ow_ota_handle) {
		bk_ota_deinit(s_ow_ota_handle);
		s_ow_ota_handle = NULL;
	}
	return BK_OK;
}

#if CONFIG_OTA_UPDATE_PUBKEY
bk_err_t bk_ota_update_public_key(bk_ota_t* ota)
{
	OTA_LOGD(TAG, "ota update public key\r\n");
	if (ota && ota->ops.updatekey) {
		return ota->ops.updatekey(ota->app_partition, ota->key_partition);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
}
#endif
