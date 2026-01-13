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

/**
 * @file psram_xip_stress_test.c
 * @brief PSRAM XIP Mode Stress Test
 * 
 * This module provides comprehensive stress testing for PSRAM in XIP (eXecute In Place) mode.
 * The test code itself runs in PSRAM space to verify:
 * 1. Instruction fetch reliability (code execution from PSRAM)
 * 2. Data access reliability (read/write operations)
 * 3. Mixed access patterns (instruction + data)
 * 4. Long-term stability under high-frequency access
 * 
 * Test Environment:
 * - CPU: Cortex-M33 @ 240MHz
 * - PSRAM: 160MHz @ 1.95V
 * - Mode: XIP (eXecute In Place)
 * 
 * Implementation Details:
 * - All test functions are marked with PSRAM_XIP_CODE_SECTION attribute, which places
 *   them in .psram.text section
 * - Linker scripts (bk7236n_bsp.ld, bk7239n_bsp.ld, bk7236_bsp.ld) are configured to:
 *   - Place .psram.text section at PSRAM address (0x60000000)
 *   - Place .psram.data section at PSRAM address (0x60008000)
 *   - Place .psram.bss section in PSRAM
 *   - Copy code and data from Flash to PSRAM during boot (via .copy.table)
 *   - Zero BSS section during boot (via .zero.table)
 * 
 * CRITICAL NOTES:
 * 1. This test code MUST run in PSRAM (XIP mode) to properly test instruction fetch reliability.
 *    If code runs from Flash or SRAM, it cannot detect PSRAM instruction fetch errors.
 * 
 * 2. PSRAM initialization is SKIPPED in this test because:
 *    - The test code itself is already executing from PSRAM
 *    - Re-initializing PSRAM would cause data corruption, as initialization may reset
 *      or reconfigure PSRAM while code is executing from it
 *    - PSRAM must be initialized BEFORE this test code is executed
 * 
 * 3. Test data area is located at 0x60008000 (after 32KB code area) to avoid conflicts
 *    with the test code itself.
 * 
 * Test Coverage:
 * - Instruction execution verification: Tests arithmetic, loops, and function calls
 * - Data write test: Writes patterns and immediately verifies correctness
 * - Data read test: Tests read reliability with multiple patterns
 * - CRC integrity test: Uses CRC32 to verify large data blocks
 * - Mixed access test: Simultaneously tests instruction fetch and data access
 * - Loop execution test: Detects instruction fetch errors in repetitive code
 * 
 * Usage:
 * 1. Enable CONFIG_PSRAM_XIP_STRESS_TEST in Kconfig
 * 2. Ensure PSRAM is initialized before running the test
 * 3. Use CLI command: psram_xip_test start [duration_sec]
 *    - duration_sec: Test duration in seconds (0 for infinite loop)
 * 4. Monitor test progress (printed every 100 iterations)
 * 5. Use "psram_xip_test stop" to stop running test
 * 6. Use "psram_xip_test status" to view current statistics
 * 
 * Example:
 *   psram_xip_test start 3600    # Run for 1 hour
 *   psram_xip_test start 0       # Run indefinitely
 *   psram_xip_test stop          # Stop current test
 *   psram_xip_test status        # Show statistics
 */

#include "cli.h"
#include <os/os.h>
#include <driver/psram.h>
#include <driver/aon_rtc.h>
#include "psram_driver.h"
#include "bk_private/bk_init.h"    /**< Beken system initialization */
#include <components/system.h>      /**< System components */
#include <os/os.h>                  /**< Operating system interface */
#include "sys_ll.h"                 /**< System low-level functions */
#include "modules/pm.h"             /**< Power management module */

#if CONFIG_TRNG_SUPPORT
#include <driver/trng.h>
#endif

#include "soc/mapping.h"
#include "cache.h"

