// Copyright 2020-2021 Beken
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

#pragma once

#include <common/bk_include.h>
#include <components/log.h>
#include <driver/gpio_types.h>

typedef struct {
	gpio_id_t gpio_id;
	gpio_dev_t dev;
} gpio_default_map_entry_t;

#if CONFIG_AON_GPIO
#include "gpio_hal.h"

typedef struct {
	gpio_hal_t hal;
} gpio_driver_t;
#else
typedef struct gpio_driver gpio_driver_t;
#endif


#define GPIO_TAG "gpio"
#define GPIO_LOGI(...) BK_LOGI(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGW(...) BK_LOGW(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGE(...) BK_LOGE(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGD(...) BK_LOGD(GPIO_TAG, ##__VA_ARGS__)

#if CONFIG_GPIO_ATE_TEST
bk_err_t bk_gpio_ate_test(void);
bk_err_t bk_gpio_ate_test_config(int pair_argc, char **pair_argv);
bk_err_t bk_gpio_ate_test_start(void);
bk_err_t bk_gpio_ate_test_show(void);
#endif

bk_err_t bk_gpio_map_set_ram_map_table(gpio_id_t gpio_id, gpio_dev_t dev);
bk_err_t bk_gpio_map_clear_ram_map_table(gpio_id_t gpio_id);
