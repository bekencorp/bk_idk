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

#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/i2c.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "icu_driver.h"
#include <os/mem.h>
#include "power_driver.h"
#include <os/os.h>
#include "i2c_hw.h"
#include "i2c_driver.h"
#include "i2c_hal.h"
#include "i2c_statis.h"
#include "sys_driver.h"
#include "bk_misc.h"
#if CONFIG_SPE
#include "security.h"
#endif

#if CONFIG_I2C_PM_CB_SUPPORT
#include <modules/pm.h>
#endif

typedef enum {
    I2C_IDLE = 0,
    I2C_START,
    I2C_TX_DEV_ADDR,
    I2C_RX_DEV_ADDR,
    I2C_TX_MEM_ADDR_HIGH_8BIT,
    I2C_TX_MEM_ADDR_LOW_8BIT,
    I2C_TX_DATA,
    I2C_RX_DATA,
    I2C_STOP,
    I2C_GET_ACK,
} i2c_master_status_t;

typedef struct {
    i2c_hal_t hal;
    uint8_t id_init_bits;
    i2c_work_mode_t work_mode;
    i2c_addr_mode_t addr_mode;
    bool is_addr_tx_done;
    bool is_with_mem_addr;
    uint8_t *data_ptr;
    uint32_t data_size;
    uint32_t data_offset;
    uint16_t dev_addr;
    uint32_t mem_addr;
    i2c_mem_addr_size_t mem_addr_size;
    i2c_master_status_t master_status;
    beken_semaphore_t tx_sema;
    beken_semaphore_t rx_sema;
    uint32_t int_status;
    volatile bk_err_t err_code;

#if CONFIG_I2C_PM_CB_SUPPORT
    uint32_t pm_backup[I2C_PM_BACKUP_REG_NUM];
    uint8_t pm_backup_is_valid;
#endif
} i2c_driver_t;

typedef struct {
    i2c_isr_t callback;
    void *param;
} i2c_callback_t;

typedef struct _cb_context_{
    beken_queue_t *i2c_queue;
    void *app_data;
    beken2_timer_t* i2c_timer;
    int is_app_cb_configured;  /**< Flag indicating if application callback context is initialized
                                       (timer or queue registered via bk_i2c_timer_callback() or
                                       bk_i2c_queue_callback()). Used together with has_timeout_limit to
                                       determine if callback mechanism should be used. */
    int has_timeout_limit;       /**< Flag indicating if current I2C operation has a timeout limit.
                                       Note: Both modes are actually synchronous (blocking), the difference
                                       is only in timeout behavior.
                                       - 1: Has timeout limit (timeout_ms != BEKEN_NEVER_TIMEOUT), operation blocks
                                            with timeout, if timeout occurs and callback is configured,
                                            callback/timer will be used for notification
                                       - 0: No timeout limit (timeout_ms == BEKEN_NEVER_TIMEOUT), operation blocks
                                            indefinitely until completion or error occurs */
    uint8_t transtate;
}i2c_cb_ctx_t;

typedef struct {
    uint8_t *data_ptr;
    uint32_t data_size;
    uint32_t data_offset;
} i2c_slave_data_t;

#define I2C_RETURN_ON_NOT_INIT() do {\
    if (!s_i2c_driver_is_init) {\
        I2C_LOGD("i2c driver not init\r\n");\
        return BK_ERR_I2C_NOT_INIT;\
    }\
} while(0)

#define I2C_RETURN_ON_INVALID_ID(id) do {\
    if ((id) >= I2C_ID_MAX) {\
        I2C_LOGD("i2c id number(%d) is invalid\r\n", (id));\
        return BK_ERR_I2C_INVALID_ID;\
    }\
} while(0)

#define I2C_RETURN_ON_ID_NOT_INIT(id) do {\
    if (!(s_i2c[id].id_init_bits & BIT((id)))) {\
        I2C_LOGD("i2c id number(%d) is not init\r\n", (id));\
        return BK_ERR_I2C_ID_NOT_INIT;\
    }\
} while(0)

#define I2C_SET_PIN(id) do {\
    i2c_hal_set_pin(&s_i2c[id].hal);\
    gpio_dev_unmap(I2C##id##_LL_SDA_PIN);\
    gpio_dev_unmap(I2C##id##_LL_SCL_PIN);\
    gpio_dev_map(I2C##id##_LL_SCL_PIN, GPIO_DEV_I2C##id##_SCL);\
    gpio_dev_map(I2C##id##_LL_SDA_PIN, GPIO_DEV_I2C##id##_SDA);\
    bk_gpio_pull_up(I2C##id##_LL_SCL_PIN);\
    bk_gpio_pull_up(I2C##id##_LL_SDA_PIN);\
} while(0)

#if CONFIG_SPE
#define I2C_CHECK_SECURE(id) do {\
    switch (id) {\
    case I2C_ID_0:\
        BK_ASSERT(DEV_IS_SECURE(I2C0) == 1);\
        break;\
    case I2C_ID_1:\
        BK_ASSERT(DEV_IS_SECURE(I2C1) == 1);\
        break;\
    default:\
        break;\
    }\
} while(0)
#else
#define I2C_CHECK_SECURE(id)
#endif

static i2c_cb_ctx_t i2c_cb_context[SOC_I2C_UNIT_NUM] ={0};
static i2c_driver_t s_i2c[SOC_I2C_UNIT_NUM] = {0};
static i2c_slave_data_t s_slave_tx_ctx[SOC_I2C_UNIT_NUM] = {0};
static i2c_slave_data_t s_slave_rx_ctx[SOC_I2C_UNIT_NUM] = {0};
static bool s_i2c_driver_is_init = false;

static void i2c0_isr(void);
static uint32_t i2c_is_timeout_limit_flag(i2c_id_t id);

#if (SOC_I2C_UNIT_NUM > 1)
static void i2c1_isr(void);
#endif
#if (SOC_I2C_UNIT_NUM > 2)
static void i2c2_isr(void);
#endif

static bk_err_t i2c_wait_sm_bus_idle(i2c_id_t id, uint32_t timeout_ms);
static bk_err_t i2c_master_start(i2c_id_t id);
static bk_err_t i2c_master_set_write_dev_addr(i2c_id_t id, uint16_t dev_addr);
static bk_err_t i2c_master_set_read_dev_addr(i2c_id_t id, uint16_t dev_addr);
static bk_err_t i2c_retry_master_write(i2c_id_t id, uint32_t dev_addr, const uint8_t *data, uint32_t size, uint32_t timeout_ms);
static bk_err_t i2c_retry_master_read(i2c_id_t id, uint32_t dev_addr, uint8_t *data, uint32_t size, uint32_t timeout_ms);
static bk_err_t i2c_retry_noaddr_write(i2c_id_t id, const uint8_t *data, uint32_t size, uint32_t timeout_ms);
static bk_err_t i2c_retry_noaddr_read(i2c_id_t id, uint8_t *data, uint32_t size, uint32_t timeout_ms);
static bk_err_t i2c_retry_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param);
static bk_err_t i2c_retry_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param);

static void i2c_init_gpio(i2c_id_t id)
{
    switch(id) {
    case I2C_ID_0:
        I2C_SET_PIN(0);
        break;
#if (SOC_I2C_UNIT_NUM > 1)
    case I2C_ID_1:
        I2C_SET_PIN(1);
        break;
#endif
#if (SOC_I2C_UNIT_NUM > 2)
    case I2C_ID_2:
        I2C_SET_PIN(2);
        break;
#endif
    default:
        break;
    }
}

static void i2c_clock_enable(i2c_id_t id)
{
    switch(id)
    {
        case I2C_ID_0:
            sys_drv_dev_clk_pwr_up(CLK_PWR_ID_I2C1, CLK_PWR_CTRL_PWR_UP);
            break;

#if (SOC_I2C_UNIT_NUM > 1)
        case I2C_ID_1:
            sys_drv_dev_clk_pwr_up(CLK_PWR_ID_I2C2, CLK_PWR_CTRL_PWR_UP);
            break;
#endif
        default:
            break;
    }
}

static void i2c_clock_disable(i2c_id_t id)
{
    switch(id)
    {
        case I2C_ID_0:
            sys_drv_dev_clk_pwr_up(CLK_PWR_ID_I2C1, CLK_PWR_CTRL_PWR_DOWN);
            break;
#if (SOC_I2C_UNIT_NUM > 1)
        case I2C_ID_1:
            sys_drv_dev_clk_pwr_up(CLK_PWR_ID_I2C2, CLK_PWR_CTRL_PWR_DOWN);
            break;
#endif
        default:
            break;
    }
}

static void i2c_interrupt_enable(i2c_id_t id)
{
    switch(id)
    {
        case I2C_ID_0:
            sys_drv_int_enable(I2C_INTERRUPT_CTRL_BIT);
            break;
#if (SOC_I2C_UNIT_NUM > 1)
        case I2C_ID_1:
            sys_drv_int_enable(I2C1_INTERRUPT_CTRL_BIT);
            break;
#endif
        default:
            break;
    }
}

static void i2c_interrupt_disable(i2c_id_t id)
{
    switch(id)
    {
        case I2C_ID_0:
            sys_drv_int_disable(I2C_INTERRUPT_CTRL_BIT);
            break;
#if (SOC_I2C_UNIT_NUM > 1)
        case I2C_ID_1:
            sys_drv_int_disable(I2C1_INTERRUPT_CTRL_BIT);
            break;
#endif
        default:
            break;
    }
}


/* 1. power up i2c
 * 2. set clk
 * 3. icu enable interrupt
 * 4. set gpio as i2c
 */
static void i2c_id_init_common(i2c_id_t id)
{
    bk_err_t ret = 0;

    i2c_clock_enable(id);
    i2c_interrupt_enable(id);

#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
    /*
     * GPIO info is setted in GPIO_DEFAULT_DEV_CONFIG and
     * inited in bk_gpio_driver_init->gpio_hal_default_map_init.
     * If needs to re-config GPIO, can deal it here.
     */
#else
    i2c_init_gpio(id);
#endif
    if (s_i2c[id].tx_sema == NULL) {
        ret = rtos_init_semaphore(&(s_i2c[id].tx_sema), 1);
        BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
    }

    if (s_i2c[id].rx_sema == NULL) {
        ret = rtos_init_semaphore(&(s_i2c[id].rx_sema), 1);
        BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
    }

    s_i2c[id].id_init_bits |= BIT(id);
}

static void i2c_id_deinit_common(i2c_id_t id)
{
    i2c_hal_stop_common(&s_i2c[id].hal);
    i2c_hal_reset_config_to_default(&s_i2c[id].hal);

    i2c_clock_disable(id);
    i2c_interrupt_disable(id);

    s_i2c[id].addr_mode = 0;

    if (s_i2c[id].tx_sema) {
        rtos_deinit_semaphore(&(s_i2c[id].tx_sema));
        s_i2c[id].tx_sema = NULL;
    }
    if (s_i2c[id].rx_sema) {
        rtos_deinit_semaphore(&(s_i2c[id].rx_sema));
        s_i2c[id].rx_sema = NULL;
    }
    s_i2c[id].id_init_bits &= ~BIT(id);
}

/**
* @brief Get GPIO pin numbers for I2C SDA and SCL
* @param id I2C ID
* @param scl_pin Pointer to store SCL pin number
* @param sda_pin Pointer to store SDA pin number
* @return BK_OK on success
*/
static bk_err_t i2c_get_gpio_pins(i2c_id_t id, uint32_t *scl_pin, uint32_t *sda_pin)
{
    switch(id) {
    case I2C_ID_0:
        *scl_pin = I2C0_LL_SCL_PIN;
        *sda_pin = I2C0_LL_SDA_PIN;
        break;
#if (SOC_I2C_UNIT_NUM > 1)
    case I2C_ID_1:
        *scl_pin = I2C1_LL_SCL_PIN;
        *sda_pin = I2C1_LL_SDA_PIN;
        break;
#endif
#if (SOC_I2C_UNIT_NUM > 2)
    case I2C_ID_2:
        *scl_pin = I2C2_LL_SCL_PIN;
        *sda_pin = I2C2_LL_SDA_PIN;
        break;
#endif
    default:
        return BK_ERR_I2C_ID_NOT_INIT;
    }
    return BK_OK;
}

/**
 * @brief Retry master write operation after bus recovery
 * @param id I2C ID
 * @param dev_addr Device address
 * @param data Data buffer
 * @param size Data size
 * @param timeout_ms Timeout in milliseconds
 * @return BK_OK on success
 */
static bk_err_t i2c_retry_master_write(i2c_id_t id, uint32_t dev_addr, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));

	uint32_t int_level = rtos_enter_critical();
	I2C_LOGI("retry i2c master write, dev_addr=0x%02x, size=%d\r\n", dev_addr, size);
	s_i2c[id].err_code = BK_OK;
	s_i2c[id].work_mode = I2C_MASTER_WRITE;
	s_i2c[id].is_with_mem_addr = false;
	s_i2c[id].dev_addr = dev_addr;
	s_i2c[id].data_ptr = (uint8_t *)data;
	s_i2c[id].data_size = size;
	s_i2c[id].data_offset = 0;
	s_i2c[id].int_status = 0;
	i2c_master_start(id);
	i2c_master_set_write_dev_addr(id, dev_addr);
	i2c_hal_set_write_int_mode(&s_i2c[id].hal, size);
#if (CONFIG_SOC_BK7236XX)
	i2c_hal_disable_addr_output(&s_i2c[id].hal);
	i2c_hal_disable_data_output(&s_i2c[id].hal);
#endif
	rtos_exit_critical(int_level);

	rtos_get_semaphore(&s_i2c[id].tx_sema, timeout_ms);

	return BK_OK;
}

