// Copyright 2022-2023 Beken
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

#include <soc/soc.h>
#include <driver/efuse.h>
#include "../../../../include/driver/hal_high_freq.h"

#define HAL_EFUSE_SECURE_BOOT_DEBUG_DISABLE_BIT      (1<<1)
#define HAL_EFUSE_FAST_BOOT_DISABLE_BIT              (1<<2)
#define HAL_EFUSE_SECURE_BOOT_SUPPORTED_BIT          (1<<3)
#define HAL_EFUSE_PLL_ENABLE_BIT                     (1<<4)
#define HAL_EFUSE_FIH_DELAY_ENABLE_BIT               (1<<5)
#define HAL_EFUSE_POWER_ON_FASTBOOT_DISABLE_BIT      (1<<6)
#define HAL_EFUSE_CRITICAL_ERR_DISABLE_BIT           (1<<7)
#define HAL_EFUSE_LONG_VERSION_LENGTH_BIT            (1<<11)
#define HAL_EFUSE_SECURE_DOWNLOAD_BIT                (1<<12)
#define HAL_EFUSE_BKFIL_CHANNEL_CONTROL_OFFSET       13
#define HAL_EFUSE_BKFIL_CHANNEL_CONTROL_MASK         0x7

#define HAL_EFUSE_DISABLE_LEGACYBOOT_HIGH_FREQ_BIT   (1<<16)
#define HAL_EFUSE_SET_CPU_120M_BIT                   (1<<17)
#define HAL_EFUSE_SET_FLASH_HIGH_FREQ_BIT            (1<<18)
#define HAL_EFUSE_GPIO_FLASH_DISABLE_BIT             (1<<19)
#define HAL_EFUSE_ATTACK_NMI_ENABLE_BIT              (1<<20)
#define HAL_EFUSE_SPI_TO_AHB_DISABLE_BIT             (1<<21)
#define HAL_EFUSE_AUTO_RESET_ENABLE_0_BIT            (1<<22)
#define HAL_EFUSE_AUTO_RESET_ENABLE_1_BIT            (1<<23)
#define HAL_EFUSE_MEMCHK_BPS_ENABLE_BIT              (1<<24)
#define HAL_EFUSE_DEBUG_HW_DISABLE_BIT               (1<<25)
#define HAL_EFUSE_SHANHAI_CLK_GATING_ENABLE_BIT      (1<<26)
#define HAL_EFUSE_FLASH_NO_CRC_BIT                   (1<<27)
#define HAL_EFUSE_FLASH_AES_MODE_BIT                 (1<<28)
#define HAL_EFUSE_FLASH_AES_ENABLE_BIT               (1<<29)
#define HAL_EFUSE_SPI_DOWNLOAD_DISABLE_BIT           (1<<30)
#define HAL_EFUSE_JTAG_DISABLE_BIT                   (1<<31)


#define OTP_EFUSE_WORD0                        *((volatile unsigned long *) (SOC_MEM_CHECK_REG_BASE + 0x40*4))

#define addSYSTEM_Reg0x2                       *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x2*4))
#define addSYSTEM_Reg0x4                       *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x4*4))
#define addSYSTEM_Reg0x8                       *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x8*4))
#define addSYSTEM_Reg0x9                       *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x9*4))
#define addSYSTEM_Reg0xc                       *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0xc*4))
#define addSYSTEM_Reg0xa                       *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0xa*4))

#define addSPI0_Reg0x4                         *((volatile unsigned long *) (0x44870000+0x4*4))

#define set_SYSTEM_Reg0x9_ckdiv_flash(val)     addSYSTEM_Reg0x9 = ((addSYSTEM_Reg0x9 & (~0xC000000)) | ((val) << 26))
#define set_SYSTEM_Reg0x9_cksel_flash(val)     addSYSTEM_Reg0x9 = ((addSYSTEM_Reg0x9 & (~0x3000000)) | ((val) << 24))

