/*****************************************************************************
*
* General Purpose I2C Bus reference driver.
* To use this file, you must implement the first 9 functions below according
* To the specific platform IO implementation.
* To increase the speed, it is recommended to change the functions
* To inline functions or to macros.
*****************************************************************************/

#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/sim_i2c.h>
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

#define TRUE 1
#define FALSE 0

#define SIM_I2C_CLK_DELAY_100K   50
#define SIM_I2C_CLK_DELAY_200K   20
#define SIM_I2C_CLK_DELAY_400K   6
#define SIM_I2C_CLK_DELAY_DEFAULT SIM_I2C_CLK_DELAY_100K

#define I2C_MIN_DELAY  1

#define SIM_I2C_SCL_PIN(id)  ((id) == I2C_ID_0 ? I2C0_LL_SCL_PIN : I2C1_LL_SCL_PIN)
#define SIM_I2C_SDA_PIN(id)  ((id) == I2C_ID_0 ? I2C0_LL_SDA_PIN : I2C1_LL_SDA_PIN)

typedef struct {
	gpio_id_t sda;
	gpio_id_t scl;
	uint32_t scl_delay;
	beken_mutex_t mutex;
	bool initialized;
} sim_i2c_dev_t;

static sim_i2c_dev_t s_sim_i2c[I2C_ID_MAX] = {0};
static sim_i2c_dev_t *s_cur_dev = NULL;

#define SCL_DELAY       (s_cur_dev->scl_delay)
#define SCL_HALF_DELAY  ((s_cur_dev->scl_delay + 1) / 2)

static inline void I2cSetSdaInput(void)
{
	bk_gpio_disable_output(s_cur_dev->sda);
	bk_gpio_enable_input(s_cur_dev->sda);
}

static inline void I2cSetSdaOutput(void)
{
	GPIO_UP(s_cur_dev->sda);
}

static inline void I2cSetSclInput(void)
{
	bk_gpio_disable_output(s_cur_dev->scl);
	bk_gpio_enable_input(s_cur_dev->scl);
}

static inline void I2cSetSclOutput(void)
{
	GPIO_UP(s_cur_dev->scl);
}

static inline void I2cSetSdaHigh(void)
{
	GPIO_UP(s_cur_dev->sda);
}

static inline void I2cSetSdaLow(void)
{
	GPIO_DOWN(s_cur_dev->sda);
}

static inline void I2cSetSclHigh(void)
{
	GPIO_UP(s_cur_dev->scl);
}

static inline void I2cSetSclLow(void)
{
	GPIO_DOWN(s_cur_dev->scl);
}

static inline uint8_t I2cReadSda(void)
{
	return (uint8_t)(bk_gpio_get_input(s_cur_dev->sda));
}

static void I2cDelay(uint32_t count)
{
	volatile uint32_t i;

	for(i = 0; i < count; i++)
	{
	}
}

/************************************************************************************
*
* Basic I2C Bus signal.
*     START, STOP, ACK, No ACK.
* Pull down SCL line before return from any routines here.
************************************************************************************/

#define I2C_PIN_DELAY()		I2cDelay(0)

static inline void I2cSclPulse()
{
	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	I2cSetSclLow();
	I2cDelay(SCL_DELAY);
}

static inline void I2cSclPulse_ack()
{
	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	I2cSetSclLow();
	I2cDelay(SCL_DELAY);
}

static void I2cStart(void)
{
	I2cSetSdaOutput();
	I2C_PIN_DELAY();

	I2cSetSdaHigh();
	I2C_PIN_DELAY();

	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	I2cSetSdaLow();
	I2cDelay(MAX(SCL_HALF_DELAY, I2C_MIN_DELAY));

	I2cSetSclLow();
	I2cDelay(SCL_DELAY);
}

static void I2cStop(void)
{
	I2cSetSclLow();
	I2cDelay(SCL_HALF_DELAY);

	I2cSetSdaLow();
	I2C_PIN_DELAY();

	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	I2cSetSdaHigh();
	I2cDelay(SCL_DELAY);
}