/**
 * @brief Retry master read operation after bus recovery
 * @param id I2C ID
 * @param dev_addr Device address
 * @param data Data buffer
 * @param size Data size
 * @param timeout_ms Timeout in milliseconds
 * @return BK_OK on success
 */
static bk_err_t i2c_retry_master_read(i2c_id_t id, uint32_t dev_addr, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));

	uint32_t int_level = rtos_enter_critical();
	I2C_LOGI("retry i2c master read, dev_addr=0x%02x, size=%d\r\n", dev_addr, size);
	s_i2c[id].err_code = BK_OK;
	s_i2c[id].work_mode = I2C_MASTER_READ;
	s_i2c[id].is_with_mem_addr = false;
	s_i2c[id].dev_addr = dev_addr;
	s_i2c[id].data_ptr = data;
	s_i2c[id].data_size = size;
	s_i2c[id].data_offset = 0;
	s_i2c[id].int_status = 0;
	i2c_master_start(id);
	i2c_master_set_read_dev_addr(id, dev_addr);
	i2c_hal_set_rx_mode(&s_i2c[id].hal);
	i2c_hal_set_read_int_mode(&s_i2c[id].hal, size);
#if (CONFIG_SOC_BK7236XX)
	i2c_hal_disable_addr_output(&s_i2c[id].hal);
	i2c_hal_disable_data_output(&s_i2c[id].hal);
#endif
	rtos_exit_critical(int_level);

	rtos_get_semaphore(&s_i2c[id].rx_sema, timeout_ms);

	return BK_OK;
}

/**
 * @brief Retry noaddr write operation after bus recovery
 * @param id I2C ID
 * @param data Data buffer (first byte is dev_addr)
 * @param size Data size (including dev_addr)
 * @param timeout_ms Timeout in milliseconds
 * @return BK_OK on success
 */
static bk_err_t i2c_retry_noaddr_write(i2c_id_t id, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));

	uint32_t int_level = rtos_enter_critical();
	I2C_LOGI("retry i2c noaddr write, size=%d\r\n", size);
	s_i2c[id].err_code = BK_OK;
	s_i2c[id].work_mode = I2C_MASTER_WRITE;
	s_i2c[id].is_with_mem_addr = false;
	s_i2c[id].dev_addr = data[0];
	s_i2c[id].data_ptr = (uint8_t *)data + 1;
	s_i2c[id].data_size = size - 1;
	s_i2c[id].data_offset = 0;
	s_i2c[id].int_status = 0;
	i2c_master_start(id);
	i2c_hal_write_byte(&s_i2c[id].hal, data[0]);
	i2c_hal_enable_start(&s_i2c[id].hal);
	s_i2c[id].master_status = I2C_TX_DATA;
	i2c_hal_set_write_int_mode(&s_i2c[id].hal, size);
	rtos_exit_critical(int_level);

	rtos_get_semaphore(&s_i2c[id].tx_sema, timeout_ms);

	return BK_OK;
}

/**
 * @brief Retry noaddr read operation after bus recovery
 * @param id I2C ID
 * @param data Data buffer
 * @param size Data size
 * @param timeout_ms Timeout in milliseconds
 * @return BK_OK on success
 */
static bk_err_t i2c_retry_noaddr_read(i2c_id_t id, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));

	uint32_t int_level = rtos_enter_critical();
	I2C_LOGI("retry i2c noaddr read, size=%d\r\n", size);
	s_i2c[id].err_code = BK_OK;
	s_i2c[id].work_mode = I2C_MASTER_READ;
	s_i2c[id].is_with_mem_addr = false;
	s_i2c[id].dev_addr = data[0];
	s_i2c[id].data_ptr = data;
	s_i2c[id].data_size = size;
	s_i2c[id].data_offset = 0;
	s_i2c[id].int_status = 0;
	i2c_master_start(id);
	// skip read slave address
	s_i2c[id].master_status = I2C_RX_DATA;
	i2c_hal_enable_start(&s_i2c[id].hal);
	s_i2c[id].int_status |= I2C1_F_START;
	i2c_hal_set_rx_mode(&s_i2c[id].hal);
	i2c_hal_set_read_int_mode(&s_i2c[id].hal, size);
	rtos_exit_critical(int_level);

	rtos_get_semaphore(&s_i2c[id].rx_sema, timeout_ms);

	return BK_OK;
}

/**
 * @brief Retry memory write operation after bus recovery
 * @param id I2C ID
 * @param mem_param Memory parameter structure
 * @return BK_OK on success
 */
static bk_err_t i2c_retry_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, mem_param->timeout_ms));

	uint32_t int_level = rtos_enter_critical();
	I2C_LOGI("retry i2c memory write, dev_addr=0x%02x, mem_addr=0x%04x, size=%d\r\n",
	         mem_param->dev_addr, mem_param->mem_addr, mem_param->data_size);
	s_i2c[id].err_code = BK_OK;
	s_i2c[id].work_mode = I2C_MASTER_WRITE;
	s_i2c[id].is_with_mem_addr = true;
	s_i2c[id].dev_addr = mem_param->dev_addr;
	s_i2c[id].mem_addr = mem_param->mem_addr;
	s_i2c[id].mem_addr_size = mem_param->mem_addr_size;
	s_i2c[id].data_ptr = mem_param->data;
	s_i2c[id].data_size = mem_param->data_size;
	s_i2c[id].data_offset = 0;
	s_i2c[id].int_status = 0;
	i2c_master_start(id);
	i2c_master_set_write_dev_addr(id, mem_param->dev_addr);
	i2c_hal_set_write_int_mode(&s_i2c[id].hal, mem_param->data_size);
	rtos_exit_critical(int_level);

	rtos_get_semaphore(&s_i2c[id].tx_sema, mem_param->timeout_ms);

	return BK_OK;
}

