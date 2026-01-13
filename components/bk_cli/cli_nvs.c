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
#include "cli.h"
#include "argtable3.h"
#include "cli_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_internal.h"
#include "nvs_thread_safety_test.h"

#if CONFIG_NVS_ENCRYPTION
extern bk_err_t nvs_flash_partition_test_various_op(void);
extern bk_err_t read_default_nvs_data(void);
#endif
extern void nvs_flash_test(void);
extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

static void cli_nvs_test_cmd_handler(void **argtable)
{
    struct arg_lit *self_test     = (struct arg_lit *)argtable[1];
    struct arg_lit *default_value = (struct arg_lit *)argtable[2];
    struct arg_lit *preset_test   = (struct arg_lit *)argtable[3];
    bk_err_t ret;

    if (self_test->count > 0) {
        #if CONFIG_NVS_ENCRYPTION
        ret = nvs_flash_partition_test_various_op();
        CLI_LOGI(ret ? "self_test fail, err = %x\r\n" : "self_test pass\r\n", ret);
        #else
        ret = BK_FAIL;
        #endif
    }

    if (default_value->count > 0) {
#if CONFIG_NVS_ENCRYPTION
        ret = read_default_nvs_data();
#else
        ret = BK_FAIL;
#endif
        CLI_LOGI(ret ? "read default value fail, err = %x\r\n" : "read default value pass\r\n", ret);
    }
    if (preset_test->count > 0) {
        nvs_flash_test();
        CLI_LOGI("preset test completed\r\n");
    }
}

