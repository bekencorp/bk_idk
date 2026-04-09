/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "soc/bk7239n/reg_base.h"
#include "partitions.h"
#include "sdkconfig.h"
#include "sys_persist_config.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

__MSP_STACK_SIZE = (4 << 10);
__MIN_HEAP_SIZE  = (40 << 10);
PERSIST_MEM_AREA_SIZE = 4;
REBOOT_TAG_AREA_SIZE = 4;

#if CONFIG_CPU0_SRAM_BASE
__CPU0_APP_RAM_BASE = CONFIG_CPU0_SRAM_BASE;
#elif CONFIG_DPD_CALI
__CPU0_APP_RAM_BASE = SOC_SRAM1_DATA_BASE; /* the first SMEM(64K) occupied by dpd-mem-tx */
#elif CONFIG_SPE
__CPU0_APP_RAM_BASE = SOC_SRAM0_DATA_BASE;
#else
__CPU0_APP_RAM_BASE = SOC_SRAM0_DATA_BASE + CONFIG_CPU0_SPE_RAM_SIZE;
#endif

#if CONFIG_DPD_CALI
__CPU0_APP_RAM_SIZE = CONFIG_CPU0_SPE_RAM_SIZE - 0x10000; /* the first SMEM(64K) occupied by dpd-mem-tx */
#elif CONFIG_SPE
__CPU0_APP_RAM_SIZE = CONFIG_CPU0_SPE_RAM_SIZE;
#else
__CPU0_APP_RAM_SIZE = 0x80000 - CONFIG_CPU0_SPE_RAM_SIZE;
#endif

#if  CONFIG_DEEP_LV
#define BOOTLOADER_IRAM_SIZE    0x1000    /* 4K for bootloader iram*/
#define BOOTROM_STACK_SIZE      0x400     /* 1K for Bootrom stack*/
__CPU0_APP_RAM_SIZE -= ( BOOTLOADER_IRAM_SIZE + BOOTROM_STACK_SIZE);/* 4K reserved for bootloader,1K reserved for Bootrom*/
#endif
/*****************************************************************************
The follow four shared memory address area mapping to the same physical memory
    0x08000000 ~ 0x080a0000  ---- instruction area, recommend put sram code
    0x18000000 ~ 0x180a0000  ---- instruction area, recommend put sram code
    0x28000000 ~ 0x280a0000  ---- data area, recommend put sram data
    0x38000000 ~ 0x380a0000  ---- data area, recommend put sram data
******************************************************************************/

/* IRAM and RAM are mapped to the SAME physical memory with different virtual
 * addresses. The CPU routes instruction fetches through IRAM address space
 * (0x08000000+) and data access through RAM address space (0x28000000+).
 * This address offset allows the processor to optimize instruction/data paths.
 */
__CPU0_APP_IRAM_OFFSET = 0x20000000;

__CPU0_APP_IRAM_BASE = __CPU0_APP_RAM_BASE - __CPU0_APP_IRAM_OFFSET;  /*0x08000000 = 0x28000000 - __CPU0_APP_IRAM_OFFSET*/
__CPU0_APP_IRAM_SIZE = __CPU0_APP_RAM_SIZE;

__CPU0_APP_VIRTUAL_CODE_START = CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_START + CONFIG_SYS_PERSIST_SIZE;
__CPU0_APP_VIRTUAL_CODE_SIZE = CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_SIZE - CONFIG_SYS_PERSIST_SIZE;

MEMORY
{
  FLASH (rx)            : ORIGIN = (SOC_FLASH_DATA_BASE + __CPU0_APP_VIRTUAL_CODE_START), LENGTH = __CPU0_APP_VIRTUAL_CODE_SIZE
  /* IRAM: Instruction RAM - used for executable code in SRAM.
   * Physical memory is the same as RAM, but accessed through different address space.
   */
  IRAM (rx)             : ORIGIN = __CPU0_APP_IRAM_BASE, LENGTH = __CPU0_APP_IRAM_SIZE
  /* RAM: Data RAM - used for data, stack and heap.
   * Physical memory is the same as IRAM, but accessed through different address space.
   */
  RAM (rwx)             : ORIGIN = __CPU0_APP_RAM_BASE, LENGTH = __CPU0_APP_RAM_SIZE

  PSRAM (rwx)           : ORIGIN = SOC_PSRAM_DATA_BASE, LENGTH = 0x4000000
}

ENTRY(Reset_Handler)