static inline void I2cAck(void)
{
	I2cSetSdaLow();
	I2C_PIN_DELAY();

	I2cSclPulse();
}

static inline void I2cNoAck(void)
{
	I2cSetSdaHigh();
	I2C_PIN_DELAY();

	I2cSclPulse();
}

/****************************************************************
*
* I2C Bus Driver routines.
*
****************************************************************/
#if (CONFIG_TP_HY4633)
	#define I2C_SPECIAL_DELAY()		I2cDelay(550)
#else
	#define I2C_SPECIAL_DELAY()
#endif

static void I2cInit(void)
{
	I2cSetSdaOutput();
	I2cSetSclOutput();
	I2cStop();
}

static bool I2cWaitForAck(void)
{
	bool Ack;

	I2cSetSdaInput();
	I2C_PIN_DELAY();

	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	Ack = (I2cReadSda() == 0);

	I2C_PIN_DELAY();

	I2cSetSclLow();
	I2C_PIN_DELAY();

	I2cDelay(SCL_DELAY);

	I2C_SPECIAL_DELAY();

	return Ack;
}

static bool I2cSend(uint8_t I2cData)
{
	uint8_t Mask;

	I2cSetSdaOutput();
	for(Mask = 128; Mask; Mask >>= 1)
	{
		if(I2cData & Mask)
		{
			I2cSetSdaHigh();
		}
		else
		{
			I2cSetSdaLow();
		}

		I2C_PIN_DELAY();

		if(Mask == 1)
		{
			I2cSclPulse_ack();
		} else {
			I2cSclPulse();
		}
	}

	return I2cWaitForAck();
}

static uint8_t I2cReceive(bool LastOne)
{
	uint8_t Mask;
	uint8_t I2cData = 0;

	I2cSetSdaInput();
	I2C_PIN_DELAY();

	for(Mask = 128; Mask; Mask >>= 1)
	{
		I2cSetSclHigh();
		I2cDelay(SCL_DELAY);

		if(I2cReadSda())
		{
			I2cData |= Mask;
		}
		I2C_PIN_DELAY();

		I2cSetSclLow();
		I2cDelay(SCL_DELAY);
	}

	I2cSetSdaOutput();
	I2C_PIN_DELAY();

	if(LastOne)
	{
		I2cNoAck();
	}
	else
	{
		I2cAck();
	}

	I2C_SPECIAL_DELAY();

	return I2cData;
}

static bool I2cWriteNoAddr(const uint8_t *pBuff, uint32_t len)
{
	I2cStart();
	while(len-- > 0)
	{
		if(I2cSend(*pBuff++) == FALSE)
		{
			I2cStop();
			return FALSE;
		}
	}
	I2cStop();

	return TRUE;
}

static bool I2cReadNoAddr(uint8_t *pBuff, uint32_t len)
{
	if(len == 0)
		return FALSE;

	I2cStart();
	while(len > 0)
	{
		*pBuff++ = I2cReceive((--len) == 0);
	}
	I2cStop();

	return TRUE;
}

static bool I2cWrite(uint8_t Addr, const uint8_t *pBuff, uint32_t len)
{
	Addr <<= 1;

	I2cStart();

	if(I2cSend(Addr) == FALSE)
	{
		I2cStop();
		return FALSE;
	}

	while(len-- > 0)
	{
		if(I2cSend(*pBuff++) == FALSE)
		{
			I2cStop();
			return FALSE;
		}
	}

	I2cStop();

	return TRUE;
}

static bool I2cRead(uint8_t Addr, uint8_t *pBuff, uint32_t len)
{
	if(len == 0)
		return FALSE;

	Addr <<= 1;
	Addr |= 0x01;

	I2cStart();

	if(I2cSend(Addr) == FALSE)
	{
		I2cStop();
		return FALSE;
	}

	while(len > 0)
	{
		*pBuff++ = I2cReceive((--len) == 0);
	}

	I2cStop();

	return TRUE;
}

