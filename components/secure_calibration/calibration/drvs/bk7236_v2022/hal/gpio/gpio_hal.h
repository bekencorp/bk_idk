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

#ifdef __cplusplus
extern "C" {
#endif

#include "soc.h"
#include "gpio_ll.h"
#include "gpio_hal.h"

typedef struct {
	gpio_hw_t *hw;
	gpio_id_t gpio_id;
} gpio_hal_t;

typedef struct {
	gpio_int_type_t gpio_int_type;
} gpio_hal_inttype_config_t;

typedef struct {
	gpio_id_t id;
	gpio_dev_t dev[GPIO_PERI_FUNC_NUM];
} gpio_map_t;

bk_err_t gpio_hal_init(gpio_hal_t *hal);
bk_err_t gpio_hal_disable_jtag_mode(gpio_hal_t *hal);

bk_err_t gpio_hal_output_enable(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_input_enable(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_pull_up_enable(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_pull_enable(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_sencond_function_enable(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_monitor_input_enable(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 enable);

bk_err_t gpio_hal_set_capacity(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 capacity);
bk_err_t gpio_hal_set_output_value(gpio_hal_t *hal, gpio_id_t gpio_id, uint32 output_value);
bk_err_t gpio_hal_get_input(gpio_hal_t *hal, gpio_id_t gpio_id);

bk_err_t gpio_hal_set_int_type(gpio_hal_t *hal, gpio_id_t gpio_id, gpio_int_type_t type);

bk_err_t gpio_hal_func_map(gpio_hal_t *hal, gpio_id_t id, gpio_dev_t dev);
bk_err_t gpio_hal_func_unmap(gpio_hal_t *hal, gpio_id_t gpio_id);
bk_err_t gpio_hal_set_config(gpio_hal_t *hal, gpio_id_t gpio_id, const gpio_config_t *config);
bk_err_t gpio_hal_devs_map(gpio_hal_t *hal, uint64 gpios, gpio_dev_t *devs, uint8 dev_num);

bk_err_t gpio_hal_enable_interrupt(gpio_hal_t *hal, gpio_id_t gpio_id);

bk_err_t gpio_hal_reg_save(uint32_t*  gpio_cfg);
bk_err_t gpio_hal_reg_restore(uint32_t*  gpio_cfg);
bk_err_t gpio_hal_wakeup_enable(int64_t index, uint64_t type_l, uint64_t type_h);
bk_err_t gpio_hal_wakeup_interrupt_clear();

#define gpio_hal_disable_interrupt(hal, id)			gpio_ll_disable_interrupt((hal)->hw, id)

#define gpio_hal_get_interrupt_status(hal, status)		gpio_ll_get_interrupt_status((hal)->hw, status)
#define gpio_hal_clear_interrupt_status(hal,status)		gpio_ll_clear_interrupt_status((hal)->hw, status)
#define gpio_hal_is_interrupt_triggered(hal, id,status)		gpio_ll_is_interrupt_triggered((hal)->hw, id, status)
#define gpio_hal_clear_chan_interrupt_status(hal, id)       gpio_ll_clear_chan_interrupt_status((hal)->hw, id)

#if CFG_HAL_DEBUG_GPIO
void gpio_struct_dump(gpio_id_t gpio_index);
#else
#define gpio_struct_dump()
#endif

#ifdef __cplusplus
}
#endif