/**
 * @brief Retry memory read operation after bus recovery
 * @param id I2C ID
 * @param mem_param Memory parameter structure
 * @return BK_OK on success
 */
static bk_err_t i2c_retry_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, mem_param->timeout_ms));

	uint32_t int_level = rtos_enter_critical();
	I2C_LOGI("retry i2c memory read, dev_addr=0x%02x, mem_addr=0x%04x, size=%d\r\n",
	         mem_param->dev_addr, mem_param->mem_addr, mem_param->data_size);
	s_i2c[id].err_code = BK_OK;
	s_i2c[id].work_mode = I2C_MASTER_READ;
	s_i2c[id].is_with_mem_addr = true;
	s_i2c[id].dev_addr = mem_param->dev_addr;
	s_i2c[id].mem_addr = mem_param->mem_addr;
	s_i2c[id].mem_addr_size = mem_param->mem_addr_size;
	s_i2c[id].data_ptr = mem_param->data;
	s_i2c[id].data_size = mem_param->data_size;
	s_i2c[id].data_offset = 0;
	s_i2c[id].int_status = 0;
	i2c_master_start(id);
	i2c_master_set_write_dev_addr(id, mem_param->dev_addr);
	i2c_hal_set_read_int_mode(&s_i2c[id].hal, mem_param->data_size);
	rtos_exit_critical(int_level);

	rtos_get_semaphore(&s_i2c[id].rx_sema, mem_param->timeout_ms);

	return BK_OK;
}

/**
 * @brief Recover I2C bus when stuck (SDA low, SCL high)
 * This function implements the standard I2C bus recovery procedure:
 * 1. Switch GPIO from I2C function to normal GPIO
 * 2. Send clock pulses to recover bus
 * 3. Send STOP condition
 * 4. Reconfigure GPIO back to I2C function
 * 5. Soft reset I2C hardware
 * @param id I2C ID
 * @return BK_OK on success
 */
static bk_err_t i2c_recover_bus(i2c_id_t id)
{
    uint32_t int_level = rtos_enter_critical();
    uint32_t scl_pin, sda_pin;
    uint32_t i = 0;
    /* Fixed 100kHz clock for recovery: half period = 5us */
    const uint32_t half_delay_us = 5;

    // Get GPIO pin numbers
    if (i2c_get_gpio_pins(id, &scl_pin, &sda_pin) != BK_OK) {
        I2C_LOGE("failed to get gpio pins for i2c id %d\r\n", id);
        rtos_exit_critical(int_level);
        return BK_ERR_I2C_ID_NOT_INIT;
    }

    I2C_LOGW("try to recover i2c bus\r\n");

    // Switch GPIO from I2C function to normal GPIO
#if CONFIG_SUPPORT_IO_MATRIX
    bk_gpio_set_value(scl_pin, 0x30);
    bk_gpio_set_value(sda_pin, 0x30);

    bk_gpio_set_value(scl_pin, 0x2000000);
    bk_gpio_set_value(sda_pin, 0x1000030);
#else
    bk_gpio_set_value(scl_pin, 0x38);
    bk_gpio_set_value(sda_pin, 0x38);

    bk_gpio_set_value(scl_pin, 0x0);
    bk_gpio_set_value(sda_pin, 0x3C);
#endif

    bk_delay_us(half_delay_us);

    // Send 9 clock pulses to recover bus
    while (!bk_gpio_get_input(sda_pin) && (i++ < 9)) {
#if CONFIG_SUPPORT_IO_MATRIX
        bk_gpio_set_value(scl_pin, 0x30);
        bk_delay_us(half_delay_us);
        bk_gpio_set_value(scl_pin, 0x2000000);
        bk_delay_us(half_delay_us);
#else
        bk_gpio_set_value(scl_pin, 0x38);
        bk_delay_us(half_delay_us);
        bk_gpio_set_value(scl_pin, 0x0);
        bk_delay_us(half_delay_us);
#endif
    }

    // Send STOP condition: SDA low -> SCL high -> SDA high
#if CONFIG_SUPPORT_IO_MATRIX
    bk_gpio_set_value(sda_pin, 0x2000000);
    bk_gpio_set_value(scl_pin, 0x30);
    bk_delay_us(half_delay_us);
    bk_gpio_set_value(sda_pin, 0x1000030);
#else
    bk_gpio_set_value(sda_pin, 0x0);
    bk_gpio_set_value(scl_pin, 0x38);
    bk_delay_us(half_delay_us);
    bk_gpio_set_value(sda_pin, 0x3C);
#endif

    // Reconfigure GPIO back to I2C function
    i2c_init_gpio(id);

    // Soft reset I2C hardware
    i2c_hal_soft_reset(&s_i2c[id].hal);

    s_i2c[id].master_status = 0;
    rtos_exit_critical(int_level);

    if (i == 10) {
        I2C_LOGW("recover i2c bus failed\r\n");
        return BK_ERR_I2C_BUS_BUSY_TIMEOUT;
    } else {
        I2C_LOGW("recover i2c bus success, clock index=%d\r\n", i);
        return BK_OK;
    }
}

static bk_err_t i2c_wait_sm_bus_idle(i2c_id_t id, uint32_t timeout_ms)
{
    uint32_t start_tick = 0;
    uint32_t cur_tick = 0;
    uint32_t tick_offset = 0;

    start_tick = rtos_get_time();
    while (i2c_hal_is_busy(&s_i2c[id].hal)) {
        cur_tick = rtos_get_time();
        tick_offset = (cur_tick >= start_tick) ? (cur_tick - start_tick) : (0xffffffff - start_tick + cur_tick);
        if (tick_offset > timeout_ms) {
            I2C_LOGW("i2c sm bus is busy\r\n");
            // Attempt to recover the bus
            if (i2c_recover_bus(id) == BK_OK) {
                // Wait a bit more after recovery
                rtos_delay_milliseconds(10);
                // Check if bus is now idle
                if (!i2c_hal_is_busy(&s_i2c[id].hal)) {
                    return BK_OK;
                }
            }
            return BK_ERR_I2C_SM_BUS_BUSY;
        }
    }

    return BK_OK;
}

static void i2c_transtate_reset (int id)
{
    i2c_cb_context[id].transtate = 0;
}

static void i2c_transtate_set (int id)
{
    i2c_cb_context[id].transtate = 1;
}

static void i2c_timer_start (int id)
{
    int ret;
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];
    i2c_cb_ctx_t *cb_context = &i2c_cb_context[id];

    if((!cb_context->is_app_cb_configured)){
        return;
    }
    if(NULL == cb_context->i2c_timer){
        return;
    }

    if (i2c_is_timeout_limit_flag(id)) {
        ret = rtos_start_oneshot_timer( cb_context->i2c_timer) ;
        if(ret != kNoErr) {
            I2C_LOGW("i2c(%d) timer start failed\r\n", id);
            i2c_drv_obj->err_code = BK_ERR_I2C_START_TIMER_FAILED;
        }
    }
}

/**
 * @brief Fill timer callback parameters
 *
 * @param id I2C device ID
 *
 * If the application has registered a timer callback, fill the callback parameter structure.
 * Parameters include: actual data length transferred, work mode, and application data pointer.
 */
static void i2c_fill_timer_callback_param(i2c_id_t id)
{
    i2c_cb_param_t *cb_param;
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];
    i2c_cb_ctx_t *cb_context = &i2c_cb_context[id];

    if(NULL == cb_context->i2c_timer){
        return;
    }
    cb_param = (i2c_cb_param_t *)cb_context->i2c_timer->left_arg;
    if(NULL == cb_param){
        return;
    }

    cb_param->data_len = i2c_drv_obj->data_offset;
    cb_param->work_mode = i2c_drv_obj->work_mode;
    cb_param->app_data = cb_context->i2c_timer->right_arg;
}

/**
 * @brief Set timeout limit flag based on timeout value
 *
 * @param id I2C device ID
 * @param timeout_ms Timeout value in milliseconds
 *
 * Sets the has_timeout_limit flag in the callback context based on whether
 * the timeout value is BEKEN_NEVER_TIMEOUT (infinite wait) or a finite value.
 */
static void i2c_set_timeout_limit_flag(i2c_id_t id, uint32_t timeout_ms)
{
    i2c_cb_ctx_t *cb_context = &i2c_cb_context[id];

    if (timeout_ms == BEKEN_NEVER_TIMEOUT) {
        cb_context->has_timeout_limit = 0;
    } else {
        cb_context->has_timeout_limit = 1;
    }
}

static uint32_t i2c_is_timeout_limit_flag(i2c_id_t id)
{
    i2c_cb_ctx_t *cb_context = &i2c_cb_context[id];
    return cb_context->has_timeout_limit;
}

static bool i2c_is_no_ack_mode_enabled(i2c_id_t id)
{
    return i2c_hal_is_no_ack_mode_enabled(&s_i2c[id].hal);
}

/**
 * @brief Switch slave work mode based on HAL TX/RX mode
 *
 * @param id I2C device ID
 * @param hal Pointer to I2C HAL structure
 *
 * In slave mode, when the HAL layer switches between TX and RX modes,
 * this function switches the driver's work mode accordingly and restores
 * the corresponding data context (data pointer, size, and offset) from
 * the saved context.
 */
