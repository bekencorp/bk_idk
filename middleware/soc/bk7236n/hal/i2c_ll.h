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

#include <soc/soc.h>
#include "hal_port.h"
#include <driver/hal/hal_gpio_types.h>
#include "i2c_hw.h"
#include <driver/hal/hal_i2c_types.h>
#include "icu_hw.h"
#include "icu_ll.h"
#include "power_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C0_LL_SCL_PIN    GPIO_20
#define I2C0_LL_SDA_PIN    GPIO_21

#define I2C1_LL_SCL_PIN    GPIO_0
#define I2C1_LL_SDA_PIN    GPIO_1

#define I2C_LL_REG_BASE(id)       ((id) == I2C_ID_0 ? SOC_I2C0_REG_BASE : SOC_I2C1_REG_BASE)
#define I2C_LL_REG(id)            ((i2c_hw_t *)((id) == I2C_ID_0 ? SOC_I2C0_REG_BASE : SOC_I2C1_REG_BASE))
#define I2C_LL_INT_STATUS_REG(id) ((id) == I2C_ID_0 ? I2C0_R_INT_STAUS : I2C1_R_INT_STAUS)
#define I2C_LL_DATA_REG(id)       ((id) == I2C_ID_0 ? I2C0_R_DATA : I2C1_R_DATA)
#define I2C_LL_F_SM_INT           BIT(0)

static inline void i2c_ll_soft_reset(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->global_ctrl.soft_reset = 0;
	for(volatile uint32_t i = 0; i < 1000; i++);
	I2C_LL_REG(id)->global_ctrl.soft_reset = 1;
}

static inline uint32_t i2c_ll_get_device_id(i2c_hw_t *hw, i2c_id_t id)
{
	return I2C_LL_REG(id)->dev_id;
}

static inline uint32_t i2c_ll_get_version_id(i2c_hw_t *hw, i2c_id_t id)
{
	return I2C_LL_REG(id)->dev_version;
}

static inline uint32_t i2c_ll_get_dev_status(i2c_hw_t *hw, i2c_id_t id)
{
	return I2C_LL_REG(id)->dev_status;
}

static inline void i2c_ll_init(i2c_hw_t *hw, i2c_id_t id)
{
	i2c_ll_soft_reset(hw, id);
	I2C_LL_REG(id)->sm_bus_cfg.v = 0;
	I2C_LL_REG(id)->sm_bus_status.v = 0;
}

static inline void i2c_ll_enable(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->global_ctrl.clk_gate_bypass = 1;
	I2C_LL_REG(id)->sm_bus_cfg.en = 1;
}

static inline void i2c_ll_disable(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.en = 0;
}

static inline void i2c_master_enable_no_ack(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_status.nack_en = 1;
}

static inline void i2c_master_disable_no_ack(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_status.nack_en = 0;
}

static inline bool i2c_ll_is_no_ack_enabled(i2c_hw_t *hw, i2c_id_t id)
{
	return !!(I2C_LL_REG(id)->sm_bus_status.nack_en);
}

static inline void i2c_ll_set_pin(i2c_hw_t *hw, i2c_id_t id)
{
}

static inline void i2c_ll_set_idle_detect_threshold(i2c_hw_t *hw, i2c_id_t id, uint32_t threshold)
{
	I2C_LL_REG(id)->sm_bus_cfg.idle_cr = threshold;
}

static inline void i2c_ll_set_scl_timeout_threshold(i2c_hw_t *hw, i2c_id_t id, uint32_t threshold)
{
	I2C_LL_REG(id)->sm_bus_cfg.scl_cr = threshold;
}

static inline void i2c_ll_set_clk_src(i2c_hw_t *hw, i2c_id_t id, uint32_t clk_src)
{
	I2C_LL_REG(id)->sm_bus_cfg.clk_src = clk_src;
}

static inline void i2c_ll_enable_scl_timeout(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.timeout_en = 1;
}

static inline void i2c_ll_disable_scl_timeout(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.timeout_en = 0;
}

static inline void i2c_ll_enable_idle_det(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.idle_det_en = 1;
}

static inline void i2c_ll_disable_idle_det(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.idle_det_en = 0;
}

static inline void i2c_ll_enable_slave(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.inh = 0;
}

static inline void i2c_ll_disable_slave(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.inh = 1;
}

static inline void i2c_ll_set_freq_div(i2c_hw_t *hw, i2c_id_t id, uint32_t freq_div)
{
	I2C_LL_REG(id)->sm_bus_cfg.freq_div = freq_div & I2C_F_FREQ_DIV_M;
}

