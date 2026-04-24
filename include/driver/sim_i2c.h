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

#include <stdbool.h>
#include <driver/i2c_types.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Init the GPIO simulated I2C
 *
 * This API init the GPIO simulated I2C:
 *  - Configure the GPIO pins for I2C based on id
 *  - Set the I2C clock rate based on baud_rate parameter
 *
 * @param id I2C id, used to select GPIO pins
 * @param cfg I2C parameter settings, baud_rate determines clock speed
 *
 * @attention 1. This is software simulated I2C using GPIO bit-banging
 * @attention 2. Supported baud rates: 100KHz, 200KHz, 400KHz
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_sim_i2c_init(i2c_id_t id, const i2c_config_t *cfg);

 /**
  * @brief     Deinit the GPIO simulated I2C
  *
  * This API deinit the GPIO simulated I2C:
  *   - Set SCL and SDA pins to low state
  *
  * @param id I2C id
  *
  * @return
  *    - BK_OK: succeed
  *    - others: other errors.
  */
bk_err_t bk_sim_i2c_deinit(i2c_id_t id);

/**
 * @brief     Write data to the GPIO simulated I2C port from a given buffer and length,
 *            It shall only be called in I2C master mode.
 *
 * @param id I2C id
 * @param dev_addr slave device address
 * @param data pointer to the buffer
 * @param size data length to write
 * @param timeout_ms timeout ms (not used in simulated I2C)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_sim_i2c_master_write(i2c_id_t id, uint32_t dev_addr, const uint8_t *data, uint32_t size, uint32_t timeout_ms);

/**
 * @brief     Read data from the GPIO simulated I2C port to a given buffer,
 *            It shall only be called in I2C master mode.
 *
 * @param id I2C id
 * @param dev_addr slave device address
 * @param data pointer to the buffer
 * @param size data length to read
 * @param timeout_ms timeout ms (not used in simulated I2C)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_sim_i2c_master_read(i2c_id_t id, uint32_t dev_addr, uint8_t *data, uint32_t size, uint32_t timeout_ms);

/**
 * @brief     Write data to the GPIO simulated I2C memory device
 *
 * @param id I2C id
 * @param mem_param memory write parameters including device address, memory address and data
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_sim_i2c_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param);

/**
 * @brief     Read data from the GPIO simulated I2C memory device
 *
 * @param id I2C id
 * @param mem_param memory read parameters including device address, memory address and data buffer
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_sim_i2c_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param);

/**
 * @brief     Write data to the GPIO simulated I2C port without slave address,
 *            It shall only be called in I2C master mode.
 *
 * @param id I2C id
 * @param data pointer to the buffer
 * @param size data length to write
 * @param timeout_ms timeout ms (not used in simulated I2C)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_sim_i2c_master_write_noaddr(i2c_id_t id, const uint8_t *data, uint32_t size, uint32_t timeout_ms);

/**
 * @brief     Read data from the GPIO simulated I2C port without slave address,
 *            It shall only be called in I2C master mode.
 *
 * @param id I2C id
 * @param data pointer to the buffer
 * @param size data length to read
 * @param timeout_ms timeout ms (not used in simulated I2C)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_sim_i2c_master_read_noaddr(i2c_id_t id, uint8_t *data, uint32_t size, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
