#include <string.h>
#include <stdint.h>
#include "ota_bl2.h"
#include "modules/ota.h"
#include <driver/flash.h>
#include "sdkconfig.h"
#include "partitions_gen.h"
#include "flash_reg.h"
#include <modules/bk_ota.h>


#define TAG "OTA_BL2"

#define BUF_SIZE 4096
/* bl2 pack header definition, must be consistent with tools/env_tools/bksecure/scripts/pack_header.py
 * PACK_HEADER_FORMAT = "12s20s8sBB2xIIII4xIIII16x"
 * We check magic/version and parse size field here.
 */
#define BL2_PACK_HEADER_MAGIC      "pack_header"
#define BL2_PACK_HEADER_MAGIC_LEN  11
#define BL2_PACK_HEADER_SIZE       96

#define BL2_PACK_HEADER_U32_BASE_OFFSET  44
/* At BL2_PACK_HEADER_U32_BASE_OFFSET:
 * uint32_t offset;
 * uint32_t size;
 * uint32_t flags;
 * uint32_t hdr_size;
 */
#define BL2_PACK_HEADER_SIZE_OFFSET (BL2_PACK_HEADER_U32_BASE_OFFSET + 4)
#define BL2_MAGIC_BYTES_LEN         8
#define BL2_MAGIC_VIRT_OFFSET       0x100
#define BL2_FLASH_MAGIC_BK_SB       "BK.SB"
#define BL2_FLASH_MAGIC_BK_SB_LEN   6
#define BL2_FLASH_MAGIC_BEKEN       "BEKEN"
#define BL2_FLASH_MAGIC_BEKEN_LEN   5
#define BL2_FLASH_MAGIC_BK7236      "BK7236"
#define BL2_FLASH_MAGIC_BK7236_LEN  6

typedef struct {
	uint8_t magic[BL2_MAGIC_BYTES_LEN];
	uint32_t virt_offset;
	size_t magic_len;
} bl2_ota_magic_info_t;

#if CONFIG_BL2_UPGRADE_WITH_APP
static uint8_t buf[BUF_SIZE];
static uint8_t bl2_first_part[512];
static bl2_ota_get_pack_addrs_fn s_get_pack_addrs;
#endif

void bl2_ota_register_get_pack_addrs(bl2_ota_get_pack_addrs_fn fn)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	s_get_pack_addrs = fn;
#else
	(void)fn;
#endif
}

static int bl2_ota_match_magic_at(const uint8_t *flash_head, uint32_t virt_offset,
                                    bl2_ota_magic_info_t *info)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	const uint8_t *base = flash_head + FLASH_VIRTUAL2PHY(virt_offset);
	const char *matched = NULL;
	size_t matched_len = 0;

	if (memcmp(base, BL2_FLASH_MAGIC_BK_SB, BL2_FLASH_MAGIC_BK_SB_LEN) == 0) {
		matched = BL2_FLASH_MAGIC_BK_SB;
		matched_len = BL2_FLASH_MAGIC_BK_SB_LEN;
	} else if (memcmp(base, BL2_FLASH_MAGIC_BEKEN, BL2_FLASH_MAGIC_BEKEN_LEN) == 0) {
		matched = BL2_FLASH_MAGIC_BEKEN;
		matched_len = BL2_FLASH_MAGIC_BEKEN_LEN;
	} else if (memcmp(base, BL2_FLASH_MAGIC_BK7236, BL2_FLASH_MAGIC_BK7236_LEN) == 0) {
		matched = BL2_FLASH_MAGIC_BK7236;
		matched_len = BL2_FLASH_MAGIC_BK7236_LEN;
	}

	if (matched != NULL) {
		memset(info->magic, 0, BL2_MAGIC_BYTES_LEN);
		memcpy(info->magic, matched, matched_len);
		info->virt_offset = virt_offset;
		info->magic_len = matched_len;
		BK_LOGI(TAG, "magic matched at 0x%x: %s\r\n", virt_offset, matched);
		return 0;
	}
#endif
	return -1;
}

static int bl2_ota_detect_magic(const uint8_t *flash_head, bl2_ota_magic_info_t *info)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	if (bl2_ota_match_magic_at(flash_head, BL2_MAGIC_VIRT_OFFSET, info) == 0) {
		return 0;
	}

	BK_LOGW(TAG, "magic not found at 0x%x, use default %s\r\n",
	        BL2_MAGIC_VIRT_OFFSET, BL2_FLASH_MAGIC_BEKEN);
	memset(info->magic, 0, BL2_MAGIC_BYTES_LEN);
	memcpy(info->magic, BL2_FLASH_MAGIC_BEKEN, BL2_FLASH_MAGIC_BEKEN_LEN);
	info->virt_offset = BL2_MAGIC_VIRT_OFFSET;
	info->magic_len = BL2_FLASH_MAGIC_BEKEN_LEN;
