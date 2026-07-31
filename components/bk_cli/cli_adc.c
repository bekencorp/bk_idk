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

#include "cli.h"
#include "bk_saradc.h"
#include <driver/adc.h>
#include "adc_statis.h"
#include <os/os.h>
#include "sys_driver.h"

static UINT8 s_adc_working_flag = 0;

static void cli_adc_help(void)
{
    CLI_LOGI("adc_driver init/deinit\n");
    CLI_LOGI("adc_test [channel] [config/deinit/start/stop/get_value/read/dump_statis/single_step_mode_read/reg_cb/saradc_val_read/use_sample_set_saradc_val/single_adc_example/multi_adc_example]\n");
    CLI_LOGI("adc_test [channel] config [mode] [src_clk] [adc_clk] [saturate_mode] [sample_rate]\n");
    CLI_LOGI("adc_test [channel] read [size] [time_out]\n");
    CLI_LOGI("adc_test [channel] reg_cb [param]\n");
    CLI_LOGI("adc_test [channel] use_sample_set_saradc_val [mode] [src_clk] [adc_clk] [saturate_mode] [sample_rate] [low/high]\n");
    CLI_LOGI("adc_api_test [set_mode/get_mode/set_clk/set_sample_rate/set_channel/set_filter/\
		set_steady_time/set_sample_cnt/set_saturate\n");
}

static void cli_adc_register_cb(uint32_t param)
{
    CLI_LOGI("param:%d , adc isr cb \r\n", param);
}

static bk_err_t cli_adc_stop_and_deinit(uint32_t adc_chan)
{
    bk_err_t ret = BK_OK;
    bk_err_t cleanup_ret = bk_adc_stop();

    if (cleanup_ret != BK_OK) {
        CLI_LOGE("bk_adc_stop failed, ret:0x%x\r\n", cleanup_ret);
        ret = cleanup_ret;
    }

    cleanup_ret = bk_adc_deinit(adc_chan);
    if (cleanup_ret != BK_OK) {
        CLI_LOGE("bk_adc_deinit failed, chan=%d, ret:0x%x\r\n", adc_chan, cleanup_ret);
        if (ret == BK_OK) {
            ret = cleanup_ret;
        }
    }

    return ret;
}

static void cli_adc_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    if (argc < 2) {
        cli_adc_help();
        return;
    }

    if (os_strcmp(argv[1], "init") == 0) {
        ret = bk_adc_driver_init();
        if (ret == BK_OK) {
            CLI_LOGI("adc driver init\n");
        } else {
            CLI_LOGE("adc driver init failed, ret:%d\n", ret);
        }
    } else if (os_strcmp(argv[1], "deinit") == 0) {
        ret = bk_adc_driver_deinit();
        if (ret == BK_OK) {
            CLI_LOGI("adc driver deinit\n");
        } else {
            CLI_LOGE("adc driver deinit failed, ret:%d\n", ret);
        }
    } else {
        cli_adc_help();
        return;
    }
}

