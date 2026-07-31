// Copyright 2022-2030 Beken
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

#include "deep_lv_reserve.h"
#include "sdkconfig.h"
#include <stddef.h>
#include <soc/soc.h>

#if MEM_CHECK_TEST
#include <os/os.h>
#endif

#if CONFIG_DEEP_LV
deep_lv_reserve_t g_deep_lv_reserve __attribute__((section(".deep_lv_reserve")));

_Static_assert(sizeof(deep_lv_reserve_t) == DEEP_LV_RESERVE_SIZE,
	"deep_lv_reserve_t must fill reserve area exactly");
_Static_assert(DEEP_LV_RESERVE_HEADER_WORDS == 57u,
	"unexpected deep_lv_reserve header size");
_Static_assert(offsetof(deep_lv_reserve_t, magic) == DEEP_LV_RESERVE_MAGIC_OFFSET,
	"deep_lv_reserve magic offset mismatch");
_Static_assert(offsetof(deep_lv_reserve_t, app_msp_addr) == DEEP_LV_RESERVE_APP_MSP_OFFSET,
	"deep_lv_reserve app_msp_addr offset mismatch");
_Static_assert(offsetof(deep_lv_reserve_t, app_entry_addr) == DEEP_LV_RESERVE_APP_ENTRY_OFFSET,
	"deep_lv_reserve app_entry_addr offset mismatch");

typedef struct {
	uint16_t reg_idx;
	uint16_t count;
	uint16_t valid_mask; /* valid flag mask: 0x4000 for 64K, 0x8000 for 128K */
	uint16_t addr_mask;  /* word offset mask: 0x3FFF for 64K, 0x7FFF for 128K */
	uint32_t mem_base;
	uint32_t mem_end;
} mem_check_block_cfg_t;

static const mem_check_block_cfg_t s_mem_check_block_cfgs[] = {
	{  8, 4, 0x4000, 0x3FFF, SOC_SRAM0_DATA_BASE, SOC_SRAM0_DATA_BASE + 0x10000 }, /* Reg08~11, SRAM0, 64K */
	{ 12, 4, 0x4000, 0x3FFF, SOC_SRAM1_DATA_BASE, SOC_SRAM1_DATA_BASE + 0x10000 }, /* Reg12~15, SRAM1, 64K */
	{ 16, 4, 0x8000, 0x7FFF, SOC_SRAM2_DATA_BASE, SOC_SRAM2_DATA_BASE + 0x20000 }, /* Reg16~19, SRAM2, 128K */
	{ 20, 4, 0x8000, 0x7FFF, SOC_SRAM3_DATA_BASE, SOC_SRAM3_DATA_BASE + 0x20000 }, /* Reg20~23, SRAM3, 128K */
	{ 24, 4, 0x8000, 0x7FFF, SOC_SRAM4_DATA_BASE, SOC_SRAM4_DATA_BASE + 0x20000 }, /* Reg24~27, SRAM4, 128K */
};

static const mem_check_block_cfg_t *mem_check_get_cfg_by_base(uint32_t mem_base)
{
	for (uint32_t blk = 0; blk < (sizeof(s_mem_check_block_cfgs) / sizeof(s_mem_check_block_cfgs[0])); blk++) {
		if (s_mem_check_block_cfgs[blk].mem_base == mem_base) {
			return &s_mem_check_block_cfgs[blk];
		}
	}

	return NULL;
}

__IRAM_SEC uint32_t mem_check_decode_bad_point_addr(uint16_t bst_high, uint32_t mem_base)
{
	const mem_check_block_cfg_t *cfg = mem_check_get_cfg_by_base(mem_base);

	if (cfg == NULL) {
		return 0;
	}

	if ((bst_high & cfg->valid_mask) == 0) {
		return 0;
	}

	return mem_base + (bst_high & cfg->addr_mask) * 4;
}