#endif
	return -1;
}

static void bl2_ota_rewrite_magic(const bl2_ota_magic_info_t *info)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	static const uint8_t version_bytes[] = CONFIG_BOOTLOADER_VERSION_BYTES;
	uint16_t data_offset;

	memcpy(&bl2_first_part[FLASH_VIRTUAL2PHY(info->virt_offset)], info->magic, BL2_MAGIC_BYTES_LEN);

	data_offset = FLASH_VIRTUAL2PHY(0x120);
	bl2_first_part[data_offset++] = version_bytes[0];
	bl2_first_part[data_offset++] = version_bytes[1];
	bl2_first_part[data_offset++] = version_bytes[2];
	bl2_first_part[data_offset++] = version_bytes[3];
	bk_flash_read_bytes(0, buf, 4096);
	bk_flash_erase_fast(0, 4096);
	memcpy(buf, bl2_first_part, FLASH_VIRTUAL2PHY(0x140));
	bk_flash_write_bytes(0, buf, 4096);
#endif
}

static void bl2_ota_upgrade(void)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	uint8_t ff[64];
	uint8_t header_buf[BL2_PACK_HEADER_SIZE];
	bl2_ota_magic_info_t magic_info;
	uint32_t bl2_real_size = 0;
	uint32_t partition_real_size = 0;
	uint32_t bl2_address = 0;
	uint32_t partition_address = 0;
	uint32_t bl2_data_addr = 0;
	uint32_t partition_data_addr = 0;
	uint32_t remain = 0;
	uint32_t write_offset = 0;
	int block_idx = 0;

	if (!s_get_pack_addrs) {
		BK_LOGW(TAG, "bl2 pack addrs callback not registered\r\n");
		return;
	}
	s_get_pack_addrs(&bl2_address, &partition_address);
	bl2_data_addr = bl2_address + BL2_PACK_HEADER_SIZE;
	partition_data_addr = partition_address + BL2_PACK_HEADER_SIZE;
	BK_LOGI(TAG, "bl2_address: %x, partition_address: %x\r\n", bl2_address, partition_address);
	BK_LOGI(TAG, "bl2_data_addr: %x, partition_data_addr: %x\r\n", bl2_data_addr, partition_data_addr);
	bk_flash_read_cbus(bl2_data_addr, buf, BUF_SIZE);
	memset(ff, 0xFF, 64);
	if (memcmp(buf, ff, 64) == 0) {
		BK_LOGW(TAG, "read new bl2 empty, return\r\n");
		return;
	}	

	bk_flash_read_cbus(bl2_address, header_buf, BL2_PACK_HEADER_SIZE);
	if (memcmp(header_buf, BL2_PACK_HEADER_MAGIC, BL2_PACK_HEADER_MAGIC_LEN) == 0) {
		/* Read size field (little-endian uint32) */
		memcpy(&bl2_real_size, header_buf + BL2_PACK_HEADER_SIZE_OFFSET, sizeof(uint32_t));
		if ((bl2_real_size == 0) || (bl2_real_size > CONFIG_BL2_PHY_PARTITION_SIZE)) {
			BK_LOGW(TAG, "invalid bl2 size in header: %u, use partition size %u\r\n",
			        bl2_real_size, CONFIG_BL2_PHY_PARTITION_SIZE);
			bl2_real_size = CONFIG_BL2_PHY_PARTITION_SIZE;
		}
	} else {
		/* No valid pack header */
		BK_LOGW(TAG, "no valid bl2 pack header, exit\r\n");
		return;
	}
	BK_LOGI(TAG, "bl2_real_size from header: %u bytes\r\n", bl2_real_size);

	bk_flash_read_bytes(0, bl2_first_part, FLASH_VIRTUAL2PHY(0x140));
	bl2_ota_detect_magic(bl2_first_part, &magic_info);
	BK_LOGI(TAG, "start erasing bl2\r\n");
	bk_flash_erase_fast(CONFIG_BL2_PHY_PARTITION_OFFSET, CONFIG_BL2_PHY_PARTITION_SIZE);

	remain = bl2_real_size;
	write_offset = 0;
	block_idx = 0;

	while (remain > 0) {
		uint32_t chunk = (remain > BUF_SIZE) ? BUF_SIZE : remain;

		BK_LOGI(TAG, "write index: %d, size: %u\r\n", block_idx, chunk);
		bk_flash_read_cbus(bl2_data_addr + write_offset, buf, chunk);
		bk_flash_write_cbus(FLASH_ADDR_REMAP(CONFIG_BL2_PHY_PARTITION_OFFSET + write_offset), buf, chunk);
		
		write_offset += chunk;
		remain -= chunk;
		block_idx++;
	}

	BK_LOGI(TAG, "rewrite bl2 magic\r\n");
	bl2_ota_rewrite_magic(&magic_info);

	BK_LOGI(TAG, "erase partition table\r\n");
	bk_flash_erase_fast(CONFIG_PARTITION_PHY_PARTITION_OFFSET, CONFIG_PARTITION_PHY_PARTITION_SIZE);

	/* Parse partition pack header at partition_address and get real partition size */
	bk_flash_read_cbus(partition_address, header_buf, BL2_PACK_HEADER_SIZE);
	if (memcmp(header_buf, BL2_PACK_HEADER_MAGIC, BL2_PACK_HEADER_MAGIC_LEN) == 0) {
		/* Read size field (little-endian uint32) */
		memcpy(&partition_real_size, header_buf + BL2_PACK_HEADER_SIZE_OFFSET, sizeof(uint32_t));
		if ((partition_real_size == 0) || (partition_real_size > CONFIG_PARTITION_PHY_PARTITION_SIZE)) {
			BK_LOGW(TAG, "invalid partition size in header: %u, use partition size %u\r\n",
			        partition_real_size, CONFIG_PARTITION_PHY_PARTITION_SIZE);
			partition_real_size = CONFIG_PARTITION_PHY_PARTITION_SIZE;
		}
	} else {
		/* No valid pack header */
		BK_LOGW(TAG, "no valid partition pack header, exit\r\n");
		return;
	}
	BK_LOGI(TAG, "partition_real_size from header: %u bytes\r\n", partition_real_size);

	remain = partition_real_size;
	write_offset = 0;
	block_idx = 0;
	while (remain > 0) {
		uint32_t chunk = (remain > BUF_SIZE) ? BUF_SIZE : remain;

		BK_LOGI(TAG, "write partition index: %d, size: %u\r\n", block_idx, chunk);
		bk_flash_read_cbus(partition_data_addr + write_offset, buf, chunk);
		bk_flash_write_bytes(CONFIG_PARTITION_PHY_PARTITION_OFFSET + write_offset, buf, chunk);

		write_offset += chunk;
		remain -= chunk;
		block_idx++;
	}
	BK_LOGI(TAG, "finish update bl2,reboot\r\n");
	bk_ota_accept_image();
	bk_reboot();