/* PSRAM XIP test configuration */
#define PSRAM_XIP_TEST_BASE_ADDR        (0x60000000)    /**< PSRAM base address */
#define PSRAM_XIP_TEST_DATA_SIZE        (128 * 1024)     /**< Test data area size: 64KB */
#define PSRAM_XIP_TEST_CODE_SIZE        (32 * 1024)     /**< Reserved code area: 32KB */
#define PSRAM_XIP_TEST_DATA_ADDR        (PSRAM_XIP_TEST_BASE_ADDR + PSRAM_XIP_TEST_CODE_SIZE)
#define PSRAM_XIP_TEST_PATTERN_SIZE     (4 * 1024)      /**< Pattern buffer size: 4KB */

/* Section attribute for placing code in PSRAM */
#if (CONFIG_PSRAM_XIP_STRESS_TEST)
#define PSRAM_XIP_CODE_SECTION __attribute__((section(".psram.text")))
#define PSRAM_XIP_DATA_SECTION __attribute__((section(".psram.data")))
#define PSRAM_XIP_BSS_SECTION  __attribute__((section(".psram.bss")))
#else
#define PSRAM_XIP_CODE_SECTION
#define PSRAM_XIP_DATA_SECTION
#define PSRAM_XIP_BSS_SECTION
#endif

/* Test pattern constants */
#define PSRAM_XIP_TEST_PATTERN_1        0xA55AA55A      /**< Test pattern 1 */
#define PSRAM_XIP_TEST_PATTERN_2        0x5AA55AA5      /**< Test pattern 2 */
#define PSRAM_XIP_TEST_PATTERN_3        0x12345678      /**< Test pattern 3 */
#define PSRAM_XIP_TEST_PATTERN_4        0x87654321      /**< Test pattern 4 */
#define PSRAM_XIP_TEST_MAGIC            0xDEADBEEF      /**< Magic number for validation */

/* Test statistics structure */
typedef struct {
    uint32_t total_iterations;          /**< Total test iterations */
    uint32_t instruction_errors;        /**< Instruction fetch errors */
    uint32_t data_read_errors;          /**< Data read errors */
    uint32_t data_write_errors;         /**< Data write errors */
    uint32_t crc_errors;                /**< CRC verification errors */
    uint32_t pattern_errors;             /**< Pattern verification errors */
    uint32_t function_call_errors;      /**< Function call execution errors */
    uint32_t loop_execution_errors;     /**< Loop execution errors */

    /* Independent error counters for each test (Test 2-5) */
    uint32_t test2_data_write_errors;  /**< Test 2: Data write test errors */
    uint32_t test3_data_read_errors;    /**< Test 3: Data read test errors */
    uint32_t test4_crc_errors;          /**< Test 4: CRC integrity test errors */
    uint32_t test5_mixed_access_errors; /**< Test 5: Mixed access test errors */

    uint64_t total_test_time_us;        /**< Total test time in microseconds */
    uint32_t last_error_addr;           /**< Last error address */
    uint32_t last_error_expected;       /**< Last error expected value */
    uint32_t last_error_actual;         /**< Last error actual value */
} psram_xip_test_stats_t;

/* Global test statistics - placed in PSRAM data section */
static PSRAM_XIP_DATA_SECTION psram_xip_test_stats_t g_test_stats = {0};

/* Test control flags - placed in PSRAM data section */
static volatile PSRAM_XIP_DATA_SECTION uint8_t g_test_running = 0;
static volatile PSRAM_XIP_DATA_SECTION uint8_t g_test_stop_requested = 0;

/* External function declarations */
extern int bk_rand(void);
extern void flush_all_dcache(void);
extern uint64_t bk_aon_rtc_get_us(void);