SECTIONS
{
    ASSERT((. == ALIGN(512)), "vector table address align fault.")
    .vectors :
    {
        __vector_table = .;
        KEEP(*(.vectors))
        *(.fix.reset_entry)
    } > FLASH

    .gnu.sgstubs ALIGN(32) : ALIGN(32)
    {
        *(.gnu.sgstubs*)
        . = ALIGN(32);
    } > FLASH

    .cli_cmds_table ALIGN(32) : ALIGN(32)
    {
        PROVIDE(_cli_cmds_start = .);
        KEEP(*(.cli_cmdtabl))
        KEEP(*(.cli_cmdtabl*))
        PROVIDE(_cli_cmds_end = .);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        _stext = .;
#if (CONFIG_SPE && !CONFIG_RELEASE_IRAM && !CONFIG_DPD_CALI)
	/* Exclude these files from FLASH .text/.rodata sections.
	 * They will be placed in IRAM instead (see .iram section below).
	 * CAUTION: Files listed here MUST have corresponding entries in IRAM section.
	 * If excluded but not re-added to IRAM, their rodata may not be copied, leading to runtime errors.
	 */
	EXCLUDE_FILE (
        mpu.c.obj
        tasks.c.obj
        queue.c.obj
        list.c.obj
        timers.c.obj
        port.c.obj
        rtos_pub.c.obj
        mem_arch.c.obj
        str_arch.c.obj
        io_matrix_driver.c.obj
        ckmn_driver.c.obj
        ckmn_hal.c.obj
        ana_rtc_driver.c.obj
        arch_interrupt.c.obj
        hal_machw.c.obj
        rw_task.c.obj
        ke_event.c.obj
        me_strategy.c.obj
        chan.c.obj
        vif_mgmt.c.obj
        rxl_cntrl.c.obj
        rxl_hwdesc.c.obj
        rxu_cntrl.c.obj
        mm_timer.c.obj
        ke_timer.c.obj
        co_list.c.obj
        rwnx.c.obj
        rwnx_rx.c.obj
        ps.c.obj
        td.c.obj
        mm.c.obj
        rf_cntrl.c.obj
        mcc.c.obj
        phy_karst_bk7236.c.obj
        crm.c.obj
        bk7239n_cal.c.obj
        manual_cal_bk7239n.c.obj
        bk_cal_common.c.obj
        nv_parameters.c.obj
        rc_driver.c.obj
        sys_wifi_driver.c.obj
        bk_feature.c.obj
        bk_pm_core.c.obj
        bk_pm_clock.c.obj
        bk_pm_debug.c.obj
        bk_pm_misc.c.obj
        bk_pm_power.c.obj
        bk_pm_sleep.c.obj
        bk_pm_wakeup_source.c.obj
        bk_pm.c.obj
        sys_ps_driver.c.obj
        sys_pm_hal.c.obj
        bk_wifi_adapter.c.obj
        net_param.c.obj
        wifi_api_wrapper.c.obj
        bk_rf_adapter.c.obj
        bk_phy_adapter.c.obj
        bk_wifi_phy_api.c.obj
        uart_driver.c.obj
        uart_hal.c.obj
#if CONFIG_WIFI_NON_STANDARD
        txu_cntrl.c.obj
        txl_cntrl.c.obj
#endif
        _aeabi_ldivmod.o
        _aeabi_uldivmod.o
        _arm_cmpsf2.o
        _arm_cmpdf2.o
        _udivmoddi4.o
        _arm_addsubsf3.o
        _arm_muldivsf3.o
        _fixunssfdi.o
        _fixunsdfdi.o
        _fixdfdi.o
        _arm_muldivdf3.o
        _arm_addsubdf3.o
        _arm_fixunsdfsi.o
        _arm_fixunssfsi.o
        _arm_fixdfsi.o
        lib_a-memcpy.o
        lib_a-memset.o
        lib_a-strcpy.o
        lib_a-strncpy.o
        lib_a-strlen-stub.o
        lib_a-strncmp.o
	) *(.text .text.* .rodata .rodata.*)
#else
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
#endif
        . = ALIGN(4);
        __devconfig_start = .;
        *(".devconfig.*")
        KEEP(*(SORT_BY_NAME(".devconfig*")))
        __devconfig_end = .;

        . = ALIGN(4);
        __apps_start = .;
        KEEP (*(.apps_data))
        __apps_end = .;
        . = ALIGN(4);
        __wifi_dp_code_start = .;
        *libiperf.a:*(.text*)
        *liblwip_intf_v2_1.a:*(.text*)
        *libbk_wifi_driver.a:*(.text*)
        *libbk_wifi.a:*(.text*)
        __wifi_dp_code_end = .;
        _etext = .;
        . = ALIGN(4);
    } > FLASH

    .a_device_null :
    {
      KEEP(*(.a_deviceobj_null))
    } > FLASH

    .a_devices :
    {
      __device_start = .;
      KEEP(*(.a_deviceobj_*))
      __device_end = .;
    } > FLASH

    .a_init_entries :
    {
      __a_init_start = .;
      KEEP(*(.a_init_entry_*))
      __a_init_end = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    /* Copy table: Defines sections to copy from Flash to RAM at boot.
     * Order in this table determines the boot-time copy sequence.
     * Critical sections (like iram) are copied first.
     */
    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__iram_flash_begin)
        LONG (__iram_start__)
        LONG ((__iram_end__ - __iram_start__) / 4)

        LONG (__data_flash_begin)
        LONG (__data_start__)
        LONG ((__data_end__ - __data_start__) / 4)

        LONG (__video_cache_text)
        LONG (__video_cache_data_start__)
        LONG ((__video_cache_data_end__ - __video_cache_data_start__) / 4)
        __copy_table_end__ = .;
    } > FLASH

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (_bss_start)
        LONG ((_bss_end - _bss_start) / 4)
        LONG (_heap_start)
        LONG ((_heap_end - _heap_start) / 4)

        /* Add each additional bss section here */
        LONG (__video_cache_bss_start__)
        LONG ((__video_cache_bss_end__ - __video_cache_bss_start__) / 4)
        __zero_table_end__ = .;
    } > FLASH

    .iram :
    {
        . = ALIGN(512);
        PROVIDE(__iram_flash_begin = LOADADDR(.iram));
        __iram_start__ = .;

        . = ALIGN(512);
        __vector_iram_table = .;
        KEEP(*(.vectors_iram))
        . = ALIGN(512);

        *(.itcm)
        *(.itcm_sec_code)
        *(.iram)
        *(.interrupt)

#if (CONFIG_SPE && !CONFIG_RELEASE_IRAM)
        /* Re-add files that were excluded from FLASH .text section above.
         * These files are placed in IRAM for better performance.
         * WARNING: Use wildcards carefully - mismatched patterns may cause rodata not to be copied.
         */
        mpu.c.obj*(.text .text.* *.rodata *.rodata.*)
        tasks.c.obj*(.text .text.* *.rodata *.rodata.*)
        queue.c.obj*(.text .text.* *.rodata *.rodata.*)
        list.c.obj*(.text .text.* *.rodata *.rodata.*)
        timers.c.obj*(.text .text.* *.rodata *.rodata.*)
        port.c.obj*(.text .text.* *.rodata *.rodata.*)
        rtos_pub.c.obj*(.text .text.* *.rodata *.rodata.*)
        mem_arch.c.obj*(.text .text.* *.rodata *.rodata.*)
        str_arch.c.obj*(.text .text.* *.rodata *.rodata.*)
        io_matrix_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        ckmn_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        ckmn_hal.c.obj*(.text .text.* *.rodata *.rodata.*)
        ana_rtc_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        arch_interrupt.c.obj*(.text .text.* *.rodata *.rodata.*)
        hal_machw.c.obj*(.text .text.* *.rodata *.rodata.*)
        rw_task.c.obj*(.text .text.* *.rodata *.rodata.*)
        ke_event.c.obj*(.text .text.* *.rodata *.rodata.*)
        me_strategy.c.obj*(.text .text.* *.rodata *.rodata.*)
        chan.c.obj*(.text .text.* *.rodata *.rodata.*)
        vif_mgmt.c.obj*(.text .text.* *.rodata *.rodata.*)
        rxl_cntrl.c.obj*(.text .text.* *.rodata *.rodata.*)
        rxl_hwdesc.c.obj*(.text .text.* *.rodata *.rodata.*)
        rxu_cntrl.c.obj*(.text .text.* *.rodata *.rodata.*)
        mm_timer.c.obj*(.text .text.* *.rodata *.rodata.*)
        ke_timer.c.obj*(.text .text.* *.rodata *.rodata.*)
        co_list.c.obj*(.text .text.* *.rodata *.rodata.*)
        rwnx.c.obj*(.text .text.* *.rodata *.rodata.*)
        rwnx_rx.c.obj*(.text .text.* *.rodata *.rodata.*)
        ps.c.obj*(.text .text.* *.rodata *.rodata.*)
        td.c.obj*(.text .text.* *.rodata *.rodata.*)
        mm.c.obj*(.text .text.* *.rodata *.rodata.*)
        rf_cntrl.c.obj*(.text .text.* *.rodata *.rodata.*)
        mcc.c.obj*(.text .text.* *.rodata *.rodata.*)
        phy_karst_bk7236.c.obj*(.text .text.* *.rodata *.rodata.*)
        crm.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk7239n_cal.c.obj*(.text .text.* *.rodata *.rodata.*)
        manual_cal_bk7239n.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_cal_common.c.obj*(.text .text.* *.rodata *.rodata.*)
        nv_parameters.c.obj*(.text .text.* *.rodata *.rodata.*)
        rc_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        sys_wifi_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_feature.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm_core.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm_clock.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm_debug.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm_misc.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm_power.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm_sleep.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm_wakeup_source.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_pm.c.obj*(.text .text.* *.rodata *.rodata.*)
        sys_ps_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        sys_pm_hal.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_wifi_adapter.c.obj*(.text .text.* *.rodata *.rodata.*)
        net_param.c.obj*(.text .text.* *.rodata *.rodata.*)
        wifi_api_wrapper.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_rf_adapter.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_phy_adapter.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_wifi_phy_api.c.obj*(.text .text.* *.rodata *.rodata.*)
        uart_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        uart_hal.c.obj*(.text .text.* *.rodata *.rodata.*)
#if CONFIG_WIFI_NON_STANDARD
        txu_cntrl.c.obj*(.text .text.* *.rodata *.rodata.*)
        txl_cntrl.c.obj*(.text .text.* *.rodata *.rodata.*)
#endif
        _aeabi_ldivmod.o*(.text .text.* *.rodata *.rodata.*)
        _aeabi_uldivmod.o*(.text .text.* *.rodata *.rodata.*)
        _arm_cmpsf2.o*(.text .text.* *.rodata *.rodata.*)
        _arm_cmpdf2.o*(.text .text.* *.rodata *.rodata.*)
        _udivmoddi4.o*(.text .text.* *.rodata *.rodata.*)
        _arm_addsubsf3.o*(.text .text.* *.rodata *.rodata.*)
        _arm_muldivsf3.o*(.text .text.* *.rodata *.rodata.*)
        _fixunssfdi.o*(.text .text.* *.rodata *.rodata.*)
        _fixunsdfdi.o*(.text .text.* *.rodata *.rodata.*)
        _fixdfdi.o*(.text .text.* *.rodata *.rodata.*)
        _arm_muldivdf3.o*(.text .text.* *.rodata *.rodata.*)
        _arm_addsubdf3.o*(.text .text.* *.rodata *.rodata.*)
        _arm_fixunsdfsi.o*(.text .text.* *.rodata *.rodata.*)
        _arm_fixunssfsi.o*(.text .text.* *.rodata *.rodata.*)
        _arm_fixdfsi.o*(.text .text.* *.rodata *.rodata.*)
        lib_a-memcpy.o*(.text .text.* *.rodata *.rodata.*)
        lib_a-memset.o*(.text .text.* *.rodata *.rodata.*)
        lib_a-strcpy.o*(.text .text.* *.rodata *.rodata.*)
        lib_a-strncpy.o*(.text .text.* *.rodata *.rodata.*)
        lib_a-strlen-stub.o*(.text .text.* *.rodata *.rodata.*)
        lib_a-strncmp.o*(.text .text.* *.rodata *.rodata.*)
#endif
        . = ALIGN(4);
        _iram_end_dummy_ = .;
    } > IRAM AT > FLASH

    /* workaround, unresolved issue:
     *           how to reproduce the issue? disable the macro:CONFIG_CLI;
     *           there are still many rodata symbols after the address:__iram_end__(_iram_end_dummy_), and then these rodata symbols cannot
     *           be initialized.
     */
    .iram_boundary :
    {
        __iram_end__ = .;
        . += 4;
        . = ALIGN(4);
    } > IRAM AT > FLASH

    /* Reserve space in RAM that matches the size of IRAM section.
     * This ensures that subsequent data sections won't overlap with code
     * that may be copied to RAM at runtime. The alignment to 16 bytes
     * follows ARM AAPCS (Procedure Call Standard) requirements for
     * optimal memory access performance.
     */
    .ramcode :
    {
        . = . +  SIZEOF(.iram);
        . = ALIGN(16);
    } > RAM AT > FLASH

    .data :
    {
        PROVIDE(__etext = LOADADDR(.data));
        PROVIDE(__data_flash_begin = LOADADDR(.data));

        . = ALIGN(4);
        __data_start__ = .;

#ifdef CONFIG_GCOV
        /* added in template for gcov: */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
#endif

        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)
        *(.data)
        *(".data.*")
        *(.sdata)

