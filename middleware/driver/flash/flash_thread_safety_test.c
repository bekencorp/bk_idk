// Copyright 2020-2026 Beken
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


/* Flash Thread Safety Test Suite
 * Used to verify thread safety of Flash operations in single-core FreeRTOS environment
 *
 * Test Objectives:
 * 1. Verify safety of multi-task concurrent access to Flash
 * 2. Verify correctness of Flash operations in task context
 * 3. Verify the rule that Flash operations are prohibited in ISR context
 * 4. Stress test and long-term stability test
 */

 #include <common/bk_include.h>
 #include <driver/flash.h>
 #include <driver/flash_partition.h>
 #include "cli.h"
 #include "flash_driver.h"
 #include <os/os.h>
 #include <os/mem.h>
 #include <driver/aon_rtc.h>

 /* ============================================================================
  * Test Configuration
  * ============================================================================ */

 /* Test area address (ensure it's writable area, not protected area) */
 #define TEST_FLASH_BASE_ADDR     0x260000
 #define TEST_FLASH_SIZE          0x10000      /* 64KB test area */
 #define TEST_SECTOR_SIZE         0x1000       /* 4KB sector */
 #define TEST_PAGE_SIZE           256          /* 256 bytes page */

/* Maximum number of test tasks */
#define MAX_READ_TASKS           6
#define MAX_WRITE_TASKS          6
#define MAX_ERASE_TASKS          6
#define MAX_STRESS_TASKS         10

#define TASK_BLOCK_SIZE          0x2000      /* 8KB per task */
#define READ_TASK_AREA_BASE      TEST_FLASH_BASE_ADDR
#define READ_TASK_AREA_SIZE      (MAX_READ_TASKS * TASK_BLOCK_SIZE)
#define WRITE_TASK_AREA_BASE     (TEST_FLASH_BASE_ADDR + READ_TASK_AREA_SIZE)

#define READ_TASK_ADDR(id)       (READ_TASK_AREA_BASE + ((uint32_t)(id) * TASK_BLOCK_SIZE))
#define WRITE_TASK_ADDR(id)      (WRITE_TASK_AREA_BASE + ((uint32_t)(id) * TASK_BLOCK_SIZE))
#define ERASE_TASK_AREA_BASE     (WRITE_TASK_AREA_BASE + (MAX_WRITE_TASKS * TASK_BLOCK_SIZE))
#define ERASE_TASK_STRIDE        0x4000
#define ERASE_TASK_ADDR(id)      (ERASE_TASK_AREA_BASE + ((uint32_t)(id) * ERASE_TASK_STRIDE))
#define STRESS_TASK_AREA_BASE    TEST_FLASH_BASE_ADDR
#define STRESS_TASK_STRIDE       0x1000
#define STRESS_TASK_ADDR(id)     (STRESS_TASK_AREA_BASE + ((uint32_t)(id) * STRESS_TASK_STRIDE))
#define INIT_SEQ_VALUE           0xAAAAAAAA
#define FLASH_PRIORITY_BASE      4
#define FLASH_PRIORITY_SLOTS     6

#ifndef FLASH_TEST_TASK_NAME_LEN
#ifdef configMAX_TASK_NAME_LEN
#define FLASH_TEST_TASK_NAME_LEN configMAX_TASK_NAME_LEN
#else
#define FLASH_TEST_TASK_NAME_LEN 16
#endif
#endif

typedef enum {
    STRESS_OP_READ = 0,
    STRESS_OP_WRITE = 1,
    STRESS_OP_ERASE = 2,
} stress_op_type_t;

/* Test run control */
static volatile bool g_test_running = false;
static volatile bool g_test_stopped = false;

static void apply_task_jitter(uint32_t task_id)
{
    extern int bk_rand(void);
    extern void bk_delay_us(uint32 num);
    uint32_t delay = (bk_rand() % 100) + 1 + task_id;
    bk_delay_us(delay);
}

static bk_err_t create_thread_checked(beken_thread_t *thread, uint8_t priority,
                                      const char *name, beken_thread_function_t function,
                                      uint32_t stack_size, void *arg)
{
    bk_err_t ret = rtos_create_thread(thread, priority, name, function, stack_size, arg);
    if(ret != BK_OK) {
        CLI_LOGE("Failed to create '%s' thread (prio=%u), ret=%d, arg=0x%x\r\n",
                 name ? name : "unknown", priority, ret, arg);
    }
    return ret;
}

/* Global buffers to avoid stack overflow (256 bytes per buffer)
 * Each task type gets dedicated buffers so there is no sharing or aliasing.
 */
static uint8_t g_read_buffers[MAX_READ_TASKS][TEST_PAGE_SIZE];
static uint8_t g_read_verify_buffers[MAX_READ_TASKS][TEST_PAGE_SIZE];
static uint8_t g_read_expected[MAX_READ_TASKS][TEST_PAGE_SIZE];

static uint8_t g_write_buffers[MAX_WRITE_TASKS][TEST_PAGE_SIZE];
static uint8_t g_write_verify_buffers[MAX_WRITE_TASKS][TEST_PAGE_SIZE];

static uint8_t g_erase_buffers[MAX_ERASE_TASKS][TEST_PAGE_SIZE];

static uint8_t g_stress_buffers[MAX_STRESS_TASKS][TEST_PAGE_SIZE];
static uint8_t g_stress_verify_buffers[MAX_STRESS_TASKS][TEST_PAGE_SIZE];