/**
 * @brief Calculate CRC32 for data verification
 * 
 * This function calculates CRC32 checksum for a data buffer.
 * Used to verify data integrity during stress testing.
 * 
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return CRC32 checksum value
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_calculate_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;

    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

/**
 * @brief Test function 1: Simple arithmetic operations
 * 
 * This function performs arithmetic operations to test instruction execution.
 * If instructions are corrupted, the result will be incorrect.
 * 
 * @param input Input value
 * @return Computed result
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_function1(uint32_t input)
{
    uint32_t result = input;
    result = result * 3 + 7;
    result = result / 2 - 1;
    result = result ^ 0x12345678;
    result = result << 3 | result >> 29;
    return result;
}

/**
 * @brief Test function 2: Complex calculations with loops
 * 
 * This function performs complex calculations with loops to test
 * instruction fetch and execution reliability.
 * 
 * @param input Input value
 * @return Computed result
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_function2(uint32_t input)
{
    uint32_t result = 0;
    uint32_t temp = input;

    for (uint32_t i = 0; i < 100; i++) {
        temp = temp * 1103515245 + 12345;
        result += temp;
        result = result ^ (i * 0x5A5A5A5A);
    }

    return result;
}

/**
 * @brief Test function 3: Recursive-like operations
 * 
 * This function tests instruction execution with recursive-like patterns.
 * 
 * @param input Input value
 * @param depth Recursion depth
 * @return Computed result
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_function3(uint32_t input, uint32_t depth)
{
    if (depth == 0) {
        return input;
    }

    uint32_t result = input;
    for (uint32_t i = 0; i < depth; i++) {
        result = psram_xip_test_function1(result);
    }

    return result;
}

/**
 * @brief Verify function execution correctness
 * 
 * This function verifies that test functions execute correctly,
 * detecting instruction fetch errors.
 * 
 * @return 0 if all functions execute correctly, non-zero on error
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_verify_instruction_execution(void)
{
    uint32_t error_count = 0;
    uint32_t result1, result2, result3;
    uint32_t expected1, expected2, expected3;

    /* Test function 1 with known input */
    result1 = psram_xip_test_function1(0x12345678);
    expected1 = 0x12345678;
    expected1 = expected1 * 3 + 7;
    expected1 = expected1 / 2 - 1;
    expected1 = expected1 ^ 0x12345678;
    expected1 = expected1 << 3 | expected1 >> 29;

    if (result1 != expected1) {
        g_test_stats.instruction_errors++;
        g_test_stats.last_error_addr = (uint32_t)psram_xip_test_function1;
        g_test_stats.last_error_expected = expected1;
        g_test_stats.last_error_actual = result1;
        error_count++;
        CLI_LOGE("Instruction error in function1: expected=0x%08X, actual=0x%08X\r\n", 
                 expected1, result1);
    }

    /* Test function 2 */
    result2 = psram_xip_test_function2(0xABCDEF00);
    /* Calculate expected value */
    expected2 = 0;
    uint32_t temp2 = 0xABCDEF00;
    for (uint32_t i = 0; i < 100; i++) {
        temp2 = temp2 * 1103515245 + 12345;
        expected2 += temp2;
        expected2 = expected2 ^ (i * 0x5A5A5A5A);
    }

    if (result2 != expected2) {
        g_test_stats.instruction_errors++;
        g_test_stats.last_error_addr = (uint32_t)psram_xip_test_function2;
        g_test_stats.last_error_expected = expected2;
        g_test_stats.last_error_actual = result2;
        error_count++;
        CLI_LOGE("Instruction error in function2: expected=0x%08X, actual=0x%08X\r\n", 
                 expected2, result2);
    }

    /* Test function 3 */
    result3 = psram_xip_test_function3(0x5A5A5A5A, 10);
    expected3 = 0x5A5A5A5A;
    for (uint32_t i = 0; i < 10; i++) {
        expected3 = expected3 * 3 + 7;
        expected3 = expected3 / 2 - 1;
        expected3 = expected3 ^ 0x12345678;
        expected3 = expected3 << 3 | expected3 >> 29;
    }

    if (result3 != expected3) {
        g_test_stats.instruction_errors++;
        g_test_stats.last_error_addr = (uint32_t)psram_xip_test_function3;
        g_test_stats.last_error_expected = expected3;
        g_test_stats.last_error_actual = result3;
        error_count++;
        CLI_LOGE("Instruction error in function3: expected=0x%08X, actual=0x%08X\r\n", 
                 expected3, result3);
    }

    if (error_count > 0) {
        g_test_stats.function_call_errors += error_count;
    }

    return error_count;
}