static float cli_adc_read_multi_chan(void)
{
    uint16_t value   = 0;
    float cali_value = 0;
    UINT8 adc_chan_buff[12] = {0,1,2,3,4,5,6,10,12,13,14,15};
    UINT32 loop_num = 10;
    adc_config_t config = {0};
    bk_err_t err = BK_OK;

    if(s_adc_working_flag == 1)
    {
        CLI_LOGI("adc_read is running\r\n");
        return 0;
    }

    config.chan = 0;
    config.adc_mode = 3;
    config.src_clk = 1;
    config.clk = 0x1E13D;
    config.saturate_mode = 4;
    config.steady_ctrl= 7;
    config.adc_filter = 0;
    if(config.adc_mode == ADC_CONTINUOUS_MODE) {
        config.sample_rate = 0;
    }

    s_adc_working_flag = 1;

    err = bk_adc_acquire();
    if (err != BK_OK) {
        CLI_LOGE("bk_adc_acquire failed, ret:0x%x\r\n", err);
        s_adc_working_flag = 0;
        return 0;
    }
    sys_drv_set_ana_pwd_gadc_buf(1);
    for(UINT8 i = 0; i < (sizeof(adc_chan_buff)/sizeof(UINT8)); i++)
    {
        config.chan = adc_chan_buff[i];
        err = bk_adc_init(config.chan);
        if (err != BK_OK) {
            CLI_LOGE("bk_adc_init failed, chan=%d, ret:0x%x\r\n", config.chan, err);
            break;
        }

        err = bk_adc_set_config(&config);
        if (err != BK_OK) {
            CLI_LOGE("bk_adc_set_config failed, chan=%d, ret:0x%x\r\n", config.chan, err);
            bk_adc_deinit(config.chan);
            break;
        }
        err = bk_adc_enable_bypass_clalibration();
        if (err != BK_OK) {
            CLI_LOGE("bk_adc_enable_bypass_clalibration failed, chan=%d, ret:0x%x\r\n", config.chan, err);
            bk_adc_deinit(config.chan);
            break;
        }
        err = bk_adc_start();
        if (err != BK_OK) {
            CLI_LOGE("bk_adc_start failed, chan=%d, ret:0x%x\r\n", config.chan, err);
            bk_adc_deinit(config.chan);
            break;
        }
        for(UINT8 j = 0; j < loop_num; j++)
        {
            err = bk_adc_read(&value, ADC_READ_SEMAPHORE_WAIT_TIME);
            if (err != BK_OK) {
                CLI_LOGE("bk_adc_read failed, chan=%d, ret:0x%x\r\n", config.chan, err);
                break;
            }

            cali_value = bk_adc_data_calculate(value, config.chan);
            CLI_LOGI("volt:%d mv,chan=%d\n",(uint32_t)(cali_value*1000),config.chan);
        }

        err = cli_adc_stop_and_deinit(config.chan);
        if (err != BK_OK) {
            break;
        }
    }
    sys_drv_set_ana_pwd_gadc_buf(0);
    bk_adc_release();
    s_adc_working_flag = 0;

    return cali_value;
}

static bk_err_t cli_adc_read_single_chan(UINT8 adc_chan)
{
    uint16_t value   = 0;
    float cali_value = 0;
    bk_err_t err = BK_OK;

    if(s_adc_working_flag == 1)
    {
        CLI_LOGI("adc_read is running\r\n");
        return BK_FAIL;
    }

    s_adc_working_flag = 1;
    err = bk_adc_acquire();
    if (err != BK_OK) {
        CLI_LOGE("bk_adc_acquire failed, ret:0x%x\r\n", err);
        s_adc_working_flag = 0;
        return err;
    }
    sys_drv_set_ana_pwd_gadc_buf(1);
    sys_drv_set_ana_hres_sel0v9();
    err = bk_adc_init(adc_chan);
    if (err != BK_OK) {
        CLI_LOGE("bk_adc_init failed, ret:0x%x\r\n", err);
        sys_drv_set_ana_pwd_gadc_buf(0);
        bk_adc_release();
        s_adc_working_flag = 0;
        return err;
    }
    adc_config_t config = {0};

    config.chan = adc_chan;
    config.adc_mode = 3;
    config.src_clk = 1;
    config.clk = 0x1E13D;
    config.saturate_mode = 4;
    config.steady_ctrl= 7;
    config.adc_filter = 0;
    if(config.adc_mode == ADC_CONTINUOUS_MODE) {
        config.sample_rate = 0;
    }

    err = bk_adc_set_config(&config);
    if (err != BK_OK) {
        CLI_LOGE("bk_adc_set_config failed, ret:0x%x\r\n", err);
        bk_adc_deinit(adc_chan);
        sys_drv_set_ana_pwd_gadc_buf(0);
        bk_adc_release();
        s_adc_working_flag = 0;
        return err;
    }
    err = bk_adc_enable_bypass_clalibration();
    if (err != BK_OK) {
        CLI_LOGE("bk_adc_enable_bypass_clalibration failed, ret:0x%x\r\n", err);
        bk_adc_deinit(adc_chan);
        sys_drv_set_ana_pwd_gadc_buf(0);
        bk_adc_release();
        s_adc_working_flag = 0;
        return err;
    }
    err = bk_adc_start();
    if (err != BK_OK) {
        CLI_LOGE("bk_adc_start failed, ret:0x%x\r\n", err);
        bk_adc_deinit(adc_chan);
        sys_drv_set_ana_pwd_gadc_buf(0);
        bk_adc_release();
        s_adc_working_flag = 0;
        return err;
    }
    err = bk_adc_read(&value, ADC_READ_SEMAPHORE_WAIT_TIME);
    if (err != BK_OK) {
        CLI_LOGE("bk_adc_read failed, ret:0x%x\r\n", err);
        bk_adc_stop();
        sys_drv_set_ana_pwd_gadc_buf(0);
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        s_adc_working_flag = 0;
        return err;
    }

    cali_value = bk_adc_data_calculate(value, adc_chan);

    err = cli_adc_stop_and_deinit(adc_chan);
    if (err != BK_OK) {
        sys_drv_set_ana_pwd_gadc_buf(0);
        bk_adc_release();
        s_adc_working_flag = 0;
        return err;
    }
    sys_drv_set_ana_pwd_gadc_buf(0);
    bk_adc_release();
    CLI_LOGI("volt value:%d mv\n",(uint32_t)(cali_value*1000));
    s_adc_working_flag = 0;

    return BK_OK;
}

