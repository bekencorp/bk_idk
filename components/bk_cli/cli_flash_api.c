// Copyright 2020-2024 Beken
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
#include <driver/flash_partition.h>
#include <driver/flash.h>
#include "flash_driver.h"

#define FLASH_PAGE_SIZE 256 /* each page has 256 bytes */
#define FLASH_RELIABILITY_TEST_ADDR 0x300000 /* Test start address (3MB) */
#define FLASH_RELIABILITY_TEST_SIZE 0x1000   /* Test size (4KB) */

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

/* Flash reliability test control */
static volatile bool g_flash_reliability_running = false;
static beken_thread_t g_reliability_task_handle = NULL;

static void cli_flash_api_cmd_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_lit *erase = (struct arg_lit *)argtable[1];
    struct arg_lit *read = (struct arg_lit *)argtable[2];
    struct arg_lit *write = (struct arg_lit *)argtable[3];
    struct arg_str *lm = (struct arg_str *)argtable[4];
    struct arg_str *lm_set_mode = (struct arg_str *)argtable[5];
    struct arg_str *arg_init = (struct arg_str *)argtable[6];
    struct arg_str *arg_address = (struct arg_str *)argtable[7];
    struct arg_int *arg_size = (struct arg_int *)argtable[8];
    struct arg_str *arg_get = (struct arg_str *)argtable[9];
    struct arg_str *arg_clock = (struct arg_str *)argtable[10];
    struct arg_str *arg_write_able = (struct arg_str *)argtable[11];
    struct arg_str *arg_deep_sleep_block = (struct arg_str *)argtable[12];
    struct arg_lit *arg_read_words = (struct arg_lit *)argtable[13];
    bk_err_t err = BK_OK;

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    if (arg_init->count > 0)
    {
        if (strcmp(arg_init->sval[0], "init") == 0) {
            BK_LOG_ON_ERR(bk_flash_driver_init());
            BK_DUMP_OUT("Flash driver initialized successfully\r\n");
        } else if (strcmp(arg_init->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_flash_driver_deinit());
            BK_DUMP_OUT("Flash driver deinitialized successfully\r\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for init: %s\r\n", arg_init->sval[0]);
        }
        return; // Exit after processing the init/deinit command
    }

    if (arg_get->count > 0)
    {
        if (strcmp(arg_get->sval[0], "id") == 0) {
            uint32_t flash_id = bk_flash_get_id();
		    BK_DUMP_OUT("flash_id:%x\r\n", flash_id);
        } else if (strcmp(arg_get->sval[0], "total_size") == 0) {
            uint32_t total_flash_size = bk_flash_get_current_total_size();
		    BK_DUMP_OUT("flash_size:%x\r\n", total_flash_size);
        } else {
            BK_DUMP_OUT("Invalid parameter for get: %s\r\n", arg_get->sval[0]);
        }
        return; // Exit after processing the get id/total_size command
    }

    if (arg_clock->count > 0)
    {
        if (strcmp(arg_clock->sval[0], "dpll") == 0) {
            // BK_LOG_ON_ERR(bk_flash_set_clk_dpll());
            BK_DUMP_OUT("Flash clock set up dpll successfully\r\n");
        } else if (strcmp(arg_clock->sval[0], "dco") == 0) {
            // BK_LOG_ON_ERR(bk_flash_set_clk_dco());
            BK_DUMP_OUT("Flash clock set up dco successfully\r\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for clock: %s\r\n", arg_clock->sval[0]);
        }
        return;
    }

    if (arg_write_able->count > 0)
    {
        if (strcmp(arg_write_able->sval[0], "enable") == 0) {
            BK_LOG_ON_ERR(bk_flash_write_enable());
            BK_DUMP_OUT("Flash write enable successfully\r\n");
        } else if (strcmp(arg_write_able->sval[0], "disable") == 0) {
            BK_LOG_ON_ERR(bk_flash_write_disable());
            BK_DUMP_OUT("Flash write disable successfully\r\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for write: %s\r\n", arg_write_able->sval[0]);
        }
        return;
    }

    uint32_t block_addr = (uint32_t)strtol(arg_address->sval[0], NULL, 0);  // Convert address from string to integer
    if (arg_deep_sleep_block->count > 0)
    {
        if (strcmp(arg_deep_sleep_block->sval[0], "erase") == 0) {
            BK_LOG_ON_ERR(bk_flash_erase_block(block_addr));
            BK_DUMP_OUT("Flash erase block successfully\r\n");
        } else if (strcmp(arg_deep_sleep_block->sval[0], "enter") == 0) {
            BK_LOG_ON_ERR(bk_flash_enter_deep_sleep());
            BK_DUMP_OUT("Flash enter deep sleep successfully\r\n");
        } else if (strcmp(arg_deep_sleep_block->sval[0], "exit") == 0) {
            BK_LOG_ON_ERR(bk_flash_exit_deep_sleep());
            BK_DUMP_OUT("Flash exit deep sleep successfully\r\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for write: %s\r\n", arg_deep_sleep_block->sval[0]);
        }
        return;
    }

    if (lm->count > 0)
    {
        const char *function_str = lm->sval[0];
        flash_line_mode_t line_mode;

        if (strcmp(function_str, "get") == 0) {
            flash_line_mode_t current_line_mode = bk_flash_get_line_mode();
            BK_DUMP_OUT("Current flash line mode: %d\r\n", current_line_mode);
            BK_DUMP_OUT("Get flash line mode succeeded\r\n");
        } else if (strcmp(function_str, "set") == 0 && lm_set_mode->count > 0) {
            const char *mode_str = lm_set_mode->sval[0];

            if (strcmp(mode_str, "2") == 0) {
                line_mode = FLASH_LINE_MODE_TWO;
            } else if (strcmp(mode_str, "4") == 0) {
                line_mode = FLASH_LINE_MODE_FOUR;
            } else {
                BK_DUMP_OUT("Invalid line mode: %s\r\n", mode_str);
                return;
            }

            err = bk_flash_set_line_mode(line_mode);
            if (err == BK_OK) {
                BK_DUMP_OUT("Set flash line mode to %s succeeded\r\n", mode_str);
            } else {
                BK_DUMP_OUT("Set flash line mode to %s failed, err = %x\r\n", mode_str, err);
            }
            return;
        } else {
            BK_DUMP_OUT("Invalid operation: %s\r\n", function_str);
            return;
        }

        return;
    }

    uint32_t start_addr = (uint32_t)strtol(arg_address->sval[0], NULL, 0);  // Convert address from string to integer
    uint32_t len = (uint32_t)arg_size->ival[0];

    if (erase->count > 0)
    {
        // Erase operation
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
            err = bk_flash_erase_sector(addr);
            if (err != BK_OK) {
                BK_DUMP_OUT("Erase failed at address 0x%x, err = %x\r\n", addr, err);
                break;
            }
        }
        bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
        BK_DUMP_OUT(err ? "Erase flash failed, err = %x\r\n" : "Erase flash succeeded\r\n", err);
    }
    else if (read->count > 0)
    {
        // Read operation
        uint8_t buf[FLASH_PAGE_SIZE] = {0};
        for (uint32_t addr = start_addr; addr < (start_addr + 200); addr += FLASH_PAGE_SIZE) {
            os_memset(buf, 0, FLASH_PAGE_SIZE);
            err = bk_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
            if (err != BK_OK) {
                BK_DUMP_OUT("Read failed at address 0x%x, err = %x\r\n", addr, err);
                break;
            }
            BK_DUMP_OUT("flash read addr: 0x%x\r\n", addr);

            BK_DUMP_OUT("dump read flash data:\r\n");
            for (uint32_t i = 0; i < 16; i++) {
                for (uint32_t j = 0; j < 16; j++) {
                    BK_DUMP_OUT("%02x ", buf[i * 16 + j]);
                }
                BK_DUMP_OUT("\r\n");
            }
        }
        BK_DUMP_OUT(err ? "Read flash failed, err = %x\r\n" : "Read flash succeeded\r\n", err);
    }
    else if (arg_read_words->count > 0)
    {
        uint32_t buf[FLASH_PAGE_SIZE] = {0};
        for (uint32_t addr = start_addr; addr < (start_addr + 200); addr += FLASH_PAGE_SIZE) {
            os_memset(buf, 0, FLASH_PAGE_SIZE);
            err = bk_flash_read_word(addr, buf, FLASH_PAGE_SIZE);
            if (err != BK_OK) {
                BK_DUMP_OUT("Read word failed at address 0x%x, err = %x\r\n", addr, err);
                break;
            }
            BK_DUMP_OUT("flash read word addr: 0x%x\r\n", addr);

            BK_DUMP_OUT("dump read word flash data:\r\n");
            for (uint32_t i = 0; i < 16; i++) {
                for (uint32_t j = 0; j < 16; j++) {
                    BK_LOG_RAW("%02x ", buf[i * 16 + j]);
                }
                BK_LOG_RAW("\r\n");
            }
        }
        BK_DUMP_OUT(err ? "Read flash word failed, err = %x\r\n" : "Read flash word succeeded\r\n", err);
    }
    else if (write->count > 0)
    {
        // Write operation
        uint8_t buf[FLASH_PAGE_SIZE] = {0};
        for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
            buf[i] = i;
        }

        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        for (uint32_t i = 0; i < len; i += FLASH_PAGE_SIZE) {
            err = bk_flash_write_bytes(start_addr, buf, FLASH_PAGE_SIZE);
            if (err != BK_OK) {
                BK_DUMP_OUT("Write failed at address 0x%x, err = %x\r\n", start_addr, err);
                break;
            }
        }
        bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
        BK_DUMP_OUT(err ? "Write flash failed, err = %x\r\n" : "Write flash succeeded\r\n", err);
    }
    else
    {
        print_help(argtable[0], argtable);
    }
}