static void i2c_switch_slave_work_mode(i2c_id_t id, i2c_hal_t *hal)
{
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];

    if (!i2c_hal_is_slave_mode(hal)) {
        return;
    }

    i2c_slave_data_t *slave_rx_ctx = &s_slave_rx_ctx[id];
    i2c_slave_data_t *slave_tx_ctx = &s_slave_tx_ctx[id];

    if (i2c_hal_is_tx_mode(hal)
        && (i2c_drv_obj->work_mode != I2C_SLAVE_WRITE)) {
        i2c_drv_obj->work_mode = I2C_SLAVE_WRITE;
        i2c_drv_obj->data_ptr = slave_tx_ctx->data_ptr;
        i2c_drv_obj->data_size = slave_tx_ctx->data_size;
        i2c_drv_obj->data_offset = slave_tx_ctx->data_offset;
    } else if (i2c_hal_is_rx_mode(hal)
        && (i2c_drv_obj->work_mode != I2C_SLAVE_READ)) {
        i2c_drv_obj->work_mode = I2C_SLAVE_READ;
        i2c_drv_obj->data_ptr = slave_rx_ctx->data_ptr;
        i2c_drv_obj->data_size = slave_rx_ctx->data_size;
        i2c_drv_obj->data_offset = slave_rx_ctx->data_offset;
    }
}

/**
 * @brief Push message queue callback notification
 *
 * @param id I2C device ID
 *
 * If the application has registered a message queue callback, create callback parameters
 * and push them to the queue. Parameters include: actual data length transferred, work mode,
 * and application data pointer.
 */
static void i2c_push_queue_callback(i2c_id_t id)
{
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];
    i2c_cb_ctx_t *cb_context = &i2c_cb_context[id];
    i2c_cb_param_t cb_param;
    bk_err_t ret;

    if (NULL == cb_context->i2c_queue) {
        return;
    }

    cb_param.data_len = i2c_drv_obj->data_offset;
    cb_param.work_mode = i2c_drv_obj->work_mode;
    cb_param.app_data = cb_context->app_data;

    ret = rtos_push_to_queue(cb_context->i2c_queue, &cb_param, BEKEN_NO_WAIT);
    if (ret != kNoErr) {
        I2C_LOGW("i2c(%d) push callback to queue failed, queue may be full\r\n", id);
    }
}

static bk_err_t i2c_master_start(i2c_id_t id)
{
    i2c_hal_soft_reset(&s_i2c[id].hal);
    i2c_hal_disable_stop(&s_i2c[id].hal);
    i2c_hal_set_tx_mode(&s_i2c[id].hal);

    s_i2c[id].master_status = I2C_TX_DEV_ADDR;
    i2c_transtate_reset(id);

    return BK_OK;
}

static bk_err_t i2c_master_stop(i2c_id_t id)
{
    i2c_hal_enable_stop(&s_i2c[id].hal);
    s_i2c[id].int_status |= I2C1_F_STOP;
    s_i2c[id].master_status = I2C_IDLE;

    i2c_fill_timer_callback_param(id);
    i2c_push_queue_callback(id);
    i2c_timer_start(id);

    return BK_OK;
}

static bk_err_t i2c_master_set_write_dev_addr(i2c_id_t id, uint16_t dev_addr)
{
    /* write slave_addr first,then enable start bit,
     * otherwise the slave_addr written will be zero
     */
    i2c_hal_write_byte(&s_i2c[id].hal, dev_addr << 1 & (~0x01));
    i2c_hal_enable_start(&s_i2c[id].hal);

    if (!s_i2c[id].is_with_mem_addr) {
        s_i2c[id].master_status = I2C_TX_DATA;
        return BK_OK;
    }

    if (s_i2c[id].mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
        s_i2c[id].master_status = I2C_TX_MEM_ADDR_HIGH_8BIT;
    } else {
        s_i2c[id].master_status = I2C_TX_MEM_ADDR_LOW_8BIT;
    }
    return BK_OK;
}

static bk_err_t i2c_master_set_read_dev_addr(i2c_id_t id, uint16_t dev_addr)
{
    i2c_hal_write_byte(&s_i2c[id].hal, dev_addr << 1 | 0x01);
    i2c_hal_enable_start(&s_i2c[id].hal);
    s_i2c[id].int_status |= I2C1_F_START;
    s_i2c[id].master_status = I2C_RX_DATA;
    return BK_OK;
}

static bk_err_t i2c_master_write_mem_addr_high_8bit(i2c_id_t id, uint16_t mem_addr)
{
    i2c_hal_write_byte(&s_i2c[id].hal, mem_addr >> 8 & 0xff);
    s_i2c[id].master_status = I2C_TX_MEM_ADDR_LOW_8BIT;
    return BK_OK;
}

static bk_err_t i2c_master_write_mem_addr_low_8bit(i2c_id_t id, uint16_t mem_addr)
{
    i2c_hal_write_byte(&s_i2c[id].hal, mem_addr & 0xff);
    if (s_i2c[id].work_mode == I2C_MASTER_WRITE) {
        s_i2c[id].master_status = I2C_TX_DATA;
    } else if (s_i2c[id].work_mode == I2C_MASTER_READ) {
        s_i2c[id].master_status = I2C_RX_DEV_ADDR;
    }
    return BK_OK;
}

static bk_err_t i2c_master_write_byte(i2c_id_t id, uint8_t data)
{
    i2c_hal_write_byte(&s_i2c[id].hal, data);
    return BK_OK;
}

#if (CONFIG_FM_I2C)
static uint8_t i2c_master_read_byte(i2c_id_t id)
{
    return i2c_hal_read_byte(&s_i2c[id].hal);
}

static bk_err_t i2c0_master_write_data(i2c_id_t id)
{
    uint32_t remain_size = s_i2c[id].data_size - s_i2c[id].data_offset;
    if (!remain_size) {
        I2C_LOGD("data_size:%d tx done\r\n", s_i2c[id].data_size);
        i2c_master_stop(id);
        rtos_set_semaphore(&s_i2c[id].tx_sema);
    } else {
        i2c_master_write_byte(id, s_i2c[id].data_ptr[(s_i2c[id].data_offset)++]);
        s_i2c[id].master_status = I2C_TX_DATA;
    }
    return BK_OK;
}
#endif

static bk_err_t i2c1_master_write_data(i2c_id_t id)
{
    uint32_t data_size = 0;
    uint32_t remain_size = s_i2c[id].data_size - s_i2c[id].data_offset;
    if (!remain_size) {
        I2C_LOGD("data_size:%d tx done\r\n", s_i2c[id].data_size);
        i2c_master_stop(id);
        rtos_set_semaphore(&s_i2c[id].tx_sema);
        return BK_OK;
    }
    uint32_t empty_fifo_num = i2c_hal_get_write_empty_fifo_num(&s_i2c[id].hal);
    if (remain_size < empty_fifo_num) {
        data_size = remain_size;
    } else {
        data_size = empty_fifo_num;
    }
    for (uint32_t i = 0; i < data_size; i++) {
        i2c_master_write_byte(id, s_i2c[id].data_ptr[(s_i2c[id].data_offset)++]);
    }
    remain_size = s_i2c[id].data_size - s_i2c[id].data_offset;
    if (remain_size < empty_fifo_num) {
        I2C_LOGD("remain_size:%d, empty_fifo_num:%d\r\n", remain_size, empty_fifo_num);
        s_i2c[id].int_status &= ~(I2C1_F_INT_MODE_V | I2C1_F_START);
    }
    s_i2c[id].master_status = I2C_TX_DATA;
    return BK_OK;
}


#if (CONFIG_FM_I2C)
static bk_err_t i2c0_master_read_data(i2c_id_t id)
{
    i2c_hal_set_rx_mode(&s_i2c[id].hal);
    /* tx ack, next isr read data */
    if (i2c_hal_is_start(&s_i2c[id].hal)) {
        i2c_hal_tx_ack(&s_i2c[id].hal);
        s_i2c[id].master_status = I2C_RX_DATA;
        return BK_OK;
    }
    s_i2c[id].data_ptr[(s_i2c[id].data_offset)++] = i2c_master_read_byte(id);
    if (s_i2c[id].data_offset == s_i2c[id].data_size) {
        I2C_LOGI("data_size:%d rx done\r\n", s_i2c[id].data_size);
        /* rx data done, send NACK and stop*/
        i2c_hal_tx_non_ack(&s_i2c[id].hal);
        i2c_master_stop(id);
        rtos_set_semaphore(&s_i2c[id].rx_sema);
    } else {
        i2c_hal_tx_ack(&s_i2c[id].hal);
        s_i2c[id].master_status = I2C_RX_DATA;
    }
    return BK_OK;
}
#endif