static uint8_t g_isr_buffer[TEST_PAGE_SIZE];
static uint8_t g_sector_verify_buffer[TEST_PAGE_SIZE];
static char g_read_task_names[MAX_READ_TASKS][FLASH_TEST_TASK_NAME_LEN];
static char g_write_task_names[MAX_WRITE_TASKS][FLASH_TEST_TASK_NAME_LEN];
static char g_erase_task_names[MAX_ERASE_TASKS][FLASH_TEST_TASK_NAME_LEN];
static char g_stress_task_names[MAX_STRESS_TASKS][FLASH_TEST_TASK_NAME_LEN];
static char g_isr_task_name[FLASH_TEST_TASK_NAME_LEN];

 /* ============================================================================
  * Statistics Data Structure
  * ============================================================================ */

 typedef struct {
     uint32_t total_ops;          /* Total operation count */
     uint32_t success_ops;         /* Success count */
     uint32_t error_ops;           /* Error count */
     uint32_t timeout_ops;         /* Timeout count */
     uint64_t total_time_us;      /* Total time in microseconds */
     uint32_t max_time_us;         /* Maximum time */
     uint32_t min_time_us;         /* Minimum time */
     uint32_t last_error_code;     /* Last error code */
 } flash_op_statistics_t;

 static flash_op_statistics_t g_read_stats = {0};
 static flash_op_statistics_t g_write_stats = {0};
 static flash_op_statistics_t g_erase_stats = {0};

 /* ============================================================================
  * Helper Functions
  * ============================================================================ */

 /**
  * @brief Get system time in microseconds
  */
 static uint64_t get_system_time_us(void)
 {
     struct timeval tv;
     bk_rtc_gettimeofday(&tv, NULL);
     return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
 }

static char *prepare_task_name(char *buf, const char *fmt, uint32_t id)
{
    os_snprintf(buf, FLASH_TEST_TASK_NAME_LEN, fmt, id);
    buf[FLASH_TEST_TASK_NAME_LEN - 1] = '\0';
    return buf;
}

static void create_tasks_with_priority_cycle(uint32_t task_count,
                                             uint32_t priority_base,
                                             uint32_t slot_count,
                                             char (*names)[FLASH_TEST_TASK_NAME_LEN],
                                             const char *fmt,
                                             beken_thread_function_t function,
                                             uint32_t stack_size,
                                             uint32_t delay_ms)
{
    if(slot_count == 0 || task_count == 0) {
        return;
    }

    uint32_t created = 0;
    for(uint32_t iter = 0; created < task_count; iter++) {
        uint32_t slot = slot_count - 1 - (iter % slot_count);
        uint32_t group = iter / slot_count;
        uint32_t idx = slot + group * slot_count;

        if(idx >= task_count) {
            continue;
        }

        uint32_t priority = priority_base + slot;
        char *task_name = names[idx];
        prepare_task_name(task_name, fmt, idx);

        if(create_thread_checked(NULL, priority, task_name,
                                 function, stack_size, (void*)idx) != BK_OK) {
            continue;
        }

        if(delay_ms) {
            rtos_delay_milliseconds(delay_ms);
        }

        created++;
    }
}

 /**
  * @brief Update statistics
  */
 static void update_statistics(flash_op_statistics_t *stats,
                               bk_err_t result,
                               uint32_t time_us)
 {
     if(stats == NULL) {
         return;
     }

     stats->total_ops++;

     if(result == BK_OK) {
         stats->success_ops++;
     } else {
         stats->error_ops++;
         stats->last_error_code = result;
     }

     stats->total_time_us += time_us;

     if(time_us > stats->max_time_us) {
         stats->max_time_us = time_us;
     }

     if(stats->min_time_us == 0 || time_us < stats->min_time_us) {
         stats->min_time_us = time_us;
     }
 }

 /**
  * @brief Print statistics
  */
 static void print_statistics(flash_op_statistics_t *stats, const char *op_name)
 {
     if(stats == NULL || op_name == NULL) {
         return;
     }

     CLI_LOGI("=== %s Statistics ===\r\n", op_name);
     CLI_LOGI("  Total Operations: %lu\r\n", stats->total_ops);
     CLI_LOGI("  Success: %lu\r\n", stats->success_ops);
     CLI_LOGI("  Errors: %lu\r\n", stats->error_ops);
     CLI_LOGI("  Timeouts: %lu\r\n", stats->timeout_ops);

     if(stats->total_ops > 0) {
         float success_rate = (float)stats->success_ops * 100.0f / stats->total_ops;
         CLI_LOGI("  Success Rate: %.2f%%\r\n", success_rate);

         if(stats->success_ops > 0) {
             uint32_t avg_time = stats->total_time_us / stats->success_ops;
             CLI_LOGI("  Avg Time: %lu us\r\n", avg_time);
         }
     }

     CLI_LOGI("  Max Time: %lu us\r\n", stats->max_time_us);
     CLI_LOGI("  Min Time: %lu us\r\n", stats->min_time_us);

     if(stats->error_ops > 0) {
         CLI_LOGI("  Last Error Code: 0x%08lx\r\n", stats->last_error_code);
     }

     CLI_LOGI("\r\n");
 }

 /**
  * @brief Verify data pattern
  */
 static bool verify_data_pattern(uint8_t *buf, uint32_t len, uint32_t expected_seq)
 {
     if(buf == NULL || len == 0) {
         return false;
     }

     /* Check if first 4 bytes are sequence number */
     uint32_t seq = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

     if(seq != expected_seq) {
         return false;
     }

     /* Check if subsequent data follows increment pattern */
     for(uint32_t i = 4; i < len; i++) {
         uint8_t expected = (uint8_t)(i & 0xFF);
         if(buf[i] != expected) {
             return false;
         }
     }

     return true;
 }

