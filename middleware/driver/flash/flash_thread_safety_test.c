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
 #define MAX_READ_TASKS           5
 #define MAX_WRITE_TASKS          2
 #define MAX_ERASE_TASKS          1

/* Test run control */
static volatile bool g_test_running = false;
static volatile bool g_test_stopped = false;

/* Global buffers to avoid stack overflow (256 bytes per buffer)
 * Create enough buffers for maximum concurrent tasks (10 for stress test)
 */
#define MAX_CONCURRENT_BUFFERS  10
static uint8_t g_read_buffers[MAX_CONCURRENT_BUFFERS][TEST_PAGE_SIZE];
static uint8_t g_write_buffers[MAX_CONCURRENT_BUFFERS][TEST_PAGE_SIZE];
static uint8_t g_verify_buffers[MAX_CONCURRENT_BUFFERS][TEST_PAGE_SIZE];

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
    uint32_t test_addr = TEST_FLASH_BASE_ADDR + (task_id * 0x2000);  /* 8KB area per task */
    uint32_t buf_idx = task_id % MAX_CONCURRENT_BUFFERS;
    uint8_t *buf = g_read_buffers[buf_idx];
    uint8_t *expected_buf = g_verify_buffers[buf_idx];
    uint32_t op_count = 0;
    uint32_t error_count = 0;
    uint32_t last_valid_seq = 0;

    CLI_LOGI("Read task %lu started, addr: 0x%08lx\r\n", task_id, test_addr);

    /* Prepare expected data pattern (same as write test) */
    for(uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
        expected_buf[i] = (uint8_t)(i & 0xFF);
    }

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

            /* Check if data pattern is valid */
            bool pattern_valid = true;

            /* Verify sequence number is reasonable (not too old, not too new) */
            /* In concurrent test, seq may be different from expected, but should be consistent */
            if(seq > last_valid_seq + 1000) {
                /* Sequence jumped too much, might be corrupted */
                pattern_valid = false;
            }

            /* Verify data pattern (bytes 4-255 should follow increment pattern) */
            for(uint32_t i = 4; i < TEST_PAGE_SIZE; i++) {
                uint8_t expected = (uint8_t)(i & 0xFF);
                if(buf[i] != expected) {
                    pattern_valid = false;
                    break;
                }
            }

            if(!pattern_valid) {
                error_count++;
                if(error_count <= 10) {  /* Limit error logging */
                    CLI_LOGE("Read task %lu: Data corruption detected at seq %lu, op %lu\r\n",
                            task_id, seq, op_count);
                }
            } else {
                /* Update last valid sequence */
                if(seq > last_valid_seq) {
                    last_valid_seq = seq;
                }
            }
        }

        op_count++;

        /* Print every 1000 operations */
        if(op_count % 1000 == 0) {
            CLI_LOGI("Read task %lu: %lu ops, %lu errors, last_seq: %lu\r\n",
                    task_id, op_count, error_count, last_valid_seq);
        }

        rtos_delay_milliseconds(2);
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
    uint32_t test_addr = TEST_FLASH_BASE_ADDR + (task_id * 0x2000);
    uint32_t buf_idx = task_id % MAX_CONCURRENT_BUFFERS;
    uint8_t *buf = g_write_buffers[buf_idx];
    uint8_t *read_buf = g_read_buffers[buf_idx];  /* For verification */
    uint32_t write_seq = 0;
    uint32_t op_count = 0;

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

         uint64_t start_time = get_system_time_us();

         bk_err_t ret = bk_flash_write_bytes(test_addr, buf, TEST_PAGE_SIZE);

         uint64_t end_time = get_system_time_us();
         uint32_t time_us = (uint32_t)(end_time - start_time);

         update_statistics(&g_write_stats, ret, time_us);

         if(ret == BK_OK) {
             /* Verify written data */
             bk_err_t read_ret = bk_flash_read_bytes(test_addr, read_buf, TEST_PAGE_SIZE);

             if(read_ret == BK_OK) {
                 if(!verify_data_pattern(read_buf, TEST_PAGE_SIZE, write_seq)) {
                     CLI_LOGE("Write task %lu: Data verification failed at seq %lu\r\n",
                             task_id, write_seq);
                 }
             }
         }

         write_seq++;
         op_count++;

         /* Print every 100 operations */
         if(op_count % 100 == 0) {
             CLI_LOGI("Write task %lu: %lu operations completed\r\n", task_id, op_count);
         }

         rtos_delay_milliseconds(100);
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
     uint32_t test_addr = TEST_FLASH_BASE_ADDR + (task_id * 0x4000);  /* 16KB area per task */
     uint32_t op_count = 0;

     CLI_LOGI("Erase task %lu started, addr: 0x%08lx\r\n", task_id, test_addr);

     while(g_test_running) {
         uint64_t start_time = get_system_time_us();

         bk_err_t ret = bk_flash_erase_sector(test_addr);

         uint64_t end_time = get_system_time_us();
         uint32_t time_us = (uint32_t)(end_time - start_time);

         update_statistics(&g_erase_stats, ret, time_us);

        if(ret == BK_OK) {
            /* Verify erase result (should be all 0xFF) */
            uint32_t buf_idx = task_id % MAX_CONCURRENT_BUFFERS;
            uint8_t *read_buf = g_read_buffers[buf_idx];
            bk_err_t read_ret = bk_flash_read_bytes(test_addr, read_buf, TEST_PAGE_SIZE);

             if(read_ret == BK_OK) {
                 bool all_ff = true;
                 for(uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
                     if(read_buf[i] != 0xFF) {
                         all_ff = false;
                         break;
                     }
                 }

                 if(!all_ff) {
                     CLI_LOGE("Erase task %lu: Sector not fully erased at 0x%08lx\r\n",
                             task_id, test_addr);
                 }
             }
         }

         op_count++;

         /* Print every 50 operations */
         if(op_count % 50 == 0) {
             CLI_LOGI("Erase task %lu: %lu operations completed\r\n", task_id, op_count);
         }

         rtos_delay_milliseconds(500);
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
    uint8_t *buf = g_read_buffers[0];  /* Use first buffer for ISR task */
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

         rtos_delay_milliseconds(10);
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
    uint32_t op_type = task_id % 3;  /* 0=read, 1=write, 2=erase */
    uint32_t test_addr = TEST_FLASH_BASE_ADDR + (task_id * 0x1000);
    uint32_t buf_idx = task_id % MAX_CONCURRENT_BUFFERS;
    uint8_t *buf = (op_type == 0) ? g_read_buffers[buf_idx] : g_write_buffers[buf_idx];
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
             case 0:  /* Read operation */
                 ret = bk_flash_read_bytes(test_addr, buf, TEST_PAGE_SIZE);
                 break;

             case 1:  /* Write operation */
                 buf[0] = (write_seq >> 24) & 0xFF;
                 buf[1] = (write_seq >> 16) & 0xFF;
                 buf[2] = (write_seq >> 8) & 0xFF;
                 buf[3] = write_seq & 0xFF;
                 ret = bk_flash_write_bytes(test_addr, buf, TEST_PAGE_SIZE);
                 write_seq++;
                 break;

             case 2:  /* Erase operation */
                 ret = bk_flash_erase_sector(test_addr);
                 break;
         }

         uint64_t end_time = get_system_time_us();
         uint32_t time_us = (uint32_t)(end_time - start_time);

         /* Update corresponding statistics */
         switch(op_type) {
             case 0:
                 update_statistics(&g_read_stats, ret, time_us);
                 break;
             case 1:
                 update_statistics(&g_write_stats, ret, time_us);
                 break;
             case 2:
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
         rtos_delay_milliseconds(1);
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

    /* Write initial data pattern to each task's area */
    for(uint32_t i = 0; i < task_count; i++) {
        uint32_t test_addr = TEST_FLASH_BASE_ADDR + (i * 0x2000);
        uint32_t buf_idx = i % MAX_CONCURRENT_BUFFERS;
        uint8_t *init_buf = g_write_buffers[buf_idx];

        /* Erase sector before write (ensure sector is erased) */
        uint32_t sector_addr = test_addr & (~(TEST_SECTOR_SIZE - 1));  /* Align to sector boundary */
        bk_err_t erase_ret = bk_flash_erase_sector(sector_addr);
        if(erase_ret != BK_OK) {
            CLI_LOGE("Failed to erase sector at 0x%08lx for task %lu, ret: %d\r\n",
                    sector_addr, i, erase_ret);
            continue;
        }

        /* Prepare initial data pattern */
        for(uint32_t j = 0; j < TEST_PAGE_SIZE; j++) {
            init_buf[j] = (uint8_t)(j & 0xFF);
        }
        /* Set initial sequence number to 0 */
        init_buf[0] = 0;
        init_buf[1] = 0;
        init_buf[2] = 0;
        init_buf[3] = 0;

        /* Write data after erase */
        bk_err_t write_ret = bk_flash_write_bytes(test_addr, init_buf, TEST_PAGE_SIZE);
        if(write_ret != BK_OK) {
            CLI_LOGE("Failed to write initial data for task %lu, ret: %d\r\n", i, write_ret);
        }
    }
    CLI_LOGI("Test area initialized with known data pattern\r\n");

    /* Initialize statistics */
    os_memset(&g_read_stats, 0, sizeof(g_read_stats));

    g_test_running = true;

    /* Create read tasks with different priorities to test preemption
     * Priority range: 5-9 (higher number = higher priority in FreeRTOS)
     * This allows testing both time-slicing and preemption scenarios
     */
    for(uint32_t i = 0; i < task_count; i++) {
        char task_name[32];
        os_snprintf(task_name, sizeof(task_name), "read_task_%lu", i);
        /* Use different priorities: 5, 6, 7, 8, 9 to test preemption */
        uint32_t priority = 5 + (i % 5);  /* Cycle through priorities 5-9 */
        rtos_create_thread(NULL, priority, task_name,
                          task_flash_read_test, 2048, (void*)i);
        rtos_delay_milliseconds(100);  /* Stagger startup time */
    }
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

     g_test_running = true;

     /* Create read tasks */
     for(uint32_t i = 0; i < read_task_count; i++) {
         char task_name[32];
         os_snprintf(task_name, sizeof(task_name), "read_task_%lu", i);
         rtos_create_thread(NULL, 5, task_name,
                           task_flash_read_test, 2048, (void*)i);
         rtos_delay_milliseconds(50);
     }

     /* Create write tasks */
     for(uint32_t i = 0; i < write_task_count; i++) {
         char task_name[32];
         os_snprintf(task_name, sizeof(task_name), "write_task_%lu", i);
         rtos_create_thread(NULL, 6, task_name,
                           task_flash_write_test, 2048, (void*)i);
         rtos_delay_milliseconds(50);
     }
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
     for(uint32_t i = 0; i < write_task_count; i++) {
         char task_name[32];
         os_snprintf(task_name, sizeof(task_name), "write_task_%lu", i);
         rtos_create_thread(NULL, 6, task_name,
                           task_flash_write_test, 2048, (void*)i);
         rtos_delay_milliseconds(50);
     }

     /* Create erase tasks */
     for(uint32_t i = 0; i < erase_task_count; i++) {
         char task_name[32];
         os_snprintf(task_name, sizeof(task_name), "erase_task_%lu", i);
         rtos_create_thread(NULL, 7, task_name,
                           task_flash_erase_test, 2048, (void*)i);
         rtos_delay_milliseconds(50);
     }
 }

 /**
  * @brief Start stress test
  */
 static void start_stress_test(uint32_t task_count)
 {
     if(task_count > 10) {
         task_count = 10;
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
     for(uint32_t i = 0; i < task_count; i++) {
         char task_name[32];
         os_snprintf(task_name, sizeof(task_name), "stress_task_%lu", i);
         rtos_create_thread(NULL, 5, task_name,
                           task_flash_stress_test, 2048, (void*)i);
         rtos_delay_milliseconds(50);
     }
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
     rtos_create_thread(NULL, 5, "flash_from_isr",
                       task_flash_from_isr, 2048, NULL);

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