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

#pragma once

#include <stdbool.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>

#define FLASH_IMPORT_SECTOR_SIZE          (4096U)
#define FLASH_IMPORT_PAGE_SIZE            (256U)

#define FLASH_IMPORT_APP_PART_ID          BK_PARTITION_APPLICATION
#define FLASH_IMPORT_APP_PART_NAME        "primary_cpu0_app"
#define FLASH_IMPORT_APP_PART_ALIAS       "application"

#define FLASH_IMPORT_FLASH_TEST_PART_ID   BK_PARTITION_FLASH_TEST
#define FLASH_IMPORT_FLASH_TEST_PART_NAME "flash_test"

#if CONFIG_FLASH_CBUS
#include "security.h"
#endif

#include "sys_driver.h"

flash_protect_type_t flash_import_get_protect(void);
flash_line_mode_t flash_import_set_line_mode(flash_line_mode_t line_mode);
void flash_import_set_protect(flash_protect_type_t protect);

typedef struct {
    uint32_t             flash_cksel;
    uint32_t             flash_ckdiv;
    flash_line_mode_t    line_mode;
    flash_protect_type_t protect_type;
} flash_import_context_t;

#define FLASH_IMPORT_CONTEXT_INITIALIZER \
    {                                    \
        .line_mode = FLASH_LINE_MODE_TWO, \
        .protect_type = FLASH_PROTECT_ALL, \
    }

static inline void flash_import_context_save(flash_import_context_t *ctx)
{
    ctx->flash_cksel = sys_drv_flash_get_clk_sel();
    ctx->flash_ckdiv = sys_drv_flash_get_clk_div();
    ctx->line_mode = bk_flash_get_line_mode();
    ctx->protect_type = flash_import_get_protect();
}

static inline void flash_import_context_restore(const flash_import_context_t *ctx)
{
    sys_drv_flash_set_clk_div(ctx->flash_ckdiv);
    sys_drv_flash_cksel(ctx->flash_cksel);
    (void)flash_import_set_line_mode(ctx->line_mode);
    flash_import_set_protect(ctx->protect_type);
}

bk_err_t flash_import_read_bytes(uint8_t *buffer, uint32_t address, uint32_t len);
bk_err_t flash_import_write_bytes(const uint8_t *buffer, uint32_t address, uint32_t len);
bk_err_t flash_import_erase_4k(uint32_t address);
#if CONFIG_FLASH_CBUS
bk_err_t flash_import_write_cbus(const uint8_t *buffer, uint32_t address, uint32_t len);
bk_err_t flash_import_read_cbus(uint8_t *buffer, uint32_t address, uint32_t len);
#endif

void flash_import_case_endurance_start(void);
void flash_import_case_endurance_stop(void);
void flash_import_case_endurance_show(void);
void flash_import_case_zero_read_start(void);
void flash_import_case_zero_read_stop(void);
void flash_import_case_zero_read_show(void);
void flash_import_case_driver_run(void);
void flash_import_case_loop_start(void);
void flash_import_case_loop_stop(void);
void flash_import_case_loop_show(void);
void flash_import_case_pd_start(uint32_t mode);
void flash_import_case_pd_stop(void);
void flash_import_case_pd_show(void);
void flash_import_case_soft_erase_start(void);
void flash_import_case_soft_erase_stop(void);
void flash_import_case_soft_erase_show(void);
#if CONFIG_FLASH_IMPORT_SLEEP_TEST
void flash_import_case_sleep_run(bool is_deep);
bk_err_t flash_import_case_sleep_resume(void);
#endif