static void cli_adc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t adc_chan;
    bk_err_t ret = BK_OK;

    if (argc < 2) {
        cli_adc_help();
        return;
    }

    adc_chan = os_strtoul(argv[1], NULL, 10);

    if (os_strcmp(argv[2], "config") == 0) {
        adc_config_t config = {0};
        os_memset(&config, 0, sizeof(adc_config_t));

        config.chan = adc_chan;

        config.adc_mode = os_strtoul(argv[3], NULL, 10);
        config.src_clk = os_strtoul(argv[4], NULL, 10);
        config.clk = os_strtoul(argv[5], NULL, 0);
        config.saturate_mode = os_strtoul(argv[6], NULL, 10);
        config.steady_ctrl= 7;
        config.adc_filter = 0;
        if(config.adc_mode == ADC_CONTINUOUS_MODE) {
            config.sample_rate = os_strtoul(argv[7], NULL, 10);
        }

        ret = bk_adc_init(adc_chan);
        if (ret != BK_OK) {
            CLI_LOGE("adc init failed, ret:%d\r\n", ret);
            return;
        }
        ret = bk_adc_set_config(&config);
        if (ret != BK_OK) {
            CLI_LOGE("adc set config failed, ret:%d\r\n", ret);
            return;
        }
        CLI_LOGI("adc init test:adc_channel:%x, adc_mode:%x,src_clk:%x,adc_clk:%x, saturate_mode:%x, sample_rate:%x",
                 adc_chan, config.adc_mode, config.src_clk, config.clk, config.saturate_mode, config.sample_rate);
        #if CONFIG_SARADC_V1P1
    } else if (os_strcmp(argv[2], "deinit") == 0) {
        ret = bk_adc_deinit(adc_chan);
        if (ret == BK_OK) {
            CLI_LOGI("adc deinit test");
        } else {
            CLI_LOGE("adc deinit failed, ret:%d\r\n", ret);
        }
    } else if (os_strcmp(argv[2], "start") == 0) {
        ret = bk_adc_set_channel(adc_chan);
        if (ret != BK_OK) {
            CLI_LOGE("adc set channel failed, ret:%d\r\n", ret);
            return;
        }
        ret = bk_adc_enable_bypass_clalibration();
        if (ret != BK_OK) {
            CLI_LOGE("adc bypass calibration failed, ret:%d\r\n", ret);
            return;
        }
        ret = bk_adc_start();
        if (ret == BK_OK) {
            CLI_LOGI("start adc test\n");
        } else {
            CLI_LOGE("adc start failed, ret:%d\r\n", ret);
        }
    } else if (os_strcmp(argv[2], "stop") == 0) {
        ret = bk_adc_stop();
        bk_adc_release();
        if (ret == BK_OK) {
            CLI_LOGI("adc_stop test: %d\n", adc_chan);
        } else {
            CLI_LOGE("adc stop failed, ret:%d\r\n", ret);
        }
    } else if (os_strcmp(argv[2], "get_value") == 0) {
        uint16_t value = 0;
        float cali_value = 0;

        if (bk_adc_set_channel(adc_chan) != BK_OK) {
            bk_adc_release();
            return;
        }

        ret = bk_adc_read(&value, ADC_READ_SEMAPHORE_WAIT_TIME);
        if (ret != BK_OK) {
            CLI_LOGE("adc read failed, ret:0x%x\r\n", ret);
            return;
        }
        cali_value = saradc_calculate(value);
        CLI_LOGI("adc value:%d mv\n", (uint32_t)(cali_value * 1000));
        #endif // CONFIG_SARADC_V1P1
    } else if (os_strcmp(argv[2], "read") == 0) {
        uint32_t sum = 0;
        uint32_t size = os_strtoul(argv[3], NULL, 10);
        uint16_t *recv_data = (uint16_t *)os_malloc(size*sizeof(uint16_t));
        if (recv_data == NULL) {
            CLI_LOGE("recv buffer malloc failed\r\n");
            return;
        }

        int time_out = os_strtoul(argv[4], NULL, 10);
        if (time_out < 0) {
            time_out = BEKEN_WAIT_FOREVER;
        }

        ret = bk_adc_acquire();
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_acquire failed, ret:0x%x\r\n", ret);
            os_free(recv_data);
            return;
        }

        if (bk_adc_set_channel(adc_chan) != BK_OK) {
            bk_adc_release();
            os_free(recv_data);
            return;
        }

        ret = bk_adc_read_raw(recv_data, size, time_out);
        bk_adc_release();
        if (ret != BK_OK) {
            CLI_LOGE("adc read failed, ret:%d\r\n", ret);
            os_free(recv_data);
            return;
        }

        if (!recv_data) {
            CLI_LOGE("adc read failed, recv_data is null \r\n");
        }
        CLI_LOGI("adc: read length :time_out:%d read_size:%d\n",time_out, size);

        for (int i = 0; i < size; i++) {
            sum = sum + recv_data[i];
            CLI_LOGI("recv_buffer[%d]=%02x, sum =%d\n", i, recv_data[i], sum);
        }
        sum = sum / size;
        CLI_LOGI("adc read size:%d, adc_result_from _data_reg:%d\n", size, sum);
        os_free(recv_data);
        #if CONFIG_SARADC_V1P1
    } else if (os_strcmp(argv[2], "dump_statis") == 0) {
        adc_statis_dump();
        CLI_LOGI("adc dump statis ok\r\n");
    } else if (os_strcmp(argv[2], "single_step_mode_read") == 0) {
        bk_err_t mode_ret;

        ret = bk_adc_acquire();
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_acquire failed, ret:0x%x\r\n", ret);
            return;
        }
        if (bk_adc_set_channel(adc_chan) != BK_OK) {
            bk_adc_release();
            return;
        }
        mode_ret = bk_adc_set_mode(ADC_SINGLE_STEP_MODE);
        if (mode_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_mode failed, ret:0x%x\r\n", mode_ret);
            bk_adc_release();
            return;
        }
        uint16_t data = 0;
        ret = bk_adc_single_read(&data);
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_single_read failed, ret:0x%x\r\n", ret);
            bk_adc_release();
            return;
        }
        bk_adc_release();
        CLI_LOGI("adc single step mode: adc value is 0x%x\n", data);
    } else if (os_strcmp(argv[2], "reg_cb") == 0) {
        uint32_t size = os_strtoul(argv[3], NULL, 10);
        ret = bk_adc_register_isr(cli_adc_register_cb, size);
        if (ret == BK_OK) {
            CLI_LOGI("adc isr cb register\r\n");
        } else {
            CLI_LOGE("adc isr cb register failed, ret:%d\r\n", ret);
        }
    }
    else if (0 == os_strcmp(argv[2], "saradc_val_read"))
    {
        uint16_t saradc_val_low = 0;
        uint16_t saradc_val_high = 0;

        saradc_get_calibrate_val(&saradc_val_low, SARADC_CALIBRATE_LOW);
        os_printf("calibrate low value:[%x]\r\n", saradc_val_low);

        saradc_get_calibrate_val(&saradc_val_high, SARADC_CALIBRATE_HIGH);
        os_printf("calibrate high value:[%x]\r\n", saradc_val_high);
    }
    else if(0 == os_strcmp(argv[2], "use_sample_set_saradc_val"))
    {
        uint16_t sample_value = 0;
        uint8_t cal_ret;
        SARADC_MODE saradc_cal_mode;
        const char *cal_mode_str = "?";

        adc_config_t config = {0};
        os_memset(&config, 0, sizeof(adc_config_t));

        config.chan = adc_chan;

        config.adc_mode = os_strtoul(argv[3], NULL, 10);
        config.src_clk = os_strtoul(argv[4], NULL, 10);
        config.clk = os_strtoul(argv[5], NULL, 0);
        config.saturate_mode = os_strtoul(argv[6], NULL, 10);
        config.steady_ctrl= 7;
        config.adc_filter = 0;
        if(config.adc_mode == ADC_CONTINUOUS_MODE)
        {
            config.sample_rate = os_strtoul(argv[7], NULL, 10);
        }

        ret = bk_adc_init(adc_chan);
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_init failed, ret:0x%x\r\n", ret);
            return;
        }
        ret = bk_adc_set_config(&config);
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_set_config failed, ret:0x%x\r\n", ret);
            bk_adc_deinit(adc_chan);
            return;
        }
        CLI_LOGI("adc init test:adc_channel:%x, adc_mode:%x,src_clk:%x,adc_clk:%x, saturate_mode:%x, sample_rate:%x",
                 adc_chan, config.adc_mode, config.src_clk, config.clk, config.saturate_mode, config.sample_rate);

        if (0 == os_strcmp(argv[8], "low"))
        {
            saradc_cal_mode = SARADC_CALIBRATE_LOW;
            cal_mode_str = "low";
        }
        else if (0 == os_strcmp(argv[8], "high"))
        {
            saradc_cal_mode = SARADC_CALIBRATE_HIGH;
            cal_mode_str = "high";
        }
        else
        {
            os_printf("invalid parameter\r\n");
            bk_adc_deinit(adc_chan);
            return;
        }

        ret = bk_adc_set_channel(adc_chan);
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_set_channel failed, ret:0x%x\r\n", ret);
            bk_adc_deinit(adc_chan);
            return;
        }

        ret = bk_adc_start();
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_start failed, ret:0x%x\r\n", ret);
            bk_adc_deinit(adc_chan);
            return;
        }
        ret = bk_adc_enable_bypass_clalibration();
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_enable_bypass_clalibration failed, ret:0x%x\r\n", ret);
            bk_adc_stop();
            bk_adc_deinit(adc_chan);
            return;
        }
        CLI_LOGI("start adc test\n");

        ret = bk_adc_acquire();
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_acquire failed, ret:0x%x\r\n", ret);
            bk_adc_stop();
            bk_adc_deinit(adc_chan);
            return;
        }
        ret = bk_adc_read(&sample_value, ADC_READ_SEMAPHORE_WAIT_TIME);
        bk_adc_release();
        if (ret != BK_OK) {
            CLI_LOGE("bk_adc_read failed, ret:0x%x\r\n", ret);
            bk_adc_stop();
            bk_adc_deinit(adc_chan);
            return;
        }

        cal_ret = saradc_set_calibrate_val(&sample_value, saradc_cal_mode);
        if (cal_ret == SARADC_FAILURE) {
            os_printf("saradc_set_calibrate_val fail\r\n");
            bk_adc_stop();
            bk_adc_deinit(adc_chan);
            return;
        }
        ret = cli_adc_stop_and_deinit(adc_chan);
        if (ret != BK_OK) {
            return;
        }

        os_printf("saradc_set_calibrate_val success\r\n");
        os_printf("mode:[%s] value:[%d]\r\n", cal_mode_str, sample_value);
        #endif // CONFIG_SARADC_V1P1
    } else if(os_strcmp(argv[2], "single_adc_example") == 0)
    {
        ret = cli_adc_read_single_chan(adc_chan);
        if (ret != BK_OK) {
            return;
        }
    }
    else if(os_strcmp(argv[2], "multi_adc_example") == 0)
    {
        cli_adc_read_multi_chan();
    }
    else
    {
        cli_adc_help();
        return;
    }
}

