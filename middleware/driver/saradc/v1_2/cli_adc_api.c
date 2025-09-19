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
#include "bk_saradc.h"
#include <driver/adc.h>
#include "adc_statis.h"
#include <os/str.h>
#include "sys_driver.h"
#include <driver/flash_partition.h>
#include "adc_driver.h"
#include "cli_section.h"

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

#define ARG_ERR_REC_CNT           20
#define CONFIG_STRUCT_FIELD_CNT   10

static void cli_adc_api_driver_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }
    else if (driver->count > 0)
    {
        if (strcmp(driver->sval[0], "init") == 0) {
            BK_LOG_ON_ERR(bk_adc_driver_init());
            BK_DUMP_OUT("ADC driver initialized successfully\r\n");
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_adc_driver_deinit());
            BK_DUMP_OUT("ADC driver deinitialized successfully\r\n");
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

static void cli_argadc_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit>", "Init or deinit adc driver");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, driver, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_adc_api_driver_handler);
}

static void cli_adc_api_channel_handler(void **argtable)
{
    adc_config_t adc_config = {0};
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *channel = (struct arg_str *)argtable[1];
    struct arg_int *arg_config_detail = (struct arg_int *)argtable[2];
    struct arg_str *id = (struct arg_str *)argtable[3];
    struct arg_int *data = (struct arg_int *)argtable[4];
    struct arg_str *timeout = (struct arg_str *)argtable[5];
    struct arg_str *cnt = (struct arg_str *)argtable[6];



    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    if (channel->count > 0)
    {
        if (strcmp(channel->sval[0], "init") == 0)
        {
            if (arg_config_detail->count >= CONFIG_STRUCT_FIELD_CNT)
            {
                adc_config.clk = (arg_config_detail->ival[0]);
                adc_config.sample_rate = (arg_config_detail->ival[1]);
                adc_config.adc_filter = (arg_config_detail->ival[2]);
                adc_config.steady_ctrl = (arg_config_detail->ival[3]);
                adc_config.is_hw_using_cali_result = (arg_config_detail->ival[4]);
                adc_config.adc_mode = (arg_config_detail->ival[5]);
                adc_config.src_clk = (arg_config_detail->ival[6]);
                adc_config.chan = (arg_config_detail->ival[7]);
                adc_config.saturate_mode = (arg_config_detail->ival[8]);
                adc_config.vol_div = (arg_config_detail->ival[9]);

                BK_DUMP_OUT("adc_config.clk:0x%x\n", adc_config.clk);
                BK_DUMP_OUT("adc_config.sample_interval:0x%x\n", adc_config.sample_rate);
                BK_DUMP_OUT("adc_config.adc_filter:0x%x\n", adc_config.adc_filter);
                BK_DUMP_OUT("adc_config.steady_ctrl:0x%x\n", adc_config.steady_ctrl);
                BK_DUMP_OUT("adc_config.is_hw_using_cali_result:0x%x\n", adc_config.is_hw_using_cali_result);
                BK_DUMP_OUT("adc_config.adc_mode:0x%x\n", adc_config.adc_mode);
                BK_DUMP_OUT("adc_config.src_clk:0x%x\n", adc_config.src_clk);
                BK_DUMP_OUT("adc_config.chan:0x%x\n", adc_config.chan);
                BK_DUMP_OUT("adc_config.saturate_mode:0x%x\n", adc_config.saturate_mode);
                BK_DUMP_OUT("adc_config.vol_div:0x%x\n", adc_config.vol_div);

                BK_LOG_ON_ERR(bk_adc_channel_init(&adc_config));
                BK_DUMP_OUT("ADC channel init succeeded\n");
            } else {
                BK_DUMP_OUT("Not enough configuration parameters provided for channel init.\n");
            }
        }
        else if (strcmp(channel->sval[0], "deinit") == 0)
        {
            adc_chan_t channel_id = (adc_chan_t)os_strtoul(id->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_adc_channel_deinit(channel_id));
            BK_DUMP_OUT("ADC channel %d deinitialized successfully\r\n",channel_id);
        }
        else if (strcmp(channel->sval[0], "read") == 0)
        {
            // uint16_t *channel_data;
            adc_chan_t channel_id = (adc_chan_t)os_strtoul(id->sval[0], NULL, 10);
            uint16_t value = (uint16_t)data->ival[0];
            uint32_t channel_timeout = (uint32_t)os_strtoul(timeout->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_adc_channel_read(channel_id, &value, channel_timeout));
            BK_DUMP_OUT("ADC channel %d read over\r\n",channel_id);
        }
        else if (strcmp(channel->sval[0], "raw_read") == 0)
        {
            uint16_t *channel_data;
            adc_chan_t channel_id = (adc_chan_t)os_strtoul(id->sval[0], NULL, 10);
            channel_data = (uint16_t *)data->ival[0];
            uint32_t channel_timeout = (uint32_t)os_strtoul(timeout->sval[0], NULL, 10);
            uint32_t sample_cnt = (uint32_t)os_strtoul(cnt->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_adc_channel_raw_read(channel_id, channel_data, channel_timeout, sample_cnt));

            BK_DUMP_OUT("ADC channel %d raw read succeed\r\n",channel_id);
        }
        else
        {
            BK_DUMP_OUT("Invalid parameter for channel: %s\r\n", channel->sval[0]);
        }
    }
    else
    {
        print_help(argtable[0], argtable);
    }
}