static inline void i2c_ll_set_slave_addr(i2c_hw_t *hw, i2c_id_t id, uint32_t slave_addr)
{
	I2C_LL_REG(id)->sm_bus_cfg.slave_addr = slave_addr & I2C_F_SLAVE_ADDR_M;
}

static inline void i2c_ll_reset_config_to_default(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_cfg.v = 0;
}

static inline void i2c_ll_enable_start(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_status.start = 1;
}

static inline void i2c_ll_disable_start(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_status.start = 0;
}

static inline bool i2c_ll_is_start(i2c_hw_t *hw, i2c_id_t id)
{
	return !!(I2C_LL_REG(id)->sm_bus_status.start);
}

static inline bool i2c_ll_is_start_triggered(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return int_status & BIT(10);
}

static inline void i2c_ll_enable_stop(i2c_hw_t *hw, i2c_id_t id)
{
	/* sm_int and stop (bit[0] and bit[9]) must set together,
	 * otherwize i2c stop will not work.
	 */
}

static inline void i2c_ll_disable_stop(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_status.stop = 0;
}

static inline bool i2c_ll_is_stop_triggered(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return int_status & BIT(9);
}

static inline void i2c_ll_set_write_int_mode(i2c_hw_t *hw, i2c_id_t id, i2c_fifo_int_level_t int_mode)
{
	i2c_hw_t *i2c_hw = I2C_LL_REG(id);
	switch (int_mode) {
	case I2C_FIFO_INT_LEVEL_1:
		i2c_hw->sm_bus_status.int_mode = 0;
		break;
	case I2C_FIFO_INT_LEVEL_4:
		i2c_hw->sm_bus_status.int_mode = 1;
		break;
	case I2C_FIFO_INT_LEVEL_8:
		i2c_hw->sm_bus_status.int_mode = 2;
		break;
	case I2C_FIFO_INT_LEVEL_12:
		i2c_hw->sm_bus_status.int_mode = 3;
		break;
	default:
		break;
	}
}

static inline void i2c_ll_set_read_int_mode(i2c_hw_t *hw, i2c_id_t id, i2c_fifo_int_level_t int_mode)
{
	i2c_hw_t *i2c_hw = I2C_LL_REG(id);
	switch (int_mode) {
	case I2C_FIFO_INT_LEVEL_1:
		i2c_hw->sm_bus_status.int_mode = 3;
		break;
	case I2C_FIFO_INT_LEVEL_4:
		i2c_hw->sm_bus_status.int_mode = 2;
		break;
	case I2C_FIFO_INT_LEVEL_8:
		i2c_hw->sm_bus_status.int_mode = 1;
		break;
	case I2C_FIFO_INT_LEVEL_12:
		i2c_hw->sm_bus_status.int_mode = 0;
		break;
	default:
		break;
	}
}

static inline uint32_t i2c_ll_get_write_empty_fifo_num(i2c_hw_t *hw, i2c_id_t id)
{
	switch (I2C_LL_REG(id)->sm_bus_status.int_mode) {
	case 0x00:
		return 16;
	case 0x01:
		return 12;
	case 0x02:
		return 8;
	case 0x03:
		return 4;
	default:
		return 0;
	}
}

static inline uint32_t i2c_ll_get_read_fifo_num(i2c_hw_t *hw, i2c_id_t id)
{
	switch (I2C_LL_REG(id)->sm_bus_status.int_mode) {
	case 0x00:
		return 12;
	case 0x01:
		return 8;
	case 0x02:
		return 4;
	case 0x03:
		return 1;
	default:
		return 0;
	}
}

static inline bool i2c_ll_is_busy(i2c_hw_t *hw, i2c_id_t id)
{
	return !!(I2C_LL_REG(id)->sm_bus_status.busy);
}

static inline bool i2c_ll_is_addr_matched(i2c_hw_t *hw, i2c_id_t id)
{
	return !!(I2C_LL_REG(id)->sm_bus_status.addr_match);
}

static inline bool i2c_ll_is_tx_mode(i2c_hw_t *hw, i2c_id_t id)
{
	return !!(I2C_LL_REG(id)->sm_bus_status.tx_mode);
}

static inline bool i2c_ll_is_rx_mode(i2c_hw_t *hw, i2c_id_t id)
{
	return !(I2C_LL_REG(id)->sm_bus_status.tx_mode);
}