#define addSYSTEM_Reg0xd                       *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0xd*4))
#define addSYSTEM_Reg0x10                      *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x10*4))
#define addSYSTEM_Reg0x40                      *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x40*4))
#define addSYSTEM_Reg0x45                      *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x45*4))
#define addSYSTEM_Reg0x46                      *((volatile unsigned long *) (SOC_SYSTEM_REG_BASE+0x46*4))
/*
 * CPU     FLASH     1ms loop cnt   1ms loop + one time reg read
 * -----  --------  -------------- -----------------------------
 * 26M    26M        3000           1700
 * 120M   60M        10000          5300
 * */

static inline void hal_loop_delay(uint32_t loop_cnt)
{
	volatile uint32_t loop = 0;
	while (loop++ < loop_cnt);
}

#define ANA_REG(id) (SOC_SYSTEM_REG_BASE + ((0x40 + id) << 2))

// Wait for maximum 60ms, CPU 26M/Flash 26M, 1ms loops about 1700 times
// 1700 * 6 = 10200
#define MAX_LOOP_CNT 102000
static inline void ana_wait_finish(uint32_t id)
{
	volatile uint32_t loop_cnt = 0;
	while ((REG_READ(SOC_SYSTEM_REG_BASE + (0x3a << 2)) & (1 << (id))) && (loop_cnt++ < MAX_LOOP_CNT));
}

static inline void ana_reg_write(uint32_t id, uint32_t v)
{
	REG_WRITE(ANA_REG(id), v);
	volatile uint32_t loop_cnt = 0;
	while ((REG_READ(SOC_SYSTEM_REG_BASE + (0x3a << 2)) & (1 << (id))) && (loop_cnt++ < MAX_LOOP_CNT));
}

static inline uint32_t ana_reg_read(uint32_t id)
{
	return REG_READ(ANA_REG(id));
}

static inline void ana_reg_or(uint32_t id, uint32_t data)
{
	REG_ANA_OR(ANA_REG(id), data);
	ana_wait_finish(id);
}

static inline void ana_reg_and(uint32_t id, uint32_t data)
{
	REG_AND(ANA_REG(id), data);
	volatile uint32_t loop_cnt = 0;
	while ((REG_READ(SOC_SYSTEM_REG_BASE + (0x3a << 2)) & (1 << (id))) && (loop_cnt++ < MAX_LOOP_CNT));
}

/* From tenglong, method to enable 7239n PLL:
  * 1.ana_reg5 set bit5
  * 2.ana_reg1 set bit14 and Set osccal_trig , elapsed time : 30us
  * 3.delay 200us
  * 4.ana_reg1 clear bit15
  * 5.ana_reg1 set bit15
  * 6.delay 200us
  * 7.ana_reg0 clear bit11
  * 8.asic sync, from Peidong
  */

void hal_enable_pll(void)
{
	// Initial register values
	uint32_t ana_reg0_val = 0x09219d00;  // sys0x40 corresponds to ana_reg_0
	uint32_t ana_reg1_val = 0xc02e3a00;  // sys0x41 corresponds to ana_reg_1
	uint32_t ana_reg5_val = 0x8407a340;  // sys0x45 corresponds to ana_reg_5

	ana_reg_write(0, ana_reg0_val);

	// 1. Enable PLL (set bit 5 of ana_reg_5)
	ana_reg5_val |= (1 << 5);  // Set bit 5
	ana_reg_write(5, ana_reg5_val);

	// 2. Enable SPI reset (set bit 14 of ana_reg_1) and Set osccal_trig (set bit 15 of ana_reg_1)
	ana_reg1_val |= (1 << 14);  // Set bit 14
	ana_reg1_val |= (1 << 15);  // Set bit 15
	ana_reg_write(1, ana_reg1_val);

	// 3. Delay for 200us
	hal_loop_delay(1000); //from peng.liu

	// 4. Clear osccal_trig (clear bit 15 of ana_reg_1) trig twice, provided by tenglong.
	ana_reg1_val &= ~(1 << 15);  // Clear bit 15
	ana_reg_write(1, ana_reg1_val);

	// 5. Set osccal_trig (set bit 15 of ana_reg_1)
	ana_reg1_val |= (1 << 15);  // Set bit 15
	ana_reg_write(1, ana_reg1_val);

	// 6. Delay for 200us
	hal_loop_delay(1000); //from peng.liu

	// 7. Disable CBEN (clear bit 11 of ana_reg_0)
	ana_reg0_val &= ~(1 << 11);  // Clear bit 11
	ana_reg_write(0, ana_reg0_val);

	// 8. Synchronize ASIC
	addSYSTEM_Reg0xc |= (1 << 20);
}

