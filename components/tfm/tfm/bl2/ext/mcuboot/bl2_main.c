/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2022 Arm Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mcuboot_config/mcuboot_config.h"
#include <assert.h>
#include "target.h"
#include "tfm_hal_device_header.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "bootutil/security_cnt.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/boot_record.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"
#include "boot_hal.h"
#include "uart_stdout.h"
#include "tfm_plat_otp.h"
#include "tfm_plat_provisioning.h"
#include "sdkconfig.h"
#include "partitions_gen.h"
#include "flash_partition.h"
#include "aon_pmu_hal.h"

#ifdef TEST_BL2
#include "mcuboot_suites.h"
#endif /* TEST_BL2 */
#include "sys_hal.h"

/* Avoids the semihosting issue */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

#ifdef MCUBOOT_ENCRYPT_RSA
#define BL2_MBEDTLS_MEM_BUF_LEN 0x3000
#else
#define BL2_MBEDTLS_MEM_BUF_LEN 0x2000
#endif

#define HDR_SZ                  0x1000

/* Static buffer to be used by mbedtls for memory allocation */
static uint8_t mbedtls_mem_buf[BL2_MBEDTLS_MEM_BUF_LEN];

extern void sys_hal_dpll_cpu_flash_time_early_init(void);

static void do_boot(struct boot_rsp *rsp)
{
    struct boot_arm_vector_table *vt;
    uintptr_t flash_base;
    int rc;

    /* The beginning of the image is the ARM vector table, containing
     * the initial stack pointer address and the reset vector
     * consecutively. Manually set the stack pointer and jump into the
     * reset vector
     */
    rc = flash_device_base(rsp->br_flash_dev_id, &flash_base);
    assert(rc == 0);

    if (rsp->br_hdr->ih_flags & IMAGE_F_RAM_LOAD) {
       /* The image has been copied to SRAM, find the vector table
        * at the load address instead of image's address in flash
        */
        vt = (struct boot_arm_vector_table *)(rsp->br_hdr->ih_load_addr +
                                         rsp->br_hdr->ih_hdr_size);
    } else {
        /* Using the flash address as not executing in SRAM */
#if CONFIG_OTA_OVERWRITE || CONFIG_DIRECT_XIP
    extern uint32_t get_flash_map_offset(uint32_t index);
    uint32_t primary_off  = get_flash_map_offset(0);
    vt = (struct boot_arm_vector_table *)(flash_base + 
                    FLASH_PHY2VIRTUAL_CODE_START(primary_off +
                                         rsp->br_hdr->ih_hdr_size / 32 * 34));
#else
    vt = (struct boot_arm_vector_table *)(flash_base + 
                    FLASH_PHY2VIRTUAL_CODE_START(rsp->br_image_off +
                                                    rsp->br_hdr->ih_hdr_size));
#endif
    }

#if MCUBOOT_LOG_LEVEL > MCUBOOT_LOG_LEVEL_OFF || TEST_BL2
#if CONFIG_BL2_CLEAN_UART_ON_EXIT
    stdio_uninit();
#endif
#endif

    /* This function never returns, because it calls the secure application
     * Reset_Handler().
     */
    boot_platform_quit(vt);
}

// Set CONFIG_DOWNLOAD_LOG to 1 to enable printf in download to debug
#define CONFIG_DOWNLOAD_LOG 0

extern uint32_t sys_is_enable_fast_boot(void);
extern uint32_t sys_is_running_from_deep_sleep(void);


static void deep_sleep_reset(void)
{
    struct boot_arm_vector_table *vt;

    if (sys_is_running_from_deep_sleep() && sys_is_enable_fast_boot()) {
        BOOT_LOG_INF("deep sleep fastboot");
        extern uint32_t get_flash_map_offset(uint32_t index);
        uint32_t phy_offset  = get_flash_map_offset(0);
        uint32_t virtual_off = FLASH_PHY2VIRTUAL(phy_offset);
        vt = (struct boot_arm_vector_table *)(FLASH_DEVICE_BASE +
                    FLASH_CEIL_ALIGN(virtual_off + HDR_SZ, CPU_VECTOR_ALIGN_SZ));
#if CONFIG_DIRECT_XIP
        uint32_t candidate_slot = 0;
        uint32_t reg = aon_pmu_ll_get_r7b();
        aon_pmu_ll_set_r0(reg);
        candidate_slot = !!(reg & BIT(2));
        extern void flash_set_excute_enable(int enable);
        flash_set_excute_enable(candidate_slot);
#endif
        boot_platform_quit(vt);
    }
}

