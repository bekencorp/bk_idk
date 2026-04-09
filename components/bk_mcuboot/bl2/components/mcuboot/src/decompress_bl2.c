#include <inttypes.h>
#include <driver/flash.h>
#include "flash_partition.h"
#include <stdlib.h>
#include <string.h>
#include "region_defs.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/bootutil_public.h"
#include "flash_map.h"
#include "security.h"

#if CONFIG_BL2_WDT
#include <driver/wdt_hal.h>
#endif

#define ALIGN_4096(addr) (((addr) + 4096 - 1) / 4096 * 4096)
#define COMPRESS_BLOCK_SIZE (64*1024)
#define MAX_BLOCK_NUM 100

typedef struct {
	uint8_t crc;
} CRC8_Context;
typedef struct {
	uint8_t index;
	uint8_t data[33];
	CRC8_Context crc8;
} resume_block_t;

extern uint8_t *decompress_in_memory();

#if CONFIG_STATIC_ARRARY

static uint8_t s_compressed_buf[COMPRESS_BLOCK_SIZE + 64];
static uint8_t s_decompressed_buf[COMPRESS_BLOCK_SIZE + 64];
#else
//Use fixed address to optimize loader time
static uint8_t *s_compressed_buf = (uint8_t*)BL2_FREE_SRAM;
static uint8_t *s_decompressed_buf = (uint8_t*)(BL2_FREE_SRAM + COMPRESS_BLOCK_SIZE + 128);
#endif

#define ERASE_VERIFY_BUFFER_SIZE (4 * 1024)
#define ERASE_RETRY_COUNT        3

/* Verify erased flash is all 0xFF; return 0 if pass, non-zero if fail */
static int verify_erase(uint32_t offset, uint32_t size)
{
	static uint8_t verify_buf[ERASE_VERIFY_BUFFER_SIZE] = {0};
	uint32_t remaining = size;
	uint32_t current_offset = offset;
	uint32_t verify_size;

	while (remaining > 0) {
		verify_size = (remaining > ERASE_VERIFY_BUFFER_SIZE) ?
			ERASE_VERIFY_BUFFER_SIZE : remaining;

		if (bk_flash_read_bytes(current_offset, verify_buf, verify_size) != BK_OK) {
			BOOT_LOG_ERR("Failed to read flash at offset=0x%x, size=0x%x",
				current_offset, verify_size);
			return -1;
		}

		{
			uint32_t i;

			for (i = 0; i < verify_size; i++) {
				if (verify_buf[i] != 0xFF) {
					BOOT_LOG_ERR("Erase verification failed at offset=0x%x, byte[%u]=0x%02x",
						current_offset + i, (unsigned)i, verify_buf[i]);
					return -1;
				}
			}
		}

		current_offset += verify_size;
		remaining -= verify_size;
	}

	return 0;
}

/* Erase with readback verify and retries; on total failure log, optional fail flag, WDT reboot */
static int flash_area_erase_fast_with_verify(uint32_t erase_off, uint32_t len)
{
	int retry_count = 0;
	int verify_result = -1;

	for (retry_count = 0; retry_count < ERASE_RETRY_COUNT; retry_count++) {
		flash_area_erase_fast(erase_off, len);

		verify_result = verify_erase(erase_off, len);
		if (verify_result == 0) {
			if (retry_count > 0) {
				BOOT_LOG_INF("Erase verification passed after %d retries, offset=0x%x, size=0x%x",
					retry_count, erase_off, len);
			}
			return 0;
		}

		BOOT_LOG_ERR("Erase verification failed (attempt %d/%d), offset=0x%x, size=0x%x",
			retry_count + 1, ERASE_RETRY_COUNT, erase_off, len);
	}

	BOOT_LOG_ERR("Flash erase verification failed after %d retries, triggering WDT reboot",
		ERASE_RETRY_COUNT);
	BOOT_LOG_ERR("Failed erase: offset=0x%x, size=0x%x", erase_off, len);
#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
	bk_boot_write_ota_fail(BOOT_FAIL_STAGE_ERASE);
#endif
	wdt_hal_force_reboot();

	return -1;
}

#if CONFIG_DECOMPRESS_RESUME


static uint8_t UpdateCRC8(uint8_t crcIn, uint8_t byte)
{
	uint8_t crc = crcIn;
	uint8_t i;

	crc ^= byte;

	for (i = 0; i < 8; i++) {
		if (crc & 0x01) {
			crc = (crc >> 1) ^ 0x8C;
		} else {
			crc >>= 1;
		}
	}
	return crc;
}

static void CRC8_Init( CRC8_Context *inContext )
{
    inContext->crc = 0;
}

static void CRC8_Update( CRC8_Context *inContext, const void *inSrc, size_t inLen )
{
	const uint8_t *src = (const uint8_t *) inSrc;
	const uint8_t *srcEnd = src + inLen;

	while ( src < srcEnd ) {
		inContext->crc = UpdateCRC8(inContext->crc, *src++);
	}
}

static void CRC8_Final( CRC8_Context *inContext, uint8_t *outResult )
{
	*outResult = inContext->crc & 0xffu;
}

