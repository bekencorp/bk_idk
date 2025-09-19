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
#include <stdbool.h>
#include <driver/i2c_types.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "gpio_driver.h"

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

#define EEPROM_DEV_ADDR          0x50
#define EEPROM_MEM_ADDR          0x10
#define I2C_SLAVE_ADDR           0x42
#define I2C_WRITE_WAIT_MAX_MS    (500)
#define I2C_READ_WAIT_MAX_MS     (500)
#define CAMERA_DEV_ADDR          (0x21)

static void cli_i2c_api_cmd_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];
    struct arg_str *int_config = (struct arg_str *)argtable[2];
    struct arg_int *baud_rate = (struct arg_int *)argtable[3];
    struct arg_str *mode = (struct arg_str *)argtable[4];
    struct arg_str *addr = (struct arg_str *)argtable[5];
    struct arg_str *id = (struct arg_str *)argtable[6];
    struct arg_str *connect = (struct arg_str *)argtable[7];
    struct arg_str *length = (struct arg_str *)argtable[8];
    struct arg_str *set = (struct arg_str *)argtable[9];
    struct arg_str *function = (struct arg_str *)argtable[10];
    // struct arg_str *time = (struct arg_str *)argtable[11];
    // bk_err_t err = BK_OK;

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (driver->count > 0)
    {
        if (strcmp(driver->sval[0], "init") == 0) {
            BK_LOG_ON_ERR(bk_i2c_driver_init());
            BK_DUMP_OUT("I2C driver initialized successfully\r\n");
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_i2c_driver_deinit());
            BK_DUMP_OUT("I2C driver deinitialized successfully\r\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for init: %s\r\n", driver->sval[0]);
        }
        return;
    }

    else if (int_config->count > 0) {
        uint32_t i2c_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);

        if (strcmp(int_config->sval[0], "init") == 0) {
            i2c_config_t i2c_cfg = {0};
            os_memset(&i2c_cfg, 0, sizeof(i2c_cfg.slave_addr ));
            i2c_cfg.baud_rate = 400000;
            i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
            i2c_cfg.slave_addr = I2C_SLAVE_ADDR;

            if (baud_rate->count > 0) {
                i2c_cfg.baud_rate = baud_rate->ival[0];
                BK_DUMP_OUT("Baud Rate: %d\r\n", baud_rate->ival[0]);
            }
            if (mode->count > 0) {
                if (os_strtoul(mode->sval[0], NULL, 10) == 7){
                    i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
                } else {
                    i2c_cfg.addr_mode = I2C_ADDR_MODE_10BIT;
                }
                BK_DUMP_OUT("I2C addr_mode: %d\r\n", i2c_cfg.addr_mode);
            }
            if (addr->count > 0) {
                i2c_cfg.slave_addr = (uint16_t)os_strtoul(addr->sval[0], NULL, 10);
                BK_DUMP_OUT("I2C slave_addr: %d\r\n", i2c_cfg.slave_addr);
            }

            BK_LOG_ON_ERR(bk_i2c_init(i2c_id, &i2c_cfg));
            BK_DUMP_OUT("i2c(%d) init\n", i2c_id);
        } else if (strcmp(int_config->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_i2c_deinit(i2c_id));
            BK_DUMP_OUT("i2c(%d) deinitialized  successfully\n", i2c_id);
        } else {
            BK_DUMP_OUT("Invalid parameter for i2c init_config: %s\r\n", int_config->sval[0]);
        }

    }

    else if (connect->count > 0) {
        uint32_t i2c_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
         if (strcmp(connect->sval[0], "master_write") == 0) {
            uint8_t data_buf[10] = {0};

            for (uint32_t i = 0; i < 10; i++) {
                data_buf[i] = i & 0xff;
            }

            BK_LOG_ON_ERR(bk_i2c_master_write(i2c_id, I2C_SLAVE_ADDR, data_buf, 10, I2C_WRITE_WAIT_MAX_MS));
            BK_DUMP_OUT("I2C initialized successfully, i2c_id=%d\n", i2c_id);
            BK_DUMP_OUT("cli_test i2c master write succeed");
         } else if (strcmp(connect->sval[0], "master_read") == 0) {
            uint8_t data_buf[10] = {0};
            BK_LOG_ON_ERR(bk_i2c_master_read(i2c_id, I2C_SLAVE_ADDR, data_buf, 10, I2C_READ_WAIT_MAX_MS));

            for (uint32_t i = 0; i < 10; i++) {
                BK_DUMP_OUT("cli_test i2c master read 0x%x,\n", data_buf[i]);
            }

    #ifndef CONFIG_SIM_I2C
        } else if (strcmp(connect->sval[0], "slave_write") == 0) {
            uint8_t data_buf[10] = {0};

            for (uint32_t i = 0; i < 10; i++) {
                data_buf[i] = i & 0xff;
            }

            BK_LOG_ON_ERR(bk_i2c_slave_write(i2c_id, data_buf, 10, BEKEN_NEVER_TIMEOUT));
            BK_DUMP_OUT("cli_test i2c slave write succeed");
        } else if (strcmp(connect->sval[0], "slave_read") == 0) {
            uint8_t data_buf[10] = {0};
            BK_LOG_ON_ERR(bk_i2c_slave_read(i2c_id, data_buf, 10, BEKEN_NEVER_TIMEOUT));

            for (uint32_t i = 0; i < 10; i++) {
                BK_DUMP_OUT("cli_test i2c_slave read 0x%x,\n", data_buf[i]);
            }

        } else if (strcmp(connect->sval[0], "set_slave_addr") == 0) {
            uint32_t slave_addr = 0;

            if (addr->count > 0) {
                slave_addr = (uint32_t)os_strtoul(addr->sval[0], NULL, 16);
                BK_DUMP_OUT("I2C slave_addr: %d\r\n", slave_addr);
            }

            bk_i2c_set_slave_address(i2c_id, slave_addr);
            BK_DUMP_OUT("i2c_slave set address 0x%x.\n", slave_addr);
    #endif
        } else if (strcmp(connect->sval[0], "memory_write") == 0) {
            uint32_t buf_len = (uint32_t)os_strtoul(length->sval[0], NULL, 16);
            uint8_t *data_buf = os_malloc(buf_len);

            if(NULL == data_buf)
            {
                BK_DUMP_OUT("malloc fail\r\n");
                return;
            }

            for (uint32_t i = 0; i < buf_len; i++) {
                data_buf[i] = (i + 1) & 0xff;
            }

            uint32_t dev_addr = (uint32_t)os_strtoul(addr->sval[0], NULL, 16);
            i2c_mem_param_t mem_param = {0};
            mem_param.dev_addr = dev_addr;
            mem_param.mem_addr = EEPROM_MEM_ADDR;
            mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
            mem_param.data = data_buf;
            mem_param.data_size = buf_len;
            mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
            BK_LOG_ON_ERR(bk_i2c_memory_write(i2c_id, &mem_param));

            if (data_buf) {
                os_free(data_buf);
                data_buf = NULL;
            }

            BK_DUMP_OUT("i2c(%d) memory_write buf_len:%d\r\n", i2c_id, buf_len);
        } else if (strcmp(connect->sval[0], "memory_read") == 0) {
            uint32_t buf_len = (uint32_t)os_strtoul(length->sval[0], NULL, 16);
            uint8_t *data_buf = os_zalloc(buf_len);

            if(NULL == data_buf)
            {
                BK_DUMP_OUT("os_zalloc fail\r\n");
                return;
            }

            i2c_mem_param_t mem_param = {0};
            mem_param.dev_addr = EEPROM_DEV_ADDR;
            mem_param.mem_addr = EEPROM_MEM_ADDR;
            mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
            mem_param.data = data_buf;
            mem_param.data_size = buf_len;
            mem_param.timeout_ms = I2C_WRITE_WAIT_MAX_MS;
            BK_LOG_ON_ERR(bk_i2c_memory_read(i2c_id, &mem_param));
            for (uint32_t i = 0; i < buf_len; i++) {
                BK_DUMP_OUT("i2c_read_buf[%d]=%x\r\n", i, data_buf[i]);
            }
            if (data_buf) {
                os_free(data_buf);
                data_buf = NULL;
            }
            BK_DUMP_OUT("i2c(%d) memory_read buf_len:%d\r\n", i2c_id, buf_len);
        }

    }

    else if (set->count > 0) {
        uint32_t i2c_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(set->sval[0], "baud_rate") == 0) {
            uint32_t i2c_baud_rate = 400000;
            i2c_baud_rate = baud_rate->ival[0];
            BK_LOG_ON_ERR(bk_i2c_set_baud_rate(i2c_id, i2c_baud_rate));
            BK_DUMP_OUT("I2c (%d) set baud rate %d succeed\n", i2c_id, i2c_baud_rate);
        } else if (strcmp(set->sval[0], "slave_address") == 0) {
            uint16_t i2c_slave_addr = 0;
            i2c_slave_addr = (uint16_t)os_strtoul(addr->sval[0], NULL, 16);
            BK_LOG_ON_ERR(bk_i2c_set_slave_address(i2c_id, i2c_slave_addr));
            BK_DUMP_OUT("I2c (%d) set slave address 0x%x succeed\n", i2c_id, i2c_slave_addr);
        } else {
            BK_DUMP_OUT("Invalid parameter for i2c(%d) set config:%s\r\n", i2c_id, set->sval[0]);
        }
    }

    else if (function->count > 0) {
        uint32_t i2c_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);

        if (strcmp(function->sval[0], "en_inter") == 0) {
            BK_LOG_ON_ERR(bk_i2c_enable_interrupt(i2c_id));
            BK_DUMP_OUT("I2c (%d) enable interrupt succeed\n", i2c_id);
        } else if (strcmp(function->sval[0], "dis_inter") == 0) {
            BK_LOG_ON_ERR(bk_i2c_disable_interrupt(i2c_id));
            BK_DUMP_OUT("I2c (%d) disable interrupt succeed\n", i2c_id);
        } else if (strcmp(function->sval[0], "is_busy") == 0) {
            BK_LOG_ON_ERR(bk_i2c_is_bus_busy(i2c_id));
            BK_DUMP_OUT("Check i2c(%d) busy succeed\n", i2c_id);
        } else if (strcmp(function->sval[0], "grt_cur") == 0) {
            BK_LOG_ON_ERR(bk_i2c_get_cur_action(i2c_id));
            BK_DUMP_OUT("Get i2c(%d) current action succeed\n", i2c_id);
        }
        else if (strcmp(function->sval[0], "get_bus") == 0) {
            BK_LOG_ON_ERR(bk_i2c_get_busstate(i2c_id));
            BK_DUMP_OUT("Init the SIM I2C (%d) succeed\n", i2c_id);
        } else if (strcmp(function->sval[0], "get_tran") == 0) {
            BK_LOG_ON_ERR(bk_i2c_get_transstate(i2c_id));
            BK_DUMP_OUT("Get i2c(%d) bus status idle or busy succeed\n", i2c_id);
        } else {
            BK_DUMP_OUT("Invalid parameter for i2c(%d) function config: %s\r\n", i2c_id, function->sval[0]);
        }
    }

    else
    {
        print_help(argtable[0], argtable);
    }
}