/**
 * @brief Test data write with pattern verification
 * 
 * This function writes test patterns to PSRAM and verifies write correctness.
 * 
 * @param base_addr Base address for test data
 * @param size Size of test area in bytes
 * @return 0 if successful, non-zero on error
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_data_write(uint32_t base_addr, uint32_t size)
{
    uint32_t error_count = 0;
    volatile uint32_t *addr_ptr;
    uint32_t pattern;
    uint32_t words = size / 4;

    /* Write pattern 1: A55AA55A */
    pattern = PSRAM_XIP_TEST_PATTERN_1;
    for (uint32_t i = 0; i < words; i++) {
        addr_ptr = (volatile uint32_t *)(base_addr + i * 4);
        *addr_ptr = pattern ^ i;
    }

    /* Verify write */
    for (uint32_t i = 0; i < words; i++) {
        addr_ptr = (volatile uint32_t *)(base_addr + i * 4);
        uint32_t read_value = *addr_ptr;
        uint32_t expected = pattern ^ i;

        if (read_value != expected) {
            g_test_stats.data_write_errors++;
            g_test_stats.last_error_addr = base_addr + i * 4;
            g_test_stats.last_error_expected = expected;
            g_test_stats.last_error_actual = read_value;
            error_count++;

            if (error_count == 1) {
                CLI_LOGE("Data write error: addr=0x%08X, expected=0x%08X, actual=0x%08X\r\n",
                         base_addr + i * 4, expected, read_value);
            }
        }
    }

    return error_count;
}

/**
 * @brief Test data read with multiple patterns
 * 
 * This function tests data read reliability with various patterns.
 * 
 * @param base_addr Base address for test data
 * @param size Size of test area in bytes
 * @return 0 if successful, non-zero on error
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_data_read(uint32_t base_addr, uint32_t size)
{
    uint32_t error_count = 0;
    volatile uint32_t *addr_ptr;
    uint32_t patterns[] = {
        PSRAM_XIP_TEST_PATTERN_1,
        PSRAM_XIP_TEST_PATTERN_2,
        PSRAM_XIP_TEST_PATTERN_3,
        PSRAM_XIP_TEST_PATTERN_4
    };
    uint32_t words = size / 4;

    for (uint32_t p = 0; p < 4; p++) {
        uint32_t pattern = patterns[p];

        /* Write pattern */
        for (uint32_t i = 0; i < words; i++) {
            addr_ptr = (volatile uint32_t *)(base_addr + i * 4);
            *addr_ptr = pattern ^ (i << 8) ^ p;
        }

        /* Flush cache if enabled */
        #if (CONFIG_CACHE_ENABLE)
        flush_all_dcache();
        #endif

        /* Read and verify */
        for (uint32_t i = 0; i < words; i++) {
            addr_ptr = (volatile uint32_t *)(base_addr + i * 4);
            uint32_t read_value = *addr_ptr;
            uint32_t expected = pattern ^ (i << 8) ^ p;

            if (read_value != expected) {
                g_test_stats.data_read_errors++;
                g_test_stats.last_error_addr = base_addr + i * 4;
                g_test_stats.last_error_expected = expected;
                g_test_stats.last_error_actual = read_value;
                error_count++;

                if (error_count <= 10) {
                    CLI_LOGE("Data read error: pattern=%d, addr=0x%08X, expected=0x%08X, actual=0x%08X\r\n",
                             p, base_addr + i * 4, expected, read_value);
                }
            }
        }
    }

    return error_count;
}