static bool I2cMemWrite(const i2c_mem_param_t *mem_param)
{
	uint32_t Addr;
	uint32_t mem_addr;
	uint8_t *pBuff;
	uint32_t len;
	uint32_t mem_addr_size;

	Addr = mem_param->dev_addr;
	mem_addr = mem_param->mem_addr;
	pBuff = mem_param->data;
	len = mem_param->data_size;
	mem_addr_size = mem_param->mem_addr_size;

	Addr <<= 1;

	I2cStart();

	if(I2cSend(Addr) == FALSE)
	{
		I2cStop();
		return FALSE;
	}

	if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
		if(I2cSend((mem_addr >> 8) & 0xff) == FALSE)
		{
			I2cStop();
			return FALSE;
		}
	}

	if(I2cSend((mem_addr & 0xff)) == FALSE)
	{
		I2cStop();
		return FALSE;
	}

	while(len-- > 0)
	{
		if(I2cSend(*pBuff++) == FALSE)
		{
			I2cStop();
			return FALSE;
		}
	}

	I2cStop();

	return TRUE;
}

static bool I2cMemRead(const i2c_mem_param_t *mem_param)
{
	uint32_t Addr;
	uint32_t mem_addr;
	uint8_t *pBuff;
	uint32_t len;
	uint32_t mem_addr_size;

	Addr = mem_param->dev_addr;
	mem_addr = mem_param->mem_addr;
	pBuff = mem_param->data;
	len = mem_param->data_size;
	mem_addr_size = mem_param->mem_addr_size;

	if(len == 0)
		return FALSE;

	I2cStart();

	Addr <<= 1;
	if(I2cSend(Addr) == FALSE)
	{
		I2cStop();
		return FALSE;
	}

	if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
		if(I2cSend((mem_addr >> 8) & 0xff) == FALSE)
		{
			I2cStop();
			return FALSE;
		}
	}

	if(I2cSend((mem_addr & 0xff)) == FALSE)
	{
		I2cStop();
		return FALSE;
	}

	I2cStart();
	Addr |= 0x01;

	if(I2cSend(Addr) == FALSE)
	{
		I2cStop();
		return FALSE;
	}

	while(len > 0)
	{
		*pBuff++ = I2cReceive((--len) == 0);
	}

	I2cStop();

	return TRUE;
}

static inline void sim_i2c_set_current_dev(i2c_id_t id)
{
	s_cur_dev = &s_sim_i2c[id];
}

bk_err_t bk_sim_i2c_init(i2c_id_t id, const i2c_config_t *cfg)
{
	if (id >= I2C_ID_MAX) {
		return BK_FAIL;
	}

	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	if (dev->initialized) {
		return BK_OK;
	}

	dev->sda = SIM_I2C_SDA_PIN(id);
	dev->scl = SIM_I2C_SCL_PIN(id);

	if (dev->mutex == NULL) {
		bk_err_t ret = rtos_init_mutex(&dev->mutex);
		if (ret != BK_OK) {
			return ret;
		}
	}

	if (cfg && cfg->baud_rate) {
		if (cfg->baud_rate <= 100000) {
			dev->scl_delay = SIM_I2C_CLK_DELAY_100K;
		} else if (cfg->baud_rate <= 200000) {
			dev->scl_delay = SIM_I2C_CLK_DELAY_200K;
		} else {
			dev->scl_delay = SIM_I2C_CLK_DELAY_400K;
		}
	} else {
		dev->scl_delay = SIM_I2C_CLK_DELAY_DEFAULT;
	}

	sim_i2c_set_current_dev(id);
	I2cInit();

	dev->initialized = true;

	return BK_OK;
}

