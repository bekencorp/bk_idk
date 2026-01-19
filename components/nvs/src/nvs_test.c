/*
 * SPDX-FileCopyrightText: 2016-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
 /*
* Change Logs:
* Date			 Author 	  Notes
* 2024-04-11	 Beken	  adapter to Beken sdk
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include "partitions.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <components/log.h>
#include <common/bk_compiler.h>
#include "nvs_internal.h"

#if CONFIG_NVS_ENCRYPTION
#include "mbedtls/aes.h"
#include "nvs_sec_provider.h"
#endif

#define TAG  "test_nvs"

#define NVST_LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define NVST_LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define NVST_LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define NVST_LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DEBUG_DEAD_WHILE(x)      while(x)

#define NVS_ERROR_CHECK(x) do {                                             \
            bk_err_t err_rc_ = (x);                                         \
            if ((err_rc_ != ESP_OK)) {                              \
                DEBUG_DEAD_WHILE(4);\
                abort();                                                    \
            }                                                               \
        } while(0)

#define TEST_ASSERT_EQUAL_INT32(val0, val1) \
    do {                          \
        if (val0 == (val1)) {        \
            break;                \
        }else{                    \
            NVST_LOGE("no pass, %s:%d val0 = 0x%x, val1 = 0x%x\r\n", __func__, __LINE__, val0, val1);          \
            DEBUG_DEAD_WHILE(4);\
        }                         \
    } while (0)

#define TEST_NVS_ERR(rc, res)     \
    do {                          \
        if (rc == (res)) {        \
            break;                \
        }else{                    \
            NVST_LOGE("no pass, %s:%d\r\n", __func__, __LINE__);          \
DEBUG_DEAD_WHILE(4);\
        }                         \
    } while (0)


#define TEST_NVS(fun)                                                    \
    do {                                                                    \
        bk_err_t rec = fun;                                                 \
        if (rec == (ESP_ERR_NVS_NOT_FOUND)) {                               \
            NVST_LOGI("empty, %s:%d\r\n", __func__, __LINE__);              \
        } else if (rec == (BK_OK)) {                                        \
            NVST_LOGI("ok, %s:%d\r\n", __func__, __LINE__);                 \
        } else {                                                            \
            NVST_LOGI("failed=0x%02x, %s:%d\r\n", rec, __func__, __LINE__); \
            return -1;                                                         \
        }                                                                   \
    } while (0)

#define NVS_PRINTF(buf, len)                               \
    do {                                                   \
        for (uint32_t i = 0; i < (len); i++) {             \
            if (i % 16 == 0)                               \
                printf("\n");                              \
            printf("%02X ", (buf)[i]);                  \
        }                                                  \
        printf("\n");                                   \
    } while (0)

#define NVS_PRINTF_TITLE(buf)                              \
    do {                                                   \
        NVST_LOGI("\n +++++++ { %s } ++++++ \n",buf);      \
    } while (0)

#define TEST_NVS_OK(rc)           \
    do {                          \
        if (rc == (ESP_OK)) {        \
            break;                \
        }else{                    \
            NVST_LOGE("failed, %s:%d\r\n", __func__, __LINE__);          \
        }                         \
    } while (0)

#define TEST_ASSERT_TRUE_LINE(condition, line) \
    do {                                       \
        if (!(condition)) {                    \
                NVST_LOGE("nvs_test.c", \
                "Assertion failed in line %d", line); \
                return;                         \
            } \
    } while(0)
#define TEST_ASSERT_TRUE(condition) TEST_ASSERT_TRUE_LINE(condition, __LINE__)

#define BK_TEST_ASSERT_TRUE_LINE(condition, line) \
    do {                                       \
        if (!(condition)) {                    \
                NVST_LOGE("nvs_test.c", \
                "Assertion failed in line %d", line); \
                return -1;                         \
            } \
    } while(0)
#define BK_TEST_ASSERT_TRUE(condition) BK_TEST_ASSERT_TRUE_LINE(condition, __LINE__)

const bk_logic_partition_t *bk_partition_find_first(esp_partition_type_t type,
        esp_partition_subtype_t subtype, const char *label);
uint32_t flash_partition_get_index(const bk_logic_partition_t *partition_ptr);


//TEST_CASE("test nvs apis for nvs partition generator utility with encryption enabled", "[nvs_part_gen]")
#if CONFIG_NVS_ENCRYPTION
void nvs_test_api_for_generator_uti_with_encrypt_en(void)
{
    char bin_data[512];
    nvs_handle_t handle;
    nvs_sec_cfg_t xts_cfg = {0};
    const bk_logic_partition_t* key_part = bk_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NVS_KEY_PARTITION_LABEL);

    const bk_logic_partition_t* nvs_part = bk_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NVS_PARTITION_LABEL);

    TEST_ASSERT_TRUE(NULL != key_part);
    TEST_ASSERT_TRUE(NULL != nvs_part);

#if (!(1 == CONFIG_SHANHAI_KEY_LADDER))
    TEST_ASSERT_TRUE(ESP_OK == nvs_flash_read_security_cfg(key_part, &xts_cfg));
#endif

    TEST_NVS_OK(nvs_flash_secure_init(&xts_cfg));

    TEST_NVS_OK(nvs_open("dummyNamespace", NVS_READONLY, &handle));
    uint8_t u8v;
    TEST_NVS_OK( nvs_get_u8(handle, "dummyU8Key", &u8v));
    TEST_ASSERT_TRUE(u8v == 127);
    int8_t i8v;
    TEST_NVS_OK( nvs_get_i8(handle, "dummyI8Key", &i8v));
    TEST_ASSERT_TRUE(i8v == -128);
    uint16_t u16v;
    TEST_NVS_OK( nvs_get_u16(handle, "dummyU16Key", &u16v));
    TEST_ASSERT_TRUE(u16v == 32768);
    uint32_t u32v;
    TEST_NVS_OK( nvs_get_u32(handle, "dummyU32Key", &u32v));
    TEST_ASSERT_TRUE(u32v == 4294967295);
    int32_t i32v;
    TEST_NVS_OK( nvs_get_i32(handle, "dummyI32Key", &i32v));
    TEST_ASSERT_TRUE(i32v == -2147483648);

    char *buf = bin_data;
    size_t buflen = 300;
    TEST_NVS_OK( nvs_get_str(handle, "dummyStringKey", buf, &buflen));

    uint8_t hexdata[] = {0x01, 0x02, 0x03, 0xab, 0xcd, 0xef};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "dummyHex2BinKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, hexdata, buflen) == 0);

    uint8_t base64data[] = {'1', '2', '3', 'a', 'b', 'c'};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "dummyBase64Key", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, base64data, buflen) == 0);

    uint8_t hexfiledata[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "hexFileKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, hexfiledata, buflen) == 0);

    uint8_t base64filedata[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xab, 0xcd, 0xef};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "base64FileKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, base64filedata, buflen) == 0);

    uint8_t strfiledata[64] = "abcdefghijklmnopqrstuvwxyz\0";
    buflen = 64;
    TEST_NVS_OK( nvs_get_str(handle, "stringFileKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, strfiledata, buflen) == 0);

    size_t bin_len = sizeof(bin_data);
    TEST_NVS_OK( nvs_get_blob(handle, "binFileKey", bin_data, &bin_len));

    nvs_close(handle);
    TEST_NVS_OK(nvs_flash_deinit());
}
#endif // CONFIG_NVS_ENCRYPTION

//TEST_CASE("Partition name no longer than 16 characters", "[nvs]")
void nvs_test_partition_congig(void)
{
    const char *TOO_LONG_NAME = "0123456789abcdefg";

    TEST_NVS_ERR(ESP_ERR_INVALID_ARG, nvs_flash_init_partition(TOO_LONG_NAME));

    nvs_flash_deinit_partition(TOO_LONG_NAME); // just in case
}

//TEST_CASE("flash erase deinitializes initialized partition", "[nvs]")
void nvs_test_partition_erase(void)
{
    nvs_handle_t handle;
    bk_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    TEST_NVS_OK( err );

    TEST_NVS_OK(nvs_flash_init());
    TEST_NVS_OK(nvs_open("uninit_ns", NVS_READWRITE, &handle));
    nvs_close(handle);
    TEST_NVS_OK(nvs_flash_erase());

    // exptected: no partition is initialized since nvs_flash_erase() deinitialized the partition again
    TEST_NVS_ERR(ESP_ERR_NVS_NOT_INITIALIZED, nvs_open("uninit_ns", NVS_READWRITE, &handle));

    // just to be sure it's deinitialized in case of error and not affecting other tests
    nvs_flash_deinit();
}

// test could have different output on host tests
//TEST_CASE("nvs deinit with open handle", "[nvs]")
void nvs_test_flash_erase_op(void)
{
    nvs_handle_t handle_1;
    bk_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        NVST_LOGW(TAG, "nvs_flash_init failed (0x%x), erasing partition and retrying", err);
        NVS_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    NVS_ERROR_CHECK( err );

    TEST_NVS_OK(nvs_open("deinit_ns", NVS_READWRITE, &handle_1));
    nvs_flash_deinit();
}

//TEST_CASE("various nvs tests", "[nvs]")
bk_err_t nvs_test_various_op(void)
{
    nvs_handle_t handle_1;
    bk_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        NVST_LOGW(TAG, "nvs_flash_init failed (0x%x), erasing partition and retrying", err);
        NVS_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    NVS_ERROR_CHECK( err );

    TEST_NVS_ERR(ESP_ERR_NVS_NOT_FOUND, nvs_open("test_namespace1", NVS_READONLY, &handle_1));

    TEST_NVS_ERR(ESP_ERR_NVS_INVALID_HANDLE, nvs_set_i32(handle_1, "foo", 0x12345678));
    nvs_close(handle_1);

    TEST_NVS(nvs_open("test_namespace2", NVS_READWRITE, &handle_1));

    TEST_NVS(nvs_erase_all(handle_1));
    TEST_NVS(nvs_set_i32(handle_1, "foo", 0x12345678));
    TEST_NVS(nvs_set_i32(handle_1, "foo", 0x23456789));

    int32_t v1;
    TEST_NVS(nvs_get_i32(handle_1, "foo", &v1));
    TEST_ASSERT_EQUAL_INT32(0x23456789, v1);

    nvs_handle_t handle_2;
    TEST_NVS(nvs_open("test_namespace3", NVS_READWRITE, &handle_2));
    TEST_NVS(nvs_erase_all(handle_2));
    TEST_NVS(nvs_set_i32(handle_2, "foo", 0x3456789a));
    const char* str = "value 0123456789abcdef0123456789abcdef";
    TEST_NVS(nvs_set_str(handle_2, "key", str));

    /*erase all handle_1*/
    bk_printf("\n { erase all handle_1 } \n");
    TEST_NVS(nvs_erase_all(handle_1));
    TEST_NVS(nvs_get_i32(handle_1, "foo", &v1));

    int32_t v2;
    TEST_NVS(nvs_get_i32(handle_2, "foo", &v2));
    TEST_ASSERT_EQUAL_INT32(0x3456789a, v2);

    char buf[strlen(str) + 1];
    size_t buf_len = sizeof(buf);

    TEST_NVS(nvs_get_str(handle_2, "key", buf, &buf_len));

    TEST_ASSERT_EQUAL_INT32(0, strcmp(buf, str));

    nvs_close(handle_1);

    // check that deinit does not leak memory if some handles are still open
    nvs_flash_deinit();

    nvs_close(handle_2);

    return BK_OK;
}