#if CONFIG_CUSTOMIZE_TY
        *(*ts2.data)
        *(*ts2.data.*)
        *(*ts2.sdata)
#endif

        *(.video_spec_data*)
        *(.gnu.linkonce.d*)
        __data_end__ = .;
    } > RAM AT > FLASH

    _data_flash_begin = __data_flash_begin;
    _data_ram_begin = __data_start__;
    _data_ram_end = .;

    /* Estimate of total Flash memory usage.
     * LOADADDR(.data) contains everything before .data in the link script:
     *   - .vectors (interrupt vector table)
     *   - .gnu.sgstubs (secure gateway stubs)
     *   - .cli_cmds_table
     *   - .text (all code and rodata from FLASH, excluding IRAM)
     *   - .a_device_null, .a_devices, .a_init_entries
     *   - .ARM.extab, .ARM.exidx (exception unwinding data)
     *   - .copy.table and .zero.table
     *   - .iram (instruction RAM section, placed BEFORE .data in FLASH image)
     *   - .iram_boundary
     *   - .ramcode (reserved space in RAM for IRAM code)
     * This value is approximate because sections defined after .data that also use
     * AT > FLASH (such as .video.cache.data) are not included in the calculation.
     */
    s_flash_end = LOADADDR(.data) + SIZEOF(.data);

    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        *(".uninitialized.*")
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM

    .PERSIST_MEM : ALIGN(4)
    {
        __persist_mem__ = .;
        . += PERSIST_MEM_AREA_SIZE;
    } > RAM

    .REBOOT_TAG : ALIGN(4)
    {
        __reboot_tag__ = .;
        . += REBOOT_TAG_AREA_SIZE;
    } > RAM

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss*)
        *(COMMON)

