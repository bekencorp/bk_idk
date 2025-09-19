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
#include <driver/uart.h>
#include <driver/trng.h>
#include "uart_statis.h"

#define UART_ID_0 0
#define UART_ID_1 1
#define UART_ID_2 2
#define UART_ID_MAX 3

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

static void cli_uart_rx_isr(uart_id_t id, void *param)
{
	BK_DUMP_OUT("uart_rx_isr(%d)\n", id);
}

static void cli_uart_tx_isr(uart_id_t id, void *param)
{
	BK_DUMP_OUT("uart_tx_isr(%d)\n", id);
}

static void cli_uart_api_cmd_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];
    struct arg_str *init_config = (struct arg_str *)argtable[2];
    struct arg_str *set = (struct arg_str *)argtable[3];
    struct arg_int *id = (struct arg_int *)argtable[4];
    struct arg_int *baud_rate = (struct arg_int *)argtable[5];
    struct arg_int *bits = (struct arg_int *)argtable[6];
    struct arg_int *threshold = (struct arg_int *)argtable[7];
    struct arg_str *parity = (struct arg_str *)argtable[8];
    struct arg_str *stop_bits = (struct arg_str *)argtable[9];
    struct arg_str *flow_ctrl = (struct arg_str *)argtable[10];
    struct arg_str *src_clk = (struct arg_str *)argtable[11];
    struct arg_lit *used = (struct arg_lit *)argtable[12];
    struct arg_str *able = (struct arg_str *)argtable[13];
    struct arg_str *connect = (struct arg_str *)argtable[14];



    if (help->count > 0)
    {
        print_help("arguart", argtable);
        return;
    }

    else if (driver->count > 0)
    {
        if (strcmp(driver->sval[0], "init") == 0) {
            BK_LOG_ON_ERR(bk_uart_driver_init());
            BK_DUMP_OUT("UART driver initialized successfully\r\n");
        }  else {
            BK_DUMP_OUT("Invalid parameter for driver: %s\r\n", driver->sval[0]);
        }
        return;
    }

    else if (used->count > 0)
    {
        uart_id_t uart_id = (uart_id_t)id->ival[0];

        if (uart_id >= UART_ID_MAX) {
            BK_DUMP_OUT("Invalid UART ID: %d\n", uart_id);
            return;
        }

        int in_use = bk_uart_is_in_used(uart_id);
        if (in_use) {
            BK_DUMP_OUT("UART ID %d is in use.\n", uart_id);
        } else {
            BK_DUMP_OUT("UART ID %d is not in use.\n", uart_id);
        }
        BK_DUMP_OUT("Successfully get the information whether uart is used or not.\n");
        return;
    }

    else if (init_config->count > 0)
    {
        uart_id_t uart_id = id->ival[0];

        if (strcmp(init_config->sval[0], "init") == 0) {

            uart_config_t config = {0};
            os_memset(&config, 0, sizeof(uart_config_t));

            if (baud_rate->count > 0) {
                config.baud_rate = baud_rate->ival[0];
            }
            if (bits->count > 0) {
                config.data_bits = bits->ival[0];
            }
            if (parity->count > 0) {
                config.parity = (uart_parity_t)os_strtoul(parity->sval[0], NULL, 10);
            }
            if (stop_bits->count > 0) {
                config.stop_bits = (uart_stop_bits_t)os_strtoul(stop_bits->sval[0], NULL, 10);
            }

            if (flow_ctrl->count > 0) {
                config.flow_ctrl = os_strtoul(flow_ctrl->sval[0], NULL, 10);
            }
            if (src_clk->count > 0) {
                config.src_clk = os_strtoul(src_clk->sval[0], NULL, 10);
            }

            BK_LOG_ON_ERR(bk_uart_init(uart_id, &config));
            BK_DUMP_OUT("UART initialized successfully, uart_id=%d\n", uart_id);
        } else if (strcmp(init_config->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_uart_deinit(uart_id));
            BK_DUMP_OUT("UART deinitialized successfully, uart_id=%d\n", uart_id);
        } else {
            BK_DUMP_OUT("Invalid configuration parameter: %s\n", init_config->sval[0]);
        }
        return;
    }

    else if (set->count > 0) {
        uart_id_t uart_id = id->ival[0]; // Ensure the uart_id is fetched first
        if (strcmp(set->sval[0], "baud") == 0) {
            uint32_t baud_rate_val = baud_rate->ival[0];
            BK_LOG_ON_ERR(bk_uart_set_baud_rate(uart_id, baud_rate_val));
            BK_DUMP_OUT("UART id %d set baud_rate %d successfully,\n", uart_id, baud_rate_val);
        } else if (strcmp(set->sval[0], "data_bits") == 0) {
            uart_data_bits_t data_bits_val = (uart_data_bits_t)bits->ival[0];
            BK_LOG_ON_ERR(bk_uart_set_data_bits(uart_id, data_bits_val));
            BK_DUMP_OUT("UART id %d set data_bits %d successfully,\n", uart_id, data_bits_val);
        } else if (strcmp(set->sval[0], "stop_bits") == 0) {
            uart_stop_bits_t stop_bits_val = (uart_stop_bits_t)os_strtoul(stop_bits->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_uart_set_stop_bits(uart_id, stop_bits_val));
            BK_DUMP_OUT("UART id %d set stop_bits %d successfully,\n", uart_id, stop_bits_val);
        } else if (strcmp(set->sval[0], "parity") == 0) {
            uart_parity_t parity_val = (uart_parity_t)os_strtoul(parity->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_uart_set_parity(uart_id, parity_val));
            BK_DUMP_OUT("UART id %d set parity %d successfully,\n", uart_id, parity_val);
        } else if (strcmp(set->sval[0], "hw_flow") == 0 || strcmp(set->sval[0], "rx_full") == 0 || strcmp(set->sval[0], "tx_empty") == 0) {
            uint8_t threshold_val = (uint8_t)threshold->ival[0];
            if (strcmp(set->sval[0], "hw_flow") == 0) {
                BK_LOG_ON_ERR(bk_uart_set_hw_flow_ctrl(uart_id, threshold_val));
                BK_DUMP_OUT("UART id %d set rx_threshold %d successfully,\n", uart_id, threshold_val);
            } else if (strcmp(set->sval[0], "rx_full") == 0) {
                BK_LOG_ON_ERR(bk_uart_set_rx_full_threshold(uart_id, threshold_val));
                BK_DUMP_OUT("UART id %d set rx_fifo_threshold %d successfully,\n", uart_id, threshold_val);
            } else if (strcmp(set->sval[0], "tx_empty") == 0) {
                BK_LOG_ON_ERR(bk_uart_set_tx_empty_threshold(uart_id, threshold_val));
                BK_DUMP_OUT("UART id %d set tx_fifo_threshold %d successfully,\n", uart_id, threshold_val);
            }
        } else if (strcmp(set->sval[0], "rx_timeout") == 0) {
            uart_rx_stop_detect_time_t rx_timeout_val = (uart_rx_stop_detect_time_t)os_strtoul(src_clk->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_uart_set_rx_timeout(uart_id, rx_timeout_val));
            BK_DUMP_OUT("UART id %d set rx_timeout %d successfully,\n", uart_id, rx_timeout_val);
        } else if (strcmp(set->sval[0], "enable_rx") == 0) {
            uint8_t uart_ext = (uint8_t)threshold->ival[0];
            if (uart_ext > 1){
                uart_ext = 1;
                BK_DUMP_OUT("The max of uart_ext is 1\n", uart_id);
            }
            BK_LOG_ON_ERR(bk_uart_set_enable_rx(uart_id, uart_ext));
            BK_DUMP_OUT("UART id %d set enable_tx %d successfully,\n", uart_id, uart_ext);
        } else if (strcmp(set->sval[0], "enable_tx") == 0) {
            uint8_t uart_ext = (uint8_t)threshold->ival[0];
            if (uart_ext > 1){
                uart_ext = 1;
                BK_DUMP_OUT("The max of uart_ext is 1\n", uart_id);
            }
            BK_LOG_ON_ERR(bk_uart_set_enable_tx(uart_id, uart_ext));
            BK_DUMP_OUT("UART id %d set enable_tx %d successfully,\n", uart_id, uart_ext);
        } else {
            BK_DUMP_OUT("UART %d invalid parameter for set %s failed,\n", uart_id, set->sval[0]);
        }
    }

    else if (able->count > 0) {
        uart_id_t uart_id = (uart_id_t)id->ival[0];
        if (strcmp(able->sval[0], "dis_hw") == 0) {
            BK_LOG_ON_ERR(bk_uart_disable_hw_flow_ctrl(uart_id));
            BK_DUMP_OUT("UART id %d set disable_hw_flow_ctrl successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "en_tx") == 0) {
            BK_LOG_ON_ERR(bk_uart_enable_tx_interrupt(uart_id));
            BK_DUMP_OUT("UART id %d set enable_tx_interrupt successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "dis_tx") == 0) {
            BK_LOG_ON_ERR(bk_uart_disable_tx_interrupt(uart_id));
            BK_DUMP_OUT("UART id %d set disable_tx_interrupt successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "en_rx") == 0) {
            BK_LOG_ON_ERR(bk_uart_enable_rx_interrupt(uart_id));
            BK_DUMP_OUT("UART id %d set enable_rx_interrupt successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "dis_rx") == 0) {
            BK_LOG_ON_ERR(bk_uart_disable_rx_interrupt(uart_id));
            BK_DUMP_OUT("UART id %d set disable_rx_interrupt successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "rx_isr") == 0) {
            BK_LOG_ON_ERR(bk_uart_register_rx_isr(uart_id, cli_uart_rx_isr, NULL));
            BK_DUMP_OUT("UART id %d set register_rx_isr successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "tx_isr") == 0) {
            BK_LOG_ON_ERR(bk_uart_register_tx_isr(uart_id, cli_uart_tx_isr, NULL));
            BK_DUMP_OUT("UART id %d set register_rx_isr successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "dis_txx") == 0) {
            BK_LOG_ON_ERR(bk_uart_disable_tx(uart_id));
            BK_DUMP_OUT("UART id %d uart disable rx successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "dis_rxx") == 0) {
            BK_LOG_ON_ERR(bk_uart_disable_rx(uart_id));
            BK_DUMP_OUT("UART id %d uart disable tx successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "en_sw") == 0) {
            BK_LOG_ON_ERR(bk_uart_enable_sw_fifo(uart_id));
            BK_DUMP_OUT("UART id %d enable sw fifo successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "dis_sw") == 0) {
            BK_LOG_ON_ERR(bk_uart_disable_sw_fifo(uart_id));
            BK_DUMP_OUT("UART id %d disable sw fifo successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "re_rx_isr") == 0) {
            BK_LOG_ON_ERR(bk_uart_recover_rx_isr(uart_id));
            BK_DUMP_OUT("UART id %d recover rx isr successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "tx_over") == 0) {
            BK_LOG_ON_ERR(bk_uart_is_tx_over(uart_id));
            BK_DUMP_OUT("UART id %d is tx over successfully,\n", uart_id);
        } else if (strcmp(able->sval[0], "pm") == 0) {
            BK_LOG_ON_ERR(bk_uart_pm_backup(uart_id));
            BK_DUMP_OUT("UART id %d pm backup successfully,\n", uart_id);
        } else {
            BK_DUMP_OUT("UART invalid parameter for  %s,\n",set->sval[0]);
        }
    }

    else if (connect->count > 0) {
        uart_id_t uart_id = (uart_id_t)id->ival[0];
        if (strcmp(connect->sval[0], "write") == 0) {
            uint32_t buf_len = (uint32_t)bits->ival[0];
            uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
            if (send_data == NULL) {
                BK_DUMP_OUT("send buffer malloc failed\r\n");
                return;
            }
            os_memset(send_data, 0, buf_len);
            for (int i = 0; i < buf_len; i++) {
                send_data[i] = i & 0xff;
            }
            BK_LOG_ON_ERR(bk_uart_write_bytes(uart_id, send_data, buf_len));
            if (send_data) {
                os_free(send_data);
            }
            send_data = NULL;
            BK_DUMP_OUT("uart write succedd, uart_id=%d, data_len:%d\n", uart_id, buf_len);
        } else if (strcmp(connect->sval[0], "read") == 0) {
            uint32_t buf_len = (uint32_t)bits->ival[0];
            uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
            uint32_t time_out = (uint32_t)os_strtoul(src_clk->sval[0], NULL, 10);
            if (recv_data == NULL) {
                BK_DUMP_OUT("recv buffer malloc failed\r\n");
                return;
            }
            if (time_out < 0) {
                time_out = BEKEN_WAIT_FOREVER;
                goto exit;
            }
            int data_len = bk_uart_read_bytes(uart_id, recv_data, buf_len, time_out);
            BK_DUMP_OUT("uart read succedd, uart_id=%d\n", uart_id);
            if (data_len < 0) {
                BK_DUMP_OUT("uart read failed, ret:-0x%x\r\n", -data_len);
                os_free(recv_data);
                recv_data = NULL;
            }
            BK_DUMP_OUT("uart read, uart_id=%d, time_out:%x data_len:%d\n", uart_id, time_out, data_len);
            for (int i = 0; i < data_len; i++) {
                BK_DUMP_OUT("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
            }
        exit:
            if (recv_data) {
                os_free(recv_data);
                recv_data = NULL;
            }
        }
    }
    else
    {
        print_help(argtable[0], argtable);
    }
}

static void cli_arguart_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init>", "Init uart driver");
    struct arg_str *init_config = arg_str0("a", "init_config", "<init/deinit>", "Init or deinit uart config");
    struct arg_str *set = arg_str0("s", "set", "<baud/data_bits/stop_bits/parity/hw_flow/rx_full/tx_empty/rx_timeout/>", "Set uart parameters");
    struct arg_int *id = arg_int0("i", "id", "<id>", "UART ID");
    struct arg_int *baud_rate = arg_int0("b", "baud_rate", "<baud_rate>", "UART baud rate");
    struct arg_int *bits = arg_int0("t", "bits", "<bits>", "Data bits");
    struct arg_int *threshold = arg_int0("l", "threshold", "<hw_flow/rx_full/tx_empty/rx_timeout/enable_rx/enable_tx>", "Set uart threshold");
    struct arg_str *parity = arg_str0("p", "parity", "<parity>", "Set uart parity");
    struct arg_str *stop_bits = arg_str0("o", "stop_bits", "<stop_bits>", "Set uart config stop_bits");
    struct arg_str *flow_ctrl = arg_str0("w", "flow_ctrl", "<flow_ctrl>", "Set uart config flow control");
    struct arg_str *src_clk = arg_str0("k", "src_clk", "<src_clk>", "Set uart config source clock");
    struct arg_lit *used = arg_lit0("u", "used", "Check if uart is in use");
    struct arg_str *able = arg_str0("e", "enable or disable", "<dis_hw/en_tx/dis_tx/en_rx/dis_rx/rx_isr/tx_isr/dis_txx/dis_rxx/en_sw/dis_sw/re_rx_isr/tx_over/pm>", "Enable or disable uart");
    struct arg_str *connect = arg_str0("c", "connect", "<write/read>", "Connect with uart");
    struct arg_end *end = arg_end(20);

    void* argtable[] = { help, driver, init_config, set, id, baud_rate, bits, threshold, parity, stop_bits, flow_ctrl, src_clk, used, able, connect, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_uart_api_cmd_handler);
}

#define UART_CMD_CNT (sizeof(s_uart_api_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_uart_api_commands[] = {
    {"arguart", "arguart { driver | init_config | set | id | baud_rate | bits | threshold | parity | stop_bits | flow_ctrl | src_clk | used | enable or disable | connect}", cli_arguart_cmd},
};

int cli_uart_api_init(void)
{
	BK_LOG_ON_ERR(bk_uart_driver_init());
	return cli_register_commands(s_uart_api_commands, UART_CMD_CNT);
}