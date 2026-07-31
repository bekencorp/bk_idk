#include "flash_map.h"
#include "Driver_Flash.h"
#include "flash_partition.h"
#include <components/log.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <driver/flash.h>
#define TAG "partition"

static uint32_t s_crc32_table[256];
static uint8_t s_crc32_table_ready;

static void partition_crc32_make_table(void)
{
	unsigned int i;

	if (s_crc32_table_ready) {
		return;
	}
	for (i = 0; i < 256; i++) {
		uint32_t c = (uint32_t)i;
		int bit;

		for (bit = 0; bit < 8; bit++) {
			if (c & 1u) {
				c = (c >> 1) ^ 0xEDB88320u;
			} else {
				c = c >> 1;
			}
		}
		s_crc32_table[i] = c;
	}
	s_crc32_table_ready = 1;
}

static uint32_t partition_table_crc32(const uint8_t *buf, uint32_t len)
{
	uint32_t crc = 0xffffffffu;
	uint32_t i;

	partition_crc32_make_table();
	for (i = 0; i < len; i++) {
		crc = (crc >> 8) ^ s_crc32_table[(crc ^ buf[i]) & 0xff];
	}
	return crc ^ 0xffffffffu;
}

typedef struct {
	uint32_t phy_offset;
	uint32_t phy_size;
    uint32_t phy_flags;
} partition_config_t;
static partition_config_t s_partition_config[PARTITION_CNT] = {0};

const char *s_partition_name[PARTITION_CNT] = {
        "xip_a",
        "xip_b",
        "overwrite",
        "ota",
        "partition",
        "primary_tfm_s",
        "primary_tfm_ns",
        "primary_cpu0_app",
        "sys_otp_nv",
        "sys_ps",
        "sys_its",
        "ow_ota_control",
        "ow_ota_resume",
        "xip_ota_control",
        "public_key"
};

static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

uint32_t piece_address(uint8_t *array,uint32_t index)
{
    return ((uint32_t)(array[index]) << 24 | (uint32_t)(array[index+1])  << 16 | (uint32_t)(array[index+2])  << 8 | (uint32_t)((array[index+3])));
}

uint16_t short_address(uint8_t *array,uint16_t index)
{
    return ((uint16_t)(array[index]) << 8 | (uint16_t)(array[index+1]));
}

uint32_t partition_get_phy_offset(uint32_t id)
{
	if (id >= PARTITION_CNT) {
		return 0;
	}
	return s_partition_config[id].phy_offset;
}

uint32_t partition_get_phy_size(uint32_t id)
{
	if (id >= PARTITION_CNT) {
		return 0;
	}
	return s_partition_config[id].phy_size;
}

int partition_init(void)
{
	bk_flash_driver_init();
	uint8_t *buf = (uint8_t *)malloc(PARTITION_ENTRY_LEN * sizeof(uint8_t));
	uint8_t *payload_buf = NULL;
	uint8_t crc_footer[PARTITION_CRC_FOOTER_LEN];

	uint32_t i;
	uint32_t entry_count = (uint32_t)BK_PARTITIONS_TABLE_SIZE;

	if (entry_count > PARTITION_AMOUNT) {
		BK_LOGE(TAG, "BK_PARTITIONS_TABLE_SIZE %u > PARTITION_AMOUNT\r\n",
			(unsigned int)entry_count);
		free(buf);
		return -1;
	}

	uint32_t payload_len = 0;
	uint32_t calc_crc = 0;
	uint32_t stored_crc = 0;

	char name[PARTITION_NAME_LEN + 1];

	uint32_t slot_base = PARTITION_PARTITION_PHY_OFFSET;
	uint32_t partition_start = slot_base + PARTITION_PPC_OFFSET;

	if (buf == NULL) {
		BK_LOGE(TAG, "memory malloc fails.\r\n");
		return -1;
	}

	payload_len = PARTITION_PPC_OFFSET + entry_count * PARTITION_ENTRY_LEN;

	payload_buf = (uint8_t *)malloc(payload_len);
	if (payload_buf == NULL) {
		BK_LOGE(TAG, "payload malloc fails.\r\n");
		free(buf);
		return -1;
	}

	bk_flash_read_bytes(slot_base, payload_buf, payload_len);
	calc_crc = partition_table_crc32(payload_buf, payload_len);

	bk_flash_read_bytes(slot_base + payload_len, crc_footer, PARTITION_CRC_FOOTER_LEN);
	stored_crc = (uint32_t)crc_footer[0]
		| ((uint32_t)crc_footer[1] << 8)
		| ((uint32_t)crc_footer[2] << 16)
		| ((uint32_t)crc_footer[3] << 24);

	if (calc_crc != stored_crc) {
		BK_LOGE(TAG, "partition table CRC mismatch: calc=0x%08x stored=0x%08x\r\n",
			(unsigned int)calc_crc, (unsigned int)stored_crc);
		free(payload_buf);
		free(buf);
		return -1;
	}

	free(payload_buf);
	payload_buf = NULL;

	for (i = 0; i < entry_count; i++) {
		bk_flash_read_bytes(partition_start + PARTITION_ENTRY_LEN * i, buf, PARTITION_ENTRY_LEN);
		if (is_alpha(buf[0]) == 0) {
			BK_LOGE(TAG, "partition entry %u invalid first byte 0x%02x\r\n",
				(unsigned int)i, (unsigned int)buf[0]);
			free(buf);
			return -1;
		}

		int j;
		for (j = 0; j < PARTITION_NAME_LEN; ++j) {
			if (buf[j] == 0xFF) {
				break;
			}
			name[j] = buf[j];
		}
		name[j] = '\0';

		for (int k = 0; k < PARTITION_CNT; k++) {
			if (strcmp(s_partition_name[k], name) == 0) {
				s_partition_config[k].phy_offset = piece_address(buf, PARTITION_OFFSET_OFFSET);
				s_partition_config[k].phy_size = piece_address(buf, PARTITION_SIZE_OFFSET);
				s_partition_config[k].phy_flags = short_address(buf, PARTITION_FLAGS_OFFSET);
			}
		}
	}

	free(buf);
	return 0;
}

void dump_partition(void)
{
	for (int k = 0; k < PARTITION_CNT; k++) {
		BK_LOGD(TAG, "%s offset=%x size=%x flags=%x\r\n", s_partition_name[k], \
            s_partition_config[k].phy_offset, s_partition_config[k].phy_size, s_partition_config[k].phy_flags);
	}
}

bk_err_t bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code)
{
	(void)magic_code;

#if CONFIG_BL2_UPGRADE_WITH_APP
	return BK_OK;
#endif

	uint32_t bl2_start = (uint32_t)CONFIG_BL2_VIRTUAL_CODE_START;
	uint32_t bl2_end = bl2_start + (uint32_t)CONFIG_BL2_VIRTUAL_CODE_SIZE;
	uint32_t range_end = (uint32_t)addr + (uint32_t)size;

	if (addr < bl2_end && range_end > bl2_start) {
		return BK_FAIL;
	}

	return BK_OK;
}
