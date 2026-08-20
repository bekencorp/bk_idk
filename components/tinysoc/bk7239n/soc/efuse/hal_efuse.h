// Copyright 2020-2026 Beken
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <soc/soc.h>
#include <stdbool.h>
#include <stdint.h>

#define OTP_EFUSE_WORD0 \
	(*((volatile uint32_t *)(SOC_MEM_CHECK_REG_BASE + 0x40 * 4)))

#define HAL_EFUSE_SECURE_BOOT_DEBUG_DISABLE_BIT      (1 << 1)
#define HAL_EFUSE_FAST_BOOT_DISABLE_BIT              (1 << 2)
#define HAL_EFUSE_SECURE_BOOT_SUPPORTED_BIT          (1 << 3)
#define HAL_EFUSE_PLL_ENABLE_BIT                     (1 << 4)
#define HAL_EFUSE_FIH_DELAY_ENABLE_BIT               (1 << 5)
#define HAL_EFUSE_POWER_ON_FASTBOOT_DISABLE_BIT      (1 << 6)
#define HAL_EFUSE_CRITICAL_ERR_DISABLE_BIT           (1 << 7)
#define HAL_EFUSE_LONG_VERSION_LENGTH_BIT            (1 << 11)
#define HAL_EFUSE_SECURE_DOWNLOAD_BIT                (1 << 12)
#define HAL_EFUSE_POR_ENABLE_BIT                     (1 << 13)
#define HAL_EFUSE_POR_DISABLE_BIT                    (1 << 14)
#define HAL_EFUSE_VBATDETSEL_OFFSET                  (1 << 15)
#define HAL_EFUSE_VBATDETSEL_MASK                    0x3
#define HAL_EFUSE_SET_CPU_120M_BIT                   (1 << 17)
#define HAL_EFUSE_SET_FLASH_HIGH_FREQ_BIT            (1 << 18)
#define HAL_EFUSE_GPIO_FLASH_DISABLE_BIT             (1 << 19)
#define HAL_EFUSE_ATTACK_NMI_ENABLE_BIT              (1 << 20)
#define HAL_EFUSE_SPI_TO_AHB_DISABLE_BIT             (1 << 21)
#define HAL_EFUSE_AUTO_RESET_ENABLE_0_BIT            (1 << 22)
#define HAL_EFUSE_AUTO_RESET_ENABLE_1_BIT            (1 << 23)
#define HAL_EFUSE_MEMCHK_BPS_ENABLE_BIT              (1 << 24)
#define HAL_EFUSE_DEBUG_HW_DISABLE_BIT               (1 << 25)
#define HAL_EFUSE_SHANHAI_CLK_GATING_ENABLE_BIT      (1 << 26)
#define HAL_EFUSE_FLASH_NO_CRC_BIT                   (1 << 27)
#define HAL_EFUSE_FLASH_AES_MODE_BIT                 (1 << 28)
#define HAL_EFUSE_FLASH_AES_ENABLE_BIT               (1 << 29)
#define HAL_EFUSE_SPI_DOWNLOAD_DISABLE_BIT           (1 << 30)
#define HAL_EFUSE_JTAG_DISABLE_BIT                   (1 << 31)

static inline uint32_t hal_efuse_get_vbatdetsel(void)
{
	return (((OTP_EFUSE_WORD0) >> HAL_EFUSE_VBATDETSEL_OFFSET) & HAL_EFUSE_VBATDETSEL_MASK);
}

static inline bool hal_efuse_is_por_detect_enabled(void)
{
	if ((OTP_EFUSE_WORD0) & HAL_EFUSE_POR_DISABLE_BIT) {
		return false;
	}

	return !!((OTP_EFUSE_WORD0) & HAL_EFUSE_POR_ENABLE_BIT);
}