#if CONFIG_CUSTOMIZE_TY
        *(*ts2.bss)
        *(*ts2.bss.*)
#endif

        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    bss_size = _bss_end - _bss_start;

    _msp_total_size = __MSP_STACK_SIZE;

    .heap (COPY) :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + (ORIGIN(RAM) + LENGTH(RAM) - _msp_total_size - _heap_start - 8); /* 16 bytes for boundary protection */
        . = ALIGN(8);
        _heap_end = .;
    } > RAM

/*multiple cores require independent stack spaces*/
    .stack (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack = .;
        __StackLimit = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTop = .;
        _estack = .;
    } > RAM

    .video.cache.data :
    {
        . = ALIGN(4);
        PROVIDE(__video_cache_text = LOADADDR(.video.cache.data));
        __video_cache_data_start__ = .;

        *(.video_cache_data)
        *(.video_cache_data*)

        . = ALIGN(4);
        __video_cache_data_end__ = .;
    } > PSRAM AT > FLASH

    .video_cache_bss (NOLOAD):
    {
        . = ALIGN(4);
        __video_cache_bss_start__ = .;
        *(.video_cache_bss)
        *(.video_cache_bss*)
        . = ALIGN(4);
        __video_cache_bss_end__ = .;
    } > PSRAM

