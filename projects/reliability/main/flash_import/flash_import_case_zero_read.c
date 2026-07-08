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

#include <os/os.h>
#include <os/mem.h>
#include <stdint.h>
#include "flash_import_internal.h"

#define FLASH_IMPORT_ZERO_READ_ADDR         (0x0U)
#define FLASH_IMPORT_ZERO_READ_SIZE         (512U)
#define FLASH_IMPORT_ZERO_READ_LOG_INTERVAL (100000U)

typedef struct {
    uint32_t loop_cnt;
    uint32_t read_cnt;
    uint32_t read_err_cnt;
    uint32_t cmp_err_cnt;
} flash_import_zero_read_stats_t;

static beken_thread_t s_zero_read_task_handle = NULL;
static bool s_zero_read_stop_requested = false;
static bool s_zero_read_context_saved = false;
static flash_import_zero_read_stats_t s_zero_read_stats = {0};
static flash_import_context_t s_zero_read_saved_ctx = FLASH_IMPORT_CONTEXT_INITIALIZER;
static uint8_t *s_zero_read_baseline_buf = NULL;
static uint8_t *s_zero_read_read_buf = NULL;

static bk_err_t flash_import_zero_read_init(void)
{
    bk_err_t ret = BK_OK;

    flash_import_context_save(&s_zero_read_saved_ctx);
    s_zero_read_context_saved = true;

    if (bk_flash_set_clk_freq(FLASH_CLK_FREQ_80M_HZ) != BK_OK) {
        os_printf("zero_read set clk 80M fail\r\n");
        return BK_FAIL;
    }

    s_zero_read_baseline_buf = (uint8_t *)os_malloc(FLASH_IMPORT_ZERO_READ_SIZE);
    if (s_zero_read_baseline_buf == NULL) {
        os_printf("zero_read baseline buf alloc fail\r\n");
        return BK_FAIL;
    }

    s_zero_read_read_buf = (uint8_t *)os_malloc(FLASH_IMPORT_ZERO_READ_SIZE);
    if (s_zero_read_read_buf == NULL) {
        os_printf("zero_read read buf alloc fail\r\n");
        return BK_FAIL;
    }

    s_zero_read_saved_ctx.line_mode = flash_import_set_line_mode(FLASH_LINE_MODE_FOUR);
    flash_import_set_protect(FLASH_PROTECT_ALL);

    ret = flash_import_read_bytes(s_zero_read_baseline_buf,
                                  FLASH_IMPORT_ZERO_READ_ADDR,
                                  FLASH_IMPORT_ZERO_READ_SIZE);
    if (ret != BK_OK) {
        os_printf("zero_read baseline read fail:%d\r\n", ret);
        return ret;
    }

    return BK_OK;
}

static void flash_import_zero_read_main_loop(void)
{
    while (1) {
        if (s_zero_read_stop_requested) {
            break;
        }

        bk_err_t ret = BK_OK;

        s_zero_read_stats.loop_cnt++;
        os_memset(s_zero_read_read_buf, 0, FLASH_IMPORT_ZERO_READ_SIZE);
        ret = flash_import_read_bytes(s_zero_read_read_buf,
                                      FLASH_IMPORT_ZERO_READ_ADDR,
                                      FLASH_IMPORT_ZERO_READ_SIZE);
        if (ret != BK_OK) {
            s_zero_read_stats.read_err_cnt++;
            continue;
        }
        s_zero_read_stats.read_cnt++;

        int cmp_ret = os_memcmp(s_zero_read_read_buf,
                                s_zero_read_baseline_buf,
                                FLASH_IMPORT_ZERO_READ_SIZE);
        if (cmp_ret != 0) {
            s_zero_read_stats.cmp_err_cnt++;
            os_printf("zero_read cmp err loop:%u\r\n", s_zero_read_stats.loop_cnt);
        }

        if ((s_zero_read_stats.loop_cnt % FLASH_IMPORT_ZERO_READ_LOG_INTERVAL) == 0U) {
            os_printf("zero_read loop:%u read:%u rerr:%u cerr:%u\r\n",
                      s_zero_read_stats.loop_cnt,
                      s_zero_read_stats.read_cnt,
                      s_zero_read_stats.read_err_cnt,
                      s_zero_read_stats.cmp_err_cnt);
        }
    }

    os_printf("zero_read stopped loop:%u read:%u rerr:%u cerr:%u\r\n",
              s_zero_read_stats.loop_cnt,
              s_zero_read_stats.read_cnt,
              s_zero_read_stats.read_err_cnt,
              s_zero_read_stats.cmp_err_cnt);
}

static void flash_import_zero_read_deinit(void)
{
    if (s_zero_read_context_saved) {
        flash_import_context_restore(&s_zero_read_saved_ctx);
        s_zero_read_context_saved = false;
    }

    if (s_zero_read_baseline_buf != NULL) {
        os_free(s_zero_read_baseline_buf);
        s_zero_read_baseline_buf = NULL;
    }

    if (s_zero_read_read_buf != NULL) {
        os_free(s_zero_read_read_buf);
        s_zero_read_read_buf = NULL;
    }

    s_zero_read_task_handle = NULL;
    rtos_delete_thread(NULL);
}

static void flash_import_zero_read_thread(beken_thread_arg_t arg)
{
    (void)arg;

    if (flash_import_zero_read_init() == BK_OK) {
        flash_import_zero_read_main_loop();
    }

    flash_import_zero_read_deinit();
}

void flash_import_case_zero_read_start(void)
{
    bk_err_t ret = BK_OK;

    if (s_zero_read_task_handle != NULL) {
        os_printf("zero_read already running\r\n");
        return;
    }

    os_memset(&s_zero_read_stats, 0, sizeof(s_zero_read_stats));
    s_zero_read_stop_requested = false;
    ret = rtos_create_thread(&s_zero_read_task_handle,
                             4,
                             "flash_import_zero_read",
                             (beken_thread_function_t)flash_import_zero_read_thread,
                             4096,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK) {
        os_printf("zero_read create thread fail:%d\r\n", ret);
        return;
    }
}

void flash_import_case_zero_read_stop(void)
{
    if (s_zero_read_task_handle != NULL) {
        s_zero_read_stop_requested = true;
        os_printf("zero_read stop requested\r\n");
    } else {
        os_printf("zero_read not running\r\n");
    }
}

void flash_import_case_zero_read_show(void)
{
    os_printf("\r\nBuild_info:%s_%s\r\n", __DATE__, __TIME__);
    os_printf("flash id:%lx\r\n", bk_flash_get_id());
    os_printf("zero_read task:%s\r\n", (s_zero_read_task_handle != NULL) ? "running" : "stopped");
    os_printf("zero_read addr:0x%08x size:%u\r\n", FLASH_IMPORT_ZERO_READ_ADDR, FLASH_IMPORT_ZERO_READ_SIZE);
    os_printf("zero_read loop_cnt:%u\r\n", s_zero_read_stats.loop_cnt);
    os_printf("zero_read read_cnt:%u\r\n", s_zero_read_stats.read_cnt);
    os_printf("zero_read read_err_cnt:%u\r\n", s_zero_read_stats.read_err_cnt);
    os_printf("zero_read cmp_err_cnt:%u\r\n", s_zero_read_stats.cmp_err_cnt);
}
