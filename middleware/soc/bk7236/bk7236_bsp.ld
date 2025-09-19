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

#include "soc/bk7236/reg_base.h"
#include "partitions.h"
#include "sdkconfig.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

__MSP_STACK_SIZE = (4 << 10);
__MIN_HEAP_SIZE  = (90 << 10);
__MPU_PROTECT_SIZE = 0x0;
__SWAP_SIZE = (0);

#if CONFIG_CPU0_SRAM_BASE
__CPU0_APP_RAM_BASE = CONFIG_CPU0_SRAM_BASE;
#elif CONFIG_SPE
__CPU0_APP_RAM_BASE = SOC_SRAM0_DATA_BASE;
#else
__CPU0_APP_RAM_BASE = SOC_SRAM0_DATA_BASE + CONFIG_CPU0_SPE_RAM_SIZE;
#endif

#if CONFIG_SPE
__CPU0_APP_RAM_SIZE = CONFIG_CPU0_SPE_RAM_SIZE;
#else
__CPU0_APP_RAM_SIZE = 0xA0000 - CONFIG_CPU0_SPE_RAM_SIZE;
#endif

/*****************************************************************************
The follow four shared memory address area mapping to the same physical memory 
    0x08000000 ~ 0x080a0000  ---- instruction area, recommend put sram code  
    0x18000000 ~ 0x180a0000  ---- instruction area, recommend put sram code 
    0x28000000 ~ 0x280a0000  ---- data area, recommend put sram data 
    0x38000000 ~ 0x380a0000  ---- data area, recommend put sram data 
******************************************************************************/
__CPU0_APP_IRAM_OFFSET = 0x20000000;

__CPU0_APP_IRAM_BASE = __CPU0_APP_RAM_BASE - __CPU0_APP_IRAM_OFFSET;  /*0x08000000 = 0x28000000 - __CPU0_APP_IRAM_OFFSET*/
__CPU0_APP_IRAM_SIZE = __CPU0_APP_RAM_SIZE;

__CPU0_APP_VIRTUAL_CODE_START = CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_START;
__CPU0_APP_VIRTUAL_CODE_SIZE = CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_SIZE;