bk_err_t i2c1_master_read_data(i2c_id_t id)
{
    if (i2c_hal_is_start(&s_i2c[id].hal)) {
        I2C_LOGD("master_read i2c_is_start\r\n");
        s_i2c[id].int_status |= I2C1_F_ACK;
        s_i2c[id].master_status = I2C_RX_DATA;
        return BK_OK;
    }
    uint32_t fifo_num = i2c_hal_get_read_fifo_num(&s_i2c[id].hal);
    for (uint32_t i = 0; i < fifo_num; i++) {
        s_i2c[id].data_ptr[(s_i2c[id].data_offset)++] = i2c_hal_read_byte(&s_i2c[id].hal);
        if (s_i2c[id].data_offset == s_i2c[id].data_size) { /* avoid index is out of array range */
            break;
        }
    }
    uint32_t remain_size = s_i2c[id].data_size - s_i2c[id].data_offset;
    if (!remain_size) {
        s_i2c[id].int_status &= ~(I2C1_F_ACK | I2C1_F_START);
        i2c_master_stop(id);
        rtos_set_semaphore(&s_i2c[id].rx_sema);
    } else if (remain_size < fifo_num) {
        s_i2c[id].int_status |= (I2C1_F_ACK | I2C1_F_INT_MODE_V);
        s_i2c[id].master_status = I2C_RX_DATA;
    } else {
        s_i2c[id].int_status |= I2C1_F_ACK;
        s_i2c[id].master_status = I2C_RX_DATA;
    }
    return BK_OK;
}

/**
 * @brief Register timer callback mechanism for I2C
 *        Attention: it is for APPLICATION to set the callback function.
 *
 * @param id I2C device ID
 * @param myTimer Pointer to a beken2_timer_t timer object that has been initialized
 *                with rtos_init_oneshot_timer(). The timer's left_arg should point
 *                to an i2c_cb_param_t structure, and the callback function should
 *                be set during timer initialization.
 *
 * @details When I2C transfer completes, the driver will fill the callback parameters
 *          in the timer's left_arg and start the timer. The callback function will be
 *          invoked when the timer expires.
 *
 * @example
 *   beken2_timer_t my_timer;
 *   i2c_cb_param_t cb_param;
 *
 *   Initialize timer with callback function
 *   rtos_init_oneshot_timer(&my_timer, 10, my_callback_func, &cb_param, app_data);
 *
 *   Register timer to I2C driver
 *   bk_i2c_timer_callback(I2C_ID_0, &my_timer);
 */
void bk_i2c_timer_callback(int id, void* myTimer)
{
    i2c_cb_ctx_t *cb_context = &i2c_cb_context[id];
    os_memset(cb_context, 0, sizeof(i2c_cb_ctx_t));

    if(NULL == myTimer){
        return;
    }
    cb_context->i2c_timer =(beken2_timer_t*) myTimer;
    cb_context->is_app_cb_configured = 1;
}

/**
 * @brief Register message queue callback mechanism for I2C
 *        Attention: it is for APPLICATION to set the callback function.
 * @note This function does NOT set a callback function. It only registers/configures
 *       the message queue that will be used for asynchronous notification.
 *       Unlike traditional callback functions, this mechanism uses message queue
 *       for communication. The application must create a task to receive messages
 *       from the queue using rtos_pop_from_queue().
 *
 * @param id I2C device ID
 * @param queue Pointer to a beken_queue_t message queue that has been initialized
 *              with rtos_init_queue(). The queue message size should be
 *              sizeof(i2c_cb_param_t).
 * @param data Application data pointer that will be included in callback messages.
 *             This pointer value will be copied to the queue messages, but the data
 *             it points to is NOT copied. The application must ensure this data
 *             remains valid until all queued messages are processed.
 *
 * @details When I2C transfer completes, the driver will push an i2c_cb_param_t
 *          message to the registered queue. The message contains:
 *          - data_len: actual data length transferred
 *          - work_mode: I2C work mode (master/slave, read/write)
 *          - app_data: the data pointer passed to this function
 *
 *          The application must have a dedicated task that continuously reads from
 *          the queue to process these messages.
 *
 * @example
 *   beken_queue_t i2c_queue;
 *   uint8_t rx_buffer[256];
 *
 *   Initialize message queue
 *   rtos_init_queue(&i2c_queue, "i2c_queue", sizeof(i2c_cb_param_t), 10);
 *
 *   Register queue to I2C driver
 *   bk_i2c_queue_callback(I2C_ID_0, &i2c_queue, rx_buffer);
 *
 *   Create task to process queue messages
 *   void i2c_handler_task(void *param) {
 *       i2c_cb_param_t msg;
 *       while (1) {
 *           if (rtos_pop_from_queue(&i2c_queue, &msg, BEKEN_WAIT_FOREVER) == kNoErr) {
 *               Process message: msg.data_len, msg.work_mode, msg.app_data
 *           }
 *       }
 *   }
 */
void bk_i2c_queue_callback(int id, beken_queue_t *queue, void *app_data)
{
    i2c_cb_ctx_t *cb_context = &i2c_cb_context[id];

    os_memset(cb_context, 0, sizeof(i2c_cb_ctx_t));
    if (NULL == queue) {
        return;
    }

    cb_context->i2c_queue = queue;
    cb_context->is_app_cb_configured = 1;
    cb_context->app_data = app_data;
}

uint8_t bk_i2c_get_busstate (int id)
{
    I2C_LOGD("bk_i2c_get_busstate[%d].\n",s_i2c[id].master_status);
    if(s_i2c[id].master_status==0){
        return 1;//idle
    } else {
        return 0;//busy
    }
}

uint8_t bk_i2c_get_transstate ( int id )
{
    return i2c_cb_context[id].transtate;
}

#if (CONFIG_I2C_PM_CB_SUPPORT)
#define I2C_PM_CHECK_RESTORE(id) do {\
    if ((id == I2C_ID_0) && bk_pm_module_lv_sleep_state_get(PM_DEV_ID_I2C1)) {\
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C0, PM_POWER_MODULE_STATE_ON);\
        i2c_pm_restore(0, (void *)id);\
        bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_I2C1); \
    } else if ((id == I2C_ID_1) && bk_pm_module_lv_sleep_state_get(PM_DEV_ID_I2C2) ) {\
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C1, PM_POWER_MODULE_STATE_ON);\
        i2c_pm_restore(0, (void *)id);\
        bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_I2C2); \
    }\
} while(0)

static int i2c_pm_backup(uint64_t sleep_time, void *args)
{
    i2c_id_t id = (i2c_id_t)args;
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);

    if (!s_i2c[id].pm_backup_is_valid)
    {
        s_i2c[id].pm_backup_is_valid = 1;
        i2c_hal_backup(&s_i2c[id].hal, &s_i2c[id].pm_backup[0]);
        i2c_clock_disable(id);
    }

    return BK_OK;
}

static int i2c_pm_restore(uint64_t sleep_time, void *args)
{
    i2c_id_t id = (i2c_id_t)args;
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);

    if (s_i2c[id].pm_backup_is_valid)
    {
        i2c_clock_enable(id);
        i2c_hal_restore(&s_i2c[id].hal, &s_i2c[id].pm_backup[0]);
        s_i2c[id].pm_backup_is_valid = 0;
    }

    return BK_OK;
}
#else
#define I2C_PM_CHECK_RESTORE(id)
#endif

bk_err_t bk_i2c_driver_init(void)
{
    if (s_i2c_driver_is_init) {
        return BK_OK;
    }

    os_memset(&s_i2c, 0, sizeof(s_i2c));
    for (int id = I2C_ID_0; id < I2C_ID_MAX; id++) {
        s_i2c[id].hal.id = id;
#if (CONFIG_I2C_PM_CB_SUPPORT)
        if (id == I2C_ID_0) {
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C0, PM_POWER_MODULE_STATE_ON);
        } else if (id == I2C_ID_1) {
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C1, PM_POWER_MODULE_STATE_ON);
        }
#endif
        i2c_hal_init(&s_i2c[id].hal);
    }
    bk_int_isr_register(INT_SRC_I2C0, i2c0_isr, NULL);
#if (SOC_I2C_UNIT_NUM > 1)
    bk_int_isr_register(INT_SRC_I2C1, i2c1_isr, NULL);
#endif
#if (SOC_I2C_UNIT_NUM > 2)
    bk_int_isr_register(INT_SRC_I2C2, i2c2_isr, NULL);
#endif
    i2c_statis_init();
    s_i2c_driver_is_init = true;

    return BK_OK;
}

bk_err_t bk_i2c_driver_deinit(void)
{
    if (!s_i2c_driver_is_init) {
        return BK_OK;
    }

    for (int id = I2C_ID_0; id < I2C_ID_MAX; id++) {
        i2c_id_deinit_common(id);
#if (CONFIG_I2C_PM_CB_SUPPORT)
        if (id == I2C_ID_0) {
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C0, PM_POWER_MODULE_STATE_OFF);
        } else if (id == I2C_ID_1) {
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C1, PM_POWER_MODULE_STATE_OFF);
        }
#endif
    }
    bk_int_isr_unregister(INT_SRC_I2C0);
#if (SOC_I2C_UNIT_NUM > 1)
    bk_int_isr_unregister(INT_SRC_I2C1);
#endif
#if (SOC_I2C_UNIT_NUM > 2)
    bk_int_isr_unregister(INT_SRC_I2C2);
#endif
    s_i2c_driver_is_init = false;

    return BK_OK;
}

bk_err_t bk_i2c_init(i2c_id_t id, const i2c_config_t *cfg)
{
    BK_RETURN_ON_NULL(cfg);
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_INVALID_ID(id);
    I2C_CHECK_SECURE(id);

#if (CONFIG_I2C_PM_CB_SUPPORT)
    pm_cb_conf_t enter_config = {i2c_pm_backup, (void *)id};
    if (id == I2C_ID_0) {
        bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_I2C1);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C0, PM_POWER_MODULE_STATE_ON);
        bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_I2C1, &enter_config, NULL);
    } else if (id == I2C_ID_1) {
        bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_I2C2);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C1, PM_POWER_MODULE_STATE_ON);
        bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_I2C2, &enter_config, NULL);
    }
#endif

