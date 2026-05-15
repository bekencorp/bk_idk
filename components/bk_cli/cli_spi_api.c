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
#include <driver/spi.h>
#include <driver/trng.h>
#include "spi_statis.h"
#include <driver/spi_types.h>
#include <common/bk_include.h>
#include <driver/dma.h>
#include <string.h>

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

static void cli_spi_rx_isr(spi_id_t id, void *param)
{
	BK_DUMP_OUT("spi_rx_isr(%d)\n", id);
}

static void cli_spi_tx_isr(spi_id_t id, void *param)
{
	BK_DUMP_OUT("spi_tx_isr(%d)\n", id);
}

static void cli_spi_api_cmd_handler(void **argtable)
{
    bk_err_t ret = BK_OK;
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];
    struct arg_str *init_config = (struct arg_str *)argtable[2];
    struct arg_str *bit_order = (struct arg_str *)argtable[3];
    struct arg_str *id = (struct arg_str *)argtable[4];
    struct arg_int *baud_rate = (struct arg_int *)argtable[5];
    struct arg_str *bits = (struct arg_str *)argtable[6];
    struct arg_str *polarity = (struct arg_str *)argtable[7];
    struct arg_str *phase = (struct arg_str *)argtable[8];
    struct arg_str *mode = (struct arg_str *)argtable[9];
    struct arg_str *role = (struct arg_str *)argtable[10];
    struct arg_str *set = (struct arg_str *)argtable[11];
    struct arg_str *spi_register = (struct arg_str *)argtable[12];
    struct arg_str *connect = (struct arg_str *)argtable[13];


    if (help->count > 0)
    {
        print_help("argspi", argtable);
        return;
    }

    else if (driver->count > 0)
    {
        if (strcmp(driver->sval[0], "init") == 0) {
            ret = bk_spi_driver_init();
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI driver init failed, ret:-0x%x\r\n", -ret);
                return;
            }
            BK_DUMP_OUT("SPI driver initialized successfully\r\n");
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            ret = bk_spi_driver_deinit();
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI driver deinit failed, ret:-0x%x\r\n", -ret);
                return;
            }
            BK_DUMP_OUT("SPI driver deinitialized successfully\r\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for driver: %s\r\n", driver->sval[0]);
        }
        return;
    }

    else if (init_config->count > 0)
    {
        uint32_t spi_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(init_config->sval[0], "init") == 0) {
            spi_config_t config = {0};
            os_memset(&config, 0, sizeof(spi_config_t));

            if (role->count > 0) {
                if (strcmp(role->sval[0], "master") == 0) {
                    config.role = SPI_ROLE_MASTER;
                } else if (strcmp(role->sval[0], "slave") == 0) {
                    config.role = SPI_ROLE_SLAVE;
                } else {
                    BK_DUMP_OUT("Invalid role: %s\r\n", role->sval[0]);
                    return;
                }
                BK_DUMP_OUT("Role: %s\r\n", role->sval[0]);
            }
            if (bits->count > 0) {
                if (os_strtoul(bits->sval[0], NULL, 10) == 8){
                    config.bit_width = SPI_BIT_WIDTH_8BITS;
                } else {
                    config.bit_width = SPI_BIT_WIDTH_16BITS;
                }
                BK_DUMP_OUT("Bit Width: %d\r\n", config.bit_width);
            }
            if (polarity->count > 0) {
                if (os_strtoul(polarity->sval[0], NULL, 10) == 0){
                    config.polarity = SPI_POLARITY_LOW;
                } else {
                    config.polarity = SPI_POLARITY_HIGH;
                }
                BK_DUMP_OUT("Polarity: %d\r\n", config.polarity);
            }
            if (phase->count > 0) {
                if (os_strtoul(phase->sval[0], NULL, 10) == 0){
                    config.phase = SPI_PHASE_1ST_EDGE;
                } else {
                    config.phase = SPI_PHASE_2ND_EDGE;
                }
            }
            if (mode->count > 0) {
                uint32_t wire_mode_raw = os_strtoul(mode->sval[0], NULL, 10);
                if (wire_mode_raw == 3){
                    config.wire_mode = SPI_3WIRE_MODE;
                } else {
                    config.wire_mode = SPI_4WIRE_MODE;
                }
                BK_DUMP_OUT("Wire Mode: %d\r\n", wire_mode_raw);
            }

            if (baud_rate->count > 0) {
                if (baud_rate->ival[0] < 0) {
                    BK_DUMP_OUT("baud_rate(%d) not support\r\n", baud_rate->ival[0]);
                    return;
                }
                config.baud_rate = baud_rate->ival[0];
                BK_DUMP_OUT("Baud Rate: %d\r\n", baud_rate->ival[0]);
            }

            if (bit_order->count > 0) {
                if (strcmp(bit_order->sval[0], "MSB") == 0) {
                    config.bit_order = SPI_MSB_FIRST;
                } else if (strcmp(bit_order->sval[0], "LSB") == 0) {
                    config.bit_order = SPI_LSB_FIRST;
                } else {
                    BK_DUMP_OUT("Invalid bit order: %s\r\n", bit_order->sval[0]);
                    return;
                }
                BK_DUMP_OUT("Bit Order: %s\r\n", bit_order->sval[0]);
            }

        #if (CONFIG_SPI_BYTE_INTERVAL)
            config.byte_interval = 1;
        #endif

        #if (CONFIG_SPI_DMA)
            config.dma_mode = 0;   /**< SPI whether use dma */
            config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);;  /**< SPI tx dma channel */
            config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);  /**< SPI rx dma channel */
            if (os_strtoul(bits->sval[0], NULL, 10) == 8) {
                config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
                config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
            } else {
                config.spi_tx_dma_width = DMA_DATA_WIDTH_16BITS;
                config.spi_rx_dma_width = DMA_DATA_WIDTH_16BITS;
            }
        #endif
            ret = bk_spi_init(spi_id, &config);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI init failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI initialized, spi_id=%d\n", spi_id);
        }

         else if (strcmp(init_config->sval[0], "deinit") == 0){
            ret = bk_spi_deinit(spi_id);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI deinit failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("spi deinit, spi_id=%d\n", spi_id);
        } else {
            BK_DUMP_OUT("Invalid configuration parameter: %s\n", init_config->sval[0]);

        }
        return;
    }

    else if (set->count > 0) {
        uint32_t spi_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);

        if (strcmp(set->sval[0], "mode") == 0) {
            if (mode->count > 0) {
                uint32_t mode_value = os_strtoul(mode->sval[0], NULL, 10);
                spi_mode_t spi_mode;

                if (mode_value == 0) {
                    spi_mode = SPI_POL_MODE_0;
                } else if (mode_value == 1) {
                    spi_mode = SPI_POL_MODE_1;
                } else if (mode_value == 2) {
                    spi_mode = SPI_POL_MODE_2;
                } else if (mode_value == 3) {
                    spi_mode = SPI_POL_MODE_3;
                } else {
                    BK_DUMP_OUT("SPI mode value must be between 0 and 3\n");
                    return;
                }

                ret = bk_spi_set_mode(spi_id, spi_mode);
                if (ret != BK_OK) {
                    BK_DUMP_OUT("SPI %d set mode failed, ret:-0x%x\n", spi_id, -ret);
                    return;
                }
                BK_DUMP_OUT("SPI %d set mode succeed, mode=%d\n", spi_id, spi_mode);
            }
        }
        else if (strcmp(set->sval[0], "bit_width") == 0) {
            if (bits->count > 0) {
                uint32_t bits_value = os_strtoul(bits->sval[0], NULL, 10);
                spi_bit_width_t bit_width;

                if (bits_value == 0) {
                    bit_width = SPI_BIT_WIDTH_8BITS;
                } else if (bits_value == 1) {
                    bit_width = SPI_BIT_WIDTH_16BITS;
                } else {
                    BK_DUMP_OUT("SPI bit width value must be between 0 and 1\n");
                    return;
                }

                ret = bk_spi_set_bit_width(spi_id, bit_width);
                if (ret != BK_OK) {
                    BK_DUMP_OUT("SPI %d set bit width failed, ret:-0x%x\n", spi_id, -ret);
                    return;
                }
                BK_DUMP_OUT("SPI %d set bit width succeed, bit_width=%d\n", spi_id, bit_width);
            }
        }
        else if (strcmp(set->sval[0], "wire_mode") == 0) {
            if (mode->count > 0) {
                uint32_t mode_value = os_strtoul(mode->sval[0], NULL, 10);
                spi_mode_t spi_mode;

                if (mode_value == 0) {
                    spi_mode = SPI_4WIRE_MODE;
                } else if (mode_value == 1) {
                    spi_mode = SPI_3WIRE_MODE;
                } else {
                    BK_DUMP_OUT("SPI bit width value must be between 0 and 1\n");
                    return;
                }

                ret = bk_spi_set_wire_mode(spi_id, spi_mode);
                if (ret != BK_OK) {
                    BK_DUMP_OUT("SPI %d set wire mode failed, ret:-0x%x\n", spi_id, -ret);
                    return;
                }
                BK_DUMP_OUT("SPI %d set wire mode succeed, spi_wire_mode=%d\n", spi_id, spi_mode);
            }
        }
        else if (strcmp(set->sval[0], "baud") == 0) {
            int baud_rate_raw = baud_rate->ival[0];
            if (baud_rate_raw < 0) {
                BK_DUMP_OUT("baud_rate(%d) not support\r\n", baud_rate_raw);
                return;
            }
            uint32_t baud_rate_val = (uint32_t)baud_rate_raw;
            ret = bk_spi_set_baud_rate(spi_id, baud_rate_val);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI id %d set baud_rate failed, ret:-0x%x,\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI id %d set baud_rate %d successfully,\n", spi_id, baud_rate_val);
        }
        else if (strcmp(set->sval[0], "bit_order") == 0) {
            if (bit_order->count > 0) {
                uint32_t bits_value = os_strtoul(bit_order->sval[0], NULL, 10);
                spi_bit_order_t bit_order;

                if (bits_value == 0) {
                    bit_order = SPI_MSB_FIRST;
                } else if (bits_value == 1) {
                    bit_order = SPI_LSB_FIRST;
                } else {
                    BK_DUMP_OUT("SPI bit order value must be between 0 and 1\n");
                    return;
                }

                ret = bk_spi_set_bit_order(spi_id, bit_order);
                if (ret != BK_OK) {
                    BK_DUMP_OUT("SPI %d set bit order failed, ret:-0x%x\n", spi_id, -ret);
                    return;
                }
                BK_DUMP_OUT("SPI %d set bit order succeed, bit_order=%d\n", spi_id, bit_order);
            }
        }
        else {
            BK_DUMP_OUT("Set parameter is missing\n");
        }
    }

    else if (spi_register->count > 0) {
        uint32_t spi_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);

        if (strcmp(spi_register->sval[0], "rx_isr") == 0) {
            ret = bk_spi_register_rx_isr(spi_id, cli_spi_rx_isr, NULL);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI ID: %d register rx isr failed, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI ID: %d Registered RX interrupt ISR\n", spi_id);
        }
        else if (strcmp(spi_register->sval[0], "tx_finish") == 0) {
            ret = bk_spi_register_tx_finish_isr(spi_id, cli_spi_tx_isr, NULL);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI ID: %d register tx finish isr failed, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI ID: %d Registered TX finish interrupt ISR\n", spi_id);
        }
        else if (strcmp(spi_register->sval[0], "rx_finish") == 0) {
            ret = bk_spi_register_rx_finish_isr(spi_id, cli_spi_rx_isr, NULL);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI ID: %d register rx finish isr failed, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI ID: %d Registered RX finish interrupt ISR\n", spi_id);
        }
        else if (strcmp(spi_register->sval[0], "unrx_isr") == 0) {
            ret = bk_spi_unregister_rx_isr(spi_id);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI ID: %d unregister rx isr failed, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI ID: %d Unregister the RX finish interrupt service routine\n", spi_id);
        }
        else if (strcmp(spi_register->sval[0], "unrx_finish") == 0) {
            ret = bk_spi_unregister_rx_finish_isr(spi_id);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI ID: %d unregister rx finish isr failed, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI ID: %d Unregister the TX finish interrupt service routine\n", spi_id);
        }
        else if (strcmp(spi_register->sval[0], "untx_finish") == 0) {
            ret = bk_spi_unregister_tx_finish_isr(spi_id);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI ID: %d unregister tx finish isr failed, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("SPI ID: %d Send data to the SPI port from a given buffer and length in async mode\n", spi_id);
        }
        else {
            BK_DUMP_OUT("Invalid parameter for spi_register: %s\n", spi_register->sval[0]);
            return;
        }
    }
    else if (connect->count > 0) {
        uint32_t spi_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(connect->sval[0], "write") == 0) {
            uint32_t buf_len = (uint32_t)os_strtoul(bits->sval[0], NULL, 10);
		    uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
            if (send_data == NULL) {
                BK_DUMP_OUT("send buffer malloc failed\r\n");
                return;
            }
            for (int i = 0; i < buf_len; i++) {
                send_data[i] = i & 0xff;
            }
            ret = bk_spi_write_bytes(spi_id, send_data, buf_len);
            if (send_data) {
                os_free(send_data);
            }
            send_data = NULL;
            if (ret != BK_OK) {
                BK_DUMP_OUT("spi write bytes failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("spi write bytes, spi_id=%d, data_len=%d\n", spi_id, buf_len);
        } else if (strcmp(connect->sval[0], "read") == 0) {
            uint32_t buf_len = (uint32_t)os_strtoul(bits->sval[0], NULL, 10);
            uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
            if (recv_data == NULL) {
                BK_DUMP_OUT("recv buffer malloc failed\r\n");
                return;
            }
            os_memset(recv_data, 0xff, buf_len);
            ret = bk_spi_read_bytes(spi_id, recv_data, buf_len);
            if (ret != BK_OK) {
                BK_DUMP_OUT("spi read failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                if (recv_data) {
                    os_free(recv_data);
                }
                recv_data = NULL;
                return;
            }
            BK_DUMP_OUT("spi read, spi_id=%d, size:%d\n", spi_id, buf_len);
            for (int i = 0; i < buf_len; i++) {
                #if CONFIG_TASK_WDT
                    extern void bk_task_wdt_feed(void);
                    bk_task_wdt_feed();
                #endif
                BK_DUMP_OUT("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
            }
            if (recv_data) {
                os_free(recv_data);
            }
            recv_data = NULL;
        } else if (strcmp(connect->sval[0], "write_async") == 0) {
            uint32_t buf_len = (uint32_t)os_strtoul(bits->sval[0], NULL, 10);
		    uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
            if (send_data == NULL) {
                BK_DUMP_OUT("send buffer malloc async failed\r\n");
                return;
            }
            for (int i = 0; i < buf_len; i++) {
                send_data[i] = i & 0xff;
            }
            ret = bk_spi_write_bytes_async(spi_id, send_data, buf_len);
            if (send_data) {
                os_free(send_data);
            }
            send_data = NULL;
            if (ret != BK_OK) {
                BK_DUMP_OUT("spi write bytes async failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            BK_DUMP_OUT("spi write bytes async, spi_id=%d, data_len=%d\n", spi_id, buf_len);
        } else if (strcmp(connect->sval[0], "read_async") == 0) {
            uint32_t buf_len = (uint32_t)os_strtoul(bits->sval[0], NULL, 10);
            uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
            if (recv_data == NULL) {
                BK_DUMP_OUT("recv buffer malloc async failed\r\n");
                return;
            }
            os_memset(recv_data, 0xff, buf_len);
            ret = bk_spi_read_bytes_async(spi_id, recv_data, buf_len);
            if (ret != BK_OK) {
                BK_DUMP_OUT("spi read async failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                if (recv_data) {
                    os_free(recv_data);
                }
                recv_data = NULL;
                return;
            }
            BK_DUMP_OUT("spi read async, spi_id=%d, size:%d\n", spi_id, buf_len);
            for (int i = 0; i < buf_len; i++) {
                #if CONFIG_TASK_WDT
                    extern void bk_task_wdt_feed(void);
                    bk_task_wdt_feed();
                #endif
                BK_DUMP_OUT("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
            }
            if (recv_data) {
                os_free(recv_data);
            }
            recv_data = NULL;
        } else if (strcmp(connect->sval[0], "transmit") == 0) {
            uint32_t send_len = (uint32_t)os_strtoul(bits->sval[0], NULL, 10);
            uint32_t recv_len = (uint32_t)os_strtoul(bit_order->sval[0], NULL, 10);
            uint8_t *send_data = (uint8_t *)os_zalloc(send_len);
            uint8_t *recv_data = (uint8_t *)os_malloc(recv_len);
            if (send_data == NULL) {
                BK_DUMP_OUT("send buffer malloc failed\r\n");
                return;
            }
            for (int i = 0; i < send_len; i++) {
                send_data[i] = i & 0xff;
            }
            if (recv_data == NULL) {
                BK_DUMP_OUT("recv buffer malloc failed\r\n");
                return;
            }
            os_memset(recv_data, 0xff, recv_len);
            int ret = bk_spi_transmit(spi_id, send_data, send_len, recv_data, recv_len);
            if (ret < 0) {
                BK_DUMP_OUT("spi transmit failed, ret:-0x%x\r\n", -ret);
                goto transmit_exit;
            }
            for (int i = 0; i < recv_len; i++) {
                #if CONFIG_TASK_WDT
                    extern void bk_task_wdt_feed(void);
                    bk_task_wdt_feed();
                #endif
                BK_DUMP_OUT("recv_buffer[%d]=0x%x\r\n", i, recv_data[i]);
            }
    transmit_exit:
            if (send_data) {
                os_free(send_data);
            }
            send_data = NULL;

            if (recv_data) {
                os_free(recv_data);
            }
            recv_data = NULL;
        }

    }


    else
    {
        print_help(argtable[0], argtable);
    }
}

static void cli_argspi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit>", "Init or deinit spi driver");
    struct arg_str *init_config = arg_str0("a", "init_config", "<init/deinit>", "Init or deinit spi config");
    struct arg_str *bit_order = arg_str0("o", "bit_order", "<MSB/LSB>", "Spi config bit_order");
    struct arg_str *id = arg_str0("i", "id", "<id>", "SPI ID");
    struct arg_int *baud_rate = arg_int0("b", "baud_rate", "<baud_rate>", "Spi baud rate");
    struct arg_str *bits = arg_str0("t", "bits", "<8/16>", "SPI bits");
    struct arg_str *polarity = arg_str0("p", "polarity", "<0/1>", "SPI polarity config polarity low/high");
    struct arg_str *phase = arg_str0("s", "phase", "<0/1>", "SPI phase config");
    struct arg_str *mode = arg_str0("m", "mode", "<3/4>", "SPI wire_mode");
    struct arg_str *role = arg_str0("l", "spi_role", "<slave/master>", "Spi config master or slave");
    struct arg_str *set = arg_str0("e", "set", "<mode/bit_width/wire_mode/baud_rate/bit_order>", "SPI set config");
    struct arg_str *spi_register = arg_str0("r", "register", "<rx_isr/tx_isr/rx_finish/unrx_isr/untx_isr/unrx_finish>", "SPI set config");
    struct arg_str *connect = arg_str0("c", "connect", "<read/write/read_async/write_async/transmit>", "Connect with spi");

    struct arg_end *end = arg_end(20);

    void* argtable[] = { help, driver, init_config, bit_order, id, baud_rate, bits, polarity, phase, mode, role, set, spi_register, connect, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_spi_api_cmd_handler);
}

#if CONFIG_SPI_DMA

#define CONFIG_STRUCT_FIELD_CNT   8
#define ARG_ERR_REC_CNT           20
static dma_id_t s_cli_spi_dma_tx_chan[SPI_ID_MAX];
static dma_id_t s_cli_spi_dma_rx_chan[SPI_ID_MAX];
static bool s_cli_spi_dma_chan_valid[SPI_ID_MAX];

static void cli_spi_api_dma_handler(void **argtable)
{
    bk_err_t ret = BK_OK;
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];
    struct arg_str *bit_order = (struct arg_str *)argtable[2];
    struct arg_str *id = (struct arg_str *)argtable[3];
    struct arg_int *baud_rate = (struct arg_int *)argtable[4];
    struct arg_str *bits = (struct arg_str *)argtable[5];
    struct arg_str *polarity = (struct arg_str *)argtable[6];
    struct arg_str *phase = (struct arg_str *)argtable[7];
    struct arg_str *mode = (struct arg_str *)argtable[8];
    struct arg_str *role = (struct arg_str *)argtable[9];
    struct arg_str *length = (struct arg_str *)argtable[10];


    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (driver->count > 0)
    {
        uint32_t spi_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
        if (spi_id >= SPI_ID_MAX) {
            BK_DUMP_OUT("Invalid spi id: %d\r\n", spi_id);
            return;
        }
        if (strcmp(driver->sval[0], "init") == 0) {
            spi_config_t config = {0};
            os_memset(&config, 0, sizeof(spi_config_t));
            config.dma_mode = 1;
            config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
            config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
            if (s_cli_spi_dma_chan_valid[spi_id]) {
                bk_dma_free(DMA_DEV_GSPI0, s_cli_spi_dma_tx_chan[spi_id]);
                bk_dma_free(DMA_DEV_GSPI0_RX, s_cli_spi_dma_rx_chan[spi_id]);
                s_cli_spi_dma_chan_valid[spi_id] = false;
            }

            if (role->count > 0) {
                if (strcmp(role->sval[0], "master") == 0) {
                    config.role = SPI_ROLE_MASTER;
                } else if (strcmp(role->sval[0], "slave") == 0) {
                    config.role = SPI_ROLE_SLAVE;
                } else {
                    BK_DUMP_OUT("Invalid role: %s\r\n", role->sval[0]);
                    return;
                }
                BK_DUMP_OUT("Role: %s\r\n", role->sval[0]);
            }
            if (bits->count > 0) {
                if (os_strtoul(bits->sval[0], NULL, 10) == 8){
                    config.bit_width = SPI_BIT_WIDTH_8BITS;
                } else {
                    config.bit_width = SPI_BIT_WIDTH_16BITS;
                }
                BK_DUMP_OUT("Bit Width: %d\r\n", config.bit_width);
            }
            if (polarity->count > 0) {
                if (os_strtoul(polarity->sval[0], NULL, 10) == 0){
                    config.polarity = SPI_POLARITY_LOW;
                } else {
                    config.polarity = SPI_POLARITY_HIGH;
                }
                BK_DUMP_OUT("Polarity: %d\r\n", config.polarity);
            }
            if (phase->count > 0) {
                if (os_strtoul(phase->sval[0], NULL, 10) == 0){
                    config.phase = SPI_PHASE_1ST_EDGE;
                } else {
                    config.phase = SPI_PHASE_2ND_EDGE;
                }
            }
            if (mode->count > 0) {
                if (os_strtoul(mode->sval[0], NULL, 10) == 3){
                    config.wire_mode = SPI_3WIRE_MODE;
                } else {
                    config.wire_mode = SPI_4WIRE_MODE;
                }
                BK_DUMP_OUT("Wire Mode: %d\r\n", config.wire_mode);
            }

            if (baud_rate->count > 0) {
                config.baud_rate = baud_rate->ival[0];
                BK_DUMP_OUT("Baud Rate: %d\r\n", baud_rate->ival[0]);
            }

            if (bit_order->count > 0) {
                if (strcmp(bit_order->sval[0], "MSB") == 0) {
                    config.bit_order = SPI_MSB_FIRST;
                } else if (strcmp(bit_order->sval[0], "LSB") == 0) {
                    config.bit_order = SPI_LSB_FIRST;
                } else {
                    BK_DUMP_OUT("Invalid bit order: %s\r\n", bit_order->sval[0]);
                    return;
                }
                BK_DUMP_OUT("Bit Order: %s\r\n", bit_order->sval[0]);
            }
        #if (CONFIG_SPI_BYTE_INTERVAL)
            config.byte_interval = 1;
        #endif

            config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);
            if (config.spi_tx_dma_chan >= DMA_ID_MAX) {
                BK_DUMP_OUT("SPI dma tx channel alloc failed\r\n");
                return;
            }
            config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);
            if (config.spi_rx_dma_chan >= DMA_ID_MAX) {
                BK_DUMP_OUT("SPI dma rx channel alloc failed\r\n");
                bk_dma_free(DMA_DEV_GSPI0, config.spi_tx_dma_chan);
                return;
            }
            if (os_strtoul(bits->sval[0], NULL, 10) == 8) {
                config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
                config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
            } else {
                config.spi_tx_dma_width = DMA_DATA_WIDTH_16BITS;
                config.spi_rx_dma_width = DMA_DATA_WIDTH_16BITS;
            }
            ret = bk_spi_init(spi_id, &config);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI dma init failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                bk_dma_free(DMA_DEV_GSPI0, config.spi_tx_dma_chan);
                bk_dma_free(DMA_DEV_GSPI0_RX, config.spi_rx_dma_chan);
                return;
            }
            s_cli_spi_dma_tx_chan[spi_id] = config.spi_tx_dma_chan;
            s_cli_spi_dma_rx_chan[spi_id] = config.spi_rx_dma_chan;
            s_cli_spi_dma_chan_valid[spi_id] = true;
            BK_DUMP_OUT("SPI initialized, spi_id=%d\n", spi_id);
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            ret = bk_spi_deinit(spi_id);
            if (ret != BK_OK) {
                BK_DUMP_OUT("SPI dma deinit failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                return;
            }
            if (s_cli_spi_dma_chan_valid[spi_id]) {
                bk_dma_free(DMA_DEV_GSPI0, s_cli_spi_dma_tx_chan[spi_id]);
                bk_dma_free(DMA_DEV_GSPI0_RX, s_cli_spi_dma_rx_chan[spi_id]);
                s_cli_spi_dma_chan_valid[spi_id] = false;
            }
            BK_DUMP_OUT("spi deinit, spi_id=%d\n", spi_id);
        } else if (strcmp(driver->sval[0], "write") == 0) {
            if (!s_cli_spi_dma_chan_valid[spi_id]) {
                BK_DUMP_OUT("SPI dma not initialized, run init first, spi_id=%d\n", spi_id);
                return;
            }
            uint32_t buf_len = (uint32_t)os_strtoul(length->sval[0], NULL, 10);
            uint8_t *send_data = (uint8_t *)os_zalloc(buf_len);
            if (send_data == NULL) {
                BK_DUMP_OUT("send buffer malloc failed\r\n");
                return;
            }
            for (int i = 0; i < buf_len; i++) {
                send_data[i] = i & 0xff;
            }
            uint32_t write_cnt = bk_spi_dma_write_bytes(spi_id, send_data, buf_len);
            if (send_data) {
                os_free(send_data);
            }
            send_data = NULL;
            if (write_cnt != buf_len) {
                BK_DUMP_OUT("spi dma send failed, spi_id=%d, write_cnt=%d, expect=%d\n", spi_id, write_cnt, buf_len);
                return;
            }
            BK_DUMP_OUT("spi dma send, spi_id=%d, data_len=%d\n", spi_id, buf_len);
        } else if (strcmp(driver->sval[0], "read") == 0) {
            if (!s_cli_spi_dma_chan_valid[spi_id]) {
                BK_DUMP_OUT("SPI dma not initialized, run init first, spi_id=%d\n", spi_id);
                return;
            }
            uint32_t buf_len = (uint32_t)os_strtoul(length->sval[0], NULL, 10);
            uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
            if (recv_data == NULL) {
                BK_DUMP_OUT("recv buffer malloc failed\r\n");
                return;
            }
            os_memset(recv_data, 0xff, buf_len);
            uint32_t read_cnt = bk_spi_dma_read_bytes(spi_id, recv_data, buf_len);
            if (read_cnt != buf_len) {
                BK_DUMP_OUT("spi dma read failed, spi_id=%d, read_cnt=%d, expect=%d\n", spi_id, read_cnt, buf_len);
                if (recv_data) {
                    os_free(recv_data);
                }
                recv_data = NULL;
                return;
            }
            BK_DUMP_OUT("spi dma recv, spi_id=%d, data_len=%d\n", spi_id, buf_len);
            for (int i = 0; i < buf_len; i++) {
                #if CONFIG_TASK_WDT
                    extern void bk_task_wdt_feed(void);
                    bk_task_wdt_feed();
                #endif
                BK_DUMP_OUT("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
            }
            if (recv_data) {
                os_free(recv_data);
            }
            recv_data = NULL;
        } else if (strcmp(driver->sval[0], "duplex") == 0) {
            if (!s_cli_spi_dma_chan_valid[spi_id]) {
                BK_DUMP_OUT("SPI dma not initialized, run init first, spi_id=%d\n", spi_id);
                return;
            }
            uint32_t buf_len = (uint32_t)os_strtoul(length->sval[0], NULL, 10);
            uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
            uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
            os_memset(recv_data, 0xff, buf_len);
            for (int i = 0; i < buf_len; i++) {
                send_data[i] = i & 0xff;
            }
            bk_spi_dma_duplex_init(spi_id);
            ret = bk_spi_dma_duplex_xfer(spi_id, send_data, buf_len, recv_data, buf_len);
            if (ret != BK_OK) {
                BK_DUMP_OUT("spi dma duplex failed, spi_id=%d, ret:-0x%x\n", spi_id, -ret);
                bk_spi_dma_duplex_deinit(spi_id);
                if (recv_data) {
                    os_free(recv_data);
                }
                if (send_data) {
                    os_free(send_data);
                }
                return;
            }
            for (int i = 0; i < buf_len; i++) {
                #if CONFIG_TASK_WDT
                    extern void bk_task_wdt_feed(void);
                    bk_task_wdt_feed();
                #endif
                BK_DUMP_OUT("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
            }
            bk_spi_dma_duplex_deinit(spi_id);
            if (recv_data) {
                os_free(recv_data);
            }
            if (send_data) {
                os_free(send_data);
            }
        } else {
            BK_DUMP_OUT("Invalid parameter for init: %s\r\n", driver->sval[0]);
        }
        return;
    }

    else
    {
        print_help(argtable[0], argtable);

    }

}

static void cli_argspi_dma_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit/write/read/duplex>", "Spi dma function config");
    struct arg_str *bit_order = arg_str0("o", "bit_order", "<MSB/LSB>", "Spi config bit_order");
    struct arg_str *id = arg_str0("i", "id", "<id>", "SPI ID");
    struct arg_int *baud_rate = arg_int0("b", "baud_rate", "<baud_rate>", "Spi baud rate");
    struct arg_str *bits = arg_str0("t", "bits", "<8/16>", "SPI bits");
    struct arg_str *polarity = arg_str0("p", "polarity", "<0/1>", "SPI polarity config polarity low/high");
    struct arg_str *phase = arg_str0("s", "phase", "<0/1>", "SPI phase config");
    struct arg_str *mode = arg_str0("m", "mode", "<3/4>", "SPI wire_mode");
    struct arg_str *role = arg_str0("r", "spi_role", "<slave/master>", "Spi config master or slave");
    struct arg_str *length = arg_str0("l", "buff_length", "<length>", "SPI data buff length");

    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, driver, bit_order, id, baud_rate, bits, polarity, phase, mode, role, length, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_spi_api_dma_handler);
}
#endif

#define SPI_API_CMD_CNT (sizeof(s_spi_api_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_spi_api_commands[] = {
    {"argspi", "argspi { driver | init_config | bit_order | id | baud_rate | bits | polarity | phase | mode | role | set | spi_register | connect }", cli_argspi_cmd},
#if CONFIG_SPI_DMA
    {"argspi_dma", "argspi { help | init | write | read }", cli_argspi_dma_cmd},
#endif

};

int cli_spi_api_init(void)
{
	BK_LOG_ON_ERR(bk_spi_driver_init());
	return cli_register_commands(s_spi_api_commands, SPI_API_CMD_CNT);
}
// eof

