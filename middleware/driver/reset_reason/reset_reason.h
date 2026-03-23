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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"
#include "soc/soc.h"

#define REBOOT_TAG_REQ         (0xAA55AA55)   //4 bytes

#if (CONFIG_SOC_BK7236N) || (CONFIG_SOC_BK7239XX)
	extern char __persist_mem__, __reboot_tag__;

	#define START_TYPE_ADDR        (SOC_AON_PMU_REG_BASE + (0 << 2))
	#define REBOOT_TAG_ADDR        ((uint32_t)&__reboot_tag__)
	#define PERSIST_MEMORY_ADDR    ((uint32_t)&__persist_mem__)
#elif (CONFIG_SOC_BK7236XX)
	#define START_TYPE_ADDR        (SOC_AON_PMU_REG_BASE + (3 << 2))

	/*REBOOT_TAG_ADDR For CPU0-APP set reset tag in nmi wdt reboot*/
	#define REBOOT_TAG_ADDR        (0x20003FF8 + SOC_ADDR_OFFSET)
	#define REBOOT_TAG_REQ         (0xAA55AA55)   //4 bytes
	#define PERSIST_MEMORY_ADDR    (0x20003FFC + SOC_ADDR_OFFSET)
#else
	#define     START_TYPE_ADDR        (0x0080a080)
	/* 1. For bk7231n/7236, persist memory lost after power on
	 * 2. For other platform, persist memory lost after interrupt watchdog or power on
	 * */
	#define PERSIST_MEMORY_ADDR (0x0040001c)
#endif

#define CRASH_ILLEGAL_JUMP_VALUE      0xbedead00
#define CRASH_UNDEFINED_VALUE         0xbedead01
#define CRASH_PREFETCH_ABORT_VALUE    0xbedead02
#define CRASH_DATA_ABORT_VALUE        0xbedead03
#define CRASH_UNUSED_VALUE            0xbedead04
#define POWERON_INIT_MEM_TAG         (0xaaaaaaaa)

// Wakeup source definitions from AON PMU HAL
#define WAKEUP_SOURCE_GPIO            0x1
#define WAKEUP_SOURCE_RTC             0x2
#define WAKEUP_SOURCE_TOUCH           0x10
#define WAKEUP_SOURCE_USB             0x20

/**
 * @brief Display reset reason information via log output
 * 
 * This function prints the current reset reason as a human-readable string
 * to the log system. If the reset reason is deep sleep GPIO wakeup, it also
 * displays the specific GPIO number that triggered the wakeup.
 * 
 * @note This function is mainly used for debugging and system initialization logging.
 */
extern void show_reset_reason(void);

/**
 * @brief Initialize reset reason detection module
 * 
 * This function reads the reset reason from hardware registers (AON PMU) and
 * determines the actual reset cause. It should be called once during system
 * early initialization phase.
 * 
 * The function performs the following operations:
 * - Reads hardware reset reason from AON PMU HAL
 * - Checks if the system woke up from deep sleep and identifies the wakeup source
 * - Sets the reset reason to POWERON for the next reset cycle
 * 
 * @return Reset reason code (RESET_SOURCE_STATUS enum value)
 * 
 * @note This function is idempotent - subsequent calls will return the same
 *       value without re-initialization.
 */
extern uint32_t reset_reason_init(void);

/**
 * @brief Set reboot tag value in persistent memory
 * 
 * This function writes a tag value to the reboot tag address (REBOOT_TAG_ADDR).
 * The reboot tag is used to mark the reboot reason, especially for NMI watchdog
 * reboots. The tag value persists across reboots and can be read after system
 * restart to determine the reboot cause.
 * 
 * @param tag Tag value to be written. Common values:
 *            - REBOOT_TAG_REQ (0xAA55AA55): Request reboot tag
 *            - 0: Clear reboot tag
 * 
 * @note The reboot tag address is platform-specific and defined by REBOOT_TAG_ADDR.
 *       For CPU0-APP, this is used to set reset tag in NMI WDT reboot scenarios.
 */
extern void set_reboot_tag(uint32_t tag);

/**
 * @brief Get reboot tag value from persistent memory
 * 
 * This function reads the reboot tag value from the persistent memory address
 * (REBOOT_TAG_ADDR). The reboot tag can be used to identify the reason for the
 * previous reboot, especially for NMI watchdog reboots.
 * 
 * @return Reboot tag value read from REBOOT_TAG_ADDR
 * 
 * @note The reboot tag persists across reboots and is typically cleared during
 *       system initialization after being read.
 */
extern uint32_t get_reboot_tag(void);

/**
 * @brief Set NMI (Non-Maskable Interrupt) vector handler
 * 
 * This function configures the NMI interrupt vector in the interrupt vector table.
 * NMI handlers are used for critical system events that require immediate attention,
 * such as watchdog timeouts or hardware faults.
 * 
 * @note The implementation is platform-specific and may involve relocating the
 *       vector table to SRAM or setting up the VTOR (Vector Table Offset Register).
 */
extern void set_nmi_vector(void);

/**
 * @brief Initialize persistent memory area
 * 
 * This function initializes the persistent memory area with a default value
 * (CRASH_ILLEGAL_JUMP_VALUE). The persistent memory area is used to store
 * crash information that persists across reboots.
 * 
 * @note The persistent memory address is platform-specific and defined by PERSIST_MEMORY_ADDR.
 */
extern void persist_memory_init(void);

/**
 * @brief Get value from persistent memory area
 * 
 * This function reads the current value stored in the persistent memory area.
 * The persistent memory can be used to store crash information or other data
 * that needs to persist across reboots.
 * 
 * @return Current value stored in PERSIST_MEMORY_ADDR
 */
extern uint32_t persist_memory_get(void);

/**
 * @brief Check if persistent memory content is lost
 * 
 * This function checks whether the persistent memory content has been lost.
 * The memory is considered lost if its value is not equal to CRASH_ILLEGAL_JUMP_VALUE.
 * 
 * @return true if persistent memory is lost, false otherwise
 */
extern bool persist_memory_is_lost(void);

#ifdef __cplusplus
}
#endif