/**
 * @brief Test CRC-based data integrity
 * 
 * This function uses CRC32 to verify data integrity over large areas.
 * 
 * @param base_addr Base address for test data
 * @param size Size of test area in bytes
 * @return 0 if successful, non-zero on error
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_crc_integrity(uint32_t base_addr, uint32_t size)
{
    uint32_t error_count = 0;
    uint8_t *data_ptr = (uint8_t *)base_addr;
    uint32_t crc1, crc2;

    /* Generate test data with known pattern */
    for (uint32_t i = 0; i < size; i++) {
        data_ptr[i] = (uint8_t)(i ^ (i >> 8) ^ (i >> 16) ^ (i >> 24));
    }

    /* Calculate CRC */
    crc1 = psram_xip_calculate_crc32(data_ptr, size);

    /* Flush cache */
    #if (CONFIG_CACHE_ENABLE)
    flush_all_dcache();
    #endif

    /* Recalculate CRC after potential cache flush */
    crc2 = psram_xip_calculate_crc32(data_ptr, size);

    if (crc1 != crc2) {
        g_test_stats.crc_errors++;
        error_count++;
        CLI_LOGE("CRC integrity error: crc1=0x%08X, crc2=0x%08X\r\n", crc1, crc2);
    }

    return error_count;
}

/**
 * @brief Test mixed instruction and data access
 * 
 * This function tests simultaneous instruction execution and data access,
 * which is the typical scenario in XIP mode.
 * 
 * @param base_addr Base address for test data
 * @param size Size of test area in bytes
 * @return 0 if successful, non-zero on error
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_mixed_access(uint32_t base_addr, uint32_t size)
{
    uint32_t error_count = 0;
    volatile uint32_t *addr_ptr;
    uint32_t words = size / 4;
    uint32_t test_value;

    /* Write test data */
    for (uint32_t i = 0; i < words; i++) {
        addr_ptr = (volatile uint32_t *)(base_addr + i * 4);
        test_value = psram_xip_test_function1(i);
        *addr_ptr = test_value;
    }

    /* Flush cache */
    #if (CONFIG_CACHE_ENABLE)
    flush_all_dcache();
    #endif

    /* Read back and verify while executing functions */
    for (uint32_t i = 0; i < words; i++) {
        /* Execute test function (instruction fetch) */
        uint32_t computed = psram_xip_test_function1(i);

        /* Read data (data access) */
        addr_ptr = (volatile uint32_t *)(base_addr + i * 4);
        uint32_t read_value = *addr_ptr;

        /* Verify both instruction execution and data read */
        if (read_value != computed) {
            g_test_stats.data_read_errors++;
            g_test_stats.instruction_errors++;
            g_test_stats.last_error_addr = base_addr + i * 4;
            g_test_stats.last_error_expected = computed;
            g_test_stats.last_error_actual = read_value;
            error_count++;

            if (error_count <= 10) {
                CLI_LOGE("Mixed access error: addr=0x%08X, expected=0x%08X, actual=0x%08X\r\n",
                         base_addr + i * 4, computed, read_value);
            }
        }
    }

    return error_count;
}

/**
 * @brief Test loop execution reliability
 * 
 * This function tests loop execution to detect instruction fetch errors
 * in repetitive code patterns.
 * 
 * @param iterations Number of loop iterations
 * @return 0 if successful, non-zero on error
 */
static PSRAM_XIP_CODE_SECTION uint32_t psram_xip_test_loop_execution(uint32_t iterations)
{
    uint32_t error_count = 0;
    uint32_t sum = 0;
    uint32_t expected_sum = 0;

    /* Calculate expected sum */
    for (uint32_t i = 0; i < iterations; i++) {
        expected_sum += i;
        expected_sum = expected_sum ^ (i * 0x12345678);
    }

    /* Execute loop (instructions fetched from PSRAM) */
    for (uint32_t i = 0; i < iterations; i++) {
        sum += i;
        sum = sum ^ (i * 0x12345678);
    }

    if (sum != expected_sum) {
        g_test_stats.loop_execution_errors++;
        g_test_stats.last_error_expected = expected_sum;
        g_test_stats.last_error_actual = sum;
        error_count++;
        CLI_LOGE("Loop execution error: expected_sum=0x%08X, actual_sum=0x%08X\r\n",
                 expected_sum, sum);
    }

    return error_count;
}