static void cli_argadc_channel_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *channel = arg_str0("c", "channel", "<init/deinit/read/raw_read>", "Init or deinit adc channel and read value");
    struct arg_int *arg_config_detail = arg_intn(NULL, NULL, "<clk/sample_rate/adc_filter/steady_ctrl/is_using_cali/adc_mode/src_clk/channel_id/saturate_mode/vol_div>", 0, CONFIG_STRUCT_FIELD_CNT, "Configure the specified channel");
    struct arg_str *id = arg_str0("i", "id", "<id>", "Channel ID");
    struct arg_int *data = arg_int0("d", "data", "<data>", "Store the average value of all sample values in ADC software buffers");
    struct arg_str *timeout = arg_str0("t", "timeout", "<timeout>", "ADC read semaphore timeout");
    struct arg_str *cnt = arg_str0("n", "sample_cnt", "<sample_cnt>", "ADC read raw data sample_cnt");

    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, channel, arg_config_detail, id, data, timeout, cnt, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_adc_api_channel_handler);
}

static void cli_adc_register_cb(uint32_t param)
{
    CLI_LOGI("param:%d , adc isr cb \r\n", param);
}

static void cli_adc_api_register_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *reg = (struct arg_str *)argtable[1];
    struct arg_str *size = (struct arg_str *)argtable[2];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (reg->count > 0)
    {
        if (strcmp(reg->sval[0], "register") == 0) {
            uint32_t reg_size = (uint32_t)os_strtoul(size->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_adc_register_isr_callback(cli_adc_register_cb, reg_size));
            BK_DUMP_OUT("ADC register isr callback over\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for register: %s\r\n", reg->sval[0]);
        }
        return;
    }

    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argadc_register_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *reg = arg_str0("r", "reg", "register", "Register the adc interrupt service routine");
    struct arg_str *size = arg_str0("s", "size", "<size>", "Register the adc interrupt service size");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, reg, size, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_adc_api_register_handler);
}

static void cli_adc_api_calculate_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *cal = (struct arg_str *)argtable[1];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (cal->count > 0)
    {
        if (strcmp(cal->sval[0], "data") == 0) {
            uint16_t value   = 0;
            float cali_value = 0;
            UINT8 adc_chan_buff[12] = {0,1,2,3,4,5,6,10,12,13,14,15};
            adc_config_t config = {0};
            config.chan = 0;
            config.adc_mode = 3;
            config.src_clk = 1;
            config.clk = 0x30e035;
            config.saturate_mode = 4;
            config.steady_ctrl= 7;
            config.adc_filter = 0;
            if(config.adc_mode == ADC_CONTINUOUS_MODE) {
                config.sample_rate = 0;
            }
            for(UINT8 i = 0; i < (sizeof(adc_chan_buff)/sizeof(UINT8)); i++)
            {
                config.chan = adc_chan_buff[i];

                BK_LOG_ON_ERR(bk_adc_channel_init(&config));
                BK_LOG_ON_ERR(bk_adc_channel_read(config.chan, &value, ADC_READ_SEMAPHORE_WAIT_TIME));

                cali_value = bk_adc_data_calculate(value, config.chan);
                BK_DUMP_OUT("volt:%d mv,chan=%d\n",(uint32_t)(cali_value*1000),config.chan);
                bk_adc_channel_deinit(config.chan);
            }
            BK_DUMP_OUT("ADC calculate voltage read successfully\r\n");
        } else {
            BK_DUMP_OUT("Invalid parameter for cal: %s\r\n", cal->sval[0]);
        }
        return;
    }

    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argadc_calculate_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *cal = arg_str0("c", "calculate", "<data>", "ADC calculate voltage");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, cal, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_adc_api_calculate_handler);
}

#define ADC_API_CMD_CNT (sizeof(s_adc_api_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_adc_api_commands[] = {
    {"argadc_driver", "argadc_driver { help | driver }", cli_argadc_driver_cmd},
    {"argadc_channel", "argadc_channel { help | channel }", cli_argadc_channel_cmd},
    {"argadc_register", "argadc_register { help | register }", cli_argadc_register_cmd},
    {"argadc_cal", "argadc_cal { help | data_calculate }", cli_argadc_calculate_cmd},

};

int cli_adc_api_init(void)
{
    BK_LOG_ON_ERR(bk_adc_driver_init());
    return cli_register_commands(s_adc_api_commands, ADC_API_CMD_CNT);
}
