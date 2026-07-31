#include "sdkconfig.h"
#include <string.h>
#include <stdlib.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include <common/bk_err.h>
#include "partitions_gen.h"
#include "CheckSumUtils.h"
#include "ota_verify.h"

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

bool pubkey_backup_sector_header_empty(uint32_t phy_off)
{
	uint8_t buf[64];
	uint8_t ff[64];

	memset(ff, 0xFF, 64);
	bk_flash_read_bytes(phy_off, buf, 64);
	return (memcmp(buf, ff, 64) == 0);
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

	BK_LOGE(TAG, "write ota confirm part=%x st=%x failed\r\n",
		(unsigned int)partition->partition_start_addr, (unsigned int)status);
	return BK_FAIL;
}

static bk_err_t read_pubkey_from_backup(uint32_t phy_off,
					uint8_t *key_out,
					uint16_t max_len,
					uint16_t *key_len_out)
{
	uint16_t vir_length;
	CRC8_Context crc8;
	CRC8_Context check_crc;
	uint16_t phy_length;
	uint8_t *pubkey_buf = NULL;
	uint32_t key_phy_base;
	uint32_t pubkey_store_base;
	uint32_t plain_key_off;

	key_phy_base = CEIL_ALIGN_34(phy_off);
	pubkey_store_base = FLASH_PHY2VIRTUAL(key_phy_base);
	plain_key_off = pubkey_store_base + 3;

	if (ota_read_pubkey(pubkey_store_base, (uint8_t *)&vir_length, 2) != BK_OK) {
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	if (vir_length == 0 || vir_length > MAX_PUBKEY_LEN || vir_length > max_len) {
		BK_LOGE(TAG, "backup pubkey bad length %#x\r\n", vir_length);
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	phy_length = vir_length;
#if defined(CONFIG_PUBLIC_KEY_PHY_PARTITION_SIZE)
	if ((uint32_t)phy_length + 3u > (uint32_t)CONFIG_PUBLIC_KEY_PHY_PARTITION_SIZE) {
		BK_LOGE(TAG, "backup pubkey blob overflow vir_len=%x part_max=%x\r\n",
			vir_length, (unsigned)CONFIG_PUBLIC_KEY_PHY_PARTITION_SIZE);
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}
#endif

	if (ota_read_pubkey(pubkey_store_base + 2, (uint8_t *)&check_crc, 1) != BK_OK) {
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	pubkey_buf = (uint8_t *)malloc(phy_length);
	if (!pubkey_buf) {
		return BK_ERR_NO_MEM;
	}

	if (ota_read_pubkey(pubkey_store_base + 3, pubkey_buf, phy_length) != BK_OK) {
		free(pubkey_buf);
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	CRC8_Init(&crc8);
	CRC8_Update(&crc8, pubkey_buf, phy_length);

	if (crc8.crc != check_crc.crc) {
		BK_LOGE(TAG, "backup pubkey crc mismatch\r\n");
		free(pubkey_buf);
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	free(pubkey_buf);

	if (ota_read_pubkey(plain_key_off, key_out, vir_length) != BK_OK) {
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}

	*key_len_out = vir_length;
	return BK_OK;
}

bk_err_t load_pubkey_from_flash_backup(uint8_t *pubkey_buf, uint16_t *pubkey_len_inout)
{
#if defined(CONFIG_PUBLIC_KEY_PHY_PARTITION_OFFSET) && defined(CONFIG_PUBLIC_KEY_PHY_PARTITION_SIZE)
	uint32_t key_phy_base;
	uint16_t out_len;
	bk_err_t ret;

	if (!pubkey_buf || !pubkey_len_inout) {
		return BK_ERR_PARAM;
	}

	key_phy_base = CEIL_ALIGN_34(CONFIG_PUBLIC_KEY_PHY_PARTITION_OFFSET);

	if (pubkey_backup_sector_header_empty(key_phy_base)) {
		return BK_ERR_NOT_FOUND;
	}

	out_len = *pubkey_len_inout;
	ret = read_pubkey_from_backup(key_phy_base, pubkey_buf, out_len, &out_len);

	if (ret == BK_OK) {
		*pubkey_len_inout = out_len;
	}

	return ret;
#else
	if (!pubkey_buf || !pubkey_len_inout) {
		return BK_ERR_PARAM;
	}

	return BK_ERR_NOT_FOUND;
#endif
}