static uint32_t get_resume_base_address(void)
{
	return partition_get_phy_offset(PARTITION_OW_OTA_CONTROL);
}


static uint8_t read_resume_block(uint32_t back_address, uint8_t* resume_data, size_t resume_data_size)
{
	resume_block_t resume_block[2];
	CRC8_Context crc_8;
	uint8_t idx = 0;

	memset(resume_block, 0xFF, sizeof(resume_block));

	for(; idx < MAX_BLOCK_NUM; ++idx) {
		CRC8_Init(&crc_8);
		resume_block_t* curr = &resume_block[idx % 2];
		memset(curr,0xFF,sizeof(resume_block_t));
		bk_flash_read_bytes(back_address + idx * sizeof(resume_block_t), (uint8_t*)curr, sizeof(resume_block_t));
		if(curr->index != 0xFF){
			CRC8_Update(&crc_8, &curr->index, sizeof(curr->index));
			CRC8_Update(&crc_8, curr->data, sizeof(curr->data));
			if(crc_8.crc != curr->crc8.crc){
				BOOT_LOG_ERR("resume block=%d crc8 error!", idx);
				return 0xffu;
			}
		} else {
			break;
		}
	}

	memcpy(resume_data,resume_block[(idx + 1) % 2].data,resume_data_size);
	return idx;
}

static uint8_t write_resume_block(uint8_t idx, uint32_t back_address, uint8_t* resume_data, size_t resume_data_size)
{
	resume_block_t resume_block;
	memset(&resume_block,0xFF,sizeof(resume_block));
	memcpy(resume_block.data,resume_data,resume_data_size);
	CRC8_Context crc_8;
	CRC8_Init(&crc_8);
	CRC8_Update(&crc_8, &idx, sizeof(idx));
	CRC8_Update(&crc_8, resume_block.data, sizeof(resume_block.data));
	resume_block.index = idx;
	resume_block.crc8 = crc_8;
	bk_flash_write_bytes(back_address + idx * sizeof(resume_block), (uint8_t*)&resume_block, sizeof(resume_block));
}

static uint32_t resume_flash(uint32_t block_num)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(FLASH_AREA_OVERWRITE_ID);
	uint32_t area_size = get_flash_map_phy_size(FLASH_AREA_OVERWRITE_ID);

	uint32_t back_address = get_resume_base_address();
	uint32_t back_data_size = CEIL_ALIGN_34(primary_all_phy_offset) - primary_all_phy_offset;
	uint8_t back_data[back_data_size];
	uint8_t restart_block_idx = read_resume_block(back_address, back_data, back_data_size);

	BOOT_LOG_INF("total block=%d, resume block=%d", block_num, restart_block_idx);

	if((restart_block_idx == 0) || (restart_block_idx == 0xffu) || (restart_block_idx > block_num)) {
		BOOT_LOG_INF("Erasing primary and resume");
		flash_area_erase_fast_with_verify(primary_all_phy_offset, area_size);
		/* Safeguard: never erase at 0 or below overwrite start (would erase bootloader) */
		uint32_t resume_addr = get_resume_base_address();
		if (resume_addr != 0 && resume_addr >= primary_all_phy_offset) {
			flash_area_erase_fast_with_verify(resume_addr, 4096);
		} else {
			BOOT_LOG_ERR("resume addr 0x%x invalid, skip resume erase to protect bootloader", (unsigned)resume_addr);
		}
#if CONFIG_BL2_WDT
		wdt_hal_force_feed();
#endif
		restart_block_idx = 0;
		return restart_block_idx;
	}

	uint32_t restart_block_offset = primary_all_phy_offset + FLASH_VIRTUAL2PHY(COMPRESS_BLOCK_SIZE) * restart_block_idx;
	uint32_t erase_size = FLASH_VIRTUAL2PHY(COMPRESS_BLOCK_SIZE) + 4 * 1024;

	if(restart_block_idx < block_num) {
		erase_size = FLASH_VIRTUAL2PHY(COMPRESS_BLOCK_SIZE) + 4 * 1024;
	} else if(restart_block_idx == block_num) {
		erase_size = primary_all_phy_offset + area_size - restart_block_offset;
	}

	BOOT_LOG_INF("Erasing primary off=0x%x, size=0x%x", restart_block_offset, erase_size);
	flash_area_erase_fast_with_verify(restart_block_offset, erase_size);
	bk_flash_write_bytes(restart_block_offset, back_data, back_data_size);

	return restart_block_idx;
}

static void back_flash(uint32_t restart_block_idx)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(FLASH_AREA_OVERWRITE_ID);

	uint32_t back_address = get_resume_base_address();
	uint32_t back_data_size = CEIL_ALIGN_34(primary_all_phy_offset) - primary_all_phy_offset; // COMPRESS_BLOCK = 68k
	uint8_t back_data[back_data_size];
	uint32_t restart_block_offset = primary_all_phy_offset + FLASH_VIRTUAL2PHY(COMPRESS_BLOCK_SIZE) * (restart_block_idx + 1);

	bk_flash_read_bytes(restart_block_offset, back_data, back_data_size);
	write_resume_block(restart_block_idx, back_address, back_data, back_data_size);
}
#endif

