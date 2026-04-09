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
#include "sdkconfig.h"
#include "partitions_gen.h"
#include "flash_partition.h"
#include "aon_pmu_hal.h"
#include <modules/pm.h>
#include <soc/soc.h>
#include "sys_persist_config.h"
#include "bootutil_priv.h"
#include <driver/flash.h>
#if CONFIG_BL2_WDT
#include <driver/wdt_hal.h>
#endif

#if (CONFIG_IOMX)
#include "iomx_ll.h"
#endif

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
#define FLASH_BASE              0x02000000

/* Static buffer to be used by mbedtls for memory allocation */
static uint8_t mbedtls_mem_buf[BL2_MBEDTLS_MEM_BUF_LEN];

extern void sys_hal_dpll_cpu_flash_time_early_init(void);

static void do_boot(struct boot_rsp *rsp)
{
    struct boot_arm_vector_table *vt;
    int rc;
    uint32_t image_off;

    if (rsp->br_hdr != NULL) {
        /* Due to hardware limitation, always use primary slot (slot0) offset
         * to calculate vector table address, regardless of which slot we're booting from
         * or which mode (XIP or overwrite) we're using.
         * The flash_set_excute_enable() call already sets the correct slot for execution.
         */
        extern uint32_t get_flash_map_offset(uint32_t index);
#if CONFIG_DIRECT_XIP || CONFIG_MIXED_OTA
        uint32_t primary_off = get_flash_map_offset(2);  /* Primary slot for image 1 (FLASH_AREA_XIP_A_ID) */
#else
        uint32_t primary_off = get_flash_map_offset(0);  /* Primary slot for image 0 (FLASH_AREA_OVERWRITE_ID) */
#endif
        image_off = primary_off;
    } else {
        BOOT_LOG_ERR("do_boot: rsp->br_hdr is NULL!");
        return;
    }

#if CONFIG_FLASH_CRC_ENABLE
    // In CRC mode, use original calculation
    vt = (struct boot_arm_vector_table *)(FLASH_BASE +
                    FLASH_PHY2VIRTUAL_CODE_START(image_off +
                                         rsp->br_hdr->ih_hdr_size / 32 * 34) + CONFIG_SYS_PERSIST_SIZE);
#else
    // In no_crc mode, vector table is at image_off + hdr_size (1:1 mapping)
    vt = (struct boot_arm_vector_table *)(FLASH_BASE + image_off + rsp->br_hdr->ih_hdr_size + CONFIG_SYS_PERSIST_SIZE);
#endif

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
    struct boot_rsp rsp;
    struct image_header fast_boot_hdr;
    const struct flash_area *fap = NULL;
    int rc;

    if (sys_is_running_from_deep_sleep() && sys_is_enable_fast_boot()) {
        BOOT_LOG_INF("deep sleep fastboot");

        memset(&rsp, 0, sizeof(rsp));

        /*
         * read image header directly from flash,
         * skip BL2 signature verification for faster wake-up.
         */
#if CONFIG_DIRECT_XIP || CONFIG_MIXED_OTA
        uint32_t running_slot = boot_read_running_slot();
        flash_set_excute_enable(running_slot);

        rc = flash_area_open(FLASH_AREA_XIP_A_ID, &fap);
#else
        rc = flash_area_open(FLASH_AREA_OVERWRITE_ID, &fap);
#endif
        if (rc != 0) {
            BOOT_LOG_ERR("fastboot: flash_area_open failed (%d)", rc);
            return;
        }

        rc = flash_area_read(fap, 0, &fast_boot_hdr, sizeof(fast_boot_hdr));
        flash_area_close(fap);
        if (rc != 0) {
            BOOT_LOG_ERR("fastboot: flash_area_read failed (%d)", rc);
            return;
        }

        if (fast_boot_hdr.ih_magic != IMAGE_MAGIC) {
            BOOT_LOG_ERR("fastboot: bad image magic 0x%x", fast_boot_hdr.ih_magic);
            return;
        }

        rsp.br_hdr = &fast_boot_hdr;
        do_boot(&rsp);
    }
}
#if CONFIG_RANDOM_AES_UPGRADE_BL2
extern uint8_t g_cur_bl2;
void *get_pc(void)
{
    void *(*funcptr)() = get_pc;

    if (funcptr > (SOC_FLASH_DATA_BASE + CONFIG_BL2_VIRTUAL_CODE_START + CONFIG_BL2_VIRTUAL_CODE_SIZE)) {
        g_cur_bl2 = 2;
    } else {
        g_cur_bl2 = 1;
    }

    return funcptr;
}
#endif

int main(void)
{
    struct boot_rsp rsp;
    fih_int fih_rc = FIH_FAILURE;
    enum tfm_plat_err_t plat_err;

#if CONFIG_EFUSE
    bk_efuse_init();
#endif

#if CONFIG_BL2_SECURE_DEBUG
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
    download_main();
#endif

#if CONFIG_BL2_WDT
    wdt_hal_force_feed();
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

#if (CONFIG_IOMX)
    hal_iomux_init();
    hal_iomux_uart_enable(IOMX_UART_TX);
#endif

    partition_init();
    flash_map_init();
    deep_sleep_reset();
    boot_show_version();
    BOOT_LOG_INF("Starting bootloader");
    dump_partition();

#if CONFIG_EFUSE
    dump_efuse();
#endif

    plat_err = tfm_plat_otp_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
            BOOT_LOG_ERR("OTP system initialization failed");
            FIH_PANIC;
    }

    /* Perform platform specific post-initialization */
    if (boot_platform_post_init() != 0) {
        BOOT_LOG_ERR("Platform post init failed");
        FIH_PANIC;
    }

    FIH_CALL(bk_boot_go, fih_rc, &rsp);
    if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
        BOOT_LOG_ERR("Unable to find bootable image");
        FIH_PANIC;
    }
    enable_dcache(0);
    sys_hal_early_deinit();
    do_boot(&rsp);

    BOOT_LOG_ERR("Never should get here");
    FIH_PANIC;
}