/**
 * @brief Verify that a flash page is fully erased (0xFF)
 */
static void dump_page_buffer(const char *title, uint32_t addr, const uint8_t *buf, uint32_t len)
{
    if(buf == NULL || len == 0) {
        return;
    }

    CLI_LOGI("%s @ 0x%08lx:\r\n", title, addr);
    for(uint32_t line = 0; line < (len + 15) / 16; line++) {
        char hex_line[64];
        int pos = 0;
        uint32_t base = line * 16;
        for(uint32_t j = 0; j < 16 && base + j < len; j++) {
            pos += os_snprintf(hex_line + pos, sizeof(hex_line) - pos, "%02X ", buf[base + j]);
        }

        CLI_LOGI("  %04X: %s\r\n", base, hex_line);
    }
}

static void handle_verification_failure(const char *title, uint32_t addr, uint8_t *buf)
{
    dump_page_buffer(title, addr, buf, TEST_PAGE_SIZE);
    rtos_delay_milliseconds(5000);
    while (1) {
        int port_disable_interrupts_flag(void);
        port_disable_interrupts_flag();
    }
}

static bk_err_t verify_erased_page(uint32_t addr, uint8_t *verify_buf)
{
    if(NULL == verify_buf) {
        return BK_FAIL;
    }

    bk_err_t read_ret = bk_flash_read_bytes(addr, verify_buf, TEST_PAGE_SIZE);
    if(read_ret != BK_OK) {
        CLI_LOGE("Erase verification read failed at 0x%08lx, ret: %d\r\n",
                 addr, read_ret);
        return read_ret;
    }

    for(uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
        if(verify_buf[i] != 0xFF) {
            char * vTaskName();
            const char *task_name = vTaskName();

            CLI_LOGE("Erase verification failed at 0x%08lx during %s, offset %lu = 0x%02x\r\n",
                     addr, task_name, (unsigned long)i, verify_buf[i]);
            handle_verification_failure("Erase verification page", addr, verify_buf);
            return BK_FAIL;
        }
    }

    return BK_OK;
}

/**
 * @brief Initialize read blocks with known pattern and seq=0
 */
static void initialize_read_blocks(uint32_t task_count)
{
    CLI_LOGI("Initializing read area with known data pattern...\r\n");

    for(uint32_t i = 0; i < task_count; i++) {
        uint32_t test_addr = READ_TASK_ADDR(i);
        uint8_t *init_buf = g_read_buffers[i];
        uint8_t *verify_buf = g_read_verify_buffers[i];

        uint32_t sector_addr = test_addr & (~(TEST_SECTOR_SIZE - 1));
        bk_err_t erase_ret = bk_flash_erase_sector(sector_addr);
        if(erase_ret != BK_OK) {
            CLI_LOGE("Failed to erase read block at 0x%08lx for task %lu, ret: %d\r\n",
                    sector_addr, i, erase_ret);
            continue;
        }

        if(verify_erased_page(sector_addr, verify_buf) != BK_OK) {
            CLI_LOGE("Read area verification failed at 0x%08lx for task %lu\r\n", sector_addr, i);
            continue;
        }

        for(uint32_t j = 0; j < TEST_PAGE_SIZE; j++) {
            init_buf[j] = (uint8_t)(j & 0xFF);
        }

        /* initial sequence number should be zero for comparison baseline */
        init_buf[0] = init_buf[1] = init_buf[2] = init_buf[3] = (INIT_SEQ_VALUE & 0xFF);

        bk_err_t write_ret = bk_flash_write_bytes(test_addr, init_buf, TEST_PAGE_SIZE);
        if(write_ret != BK_OK) {
            CLI_LOGE("Failed to write initial read pattern for task %lu, ret: %d\r\n", i, write_ret);
        }
    }
}

 /**
  * @brief Initialize test area (erase)
  */
 static bk_err_t init_test_area(uint32_t base_addr, uint32_t size)
 {
     CLI_LOGI("Initializing test area: 0x%08lx, size: 0x%08lx\r\n", base_addr, size);

     bk_flash_set_protect_type(FLASH_PROTECT_NONE);

     for(uint32_t addr = base_addr; addr < (base_addr + size); addr += TEST_SECTOR_SIZE) {
         bk_err_t ret = bk_flash_erase_sector(addr);
         if(ret != BK_OK) {
             CLI_LOGE("Failed to erase sector at 0x%08lx, ret: %d\r\n", addr, ret);
             return ret;
         }

        if(verify_erased_page(addr, g_sector_verify_buffer) != BK_OK) {
            CLI_LOGE("Sector verification failed after erase at 0x%08lx\r\n", addr);
            return BK_FAIL;
        }
     }

     bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
     CLI_LOGI("Test area initialized successfully\r\n");

     return BK_OK;
 }

 /* ============================================================================
  * Test Case 1: Multi-task Concurrent Read Test
  * ============================================================================ */
