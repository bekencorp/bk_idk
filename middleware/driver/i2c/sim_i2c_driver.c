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

#define TRUE 1
#define FALSE 0

static gpio_id_t s_sim_i2c_sda = I2C1_LL_SDA_PIN;
static gpio_id_t s_sim_i2c_scl = I2C1_LL_SCL_PIN;

#define SIM_I2C_SCL_PIN(id)  ((id) == I2C_ID_0 ? I2C0_LL_SCL_PIN : I2C1_LL_SCL_PIN)
#define SIM_I2C_SDA_PIN(id)  ((id) == I2C_ID_0 ? I2C0_LL_SDA_PIN : I2C1_LL_SDA_PIN)

#define SIM_I2C_CLK_DELAY_100K   50
#define SIM_I2C_CLK_DELAY_200K   20
#define SIM_I2C_CLK_DELAY_400K   10
#define SIM_I2C_CLK_DELAY_DEFAULT SIM_I2C_CLK_DELAY_100K

static uint32_t s_scl_delay = SIM_I2C_CLK_DELAY_DEFAULT;

#define I2C_MIN_DELAY  1

#define SCL_DELAY       s_scl_delay
#define SCL_HALF_DELAY  ((s_scl_delay + 1) / 2)

static inline void I2cSetSdaInput(void)
{
	bk_gpio_disable_output(s_sim_i2c_sda);
	bk_gpio_enable_input(s_sim_i2c_sda);
}

static inline void I2cSetSdaOutput(void)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(s_sim_i2c_sda);
	bk_gpio_enable_output(s_sim_i2c_sda);
#else
	GPIO_UP(s_sim_i2c_sda);
#endif
}

static inline void I2cSetSclInput(void)
{
	bk_gpio_disable_output(s_sim_i2c_scl);
	bk_gpio_enable_input(s_sim_i2c_scl);
}

static inline void I2cSetSclOutput(void)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(s_sim_i2c_scl);
	bk_gpio_enable_output(s_sim_i2c_scl);
#else
	GPIO_UP(s_sim_i2c_scl);
#endif
}

static inline void I2cSetSdaHigh(void)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(s_sim_i2c_sda);
	bk_gpio_enable_output(s_sim_i2c_sda);
	bk_gpio_set_output_high(s_sim_i2c_sda);
#else
	GPIO_UP(s_sim_i2c_sda);
#endif
}

static inline void I2cSetSdaLow(void)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(s_sim_i2c_sda);
	bk_gpio_enable_output(s_sim_i2c_sda);
	bk_gpio_set_output_low(s_sim_i2c_sda);
#else
	GPIO_DOWN(s_sim_i2c_sda);
#endif
}

static inline void I2cSetSclHigh(void)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(s_sim_i2c_scl);
	bk_gpio_enable_output(s_sim_i2c_scl);
	bk_gpio_set_output_high(s_sim_i2c_scl);
#else
	GPIO_UP(s_sim_i2c_scl);
#endif
}

static inline void I2cSetSclLow(void)
{
#ifdef SIM_I2C_GPIO_API_EN
	bk_gpio_disable_input(s_sim_i2c_scl);
	bk_gpio_enable_output(s_sim_i2c_scl);
	bk_gpio_set_output_low(s_sim_i2c_scl);
#else
	GPIO_DOWN(s_sim_i2c_scl);
#endif
}

static inline uint8_t I2cReadSda(void)
{
	return (uint8_t)(bk_gpio_get_input(s_sim_i2c_sda));
}

void I2cDelay(uint32_t count)
{
	volatile uint32_t 	i;

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
	I2cSetSdaInput();
	I2cDelay(SCL_DELAY);
}

void I2cStart(void)
{
	// Just to ensure SCL is in LOW state.
//	I2cSetSclLow();
//	I2cDelay(SCL_HALF_DELAY);

	I2cSetSdaOutput();
	I2C_PIN_DELAY();

	// SDA SCL in HIGH state
	I2cSetSdaHigh();
	I2C_PIN_DELAY();

	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	// A HIGH to LOW transition on the SDA line while SCL is HIGH
	I2cSetSdaLow();
	I2cDelay(MAX(SCL_HALF_DELAY, I2C_MIN_DELAY));

	// to default state.
	I2cSetSclLow();
	I2cDelay(SCL_DELAY);
}

void I2cStop(void)
{
	// Just to ensure SCL is in LOW state.
	I2cSetSclLow();
	I2cDelay(SCL_HALF_DELAY);

	// SDA in LOW state, SCL in HIGH state
	I2cSetSdaLow();
	I2C_PIN_DELAY();

	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	// A LOW to HIGH transition on the SDA line while SCL is HIGH
	I2cSetSdaHigh();
	I2cDelay(SCL_DELAY);

	// to default state.
	/*
	I2cSetSclLow();
	I2cDelay(SCL_DELAY);
	*/ // should discard these codes for multi-master I2C bus.( or new devices of latest protocol. )
}