static void cli_flash_protect_api_cmd_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *arg_protect_type = (struct arg_str *)argtable[1];
    struct arg_str *arg_protect_type_mode = (struct arg_str *)argtable[2];
    bk_err_t err = BK_OK;

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    if (arg_protect_type->count > 0)
    {
        flash_protect_type_t protect_mode;

        if (strcmp(arg_protect_type->sval[0], "get") == 0) {
            flash_protect_type_t current_protect_type = bk_flash_get_protect_type();
            BK_DUMP_OUT("Current flash protect type: %d\r\n", current_protect_type);
        } else if (strcmp(arg_protect_type->sval[0], "set") == 0) {
            const char *protect_mode_str = arg_protect_type_mode->sval[0];

            if (strcmp(protect_mode_str, "none") == 0) {
                protect_mode = FLASH_PROTECT_NONE;
            } else if (strcmp(protect_mode_str, "all") == 0) {
                protect_mode = FLASH_PROTECT_ALL;
            } else if (strcmp(protect_mode_str, "half") == 0) {
                protect_mode = FLASH_PROTECT_HALF;
            } else if (strcmp(protect_mode_str, "unprotect") == 0) {
                protect_mode = FLASH_UNPROTECT_LAST_BLOCK;
            } else {
                BK_DUMP_OUT("Invalid protect mode: %s\r\n", protect_mode_str);
                return;
            }

            err = bk_flash_set_protect_type(protect_mode);
            if (err == BK_OK) {
                BK_DUMP_OUT("Flash protect type mode is %s succeeded\r\n", protect_mode_str);
            } else {
                BK_DUMP_OUT("Flash protect type mode is %s failed, err = %x\r\n", protect_mode_str, err);
            }
        } else {
            BK_DUMP_OUT("Invalid parameter for protect type: %s\r\n", arg_protect_type->sval[0]);
        }
        return;
    }
    else
    {
        print_help(argtable[0], argtable);
    }
}