MEMORY
{
  FLASH (rx)            : ORIGIN = (SOC_FLASH_DATA_BASE + __CPU0_APP_VIRTUAL_CODE_START), LENGTH = __CPU0_APP_VIRTUAL_CODE_SIZE
  IRAM (rx)             : ORIGIN = __CPU0_APP_IRAM_BASE, LENGTH = __CPU0_APP_IRAM_SIZE
  RAM (rwx)             : ORIGIN = __CPU0_APP_RAM_BASE, LENGTH = __CPU0_APP_RAM_SIZE - __SWAP_SIZE
  SWAP (rwx)            : ORIGIN = __CPU0_APP_RAM_BASE + __CPU0_APP_RAM_SIZE - __SWAP_SIZE, LENGTH = __SWAP_SIZE

  ITCM (rwx)            : ORIGIN = SOC_ITCM_DATA_BASE + __MPU_PROTECT_SIZE, LENGTH = CONFIG_ITCM_SIZE - __MPU_PROTECT_SIZE
  DTCM (rwx)            : ORIGIN = SOC_DTCM_DATA_BASE + 0, LENGTH = CONFIG_DTCM_SIZE
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

    .text :
    {
        . = ALIGN(4);
        _stext = .;

#if (CONFIG_SPE && !CONFIG_RELEASE_IRAM)
	EXCLUDE_FILE (
        libos_source.a:tasks.c.obj
        libos_source.a:queue.c.obj
        libos_source.a:list.c.obj
        libos_source.a:timers.c.obj
        libos_source.a:port.c.obj
        rtos_pub.c.obj
        heap_4.c.obj
        mem_arch.c.obj
        gpio_driver_base.c.obj
        flash_driver.c.obj
        ckmn_driver.c.obj
        arch_interrupt.c.obj
        hal_machw.c.obj
        interrupts.c.obj
        rw_task.c.obj
        ke_event.c.obj
        me_strategy.c.obj
        chan.c.obj
        vif_mgmt.c.obj
        rxl_cntrl.c.obj
        rxl_hwdesc.c.obj
        mm_time.c.obj
        rwnx.c.obj
        rwnx_rx.c.obj
        libwifi.a:ps.c.obj
        libwifi.a:mm.c.obj
        rf_cntrl.c.obj
        mcc.c.obj
        phy_karst_bk7236.c.obj
        crm.c.obj
        bk7236_cal.c.obj
        manual_cal_bk7236.c.obj
        bk_cal_common.c.obj
        nv_parameters.c.obj
        macif_fhost.c.obj
        rc_driver.c.obj
        sys_wifi_driver.c.obj
        wifi_v2.c.obj
        bk_feature.c.obj
        libbk_pm.a:pm.c.obj
        sys_ps_driver.c.obj
        sys_pm_hal.c.obj
        bk_wifi_adapter.c.obj
        net_param.c.obj
        wifi_api_wrapper.c.obj
        bk_rf_adapter.c.obj
        bk_phy_adapter.c.obj
        shell_task.c.obj
        shell_uart.c.obj
        uart_driver.c.obj
        uart_hal.c.obj
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

#if ((!defined(CONFIG_SOC_SMP)) || (0 == CONFIG_SOC_SMP))
        LONG (__itcm_text)
        LONG (__itcm_start__)
        LONG ((__itcm_end__ - __itcm_start__) / 4)

        LONG (__dtcm_content)
        LONG (__dtcm_start__)
        LONG ((__dtcm_end__ - __dtcm_start__) / 4)
#endif

#if CONFIG_SPINLOCK_SECTION
        LONG (__spinlock_begin)
        LONG (_spinlock_start)
        LONG ((_spinlock_end - _spinlock_start) / 4)
#endif
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

    .itcm :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_text = LOADADDR(.itcm));
        __itcm_start__ = .;
        KEEP(*(.null_trap_handler))


        *(.itcm_section*)
        *(.itcm_sec_code*)
        . = ALIGN(4);

        __itcm_end__ = .;
    } > ITCM AT > FLASH

    .dtcm :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_content = LOADADDR(.dtcm));
        __dtcm_start__ = .;
        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)
        . = ALIGN(4);
        __dtcm_end__ = .;
    } > DTCM AT > FLASH

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
        *(.iram)
        *(.interrupt)