/**
 * @brief Print test statistics
 * 
 * This function prints comprehensive test statistics.
 */
static void psram_xip_print_statistics(void)
{
    CLI_LOGI("========== PSRAM XIP Stress Test Statistics ==========\r\n");
    CLI_LOGI("Total iterations: %lu\r\n", g_test_stats.total_iterations);
    CLI_LOGI("Instruction errors: %lu\r\n", g_test_stats.instruction_errors);
    CLI_LOGI("Data read errors: %lu\r\n", g_test_stats.data_read_errors);
    CLI_LOGI("Data write errors: %lu\r\n", g_test_stats.data_write_errors);
    CLI_LOGI("CRC errors: %lu\r\n", g_test_stats.crc_errors);
    CLI_LOGI("Pattern errors: %lu\r\n", g_test_stats.pattern_errors);
    CLI_LOGI("Function call errors: %lu\r\n", g_test_stats.function_call_errors);
    CLI_LOGI("Loop execution errors: %lu\r\n", g_test_stats.loop_execution_errors);

    CLI_LOGI("\r\n--- Independent Test Error Counters (Test 2-5) ---\r\n");
    CLI_LOGI("Test 2 (Data Write) errors: %lu\r\n", g_test_stats.test2_data_write_errors);
    CLI_LOGI("Test 3 (Data Read) errors: %lu\r\n", g_test_stats.test3_data_read_errors);
    CLI_LOGI("Test 4 (CRC Integrity) errors: %lu\r\n", g_test_stats.test4_crc_errors);
    CLI_LOGI("Test 5 (Mixed Access) errors: %lu\r\n", g_test_stats.test5_mixed_access_errors);
    CLI_LOGI("Total test time: %llu us\r\n", g_test_stats.total_test_time_us);

    if (g_test_stats.instruction_errors > 0 || 
        g_test_stats.data_read_errors > 0 || 
        g_test_stats.data_write_errors > 0) {
        CLI_LOGI("Last error - Addr: 0x%08X, Expected: 0x%08X, Actual: 0x%08X\r\n",
                 g_test_stats.last_error_addr,
                 g_test_stats.last_error_expected,
                 g_test_stats.last_error_actual);
    }

    uint32_t total_errors = g_test_stats.instruction_errors +
                           g_test_stats.data_read_errors +
                           g_test_stats.data_write_errors +
                           g_test_stats.crc_errors +
                           g_test_stats.pattern_errors +
                           g_test_stats.function_call_errors +
                           g_test_stats.loop_execution_errors +
                           g_test_stats.test2_data_write_errors +
                           g_test_stats.test3_data_read_errors +
                           g_test_stats.test4_crc_errors +
                           g_test_stats.test5_mixed_access_errors;

    if (total_errors == 0) {
        CLI_LOGI("Test result: PASS (No errors detected)\r\n");
    } else {
        CLI_LOGI("Test result: FAIL (Total errors: %lu)\r\n", total_errors);
    }

    CLI_LOGI("======================================================\r\n");
}

/**
 * @brief Reset test statistics
 * 
 * This function resets all test statistics to zero.
 */
static void psram_xip_reset_statistics(void)
{
    os_memset(&g_test_stats, 0, sizeof(psram_xip_test_stats_t));
}

/**
 * @brief Main XIP stress test function
 * 
 * This function performs comprehensive stress testing of PSRAM in XIP mode.
 * The function itself runs in PSRAM, testing both instruction fetch and data access.
 * 
 * @param arg Test duration in seconds (0 for infinite), passed as beken_thread_arg_t
 * @return Thread exit code (not used)
 */