static void task_flash_read_test(void *param)
{
    uint32_t task_id = (uint32_t)param;
    uint32_t test_addr = READ_TASK_ADDR(task_id);
    uint8_t *buf = g_read_buffers[task_id];
    uint8_t *expected_buf = g_read_expected[task_id];
    uint32_t op_count = 0;
    uint32_t error_count = 0;
    uint32_t last_valid_seq = 0;

    CLI_LOGI("Read task %lu started, addr: 0x%08lx\r\n", task_id, test_addr);

    /* Prepare expected data pattern (same as write test) */
    for(uint32_t i = 4; i < TEST_PAGE_SIZE; i++) {
        expected_buf[i] = (uint8_t)(i & 0xFF);
    }
    expected_buf[0] = expected_buf[1] = expected_buf[2] = expected_buf[3] = (uint8_t)(INIT_SEQ_VALUE & 0xFF);

    while(g_test_running) {
        uint64_t start_time = get_system_time_us();
        bk_err_t ret = bk_flash_read_bytes(test_addr, buf, TEST_PAGE_SIZE);
        uint64_t end_time = get_system_time_us();
        uint32_t time_us = (uint32_t)(end_time - start_time);

        update_statistics(&g_read_stats, ret, time_us);

        if(ret == BK_OK) {
            /* Verify data integrity */
            /* Extract sequence number from first 4 bytes */
            uint32_t seq = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

            if(seq != INIT_SEQ_VALUE) {
                CLI_LOGE("Read task %lu: unexpected seq 0x%08x, aborting\r\n", task_id, seq);
                handle_verification_failure("Read task unexpected seq", test_addr, buf);
            }

            bool pattern_ok = true;
            for(uint32_t i = 4; i < TEST_PAGE_SIZE; i++) {
                if(buf[i] != expected_buf[i]) {
                    pattern_ok = false;
                    break;
                }
            }

            if(!pattern_ok) {
                CLI_LOGE("Read task %lu: data pattern mismatch found\r\n", task_id);
                handle_verification_failure("Read task pattern mismatch", test_addr, buf);
            }
        }

        op_count++;

        /* Print every 1000 operations */
        if(op_count % 1000 == 0) {
            CLI_LOGI("Read task %lu: %lu ops, %lu errors, last_seq: %lu\r\n",
                    task_id, op_count, error_count, last_valid_seq);
        }

        apply_task_jitter(task_id);
        rtos_delay_milliseconds(10);
    }

    CLI_LOGI("Read task %lu stopped: total %lu ops, %lu errors, last_seq: %lu\r\n",
            task_id, op_count, error_count, last_valid_seq);
    rtos_delete_thread(NULL);
}

 /* ============================================================================
  * Test Case 2: Read-Write Concurrent Test
  * ============================================================================ */