#if CONFIG_SARADC_V1P1
static void cli_adc_api_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t api_ret = BK_OK;

    if (argc < 2) {
        cli_adc_help();
        return;
    }

    if (os_strcmp(argv[1], "set_clk") == 0) {
        uint32_t clk = os_strtoul(argv[2], NULL, 10);
        uint32_t src_clk = os_strtoul(argv[3], NULL, 10);
        api_ret = bk_adc_set_clk(src_clk, clk);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_clk failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc set clk: clk_src:%d, clk:%d", src_clk, clk);
    } else if (os_strcmp(argv[1], "set_mode") == 0) {
        uint32_t mode = os_strtoul(argv[2], NULL, 10);
        api_ret = bk_adc_set_mode((adc_mode_t)mode);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_mode failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc set_mode: %x", mode);
    } else if (os_strcmp(argv[1], "set_channel") == 0) {
        uint32_t adc_chan_api = os_strtoul(argv[2], NULL, 10);
        api_ret = bk_adc_set_channel(adc_chan_api);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_channel failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc %d test", adc_chan_api);
    } else if (os_strcmp(argv[1], "set_sample_rate") == 0) {
        uint32_t sample_rate = os_strtoul(argv[2], NULL, 10);
        api_ret = bk_adc_set_sample_rate(sample_rate);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_sample_rate failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc sample rate: %x", sample_rate);
    } else if (os_strcmp(argv[1], "set_filter") == 0) {
        uint32_t filter = os_strtoul(argv[2], NULL, 10);
        api_ret = bk_adc_set_filter(filter);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_filter failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc set_filter : %x", filter);
    } else if (os_strcmp(argv[1], "set_steady_time") == 0) {
        uint32_t time = os_strtoul(argv[2], NULL, 10);
        api_ret = bk_adc_set_steady_time(time);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_steady_time failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc set_steady time : %x", time);
    } else if (os_strcmp(argv[1], "set_sample_cnt") == 0) {
        uint32_t sample_cnt = os_strtoul(argv[2], NULL, 10);
        api_ret = bk_adc_set_sample_cnt(sample_cnt);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_sample_cnt failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc set_sample_cnt: %x", sample_cnt);
    } else if (os_strcmp(argv[1], "set_saturate") == 0) {
        uint32_t saturate = os_strtoul(argv[2], NULL, 10);
        api_ret = bk_adc_set_saturate_mode(saturate);
        if (api_ret != BK_OK) {
            CLI_LOGE("bk_adc_set_saturate_mode failed, ret:0x%x\r\n", api_ret);
            return;
        }
        CLI_LOGI("adc set_set_saturate: %x", saturate);
    } else if (os_strcmp(argv[1], "get_mode") == 0) {
        uint32_t mode = 0;
        mode = bk_adc_get_mode();
        CLI_LOGI("adc get_mode: %x", mode);
    } else {
        cli_adc_help();
        return;
    }
}
#endif // CONFIG_SARADC_V1P1

#define ADC_CMD_CNT (sizeof(s_adc_commands) / sizeof(struct cli_command))
static const struct cli_command s_adc_commands[] = {
    {"adc_driver", "adc_driver [init/deinit]", cli_adc_driver_cmd},
    {"adc_test", "adc_test [channel] [config/deinit/start/stop/get_value/read/dump_statis/single_step_mode_read/reg_cb/saradc_val_read/use_sample_set_saradc_val/single_adc_example/multi_adc_example]", cli_adc_cmd},
    #if CONFIG_SARADC_V1P1
    {"adc_api_test", "adc_api_test []", cli_adc_api_cmd},
    #endif // CONFIG_SARADC_V1P1
};

int cli_adc_init(void)
{
    return cli_register_commands(s_adc_commands, ADC_CMD_CNT);
}