__IRAM_SEC void sys_hal_mem_check_bad_point_addr_get(void)
{
	volatile uint32_t *reg_base = (volatile uint32_t *)SOC_MEM_CHECK_REG_BASE;
	uint32_t save_idx = 0;

	for (uint32_t blk = 0; blk < (sizeof(s_mem_check_block_cfgs) / sizeof(s_mem_check_block_cfgs[0])); blk++) {
		const mem_check_block_cfg_t *cfg = &s_mem_check_block_cfgs[blk];
		uint32_t mem_base = cfg->mem_base;

		for (uint32_t i = 0; i < cfg->count; i++) {
			uint32_t reg_idx = cfg->reg_idx + i;
			uint16_t bst_high = (uint16_t)(reg_base[reg_idx] >> 16);

			g_deep_lv_reserve.bad_points[save_idx].addr =
				mem_check_decode_bad_point_addr(bst_high, mem_base);
			save_idx++;
		}
	}
}

__IRAM_SEC void sys_hal_mem_check_bad_point_value_save(void)
{
	for (uint32_t i = 0; i < MEM_CHECK_BST_POINT_NUM; i++) {
		uint32_t bad_addr = g_deep_lv_reserve.bad_points[i].addr;

		g_deep_lv_reserve.bad_points[i].value = (bad_addr != 0) ?
			*(volatile uint32_t *)bad_addr : 0;
	}

	g_deep_lv_reserve.magic = DEEP_LV_RESERVE_MAGIC;
	*(volatile uint32_t *)DEEP_LV_RESERVE_MAGIC_BOOTROM_ADDR = DEEP_LV_RESERVE_MAGIC;
}

__IRAM_SEC void sys_hal_mem_check_bad_point_value_restore(void)
{
	if (!deep_lv_reserve_is_valid(&g_deep_lv_reserve)) {
		return;
	}

	for (uint32_t i = 0; i < MEM_CHECK_BST_POINT_NUM; i++) {
		uint32_t bad_addr = g_deep_lv_reserve.bad_points[i].addr;

		if (bad_addr != 0) {
			*(volatile uint32_t *)bad_addr = g_deep_lv_reserve.bad_points[i].value;
		}
	}
}

#if MEM_CHECK_TEST
#define OTP_APB_MEM_CHECK_MARK_BASE       (SOC_OTP_APB_BASE + 0x400)

mem_check_bad_point_t bad_points[MEM_CHECK_BST_POINT_NUM] = {
	/* mem0 (SRAM0): 0x28000000 - 0x2800FFFF */
	{ .addr = 0x28004D70, .value = 0 },
	{ .addr = 0x28006D60, .value = 0 },
	{ .addr = 0x28009DDC, .value = 0 },
	{ .addr = 0x2800C5A8, .value = 0 },
	/* mem1 (SRAM1): 0x28010000 - 0x2801FFFF */
	{ .addr = 0x280126F0, .value = 0 },
	{ .addr = 0x280147D8, .value = 0 },
	{ .addr = 0x2801B120, .value = 0 },
	{ .addr = 0x2801C670, .value = 0 },
	/* mem2 (SRAM2): 0x28020000 - 0x2803FFFF */
	{ .addr = 0x280263C0, .value = 0 },
	{ .addr = 0x28027D08, .value = 0 },
	{ .addr = 0x28034648, .value = 0 },
	{ .addr = 0x2803CE80, .value = 0 },
	/* mem3 (SRAM3): 0x28040000 - 0x2805FFFF */
	{ .addr = 0x2804DF6C, .value = 0 },
	{ .addr = 0x280575CC, .value = 0 },
	{ .addr = 0x2805B68C, .value = 0 },
	{ .addr = 0x2805CB14, .value = 0 },
	/* mem4 (SRAM4): 0x28060000 - 0x2807FFFF, exclude 0x2807EA00-0x2807EBFF */
	{ .addr = 0x28069D10, .value = 0 },
	{ .addr = 0x28077264, .value = 0 },
	{ .addr = 0x28078C94, .value = 0 },
	{ .addr = 0x2807E6A0, .value = 0 },
};