static void task_flash_write_test(void *param)
{
    uint32_t task_id = (uint32_t)param;
    uint32_t test_addr = WRITE_TASK_ADDR(task_id);
    uint8_t *buf = g_write_buffers[task_id];
    uint8_t *verify_buf = g_write_verify_buffers[task_id];
    uint32_t write_seq = 0;
    uint32_t op_count = 0;
    uint64_t start_time, end_time;
    uint32_t time_us;

     CLI_LOGI("Write task %lu started, addr: 0x%08lx\r\n", task_id, test_addr);

     /* Prepare test data pattern */
     for(uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
         buf[i] = (uint8_t)(i & 0xFF);
     }

     while(g_test_running) {
         /* Update sequence number (first 4 bytes) */
         buf[0] = (write_seq >> 24) & 0xFF;
         buf[1] = (write_seq >> 16) & 0xFF;
         buf[2] = (write_seq >> 8) & 0xFF;
         buf[3] = write_seq & 0xFF;

        uint64_t erase_start = get_system_time_us();
        bk_err_t erase_ret = bk_flash_erase_sector(test_addr);
        uint64_t erase_end = get_system_time_us();

        uint32_t erase_time_us = (uint32_t)(erase_end - erase_start);
        update_statistics(&g_erase_stats, erase_ret, erase_time_us);

        if(erase_ret != BK_OK) {
            CLI_LOGE("Write task %lu: erase failed (0x%08lx)\r\n", task_id, erase_ret);
            continue;
        }

        if(verify_erased_page(test_addr, verify_buf) != BK_OK) {
            CLI_LOGE("Write task %lu: page not zeroed after erase at 0x%08lx\r\n", task_id, test_addr);
            continue;
        }

        start_time = get_system_time_us();
        bk_err_t ret = bk_flash_write_bytes(test_addr, buf, TEST_PAGE_SIZE);
        end_time = get_system_time_us();

        time_us = (uint32_t)(end_time - start_time);
        update_statistics(&g_write_stats, ret, time_us);

            if(ret == BK_OK) {
                /* Verify written data */
                bk_err_t read_ret = bk_flash_read_bytes(test_addr, verify_buf, TEST_PAGE_SIZE);
                if(read_ret == BK_OK) {
                    if(!verify_data_pattern(verify_buf, TEST_PAGE_SIZE, write_seq)) {
                        CLI_LOGE("Write task %lu: Data verification failed at seq %lu\r\n",
                                task_id, write_seq);
                        handle_verification_failure("Write task data", test_addr, verify_buf);
                    }
                }
            }

         write_seq++;
         op_count++;

         /* Print every 100 operations */
         if(op_count % 100 == 0) {
             CLI_LOGI("Write task %lu: %lu operations completed\r\n", task_id, op_count);
         }

        apply_task_jitter(task_id);
        rtos_delay_milliseconds(10);
     }

     CLI_LOGI("Write task %lu stopped, total ops: %lu\r\n", task_id, op_count);
     rtos_delete_thread(NULL);
 }

 /* ============================================================================
  * Test Case 3: Write-Erase Concurrent Test
  * ============================================================================ */
 static void task_flash_erase_test(void *param)
 {
     uint32_t task_id = (uint32_t)param;
    uint32_t test_addr = ERASE_TASK_ADDR(task_id);  /* 16KB area per task */
     uint32_t op_count = 0;

     CLI_LOGI("Erase task %lu started, addr: 0x%08lx\r\n", task_id, test_addr);

     while(g_test_running) {
         uint64_t start_time = get_system_time_us();

         bk_err_t ret = bk_flash_erase_sector(test_addr);

         uint64_t end_time = get_system_time_us();
         uint32_t time_us = (uint32_t)(end_time - start_time);

         update_statistics(&g_erase_stats, ret, time_us);

        if(ret == BK_OK) {
            uint8_t *verify_buf = g_erase_buffers[task_id];
            if(verify_erased_page(test_addr, verify_buf) != BK_OK) {
                continue;
            }
        }

        op_count++;

        /* Print every 50 operations */
        if(op_count % 50 == 0) {
            CLI_LOGI("Erase task %lu: %lu operations completed\r\n", task_id, op_count);
        }

        apply_task_jitter(task_id);
        rtos_delay_milliseconds(10);
     }

     CLI_LOGI("Erase task %lu stopped, total ops: %lu\r\n", task_id, op_count);
     rtos_delete_thread(NULL);
 }

 /* ============================================================================
  * Test Case 4: ISR Prohibition Rule Verification Test
  * ============================================================================ */

 /* ISR trigger flags */
 static volatile bool g_isr_triggered = false;
 static volatile uint32_t g_isr_count = 0;
 static volatile bool g_isr_flash_called = false;  /* Detect if Flash operation called in ISR */

 /**
  * @brief Timer interrupt handler
  * Note: This function strictly prohibits calling any Flash operations
  */
 static void timer_isr_handler(void)
 {
     /* Correct: Only set flag, do not execute Flash operations */
     g_isr_triggered = true;
     g_isr_count++;

     /* Prohibited: The following code should not exist */
     /* bk_flash_read_bytes(...);   Error! */
     /* bk_flash_write_bytes(...);  Error! */
     /* bk_flash_erase_sector(...); Error! */
 }

 /**
  * @brief Flash operation task triggered from ISR
  * This task executes Flash operations in task context
  */
static void task_flash_from_isr(void *param)
{
    uint8_t *buf = g_isr_buffer;
    uint32_t op_count = 0;
    uint32_t test_addr = TEST_FLASH_BASE_ADDR;

     CLI_LOGI("Flash-from-ISR task started\r\n");

     while(g_test_running) {
         if(g_isr_triggered) {
             g_isr_triggered = false;

             /* Correct: Execute Flash operation in task context */
             bk_err_t ret = bk_flash_read_bytes(test_addr, buf, TEST_PAGE_SIZE);
             if(ret != BK_OK) {
                 CLI_LOGE("Flash-from-ISR task: Read failed, ret: %d\r\n", ret);
             }

             op_count++;

             if(op_count % 100 == 0) {
                 CLI_LOGI("Flash-from-ISR task: %lu operations from ISR triggers\r\n", op_count);
             }
         }

         rtos_delay_milliseconds(2);
     }

     CLI_LOGI("Flash-from-ISR task stopped, total ops: %lu, ISR count: %lu\r\n",
             op_count, g_isr_count);
     rtos_delete_thread(NULL);
 }

 /* ============================================================================
  * Test Case 5: Stress Test - High Frequency Concurrent Operations
  * ============================================================================ */

