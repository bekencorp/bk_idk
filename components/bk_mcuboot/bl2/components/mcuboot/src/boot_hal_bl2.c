/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "target_cfg.h"
#include "cmsis.h"
#include "boot_hal.h"
#include "Driver_Flash.h"
#include "flash_layout.h"
#include "soc/soc.h"
#include "flash_partition.h"
#ifdef CRYPTO_HW_ACCELERATOR
#include "crypto_hw.h"
#include "bootutil/fault_injection_hardening.h"
#endif /* CRYPTO_HW_ACCELERATOR */
#include "driver/efuse.h"
#include "bootutil/bootutil_log.h"
#include "aon_pmu_ll.h"

#define EFUSE_SECURBOOT_ADDR     0
#define TAG "bl2_boot"

void bl2_secure_debug(void);

uint32_t flash_max_size;

void boot_clear_ram_area(void)
{
#if CONFIG_BL2_CLEAN_RAM_ON_EXIT
    __ASM volatile(
#if !defined(__ICCARM__)
        ".syntax unified                             \n"
#endif
        "movs    r0, #0                              \n"
#if !defined(__ICCARM__)
        "ldr     r1, =Image$$ER_DATA$$Base           \n"
        "ldr     r2, =Image$$ARM_LIB_HEAP$$ZI$$Limit \n"
#else
        "ldr     r1, =ER_DATA$$Base                  \n"
        "ldr     r2, =ARM_LIB_HEAP$$Limit            \n"
#endif
        "subs    r2, r2, r1                          \n"
        "Loop:                                       \n"
        "subs    r2, #4                              \n"
        "blt     Clear_done                          \n"
        "str     r0, [r1, r2]                        \n"
        "b       Loop                                \n"
        "Clear_done:                                 \n"
        "bx      lr                                  \n"
         : : : "r0" , "r1" , "r2" , "memory"
    );
#endif
}

/*!
 * \brief Chain-loading the next image in the boot sequence.
 *
 * This function calls the Reset_Handler of the next image in the boot sequence,
 * usually it is the secure firmware. Before passing the execution to next image
 * there is conditional rule to remove the secrets from the memory. This must be
 * done if the following conditions are satisfied:
 *  - Memory is shared between SW components at different stages of the trusted
 *    boot process.
 *  - There are secrets in the memory: KDF parameter, symmetric key,
 *    manufacturer sensitive code/data, etc.
 */
#if defined(__ICCARM__)
#pragma required = boot_clear_ram_area
#endif

__attribute__((noreturn))
void boot_jump_to_next_image(uint32_t reset_handler_addr) 
{
    __ASM volatile(
#if !defined(__ICCARM__)
        ".syntax unified                 \n"
#endif
        "mov     r7, r0                  \n"
        "bl      boot_clear_ram_area     \n" /* Clear RAM before jump */
        "movs    r0, #0                  \n" /* Clear registers: R0-R12, */
        "mov     r1, r0                  \n" /* except R7 */
        "mov     r2, r0                  \n"
        "mov     r3, r0                  \n"
        "mov     r4, r0                  \n"
        "mov     r5, r0                  \n"
        "mov     r6, r0                  \n"
        "mov     r8, r0                  \n"
        "mov     r9, r0                  \n"
        "mov     r10, r0                 \n"
        "mov     r11, r0                 \n"
        "mov     r12, r0                 \n"
        "mov     lr,  r0                 \n"
        "bx      r7                      \n" /* Jump to Reset_handler */
    );
}

void boot_platform_enable_fault_interrupt(void)
{
	BK_LOGD(TAG, "Enable CM33 Fault\r\n");
	SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
	SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
	SCB->SHCSR |= SCB_SHCSR_SECUREFAULTENA_Msk;
}

/* bootloader platform-specific hw initialization */
int32_t boot_platform_init(void)
{
    int32_t result;

    // aon_pmu_hal_back_and_clear_reset_reason();
#if CONFIG_BL2_WDT
    close_wdt();
    // update_aon_wdt(0xFFFF);
#endif

#if CONFIG_BL2_SECURE_DEBUG
    bl2_secure_debug();
#endif

    boot_platform_enable_fault_interrupt();

    flash_max_size = bk_flash_get_current_total_size();

    return 0;
}

int32_t boot_platform_post_init(void)
{
#ifdef CRYPTO_HW_ACCELERATOR
    int32_t result;

    result = crypto_hw_accelerator_init();
    if (result) {
        return 1;
    }

    (void)fih_delay_init();
#else
    dubhe_driver_init(0x4b110000);
#endif /* CRYPTO_HW_ACCELERATOR */

    return 0;
}

void boot_platform_quit(struct boot_arm_vector_table *vt)
{
    /* Clang at O0, stores variables on the stack with SP relative addressing.
     * When manually set the SP then the place of reset vector is lost.
     * Static variables are stored in 'data' or 'bss' section, change of SP has
     * no effect on them.
     */
    static struct boot_arm_vector_table *vt_cpy;
    int32_t result;

#ifdef CRYPTO_HW_ACCELERATOR
    result = crypto_hw_accelerator_finish();
    if (result) {
	BOOT_LOG_ERR("te200\r\n");
        FIH_PANIC;
    }
#endif /* CRYPTO_HW_ACCELERATOR */

    vt_cpy = vt;
    
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) \
 || defined(__ARM_ARCH_8_1M_MAIN__)
    /* Restore the Main Stack Pointer Limit register's reset value
     * before passing execution to runtime firmware to make the
     * bootloader transparent to it.
     */
    __set_MSPLIM(0);
#endif /* defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) \
       || defined(__ARM_ARCH_8_1M_MAIN__) */
    __set_MSP(vt_cpy->msp);
    __DSB();
    __ISB();

    // aon_pmu_hal_restore_reset_reason();

    boot_jump_to_next_image(vt_cpy->reset);
}

void boot_show_version(void)
{
	uint8_t bootloader_version[4];
	int ret;

	ret = bk_flash_read_bytes(BOOT_VERSION_OFFSET, bootloader_version, 4);
	if (0 == ret) {
		BOOT_LOG_FORCE("bl2 %d.%d.%d", bootloader_version[0], bootloader_version[1], (bootloader_version[2] | (bootloader_version[3] << 8)));
	} else {
		BOOT_LOG_FORCE("bl2 unknown");
	}
}