#if (CONFIG_SPE && !CONFIG_RELEASE_IRAM)
        libos_source.a:tasks.c.obj*(.text .text.* *.rodata *.rodata.*)
        libos_source.a:queue.c.obj*(.text .text.* *.rodata *.rodata.*)
        libos_source.a:list.c.obj*(.text .text.* *.rodata *.rodata.*)
        libos_source.a:timers.c.obj*(.text .text.* *.rodata *.rodata.*)
        libos_source.a:port.c.obj*(.text .text.* *.rodata *.rodata.*)
        rtos_pub.c.obj*(.text .text.* *.rodata *.rodata.*)
        heap_4.c.obj*(.text .text.* *.rodata *.rodata.*)
        mem_arch.c.obj*(.text .text.* *.rodata *.rodata.*)
        gpio_driver_base.c.obj*(.text .text.* *.rodata *.rodata.*)
        flash_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        ckmn_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        arch_interrupt.c.obj*(.text .text.* *.rodata *.rodata.*)
        hal_machw.c.obj*(.text .text.* *.rodata *.rodata.*)
        interrupts.c.obj*(.text .text.* *.rodata *.rodata.*)
        rw_task.c.obj*(.text .text.* *.rodata *.rodata.*)
        ke_event.c.obj*(.text .text.* *.rodata *.rodata.*)
        me_strategy.c.obj*(.text .text.* *.rodata *.rodata.*)
        chan.c.obj*(.text .text.* *.rodata *.rodata.*)
        vif_mgmt.c.obj*(.text .text.* *.rodata *.rodata.*)
        rxl_cntrl.c.obj*(.text .text.* *.rodata *.rodata.*)
        rxl_hwdesc.c.obj*(.text .text.* *.rodata *.rodata.*)
        mm_time.c.obj*(.text .text.* *.rodata *.rodata.*)
        rwnx.c.obj*(.text .text.* *.rodata *.rodata.*)
        rwnx_rx.c.obj*(.text .text.* *.rodata *.rodata.*)
        libwifi.a:ps.c.obj*(.text .text.* *.rodata *.rodata.*)
        libwifi.a:mm.c.obj*(.text .text.* *.rodata *.rodata.*)
        rf_cntrl.c.obj*(.text .text.* *.rodata *.rodata.*)
        mcc.c.obj*(.text .text.* *.rodata *.rodata.*)
        phy_karst_bk7236.c.obj*(.text .text.* *.rodata *.rodata.*)
        crm.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk7236_cal.c.obj*(.text .text.* *.rodata *.rodata.*)
        manual_cal_bk7236.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_cal_common.c.obj*(.text .text.* *.rodata *.rodata.*)
        nv_parameters.c.obj*(.text .text.* *.rodata *.rodata.*)
        macif_fhost.c.obj*(.text .text.* *.rodata *.rodata.*)
        rc_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        sys_wifi_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        wifi_v2.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_feature.c.obj*(.text .text.* *.rodata *.rodata.*)
        libbk_pm.a:pm.c.obj*(.text .text.* *.rodata *.rodata.*)
        sys_ps_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        sys_pm_hal.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_wifi_adapter.c.obj*(.text .text.* *.rodata *.rodata.*)
        net_param.c.obj*(.text .text.* *.rodata *.rodata.*)
        wifi_api_wrapper.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_rf_adapter.c.obj*(.text .text.* *.rodata *.rodata.*)
        bk_phy_adapter.c.obj*(.text .text.* *.rodata *.rodata.*)
        shell_task.c.obj*(.text .text.* *.rodata *.rodata.*)
        shell_uart.c.obj*(.text .text.* *.rodata *.rodata.*)
        uart_driver.c.obj*(.text .text.* *.rodata *.rodata.*)
        uart_hal.c.obj*(.text .text.* *.rodata *.rodata.*)
#endif
        . = ALIGN(4);

        _iram_end_dummy_ = .;
    } > IRAM AT > FLASH

    .iram_boundary :
    {
        . += 4;
        . = ALIGN(4);
        __iram_end__ = .;
    } > IRAM AT > FLASH

    .ramcode :
    {
        . = . + (__iram_end__ - __iram_start__);
        . = ALIGN(512);
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
        *(.data)
        *(".data.*")
        *(.sdata)

        *(.video_spec_data*)
        *(.gnu.linkonce.d*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH

    _data_flash_begin = __data_flash_begin;
    _data_ram_begin = __data_start__;
    _data_ram_end = .;

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

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss*)
        *(COMMON)

#if CONFIG_CACHE_ENABLE
        . = ALIGN(4);
        _nocache_start = .;
        *(.sram_nocache*)
        _nocache_end = .;
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

    .swap ORIGIN(SWAP):
    {
        . = ALIGN(8);
        _swap_start = .;

        * (.swap_data)
        * (.swap_data*)
        . = ALIGN(4);
        _swap_end = .;
    } > SWAP AT > FLASH

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

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - __MSP_STACK_SIZE;

    ASSERT((s_flash_end < ORIGIN(FLASH) + LENGTH(FLASH)), "flash space overflow!!!")
    ASSERT(((ORIGIN(RAM) + LENGTH(RAM)) > (_heap_start + __MSP_STACK_SIZE + 8)), "Stack overflowed with bss")
    ASSERT(((_heap_end - _heap_start) >= __MIN_HEAP_SIZE), "Heap smaller than minimize size 90K!!!")
}
//eof

