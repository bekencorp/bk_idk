/*
 * SPDX-FileCopyrightText: 2016-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "nvs_thread_safety_test.h"
#include <components/log.h>
#include <os/os.h>
#include "nvs.h"
#include "nvs_flash.h"

#if CONFIG_SUPPORT_NVS_THREAD_SAFETY_TEST
#include "cli.h"
#include "argtable3.h"
#include "cli_common.h"
#endif

#define TAG "nvs_thread_safety"

#define NVS_THREAD_STACK_SIZE       (4096)
#define NVS_THREAD_WRITE_DELAY_MS   (50)
#define NVS_THREAD_READ_DELAY_MS    (30)
#define NVS_THREAD_ERASE_DELAY_MS   (500)
#define NVS_THREAD_NAMESPACE        "dummyNamespace"
#define NVS_RACE_BASE_PRIORITY      (3)

#define MAX_READ_TASKS   NVS_THREAD_TEST_MAX_READERS
#define MAX_WRITE_TASKS  NVS_THREAD_TEST_MAX_WRITERS
#define MAX_STR_LEN      NVS_THREAD_TEST_STR_LEN
#define MAX_OPS_PER_TASK 9

typedef struct {
    nvs_type_t type;
    int64_t signed_value;
    uint64_t unsigned_value;
    char str_value[MAX_STR_LEN];
    size_t str_len;
    uint8_t blob_value[MAX_STR_LEN];
    size_t blob_len;
} nvs_value_t;

typedef struct {
    char key[MAX_STR_LEN];
    nvs_type_t type;
    nvs_value_t last_value;
} nvs_shared_entry_t;

typedef struct {
    nvs_type_t type;
    char key[MAX_STR_LEN];
    char value[MAX_STR_LEN];
    size_t shared_index;
} nvs_operation_desc_t;

typedef struct {
    beken_thread_t handle;
    int priority;
    nvs_operation_desc_t ops[MAX_OPS_PER_TASK];
    size_t op_count;
    size_t current_op;
} nvs_writer_task_t;

typedef struct {
    beken_thread_t handle;
    int priority;
    nvs_operation_desc_t ops[MAX_OPS_PER_TASK];
    size_t op_count;
    size_t current_op;
} nvs_reader_task_t;

static const struct {
    nvs_type_t type;
    const char *key;
    const char *value;
} s_default_reader_ops[] = {
    { NVS_TYPE_U8,  "dummyU8Key",      NULL },
    { NVS_TYPE_I8,  "dummyI8Key",      NULL },
    { NVS_TYPE_U16, "dummyU16Key",     NULL },
    { NVS_TYPE_I16, "dummyI16Key",     NULL },
    { NVS_TYPE_U32, "dummyU32Key",     NULL },
    { NVS_TYPE_I32, "dummyI32Key",     NULL },
    { NVS_TYPE_STR, "dummyStringKey",  NULL },
    { NVS_TYPE_BLOB,"dummyHex2BinKey", NULL },
    { NVS_TYPE_BLOB,"dummyBase64Key",  NULL },
};

static const struct {
    nvs_type_t type;
    const char *key;
    const char *value;
} s_default_writer_ops[] = {
    { NVS_TYPE_U8,  "dummyU8Key",      "111" },
    { NVS_TYPE_I8,  "dummyI8Key",      "-101" },
    { NVS_TYPE_U16, "dummyU16Key",     "6666" },
    { NVS_TYPE_I16, "dummyI16Key",     "-6666" },
    { NVS_TYPE_U32, "dummyU32Key",     "66666666" },
    { NVS_TYPE_I32, "dummyI32Key",     "-666666666" },
    { NVS_TYPE_STR, "dummyStringKey",  "beken" },
    { NVS_TYPE_BLOB,"dummyHex2BinKey", "123" },
    { NVS_TYPE_BLOB,"dummyBase64Key",  "666" },
};

static volatile bool nvs_thread_safety_stop = false;
static volatile bool nvs_thread_safety_running = false;

static nvs_reader_task_t s_reader_tasks[MAX_READ_TASKS];
static nvs_writer_task_t s_writer_tasks[MAX_WRITE_TASKS];
static nvs_shared_entry_t s_shared_entries[MAX_OPS_PER_TASK];
static size_t s_reader_task_count = 0;
static size_t s_writer_task_count = 0;
static size_t s_shared_entry_count = 0;

static beken_thread_t s_eraser_handle = NULL;
static char s_reader_names[MAX_READ_TASKS][16];
static char s_writer_names[MAX_WRITE_TASKS][16];

typedef struct {
    uint32_t read_ok;
    uint32_t read_fail;
    uint32_t read_not_found;  // Expected: key not found (e.g., after erase)
    uint32_t write_ok;
    uint32_t write_fail;
} nvs_op_stats_t;

static nvs_op_stats_t s_nvs_stats = {0};

static const char *nvs_type_to_string(nvs_type_t type)
{
    switch (type) {
    case NVS_TYPE_I8:  return "i8";
    case NVS_TYPE_U8:  return "u8";
    case NVS_TYPE_I16: return "i16";
    case NVS_TYPE_U16: return "u16";
    case NVS_TYPE_I32: return "i32";
    case NVS_TYPE_U32: return "u32";
    case NVS_TYPE_STR: return "str";
    case NVS_TYPE_BLOB: return "blob";
    default: return "unknown";
    }
}

static size_t nvs_find_shared_entry(const char *key)
{
    for (size_t i = 0; i < s_shared_entry_count; i++) {
        if (strcmp(s_shared_entries[i].key, key) == 0) {
            return i;
        }
    }
    return SIZE_MAX;
}

static size_t nvs_register_shared_entry(const char *key, nvs_type_t type)
{
    size_t idx = nvs_find_shared_entry(key);
    if (idx != SIZE_MAX) {
        return idx;
    }
    if (s_shared_entry_count >= MAX_OPS_PER_TASK) {
        return SIZE_MAX;
    }
    strncpy(s_shared_entries[s_shared_entry_count].key, key, sizeof(s_shared_entries[s_shared_entry_count].key) - 1);
    s_shared_entries[s_shared_entry_count].key[sizeof(s_shared_entries[s_shared_entry_count].key) - 1] = '\0';
    s_shared_entries[s_shared_entry_count].type = type;
    return s_shared_entry_count++;
}

static void nvs_reset_shared_entries(void)
{
    memset(s_shared_entries, 0, sizeof(s_shared_entries));
    s_shared_entry_count = 0;
}

static void nvs_writer_push_shared(size_t shared_idx, const nvs_value_t *value)
{
    if (shared_idx == SIZE_MAX || shared_idx >= s_shared_entry_count) {
        return;
    }

    s_shared_entries[shared_idx].last_value = *value;
    s_shared_entries[shared_idx].type = value->type;
}

static void nvs_reader_compare_with_shared(size_t shared_idx, const nvs_value_t *value)
{
    if (shared_idx >= s_shared_entry_count) {
        return;
    }
    bool mismatch = false;
    const nvs_shared_entry_t *entry = &s_shared_entries[shared_idx];
    switch (value->type) {
    case NVS_TYPE_I8:
    case NVS_TYPE_I16:
    case NVS_TYPE_I32:
        mismatch = (value->signed_value != entry->last_value.signed_value);
        break;
    case NVS_TYPE_U8:
    case NVS_TYPE_U16:
    case NVS_TYPE_U32:
        mismatch = (value->unsigned_value != entry->last_value.unsigned_value);
        break;
    case NVS_TYPE_STR:
        mismatch = (value->str_len != entry->last_value.str_len) ||
                   (memcmp(value->str_value, entry->last_value.str_value, value->str_len) != 0);
        break;
    case NVS_TYPE_BLOB:
        mismatch = (value->blob_len != entry->last_value.blob_len) ||
                   (memcmp(value->blob_value, entry->last_value.blob_value, value->blob_len) != 0);
        break;
    default:
        break;
    }

    if (mismatch) {
        BK_LOGD(TAG, "reader sees mismatch key=%s", entry->key);
    }
}

static bk_err_t nvs_perform_read(nvs_handle_t handle, nvs_type_t type, const char *key, nvs_value_t *out)
{
    bk_err_t err = BK_FAIL;
    memset(out, 0, sizeof(*out));
    out->type = type;

    switch (type) {
    case NVS_TYPE_I8: {
        int8_t v = 0;
        err = nvs_get_i8(handle, key, &v);
        out->signed_value = v;
        break;
    }
    case NVS_TYPE_U8: {
        uint8_t v = 0;
        err = nvs_get_u8(handle, key, &v);
        out->unsigned_value = v;
        break;
    }
    case NVS_TYPE_I16: {
        int16_t v = 0;
        err = nvs_get_i16(handle, key, &v);
        out->signed_value = v;
        break;
    }
    case NVS_TYPE_U16: {
        uint16_t v = 0;
        err = nvs_get_u16(handle, key, &v);
        out->unsigned_value = v;
        break;
    }
    case NVS_TYPE_I32: {
        int32_t v = 0;
        err = nvs_get_i32(handle, key, &v);
        out->signed_value = v;
        break;
    }
    case NVS_TYPE_U32: {
        uint32_t v = 0;
        err = nvs_get_u32(handle, key, &v);
        out->unsigned_value = v;
        break;
    }
    case NVS_TYPE_STR: {
        size_t req = sizeof(out->str_value);
        err = nvs_get_str(handle, key, out->str_value, &req);
        out->str_len = req;
        break;
    }
    case NVS_TYPE_BLOB: {
        size_t req = sizeof(out->blob_value);
        err = nvs_get_blob(handle, key, out->blob_value, &req);
        out->blob_len = req;
        break;
    }
    default:
        err = BK_FAIL;
        break;
    }

    // Update statistics
    if (err == BK_OK) {
        s_nvs_stats.read_ok++;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Expected: key not found (e.g., after erase or not written yet)
        s_nvs_stats.read_not_found++;
    } else {
        // Real error (not expected)
        s_nvs_stats.read_fail++;
        BK_LOGW(TAG, "read failed: type=%s, key=%s, err=%d", 
                nvs_type_to_string(type), key, err);
    }

    return err;
}

static bk_err_t nvs_perform_write(nvs_handle_t handle, nvs_type_t type, const char *key, const char *value, nvs_value_t *out)
{
    bk_err_t err = BK_FAIL;
    memset(out, 0, sizeof(*out));
    out->type = type;

    switch (type) {
    case NVS_TYPE_I8: {
        int32_t val = (int32_t)strtol(value, NULL, 0);
        err = nvs_set_i8(handle, key, (int8_t)val);
        out->signed_value = (int8_t)val;
        break;
    }
    case NVS_TYPE_U8: {
        uint32_t val = (uint32_t)strtoul(value, NULL, 0);
        err = nvs_set_u8(handle, key, (uint8_t)val);
        out->unsigned_value = (uint8_t)val;
        break;
    }
    case NVS_TYPE_I16: {
        int32_t val = (int32_t)strtol(value, NULL, 0);
        err = nvs_set_i16(handle, key, (int16_t)val);
        out->signed_value = (int16_t)val;
        break;
    }
    case NVS_TYPE_U16: {
        uint32_t val = (uint32_t)strtoul(value, NULL, 0);
        err = nvs_set_u16(handle, key, (uint16_t)val);
        out->unsigned_value = (uint16_t)val;
        break;
    }
    case NVS_TYPE_I32: {
        int32_t val = (int32_t)strtol(value, NULL, 0);
        err = nvs_set_i32(handle, key, val);
        out->signed_value = val;
        break;
    }
    case NVS_TYPE_U32: {
        uint32_t val = (uint32_t)strtoul(value, NULL, 0);
        err = nvs_set_u32(handle, key, val);
        out->unsigned_value = val;
        break;
    }
    case NVS_TYPE_STR: {
        err = nvs_set_str(handle, key, value);
        size_t len = strnlen(value, sizeof(out->str_value));
        memcpy(out->str_value, value, len);
        out->str_len = len;
        break;
    }
    case NVS_TYPE_BLOB: {
        size_t len = strnlen(value, sizeof(out->blob_value));
        err = nvs_set_blob(handle, key, (const uint8_t *)value, len);
        memcpy(out->blob_value, value, len);
        out->blob_len = len;
        break;
    }
    default:
        err = BK_FAIL;
        break;
    }

    if (err == BK_OK) {
        err = nvs_commit(handle);
    }

    // Update statistics
    if (err == BK_OK) {
        s_nvs_stats.write_ok++;
    } else {
        s_nvs_stats.write_fail++;
        BK_LOGW(TAG, "write failed: type=%s, key=%s, value=%s, err=%d", 
                nvs_type_to_string(type), key, value ? value : "NULL", err);
    }

    return err;
}

static void nvs_reader_task_entry(beken_thread_arg_t arg)
{
    nvs_reader_task_t *task = (nvs_reader_task_t *)arg;
    nvs_handle_t handle = 0;
    bk_err_t err = nvs_open(NVS_THREAD_NAMESPACE, NVS_READWRITE, &handle);
    if (err != BK_OK) {
        BK_LOGE(TAG, "reader failed to open (%d)", err);
        return;
    }

    if (task->op_count == 0) {
        BK_LOGW(TAG, "reader task has no operations, exiting");
        nvs_close(handle);
        return;
    }

    while (!nvs_thread_safety_stop) {
        nvs_operation_desc_t *op = &task->ops[task->current_op];
        nvs_value_t read_value;
        err = nvs_perform_read(handle, op->type, op->key, &read_value);
        if (err == BK_OK) {
            nvs_reader_compare_with_shared(op->shared_index, &read_value);
        } else if (err != ESP_ERR_NVS_NOT_FOUND) {
            BK_LOGW(TAG, "reader %s read err=%d", op->key, err);
        }
        task->current_op = (task->current_op + 1) % task->op_count;
        rtos_delay_milliseconds(NVS_THREAD_READ_DELAY_MS);
    }

    nvs_close(handle);
}

static void nvs_writer_task_entry(beken_thread_arg_t arg)
{
    nvs_writer_task_t *task = (nvs_writer_task_t *)arg;
    nvs_handle_t handle = 0;
    bk_err_t err = nvs_open(NVS_THREAD_NAMESPACE, NVS_READWRITE, &handle);
    if (err != BK_OK) {
        BK_LOGE(TAG, "writer failed to open (%d)", err);
        return;
    }

    if (task->op_count == 0) {
        BK_LOGW(TAG, "writer task has no operations, exiting");
        nvs_close(handle);
        return;
    }

    while (!nvs_thread_safety_stop) {
        nvs_operation_desc_t *op = &task->ops[task->current_op];
        nvs_value_t wrote_value;
        err = nvs_perform_write(handle, op->type, op->key, op->value, &wrote_value);
        if (err == BK_OK) {
            nvs_writer_push_shared(op->shared_index, &wrote_value);
        } else {
            BK_LOGW(TAG, "writer %s write err=%d", op->key, err);
        }
        task->current_op = (task->current_op + 1) % task->op_count;
        rtos_delay_milliseconds(NVS_THREAD_WRITE_DELAY_MS);
    }

    nvs_close(handle);
}

static void nvs_eraser_task_entry(beken_thread_arg_t arg)
{
    nvs_handle_t handle = 0;
    bk_err_t err = nvs_open(NVS_THREAD_NAMESPACE, NVS_READWRITE, &handle);
    if (err != BK_OK) {
        BK_LOGE(TAG, "eraser failed to open (%d)", err);
        return;
    }

    while (!nvs_thread_safety_stop) {
        err = nvs_erase_all(handle);
        if (err == BK_OK) {
            err = nvs_commit(handle);
        }
        if (err != BK_OK) {
            BK_LOGW(TAG, "eraser err=%d", err);
        }
        rtos_delay_milliseconds(NVS_THREAD_ERASE_DELAY_MS);
    }

    nvs_close(handle);
}

static void nvs_sort_tasks_by_priority(size_t count, int order[], int priorities[])
{
    for (size_t i = 0; i < count; i++) {
        order[i] = i;
    }
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (priorities[order[i]] > priorities[order[j]]) {
                int tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }
}

static bk_err_t nvs_create_reader_threads(void)
{
    int order[MAX_READ_TASKS];
    int priorities[MAX_READ_TASKS];
    for (size_t i = 0; i < s_reader_task_count; i++) {
        priorities[i] = s_reader_tasks[i].priority;
    }
    nvs_sort_tasks_by_priority(s_reader_task_count, order, priorities);

    for (size_t i = 0; i < s_reader_task_count; i++) {
        size_t idx = order[i];
        snprintf(s_reader_names[idx], sizeof(s_reader_names[idx]), "nvs_reader_%zu", idx);
        bk_err_t err = rtos_create_thread(&s_reader_tasks[idx].handle,
                                          s_reader_tasks[idx].priority,
                                          s_reader_names[idx],
                                          nvs_reader_task_entry,
                                          NVS_THREAD_STACK_SIZE,
                                          &s_reader_tasks[idx]);
        if (err != BK_OK) {
            return err;
        }
    }
    return BK_OK;
}

static bk_err_t nvs_create_writer_threads(void)
{
    int order[MAX_WRITE_TASKS];
    int priorities[MAX_WRITE_TASKS];
    for (size_t i = 0; i < s_writer_task_count; i++) {
        priorities[i] = s_writer_tasks[i].priority;
    }
    nvs_sort_tasks_by_priority(s_writer_task_count, order, priorities);

    for (size_t i = 0; i < s_writer_task_count; i++) {
        size_t idx = order[i];
        snprintf(s_writer_names[idx], sizeof(s_writer_names[idx]), "nvs_writer_%zu", idx);
        bk_err_t err = rtos_create_thread(&s_writer_tasks[idx].handle,
                                          s_writer_tasks[idx].priority,
                                          s_writer_names[idx],
                                          nvs_writer_task_entry,
                                          NVS_THREAD_STACK_SIZE,
                                          &s_writer_tasks[idx]);
        if (err != BK_OK) {
            return err;
        }
    }
    return BK_OK;
}

static bk_err_t nvs_create_eraser_thread(void)
{
    return rtos_create_thread(&s_eraser_handle,
                              NVS_RACE_BASE_PRIORITY,
                              "nvs_eraser",
                              nvs_eraser_task_entry,
                              NVS_THREAD_STACK_SIZE,
                              NULL);
}

static void nvs_cleanup_threads(void)
{
    nvs_thread_safety_stop = true;
    rtos_delay_milliseconds(200);
    s_reader_task_count = 0;
    s_writer_task_count = 0;
    nvs_reset_shared_entries();
    memset(&s_nvs_stats, 0, sizeof(s_nvs_stats));
}

static void nvs_reset_stats(void)
{
    memset(&s_nvs_stats, 0, sizeof(s_nvs_stats));
}

static void nvs_get_race_stats(uint32_t *read_ok, uint32_t *read_fail, uint32_t *write_ok, uint32_t *write_fail)
{
    if (read_ok) {
        *read_ok = s_nvs_stats.read_ok;
    }
    if (read_fail) {
        *read_fail = s_nvs_stats.read_fail;
    }
    if (write_ok) {
        *write_ok = s_nvs_stats.write_ok;
    }
    if (write_fail) {
        *write_fail = s_nvs_stats.write_fail;
    }
}

static void nvs_get_all_stats(uint32_t *read_ok, uint32_t *read_fail, uint32_t *read_not_found, 
                              uint32_t *write_ok, uint32_t *write_fail)
{
    if (read_ok) {
        *read_ok = s_nvs_stats.read_ok;
    }
    if (read_fail) {
        *read_fail = s_nvs_stats.read_fail;
    }
    if (read_not_found) {
        *read_not_found = s_nvs_stats.read_not_found;
    }
    if (write_ok) {
        *write_ok = s_nvs_stats.write_ok;
    }
    if (write_fail) {
        *write_fail = s_nvs_stats.write_fail;
    }
}

static void nvs_fill_reader_ops(nvs_reader_task_t *task)
{
    task->op_count = 0;
    task->current_op = 0;
    size_t count = sizeof(s_default_reader_ops) / sizeof(s_default_reader_ops[0]);
    for (size_t i = 0; i < count && i < MAX_OPS_PER_TASK; i++) {
        task->ops[i].type = s_default_reader_ops[i].type;
        strncpy(task->ops[i].key, s_default_reader_ops[i].key, sizeof(task->ops[i].key) - 1);
        task->ops[i].key[sizeof(task->ops[i].key) - 1] = '\0';
        task->ops[i].value[0] = '\0';
        size_t shared_idx = nvs_find_shared_entry(task->ops[i].key);
        task->ops[i].shared_index = shared_idx;
    }
    task->op_count = (count < MAX_OPS_PER_TASK) ? count : MAX_OPS_PER_TASK;
}

static void nvs_fill_writer_ops(nvs_writer_task_t *task)
{
    task->op_count = 0;
    task->current_op = 0;
    size_t count = sizeof(s_default_writer_ops) / sizeof(s_default_writer_ops[0]);
    for (size_t i = 0; i < count && i < MAX_OPS_PER_TASK; i++) {
        task->ops[i].type = s_default_writer_ops[i].type;
        strncpy(task->ops[i].key, s_default_writer_ops[i].key, sizeof(task->ops[i].key) - 1);
        task->ops[i].key[sizeof(task->ops[i].key) - 1] = '\0';
        strncpy(task->ops[i].value, s_default_writer_ops[i].value, sizeof(task->ops[i].value) - 1);
        task->ops[i].value[sizeof(task->ops[i].value) - 1] = '\0';
        size_t shared_idx = nvs_register_shared_entry(task->ops[i].key, task->ops[i].type);
        task->ops[i].shared_index = shared_idx;
    }
    task->op_count = (count < MAX_OPS_PER_TASK) ? count : MAX_OPS_PER_TASK;
}

static void nvs_prepare_tasks_from_config(size_t reader_count, size_t writer_count)
{
    nvs_reset_shared_entries();

    // Fill writer tasks first to register shared entries
    s_writer_task_count = writer_count;
    for (size_t i = writer_count; i-- > 0; ) {
        s_writer_tasks[i].priority = NVS_RACE_BASE_PRIORITY + (int)i;
        nvs_fill_writer_ops(&s_writer_tasks[i]);
    }

    // Fill reader tasks after shared entries are registered
    s_reader_task_count = reader_count;
    for (size_t i = reader_count; i-- > 0; ) {
        s_reader_tasks[i].priority = NVS_RACE_BASE_PRIORITY + (int)i;
        nvs_fill_reader_ops(&s_reader_tasks[i]);
    }
}

static bk_err_t nvs_flash_init_safely(void)
{
    bk_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err;
}

bk_err_t nvs_thread_safety_test_start_custom(const nvs_thread_reader_cfg_t *readers,
                                            size_t reader_count,
                                            const nvs_thread_writer_cfg_t *writers,
                                            size_t writer_count)
{
    if (nvs_thread_safety_running) {
        BK_LOGW(TAG, "thread safety test already running");
        return BK_FAIL;
    }

    if (reader_count == 0 || writer_count == 0 ||
        reader_count > MAX_READ_TASKS || writer_count > MAX_WRITE_TASKS) {
        BK_LOGE(TAG, "invalid configuration");
        return BK_FAIL;
    }

    bk_err_t err = nvs_flash_init_safely();
    if (err != BK_OK) {
        BK_LOGE(TAG, "nvs_flash_init failed (%d)", err);
        return err;
    }

    nvs_thread_safety_stop = false;
    nvs_prepare_tasks_from_config(reader_count, writer_count);

    err = nvs_create_writer_threads();
    if (err != BK_OK) {
        goto cleanup;
    }

    err = nvs_create_reader_threads();
    if (err != BK_OK) {
        goto cleanup;
    }

    err = nvs_create_eraser_thread();
    if (err != BK_OK) {
        goto cleanup;
    }

    nvs_thread_safety_running = true;
    return BK_OK;

cleanup:
    nvs_thread_safety_stop = true;
    rtos_delay_milliseconds(100);
    nvs_flash_deinit();
    nvs_thread_safety_running = false;
    nvs_cleanup_threads();
    return err;
}

bk_err_t nvs_thread_safety_test_start(void)
{
    return nvs_thread_safety_test_start_custom(NULL, 1, NULL, 1);
}

void nvs_thread_safety_test_stop(void)
{
    if (!nvs_thread_safety_running) {
        return;
    }

    nvs_thread_safety_stop = true;
    rtos_delay_milliseconds(200);
    nvs_flash_deinit();
    nvs_thread_safety_running = false;
    nvs_cleanup_threads();
}

bool nvs_thread_safety_test_is_running(void)
{
    return nvs_thread_safety_running;
}

const nvs_thread_reader_cfg_t *nvs_thread_safety_test_default_readers(size_t *count)
{
    if (count) {
        *count = 0;
    }
    return NULL;
}

const nvs_thread_writer_cfg_t *nvs_thread_safety_test_default_writers(size_t *count)
{
    if (count) {
        *count = 0;
    }
    return NULL;
}

static void print_nvs_race_help(void)
{
    CLI_LOGI("nvs_race_test CLI:\r\n");
    CLI_LOGI("  --start <read_task_cnt> <write_task_cnt>  : start thread safety test\r\n");
    CLI_LOGI("  --stop                                   : stop thread safety test\r\n");
    CLI_LOGI("  --stats                                  : show operation statistics\r\n");
    CLI_LOGI("  --help                                   : show this help message\r\n");
    CLI_LOGI("Examples:\r\n");
    CLI_LOGI("  nvs_race_test --start 2 3\r\n");
    CLI_LOGI("  nvs_race_test --stop\r\n");
    CLI_LOGI("  nvs_race_test --stats\r\n");
    CLI_LOGI("Note:\r\n");
    CLI_LOGI("  - Each read task will perform all 9 read operations (u8/i8/u16/i16/u32/i32/str/blob/blob)\r\n");
    CLI_LOGI("  - Each write task will perform all 9 write operations\r\n");
    CLI_LOGI("  - Tasks are created in priority order (low priority first)\r\n");
}

static void cli_nvs_thread_safety_test_cmd_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_lit *start = (struct arg_lit *)argtable[1];
    struct arg_lit *stop = (struct arg_lit *)argtable[2];
    struct arg_lit *stats = (struct arg_lit *)argtable[3];
    struct arg_int *read_cnt = (struct arg_int *)argtable[4];
    struct arg_int *write_cnt = (struct arg_int *)argtable[5];

    if (help->count > 0) {
        print_nvs_race_help();
        return;
    }

    if (stats->count > 0) {
        uint32_t read_ok = 0, read_fail = 0, read_not_found = 0, write_ok = 0, write_fail = 0;
        nvs_get_all_stats(&read_ok, &read_fail, &read_not_found, &write_ok, &write_fail);
        uint32_t read_total = read_ok + read_fail + read_not_found;
        uint32_t write_total = write_ok + write_fail;
        CLI_LOGI("NVS Thread Safety Test Statistics:\r\n");
        CLI_LOGI("  Read operations:  OK=%lu, NOT_FOUND=%lu (expected), FAIL=%lu (error), Total=%lu\r\n", 
                 (unsigned long)read_ok, (unsigned long)read_not_found, 
                 (unsigned long)read_fail, (unsigned long)read_total);
        CLI_LOGI("  Write operations: OK=%lu, FAIL=%lu, Total=%lu\r\n", 
                 (unsigned long)write_ok, (unsigned long)write_fail, (unsigned long)write_total);
        if (read_total > 0) {
            CLI_LOGI("  Read success rate: %.2f%% (excluding NOT_FOUND)\r\n", 
                     (read_ok * 100.0f) / read_total);
        }
        if (write_total > 0) {
            CLI_LOGI("  Write success rate: %.2f%%\r\n", (write_ok * 100.0f) / write_total);
        }
        return;
    }

    if (stop->count > 0) {
        uint32_t read_ok = 0, read_fail = 0, read_not_found = 0, write_ok = 0, write_fail = 0;
        nvs_get_all_stats(&read_ok, &read_fail, &read_not_found, &write_ok, &write_fail);
        nvs_thread_safety_test_stop();
        CLI_LOGI("thread safety test stopped\r\n");
        CLI_LOGI("Final statistics: Read OK=%lu/NOT_FOUND=%lu/FAIL=%lu, Write OK=%lu/FAIL=%lu\r\n",
                 (unsigned long)read_ok, (unsigned long)read_not_found, (unsigned long)read_fail,
                 (unsigned long)write_ok, (unsigned long)write_fail);
        return;
    }

    if (start->count > 0) {
        if (read_cnt->count == 0 || write_cnt->count == 0) {
            CLI_LOGI("missing read_task_cnt or write_task_cnt\r\n");
            print_nvs_race_help();
            return;
        }

        int reader_count = read_cnt->ival[0];
        int writer_count = write_cnt->ival[0];

        if (reader_count <= 0 || writer_count <= 0 ||
            reader_count > MAX_READ_TASKS || writer_count > MAX_WRITE_TASKS) {
            CLI_LOGI("invalid task count: read=%d (max %d), write=%d (max %d)\n",
                     reader_count, MAX_READ_TASKS, writer_count, MAX_WRITE_TASKS);
            print_nvs_race_help();
            return;
        }

        nvs_reset_stats();
        bk_err_t err = nvs_thread_safety_test_start_custom(NULL, (size_t)reader_count, NULL, (size_t)writer_count);
        CLI_LOGI(err ? "thread safety start failed (%x)\r\n" : "thread safety start ok\r\n", err);
        return;
    }

    CLI_LOGI("nvs_race_test requires --start, --stop, or --stats\r\n");
    print_nvs_race_help();
}

void cli_nvs_thread_safety_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help   = arg_lit0("h", "help",    "Display this help message");
    struct arg_lit *start  = arg_lit0("s", "start",   "Start thread safety workload");
    struct arg_lit *stop   = arg_lit0("u", "stop",    "Stop thread safety workload");
    struct arg_lit *stats  = arg_lit0("t", "stats",   "Show operation statistics");
    struct arg_int *read_cnt = arg_intn(NULL, NULL, "<read_task_cnt>", 0, 1, "Number of read tasks");
    struct arg_int *write_cnt = arg_intn(NULL, NULL, "<write_task_cnt>", 0, 1, "Number of write tasks");
    struct arg_end *end    = arg_end(20);

    void *argtable[] = { help, start, stop, stats, read_cnt, write_cnt, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_nvs_thread_safety_test_cmd_handler);
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
}
