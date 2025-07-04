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
#include <driver/gpio_types.h>
#include <driver/gpio.h>

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

static void cli_gpio_api_cmd_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];
    struct arg_str *id = (struct arg_str *)argtable[2];
    struct arg_str *able = (struct arg_str *)argtable[3];
    struct arg_str *pull = (struct arg_str *)argtable[4];
    struct arg_str *pull_mode = (struct arg_str *)argtable[5];
    __maybe_unused struct arg_str *func_mode = (struct arg_str *)argtable[6];
    struct arg_str *set = (struct arg_str *)argtable[7];
    struct arg_str *value = (struct arg_str *)argtable[8];
    struct arg_str *capacity = (struct arg_str *)argtable[9];
    struct arg_str *type = (struct arg_str *)argtable[10];
    struct arg_str *get = (struct arg_str *)argtable[11];
    struct arg_str *intterrupt = (struct arg_str *)argtable[12];
    __maybe_unused struct arg_str *wake_up = (struct arg_str *)argtable[13];
    __maybe_unused struct arg_str *status = (struct arg_str *)argtable[14];
    __maybe_unused struct arg_str *io_mode = (struct arg_str *)argtable[15];
    __maybe_unused struct arg_str *retention = (struct arg_str *)argtable[16];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (driver->count > 0)
    {
        if (strcmp(driver->sval[0], "init") == 0) {
            BK_LOG_ON_ERR(bk_gpio_driver_init());
            CLI_LOGI("GPIO driver initialized successfully\r\n");
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_gpio_driver_deinit());
            CLI_LOGI("GPIO driver deinitialized successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for init: %s\r\n", driver->sval[0]);
        }
        return; 
    } 

    else if (able->count > 0)
    {
        gpio_id_t gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(able->sval[0], "en_output") == 0) {
            BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));
            CLI_LOGI("Enable GPIO %d output mode successfully\r\n", gpio_id);
        } else if (strcmp(able->sval[0], "dis_output") == 0) {
            BK_LOG_ON_ERR(bk_gpio_disable_output(gpio_id));
            CLI_LOGI("Disable GPIO %d output mode successfully\r\n", gpio_id);
        } else if (strcmp(able->sval[0], "en_input") == 0) {
            BK_LOG_ON_ERR(bk_gpio_enable_input(gpio_id));
            CLI_LOGI("Enable GPIO %d input mode successfully\r\n", gpio_id);
        } else if (strcmp(able->sval[0], "dis_input") == 0) {
            BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
            CLI_LOGI("Disable GPIO %d input mode successfully\r\n", gpio_id);
        } else if (strcmp(able->sval[0], "en_pull") == 0) {
            BK_LOG_ON_ERR(bk_gpio_enable_pull(gpio_id));
            CLI_LOGI("Enable GPIO %d pull mode successfully\r\n", gpio_id);
        } else if (strcmp(able->sval[0], "dis_pull") == 0) {
            BK_LOG_ON_ERR(bk_gpio_disable_pull(gpio_id));
            CLI_LOGI("Disable GPIO %d pull mode successfully\r\n", gpio_id);
        } else {
            CLI_LOGE("Invalid parameter for able: %s\r\n", able->sval[0]);
        }
        return; 
    }
    
    else if (pull->count > 0)
    {
        gpio_id_t gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(pull->sval[0], "up") == 0) {
            BK_LOG_ON_ERR(bk_gpio_pull_up(gpio_id));
            CLI_LOGI("Set GPIO %d as pull up mode successfully\r\n", gpio_id);
        } else if (strcmp(pull->sval[0], "down") == 0) {
            BK_LOG_ON_ERR(bk_gpio_pull_down(gpio_id));
            CLI_LOGI("Set GPIO %d as pull down mode successfully\r\n", gpio_id);
        } else {
            CLI_LOGE("Invalid parameter for pull: %s\r\n", pull->sval[0]);
        }
        return; 
    } 

    else if (set->count > 0)
    {
        gpio_id_t gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(set->sval[0], "value") == 0) {
            uint32_t gpio_value = os_strtoul(value->sval[0], NULL, 10);
            bk_gpio_set_value(gpio_id, gpio_value);
            CLI_LOGI("Direct set GPIO %d config value %d. successfully\r\n",gpio_id,gpio_value);
        } else if (strcmp(set->sval[0], "config") == 0) {
            gpio_config_t mode;
            mode.io_mode = os_strtoul(io_mode->sval[0], NULL, 10);
            mode.pull_mode = os_strtoul(pull_mode->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_gpio_set_config(gpio_id, &mode));
            CLI_LOGI("gpio io(output/disable/input): %x ,  pull(disable/down/up) : %x\n", mode.io_mode, mode.pull_mode);
        } else if (strcmp(set->sval[0], "output_high") == 0) {
            BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));
            CLI_LOGI("Set GPIO %d output high successfully\r\n",gpio_id);
        } else if (strcmp(set->sval[0], "output_low") == 0) {
            BK_LOG_ON_ERR(bk_gpio_set_output_low(gpio_id));
            CLI_LOGI("Set GPIO %d output low successfully\r\n",gpio_id);
        } else if (strcmp(set->sval[0], "capacity") == 0) {
            uint32_t gpio_capacity = os_strtoul(capacity->sval[0], NULL, 10);
            bk_gpio_set_capacity(gpio_id, gpio_capacity);
            CLI_LOGI("Direct set GPIO %d config capacity %d. successfully\r\n", gpio_id, gpio_capacity);
        } else if (strcmp(set->sval[0], "type") == 0) {
            gpio_int_type_t  interrupt_type = os_strtoul(type->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_gpio_set_interrupt_type(gpio_id, interrupt_type));
            CLI_LOGI("Direct set GPIO %d intterrupt type mode %d successfully when use gpio intterrupt mode\r\n", gpio_id, interrupt_type);
        } else {
            CLI_LOGE("Invalid parameter for set: %s\r\n", set->sval[0]);
        }
        return; 
    }

    else if (get->count > 0)
    {
        uint32_t val = 0;
        bool gpio_value = 0;
        gpio_id_t gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(get->sval[0], "input") == 0) {
            gpio_value = bk_gpio_get_input(gpio_id);
            CLI_LOGI("Get GPIO %d input level(0 :low_level 1:high_level) successfully,input is %d\r\n", gpio_id,gpio_value);
        } else if (strcmp(get->sval[0], "value") == 0) {
            val = bk_gpio_get_value(gpio_id);
            CLI_LOGI("Get GPIO %d config value successfully,val is %d\r\n", gpio_id, val);
        } else {
            CLI_LOGE("Invalid parameter for get: %s\r\n", get->sval[0]);
        }
        return; 
    } 

    else if (intterrupt->count > 0)
    {
        gpio_id_t gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        if (strcmp(intterrupt->sval[0], "enable") == 0) {
            BK_LOG_ON_ERR(bk_gpio_enable_interrupt(gpio_id));
            CLI_LOGI("Enable GPIO %d intterrupt successfully\r\n", gpio_id);
        } else if (strcmp(intterrupt->sval[0], "disable") == 0) {
            BK_LOG_ON_ERR(bk_gpio_disable_interrupt(gpio_id));
            CLI_LOGI("Disable GPIO %d intterrupt successfully\r\n", gpio_id);
        } else if (strcmp(intterrupt->sval[0], "clear") == 0) {
            BK_LOG_ON_ERR(bk_gpio_clear_interrupt(gpio_id));
            CLI_LOGI("Register the interrupt service routine for GPIO %d successfully\r\n", gpio_id);
        } else {
            CLI_LOGE("Invalid parameter for intterrupt: %s\r\n", intterrupt->sval[0]);
        }
        return; 
    } 

    #if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
    else if (wake_up->count > 0)
    {
        gpio_id_t gpio_id = 0;
        uint32_t gpio_mode = 0;

        gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        gpio_mode = os_strtoul(func_mode->sval[0], NULL, 10);
        if (strcmp(wake_up->sval[0], "register") == 0) {
            bk_gpio_register_wakeup_source(gpio_id, gpio_mode);
            CLI_LOGI("GPIO %d register wake up successfully\r\n", gpio_id);
        } else if (strcmp(wake_up->sval[0], "unregister") == 0) {
            bk_gpio_unregister_wakeup_source(gpio_id);
            CLI_LOGI("GPIO %d driver unregister wakeup successfully\r\n", gpio_id);
        } else if (strcmp(wake_up->sval[0], "get_id") == 0) {
            CLI_LOGI("GET wakeup gpio id: %d\r\n", bk_gpio_get_wakeup_gpio_id());
        } else {
            CLI_LOGE("Invalid parameter for wake up: %s\r\n", wake_up->sval[0]);
        }
        return;
    }
    #endif

    #if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
    else if (status->count > 0)
    {
        gpio_id_t gpio_id = 0;
        gpio_config_t config;
		config.io_mode = GPIO_IO_DISABLE;
		config.pull_mode = GPIO_PULL_DISABLE;
		config.func_mode = GPIO_SECOND_FUNC_DISABLE;

        gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        config.io_mode = os_strtoul(io_mode->sval[0], NULL, 10);
        config.pull_mode = os_strtoul(pull_mode->sval[0], NULL, 10);
        config.func_mode = os_strtoul(func_mode->sval[0], NULL, 10);
        if (strcmp(status->sval[0], "register") == 0) {
            CLI_LOGI("cli_gpio_set_keep_status_config register gpio_id: %d\r\n", gpio_id);
            bk_gpio_register_lowpower_keep_status(gpio_id, &config);
        } else if (strcmp(status->sval[0], "unregister") == 0) {
            CLI_LOGI("cli_gpio_set_keep_status_config unregister gpio_id: %d\r\n", gpio_id);
            bk_gpio_unregister_lowpower_keep_status(gpio_id);
        } else if (strcmp(status->sval[0], "external_ldo") == 0) {
            gpio_ctrl_ldo_module_e module = 0;
            gpio_output_state_e value = 0;

            module = os_strtoul(pull_mode->sval[0], NULL, 10);
            value = os_strtoul(func_mode->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_gpio_ctrl_external_ldo(module, gpio_id, value));
            CLI_LOGI("Gpio %d module(GPIO_CTRL_LDO_MODULE_SDIO/GPIO_CTRL_LDO_MODULE_LCD/GPIO_CTRL_LDO_MODULE_MAX): %d ,  value(GPIO_OUTPUT_STATE_LOW/GPIO_OUTPUT_STATE_HIGH) : %d\n",gpio_id, module, value);
        } else {
            CLI_LOGE("Invalid parameter for status: %s\r\n", status->sval[0]);
        }
        return;
    } 
    #endif

    #if CONFIG_GPIO_RETENTION_SUPPORT
    else if (retention->count > 0)
    {
        gpio_id_t gpio_id = 0;
        gpio_output_state_e gpio_output_state = 0;

        gpio_id = (gpio_id_t)os_strtoul(id->sval[0], NULL, 10);
        gpio_output_state = os_strtoul(type->sval[0], NULL, 10);
        if (strcmp(retention->sval[0], "set") == 0) {
            bk_gpio_retention_set(gpio_id, gpio_output_state);
            CLI_LOGI("Set GPIO %d retention index and output_state(GPIO_OUTPUT_STATE_LOW/GPIO_OUTPUT_STATE_HIGH) %d successfully\r\n", gpio_id,gpio_output_state);
        } else if (strcmp(retention->sval[0], "clr") == 0) {
            bk_gpio_retention_clr(gpio_id);
            CLI_LOGI("GPIO %d clear the input GPIO retention index successfully\r\n", gpio_id);
        } else {
            CLI_LOGE("Invalid parameter for retention: %s\r\n", retention->sval[0]);
        }
        return;
    }
    #endif

    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_arggpio_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit>", "Init or deinit GPIO driver");
    struct arg_str *id = arg_str0("i", "id", "<id>", "GPIO ID");
    struct arg_str *able = arg_str0("a", "enable/disable", "<en_output/dis_output/en_input/dis_input/en_pull/dis_pull>", "Enable or disable gpio config");
    struct arg_str *pull = arg_str0("p", "pull", "<up/down>", "Pull up or pull down GPIO value");
    struct arg_str *pull_mode = arg_str0("m", "pull_mode", "<0/1/2>", "Set gpio pull mode");
    struct arg_str *func_mode = arg_str0("f", "func_mode", "<0/1/2>", "Set PIO second function disable/enable");
    struct arg_str *set = arg_str0("s", "set", "<value/config/output_high/output_low/capacity/interrput_type>", "Set gpio config");
    struct arg_str *value = arg_str0("v", "value", "<value>", "GPIO value");
    struct arg_str *capacity = arg_str0("c", "capacity", "<0/1/2/3>", "GPIO capacity");
    struct arg_str *type = arg_str0("t", "type", "<0/1/2/3>", "GPIO interrupt type");
    struct arg_str *get = arg_str0("g", "get", "<input/value>", "GPIO get informaiton");
    struct arg_str *intterrupt = arg_str0("n", "gpio_intterrupt", "<enable/disable/clear>", "GPIO intterrupt config");
    struct arg_str *wake_up = arg_str0("w", "wake_up", "<register/unregister/get_id>", "GPIO wake up config");
    struct arg_str *status = arg_str0("u", "lowpower_keep_status", "<register/unregister/external_ldo>", "Control the external ldo,multi modules power on use one gpio control");
    struct arg_str *io_mode = arg_str0("o", "io_mode", "<0/1/2>", "Set gpio io mode");
    struct arg_str *retention = arg_str0("r", "gpio_retention", "<set/clr>", "Set GPIO retention config");



    struct arg_end *end = arg_end(20);

    void* argtable[] = { help, driver, id, able, pull, pull_mode, func_mode, set, value, capacity, type, get, intterrupt, wake_up, status, io_mode, retention, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_gpio_api_cmd_handler);
}

#define GPIO_CMD_CNT (sizeof(s_gpio_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_gpio_commands[] = {
    {"arggpio", "arggpio { help | driver | arg_size }", cli_arggpio_cmd},
};

int bk_gpio_api_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_gpio_driver_init());
	return cli_register_module_test_feature(s_gpio_commands, GPIO_CMD_CNT);
}