#if (CONFIG_CPU_CNT > 1)
    uint32_t support_id = CONFIG_I2C_SUPPORT_ID_BITS;
    uint32_t id_init_bits = BIT(id);
    if (!(support_id & id_init_bits))
        return BK_ERR_I2C_CHECK_DEFCONFIG;
#endif
    s_i2c[id].int_status = 0;
    s_i2c[id].addr_mode = cfg->addr_mode;
    i2c_id_init_common(id);
    i2c_hal_soft_reset(&s_i2c[id].hal);
    i2c_hal_configure(&s_i2c[id].hal, cfg);
    i2c_hal_start_common(&s_i2c[id].hal);
    I2C_LOGI("I2C(%d) init ok, baud_rate:%d\r\n", id, cfg->baud_rate);
    if (cfg->src_clk == I2C_SCLK_80M)
        sys_drv_i2c_select_clock(id, I2C_SCLK_80M);
    else
        sys_drv_i2c_select_clock(id, I2C_SCLK_XTAL_40M);
    return BK_OK;
}

bk_err_t bk_i2c_deinit(i2c_id_t id)
{
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_INVALID_ID(id);

    i2c_id_deinit_common(id);
#if (CONFIG_I2C_PM_CB_SUPPORT)
    if (id == I2C_ID_0) {
        bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_I2C1, true, true);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C0, PM_POWER_MODULE_STATE_OFF);
    } else if (id == I2C_ID_1) {
        bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_I2C2, true, true);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_I2C1, PM_POWER_MODULE_STATE_OFF);
    }
#endif

    return BK_OK;
}

bk_err_t bk_i2c_master_write(i2c_id_t id, uint32_t dev_addr, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    bk_err_t ret = BK_OK;
    I2C_RETURN_ON_INVALID_ID(id);
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];

    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));
    i2c_set_timeout_limit_flag(id, timeout_ms);

    uint32_t int_level = rtos_enter_critical();
    i2c_drv_obj->err_code = BK_OK;
    i2c_drv_obj->work_mode = I2C_MASTER_WRITE;
    i2c_drv_obj->is_with_mem_addr = false;
    i2c_drv_obj->dev_addr = dev_addr;
    i2c_drv_obj->data_ptr = (uint8_t *)data;
    i2c_drv_obj->data_size = size;
    i2c_drv_obj->data_offset = 0;
    i2c_drv_obj->int_status = 0;
    i2c_master_start(id);
    i2c_master_set_write_dev_addr(id, dev_addr);
    i2c_hal_set_write_int_mode(&i2c_drv_obj->hal, size);

    i2c_hal_disable_addr_output(&i2c_drv_obj->hal);
    i2c_hal_disable_data_output(&i2c_drv_obj->hal);

    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&i2c_drv_obj->tx_sema, timeout_ms);
    if (ret != kNoErr) {
        I2C_LOGW("I2C(%d) master_write get semaphore timeout\r\n", id);
        i2c_drv_obj->err_code = BK_ERR_I2C_BUS_BUSY_TIMEOUT;
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_master_write(id, dev_addr, data, size, timeout_ms);
        }
    }

    if (!i2c_is_no_ack_mode_enabled(id) && (i2c_drv_obj->err_code == BK_ERR_I2C_ACK_TIMEOUT)) {
        I2C_LOGW("I2C(%d) master_write no ack, err=%d\r\n", id, i2c_drv_obj->err_code);
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_master_write(id, dev_addr, data, size, timeout_ms);
        }
    }

    return i2c_drv_obj->err_code;
}

bk_err_t bk_i2c_master_write_noaddr(i2c_id_t id, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    bk_err_t ret = BK_OK;
    I2C_RETURN_ON_INVALID_ID(id);
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];

    if ((data == NULL) || (size < 2)) {
        return BK_ERR_PARAM;
    }
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));
    i2c_set_timeout_limit_flag(id, timeout_ms);

    uint32_t int_level = rtos_enter_critical();
    i2c_drv_obj->err_code = BK_OK;
    i2c_drv_obj->work_mode = I2C_MASTER_WRITE;
    i2c_drv_obj->is_with_mem_addr = false;
    i2c_drv_obj->dev_addr = data[0];
    i2c_drv_obj->data_ptr = (uint8_t *)data + 1;
    i2c_drv_obj->data_size = size - 1;
    i2c_drv_obj->data_offset = 0;
    i2c_drv_obj->int_status = 0;
    i2c_master_start(id);

    // write first byte
    i2c_hal_write_byte(&i2c_drv_obj->hal, data[0]);
    i2c_hal_enable_start(&i2c_drv_obj->hal);
    i2c_drv_obj->master_status = I2C_TX_DATA;
    i2c_hal_set_write_int_mode(&i2c_drv_obj->hal, size);
    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&i2c_drv_obj->tx_sema, timeout_ms);
    if (ret != kNoErr) {
        I2C_LOGW("I2C(%d) master_write_noaddr get semaphore timeout\r\n", id);
        i2c_drv_obj->err_code = BK_ERR_I2C_BUS_BUSY_TIMEOUT;
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_noaddr_write(id, data, size, timeout_ms);
        }
    }

    if (!i2c_is_no_ack_mode_enabled(id) && (i2c_drv_obj->err_code == BK_ERR_I2C_ACK_TIMEOUT)) {
        I2C_LOGW("I2C(%d) master_write_noaddr no ack, err=%d\r\n", id, i2c_drv_obj->err_code);
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_noaddr_write(id, data, size, timeout_ms);
        }
    }

    return i2c_drv_obj->err_code;
}

bk_err_t bk_i2c_master_read(i2c_id_t id, uint32_t dev_addr, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    bk_err_t ret = BK_OK;
    I2C_RETURN_ON_INVALID_ID(id);
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];

    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));
    i2c_set_timeout_limit_flag(id, timeout_ms);

    uint32_t int_level = rtos_enter_critical();
    i2c_drv_obj->err_code = BK_OK;
    i2c_drv_obj->work_mode = I2C_MASTER_READ;
    i2c_drv_obj->is_with_mem_addr = false;
    i2c_drv_obj->dev_addr = dev_addr;
    i2c_drv_obj->data_ptr = data;
    i2c_drv_obj->data_size = size;
    i2c_drv_obj->data_offset = 0;
    i2c_drv_obj->int_status = 0;
    i2c_master_start(id);

    i2c_master_set_read_dev_addr(id, dev_addr);
    i2c_hal_set_rx_mode(&i2c_drv_obj->hal);
    i2c_hal_set_read_int_mode(&i2c_drv_obj->hal, size);
    i2c_hal_disable_addr_output(&i2c_drv_obj->hal);
    i2c_hal_disable_data_output(&i2c_drv_obj->hal);
    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&i2c_drv_obj->rx_sema, timeout_ms);
    if (ret != kNoErr) {
        I2C_LOGW("I2C(%d) master_read get semaphore timeout\r\n", id);
        i2c_drv_obj->err_code = BK_ERR_I2C_BUS_BUSY_TIMEOUT;
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_master_read(id, dev_addr, data, size, timeout_ms);
        }
    }

    return i2c_drv_obj->err_code;
}

bk_err_t bk_i2c_master_read_noaddr(i2c_id_t id, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    bk_err_t ret = BK_OK;
    I2C_RETURN_ON_INVALID_ID(id);
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];

    if ((data == NULL) || (size < 2)) {
        return BK_ERR_PARAM;
    }
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));
    i2c_set_timeout_limit_flag(id, timeout_ms);

    uint32_t int_level = rtos_enter_critical();
    i2c_drv_obj->err_code = BK_OK;
    i2c_drv_obj->work_mode = I2C_MASTER_READ;
    i2c_drv_obj->is_with_mem_addr = false;
    i2c_drv_obj->dev_addr = data[0];
    i2c_drv_obj->data_ptr = data;
    i2c_drv_obj->data_size = size;
    i2c_drv_obj->data_offset = 0;
    i2c_drv_obj->int_status = 0;
    i2c_master_start(id);

    // skip read slave address
    i2c_drv_obj->master_status = I2C_RX_DATA;
    i2c_hal_enable_start(&i2c_drv_obj->hal);
    i2c_drv_obj->int_status |= I2C1_F_START;
    i2c_hal_set_rx_mode(&i2c_drv_obj->hal);
    i2c_hal_set_read_int_mode(&i2c_drv_obj->hal, size);
    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&i2c_drv_obj->rx_sema, timeout_ms);
    if (ret != kNoErr) {
        I2C_LOGW("I2C(%d) master_read_noaddr get semaphore timeout\r\n", id);
        i2c_drv_obj->err_code = BK_ERR_I2C_BUS_BUSY_TIMEOUT;
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_noaddr_read(id, data, size, timeout_ms);
        }
    }

    return i2c_drv_obj->err_code;
}

bk_err_t bk_i2c_slave_write(i2c_id_t id, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    bk_err_t ret;
    I2C_RETURN_ON_INVALID_ID(id);
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];
    i2c_slave_data_t *slave_tx_ctx = &s_slave_tx_ctx[id];

    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));
    i2c_set_timeout_limit_flag(id, timeout_ms);

    uint32_t int_level = rtos_enter_critical();
    i2c_hal_set_write_int_mode(&s_i2c[id].hal, size);
    i2c_drv_obj->work_mode = I2C_SLAVE_WRITE;
    i2c_drv_obj->is_with_mem_addr = false;
    i2c_drv_obj->data_ptr = (uint8_t *)data;
    i2c_drv_obj->data_size = size;
    i2c_drv_obj->data_offset = 0;
    i2c_drv_obj->int_status = 0;

    slave_tx_ctx->data_ptr = (uint8_t *)data;
    slave_tx_ctx->data_size = size;
    slave_tx_ctx->data_offset = 0;
    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&i2c_drv_obj->tx_sema, timeout_ms);
    if (0 == (i2c_is_timeout_limit_flag(id)) && (ret != kNoErr)) {
        return BK_ERR_I2C_SCL_TIMEOUT;
    }

    //reset i2c to clear fifo
    i2c_hal_stop_common(&i2c_drv_obj->hal);
    i2c_hal_start_common(&i2c_drv_obj->hal);

    return BK_OK;
}