static void cli_nvs_cmd_handler(void **argtable)
{
    struct arg_lit *get_value     = (struct arg_lit *)argtable[1];
    struct arg_lit *set_value     = (struct arg_lit *)argtable[2];
    struct arg_str *arg_namespace = (struct arg_str *)argtable[3];
    struct arg_str *arg_type      = (struct arg_str *)argtable[4];
    struct arg_str *arg_key       = (struct arg_str *)argtable[5];
    struct arg_str *arg_value     = (struct arg_str *)argtable[6];
    bk_err_t err = 0;
    nvs_handle_t handle;

    if (get_value->count > 0 || set_value->count > 0) {
        err = nvs_flash_init();
        if (err != BK_OK && err != ESP_ERR_NVS_NO_FREE_PAGES) {
            CLI_LOGI("Failed to initialize NVS");
            return;
        }
        err = nvs_open(arg_namespace->sval[0], NVS_READWRITE, &handle);
        if (err != BK_OK) {
            CLI_LOGI("Failed to open NVS namespace");
            return;
        }

        if (strcmp(arg_type->sval[0], "i8") == 0) {
            if (get_value->count > 0)
            {
                int8_t val;
                err = nvs_get_i8(handle, arg_key->sval[0], &val);
                if (err == BK_OK) {
                    CLI_LOGI("Get NVS value: %d\n", val);
                }
            }
            else
            {
                int8_t val = (int8_t) atoi(arg_value->sval[0]);
                err = nvs_set_i8(handle, arg_key->sval[0], val);
            }
        } else if (strcmp(arg_type->sval[0], "u8") == 0) {
            if (get_value->count > 0)
            {
                uint8_t val;
                err = nvs_get_u8(handle, arg_key->sval[0], &val);
                if (err == BK_OK) {
                    CLI_LOGI("Get NVS value: %u\n", val);
                }
            }
            else
            {
                uint8_t val = (uint8_t) atoi(arg_value->sval[0]);
                err = nvs_set_u8(handle, arg_key->sval[0], val);
            }
        } else if (strcmp(arg_type->sval[0], "i16") == 0) {
            if (get_value->count > 0)
            {
                int16_t val;
                err = nvs_get_i16(handle, arg_key->sval[0], &val);
                if (err == BK_OK) {
                    CLI_LOGI("Get NVS value: %d\n", val);
                }
            }
            else
            {
                int16_t val = (int16_t) atoi(arg_value->sval[0]);
                err = nvs_set_i16(handle, arg_key->sval[0], val);
            }
        } else if (strcmp(arg_type->sval[0], "u16") == 0) {
            if (get_value->count > 0)
            {
                uint16_t val;
                err = nvs_get_u16(handle, arg_key->sval[0], &val);
                if (err == BK_OK) {
                    CLI_LOGI("Get NVS value: %u\n", val);
                }
            }
            else
            {
                uint16_t val = (uint16_t) atoi(arg_value->sval[0]);
                err = nvs_set_u16(handle, arg_key->sval[0], val);
            }
        } else if (strcmp(arg_type->sval[0], "i32") == 0) {
            if (get_value->count > 0)
            {
                int32_t val;
                err = nvs_get_i32(handle, arg_key->sval[0], &val);
                if (err == BK_OK) {
                    CLI_LOGI("Get NVS value: %d\n", val);
                }
            }
            else
            {
                int32_t val = (int32_t) atoi(arg_value->sval[0]);
                err = nvs_set_i32(handle, arg_key->sval[0], val);
            }
        } else if (strcmp(arg_type->sval[0], "u32") == 0) {
            if (get_value->count > 0)
            {
                uint32_t val;
                err = nvs_get_u32(handle, arg_key->sval[0], &val);
                if (err == BK_OK) {
                    CLI_LOGI("Get NVS value: %u\n", val);
                }
            }
            else
            {
                uint32_t val = (uint32_t) atoi(arg_value->sval[0]);
                err = nvs_set_u32(handle, arg_key->sval[0], val);
            }
        } else if (strcmp(arg_type->sval[0], "str") == 0) {
            if (get_value->count > 0)
            {
                size_t required_size;
                err = nvs_get_str(handle, arg_key->sval[0], NULL, &required_size);
                if (err == BK_OK) {
                    char *str_val = os_malloc(required_size);
                    if (str_val) {
                        err = nvs_get_str(handle, arg_key->sval[0], str_val, &required_size);
                        if (err == BK_OK) {
                            CLI_LOGI("Get NVS value: \n");
                            char *str_copy = strdup(str_val);
                            if (str_copy) {
                                char *line = strtok(str_copy, "\n");
                                while (line != NULL) {
                                    BK_DUMP_OUT("%s\n", line);
                                    line = strtok(NULL, "\n");
                                }
                                os_free(str_copy);
                            }
                        }
                        os_free(str_val);
                    } else {
                        err = BK_ERR_NO_MEM;
                    }
                }
            }
            else
            {
                err = nvs_set_str(handle, arg_key->sval[0], arg_value->sval[0]);
            }
        } else if (strcmp(arg_type->sval[0], "blob") == 0) {
            if (get_value->count > 0)
            {
                size_t required_size;
                err = nvs_get_blob(handle, arg_key->sval[0], NULL, &required_size);
                if (err == BK_OK) {
                    uint8_t *blob_val = os_malloc(required_size);
                    if (blob_val) {
                        err = nvs_get_blob(handle, arg_key->sval[0], blob_val, &required_size);
                        if (err == BK_OK) {
                            CLI_LOGI("Value (blob): \n");
                            for (size_t i = 0; i < required_size; i++) {
                                BK_DUMP_OUT("%02x ", blob_val[i]);
                            }
                            CLI_LOGI("\n");
                        }
                        os_free(blob_val);
                    } else {
                        err = BK_ERR_NO_MEM;
                    }
                }
            }
            else
            {
                err = nvs_set_blob(handle, arg_key->sval[0], arg_value->sval[0], strlen(arg_value->sval[0]));
            }
        } else {
            err = BK_FAIL;
            CLI_LOGI("Unknown type\n");
            nvs_close(handle);
            nvs_flash_deinit();
        }
        if (get_value->count > 0 && err)
            CLI_LOGI("Failed to get value : %x\n",err);

        if (set_value->count > 0)
            CLI_LOGI(err ? "Failed to set value\n" : "NVS value successfully set.\n",err);

        nvs_close(handle);
        nvs_flash_deinit();
    }
    else
    {
        print_help(argtable[0],argtable);
    }
}