static PSRAM_XIP_CODE_SECTION void psram_xip_stress_test_main(beken_thread_arg_t arg)
{
    uint32_t total_errors = 0;
    uint64_t start_time, current_time, end_time;
    uint32_t iteration = 0;
    uint32_t test_duration_sec = (uint32_t)(uintptr_t)arg;

    CLI_LOGI("========== PSRAM XIP Stress Test Started ==========\r\n");
    CLI_LOGI("Test configuration:\r\n");
    CLI_LOGI("  CPU: Cortex-M33 @ 240MHz\r\n");
    CLI_LOGI("  PSRAM: 160MHz @ 1.95V\r\n");
    CLI_LOGI("  Mode: XIP (eXecute In Place)\r\n");
    CLI_LOGI("  Test data area: 0x%08X - 0x%08X (%d KB)\r\n",
             PSRAM_XIP_TEST_DATA_ADDR,
             PSRAM_XIP_TEST_DATA_ADDR + PSRAM_XIP_TEST_DATA_SIZE,
             PSRAM_XIP_TEST_DATA_SIZE / 1024);
    CLI_LOGI("  Test duration: %s\r\n", 
             test_duration_sec == 0 ? "Infinite" : "Limited");
    CLI_LOGI("==================================================\r\n");

    /* Set CPU frequency to 240MHz */
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_240M);

    /* Configure Flash clock frequency to 80MHz */
    sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0);  /* Set division factor to 0 */
    sys_ll_set_cpu_clk_div_mode2_cksel_flash(2);  /* Select clock source: 0/1:40MHz, 2:80MHz, 3:120MHz */


    /* Note: PSRAM initialization is skipped here
     * This function is already running in PSRAM (XIP mode).
     * Re-initializing PSRAM would cause data corruption in PSRAM,
     * as the initialization process may reset or reconfigure PSRAM
     * while code is executing from it, leading to unpredictable behavior.
     * PSRAM should be initialized before this test code is executed.
     */
    /* bk_err_t ret = bk_psram_init();
    if (ret != BK_OK) {
        CLI_LOGE("PSRAM initialization failed: %d\r\n", ret);
        g_test_running = 0;
        return;
    }*/

    /* Reset statistics */
    psram_xip_reset_statistics();

    /* Get start time */
    start_time = bk_aon_rtc_get_us();
    if (test_duration_sec > 0) {
        end_time = start_time + (uint64_t)test_duration_sec * 1000000;
    } else {
        end_time = 0;
    }

    g_test_running = 1;
    g_test_stop_requested = 0;

    /* Main test loop */
    while (!g_test_stop_requested) {
        iteration++;
        g_test_stats.total_iterations = iteration;

        /* Check time limit */
        if (test_duration_sec > 0) {
            current_time = bk_aon_rtc_get_us();
            if (current_time >= end_time) {
                break;
            }
        }

        /* Test 1: Instruction execution verification */
        total_errors += psram_xip_verify_instruction_execution();

        /* Test 2: Data write test - independent error counting */
        {
            uint32_t test2_errors = psram_xip_test_data_write(PSRAM_XIP_TEST_DATA_ADDR, 
                                                               PSRAM_XIP_TEST_PATTERN_SIZE);
            g_test_stats.test2_data_write_errors += test2_errors;
            total_errors += test2_errors;
        }

        /* Test 3: Data read test - independent error counting */
        {
            uint32_t test3_errors = psram_xip_test_data_read(PSRAM_XIP_TEST_DATA_ADDR, 
                                                              PSRAM_XIP_TEST_PATTERN_SIZE);
            g_test_stats.test3_data_read_errors += test3_errors;
            total_errors += test3_errors;
        }

        /* Test 4: CRC integrity test - independent error counting */
        {
            uint32_t test4_errors = psram_xip_test_crc_integrity(PSRAM_XIP_TEST_DATA_ADDR, 
                                                                  PSRAM_XIP_TEST_PATTERN_SIZE);
            g_test_stats.test4_crc_errors += test4_errors;
            total_errors += test4_errors;
        }

        /* Test 5: Mixed instruction and data access - independent error counting */
        {
            uint32_t test5_errors = psram_xip_test_mixed_access(PSRAM_XIP_TEST_DATA_ADDR, 
                                                                 PSRAM_XIP_TEST_PATTERN_SIZE);
            g_test_stats.test5_mixed_access_errors += test5_errors;
            total_errors += test5_errors;
        }

        /* Test 6: Loop execution test */
        total_errors += psram_xip_test_loop_execution(1000);

        /* Print progress every 100 iterations */
        if (iteration % 1000 == 0) {
            current_time = bk_aon_rtc_get_us();
            g_test_stats.total_test_time_us = current_time - start_time;
            CLI_LOGI("Test progress: iteration=%lu, errors=%lu, time=%llu us\r\n",
                     iteration, total_errors, g_test_stats.total_test_time_us);
        }

        /* Small delay to prevent overwhelming the system
         * Sleep 1 millisecond (not 1 second) to give system a tiny break
         * For high-intensity stress test, 1ms is appropriate
         * If more system breathing room is needed, increase this value (e.g., 10ms)
         */
        rtos_thread_msleep(1);
    }

    /* Calculate final statistics */
    current_time = bk_aon_rtc_get_us();
    g_test_stats.total_test_time_us = current_time - start_time;
    g_test_running = 0;

    /* Print final statistics */
    psram_xip_print_statistics();

    CLI_LOGI("PSRAM XIP stress test completed (total errors: %lu)\r\n", total_errors);

    g_test_running = 0;
    rtos_delete_thread(NULL);
}