static void cli_argi2c_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit>", "Init or deinit I2C driver");
    struct arg_str *init_config = arg_str0("g", "init_config", "<init/deinit>", "Init or deinit i2c config");
    struct arg_int *baud_rate = arg_int0("b", "baud_rate", "<baud_rate>", "I2C baud rate");
    struct arg_str *mode = arg_str0("m", "mode", "<i2c mode>", "I2c mdoe config");
    struct arg_str *addr = arg_str0("a", "addr", "<i2c addr>", "I2c addr config");
    struct arg_str *id = arg_str0("i", "id", "<id>", "I2C ID");
    struct arg_str *connect = arg_str0("c", "connect", "<master_write/master_read/slave_write/slave_read/set_slave_addr/memory_write/memory_read>", "I2C connect");
    struct arg_str *length = arg_str0("l", "data_length", "<data_length>", "I2C data length");
    struct arg_str *set = arg_str0("s", "set_config", "<baud_rate/slave_address>", "I2C set config");
    struct arg_str *function = arg_str0("f", "function_config", "<en_inter/dis_inter/is_busy/grt_cur/get_bus/get_tran>", "I2C set function");
    struct arg_end *end = arg_end(20);

    void* argtable[] = { help, driver, init_config, baud_rate, mode, addr, id, connect, /*time,*/ length, set, function, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_i2c_api_cmd_handler);
}

#define I2C_API_CMD_CNT (sizeof(s_i2c_api_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_i2c_api_commands[] = {
    {"argi2c", "argi2c { help | driver | arg_size }", cli_argi2c_cmd},
};

int cli_i2c_api_init(void)
{
	BK_LOG_ON_ERR(bk_i2c_driver_init());
    return cli_register_commands(s_i2c_api_commands, I2C_API_CMD_CNT);
}