int main(void)
{
    struct boot_rsp rsp;
    fih_int fih_rc = FIH_FAILURE;
    enum tfm_plat_err_t plat_err;


    bk_efuse_init();
#if 1//CONFIG_BL2_SECURE_DEBUG
    extern void hal_secure_debug(void);
    hal_secure_debug(); 
#endif

#if CONFIG_DOWNLOAD_LOG
    stdio_init();
#endif

    /* Perform platform specific initialization */
    if (boot_platform_init() != 0) {
        BOOT_LOG_ERR("Platform init failed");
        FIH_PANIC;
    }

#if CONFIG_BL2_DOWNLOAD
    // sys_drv_early_init();
    sys_hal_dpll_cpu_flash_time_early_init();
    if (efuse_is_secure_download_enabled()) {
        flash_switch_to_line_mode_two();
        bk_flash_cpu_write_enable();
        enable_dcache(0);
        void legacy_boot_main(void);
        legacy_boot_main();
       enable_dcache(1);
        bk_flash_cpu_write_disable();
        flash_restore_line_mode();
    }
    sys_hal_flash_set_clk(0x1);
    sys_hal_flash_set_clk_div(0x0);
    sys_hal_ctrl_vdddig_h_vol(0xD);
    sys_hal_switch_freq(0x3, 0x0, 0x0);
#endif

    close_wdt();
#if CONFIG_BL2_WDT
    update_aon_wdt(CONFIG_BL2_WDT_PERIOD);
#endif


    /* Initialise the mbedtls static memory allocator so that mbedtls allocates
     * memory from the provided static buffer instead of from the heap.
     */
    mbedtls_memory_buffer_alloc_init(mbedtls_mem_buf, BL2_MBEDTLS_MEM_BUF_LEN);

#if !CONFIG_DOWNLOAD_LOG
#if MCUBOOT_LOG_LEVEL > MCUBOOT_LOG_LEVEL_OFF || TEST_BL2
    stdio_init();
#endif
#endif
    partition_init();
    flash_map_init();
    deep_sleep_reset();
    BOOT_LOG_INF("Starting bootloader");
    dump_partition();
    dump_efuse();


    plat_err = tfm_plat_otp_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
            BOOT_LOG_ERR("OTP system initialization failed");
            FIH_PANIC;
    }

#if TFM_PROVISIONING
    if (tfm_plat_provisioning_is_required()) {
        plat_err = tfm_plat_provisioning_perform();
        if (plat_err != TFM_PLAT_ERR_SUCCESS) {
            BOOT_LOG_ERR("Provisioning failed");
            FIH_PANIC;
        }
    } else {
        tfm_plat_provisioning_check_for_dummy_keys();
    }
#endif

    FIH_CALL(boot_nv_security_counter_init, fih_rc);
    if (fih_not_eq(fih_rc, FIH_SUCCESS)) {
        BOOT_LOG_ERR("Error while initializing the security counter");
        FIH_PANIC;
    }

    /* Perform platform specific post-initialization */
    if (boot_platform_post_init() != 0) {
        BOOT_LOG_ERR("Platform post init failed");
        FIH_PANIC;
    }

#ifdef TEST_BL2
    (void)run_mcuboot_testsuite();
#endif /* TEST_BL2 */

    FIH_CALL(boot_go, fih_rc, &rsp);
    if (fih_not_eq(fih_rc, FIH_SUCCESS)) {
        BOOT_LOG_ERR("Unable to find bootable image");
        FIH_PANIC;
    }

    BOOT_LOG_INF("Bootloader chainload address offset: 0x%x",
                 rsp.br_image_off);
    BOOT_LOG_INF("Jumping to the first image slot");
    do_boot(&rsp);

    BOOT_LOG_ERR("Never should get here");
    FIH_PANIC;
}
// eof

