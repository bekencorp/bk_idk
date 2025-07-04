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
#include <driver/pwm_types.h>
#include <driver/pwm.h>
#include <driver/gpio.h>


void bk_dump_hex(const char *prefix, const void *ptr, uint32_t buflen);

#define CONFIG_STRUCT_FIELD_CNT   5
#define ARG_ERR_REC_CNT           20

static void cli_pwm_api_driver_handler(void **argtable)
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
            BK_LOG_ON_ERR(bk_pwm_driver_init());
            CLI_LOGI("PWM driver initialized successfully\r\n");
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_pwm_driver_deinit());
            CLI_LOGI("PWM driver deinitialized successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for init: %s\r\n", driver->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argpwm_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit>", "Init or deinit pwm driver");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, driver, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_pwm_api_driver_handler);
}

static void cli_pwm_api_handler(void **argtable)
{
    pwm_init_config_t pwm_config = {0};
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];
    struct arg_str *chan = (struct arg_str *)argtable[2];
    struct arg_int *arg_config_detail = (struct arg_int *)argtable[3];


    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (driver->count > 0)
    {
        uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
        if (strcmp(driver->sval[0], "init") == 0) {
            if (arg_config_detail->count >= CONFIG_STRUCT_FIELD_CNT)
            {
                pwm_config.period_cycle = (arg_config_detail->ival[0]);
                pwm_config.duty_cycle = (arg_config_detail->ival[1]);
                pwm_config.duty2_cycle = (arg_config_detail->ival[2]);
                pwm_config.duty3_cycle = (arg_config_detail->ival[3]);
                pwm_config.psc = (arg_config_detail->ival[4]);

                CLI_LOGI("pwm_config.period_cycle:0x%x\n", pwm_config.period_cycle);
                CLI_LOGI("pwm_config.duty_cycle:0x%x\n", pwm_config.duty_cycle);
                CLI_LOGI("pwm_config.duty2_cycle:0x%x\n", pwm_config.duty2_cycle);
                CLI_LOGI("pwm_config.duty3_cycle:0x%x\n", pwm_config.duty3_cycle);
                CLI_LOGI("pwm_config.psc:0x%x\n", pwm_config.psc);


                BK_LOG_ON_ERR(bk_pwm_init(channel_id, &pwm_config));
                CLI_LOGI("PWM channel %d init succeeded\n", channel_id);
            } else {
                CLI_LOGE("Not enough configuration parameters provided for channel init.\n");
            }
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_pwm_deinit(channel_id));
            CLI_LOGI("PWM %d driver deinitialized successfully\r\n", channel_id);
        } else if (strcmp(driver->sval[0], "start") == 0) {
            BK_LOG_ON_ERR(bk_pwm_start(channel_id));
            CLI_LOGI("PWM %d driver start successfully\r\n", channel_id);
        } else if (strcmp(driver->sval[0], "stop") == 0) {
            BK_LOG_ON_ERR(bk_pwm_stop(channel_id));
            CLI_LOGI("PWM %d driver stop successfully\r\n", channel_id);
        } else {
            CLI_LOGE("Invalid parameter for pwm: %s\r\n", driver->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argpwm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit/start/stop>", "Init or deinit pwm driver");
    struct arg_str *chan = arg_str0("c", "channel", "<init/deinit/start/stop>", "Init or deinit pwm channel");
    struct arg_int *arg_config_detail = arg_intn(NULL, NULL, "<period_cycle/duty_cycle/duty2_cycle/duty3_cycle/psc>", 0, CONFIG_STRUCT_FIELD_CNT, "Configure the specified channel");

    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, driver, chan, arg_config_detail, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_pwm_api_handler);
}

#ifndef PWM_CLOCK_SRC_XTAL
#define PWM_CLOCK_SRC_XTAL 26000000
#endif
#define PWM_FREQ           (16000)
static uint32_t s_period_cycle = PWM_CLOCK_SRC_XTAL / PWM_FREQ;