bk_err_t bk_sim_i2c_deinit(i2c_id_t id)
{
	if (id >= I2C_ID_MAX) {
		return BK_FAIL;
	}

	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	if (!dev->initialized) {
		return BK_OK;
	}

	rtos_lock_mutex(&dev->mutex);

	sim_i2c_set_current_dev(id);
	I2cSetSclLow();
	I2cSetSdaLow();
	dev->initialized = false;

	rtos_unlock_mutex(&dev->mutex);

	rtos_deinit_mutex(&dev->mutex);
	dev->mutex = NULL;

	return BK_OK;
}

bk_err_t bk_sim_i2c_master_write(i2c_id_t id, uint32_t dev_addr, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	if (id >= I2C_ID_MAX || !s_sim_i2c[id].initialized) {
		return BK_ERR_I2C_NOT_INIT;
	}

	bk_err_t ret = BK_OK;
	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	rtos_lock_mutex(&dev->mutex);
	sim_i2c_set_current_dev(id);
	if (I2cWrite(dev_addr, data, size) == FALSE) {
		ret = BK_ERR_I2C_ACK_TIMEOUT;
	}
	rtos_unlock_mutex(&dev->mutex);

	return ret;
}

bk_err_t bk_sim_i2c_master_read(i2c_id_t id, uint32_t dev_addr, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	if (id >= I2C_ID_MAX || !s_sim_i2c[id].initialized) {
		return BK_ERR_I2C_NOT_INIT;
	}

	bk_err_t ret = BK_OK;
	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	rtos_lock_mutex(&dev->mutex);
	sim_i2c_set_current_dev(id);
	if (I2cRead(dev_addr, data, size) == FALSE) {
		ret = BK_ERR_I2C_ACK_TIMEOUT;
	}
	rtos_unlock_mutex(&dev->mutex);

	return ret;
}

bk_err_t bk_sim_i2c_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	if (id >= I2C_ID_MAX || !s_sim_i2c[id].initialized) {
		return BK_ERR_I2C_NOT_INIT;
	}

	bk_err_t ret = BK_OK;
	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	rtos_lock_mutex(&dev->mutex);
	sim_i2c_set_current_dev(id);
	if (I2cMemWrite(mem_param) == FALSE) {
		ret = BK_ERR_I2C_ACK_TIMEOUT;
	}
	rtos_unlock_mutex(&dev->mutex);

	return ret;
}

bk_err_t bk_sim_i2c_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	if (id >= I2C_ID_MAX || !s_sim_i2c[id].initialized) {
		return BK_ERR_I2C_NOT_INIT;
	}

	bk_err_t ret = BK_OK;
	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	rtos_lock_mutex(&dev->mutex);
	sim_i2c_set_current_dev(id);
	if (I2cMemRead(mem_param) == FALSE) {
		ret = BK_ERR_I2C_ACK_TIMEOUT;
	}
	rtos_unlock_mutex(&dev->mutex);

	return ret;
}

bk_err_t bk_sim_i2c_master_write_noaddr(i2c_id_t id, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	if (id >= I2C_ID_MAX || !s_sim_i2c[id].initialized) {
		return BK_ERR_I2C_NOT_INIT;
	}

	bk_err_t ret = BK_OK;
	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	rtos_lock_mutex(&dev->mutex);
	sim_i2c_set_current_dev(id);
	if (I2cWriteNoAddr(data, size) == FALSE) {
		ret = BK_ERR_I2C_ACK_TIMEOUT;
	}
	rtos_unlock_mutex(&dev->mutex);

	return ret;
}

bk_err_t bk_sim_i2c_master_read_noaddr(i2c_id_t id, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	if (id >= I2C_ID_MAX || !s_sim_i2c[id].initialized) {
		return BK_ERR_I2C_NOT_INIT;
	}

	bk_err_t ret = BK_OK;
	sim_i2c_dev_t *dev = &s_sim_i2c[id];

	rtos_lock_mutex(&dev->mutex);
	sim_i2c_set_current_dev(id);
	if (I2cReadNoAddr(data, size) == FALSE) {
		ret = BK_ERR_I2C_ACK_TIMEOUT;
	}
	rtos_unlock_mutex(&dev->mutex);

	return ret;
}