static uint32_t idx_sum(uint16_t* buffer, size_t idx)
{
	uint32_t sum = 0;

	for( size_t i = 0; i < idx; ++i){
		sum += buffer[i];
	}

	return sum;
}

static void clean_buf(void)
{
	os_memset(s_decompressed_buf, 0, COMPRESS_BLOCK_SIZE);
	os_memset(s_compressed_buf, 0, COMPRESS_BLOCK_SIZE);
}

int
boot_copy_region(struct boot_loader_state *state, const struct flash_area *fap_src,
	const struct flash_area *fap_dst, uint32_t off_src, uint32_t off_dst, uint32_t sz)
{
	uint32_t primary_all_vir_size = get_flash_map_size(FLASH_AREA_OVERWRITE_ID);
	uint32_t primary_all_phy_offset = get_flash_map_offset(FLASH_AREA_OVERWRITE_ID);
	uint32_t ota_phy_size = get_flash_map_phy_size(FLASH_AREA_OTA_ID);
	uint32_t block_num = (primary_all_vir_size) / COMPRESS_BLOCK_SIZE;
	uint16_t block_list[block_num + 2];
	uint32_t bytes_copied = 0;
	uint8_t restart_block_idx;

	flash_protect_type_t protect = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);

#if CONFIG_DECOMPRESS_RESUME
        restart_block_idx = resume_flash(block_num);
#else
        restart_block_idx = 0;
        uint32_t area_size = fap_dst->fa_phy_size;
        flash_area_erase_fast_with_verify(primary_all_phy_offset, area_size);
#endif

	int rate_process = block_num / 5;
	uint32_t vir_primary_all_start_address = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(primary_all_phy_offset));

	uint8_t block_idx = 0;

	bytes_copied = BL2_HEADER_SIZE;
	flash_area_read(fap_src, off_src + bytes_copied, block_list, 2 * (block_num + 2));
	bytes_copied += 2 * (block_num + 2);

	bytes_copied += idx_sum(block_list, restart_block_idx);

	for (block_idx = restart_block_idx; block_idx < block_num; block_idx++){
#if CONFIG_BL2_WDT
		wdt_hal_force_feed();
#endif
		clean_buf();
		flash_area_read(fap_src, off_src + bytes_copied, s_compressed_buf, block_list[block_idx]);
		{
			uint8_t *decomp_result = decompress_in_memory(s_compressed_buf, s_decompressed_buf, COMPRESS_BLOCK_SIZE, 0);

			if (decomp_result == NULL) {
				BOOT_LOG_ERR("OTA decompress failed at block %d", block_idx);
#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
				bk_boot_write_ota_fail(BOOT_FAIL_STAGE_OTA_DECOMP);
#endif
				bk_flash_set_protect_type(protect);
				return -1;
			}
		}
		bk_flash_write_cbus(vir_primary_all_start_address + COMPRESS_BLOCK_SIZE * block_idx, (s_decompressed_buf), COMPRESS_BLOCK_SIZE);

#if CONFIG_DECOMPRESS_RESUME
		back_flash(restart_block_idx);
#endif

		restart_block_idx += 1;
		bytes_copied += block_list[block_idx];

		if((block_idx + 1) % rate_process == 0){
			BOOT_LOG_INF("OTA %d%%",(block_idx / rate_process + 1) * 20);
		}
	}

	uint16_t last_block_before_size = block_list[block_idx+1];
	uint16_t last_block_after_size  = block_list[block_idx];

	if (last_block_before_size > 0) {
		clean_buf();
		flash_area_read(fap_src, off_src + bytes_copied, s_compressed_buf, last_block_after_size);
		{
			uint8_t *decomp_result = decompress_in_memory(s_compressed_buf, s_decompressed_buf, last_block_before_size, 0);

			if (decomp_result == NULL) {
				BOOT_LOG_ERR("OTA decompress failed at last block");
#if CONFIG_OTA_OVERWRITE && CONFIG_OTA_CONFIRM_UPDATE
				bk_boot_write_ota_fail(BOOT_FAIL_STAGE_OTA_DECOMP);
#endif
				bk_flash_set_protect_type(protect);
				return -1;
			}
		}

		uint16_t write_size = (last_block_before_size + 31) / 32 * 32;
		bk_flash_write_cbus(vir_primary_all_start_address + COMPRESS_BLOCK_SIZE * block_idx, s_decompressed_buf, write_size);
	}

#if CONFIG_DECOMPRESS_RESUME
	{
		uint32_t resume_addr = get_resume_base_address();
		if (resume_addr != 0 && resume_addr >= primary_all_phy_offset) {
			BOOT_LOG_INF("erasing resume");
			flash_area_erase_fast_with_verify(resume_addr, 4096);
		} else {
			BOOT_LOG_ERR("resume addr 0x%x invalid, skip resume erase", (unsigned)resume_addr);
		}
	}
#endif
	bk_flash_set_protect_type(protect);
	return 0;
}