static void cli_argflash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_lit *erase = arg_lit0("e", "erase", "Erase flash");
    struct arg_lit *read = arg_lit0("r", "read", "Read flash");
    struct arg_lit *write = arg_lit0("w", "write", "Write flash");
    struct arg_str *lm = arg_str0("l", "line mode", "<get/set>", "Get or set flash line mode");
    struct arg_str *lm_set_mode = arg_str0("m", "set mode", "<2/4>", "Set flash line mode");
    struct arg_str *arg_init = arg_str0("i", "init", "<init/deinit>", "Init or deinit flash");
    struct arg_str *arg_address = arg_str0("a", "address", "<flash_address>", "Flash address");
    struct arg_int *arg_size = arg_int0("s", "size", "<size>", "flash size");
    struct arg_str *arg_get = arg_str0("g", "get", "<id/total_size>", "Get flash status");
    struct arg_str *arg_clock = arg_str0("c", "flash clock", "<dpll/dco>", "Set up flash clock");
    struct arg_str *arg_write_able = arg_str0("v", "flash write", "<enable/disable>", "Flash enable or disable");
    struct arg_str *arg_deep_sleep_block = arg_str0("k", "flash deep sleep and block", "<erase/enter/exit>", "Flash deep sleep config and flash address block");
    struct arg_lit *arg_read_words = arg_lit0("b", "read words", "Read flash words");
    struct arg_end *end = arg_end(20);

    void* argtable[] = { help, erase, read, write, lm, lm_set_mode, arg_init, arg_address, arg_size, arg_get, arg_clock, arg_write_able, arg_deep_sleep_block, arg_read_words, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_flash_api_cmd_handler);
}