static inline bool i2c_ll_is_master_mode(i2c_hw_t *hw, i2c_id_t id)
{
	return !!(I2C_LL_REG(id)->sm_bus_status.master);
}

static inline bool i2c_ll_is_slave_mode(i2c_hw_t *hw, i2c_id_t id)
{
	return !(I2C_LL_REG(id)->sm_bus_status.master);
}

static inline uint32_t i2c_ll_get_interrupt_status(i2c_hw_t *hw, i2c_id_t id)
{
	return REG_READ(I2C_LL_INT_STATUS_REG(id));
}

static inline void i2c_ll_clear_interrupt_status(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	volatile uint32_t retry = 0;
	const uint32_t max_retry = 1000;
	uint32_t reg = I2C_LL_INT_STATUS_REG(id);

	if (int_status & I2C_LL_F_SM_INT) {
		uint32_t clear_val = int_status & ~I2C_LL_F_SM_INT;
		REG_WRITE(reg, clear_val);
		while ((REG_READ(reg) & I2C_LL_F_SM_INT) && (retry++ < max_retry)) {
			REG_WRITE(reg, clear_val);
		}
	} else {
		REG_WRITE(reg, int_status);
	}
}

static inline bool i2c_ll_is_sm_int_triggered(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return !!(int_status & BIT(0));
}

static inline bool i2c_ll_is_scl_timeout_triggered(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return !!(int_status & BIT(1));
}

static inline bool i2c_ll_is_arb_lost_triggered(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return !!(int_status & BIT(3));
}

static inline bool i2c_ll_is_rx_ack_triggered(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return !!(int_status & BIT(8));
}

static inline bool i2c_ll_is_tx_ack_triggered(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return !!(int_status & BIT(8));
}

static inline bool i2c_ll_is_receiver_ack_req(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return !!(int_status & BIT(12));
}

static inline bool i2c_ll_is_transmitter_mode(i2c_hw_t *hw, i2c_id_t id, uint32_t int_status)
{
	return !!(int_status & BIT(13));
}

static inline void i2c_ll_tx_ack(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_status.ack = 1;
}

static inline void i2c_ll_tx_non_ack(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_status.ack = 0;
}

static inline void i2c_ll_set_tx_mode(i2c_hw_t *hw, i2c_id_t id)
{
}

static inline void i2c_ll_set_rx_mode(i2c_hw_t *hw, i2c_id_t id)
{
}

static inline void i2c_ll_write_byte(i2c_hw_t *hw, i2c_id_t id, uint32_t data)
{
	REG_WRITE(I2C_LL_DATA_REG(id), data & I2C_F_DATA_M);
}

static inline uint8_t i2c_ll_read_byte(i2c_hw_t *hw, i2c_id_t id)
{
	return I2C_LL_REG(id)->sm_bus_data.data;
}

static inline void i2c_ll_enable_addr_output(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_ex_cfg.addr_h_outen = 1;
}

static inline void i2c_ll_disable_addr_output(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_ex_cfg.addr_h_outen = 0;
}

static inline void i2c_ll_enable_data_output(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_ex_cfg.data_h_outen = 1;
}

static inline void i2c_ll_disable_data_output(i2c_hw_t *hw, i2c_id_t id)
{
	I2C_LL_REG(id)->sm_bus_ex_cfg.data_h_outen = 0;
}

static inline void i2c_ll_set_byte_interval(i2c_hw_t *hw, i2c_id_t id, uint32_t interval)
{
	I2C_LL_REG(id)->sm_bus_ex_cfg.byte_interval = interval;
}

#if (CONFIG_I2C_PM_CB_SUPPORT)
#define I2C_PM_BACKUP_REG_NUM    3

static inline void i2c_ll_backup(i2c_hw_t *hw, i2c_id_t id, uint32_t *pm_backup)
{
	i2c_hw_t *i2c_hw = I2C_LL_REG(id);
	pm_backup[0] = i2c_hw->global_ctrl.v;
	pm_backup[1] = i2c_hw->sm_bus_cfg.v;
	pm_backup[2] = i2c_hw->sm_bus_status.v;
}

static inline void i2c_ll_restore(i2c_hw_t *hw, i2c_id_t id, uint32_t *pm_backup)
{
	i2c_hw_t *i2c_hw = I2C_LL_REG(id);
	i2c_hw->global_ctrl.v = pm_backup[0];
	i2c_hw->sm_bus_cfg.v = pm_backup[1];
	i2c_hw->sm_bus_status.v = pm_backup[2];
}
#endif

#ifdef __cplusplus
}
#endif