static void cli_nvs_api_cmd_handler(void **argtable)
{
    struct arg_lit *eraseall       = (struct arg_lit *)argtable[1];
    struct arg_lit *getstats       = (struct arg_lit *)argtable[2];
    struct arg_lit *getused        = (struct arg_lit *)argtable[3];
    struct arg_str *arg_namespace = (struct arg_str *)argtable[4];

    bk_err_t err;
    nvs_handle_t handle;

    if (eraseall->count > 0) {
        err = nvs_flash_init();
        if (err != BK_OK && err != ESP_ERR_NVS_NO_FREE_PAGES) {
            CLI_LOGI("Failed to initialize NVS");
            return;
        }
        err = nvs_open(arg_namespace->sval[0], NVS_READWRITE, &handle);
        if (err != BK_OK) {
            CLI_LOGI("Failed to open NVS namespace");
            return;
        }
        err = nvs_erase_all(handle);
        CLI_LOGI(err ? "Erase all namespace fail, err = %x\r\n" : "Erase all namespace pass\r\n", err);

        nvs_close(handle);
        nvs_flash_deinit();
    }
    if (getstats->count > 0)
    {
        err = nvs_flash_init();
        if (err != BK_OK && err != ESP_ERR_NVS_NO_FREE_PAGES) {
            CLI_LOGI("Failed to initialize NVS");
            return;
        }
        nvs_stats_t stat1;
        err = nvs_get_stats(NULL, &stat1);
        CLI_LOGI(err ? "Get stats fail, err = %x\r\n" : "Get stats pass\r\n", err);

        if (!err)
        {
            CLI_LOGI("NVS Stats:\n");
            CLI_LOGI("Used Entries: %zu\n", stat1.used_entries);
            CLI_LOGI("Free Entries: %zu\n", stat1.free_entries);
            CLI_LOGI("Total Entries: %zu\n", stat1.total_entries);
            CLI_LOGI("Namespace Count: %zu\n", stat1.namespace_count);
        }
        nvs_flash_deinit();
    }
    if (getused->count > 0)
    {
        err = nvs_flash_init();
        if (err != BK_OK && err != ESP_ERR_NVS_NO_FREE_PAGES) {
            CLI_LOGI("Failed to initialize NVS");
            return;
        }
        err = nvs_open(arg_namespace->sval[0], NVS_READWRITE, &handle);
        if (err != BK_OK) {
            CLI_LOGI("Failed to open NVS namespace");
            return;
        }
        size_t h_count_entries;
        err = nvs_get_used_entry_count(handle, &h_count_entries);
        CLI_LOGI(err ? "Get used entry count fail, err = %x\r\n" : "Get used entry count pass\r\n", err);
        if (!err)
        {
            CLI_LOGI("NVS used_entry_count:\n");
            CLI_LOGI("Count_entries: %zu\n", h_count_entries);
        }
        nvs_close(handle);
        nvs_flash_deinit();
    }

}

static void cli_nvs_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help          = arg_lit0("h", "help",     "Display this help message");
    struct arg_lit *self_test     = arg_lit0("s", "selftest", "Set self test");
    struct arg_lit *default_value = arg_lit0("d", "default",  "Read default value in NVS");
    struct arg_lit *preset_test   = arg_lit0("p", "preset",   "Run preset test (nvs_flash_test)");
    struct arg_end *end           = arg_end(20);

    void* argtable[] = { help, self_test, default_value, preset_test, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_nvs_test_cmd_handler);
}

static void cli_nvs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help          = arg_lit0("h", "help",        "Display this help message");
    struct arg_lit *get_value     = arg_lit0("g", "get",         "Get NVS value");
    struct arg_lit *set_value     = arg_lit0("s", "set",         "Set NVS value (Note: -v/--value parameter is required)");
    struct arg_str *arg_namespace = arg_str1("n", "namespace", "<namespace>", "Namespace for the NVS");
    struct arg_str *arg_type      = arg_str1("t", "type",      "<type>",      "Type of value (i8, u8, i16, u16, i32, u32, str, blob)");
    struct arg_str *arg_key       = arg_str1("k", "key",       "<key>",       "Key name in NVS");
    struct arg_str *arg_value     = arg_str0("v", "value",     "<value>",     "Value to be set (Required for -s/--setvalue)");
    struct arg_end *end           = arg_end(20);

    void* argtable[] = { help, get_value, set_value, arg_namespace, arg_type, arg_key, arg_value, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_nvs_cmd_handler);
}

static void cli_nvs_api_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help            = arg_lit0("h", "help",         "Display this help message");
    struct arg_lit *eraseall        = arg_lit0("e", "eraseall",     "Erase all namespace in NVS");
    struct arg_lit *getstats        = arg_lit0("g", "getstats",     "Get stats in NVS");
    struct arg_lit *getused         = arg_lit0("u", "getused",         "Get used in NVS");
    struct arg_str *arg_namespace   = arg_str0("n", "namespace",    "<namespace>",             "Namespace for the NVS");
    struct arg_end *end             = arg_end(20);

    void* argtable[] = { help, eraseall, getstats, getused, arg_namespace, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_nvs_api_cmd_handler);
}

#define NVS_CMD_CNT (sizeof(s_nvs_commands) / sizeof(struct cli_command))
static const struct cli_command s_nvs_commands[] = {
    {"nvs_test", "nvs_test { selftest | default | preset } (example: nvs_test --selftest)", cli_nvs_test_cmd},
    {"nvs", "nvs { get | set | eraseall } (example: nvs -s -n myns -t i32 -k counter -v 42)", cli_nvs_cmd},
    {"nvs_api", "nvs_api { erase | getstats | getused } (example: nvs_api --getstats)", cli_nvs_api_cmd},
#if CONFIG_SUPPORT_NVS_THREAD_SAFETY_TEST
    {"nvs_race_test", "nvs_race_test { --start read_task_cnt write_task_cnt | --stop }", cli_nvs_thread_safety_test_cmd},
#endif
};

int cli_nvs_init(void)
{
    return cli_register_commands(s_nvs_commands, NVS_CMD_CNT);
}