static void cli_argflash_protect_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *arg_protect_type = arg_str0("p", "flash protect type", "<set/get>", "Flash protect type config");
    struct arg_str *arg_protect_type_mode = arg_str0("m", "flash protect type mode", "<none/all/half/unprotect>", "Flash protect type mode config");
    struct arg_end *end = arg_end(20);

    void* argtable[] = { help, arg_protect_type, arg_protect_type_mode, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_flash_protect_api_cmd_handler);
}

/* Flash reliability test task */
static void flash_reliability_test_task(void)
{
    uint32_t test_addr = FLASH_RELIABILITY_TEST_ADDR;
    uint32_t test_size = FLASH_RELIABILITY_TEST_SIZE;
    uint8_t *write_buf = (uint8_t *)os_malloc(test_size);
    uint8_t *read_buf = (uint8_t *)os_malloc(test_size);
    
    if (write_buf == NULL || read_buf == NULL) {
        BK_DUMP_OUT("Failed to allocate memory for flash reliability test\r\n");
        g_flash_reliability_running = false;
        return;
    }

    uint32_t cycle_count = 0;
    uint32_t error_count = 0;

    while (1) {
        /* Wait until start command */
        while (!g_flash_reliability_running) {
            rtos_thread_sleep(1);
        }

        /* Reset statistics for new test */
        test_addr = FLASH_RELIABILITY_TEST_ADDR;
        test_size = FLASH_RELIABILITY_TEST_SIZE;
        cycle_count = 0;
        error_count = 0;
        
        BK_DUMP_OUT("Flash reliability test started at address 0x%x, size 0x%x\r\n", test_addr, test_size);

        while (g_flash_reliability_running) {
            cycle_count++;

            /* Generate test pattern - use cycle count to make data unique */
            for (uint32_t i = 0; i < test_size; i++) {
                write_buf[i] = (uint8_t)((cycle_count + i) & 0xFF);
            }

            /* Erase flash sectors */
            bk_flash_set_protect_type(FLASH_PROTECT_NONE);
            for (uint32_t addr = test_addr; addr < (test_addr + test_size); addr += FLASH_SECTOR_SIZE) {
                bk_flash_erase_sector(addr);
            }
            bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

            /* Write data to flash */
            bk_flash_set_protect_type(FLASH_PROTECT_NONE);
            for (uint32_t i = 0; i < test_size; i += FLASH_PAGE_SIZE) {
                bk_flash_write_bytes(test_addr + i, write_buf + i, FLASH_PAGE_SIZE);
            }
            bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

            /* Read data from flash */
            for (uint32_t i = 0; i < test_size; i += FLASH_PAGE_SIZE) {
                bk_flash_read_bytes(test_addr + i, read_buf + i, FLASH_PAGE_SIZE);
            }

            /* Verify data */
            bool verify_ok = true;
            for (uint32_t i = 0; i < test_size; i++) {
                if (write_buf[i] != read_buf[i]) {
                    verify_ok = false;
                    error_count++;
                    if (error_count <= 10) { /* Only print first 10 errors */
                        BK_DUMP_OUT("Verify error at offset 0x%x: expected 0x%02x, got 0x%02x\r\n", 
                                    i, write_buf[i], read_buf[i]);
                    }
                }
            }

            if (!verify_ok && error_count == 10) {
                BK_DUMP_OUT("More verification errors occurred, only showing first 10...\r\n");
            }

            /* Print progress every 100 cycles */
            if ((cycle_count % 100) == 0) {
                BK_DUMP_OUT("Flash test cycle: %lu, errors: %lu\r\n", cycle_count, error_count);
            }

            /* Small delay to prevent task starvation */
            rtos_thread_sleep(1);
        }

        /* Final statistics */
        BK_DUMP_OUT("Flash reliability test stopped\r\n");
        BK_DUMP_OUT("Total cycles: %lu\r\n", cycle_count);
        BK_DUMP_OUT("Total errors: %lu\r\n", error_count);
        BK_DUMP_OUT("Error rate: %.2f%%\r\n", error_count > 0 ? (100.0f * error_count / test_size / cycle_count) : 0.0f);
    }

    /* Never reach here */
    os_free(write_buf);
    os_free(read_buf);
    g_reliability_task_handle = NULL;
}