static void cli_pwm_api_set_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *set = (struct arg_str *)argtable[1];
    struct arg_str *id = (struct arg_str *)argtable[2];

    pwm_period_duty_config_t pwm_config = {0};
	uint32_t pwm_step = s_period_cycle / 10;
	uint32_t i = 0;
	uint32_t j = 0;
	pwm_chan_t chan = 0;

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (set->count > 0)
    {
        if (strcmp(set->sval[0], "period") == 0) {
            for (j = 0; j < 5; j++) {
                CLI_LOGI("turn on\r\n");
                for (i = 0; i < 11; i++) {
                    for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
                        pwm_config.period_cycle = s_period_cycle;
                        pwm_config.duty_cycle = pwm_step * i;
                        pwm_config.duty2_cycle = 0;
                        pwm_config.duty3_cycle = 0;
                        pwm_config.psc = 0;
                        bk_pwm_set_period_duty(chan, &pwm_config);
                    }
                    rtos_delay_milliseconds(100);
                    chan = 0;
                }

                CLI_LOGI("turn off\r\n");
                for (i = 0; i < 11; i++) {
                    for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
                        pwm_config.period_cycle = s_period_cycle;
                        pwm_config.duty_cycle = pwm_step * (10 - i);
                        pwm_config.duty2_cycle = 0;
                        pwm_config.duty3_cycle = 0;
                        pwm_config.psc = 0;
                        bk_pwm_set_period_duty(chan, &pwm_config);
                    }
                    rtos_delay_milliseconds(100);
                }
            }
            BK_LOG_ON_ERR(bk_pwm_driver_init());
            CLI_LOGI("PWM driver initialized successfully\r\n");
        } else if (strcmp(set->sval[0], "init_low") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_pwm_set_init_signal_low(channel_id));
            CLI_LOGI("PWM %d set the initial signal to low successfully\r\n", channel_id);
        } else if (strcmp(set->sval[0], "init_high") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_pwm_set_init_signal_high(channel_id));
            CLI_LOGI("PWM %d set the initial signal to high successfully\r\n", channel_id);
        } else if (strcmp(set->sval[0], "mode_timer") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(id->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_pwm_set_mode_timer(channel_id));
            CLI_LOGI("Set pwm %d channel as timer modesuccessfully\r\n", channel_id);
        } else {
            CLI_LOGE("Invalid parameter for set: %s\r\n", set->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argpwm_set_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *set = arg_str0("s", "set", "<period/init_low/init_high/mode_timer>", "Set pwm config");
    struct arg_str *id = arg_str0("i", "channel id", "<init/deinit/start/stop>", "Init or deinit pwm channel");

    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, set, id, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_pwm_api_set_handler);
}

static void cli_pwm_api_isr_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *interrupt = (struct arg_str *)argtable[1];
    struct arg_str *isr = (struct arg_str *)argtable[2];
    struct arg_str *chan = (struct arg_str *)argtable[3];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (interrupt->count > 0)
    {
        if (strcmp(interrupt->sval[0], "register") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
            pwm_isr_t pwm_isr = (pwm_isr_t)os_strtoul(isr->sval[0], NULL, 10);

            BK_LOG_ON_ERR(bk_pwm_register_isr(channel_id, pwm_isr));
            CLI_LOGI("PWM chan %d register interrupt isr succeed\n", channel_id);
        } else if (strcmp(interrupt->sval[0], "enable") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);

            BK_LOG_ON_ERR(bk_pwm_enable_interrupt(channel_id));
            CLI_LOGI("PWM chan%d enable interrupt succeed\n", channel_id);
        } else if (strcmp(interrupt->sval[0], "disable") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_pwm_disable_interrupt(channel_id));
            CLI_LOGI("PWM chan%d disable interrupt succeed\n", channel_id);
        } else {
            CLI_LOGE("Invalid parameter for interrupt: %s\r\n", interrupt->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argpwm_isr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *interrupt = arg_str0("i", "interrupt", "<register/enable/disable>", "Set pwm interrupt config");
    struct arg_str *isr = arg_str0("s", "isr", "<isr>", "Set interrupt of PWM channel");
    struct arg_str *chan = arg_str0("c", "channel", "<init/deinit/start/stop>", "Init or deinit pwm channel");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, interrupt, isr, chan, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_pwm_api_isr_handler);
}

