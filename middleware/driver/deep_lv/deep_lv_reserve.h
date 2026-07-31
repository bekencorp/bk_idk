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
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MEM_CHECK_TEST
#define MEM_CHECK_TEST                    0
#endif

#define MEM_CHECK_BST_POINT_NUM           20
#define SYS_ANA_REGS_AREA_NUM             16
#define SYS_DIG_REGS_AREA_NUM             12

#define DEEP_LV_RESERVE_SIZE              0x200
#define DEEP_LV_RESERVE_MAGIC             0x444C5652u  /* "DLVR" */
#define DEEP_LV_RESERVE_MAGIC_BOOTROM_ADDR 0x2807FFF4u /* bootrom retention check */

typedef struct {
	uint32_t addr;  /* decoded bad point address, 0 if invalid */
	uint32_t value; /* 32-bit data stored at bad point address */
} mem_check_bad_point_t;

#define DEEP_LV_RESERVE_TAIL_WORDS        3u /* magic, app_msp_addr, app_entry_addr */
#define DEEP_LV_RESERVE_HEADER_WORDS      ((DEEP_LV_RESERVE_SIZE - \
	(MEM_CHECK_BST_POINT_NUM * (uint32_t)sizeof(mem_check_bad_point_t)) - \
	(SYS_ANA_REGS_AREA_NUM * (uint32_t)sizeof(uint32_t)) - \
	(SYS_DIG_REGS_AREA_NUM * (uint32_t)sizeof(uint32_t)) - \
	(DEEP_LV_RESERVE_TAIL_WORDS * (uint32_t)sizeof(uint32_t))) / \
	(uint32_t)sizeof(uint32_t))

#define DEEP_LV_RESERVE_MAGIC_OFFSET      (DEEP_LV_RESERVE_SIZE - 12u)
#define DEEP_LV_RESERVE_APP_MSP_OFFSET    (DEEP_LV_RESERVE_SIZE - 8u)
#define DEEP_LV_RESERVE_APP_ENTRY_OFFSET  (DEEP_LV_RESERVE_SIZE - 4u)

typedef struct {
	uint32_t reserved[DEEP_LV_RESERVE_HEADER_WORDS];
	mem_check_bad_point_t bad_points[MEM_CHECK_BST_POINT_NUM];
	uint32_t ana_regs[SYS_ANA_REGS_AREA_NUM];
	uint32_t dig_regs[SYS_DIG_REGS_AREA_NUM];
	uint32_t magic;          /* offset 0x1F4, RAM 0x2807EBF4, bootrom retention magic */
	uint32_t app_msp_addr;   /* offset 0x1F8, RAM 0x2807EBF8, bootrom fast-boot MSP */
	uint32_t app_entry_addr; /* offset 0x1FC, RAM 0x2807EBFC, bootrom fast-boot entry PC */
} deep_lv_reserve_t;

extern deep_lv_reserve_t g_deep_lv_reserve;

static inline int deep_lv_reserve_is_valid(const deep_lv_reserve_t *reserve)
{
	return reserve->magic == DEEP_LV_RESERVE_MAGIC;
}

uint32_t mem_check_decode_bad_point_addr(uint16_t bst_high, uint32_t mem_base);

#if MEM_CHECK_TEST
uint16_t mem_check_encode_bad_point_addr(uint32_t ram_addr, uint32_t mem_base);
uint16_t mem_check_ram_addr_to_bst_high(uint32_t ram_addr);
void mem_check_bad_points_write_otp(const mem_check_bad_point_t *points, uint32_t count);
void mem_check_bad_points_print(const mem_check_bad_point_t *points, uint32_t count);
void mem_check_test_bad_points_write_otp(void);
void mem_check_test_bad_points_print(void);
#endif

void sys_hal_mem_check_bad_point_addr_get(void);
void sys_hal_mem_check_bad_point_value_save(void);
void sys_hal_mem_check_bad_point_value_restore(void);

#ifdef __cplusplus
}
#endif