//TEST_CASE("calculate used and free space", "[nvs]")
bk_err_t nvs_test_calc_space(void)
{
    TEST_NVS_ERR(ESP_ERR_INVALID_ARG, nvs_get_stats(NULL, NULL));
    nvs_stats_t stat1;
    nvs_stats_t stat2;
    TEST_NVS_ERR(ESP_ERR_NVS_NOT_INITIALIZED, nvs_get_stats(NULL, &stat1));
    BK_TEST_ASSERT_TRUE(stat1.free_entries == 0);
    BK_TEST_ASSERT_TRUE(stat1.namespace_count == 0);
    BK_TEST_ASSERT_TRUE(stat1.total_entries == 0);
    BK_TEST_ASSERT_TRUE(stat1.used_entries == 0);

    nvs_handle_t handle = 0;
    size_t h_count_entries;
    TEST_NVS_ERR(ESP_ERR_NVS_INVALID_HANDLE, nvs_get_used_entry_count(handle, &h_count_entries));
    BK_TEST_ASSERT_TRUE(h_count_entries == 0);

    bk_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        NVST_LOGW(TAG, "nvs_flash_init failed (0x%x), erasing partition and retrying", err);
        const bk_logic_partition_t* nvs_partition = bk_partition_find_first(
                ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs");
        BK_ASSERT(nvs_partition && "partition table must have an NVS partition");
        NVS_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    NVS_ERROR_CHECK( err );

    // erase if have any namespace
    TEST_NVS(nvs_get_stats(NULL, &stat1));
    if(stat1.namespace_count != 0) {
        TEST_NVS(nvs_flash_deinit());
        TEST_NVS(nvs_flash_erase());
        TEST_NVS(nvs_flash_init());
    }

    // after erase. empty partition
    TEST_NVS(nvs_get_stats(NULL, &stat1));
    BK_TEST_ASSERT_TRUE(stat1.free_entries != 0);
    BK_TEST_ASSERT_TRUE(stat1.namespace_count == 0);
    BK_TEST_ASSERT_TRUE(stat1.total_entries != 0);
    BK_TEST_ASSERT_TRUE(stat1.used_entries == 0);

    // create namespace test_k1
    nvs_handle_t handle_1;
    TEST_NVS(nvs_open("test_k1", NVS_READWRITE, &handle_1));
    TEST_NVS(nvs_get_stats(NULL, &stat2));
    BK_TEST_ASSERT_TRUE(stat2.free_entries + 1 == stat1.free_entries);
    BK_TEST_ASSERT_TRUE(stat2.namespace_count == 1);
    BK_TEST_ASSERT_TRUE(stat2.total_entries == stat1.total_entries);
    BK_TEST_ASSERT_TRUE(stat2.used_entries == 1);

    // create pair key-value com
    TEST_NVS(nvs_set_i32(handle_1, "com", 0x12345678));
    TEST_NVS(nvs_get_stats(NULL, &stat1));
    BK_TEST_ASSERT_TRUE(stat1.free_entries + 1 == stat2.free_entries);
    BK_TEST_ASSERT_TRUE(stat1.namespace_count == 1);
    BK_TEST_ASSERT_TRUE(stat1.total_entries == stat2.total_entries);
    BK_TEST_ASSERT_TRUE(stat1.used_entries == 2);

    // change value in com
    TEST_NVS(nvs_set_i32(handle_1, "com", 0x01234567));
    TEST_NVS(nvs_get_stats(NULL, &stat2));
    BK_TEST_ASSERT_TRUE(stat2.free_entries == stat1.free_entries);
    BK_TEST_ASSERT_TRUE(stat2.namespace_count == 1);
    BK_TEST_ASSERT_TRUE(stat2.total_entries != 0);
    BK_TEST_ASSERT_TRUE(stat2.used_entries == 2);

    // create pair key-value ru
    TEST_NVS(nvs_set_i32(handle_1, "ru", 0x00FF00FF));
    TEST_NVS(nvs_get_stats(NULL, &stat1));
    BK_TEST_ASSERT_TRUE(stat1.free_entries + 1 == stat2.free_entries);
    BK_TEST_ASSERT_TRUE(stat1.namespace_count == 1);
    BK_TEST_ASSERT_TRUE(stat1.total_entries != 0);
    BK_TEST_ASSERT_TRUE(stat1.used_entries == 3);

    // amount valid pair in namespace 1
    size_t h1_count_entries;
    TEST_NVS(nvs_get_used_entry_count(handle_1, &h1_count_entries));
    BK_TEST_ASSERT_TRUE(h1_count_entries == 2);

    nvs_handle_t handle_2;
    // create namespace test_k2
    TEST_NVS(nvs_open("test_k2", NVS_READWRITE, &handle_2));
    TEST_NVS(nvs_get_stats(NULL, &stat2));
    BK_TEST_ASSERT_TRUE(stat2.free_entries + 1 == stat1.free_entries);
    BK_TEST_ASSERT_TRUE(stat2.namespace_count == 2);
    BK_TEST_ASSERT_TRUE(stat2.total_entries == stat1.total_entries);
    BK_TEST_ASSERT_TRUE(stat2.used_entries == 4);

    // create pair key-value
    TEST_NVS(nvs_set_i32(handle_2, "su1", 0x00000001));
    TEST_NVS(nvs_set_i32(handle_2, "su2", 0x00000002));
    TEST_NVS(nvs_set_i32(handle_2, "sus", 0x00000003));
    TEST_NVS(nvs_get_stats(NULL, &stat1));
    BK_TEST_ASSERT_TRUE(stat1.free_entries + 3 == stat2.free_entries);
    BK_TEST_ASSERT_TRUE(stat1.namespace_count == 2);
    BK_TEST_ASSERT_TRUE(stat1.total_entries == stat2.total_entries);
    BK_TEST_ASSERT_TRUE(stat1.used_entries == 7);

    BK_TEST_ASSERT_TRUE(stat1.total_entries == (stat1.used_entries + stat1.free_entries));

    // amount valid pair in namespace 2
    size_t h2_count_entries;
    TEST_NVS(nvs_get_used_entry_count(handle_2, &h2_count_entries));
    BK_TEST_ASSERT_TRUE(h2_count_entries == 3);

    BK_TEST_ASSERT_TRUE(stat1.used_entries == (h1_count_entries + h2_count_entries + stat1.namespace_count));

    nvs_close(handle_1);
    nvs_close(handle_2);

    size_t temp = h2_count_entries;
    TEST_NVS_ERR(ESP_ERR_NVS_INVALID_HANDLE, nvs_get_used_entry_count(handle_1, &h2_count_entries));
    BK_TEST_ASSERT_TRUE(h2_count_entries == 0);
    h2_count_entries = temp;
    TEST_NVS_ERR(ESP_ERR_INVALID_ARG, nvs_get_used_entry_count(handle_1, NULL));

    nvs_handle_t handle_3;
    // create namespace test_k3
    TEST_NVS(nvs_open("test_k3", NVS_READWRITE, &handle_3));
    TEST_NVS(nvs_get_stats(NULL, &stat2));
    BK_TEST_ASSERT_TRUE(stat2.free_entries + 1 == stat1.free_entries);
    BK_TEST_ASSERT_TRUE(stat2.namespace_count == 3);
    BK_TEST_ASSERT_TRUE(stat2.total_entries == stat1.total_entries);
    BK_TEST_ASSERT_TRUE(stat2.used_entries == 8);

    // create pair blobs
    uint32_t blob[12];
    TEST_NVS(nvs_set_blob(handle_3, "bl1", &blob, sizeof(blob)));
    TEST_NVS(nvs_get_stats(NULL, &stat1));
    BK_TEST_ASSERT_TRUE(stat1.free_entries + 4 == stat2.free_entries);
    BK_TEST_ASSERT_TRUE(stat1.namespace_count == 3);
    BK_TEST_ASSERT_TRUE(stat1.total_entries == stat2.total_entries);
    BK_TEST_ASSERT_TRUE(stat1.used_entries == 12);

    // amount valid pair in namespace 2
    size_t h3_count_entries;
    TEST_NVS(nvs_get_used_entry_count(handle_3, &h3_count_entries));
    BK_TEST_ASSERT_TRUE(h3_count_entries == 4);

    BK_TEST_ASSERT_TRUE(stat1.used_entries == (h1_count_entries + h2_count_entries + h3_count_entries + stat1.namespace_count));

    nvs_close(handle_3);

    TEST_NVS(nvs_flash_deinit());
    TEST_NVS(nvs_flash_erase());

    return BK_OK;
}

//TEST_CASE("check for memory leaks in nvs_set_blob", "[nvs]")
void nvs_test_check_memory_leaks(void)
{
    bk_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        NVS_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    TEST_NVS_OK( err );

    for (int i = 0; i < 500; ++i) {
        nvs_handle_t my_handle;
        uint8_t key[20] = {0};

        TEST_NVS_OK( nvs_open("leak_check_ns", NVS_READWRITE, &my_handle) );
        TEST_NVS_OK( nvs_set_blob(my_handle, "key", key, sizeof(key)) );
        TEST_NVS_OK( nvs_commit(my_handle) );
        nvs_close(my_handle);
        NVST_LOGW("%" PRId32 "\n", rtos_get_free_heap_size());
    }

    nvs_flash_deinit();
    NVST_LOGW("%" PRId32 "\n", rtos_get_free_heap_size());
    /* heap leaks will be checked in unity_platform.c */
}

#if CONFIG_NVS_ENCRYPTION
//TEST_CASE("check underlying xts code for 32-byte size sector encryption", "[nvs]")
void nvs_test_check_xts(void)
{
    uint8_t eky_hex[2 * NVS_KEY_SIZE] = { /* Encryption key below*/
                                          0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                          0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                          0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                          0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                          /* Tweak key below*/
                                          0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
                                          0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
                                          0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
                                          0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };


    uint8_t ba_hex[16] = { 0x33,0x33,0x33,0x33,0x33,0x00,0x00,0x00,
                           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

    uint8_t ptxt_hex[32] = { 0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
                             0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
                             0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
                             0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44 };

    uint8_t crypted_hex[32];
    uint8_t ctxt_hex[32] = { 0xe6,0x22,0x33,0x4f,0x18,0x4b,0xbc,0xe1,
                             0x29,0xa2,0x5b,0x2a,0xc7,0x6b,0x3d,0x92,
                             0xab,0xf9,0x8e,0x22,0xdf,0x5b,0xdd,0x15,
                             0xaf,0x47,0x1f,0x3d,0xb8,0x94,0x6a,0x85 };

    mbedtls_aes_xts_context ectx[1];

    mbedtls_aes_xts_init(ectx);

    TEST_ASSERT_TRUE(!mbedtls_aes_xts_setkey_enc(ectx, eky_hex, 2 * NVS_KEY_SIZE * 8));

    TEST_ASSERT_TRUE(!mbedtls_aes_crypt_xts(ectx, MBEDTLS_AES_ENCRYPT, 32, ba_hex, ptxt_hex, crypted_hex));
    TEST_ASSERT_TRUE(!memcmp(crypted_hex, ctxt_hex, 32));

#define CONFIG_ENABLE_NVS_MULTI_XTS_CHECK 0
#if CONFIG_ENABLE_NVS_MULTI_XTS_CHECK
    TEST_ASSERT_TRUE(!mbedtls_aes_crypt_xts(ectx, MBEDTLS_AES_ENCRYPT, 32, ba_hex, ptxt_hex, crypted_hex));
    TEST_ASSERT_TRUE(!memcmp(crypted_hex, ctxt_hex, 32));

    TEST_ASSERT_TRUE(!mbedtls_aes_crypt_xts(ectx, MBEDTLS_AES_ENCRYPT, 32, ba_hex, ptxt_hex, crypted_hex));
    TEST_ASSERT_TRUE(!memcmp(crypted_hex, ctxt_hex, 32));

    TEST_ASSERT_TRUE(!mbedtls_aes_crypt_xts(ectx, MBEDTLS_AES_ENCRYPT, 32, ba_hex, ptxt_hex, crypted_hex));
    TEST_ASSERT_TRUE(!memcmp(crypted_hex, ctxt_hex, 32));
#endif
    mbedtls_aes_xts_free(ectx);
}

//TEST_CASE("Check nvs key partition APIs (read and generate keys)", "[nvs]")
#if (!(1 == CONFIG_SHANHAI_KEY_LADDER))
void nvs_test_check_enc_partition_api(void)
{
    nvs_sec_cfg_t cfg, cfg2;
    const bk_logic_partition_t* key_part = bk_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NVS_KEY_PARTITION_LABEL);

    TEST_NVS_OK(bk_flash_partition_erase((bk_partition_t)flash_partition_get_index(key_part), 0, key_part->partition_length));
    TEST_NVS_ERR(ESP_ERR_NVS_KEYS_NOT_INITIALIZED, nvs_flash_read_security_cfg(key_part, &cfg));

    TEST_NVS_OK(nvs_flash_generate_keys(key_part, &cfg));

    TEST_NVS_OK(nvs_flash_read_security_cfg(key_part, &cfg2));

    TEST_ASSERT_TRUE(!memcmp(&cfg, &cfg2, sizeof(nvs_sec_cfg_t)));
}
#endif

//TEST_CASE("test nvs apis with encryption enabled", "[nvs]")
void nvs_test_apis_with_encrypted(void)
{
    uint32_t done = 0;

#if (!(1 == CONFIG_SHANHAI_KEY_LADDER))
    const bk_logic_partition_t* key_part = bk_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NVS_KEY_PARTITION_LABEL);
    BK_ASSERT(key_part && "partition table must have an NVS partition");

    TEST_NVS_OK(bk_flash_partition_erase((bk_partition_t)flash_partition_get_index(key_part), 0, key_part->partition_length));
#endif

    do {
        bk_err_t err = ESP_FAIL;
        nvs_sec_cfg_t cfg;
        nvs_handle_t handle_1;

        const bk_logic_partition_t* nvs_partition = bk_partition_find_first(
                ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs");
        TEST_NVS_OK(bk_flash_partition_erase((bk_partition_t)flash_partition_get_index(nvs_partition), 0, nvs_partition->partition_length));

#if (!(1 == CONFIG_SHANHAI_KEY_LADDER))
        err = nvs_flash_read_security_cfg(key_part, &cfg);
#endif
        (void)err;

        TEST_NVS_OK(nvs_flash_secure_init(&cfg));

        TEST_NVS_ERR(ESP_ERR_NVS_NOT_FOUND, nvs_open("namespace1", NVS_READONLY, &handle_1));

        TEST_NVS_OK(nvs_open("namespace1", NVS_READWRITE, &handle_1));

        TEST_NVS_OK(nvs_set_i32(handle_1, "foo", 0x12345678));
        TEST_NVS_OK(nvs_set_i32(handle_1, "foo", 0x23456789));

        nvs_handle_t handle_2;
        TEST_NVS_OK(nvs_open("namespace2", NVS_READWRITE, &handle_2));
        TEST_NVS_OK(nvs_set_i32(handle_2, "foo", 0x3456789a));
        const char* str = "value 0123456789abcdef0123456789abcdef";
        TEST_NVS_OK(nvs_set_str(handle_2, "key", str));

        int32_t v1;
        TEST_NVS_OK(nvs_get_i32(handle_1, "foo", &v1));
        TEST_ASSERT_TRUE(0x23456789 == v1);

        int32_t v2;
        TEST_NVS_OK(nvs_get_i32(handle_2, "foo", &v2));
        TEST_ASSERT_TRUE(0x3456789a == v2);

        char buf[strlen(str) + 1];
        size_t buf_len = sizeof(buf);

        size_t buf_len_needed;
        TEST_NVS_OK(nvs_get_str(handle_2, "key", NULL, &buf_len_needed));
        TEST_ASSERT_TRUE(buf_len_needed == buf_len);

        size_t buf_len_short = buf_len - 1;
        TEST_NVS_ERR(ESP_ERR_NVS_INVALID_LENGTH, nvs_get_str(handle_2, "key", buf, &buf_len_short));
        TEST_ASSERT_TRUE(buf_len_short == buf_len);

        size_t buf_len_long = buf_len + 1;
        TEST_NVS_OK(nvs_get_str(handle_2, "key", buf, &buf_len_long));
        TEST_ASSERT_TRUE(buf_len_long == buf_len);

        TEST_NVS_OK(nvs_get_str(handle_2, "key", buf, &buf_len));

        TEST_ASSERT_TRUE(0 == strcmp(buf, str));

        nvs_close(handle_1);
        nvs_close(handle_2);
        done ++;

        TEST_NVS_OK(nvs_flash_deinit());
    } while(!done);
}

//TEST_CASE("test nvs apis for nvs partition generator utility with encryption enabled", "[nvs_part_gen]")
void nvs_test_partition_generator_utility(void)
{
    return;
    nvs_handle_t handle;
    nvs_sec_cfg_t xts_cfg;

    const bk_logic_partition_t* nvs_part = bk_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs");

    BK_ASSERT(nvs_part && "partition table must have an NVS partition");
    NVST_LOGW("\n nvs_part size:%" PRId32 "\n", nvs_part->partition_length);

    NVS_ERROR_CHECK(bk_flash_partition_erase((bk_partition_t)flash_partition_get_index(nvs_part), 0, nvs_part->partition_length));


    TEST_NVS_OK(nvs_flash_secure_init(&xts_cfg));

    TEST_NVS_OK(nvs_open("dummyNamespace", NVS_READONLY, &handle));
    uint8_t u8v;
    TEST_NVS_OK( nvs_get_u8(handle, "dummyU8Key", &u8v));
    TEST_ASSERT_TRUE(u8v == 127);
    int8_t i8v;
    TEST_NVS_OK( nvs_get_i8(handle, "dummyI8Key", &i8v));
    TEST_ASSERT_TRUE(i8v == -128);
    uint16_t u16v;
    TEST_NVS_OK( nvs_get_u16(handle, "dummyU16Key", &u16v));
    TEST_ASSERT_TRUE(u16v == 32768);
    uint32_t u32v;
    TEST_NVS_OK( nvs_get_u32(handle, "dummyU32Key", &u32v));
    TEST_ASSERT_TRUE(u32v == 4294967295);
    int32_t i32v;
    TEST_NVS_OK( nvs_get_i32(handle, "dummyI32Key", &i32v));
    TEST_ASSERT_TRUE(i32v == -2147483648);

    char buf[64] = {0};
    size_t buflen = 64;
    TEST_NVS_OK( nvs_get_str(handle, "dummyStringKey", buf, &buflen));
    TEST_ASSERT_TRUE(strncmp(buf, "0A:0B:0C:0D:0E:0F", buflen) == 0);

    uint8_t hexdata[] = {0x01, 0x02, 0x03, 0xab, 0xcd, 0xef};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "dummyHex2BinKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, hexdata, buflen) == 0);

    uint8_t base64data[] = {'1', '2', '3', 'a', 'b', 'c'};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "dummyBase64Key", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, base64data, buflen) == 0);

    uint8_t hexfiledata[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "hexFileKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, hexfiledata, buflen) == 0);

    uint8_t base64filedata[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xab, 0xcd, 0xef};
    buflen = 64;
    TEST_NVS_OK( nvs_get_blob(handle, "base64FileKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, base64filedata, buflen) == 0);

    uint8_t strfiledata[64] = "abcdefghijklmnopqrstuvwxyz\0";
    buflen = 64;
    TEST_NVS_OK( nvs_get_str(handle, "stringFileKey", buf, &buflen));
    TEST_ASSERT_TRUE(memcmp(buf, strfiledata, buflen) == 0);

    nvs_close(handle);
    TEST_NVS_OK(nvs_flash_deinit());
}
#else
// NOTE: `nvs_flash_init_partition_ptr` does not support NVS encryption
//TEST_CASE("nvs_flash_init_partition_ptr() works correctly", "[nvs]")
void nvs_test_partition_open_write(void)
{
    // First, open and write to partition using normal initialization
    nvs_handle_t handle;
    bk_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    TEST_NVS_OK(err);
    TEST_NVS_OK(nvs_open("uninit_ns", NVS_READWRITE, &handle));
    TEST_NVS_OK(nvs_set_i32(handle, "foo", 0x12345678));
    nvs_close(handle);
    nvs_flash_deinit();

    // Then open and read using partition ptr initialization
    const bk_logic_partition_t* nvs_partition = bk_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs");
    TEST_NVS_OK(nvs_flash_init_partition_ptr(nvs_partition));

    TEST_NVS_OK(nvs_open("uninit_ns", NVS_READWRITE, &handle));
    int32_t foo = 0;
    TEST_NVS_OK(nvs_get_i32(handle, "foo", &foo));
    nvs_close(handle);
    TEST_ASSERT_EQUAL_INT32(foo, 0x12345678);

    nvs_flash_deinit();
}
#endif

#define CONFIG_TEST_ENCRYPTED_NVS_USING_NVS_GENEARTOR_UTILITY 1
void nvs_flash_test(void)
{
    bk_printf("nvs_flash_test start\r\n");

    #if CONFIG_TEST_ENCRYPTED_NVS_USING_NVS_GENEARTOR_UTILITY && CONFIG_NVS_ENCRYPTION
    nvs_test_api_for_generator_uti_with_encrypt_en();
	return;
    #endif

#if (1 == CONFIG_SHANHAI_KEY_LADDER)
	int dubhe_aes_xts_self_test(int verbose);

    bk_printf("dubhe_aes_xts_self_test 0\r\n");
    dubhe_aes_xts_self_test(1);
#elif CONFIG_NVS_ENCRYPTION
    bk_printf("nvs_test_check_xts 0\r\n");
    nvs_test_check_xts();
#endif

    bk_printf("nvs_test_partition_congig\r\n");
    nvs_test_partition_congig();

    bk_printf("nvs_test_partition_erase\r\n");
    nvs_test_partition_erase();

    bk_printf("nvs_test_flash_erase_op\r\n");
    nvs_test_flash_erase_op();

    bk_printf("nvs_test_various_op\r\n");
    nvs_test_various_op();

    bk_printf("nvs_test_calc_space\r\n");
    nvs_test_calc_space();

    bk_printf("nvs_test_check_memory_leaks\r\n");
    nvs_test_check_memory_leaks();

#if CONFIG_NVS_ENCRYPTION
#if (!(1 == CONFIG_SHANHAI_KEY_LADDER))
    bk_printf("nvs_test_check_xts\r\n");
    nvs_test_check_xts();
    bk_printf("nvs_test_check_enc_partition_api\r\n");
    nvs_test_check_enc_partition_api();
#endif

    bk_printf("nvs_test_apis_with_encrypted\r\n");
    nvs_test_apis_with_encrypted();

    bk_printf("nvs_test_partition_generator_utility\r\n");
    nvs_test_partition_generator_utility();
#else
    bk_printf("nvs_test_partition_open_write\r\n");
    nvs_test_partition_open_write();

#endif
    bk_printf("nvs_flash_test over\r\n");
}

#if (CONFIG_NVS_ENCRYPTION)
bk_err_t nvs_api_test(nvs_handle_t handle_1)
{
    nvs_entry_info_t info;
    nvs_iterator_t it = (nvs_iterator_t)(uintptr_t)(0xbeef);
    TEST_NVS_OK(nvs_entry_find(NULL, NULL, NVS_TYPE_ANY, &it));
    TEST_NVS(nvs_entry_find("nvs", "namespace1", NVS_TYPE_ANY, &it));

    TEST_NVS(nvs_entry_info(it, &info));
    NVST_LOGI("\n { name_space = %s } \n",info.namespace_name);
    NVST_LOGI("\n { key = %s } \n",info.key);

    NVS_PRINTF_TITLE("nvs_erase_key");
    TEST_NVS(nvs_erase_key(handle_1, "value1"));
    TEST_NVS(nvs_entry_info(it, &info));
    NVST_LOGI("\n { name_space = %s } \n",info.namespace_name);
    NVST_LOGI("\n { key = %s } \n",info.key);

    TEST_NVS(nvs_entry_next(&it));
    
    TEST_NVS(nvs_entry_info(it, &info));
    NVST_LOGI("\n { name_space = %s } \n",info.namespace_name);
    NVST_LOGI("\n  { key = %s } \n",info.key);

    nvs_release_iterator(it);

    return BK_OK;
}

bk_err_t test_all_type_nvs_various_op(void)
{
    bk_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		BK_LOGE(TAG, "nvs_flash_init failed (0x%x), erasing partition and retrying", err);
		TEST_NVS(nvs_flash_erase());
		err = nvs_flash_init();
	}

    nvs_handle_t handle_1;
    nvs_handle_t handle_2;
    const  uint32_t blob = 0x11223344;
    const char *name_1 = "namespace1";
    const char *name_2 = "namespace2";

    TEST_NVS(nvs_open(name_1, NVS_READWRITE, &handle_1));
    TEST_NVS(nvs_open(name_2, NVS_READWRITE, &handle_2));
    
    TEST_NVS(nvs_set_i8(handle_1, "value1", -11));
    TEST_NVS(nvs_set_u8(handle_1, "value2", 11));
    TEST_NVS(nvs_set_i16(handle_1, "value3", -1234));
    TEST_NVS(nvs_set_u16(handle_1, "value4", 1234));
    TEST_NVS(nvs_set_i32(handle_1, "value5", -222));
    TEST_NVS(nvs_set_u32(handle_1, "value6", 222));
    TEST_NVS(nvs_set_str(handle_1, "value7", "foo"));
    TEST_NVS(nvs_set_blob(handle_1, "value8", &blob, sizeof(blob)));

    int8_t i8v;
    uint8_t u8v;
    int16_t i16v;
    uint16_t u16v;
    int32_t i32v;
    uint32_t u32v;
    char buf[64] = {0};
    uint8_t hexdata[] = {0x44, 0x33, 0x22, 0x11};

    TEST_NVS(nvs_get_i8(handle_1, "value1", &i8v));
    TEST_ASSERT_EQUAL_INT32(i8v,-11);
    TEST_NVS(nvs_get_u8(handle_1, "value2", &u8v));
    TEST_ASSERT_EQUAL_INT32(u8v,11);
    TEST_NVS(nvs_get_i16(handle_1, "value3", &i16v));
    TEST_ASSERT_EQUAL_INT32(i16v,-1234);
    TEST_NVS(nvs_get_u16(handle_1, "value4", &u16v));
    TEST_ASSERT_EQUAL_INT32(u16v,1234);
    TEST_NVS(nvs_get_i32(handle_1, "value5", &i32v));
    TEST_ASSERT_EQUAL_INT32(i32v,-222);
    TEST_NVS(nvs_get_u32(handle_1, "value6", &u32v));
    TEST_ASSERT_EQUAL_INT32(u32v,222);
    size_t buflen = 64;
    TEST_NVS( nvs_get_str(handle_1, "value7", buf, &buflen));
    TEST_NVS(strncmp(buf, "foo", buflen));
    buflen = 64;
    TEST_NVS( nvs_get_blob(handle_1, "value8", buf, &buflen));
    NVS_PRINTF_TITLE("nvs_get_blob ");
    NVS_PRINTF(buf,buflen);
    TEST_NVS(memcmp(buf, hexdata, buflen));

    /* handle_2 */
    TEST_NVS(nvs_set_i32(handle_2, "value1", -111));
    TEST_NVS(nvs_set_i64(handle_2, "value2", -555));
    TEST_NVS(nvs_set_u64(handle_2, "value3", 555));

    err = nvs_api_test(handle_1);
    if (err != BK_OK) return err;

    nvs_close(handle_1);
    nvs_close(handle_2);

    TEST_NVS(nvs_flash_deinit_partition(NVS_DEFAULT_PART_NAME));

    return BK_OK;
}

bk_err_t read_default_nvs_data(void)
{
    bk_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		BK_LOGE(TAG, "nvs_flash_init failed (0x%x), erasing partition and retrying", err);
		TEST_NVS(nvs_flash_erase());
		err = nvs_flash_init();
	}

	nvs_handle_t handle;
    char bin_data[512];

    TEST_NVS(nvs_open("test_namespace2", NVS_READONLY, &handle));
    TEST_NVS(nvs_open("dummyNamespace", NVS_READONLY, &handle));
    TEST_NVS(nvs_open("dummyNamespace", NVS_READWRITE, &handle));
    TEST_NVS(nvs_open("test_namespace2", NVS_READWRITE, &handle));

    TEST_NVS(nvs_open("dummyNamespace", NVS_READONLY, &handle));
    uint8_t u8v;
    TEST_NVS( nvs_get_u8(handle, "dummyU8Key", &u8v));
    BK_TEST_ASSERT_TRUE(u8v == 127);
    int8_t i8v;
    TEST_NVS( nvs_get_i8(handle, "dummyI8Key", &i8v));
    BK_TEST_ASSERT_TRUE(i8v == -128);
    uint16_t u16v;
    TEST_NVS( nvs_get_u16(handle, "dummyU16Key", &u16v));
    BK_TEST_ASSERT_TRUE(u16v == 32768);
    uint32_t u32v;
    TEST_NVS( nvs_get_u32(handle, "dummyU32Key", &u32v));
    BK_TEST_ASSERT_TRUE(u32v == 4294967295);
    int32_t i32v;
    TEST_NVS( nvs_get_i32(handle, "dummyI32Key", &i32v));
    BK_TEST_ASSERT_TRUE(i32v == -2147483648);

    char *buf = bin_data;
    size_t buflen = 300;
    TEST_NVS( nvs_get_str(handle, "dummyStringKey", buf, &buflen));

    uint8_t hexdata[] = {0x01, 0x02, 0x03, 0xab, 0xcd, 0xef};
    buflen = 64;
    TEST_NVS( nvs_get_blob(handle, "dummyHex2BinKey", buf, &buflen));
    BK_TEST_ASSERT_TRUE(memcmp(buf, hexdata, buflen) == 0);

    uint8_t base64data[] = {'1', '2', '3', 'a', 'b', 'c'};
    buflen = 64;
    TEST_NVS( nvs_get_blob(handle, "dummyBase64Key", buf, &buflen));
    BK_TEST_ASSERT_TRUE(memcmp(buf, base64data, buflen) == 0);

    nvs_close(handle);
    TEST_NVS(nvs_flash_deinit());

    return BK_OK;
}

