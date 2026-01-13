/*
 * SPDX-FileCopyrightText: 2016-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <common/bk_include.h>

#define NVS_THREAD_TEST_MAX_READERS 12
#define NVS_THREAD_TEST_MAX_WRITERS 12
#define NVS_THREAD_TEST_STR_LEN     64

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *type;      /**< Supported values: i8, u8, i16, u16, i32, u32, str, blob */
    const char *key;       /**< NVS key that the thread should touch (e.g., dummyI8Key) */
    int priority;          /**< OS priority for the reader task */
} nvs_thread_reader_cfg_t;

typedef struct {
    const char *type;      /**< Same set as reader config */
    const char *key;       /**< NVS key to update */
    int priority;          /**< OS priority for the writer task */
    const char *value;     /**< Literal value to write (interpreted per type) */
} nvs_thread_writer_cfg_t;

/**
 * @brief Start the NVS thread safety stress test with custom configuration.
 *
 * @param readers Reader descriptions that define type/key/priority combinations.
 * @param reader_count Number of reader entries.
 * @param writers Writer descriptions.
 * @param writer_count Number of writer entries.
 *
 * @return BK_OK when the workload successfully starts, BK_FAIL otherwise.
 */
bk_err_t nvs_thread_safety_test_start_custom(const nvs_thread_reader_cfg_t *readers,
                                            size_t reader_count,
                                            const nvs_thread_writer_cfg_t *writers,
                                            size_t writer_count);

/**
 * @brief Start the NVS thread safety stress test with the hardcoded CLI defaults.
 *
 * This is equivalent to invoking the CLI command without overriding the
 * reader/writer list.
 */
bk_err_t nvs_thread_safety_test_start(void);

/**
 * @brief Stop the currently running NVS thread safety test.
 */
void nvs_thread_safety_test_stop(void);

/**
 * @brief Query whether the thread safety test is active.
 */
bool nvs_thread_safety_test_is_running(void);

/**
 * @brief Access the built-in reader configuration that backs the CLI defaults.
 */
const nvs_thread_reader_cfg_t *nvs_thread_safety_test_default_readers(size_t *count);

/**
 * @brief Access the built-in writer configuration that backs the CLI defaults.
 */
const nvs_thread_writer_cfg_t *nvs_thread_safety_test_default_writers(size_t *count);

void cli_nvs_thread_safety_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef __cplusplus
}
#endif