void hal_disable_pll(void)
{
}

void hal_set_spi_clock(spi_clock_t clock)
{
}

void hal_set_cpu_clock(cpu_clock_t clock)
{
	if (clock == CLOCK_PLL_CPU_120M) {
		addSYSTEM_Reg0x8 = ((addSYSTEM_Reg0x8 & (~(0xf))) | 0x03);// sel 480M PLL ; div4
		addSYSTEM_Reg0x8 = ((addSYSTEM_Reg0x8 & (~(0x30))) | 0x10);// sel 480M PLL ; div4 ,support 120M, 1st set div
	} else if (clock == CLOCK_PLL_CPU_80M) {
		addSYSTEM_Reg0x8 = ((addSYSTEM_Reg0x8 & (~(0xf))) | 0x05);// sel 480M PLL ; div6;// sel 480M PLL ; div6 ,support 80M, 1st set div
		addSYSTEM_Reg0x8 = ((addSYSTEM_Reg0x8 & (~(0x30))) | 0x10);// 2nd, set clk
	} else {
		addSYSTEM_Reg0x8 = ((addSYSTEM_Reg0x8 & (~(0xf))) | 0x00);// 1st sel 0 div : 40M
		addSYSTEM_Reg0x8 = ((addSYSTEM_Reg0x8 & (~(0x30))) | 0x00);// 2nd, set XTAL clk
	}
}

void hal_set_flash_clock(flash_clock_t clock)	////speed: 0-XTAL 26MHz, 1-PLL 60MHz
{
	if (clock == CLOCK_PLL_FLASH_80M) {
		set_SYSTEM_Reg0x9_ckdiv_flash(0x0);// flash clk div1
		set_SYSTEM_Reg0x9_cksel_flash(0x2);// sel 80M
	}
	else
	{
		/*max flash freq < 50M*/
		set_SYSTEM_Reg0x9_ckdiv_flash(0x0);// flash clk div1
		set_SYSTEM_Reg0x9_cksel_flash(0x0);// sel 40M XTAL
	}
	spi_hal_set_baud_rate();
}

void hal_enable_high_freq(void)
{
	if (!!(OTP_EFUSE_WORD0 & HAL_EFUSE_SET_CPU_120M_BIT)) {
		hal_set_cpu_clock(CLOCK_PLL_CPU_120M);
	}
	else{
		hal_set_cpu_clock(CLOCK_PLL_CPU_80M);
	}
	if (!!(OTP_EFUSE_WORD0 & HAL_EFUSE_SET_FLASH_HIGH_FREQ_BIT)) {
		hal_set_flash_clock(CLOCK_PLL_FLASH_80M);
	}
}

void spi_hal_set_baud_rate(void)
{
#if CONFIG_FORCE_ENABLE_PLL
	addSPI0_Reg0x4 &= ~(0xff00);
	addSPI0_Reg0x4 |= (1 << 8);
	addSYSTEM_Reg0xa |= (0x2 << 4); // 0:40 1:60 2:80
#else
	if (!!(OTP_EFUSE_WORD0 & HAL_EFUSE_PLL_ENABLE_BIT)) {
		addSPI0_Reg0x4 &= ~(0xff00);
		addSPI0_Reg0x4 |= (1 << 8);
		addSYSTEM_Reg0xa |= (0x2 << 4); // 0:40 1:60 2:80
	}
#endif
}

void enable_high_freq(void)
{
	hal_enable_high_freq();
}