bk_err_t nvs_flash_partition_test_various_op(void)
{
    uint8_t test_hex[2 * NVS_KEY_SIZE] = { /* part 1*/
                                        0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                        0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                        0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                        0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                        /* part 2*/
                                        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
                                        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
                                        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
                                        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };


    bk_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		BK_LOGE(TAG, "nvs_flash_init failed (0x%x), erasing partition and retrying", err);
		TEST_NVS(nvs_flash_erase());
		err = nvs_flash_init();
	}

	nvs_handle_t handle_1;
	TEST_NVS(nvs_open("test_namespace2", NVS_READWRITE, &handle_1));
	int32_t v1;
	TEST_NVS(nvs_get_i32(handle_1, "foo", &v1));

    NVS_PRINTF_TITLE("read -- [BK_PARTITION_USER_MFR]");
	uint8_t buffer[100] = {0};
	bk_flash_partition_read(BK_PARTITION_USER_MFR, buffer, 0, 64);
    NVS_PRINTF(buffer,64);


    NVS_PRINTF_TITLE("read_by_name --- [user_mfr]");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 0, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("read_by_name --- [user_mfr] not aligned with 32Byte");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 2, 8);
    NVS_PRINTF(buffer,8);

    NVS_PRINTF_TITLE("read_by_name --- [user_mfr] not aligned with 32Byte");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 33, 8);
    NVS_PRINTF(buffer,8);

    NVS_PRINTF_TITLE("read_by_name --- [user_mfr] not aligned with 32Byte");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 33, 33);
    NVS_PRINTF(buffer,33);

    NVS_PRINTF_TITLE("read_by_name --- [nvs]");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("nvs", buffer, 0, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("erase_all nvs");
    bk_flash_partition_erase_all("nvs");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("nvs", buffer, 0, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("write nvs");
    bk_flash_partition_write_by_name("nvs", test_hex, 0, 64);
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("nvs", buffer, 0, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("erase_32Byte nvs");
    bk_flash_partition_erase_by_name("nvs",0,32);
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("nvs", buffer, 0, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("erase user_mfr");
    bk_flash_partition_erase_by_name("user_mfr",0,32);
    bk_flash_partition_write_by_name("user_mfr", test_hex, 0, 64);
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 0, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("read 32B user_mfr");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 32, 32);
    NVS_PRINTF(buffer,32);

    NVS_PRINTF_TITLE("read 64B user_mfr");
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 32, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("erase user_mfr");
    bk_flash_partition_erase_by_name("user_mfr",0,32);
    bk_flash_partition_write_by_name("user_mfr", test_hex, 32, 32);
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 0, 64);
    NVS_PRINTF(buffer,64);

    NVS_PRINTF_TITLE("erase user_mfr");
    bk_flash_partition_erase_by_name("user_mfr",0,32);
    bk_flash_partition_write_by_name("user_mfr", &test_hex[32], 32, 32);
    memset(buffer,0,64);
    bk_flash_partition_read_by_name("user_mfr", buffer, 0, 64);
    NVS_PRINTF(buffer,64);


    nvs_handle_t handle_2;
    TEST_NVS(nvs_open("test_namespace3", NVS_READWRITE, &handle_2));
    int32_t v2;
    TEST_NVS(nvs_get_i32(handle_1, "foo", &v2));

    err = nvs_test_various_op();
    if (err != BK_OK) return err;

    err = nvs_test_calc_space();
    if (err != BK_OK) return err;

    err = test_all_type_nvs_various_op();
    return err == BK_OK ? BK_OK : BK_FAIL; 
}
#endif

// eof

