// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/mem.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_driver.h"
#include "flash_hal.h"

#if CONFIG_FLASH_PARTITION_USER
#include "vendor_flash_partition.h"
#endif

#if CONFIG_FLASH_ORIGIN_API
#define PAR_OPT_READ_POS      (0)
#define PAR_OPT_WRITE_POS     (1)

#define PAR_OPT_READ_DIS      (0x0u << PAR_OPT_READ_POS)
#define PAR_OPT_READ_EN       (0x1u << PAR_OPT_READ_POS)
#define PAR_OPT_WRITE_DIS     (0x0u << PAR_OPT_WRITE_POS)
#define PAR_OPT_WRITE_EN      (0x1u << PAR_OPT_WRITE_POS)
#endif

#define ADDR_PHY_2_VIRT(addr)  (((addr / 34) * 32))
#define PARTITION_IRAM         __attribute__((section(".iram")))

static bool pt_partition_id_is_valid(bk_partition_t partition)
{
	if (partition >= BK_PARTITION_BOOTLOADER) {
		return true;
	} else {
		return false;
	}
}

uint32_t pt_get_partition_count(void)
{
	return sizeof(logic_partitions) / sizeof(logic_partitions[0]);
}

PARTITION_IRAM bk_logic_partition_t *bk_flash_partition_get_info(bk_partition_t partition)
{
	bk_logic_partition_t *pt = NULL;

	if (pt_partition_id_is_valid(partition)) {
		pt = (bk_logic_partition_t *)&logic_partitions[partition];
	}

	return pt;
}

bk_err_t bk_flash_partition_erase(bk_partition_t partition, uint32_t offset, uint32_t size)
{
	uint32_t erase_addr = 0;
	uint32_t start_sector, end_sector = 0;
	bk_logic_partition_t *partition_info = NULL;
	GLOBAL_INT_DECLARATION();

	partition_info = bk_flash_partition_get_info(partition);
	start_sector = offset >> FLASH_SECTOR_SIZE_OFFSET; /* offset / FLASH_SECTOR_SIZE */
	end_sector = (offset + size - 1) >> FLASH_SECTOR_SIZE_OFFSET;

	for (uint32_t i = start_sector; i <= end_sector; i ++) {
		erase_addr = partition_info->partition_start_addr + (i << FLASH_SECTOR_SIZE_OFFSET);
		GLOBAL_INT_DISABLE();
		bk_flash_erase_sector(erase_addr);
		GLOBAL_INT_RESTORE();
	}

	return BK_OK;
}

bk_err_t bk_flash_partition_write(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
	BK_RETURN_ON_NULL(buffer);

	uint32_t start_addr;
	bk_logic_partition_t *partition_info;

	GLOBAL_INT_DECLARATION();

	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partition not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

	start_addr = partition_info->partition_start_addr + offset;
	if((offset + buffer_len) > partition_info->partition_length) {
		FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	GLOBAL_INT_DISABLE();

	flash_protect_type_t  partition_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	if((offset + buffer_len) <= partition_info->partition_length) {
		bk_flash_write_bytes(start_addr, buffer, buffer_len);
	}
	bk_flash_set_protect_type(partition_type);

	GLOBAL_INT_RESTORE();

	return BK_OK;
}

bk_err_t bk_flash_partition_read(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
	BK_RETURN_ON_NULL(out_buffer);

	uint32_t start_addr;
	bk_logic_partition_t *partition_info;
	GLOBAL_INT_DECLARATION();

	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partiion not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

	start_addr = partition_info->partition_start_addr + offset;
	GLOBAL_INT_DISABLE();
	bk_flash_read_bytes(start_addr, out_buffer, buffer_len);
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

PARTITION_IRAM void *__iram_memcpy(void *d, const void *s, size_t n)
{
    /* attempt word-sized copying only if buffers have identical alignment */

    unsigned char *d_byte = (unsigned char *)d;
    const unsigned char *s_byte = (const unsigned char *)s;
    const uint32_t mask = sizeof(uint32_t) - 1;

    if ((((uint32_t)d ^ (uint32_t)s_byte) & mask) == 0) {
        /* do byte-sized copying until word-aligned or finished */
        while (((uint32_t)d_byte) & mask) {
                if (n == 0) {
                        return d;
                }
                *(d_byte++) = *(s_byte++);
                n--;
        };

        /* do word-sized copying as long as possible */
        uint32_t *d_word = (uint32_t *)d_byte;
        const uint32_t *s_word = (const uint32_t *)s_byte;

        while (n >= sizeof(uint32_t)) {
                *(d_word++) = *(s_word++);
                n -= sizeof(uint32_t);
        }

        d_byte = (unsigned char *)d_word;
        s_byte = (unsigned char *)s_word;
    }

    /* do byte-sized copying until finished */
    while (n > 0) {
        *(d_byte++) = *(s_byte++);
        n--;
    }

    return d;
}

PARTITION_IRAM bk_err_t bk_flash_partition_read_cbus(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
	BK_RETURN_ON_NULL(out_buffer);

    uint32_t in_ptr;
	uint32_t start_addr, flash_base;
	bk_logic_partition_t *partition_info;
	GLOBAL_INT_DECLARATION();

	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partiion not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

    flash_base = 0x02000000 + SOC_ADDR_OFFSET;
	start_addr = partition_info->partition_start_addr;
    in_ptr = flash_base + ADDR_PHY_2_VIRT(start_addr) + offset;

	GLOBAL_INT_DISABLE();
    __iram_memcpy(out_buffer, (void *)in_ptr, buffer_len);
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

PARTITION_IRAM bk_err_t bk_flash_partition_write_cbus(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
	BK_RETURN_ON_NULL(buffer);

    uint32_t wr_ptr;
	uint32_t start_addr, flash_base;
	bk_logic_partition_t *partition_info;

	GLOBAL_INT_DECLARATION();
	FLASH_LOGI("bk_flash_partition_write_cbus:0x%x\r\n", buffer_len);
	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partition not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

    flash_base = 0x02000000 + SOC_ADDR_OFFSET;
	start_addr = partition_info->partition_start_addr;
    wr_ptr = flash_base + ADDR_PHY_2_VIRT(start_addr) + offset;
	if((offset + buffer_len) > partition_info->partition_length) {
		FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	GLOBAL_INT_DISABLE();

	flash_protect_type_t  partition_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	if((offset + buffer_len) <= partition_info->partition_length) {
        __iram_memcpy((void *)wr_ptr, buffer, buffer_len);
	}
	bk_flash_set_protect_type(partition_type);

	GLOBAL_INT_RESTORE();

	return BK_OK;
}

// eof