#endif
}

void bl2_ota_check(void)
{
#if CONFIG_BL2_UPGRADE_WITH_APP
	uint8_t bootloader_version[4];
	static const uint8_t expected_bl2_version[] = CONFIG_BOOTLOADER_VERSION_BYTES;
	bk_flash_read_bytes(FLASH_VIRTUAL2PHY(0x120), bootloader_version, 4);
	BK_LOGI(TAG, "current bl2 version: %d.%d.%d\r\n", bootloader_version[0], bootloader_version[1],
		(bootloader_version[2] | (bootloader_version[3] << 8)));
	BK_LOGI(TAG, "expected bl2 version: %d.%d.%d\r\n", expected_bl2_version[0], expected_bl2_version[1],
		(expected_bl2_version[2] | (expected_bl2_version[3] << 8)));
	if (!(bootloader_version[0] == expected_bl2_version[0]
		&& bootloader_version[1] == expected_bl2_version[1]
		&& bootloader_version[2] == expected_bl2_version[2]
		&& bootloader_version[3] == expected_bl2_version[3])) {
		BK_LOGI(TAG, "bl2 version mismatch, start to update\r\n");
		bk_ota_register_bl2_pack_addrs();
#if CONFIG_SOC_BK7236
		flash_protect_type_t protect_type;
		protect_type = bk_flash_get_protect_type();
		bk_flash_set_protect_none_full();
#endif

		bl2_ota_upgrade();

#if CONFIG_SOC_BK7236
		bk_flash_set_protect_type_full(protect_type);
#endif

		bl2_ota_upgrade();
	}
	else {
		BK_LOGI(TAG, "bl2 version match, no need to update bl2\r\n");
	}
#endif
}