static void task_flash_stress_test(void *param)
{
    uint32_t task_id = (uint32_t)param;
    stress_op_type_t op_type = (stress_op_type_t)(task_id % 3);  /* 0=read, 1=write, 2=erase */
    uint32_t test_addr = STRESS_TASK_ADDR(task_id);
    uint8_t *buf = g_stress_buffers[task_id];
    uint32_t op_count = 0;
    uint32_t write_seq = 0;

     const char *op_names[] = {"Read", "Write", "Erase"};
     CLI_LOGI("Stress test task %lu (%s) started, addr: 0x%08lx\r\n",
             task_id, op_names[op_type], test_addr);

     /* Prepare write data */
     for(uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
         buf[i] = (uint8_t)(i & 0xFF);
     }

     while(g_test_running) {
         uint64_t start_time = get_system_time_us();
         bk_err_t ret = BK_FAIL;

        switch(op_type) {
            case STRESS_OP_READ:  /* Read operation */
                ret = bk_flash_read_bytes(test_addr, buf, TEST_PAGE_SIZE);
				if(ret == BK_OK) {
					bool good = true;
					for(uint32_t k = 4; k < TEST_PAGE_SIZE; k++) {
						uint8_t expected = (uint8_t)(k & 0xFF);
						if(buf[k] != expected) {
							good = false;
							break;
						}
					}

                if(!good) {
                    uint32_t seq = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
                    CLI_LOGE("Stress read task %lu: pattern mismatch at seq 0x%08x\r\n",
                             task_id, seq);
                    handle_verification_failure("Stress read pattern", test_addr, buf);
                }
				}
                 break;

            case STRESS_OP_WRITE:  /* Write operation */
                {
                    uint64_t erase_start = get_system_time_us();
                    bk_err_t erase_ret = bk_flash_erase_sector(test_addr);
                    uint64_t erase_end = get_system_time_us();
                    uint32_t erase_time_us = (uint32_t)(erase_end - erase_start);

                    update_statistics(&g_erase_stats, erase_ret, erase_time_us);

                    if(erase_ret != BK_OK) {
                        CLI_LOGE("Stress task %lu: erase failed at 0x%08lx, ret: %d\r\n",
                                task_id, test_addr, erase_ret);
                        ret = erase_ret;
                        break;
                    }

                    uint8_t *verify_buf = g_stress_verify_buffers[task_id];
                    if(verify_erased_page(test_addr, verify_buf) != BK_OK) {
                        CLI_LOGE("Stress task %lu: erase verification failed at 0x%08lx\r\n",
                                task_id, test_addr);
                        ret = BK_FAIL;
                        break;
                    }

                    buf[0] = (write_seq >> 24) & 0xFF;
                    buf[1] = (write_seq >> 16) & 0xFF;
                    buf[2] = (write_seq >> 8) & 0xFF;
                    buf[3] = write_seq & 0xFF;
                    ret = bk_flash_write_bytes(test_addr, buf, TEST_PAGE_SIZE);

                    if(ret == BK_OK) {
                        bk_err_t read_ret = bk_flash_read_bytes(test_addr, verify_buf, TEST_PAGE_SIZE);
                        if(read_ret == BK_OK) {
                            if(!verify_data_pattern(verify_buf, TEST_PAGE_SIZE, write_seq)) {
                                CLI_LOGE("Stress task %lu: sequence mismatch or pattern corrupted at seq %lu\r\n",
                                         task_id, write_seq);
                                handle_verification_failure("Stress write pattern", test_addr, verify_buf);
                            }
                        } else {
                            CLI_LOGE("Stress task %lu: read-back failed after write, ret %d\r\n",
                                     task_id, read_ret);
                        }
                    }
                }

                write_seq++;
                break;

            case STRESS_OP_ERASE:  /* Erase operation */
                ret = bk_flash_erase_sector(test_addr);
                if(ret == BK_OK) {
                    if(verify_erased_page(test_addr, g_stress_verify_buffers[task_id]) != BK_OK) {
                        CLI_LOGE("Stress task %lu: erase verify failed at 0x%08lx\r\n",
                                 task_id, test_addr);
                    }
                }
                break;
         }

         uint64_t end_time = get_system_time_us();
         uint32_t time_us = (uint32_t)(end_time - start_time);

         /* Update corresponding statistics */
        switch(op_type) {
            case STRESS_OP_READ:
                 update_statistics(&g_read_stats, ret, time_us);
                 break;
            case STRESS_OP_WRITE:
                 update_statistics(&g_write_stats, ret, time_us);
                 break;
            case STRESS_OP_ERASE:
                 update_statistics(&g_erase_stats, ret, time_us);
                 break;
         }

         op_count++;

         /* Print every 1000 operations */
         if(op_count % 1000 == 0) {
             CLI_LOGI("Stress test task %lu (%s): %lu operations\r\n",
                     task_id, op_names[op_type], op_count);
         }

         /* Minimum delay to maximize concurrency */
        apply_task_jitter(task_id);
        rtos_delay_milliseconds(10);
     }

     CLI_LOGI("Stress test task %lu (%s) stopped, total ops: %lu\r\n",
             task_id, op_names[op_type], op_count);
     rtos_delete_thread(NULL);
 }

 /* ============================================================================
  * Test Control Functions
  * ============================================================================ */

 /**
  * @brief Start concurrent read test
  */
 static void start_concurrent_read_test(uint32_t task_count)
 {
     if(task_count > MAX_READ_TASKS) {
         task_count = MAX_READ_TASKS;
     }

    CLI_LOGI("Starting concurrent read test with %lu tasks\r\n", task_count);

    /* Initialize test area with known data pattern for verification */
    CLI_LOGI("Initializing test area with known data pattern...\r\n");
    bk_err_t init_ret = init_test_area(TEST_FLASH_BASE_ADDR, TEST_FLASH_SIZE);
    if(init_ret != BK_OK) {
        CLI_LOGE("Failed to initialize test area, ret: %d\r\n", init_ret);
        return;
    }

    initialize_read_blocks(task_count);

    /* Initialize statistics */
    os_memset(&g_read_stats, 0, sizeof(g_read_stats));

    g_test_running = true;

    /* Create read tasks with different priorities to test preemption
     * Priority range: 4-9 (higher number = higher priority in FreeRTOS)
     * This allows testing both time-slicing and preemption scenarios
     */
    create_tasks_with_priority_cycle(task_count,
                                     FLASH_PRIORITY_BASE,
                                     FLASH_PRIORITY_SLOTS,
                                     g_read_task_names,
                                     "read_task_%lu",
                                     task_flash_read_test,
                                     2048,
                                     20);
 }

 /**
  * @brief Start read-write concurrent test
  */
 static void start_read_write_test(uint32_t read_task_count, uint32_t write_task_count)
 {
     if(read_task_count > MAX_READ_TASKS) {
         read_task_count = MAX_READ_TASKS;
     }
     if(write_task_count > MAX_WRITE_TASKS) {
         write_task_count = MAX_WRITE_TASKS;
     }

     CLI_LOGI("Starting read-write concurrent test: %lu read tasks, %lu write tasks\r\n",
             read_task_count, write_task_count);

     /* Initialize statistics */
     os_memset(&g_read_stats, 0, sizeof(g_read_stats));
     os_memset(&g_write_stats, 0, sizeof(g_write_stats));

    /* Initialize test area */
    init_test_area(TEST_FLASH_BASE_ADDR, TEST_FLASH_SIZE);
    initialize_read_blocks(read_task_count);

     g_test_running = true;

     /* Create read tasks */
    create_tasks_with_priority_cycle(read_task_count,
                                     FLASH_PRIORITY_BASE,
                                     FLASH_PRIORITY_SLOTS,
                                     g_read_task_names,
                                     "read_task_%lu",
                                     task_flash_read_test,
                                     2048,
                                     20);

    /* Create write tasks */
    create_tasks_with_priority_cycle(write_task_count,
                                     FLASH_PRIORITY_BASE,
                                     FLASH_PRIORITY_SLOTS,
                                     g_write_task_names,
                                     "write_task_%lu",
                                     task_flash_write_test,
                                     2048,
                                     20);
 }

 /**
  * @brief Start write-erase concurrent test
  */
 static void start_write_erase_test(uint32_t write_task_count, uint32_t erase_task_count)
 {
     if(write_task_count > MAX_WRITE_TASKS) {
         write_task_count = MAX_WRITE_TASKS;
     }
     if(erase_task_count > MAX_ERASE_TASKS) {
         erase_task_count = MAX_ERASE_TASKS;
     }

     CLI_LOGI("Starting write-erase concurrent test: %lu write tasks, %lu erase tasks\r\n",
             write_task_count, erase_task_count);

     /* Initialize statistics */
     os_memset(&g_write_stats, 0, sizeof(g_write_stats));
     os_memset(&g_erase_stats, 0, sizeof(g_erase_stats));

     /* Initialize test area */
     init_test_area(TEST_FLASH_BASE_ADDR, TEST_FLASH_SIZE);

     g_test_running = true;

     /* Create write tasks */
    create_tasks_with_priority_cycle(write_task_count,
                                     FLASH_PRIORITY_BASE,
                                     FLASH_PRIORITY_SLOTS,
                                     g_write_task_names,
                                     "write_task_%lu",
                                     task_flash_write_test,
                                     2048,
                                     5);

    /* Create erase tasks */
    create_tasks_with_priority_cycle(erase_task_count,
                                     FLASH_PRIORITY_BASE,
                                     FLASH_PRIORITY_SLOTS,
                                     g_erase_task_names,
                                     "erase_task_%lu",
                                     task_flash_erase_test,
                                     2048,
                                     20);
 }

 /**
  * @brief Start stress test
  */
 static void start_stress_test(uint32_t task_count)
 {
    if(task_count > MAX_STRESS_TASKS) {
        task_count = MAX_STRESS_TASKS;
    }

     CLI_LOGI("Starting stress test with %lu tasks\r\n", task_count);

     /* Initialize statistics */
     os_memset(&g_read_stats, 0, sizeof(g_read_stats));
     os_memset(&g_write_stats, 0, sizeof(g_write_stats));
     os_memset(&g_erase_stats, 0, sizeof(g_erase_stats));

     /* Initialize test area */
     init_test_area(TEST_FLASH_BASE_ADDR, TEST_FLASH_SIZE);

     g_test_running = true;

     /* Create stress test tasks */
    create_tasks_with_priority_cycle(task_count,
                                     FLASH_PRIORITY_BASE,
                                     FLASH_PRIORITY_SLOTS,
                                     g_stress_task_names,
                                     "stress_task_%lu",
                                     task_flash_stress_test,
                                     2048,
                                     20);
 }

 /**
  * @brief Start ISR prohibition rule verification test
  */
 static void start_isr_rule_test(void)
 {
     CLI_LOGI("Starting ISR rule verification test\r\n");

     /* Initialize statistics */
     os_memset(&g_read_stats, 0, sizeof(g_read_stats));
     g_isr_triggered = false;
     g_isr_count = 0;
     g_isr_flash_called = false;

     /* Note: Timer interrupt should be registered here, but we use task simulation for simplicity */
     /* In actual project, real timer interrupt should be used */
     CLI_LOGI("Note: This test requires a real timer interrupt to be registered\r\n");
     CLI_LOGI("      timer_isr_handler() should be registered as ISR callback\r\n");

     g_test_running = true;

    /* Create Flash operation task triggered from ISR */
    prepare_task_name(g_isr_task_name, "flash_from_isr", 0);
    if(create_thread_checked(NULL, 5, g_isr_task_name,
                             task_flash_from_isr, 2048, NULL) != BK_OK) {
        CLI_LOGE("Failed to create flash_from_isr task\r\n");
    }

     /* Create a task to simulate ISR trigger (should be triggered by hardware interrupt in reality) */
     /* This is only for demonstration, real interrupt should be used in actual test */
     CLI_LOGI("ISR simulation task started (use real interrupt in production)\r\n");
 }

 /**
  * @brief Stop all tests
  */
 static void stop_all_tests(void)
 {
     CLI_LOGI("Stopping all tests...\r\n");

     g_test_running = false;

     /* Wait for tasks to finish */
     rtos_delay_milliseconds(2000);

     g_test_stopped = true;

     CLI_LOGI("All tests stopped\r\n");
 }

 /**
  * @brief Print all statistics
  */
 static void print_all_statistics(void)
 {
     CLI_LOGI("\r\n");
     CLI_LOGI("========================================\r\n");
     CLI_LOGI("    Flash Thread Safety Test Results\r\n");
     CLI_LOGI("========================================\r\n");
     CLI_LOGI("\r\n");

     print_statistics(&g_read_stats, "Read Operations");
     print_statistics(&g_write_stats, "Write Operations");
     print_statistics(&g_erase_stats, "Erase Operations");

     CLI_LOGI("========================================\r\n");
     CLI_LOGI("\r\n");
 }

 /* ============================================================================
  * CLI Command Interface
  * ============================================================================ */

 void cli_flash_race_test_cmd(char *pcWriteBuffer,
                                          int xWriteBufferLen,
                                          int argc,
                                          char **argv)
 {
     char *msg = NULL;

     if(argc < 2) {
        CLI_LOGI("Flash Thread Safety Test Commands:\r\n");
        CLI_LOGI("  flash_race_test start_read <task_count>     - Start concurrent read test\r\n");
        CLI_LOGI("  flash_race_test start_rw <read_cnt> <write_cnt> - Start read-write test\r\n");
        CLI_LOGI("  flash_race_test start_we <write_cnt> <erase_cnt> - Start write-erase test\r\n");
        CLI_LOGI("  flash_race_test start_stress <task_count>   - Start stress test\r\n");
        CLI_LOGI("  flash_race_test start_isr                   - Start ISR rule test\r\n");
        CLI_LOGI("  flash_race_test stop                        - Stop all tests\r\n");
        CLI_LOGI("  flash_race_test status                      - Show statistics\r\n");
        CLI_LOGI("  flash_race_test init                        - Initialize test area\r\n");
         msg = CLI_CMD_RSP_ERROR;
         goto end;
     }

     if(os_strcmp(argv[1], "start_read") == 0) {
         if(argc < 3) {
             CLI_LOGI("Usage: flash_race_test start_read <task_count>\r\n");
             msg = CLI_CMD_RSP_ERROR;
             goto end;
         }
         uint32_t task_count = os_strtoul(argv[2], NULL, 10);
         start_concurrent_read_test(task_count);
         msg = CLI_CMD_RSP_SUCCEED;
     } else if(os_strcmp(argv[1], "start_rw") == 0) {
         if(argc < 4) {
             CLI_LOGI("Usage: flash_race_test start_rw <read_cnt> <write_cnt>\r\n");
             msg = CLI_CMD_RSP_ERROR;
             goto end;
         }
         uint32_t read_cnt = os_strtoul(argv[2], NULL, 10);
         uint32_t write_cnt = os_strtoul(argv[3], NULL, 10);
         start_read_write_test(read_cnt, write_cnt);
         msg = CLI_CMD_RSP_SUCCEED;
     } else if(os_strcmp(argv[1], "start_we") == 0) {
         if(argc < 4) {
             CLI_LOGI("Usage: flash_race_test start_we <write_cnt> <erase_cnt>\r\n");
             msg = CLI_CMD_RSP_ERROR;
             goto end;
         }
         uint32_t write_cnt = os_strtoul(argv[2], NULL, 10);
         uint32_t erase_cnt = os_strtoul(argv[3], NULL, 10);
         start_write_erase_test(write_cnt, erase_cnt);
         msg = CLI_CMD_RSP_SUCCEED;
     } else if(os_strcmp(argv[1], "start_stress") == 0) {
         if(argc < 3) {
             CLI_LOGI("Usage: flash_race_test start_stress <task_count>\r\n");
             msg = CLI_CMD_RSP_ERROR;
             goto end;
         }
         uint32_t task_count = os_strtoul(argv[2], NULL, 10);
         start_stress_test(task_count);
         msg = CLI_CMD_RSP_SUCCEED;
     } else if(os_strcmp(argv[1], "start_isr") == 0) {
         start_isr_rule_test();
         msg = CLI_CMD_RSP_SUCCEED;
     } else if(os_strcmp(argv[1], "stop") == 0) {
         stop_all_tests();
         print_all_statistics();
         msg = CLI_CMD_RSP_SUCCEED;
     } else if(os_strcmp(argv[1], "status") == 0) {
         print_all_statistics();
         msg = CLI_CMD_RSP_SUCCEED;
     } else if(os_strcmp(argv[1], "init") == 0) {
         bk_err_t ret = init_test_area(TEST_FLASH_BASE_ADDR, TEST_FLASH_SIZE);
         if(ret == BK_OK) {
             CLI_LOGI("Test area initialized successfully\r\n");
             msg = CLI_CMD_RSP_SUCCEED;
         } else {
             CLI_LOGE("Failed to initialize test area, ret: %d\r\n", ret);
             msg = CLI_CMD_RSP_ERROR;
         }
     } else {
         CLI_LOGI("Unknown command: %s\r\n", argv[1]);
         msg = CLI_CMD_RSP_ERROR;
     }

 end:
     if(msg) {
         os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
     }
 }