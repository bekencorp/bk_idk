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
#include <stdlib.h>
#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>

#if CONFIG_SOC_BK7236N || CONFIG_SOC_BK7239XX
#if CONFIG_TRUSTENGINE
void arm_ce_trng_driver_init( void );
int arm_ce_seed_read( unsigned char *buf, size_t buf_len );
void arm_ce_init(void)
{
	arm_ce_trng_driver_init();
}

bool random_is_initial = false;

int bk_rand(void)
{
	if (!random_is_initial)
	{
		arm_ce_init();
		random_is_initial = true;
	}

	int number, ret;

	ret = arm_ce_seed_read((unsigned char *)&number, sizeof(number));
	if(0 != ret){
		number = 0;
	}

	return (number & RAND_MAX);
}
#else
#if defined(CONFIG_OTP_PUF) && (CONFIG_OTP_PUF)
#include <driver/otp.h>
#endif

#define TRNG_RETURN_ON_NOT_INIT() do {\
		if (!s_trng_driver_is_init) {\
			return BK_FAIL;\
		}\
	} while(0)

#define TRNG_READ_COUNT  (16)

static bool s_trng_driver_is_init = false;

#define TRNG_READ_REG(offset)       *(volatile uint32_t *)(SOC_TRNG_REG_BASE + (offset))
#define TRNG_WRITE_REG(offset, val)  (*(volatile uint32_t *)(SOC_TRNG_REG_BASE + (offset))) = (uint32_t)(val)
#define TRNG_CLKRST_REG_OFFSET       (0x02)
#define TRNG_CONFIG_REG_OFFSET       (0x04)
#define TRNG_DATA_REG_OFFSET         (0x05)

static void trng_init_common(void)
{
    uint32_t val;

    TRNG_WRITE_REG(TRNG_CLKRST_REG_OFFSET * 4, 0x1);

    val = TRNG_READ_REG(TRNG_CONFIG_REG_OFFSET * 4);
    TRNG_WRITE_REG(TRNG_CONFIG_REG_OFFSET * 4, val | 0x1);
    s_trng_driver_is_init = true;
}

static void trng_deinit_common(void)
{
        uint32_t val;

        s_trng_driver_is_init = false;
        val = TRNG_READ_REG(TRNG_CONFIG_REG_OFFSET * 4);
        TRNG_WRITE_REG(TRNG_CONFIG_REG_OFFSET * 4, val & (~0x1));
}

static uint32_t trng_get_random_number(void)
{
        uint32_t number;

        number = TRNG_READ_REG(TRNG_DATA_REG_OFFSET * 4);

        return number;
}

static bk_err_t bk_trng_start(void)
{
	trng_init_common();
	return BK_OK;
}

static bk_err_t bk_trng_stop(void)
{
	TRNG_RETURN_ON_NOT_INIT();
	trng_deinit_common();
	return BK_OK;
}

static void trng_delay_us(uint32 num)
{
	volatile uint32 i, j, us_count;

	us_count = 4;
	for (i = 0; i < num; i++){
		for (j = 0; j < us_count; j++){
                        ;
                }
        }
}

static int bk_trng_rand_impl(void)
{
	int i;
	int number;

	bk_trng_start();

	trng_delay_us(50);  //add delay to make trng disckg take effect
	/*Different board , same time point, the trng generate random number maybe same*/
	for(i = 0; i < TRNG_READ_COUNT; i++) {
		trng_get_random_number();
	}

	number = (int)trng_get_random_number();
	bk_trng_stop();

	return (number & RAND_MAX);
}

int bk_rand(void)
{
#if defined(CONFIG_OTP_PUF) && (CONFIG_OTP_PUF)
	int random_number = 0;
	bk_err_t ret = bk_otp_read_random_number((uint8_t *)&random_number, sizeof(random_number));
	if (ret == BK_OK) {
		return (random_number & RAND_MAX);
	}
#endif

	return bk_trng_rand_impl();
}

#endif /* CONFIG_MBEDTLS_ACCELERATOR && CONFIG_SPE */
#endif
// eof