static void cli_argflash_reliability_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        BK_DUMP_OUT("Usage: flash_reliability <start/end>\r\n");
        return;
    }

    const char *action = argv[1];

    if (strcmp(action, "start") == 0) {
        /* If already running, do nothing */
        if (g_flash_reliability_running) {
            BK_DUMP_OUT("Flash reliability test is already running\r\n");
            return;
        }

        /* Create task if it doesn't exist */
        if (g_reliability_task_handle == NULL) {
            bk_err_t ret = rtos_create_thread(&g_reliability_task_handle,
                                              20,
                                              "flash_reli",
                                              (beken_thread_function_t)flash_reliability_test_task,
                                              1024,
                                              (beken_thread_arg_t)NULL);
            if (ret != BK_OK) {
                BK_DUMP_OUT("Failed to create flash reliability test task, err=%d\r\n", ret);
                g_reliability_task_handle = NULL;
                return;
            }
        }

        /* Start new test immediately by setting flag */
        g_flash_reliability_running = true;
        BK_DUMP_OUT("Flash reliability test started\r\n");
    } else if (strcmp(action, "end") == 0) {
        if (!g_flash_reliability_running) {
            BK_DUMP_OUT("Flash reliability test is not running\r\n");
            return;
        }

        BK_DUMP_OUT("Stopping flash reliability test...\r\n");
        g_flash_reliability_running = false;
        /* Statistics will be printed by task itself */
        BK_DUMP_OUT("Flash reliability test stopped\r\n");
    } else if (strcmp(action, "help") == 0 || strcmp(action, "-h") == 0 || strcmp(action, "--help") == 0) {
        BK_DUMP_OUT("Usage: flash_reliability <start/end>\r\n");
        BK_DUMP_OUT("  start - Start flash reliability test\r\n");
        BK_DUMP_OUT("  end   - Stop flash reliability test\r\n");
    } else {
        BK_DUMP_OUT("Invalid argument: %s (use 'start' or 'end')\r\n", action);
    }
}

#define FLASH_API_CMD_CNT (sizeof(s_flash_api_commands) / sizeof(struct cli_command))
static const struct cli_command s_flash_api_commands[] = {
    {"argflash", "argflash { init | deinit | erase | read | write | line mode <get/set> | set line mode <2/4> | get flash <id/total_size> | set flash <dpll/dco> | flash write <enable/disable> | <enter/exit> flash deep sleep or erase flash block | Read flash words | <suspend/resume> flash register }", cli_argflash_cmd},
    {"argflash_protect", "argflash { <set/get> flash protect type | set flash protect type mode <none/all/half/unprotect> }", cli_argflash_protect_cmd},
    {"flash_reliability", "flash_reliability <start/end>", cli_argflash_reliability_cmd},
};

int cli_flash_api_test_init(void)
{
    return cli_register_commands(s_flash_api_commands, FLASH_API_CMD_CNT);
}
// eof