/**
 * @brief Stop running stress test
 * 
 * This function requests the stress test to stop.
 */
void psram_xip_stress_test_stop(void)
{
    g_test_stop_requested = 1;
    CLI_LOGI("PSRAM XIP stress test stop requested\r\n");
}

/**
 * @brief Check if stress test is running
 * 
 * @return 1 if test is running, 0 otherwise
 */
uint8_t psram_xip_stress_test_is_running(void)
{
    return g_test_running;
}

/**
 * @brief CLI command handler for XIP stress test
 * 
 * @param pcWriteBuffer Write buffer for CLI output
 * @param xWriteBufferLen Length of write buffer
 * @param argc Number of arguments
 * @param argv Argument array
 */
void cli_psram_xip_stress_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        CLI_LOGI("Usage: psram_xip_test {start [duration_sec]|stop|status}\r\n");
        CLI_LOGI("  start [duration_sec]: Start stress test (duration_sec=0 for infinite)\r\n");
        CLI_LOGI("  stop: Stop running stress test\r\n");
        CLI_LOGI("  status: Show test status\r\n");
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        uint32_t duration = 0;

        if (argc >= 3) {
            duration = os_strtoul(argv[2], NULL, 10);
        }

        if (g_test_running) {
            CLI_LOGI("Test is already running\r\n");
            return;
        }

        /* Create test thread */
        beken_thread_t test_thread;
        int ret = rtos_create_thread(&test_thread,
                                      5,
                                      "psram_xip_test",
                                      (beken_thread_function_t)psram_xip_stress_test_main,
                                      4096,
                                      (beken_thread_arg_t)(uintptr_t)duration);

        if (ret != BK_OK) {
            CLI_LOGE("Failed to create test thread: %d\r\n", ret);
        } else {
            CLI_LOGI("PSRAM XIP stress test started (duration=%lu seconds)\r\n", duration);
        }

    } else if (os_strcmp(argv[1], "stop") == 0) {
        psram_xip_stress_test_stop();
        CLI_LOGI("Stop request sent\r\n");

    } else if (os_strcmp(argv[1], "status") == 0) {
        if (g_test_running) {
            CLI_LOGI("Test status: RUNNING\r\n");
            psram_xip_print_statistics();
        } else {
            CLI_LOGI("Test status: STOPPED\r\n");
        }

    } else {
        CLI_LOGI("Unknown command: %s\r\n", argv[1]);
    }
}