static void cli_pwm_api_group_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *group = (struct arg_str *)argtable[1];
    struct arg_int *arg_config_detail = (struct arg_int *)argtable[3];
    pwm_group_init_config_t config = {0};
    pwm_group_t pwm_group = 0;

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (group->count > 0)
    {
        if (strcmp(group->sval[0], "init") == 0) {
            config.chan1 = (arg_config_detail->ival[0]);
            config.chan2 = (arg_config_detail->ival[1]);
            config.period_cycle = (arg_config_detail->ival[2]);
            config.chan1_duty_cycle = (arg_config_detail->ival[3]);
            config.chan2_duty_cycle = (arg_config_detail->ival[4]);
            BK_LOG_ON_ERR(bk_pwm_group_init(&config, &pwm_group));
            CLI_LOGI("pwm init, group=%d chan1=%d chan2=%d period=%x d1=%x d2=%x\n",
                    pwm_group, config.chan1, config.chan2, config.period_cycle,
                    config.chan1_duty_cycle, config.chan2_duty_cycle);
        } else if (strcmp(group->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_pwm_group_deinit(pwm_group));
            CLI_LOGI("pwm deinit, group=%d\n", pwm_group);
        } else if (strcmp(group->sval[0], "start") == 0) {
            BK_LOG_ON_ERR(bk_pwm_group_start(pwm_group));
            CLI_LOGI("pwm start, group=%d\n", pwm_group);
        } else if (strcmp(group->sval[0], "stop") == 0) {
            BK_LOG_ON_ERR(bk_pwm_group_stop(pwm_group));
            CLI_LOGI("pwm stop, group=%d\n", pwm_group);
        } else if (strcmp(group->sval[0], "set_config") == 0) {
		    pwm_group_config_t config = {0};
            config.period_cycle = (arg_config_detail->ival[0]);
            config.chan1_duty_cycle = (arg_config_detail->ival[1]);
            config.chan2_duty_cycle = (arg_config_detail->ival[2]);
            BK_LOG_ON_ERR(bk_pwm_group_set_config(pwm_group, &config));
		    CLI_LOGI("pwm config, group=%x period=%x chan1_duty=%x chan2_duty=%x\n",
				 pwm_group, config.period_cycle, config.chan1_duty_cycle, config.chan2_duty_cycle);
        } else {
            CLI_LOGE("Invalid parameter for group: %s\r\n", group->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argpwm_group_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *group = arg_str0("g", "group", "<init/deinit/set_config/start/stop>", "Store the pwm group");
    struct arg_int *arg_config_detail = arg_intn(NULL, NULL, "<period_cycle/duty_cycle/duty2_cycle/duty3_cycle/psc>", 0, CONFIG_STRUCT_FIELD_CNT, "Configure the specified channel");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, group, arg_config_detail, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_pwm_api_group_handler);
}

static void cli_pwm_capture_isr(pwm_chan_t channel_id)
{
	CLI_LOGI("capture(%d), value=%x\n", channel_id, bk_pwm_capture_get_value(channel_id));
}

static void cli_pwm_api_capture_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *capture = (struct arg_str *)argtable[1];
    struct arg_str *chan = (struct arg_str *)argtable[2];
    struct arg_str *edge = (struct arg_str *)argtable[3];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (capture->count > 0)
    {
        if (strcmp(capture->sval[0], "init") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
            pwm_capture_init_config_t config = {0};
            if (strcmp(edge->sval[0], "pos") == 0) {
                config.edge = PWM_CAPTURE_POS;
            } else if (strcmp(edge->sval[0], "neg") == 0) {
                config.edge = PWM_CAPTURE_NEG;
            } else if (strcmp(edge->sval[0], "edge") == 0) {
                config.edge = PWM_CAPTURE_EDGE;
            } else {
                CLI_LOGE("Invalid parameter for edge: %s\r\n", edge->sval[0]);
            }
            config.isr = cli_pwm_capture_isr;
            BK_LOG_ON_ERR(bk_pwm_capture_init(channel_id, &config));
            CLI_LOGI("pwm_capture init, chan=%d\n", channel_id);
        } else if (strcmp(capture->sval[0], "deinit") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_pwm_capture_deinit(channel_id));
		    CLI_LOGI("pwm_capture deinit, chan=%d\n", channel_id);
        } else if (strcmp(capture->sval[0], "start") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_pwm_capture_start(channel_id));
	    	CLI_LOGI("pwm_capture start, chan=%d\n", channel_id);
        } else if (strcmp(capture->sval[0], "stop") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_pwm_capture_stop(channel_id));
            CLI_LOGI("pwm_capture stop, chan=%d\n", channel_id);
        } else if (strcmp(capture->sval[0], "get_value") == 0) {
            uint32_t channel_id = (uint32_t)os_strtoul(chan->sval[0], NULL, 10);
            CLI_LOGI("capture(%d), value=%x\n", channel_id, bk_pwm_capture_get_value(channel_id));

        } else {
            CLI_LOGE("Invalid parameter for capture: %s\r\n", capture->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argpwm_capture_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *capture = arg_str0("c", "capture", "<init/deinit/start/stop/get_value>", "Set the the configuration of capture");
    struct arg_str *chan = arg_str0("a", "chan", "<channel>", "Set the the configuration of pwm channel");
    struct arg_str *edge = arg_str0("e", "edge", "<pos/neg/edge>", "Set the the configuration of pwm chan capture edge");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, capture, chan, edge, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_pwm_api_capture_handler);
}

#define PWM_CMD_CNT (sizeof(s_pwm_api_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_pwm_api_commands[] = {
    {"argpwm_driver", "argpwm_driver { help | driver }", cli_argpwm_driver_cmd},
    {"argpwm", "argpwm { help | driver config}", cli_argpwm_cmd},
    {"argpwm_set", "argpwm_set { help | set }", cli_argpwm_set_cmd},
    {"argpwm_isr", "argpwm_isr { help | isr }", cli_argpwm_isr_cmd},
    {"argpwm_group", "argpwm_group { help | group }", cli_argpwm_group_cmd},
    {"argpwm_capture", "argpwm_capture { help | capture }", cli_argpwm_capture_cmd},
};

int bk_pwm_register_cli_api_test_feature(void)
{
    return cli_register_module_test_feature(s_pwm_api_commands, PWM_CMD_CNT);
}
// eof