bk_err_t bk_i2c_slave_read(i2c_id_t id, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    bk_err_t ret;
    I2C_RETURN_ON_INVALID_ID(id);
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];
    i2c_slave_data_t *slave_rx_ctx = &s_slave_rx_ctx[id];

    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, timeout_ms));
    i2c_set_timeout_limit_flag(id, timeout_ms);

    uint32_t int_level = rtos_enter_critical();
    i2c_hal_set_read_int_mode(&i2c_drv_obj->hal, 1); /*double check*/
    i2c_drv_obj->work_mode = I2C_SLAVE_READ;
    i2c_drv_obj->is_with_mem_addr = false;
    i2c_drv_obj->data_ptr = data;
    i2c_drv_obj->data_size = size;
    i2c_drv_obj->data_offset = 0;
    i2c_drv_obj->int_status = 0;

    slave_rx_ctx->data_ptr = data;
    slave_rx_ctx->data_size = size;
    slave_rx_ctx->data_offset = 0;
    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&i2c_drv_obj->rx_sema, timeout_ms);
    if (0 == (i2c_is_timeout_limit_flag(id)) && (ret != kNoErr)) {
        return BK_ERR_I2C_SCL_TIMEOUT;
    }

    return BK_OK;
}

bk_err_t bk_i2c_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
    bk_err_t ret = BK_OK;
    I2C_RETURN_ON_INVALID_ID(id);
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];

    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_INVALID_ID(id);
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, mem_param->timeout_ms));

    uint32_t int_level = rtos_enter_critical();
    i2c_drv_obj->err_code = BK_OK;
    i2c_drv_obj->work_mode = I2C_MASTER_WRITE;
    i2c_drv_obj->is_with_mem_addr = true;
    i2c_drv_obj->dev_addr = mem_param->dev_addr;
    i2c_drv_obj->mem_addr = mem_param->mem_addr;
    i2c_drv_obj->mem_addr_size = mem_param->mem_addr_size;
    i2c_drv_obj->data_ptr = mem_param->data;
    i2c_drv_obj->data_size = mem_param->data_size;
    i2c_drv_obj->data_offset = 0;
    i2c_drv_obj->int_status = 0;
    i2c_master_start(id);
    i2c_master_set_write_dev_addr(id, mem_param->dev_addr);
    i2c_hal_set_write_int_mode(&i2c_drv_obj->hal, mem_param->data_size);
    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&i2c_drv_obj->tx_sema, mem_param->timeout_ms);
    if (ret != kNoErr) {
        I2C_LOGW("I2C(%d) memory_write get semaphore timeout\r\n", id);
        i2c_drv_obj->err_code = BK_ERR_I2C_BUS_BUSY_TIMEOUT;
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_memory_write(id, mem_param);
        }
    }

    if (!i2c_is_no_ack_mode_enabled(id) && (i2c_drv_obj->err_code == BK_ERR_I2C_ACK_TIMEOUT)) {
        I2C_LOGW("I2C(%d) memory_write no ack, err=%d\r\n", id, i2c_drv_obj->err_code);
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_memory_write(id, mem_param);
        }
    }

    return i2c_drv_obj->err_code;
}

bk_err_t bk_i2c_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
    bk_err_t ret = BK_OK;

    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_INVALID_ID(id);
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    BK_RETURN_ON_ERR(i2c_wait_sm_bus_idle(id, mem_param->timeout_ms));

    uint32_t int_level = rtos_enter_critical();
    s_i2c[id].err_code = BK_OK;
    s_i2c[id].work_mode = I2C_MASTER_READ;
    s_i2c[id].is_with_mem_addr = true;
    s_i2c[id].dev_addr = mem_param->dev_addr;
    s_i2c[id].mem_addr = mem_param->mem_addr;
    s_i2c[id].mem_addr_size = mem_param->mem_addr_size;
    s_i2c[id].data_ptr = mem_param->data;
    s_i2c[id].data_size = mem_param->data_size;
    s_i2c[id].data_offset = 0;
    s_i2c[id].int_status = 0;
    i2c_master_start(id);
    i2c_master_set_write_dev_addr(id, mem_param->dev_addr);
    i2c_hal_set_read_int_mode(&s_i2c[id].hal, mem_param->data_size);
    rtos_exit_critical(int_level);

    ret = rtos_get_semaphore(&s_i2c[id].rx_sema, mem_param->timeout_ms);
    if (ret != kNoErr) {
        I2C_LOGW("I2C(%d) memory_read get semaphore timeout\r\n", id);
        s_i2c[id].err_code = BK_ERR_I2C_BUS_BUSY_TIMEOUT;
        if (i2c_recover_bus(id) == BK_OK) {
            // Wait a bit more after recovery
            rtos_delay_milliseconds(2);
            i2c_retry_memory_read(id, mem_param);
        }
    }

    return s_i2c[id].err_code;
}

bk_err_t bk_i2c_set_baud_rate(i2c_id_t id, uint32_t baud_rate)
{
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_INVALID_ID(id);
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    i2c_hal_set_baud_rate(&s_i2c[id].hal, baud_rate);
    return BK_OK;
}

//i2c version must be greater than or equal to 10002
bk_err_t bk_i2c_set_no_ack_mode(i2c_id_t id, bool enable)
{
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_INVALID_ID(id);
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    i2c_hal_set_no_ack_mode(&s_i2c[id].hal, enable);
    return BK_OK;
}

bk_err_t bk_i2c_set_slave_address(i2c_id_t id, uint16_t slave_addr)
{
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_INVALID_ID(id);
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    i2c_hal_set_slave_addr(&s_i2c[id].hal, slave_addr);
    return BK_OK;
}

bk_err_t bk_i2c_enable_interrupt(i2c_id_t id)
{
    I2C_RETURN_ON_INVALID_ID(id);

    i2c_interrupt_enable(id);

    return BK_OK;
}

bk_err_t bk_i2c_disable_interrupt(i2c_id_t id)
{
    I2C_RETURN_ON_INVALID_ID(id);

    i2c_interrupt_disable(id);

    return BK_OK;
}

bool bk_i2c_is_bus_busy(i2c_id_t id)
{
    I2C_RETURN_ON_NOT_INIT();
    I2C_RETURN_ON_ID_NOT_INIT(id);
    I2C_PM_CHECK_RESTORE(id);
    return i2c_hal_is_busy(&s_i2c[id].hal);
}

uint32_t bk_i2c_get_cur_action(i2c_id_t id)
{
    return s_i2c[id].master_status;
}

static void i2c_master_isr_common(i2c_id_t id)
{
    I2C_LOGD("s_i2c[id].master_status=%d\r\n", s_i2c[id].master_status);
    switch (s_i2c[id].master_status) {
    case I2C_START:
        i2c_master_start(id);
        break;
    case I2C_TX_DEV_ADDR:
        i2c_master_set_write_dev_addr(id, s_i2c[id].dev_addr);
        break;
    case I2C_RX_DEV_ADDR:
        i2c_master_set_read_dev_addr(id, s_i2c[id].dev_addr);
        break;
    case I2C_TX_MEM_ADDR_HIGH_8BIT:
        i2c_master_write_mem_addr_high_8bit(id, s_i2c[id].mem_addr);
        break;
    case I2C_TX_MEM_ADDR_LOW_8BIT:
        i2c_master_write_mem_addr_low_8bit(id, s_i2c[id].mem_addr);
        break;
    case I2C_TX_DATA: {
        if (id == I2C_ID_0) {
#if (CONFIG_FM_I2C)
            i2c0_master_write_data(id);
#else
            i2c1_master_write_data(id);
#endif

        } else {
            i2c1_master_write_data(id);
        }
        break;
    }
    case I2C_RX_DATA: {
        if (id == I2C_ID_0) {
#if (CONFIG_FM_I2C)
            i2c0_master_read_data(id);
#else
            i2c1_master_read_data(id);
#endif
        } else {
            i2c1_master_read_data(id);
        }
        break;
    }
    case I2C_STOP:
        i2c_master_stop(id);
        break;
    default:
        s_i2c[id].master_status = I2C_IDLE;
        break;
    }
}