#if CONFIG_CUSTOMIZE_TY
    .ty_null_text_section :
    {

    } > FLASH

    PROVIDE(__ty_null_text_section = LOADADDR(.ty_null_text_section));
    . = __ty_null_text_section + SIZEOF(.ty_null_text_section);
    . = ALIGN(0x8);
    .ty_text_section ALIGN(0x1000) :
    {
        _ty_section_start = .;

        /* this is for c++ global constructor init */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(*.init_array.*)))
        KEEP (*(*.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);

        _ty_section_caa_end = .;
        *libCHIP.a:*(.text .text.*)
        *(*ts2.text *ts2.text.*)
        _ty_section_text_end = .;
        *libCHIP.a:*(.rodata .rodata.*)
        *(*ts2.rodata *ts2.rodata.*)
        _ty_section_end = .;
    } >FLASH
    ty_section_size = SIZEOF(.ty_text_section);
#endif

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - __MSP_STACK_SIZE;

    ASSERT((s_flash_end < ORIGIN(FLASH) + LENGTH(FLASH)), "Flash Overflow!!!")
    ASSERT(((ORIGIN(RAM) + LENGTH(RAM)) > (_heap_start + __MSP_STACK_SIZE + 8)), "Stack overflowed with bss")
    ASSERT(((_heap_end - _heap_start) >= __MIN_HEAP_SIZE), "Heap smaller than minimize size 40K!!!")
}
//eof