/*
 * Encode RAM address to mem check bst format:
 *   64K  mem: valid_mask 0x4000, addr_mask 0x3FFF
 *   128K mem: valid_mask 0x8000, addr_mask 0x7FFF
 */
__IRAM_SEC uint16_t mem_check_encode_bad_point_addr(uint32_t ram_addr, uint32_t mem_base)
{
	const mem_check_block_cfg_t *cfg = mem_check_get_cfg_by_base(mem_base);
	uint32_t offset;

	if (cfg == NULL) {
		return 0;
	}

	if ((ram_addr < cfg->mem_base) || (ram_addr >= cfg->mem_end) || ((ram_addr - cfg->mem_base) & 0x3)) {
		return 0;
	}

	offset = (ram_addr - cfg->mem_base) >> 2;
	if (offset > cfg->addr_mask) {
		return 0;
	}

	return (uint16_t)((offset & cfg->addr_mask) | cfg->valid_mask);
}

__IRAM_SEC uint16_t mem_check_ram_addr_to_bst_high(uint32_t ram_addr)
{
	for (uint32_t blk = 0; blk < (sizeof(s_mem_check_block_cfgs) / sizeof(s_mem_check_block_cfgs[0])); blk++) {
		const mem_check_block_cfg_t *cfg = &s_mem_check_block_cfgs[blk];

		if ((ram_addr >= cfg->mem_base) && (ram_addr < cfg->mem_end)) {
			return mem_check_encode_bad_point_addr(ram_addr, cfg->mem_base);
		}
	}

	return 0;
}

/*
 * Pack two 16-bit mem check addresses into one OTP word (little-endian):
 *   word[15:0]  = 1st bst_high
 *   word[31:16] = 2nd bst_high
 */
__IRAM_SEC void mem_check_bad_points_write_otp(const mem_check_bad_point_t *points, uint32_t count)
{
	volatile uint32_t *otp_words = (volatile uint32_t *)OTP_APB_MEM_CHECK_MARK_BASE;
	uint32_t word_num = (count + 1U) / 2U;

	for (uint32_t w = 0; w < word_num; w++) {
		uint32_t idx = w * 2U;
		uint16_t lo = 0;
		uint16_t hi = 0;

		if ((idx < count) && (points[idx].addr != 0)) {
			lo = mem_check_ram_addr_to_bst_high(points[idx].addr);
		}
		if (((idx + 1U) < count) && (points[idx + 1].addr != 0)) {
			hi = mem_check_ram_addr_to_bst_high(points[idx + 1].addr);
		}

		otp_words[w] = (uint32_t)lo | ((uint32_t)hi << 16);
	}
}

void mem_check_test_bad_points_write_otp(void)
{
	mem_check_bad_points_write_otp(bad_points, MEM_CHECK_BST_POINT_NUM);
}

void mem_check_bad_points_print(const mem_check_bad_point_t *points, uint32_t count)
{
	static const char *blk_names[] = { "mem0", "mem1", "mem2", "mem3", "mem4" };

	for (uint32_t i = 0; i < count; i++) {
		uint32_t addr = points[i].addr;
		uint32_t val = (addr != 0) ? *(volatile uint32_t *)addr : 0;
		uint16_t bst = (addr != 0) ? mem_check_ram_addr_to_bst_high(addr) : 0;
		const char *blk = (i / 4U < 5U) ? blk_names[i / 4U] : "???";

		os_printf("[%s][%u] addr=0x%08X, value=0x%08X, bst=0x%04X\r\n",
			blk, i % 4U, addr, val, bst);
	}
}

void mem_check_test_bad_points_print(void)
{
	mem_check_bad_points_print(bad_points, MEM_CHECK_BST_POINT_NUM);
}

#endif
#endif