#if CONFIG_FM_I2C
static void  i2c0_isr_common(i2c_id_t id)
{
    i2c_hal_t *hal = &s_i2c[id].hal;
    uint32_t int_status = 0;

    int_status = i2c_hal_get_interrupt_status(hal);
    I2C_LOGD("isr_i2c0_config:0x%x\r\n", int_status);
    if(!i2c_hal_is_sm_int_triggered(hal, int_status)) {
        I2C_LOGW("sm bus int not triggered\r\n");
        return;
    }

    switch (s_i2c[id].work_mode) {
    case I2C_MASTER_WRITE: {
        if (!i2c_is_no_ack_mode_enabled(id) && !i2c_hal_is_rx_ack_triggered(hal, int_status)) {
            I2C_LOGW("i2c(%d) master_write get ack failed\r\n", id);
            i2c_transtate_set(id);
            i2c_master_stop(id);
            rtos_set_semaphore(&s_i2c[id].tx_sema);
            break;
        }
        i2c_master_isr_common(id);
        break;
    }
    case I2C_MASTER_READ: {
        if (s_i2c[id].master_status != I2C_RX_DATA &&
            !i2c_hal_is_rx_ack_triggered(hal, int_status)) {
            I2C_LOGW("i2c(%d) master_read get ack failed\r\n", id);
            i2c_transtate_set(id);
            i2c_master_stop(id);
            rtos_set_semaphore(&s_i2c[id].rx_sema);
            break;
        }
        if (i2c_hal_is_start_triggered(hal, int_status) &&
            !i2c_hal_is_rx_ack_triggered(hal, int_status)) {
            I2C_LOGW("i2c_read get ack failed\r\n");
            i2c_transtate_set(id);
            i2c_master_stop(id);
            rtos_set_semaphore(&s_i2c[id].rx_sema);
            break;
        }
        i2c_master_isr_common(id);
        break;
    }
    default:
        break;
    }
    /* must clear SI at last,otherwise slave_addr will be sent twice */
    i2c_hal_clear_interrupt_status(hal, int_status);
}
#endif

static void i2c1_isr_common(i2c_id_t id)
{
    int ret = kNoErr;
    i2c_driver_t *i2c_drv_obj = &s_i2c[id];
    i2c_hal_t *hal = &i2c_drv_obj->hal;
    uint32_t int_status = i2c_drv_obj->int_status;
    uint32_t data_num = 0;
    uint32_t remain_size = 0;

    int_status = i2c_hal_get_interrupt_status(hal);
    i2c_drv_obj->int_status = int_status;
    I2C_LOGD("isr_i2c1_status:%x\r\n", int_status);

    if (!i2c_hal_is_stop_triggered(hal, int_status)) {
        i2c_switch_slave_work_mode(id, hal);
    }

    if(!i2c_hal_is_sm_int_triggered(hal, int_status)) {
        if (i2c_hal_is_scl_timeout_triggered(hal, int_status)) {
            I2C_LOGW("SCL timeout triggered, restart i2c\r\n");
            i2c_drv_obj->master_status = I2C_IDLE;
            /* i2c must close->open when SCL low timeout triggered */
            i2c_hal_disable_scl_timeout(hal);    //disable scl_timeout interrupt to aviod error log too much
            i2c_hal_disable(hal);
            i2c_hal_enable(hal);
            i2c_drv_obj->err_code = BK_ERR_I2C_SCL_TIMEOUT;
            if (i2c_drv_obj->work_mode == I2C_MASTER_WRITE) {
                ret = rtos_set_semaphore(&i2c_drv_obj->tx_sema);
            } else if (i2c_drv_obj->work_mode == I2C_MASTER_READ) {
                ret = rtos_set_semaphore(&i2c_drv_obj->rx_sema);
            }
            if(ret != kNoErr) {
                I2C_LOGW("i2c(%d) master_write/read set semaphore failed\r\n", id);
                i2c_drv_obj->err_code = BK_ERR_I2C_SET_SEMAPHORE_FAILED;
            }
        }
        if (i2c_hal_is_arb_lost_triggered(hal, int_status)) {
            I2C_LOGW("arb lost int triggered\r\n");
        }
        return;
    }

    switch (i2c_drv_obj->work_mode) {
    case I2C_MASTER_WRITE: {
        i2c_drv_obj->int_status &= ~I2C1_F_START;
        /* send stop if not receive ack */
        if (!i2c_is_no_ack_mode_enabled(id) && !i2c_hal_is_rx_ack_triggered(hal, int_status)) {
            I2C_LOGW("i2c(%d) master_write get ack failed\r\n", id);
            i2c_transtate_set(id);
            i2c_master_stop(id);
            i2c_drv_obj->err_code = BK_ERR_I2C_ACK_TIMEOUT;
            ret = rtos_set_semaphore(&i2c_drv_obj->tx_sema);
            if(ret != kNoErr) {
                I2C_LOGW("i2c(%d) master_write set semaphore failed\r\n", id);
                i2c_drv_obj->err_code = BK_ERR_I2C_SET_SEMAPHORE_FAILED;
            }
            break;
        }
        i2c_master_isr_common(id);
        break;
    }

    case I2C_MASTER_READ: {
        i2c_drv_obj->int_status &= ~I2C1_F_START;
        if (i2c_hal_is_start_triggered(hal, int_status) &&
            !i2c_hal_is_rx_ack_triggered(hal, int_status)) {
            I2C_LOGW("i2c(%d) master_read get ack failed\r\n", id);
            i2c_drv_obj->master_status = I2C_IDLE;
            i2c_drv_obj->int_status |= I2C1_F_STOP;
            i2c_drv_obj->err_code = BK_ERR_I2C_ACK_TIMEOUT;
            ret = rtos_set_semaphore(&i2c_drv_obj->rx_sema);
            if(ret != kNoErr) {
                I2C_LOGW("i2c(%d) master_read set semaphore failed\r\n", id);
                i2c_drv_obj->err_code = BK_ERR_I2C_SET_SEMAPHORE_FAILED;
            }
            break;
        }
        i2c_master_isr_common(id);
        break;
    }

    case I2C_SLAVE_WRITE:
        if (i2c_hal_is_stop_triggered(hal, int_status) || i2c_hal_is_rx_mode(hal)) {
            I2C_LOGI("i2c(%d) slave_write get stopped\r\n", id);
            i2c_drv_obj->data_offset--;

            i2c_fill_timer_callback_param(id);
            i2c_push_queue_callback(id);
            s_slave_tx_ctx[id].data_offset = i2c_drv_obj->data_offset;
            i2c_timer_start(id);
            ret = rtos_set_semaphore(&i2c_drv_obj->tx_sema);
            if(ret != kNoErr) {
                I2C_LOGW("i2c(%d) slave_write set semaphore failed\r\n", id);
                i2c_drv_obj->err_code = BK_ERR_I2C_SET_SEMAPHORE_FAILED;
            }
            //reset i2c to clear fifo
            i2c_hal_stop_common(&i2c_drv_obj->hal);
            i2c_hal_start_common(&i2c_drv_obj->hal);
            break;
        }
        if (i2c_hal_is_addr_matched(hal)) {
            i2c_drv_obj->int_status |= I2C1_F_ACK;
        }

        i2c_hal_write_byte(hal, i2c_drv_obj->data_ptr[(i2c_drv_obj->data_offset)++]);
        if (i2c_drv_obj->data_offset == i2c_drv_obj->data_size) {
            I2C_LOGD("i2c(%d) slave_write data_offset==data_size\r\n", id);
        }

        break;

    case I2C_SLAVE_READ:
        if (i2c_hal_is_stop_triggered(hal, int_status)) {
            I2C_LOGI("i2c(%d) slave_read get stopped\r\n", id);
            i2c_fill_timer_callback_param(id);
            i2c_push_queue_callback(id);
            s_slave_rx_ctx[id].data_offset = i2c_drv_obj->data_offset;
            i2c_timer_start(id);
            ret = rtos_set_semaphore(&i2c_drv_obj->rx_sema);
            if(ret != kNoErr) {
                I2C_LOGW("i2c(%d) slave_read set semaphore failed\r\n", id);
                i2c_drv_obj->err_code = BK_ERR_I2C_SET_SEMAPHORE_FAILED;
            }
            break;
        }

        if(i2c_hal_is_receiver_ack_req(hal, int_status)){
            i2c_hal_tx_ack(hal);
        }

        if (i2c_hal_is_addr_matched(hal)) {
            i2c_drv_obj->int_status |= I2C1_F_ACK;
            break;
        }
        remain_size = i2c_drv_obj->data_size - i2c_drv_obj->data_offset;
        data_num = i2c_hal_get_read_fifo_num(hal);
        for (uint32_t i = 0; i < data_num; i++) {
            i2c_drv_obj->data_ptr[(i2c_drv_obj->data_offset)++] = i2c_hal_read_byte(hal);
            remain_size--;
            if (!remain_size) { /* avoid index is out of array range */
                break;
            }
        }
        if (!remain_size) {
            i2c_drv_obj->int_status |= I2C1_F_ACK;
        } else if (remain_size < data_num) {
            i2c_drv_obj->int_status |= (I2C1_F_ACK | I2C1_F_INT_MODE_V);
        } else {
            i2c_drv_obj->int_status |= I2C1_F_ACK;
        }
        break;

    default:
        break;
    }
    /* int_status sm_int/ack/start/stop/int_mode must operate together, otherwise will not work
     * e.g.
     * i2c_hal_enable_ack();  // only set bit of ack(bit[8]), no ack will be sent
     */
    i2c_hal_clear_interrupt_status(hal, i2c_drv_obj->int_status);
}

static void i2c0_isr(void)
{
    I2C_LOGD("enter i2c0_isr\r\n");
#if CONFIG_FM_I2C
    i2c0_isr_common(I2C_ID_0);
#else
    i2c1_isr_common(I2C_ID_0);
#endif
}

#if (SOC_I2C_UNIT_NUM  > 1)
static void i2c1_isr(void)
{
    I2C_LOGD("enter i2c1_isr\r\n");
    i2c1_isr_common(I2C_ID_1);
}
#endif

#if (SOC_I2C_UNIT_NUM  > 2)
static void i2c2_isr(void)
{
    I2C_LOGD("enter i2c2_isr\r\n");
    i2c1_isr_common(I2C_ID_2);
}
#endif

