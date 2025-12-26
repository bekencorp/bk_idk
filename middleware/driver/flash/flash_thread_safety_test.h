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

#pragma once

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Test Configuration
 * ============================================================================ */

/* Test area address (ensure it's writable area, not protected area) */
#define FLASH_TEST_BASE_ADDR       0x260000
#define FLASH_TEST_SIZE            0x10000      /* 64KB test area */
#define FLASH_TEST_SECTOR_SIZE     0x1000       /* 4KB sector */
#define FLASH_TEST_PAGE_SIZE       256          /* 256 bytes page */

/* Maximum number of test tasks */
#define FLASH_TEST_MAX_READ_TASKS  5
#define FLASH_TEST_MAX_WRITE_TASKS 2
#define FLASH_TEST_MAX_ERASE_TASKS 1

/* ============================================================================
 * Statistics Data Structure
 * ============================================================================ */

/**
 * @brief Flash operation statistics structure
 * 
 * Used to track performance and reliability metrics for Flash operations
 * during thread safety testing.
 */
typedef struct {
    uint32_t total_ops;          /**< Total operation count */
    uint32_t success_ops;         /**< Success count */
    uint32_t error_ops;           /**< Error count */
    uint32_t timeout_ops;         /**< Timeout count */
    uint64_t total_time_us;      /**< Total time in microseconds */
    uint32_t max_time_us;         /**< Maximum operation time */
    uint32_t min_time_us;         /**< Minimum operation time */
    uint32_t last_error_code;     /**< Last error code encountered */
} flash_op_statistics_t;

/* ============================================================================
 * Test Function Declarations
 * ============================================================================ */

/**
 * @brief Initialize test area (erase all sectors)
 * 
 * @param base_addr Base address of test area
 * @param size Size of test area in bytes
 * @return bk_err_t BK_OK on success, error code otherwise
 */
bk_err_t flash_test_init_area(uint32_t base_addr, uint32_t size);

/**
 * @brief Get read operation statistics
 * 
 * @param stats Pointer to statistics structure to fill
 * @return bk_err_t BK_OK on success
 */
bk_err_t flash_test_get_read_stats(flash_op_statistics_t *stats);

/**
 * @brief Get write operation statistics
 * 
 * @param stats Pointer to statistics structure to fill
 * @return bk_err_t BK_OK on success
 */
bk_err_t flash_test_get_write_stats(flash_op_statistics_t *stats);

/**
 * @brief Get erase operation statistics
 * 
 * @param stats Pointer to statistics structure to fill
 * @return bk_err_t BK_OK on success
 */
bk_err_t flash_test_get_erase_stats(flash_op_statistics_t *stats);

/**
 * @brief Reset all statistics
 */
void flash_test_reset_statistics(void);

/**
 * @brief Check if test is currently running
 * 
 * @return bool true if test is running, false otherwise
 */
bool flash_test_is_running(void);

/**
 * @brief Stop all running tests
 */
void flash_test_stop_all(void);

/**
 * @brief CLI entry for flash race/thread-safety tests
 *
 * @param pcWriteBuffer CLI response buffer
 * @param xWriteBufferLen Buffer length
 * @param argc Argument count
 * @param argv Argument vector
 */
void cli_flash_race_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef __cplusplus
}
#endif
// eof

