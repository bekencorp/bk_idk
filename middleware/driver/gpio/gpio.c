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

#include <common/bk_include.h>
#include <components/log.h>
#include <driver/gpio_types.h>
#include <driver/gpio.h>
#include "gpio_driver_base.h"
#include "gpio_driver.h"

/* ========== GPIO Map Table Implementation ========== */

static gpio_dev_t s_gpio_dev_map_ram[SOC_GPIO_NUM];
static bool s_gpio_map_initialized = false;

static inline bool gpio_map_is_valid_id(gpio_id_t gpio_id)
{
	return (gpio_id < SOC_GPIO_NUM);
}

static inline bool gpio_map_is_valid_dev(gpio_dev_t dev)
{
	return (dev < GPIO_DEV_INVALID);
}

static void gpio_map_table_set_ram_entry_direct(gpio_id_t gpio_id, gpio_dev_t dev)
{
	if (!gpio_map_is_valid_id(gpio_id)) {
		return;
	}
	if (!gpio_map_is_valid_dev(dev)) {
		GPIO_LOGD("set_ram_entry_direct: skip invalid dev 0x%x\r\n", (unsigned int)dev);
		return;
	}
	s_gpio_dev_map_ram[gpio_id] = dev;
}

static const gpio_default_map_entry_t s_gpio_default_map_table[] = GPIO_RAM_DEFAULT_MAP;

static void gpio_map_table_set_default_mappings(void)
{
	for (uint32_t i = 0; i < sizeof(s_gpio_default_map_table) / sizeof(s_gpio_default_map_table[0]); i++) {
		gpio_map_table_set_ram_entry_direct(s_gpio_default_map_table[i].gpio_id, s_gpio_default_map_table[i].dev);
	}
}

static void gpio_map_table_init(void)
{
	if (s_gpio_map_initialized) {
		return;
	}

	for (uint32_t i = 0; i < SOC_GPIO_NUM; i++) {
		s_gpio_dev_map_ram[i] = GPIO_DEV_NONE;
	}

	gpio_map_table_set_default_mappings();

	s_gpio_map_initialized = true;
	GPIO_LOGD("gpio map table initialized, RAM size: %d bytes\r\n",
		   (int)(SOC_GPIO_NUM * sizeof(gpio_dev_t)));
}

static gpio_dev_t gpio_map_find_dev_by_pin(gpio_id_t gpio_id)
{
	if (!gpio_map_is_valid_id(gpio_id)) {
		GPIO_LOGD("find_dev_by_pin: invalid gpio id %d\r\n", gpio_id);
		return GPIO_DEV_INVALID;
	}

	if (!s_gpio_map_initialized) {
		gpio_map_table_init();
	}

	return s_gpio_dev_map_ram[gpio_id];
}

static gpio_id_t gpio_map_find_pin_by_dev(gpio_dev_t dev)
{
	if (dev == GPIO_DEV_NONE || !gpio_map_is_valid_dev(dev)) {
		GPIO_LOGD("find_pin_by_dev: invalid dev 0x%x\r\n", (unsigned int)dev);
		return GPIO_NUM;
	}

	if (!s_gpio_map_initialized) {
		gpio_map_table_init();
	}

	for (uint32_t i = 0; i < SOC_GPIO_NUM; i++) {
		if (s_gpio_dev_map_ram[i] == dev) {
			return (gpio_id_t)i;
		}
	}

	GPIO_LOGD("find_pin_by_dev: dev 0x%x not in map\r\n", (unsigned int)dev);
	return GPIO_NUM;
}

bk_err_t bk_gpio_map_set_ram_map_table(gpio_id_t gpio_id, gpio_dev_t dev)
{
	gpio_dev_t old_dev;

	if (!gpio_map_is_valid_id(gpio_id)) {
		GPIO_LOGD("set_ram_map_table: invalid gpio id %d\r\n", gpio_id);
		return BK_ERR_GPIO_INVALID_ID;
	}
	if (!gpio_map_is_valid_dev(dev)) {
		GPIO_LOGD("set_ram_map_table: invalid dev 0x%x\r\n", (unsigned int)dev);
		return BK_ERR_GPIO_INVALID_ID;
	}

	if (!s_gpio_map_initialized) {
		gpio_map_table_init();
	}

	old_dev = s_gpio_dev_map_ram[gpio_id];
	s_gpio_dev_map_ram[gpio_id] = dev;

	GPIO_LOGD("gpio %d: dev 0x%x -> 0x%x\r\n", gpio_id, old_dev, dev);

	return BK_OK;
}

bk_err_t bk_gpio_map_clear_ram_map_table(gpio_id_t gpio_id)
{
	if (!gpio_map_is_valid_id(gpio_id)) {
		GPIO_LOGD("clear_ram_map_table: invalid gpio id %d\r\n", gpio_id);
		return BK_ERR_GPIO_INVALID_ID;
	}
	return bk_gpio_map_set_ram_map_table(gpio_id, GPIO_DEV_NONE);
}

bk_err_t bk_gpio_map_dev_to_pin(gpio_id_t gpio_id, gpio_dev_t dev)
{
	gpio_id_t old_pin;
	gpio_dev_t old_dev;
	bk_err_t ret;

	if (!gpio_map_is_valid_id(gpio_id)) {
		GPIO_LOGE("bk_gpio_map_dev_to_pin: invalid gpio id %d\r\n", gpio_id);
		return BK_ERR_GPIO_INVALID_ID;
	}
	if (!gpio_map_is_valid_dev(dev)) {
		GPIO_LOGE("bk_gpio_map_dev_to_pin: invalid dev 0x%x\r\n", (unsigned int)dev);
		return BK_ERR_GPIO_SET_INVALID_FUNC_MODE;
	}

	if (!s_gpio_map_initialized)
		gpio_map_table_init();

	/* If dev is already mapped to another pin, unmap it first */
	old_pin = gpio_map_find_pin_by_dev(dev);
	if (old_pin != GPIO_NUM && old_pin != gpio_id) {
		ret = gpio_dev_unmap(old_pin);
		if (ret != BK_OK) {
			GPIO_LOGE("bk_gpio_map_dev_to_pin: unmap old pin %d for dev 0x%x failed, ret=%d\r\n",
				  old_pin, (unsigned int)dev, ret);
			return ret;
		}
	}

	/* If target pin is already mapped to a different dev, unmap it first */
	old_dev = gpio_map_find_dev_by_pin(gpio_id);
	if (gpio_map_is_valid_dev(old_dev) && old_dev != GPIO_DEV_NONE && old_dev != dev) {
		ret = gpio_dev_unmap(gpio_id);
		if (ret != BK_OK) {
			GPIO_LOGE("bk_gpio_map_dev_to_pin: unmap old dev 0x%x from pin %d failed, ret=%d\r\n",
				  (unsigned int)old_dev, gpio_id, ret);
			return ret;
		}
	}

	ret = gpio_dev_map(gpio_id, dev);
	if (ret != BK_OK) {
		GPIO_LOGE("bk_gpio_map_dev_to_pin: map pin %d for dev 0x%x failed, ret=%d\r\n",
				  gpio_id, (unsigned int)dev, ret);
		return ret;
	}

	return BK_OK;
}