static inline void I2cAck(void)
{
	// Pull down SDA line.
	I2cSetSdaLow();
	I2C_PIN_DELAY();

	I2cSclPulse();
}

static inline void I2cNoAck(void)
{
	// let SDA line in HIGH state during ACK clock cycle.
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
	#define I2C_SPECIAL_DELAY()		I2cDelay(550) // special timing requirements, the delay between bytes and bytes needs to be more than 25us.
#else
	#define I2C_SPECIAL_DELAY()
#endif

void I2cInit( void )
{
	I2cSetSdaOutput();
	I2cSetSclOutput();
	//I2cSetSclLow();
	I2cStop();
}

bool I2cWaitForAck(void)
{
	bool	Ack;

	I2cSetSdaInput();
	I2C_PIN_DELAY();

	I2cSetSclHigh();
	I2cDelay(SCL_DELAY);

	Ack = (I2cReadSda() == 0);

	I2C_PIN_DELAY();

	I2cSetSclLow();
	I2C_PIN_DELAY();

	I2cDelay(SCL_DELAY);
	//Please note - the following call is required because
	//the I2cSetSdaHigh and I2cSetSdaLow functions do not set the port to output state
	//I2cSetSdaOutput();

	I2C_SPECIAL_DELAY();

	return Ack;
}

bool I2cSend(uint8_t I2cData)
{
	uint8_t	Mask;

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
			//The 9th bit of ack would exchange the output/input direction
			//Change SDA PIN as input mode in time to improve wave not normative issue
			I2cSclPulse_ack();
		} else {
			I2cSclPulse();
		}
	}

	return I2cWaitForAck();
}

uint8_t I2cReceive(bool LastOne)
{
	uint8_t		Mask;
	uint8_t		I2cData = 0;

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

	//Please note - the following call is required because
	//the I2cSetSdaHigh and I2cSetSdaLow functions do not return the port to output state
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

bool I2cWriteNoAddr(const uint8_t *pBuff, uint32_t len)
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

bool I2cReadNoAddr(uint8_t *pBuff, uint32_t len)
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

bool I2cWrite(uint8_t Addr, const uint8_t * pBuff, uint32_t len)
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

bool I2cRead(uint8_t Addr, uint8_t * pBuff, uint32_t len)
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

bool I2cMemWrite(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	uint32_t Addr;
	uint32_t mem_addr;
	uint8_t * pBuff;
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

bool I2cMemRead(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	uint32_t Addr;
	uint32_t mem_addr;
	uint8_t * pBuff;
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

bk_err_t bk_i2c_memory_write(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	I2cMemWrite(id, mem_param);
	return BK_OK;
}

bk_err_t bk_i2c_memory_read(i2c_id_t id, const i2c_mem_param_t *mem_param)
{
	I2cMemRead(id, mem_param);
	return BK_OK;
}

bk_err_t bk_sim_i2c_init(i2c_id_t id, const i2c_config_t *cfg)
{
	s_sim_i2c_sda = SIM_I2C_SDA_PIN(id);
	s_sim_i2c_scl = SIM_I2C_SCL_PIN(id);

	if (cfg && cfg->baud_rate) {
		if (cfg->baud_rate <= 100000) {
			s_scl_delay = SIM_I2C_CLK_DELAY_100K;
		} else if (cfg->baud_rate <= 200000) {
			s_scl_delay = SIM_I2C_CLK_DELAY_200K;
		} else {
			s_scl_delay = SIM_I2C_CLK_DELAY_400K;
		}
	} else {
		s_scl_delay = SIM_I2C_CLK_DELAY_DEFAULT;
	}

	I2cInit();

	return BK_OK;
}

bk_err_t bk_sim_i2c_deinit(i2c_id_t id)
{
	s_sim_i2c_sda = SIM_I2C_SDA_PIN(id);
	s_sim_i2c_scl = SIM_I2C_SCL_PIN(id);

	I2cSetSclLow();
	I2cSetSdaLow();
	return BK_OK;
}

uint8_t bk_i2c_get_busstate(int id)
{
	return 1; //idle
}

bk_err_t bk_i2c_master_write(i2c_id_t id, uint32_t dev_addr, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	I2cWrite(dev_addr, data, size);
	return BK_OK;
}

bk_err_t bk_i2c_master_read(i2c_id_t id, uint32_t dev_addr, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	I2cRead(dev_addr, data, size);
	return BK_OK;
}

bk_err_t bk_i2c_master_write_noaddr(i2c_id_t id, const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	I2cWriteNoAddr(data, size);
	return BK_OK;
}

bk_err_t bk_i2c_master_read_noaddr(i2c_id_t id, uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	I2cReadNoAddr(data, size);
	return BK_OK;
}
