/*
 * This file is part of the CmBacktrace Library.
 *
 * Copyright (c) 2016-2019, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Initialize function and other general function.
 * Created on: 2016-12-15
 */

#include <cm_backtrace.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#if __STDC_VERSION__ < 199901L
    #error "must be C99 or higher. try to add '-std=c99' to compile parameters"
#endif

#if defined(__ARMCC_VERSION)
    #define SECTION_START(_name_)                _name_##$$Base
    #define SECTION_END(_name_)                  _name_##$$Limit
    #define IMAGE_SECTION_START(_name_)          Image$$##_name_##$$Base
    #define IMAGE_SECTION_END(_name_)            Image$$##_name_##$$Limit
    #define CSTACK_BLOCK_START(_name_)           SECTION_START(_name_)
    #define CSTACK_BLOCK_END(_name_)             SECTION_END(_name_)
    #define CODE_SECTION_START(_name_)           IMAGE_SECTION_START(_name_)
    #define CODE_SECTION_END(_name_)             IMAGE_SECTION_END(_name_)

    extern const int CSTACK_BLOCK_START(CMB_CSTACK_BLOCK_NAME);
    extern const int CSTACK_BLOCK_END(CMB_CSTACK_BLOCK_NAME);
    extern const int CODE_SECTION_START(CMB_CODE_SECTION_NAME);
    extern const int CODE_SECTION_END(CMB_CODE_SECTION_NAME);
#elif defined(__ICCARM__)
    #pragma section=CMB_CSTACK_BLOCK_NAME
    #pragma section=CMB_CODE_SECTION_NAME
#elif defined(__GNUC__)
#if CONFIG_FREERTOS_SMP
    extern const int CMB_CSTACK_BLOCK_START_CPU0;
    extern const int CMB_CSTACK_BLOCK_END_CPU0;
    extern const int CMB_CSTACK_BLOCK_START_CPU1;
    extern const int CMB_CSTACK_BLOCK_END_CPU1;
    extern const int CMB_CSTACK_BLOCK_START_CPU2;
    extern const int CMB_CSTACK_BLOCK_END_CPU2;
#else
    extern const int CMB_CSTACK_BLOCK_START;
    extern const int CMB_CSTACK_BLOCK_END;
#endif
    extern const int CMB_CODE_SECTION_START;
    extern const int CMB_CODE_SECTION_END;
#else
    #error "not supported compiler"
#endif

enum {
    PRINT_MAIN_STACK_CFG_ERROR,
    PRINT_FIRMWARE_INFO,
    PRINT_ASSERT_ON_THREAD,
    PRINT_ASSERT_ON_HANDLER,
    PRINT_THREAD_STACK_INFO,
    PRINT_MAIN_STACK_INFO,
    PRINT_THREAD_STACK_OVERFLOW,
    PRINT_MAIN_STACK_OVERFLOW,
    PRINT_CALL_STACK_INFO,
    PRINT_CALL_STACK_ERR,
    PRINT_FAULT_ON_THREAD,
    PRINT_FAULT_ON_HANDLER,
    PRINT_REGS_TITLE,
    PRINT_HFSR_VECTBL,
    PRINT_MFSR_IACCVIOL,
    PRINT_MFSR_DACCVIOL,
    PRINT_MFSR_MUNSTKERR,
    PRINT_MFSR_MSTKERR,
    PRINT_MFSR_MLSPERR,
    PRINT_BFSR_IBUSERR,
    PRINT_BFSR_PRECISERR,
    PRINT_BFSR_IMPREISERR,
    PRINT_BFSR_UNSTKERR,
    PRINT_BFSR_STKERR,
    PRINT_BFSR_LSPERR,
    PRINT_UFSR_UNDEFINSTR,
    PRINT_UFSR_INVSTATE,
    PRINT_UFSR_INVPC,
    PRINT_UFSR_NOCP,
#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
    PRINT_UFSR_STKOF,
#endif
    PRINT_UFSR_UNALIGNED,
    PRINT_UFSR_DIVBYZERO0,
    PRINT_DFSR_HALTED,
    PRINT_DFSR_BKPT,
    PRINT_DFSR_DWTTRAP,
    PRINT_DFSR_VCATCH,
    PRINT_DFSR_EXTERNAL,
    PRINT_MMAR,
    PRINT_BFAR,
};

static const char * const print_info[] = {
#if (CMB_PRINT_LANGUAGE == CMB_PRINT_LANGUAGE_ENGLISH)
    #include "Languages/en-US/cmb_en_US.h"
#elif (CMB_PRINT_LANGUAGE == CMB_PRINT_LANGUAGE_CHINESE)
    #include "Languages/zh-CN/cmb_zh_CN.h"
#elif (CMB_PRINT_LANGUAGE == CMB_PRINT_LANGUAGE_CHINESE_UTF8)
    #include "Languages/zh-CN/cmb_zh_CN_UTF8.h"
#else
    #error "CMB_PRINT_LANGUAGE defined error in 'cmb_cfg.h'"
#endif
};

#if CONFIG_FREERTOS_SMP
#define NR_CPUS            CONFIG_CPU_CNT
#else
#define NR_CPUS            1
#endif

static char fw_name[NR_CPUS][CMB_NAME_MAX] = {0};
static char hw_ver[NR_CPUS][CMB_NAME_MAX] = {0};
static char sw_ver[NR_CPUS][CMB_NAME_MAX] = {0};
static uint32_t main_stack_start_addr[NR_CPUS] = {0, };
static size_t main_stack_size[NR_CPUS] = {0, };
static uint32_t code_start_addr = 0;
static size_t code_size = 0;
static bool init_ok[NR_CPUS] = {false, };
static char call_stack_info[NR_CPUS][CMB_CALL_STACK_MAX_DEPTH * (8 + 1)] = { 0 };
static bool on_fault[NR_CPUS] = {false, };
static bool stack_is_overflow[NR_CPUS] = {false, };
static struct cmb_hard_fault_regs regs[NR_CPUS];

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
    (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
static bool statck_has_fpu_regs[NR_CPUS] = {false, };
#endif

static bool on_thread_before_fault[NR_CPUS] = {false, };

int cm_backtrace_get_core()
{
#if CONFIG_FREERTOS_SMP
    return rtos_get_core_id();
#else
    return 0;
#endif
}

/**
 * library initialize
 */
void cm_backtrace_init(const char *firmware_name, const char *hardware_ver, const char *software_ver) {
    int core = cm_backtrace_get_core();

    strncpy(fw_name[core], firmware_name, CMB_NAME_MAX);
    strncpy(hw_ver[core], hardware_ver, CMB_NAME_MAX);
    strncpy(sw_ver[core], software_ver, CMB_NAME_MAX);

#if defined(__ARMCC_VERSION)
    main_stack_start_addr = (uint32_t)&CSTACK_BLOCK_START(CMB_CSTACK_BLOCK_NAME);
    main_stack_size = (uint32_t)&CSTACK_BLOCK_END(CMB_CSTACK_BLOCK_NAME) - main_stack_start_addr;
    code_start_addr = (uint32_t)&CODE_SECTION_START(CMB_CODE_SECTION_NAME);
    code_size = (uint32_t)&CODE_SECTION_END(CMB_CODE_SECTION_NAME) - code_start_addr;
#elif defined(__ICCARM__)
    main_stack_start_addr = (uint32_t)__section_begin(CMB_CSTACK_BLOCK_NAME);
    main_stack_size = (uint32_t)__section_end(CMB_CSTACK_BLOCK_NAME) - main_stack_start_addr;
    code_start_addr = (uint32_t)__section_begin(CMB_CODE_SECTION_NAME);
    code_size = (uint32_t)__section_end(CMB_CODE_SECTION_NAME) - code_start_addr;
#elif defined(__GNUC__)
#if CONFIG_FREERTOS_SMP
    if (core == CPU0_CORE_ID) {
        main_stack_start_addr[core] = (uint32_t)(&CMB_CSTACK_BLOCK_START_CPU0);
        main_stack_size[core] = (uint32_t)(&CMB_CSTACK_BLOCK_END_CPU0) - main_stack_start_addr[core];
    } else if (core == CPU1_CORE_ID) {
        main_stack_start_addr[core] = (uint32_t)(&CMB_CSTACK_BLOCK_START_CPU1);
        main_stack_size[core] = (uint32_t)(&CMB_CSTACK_BLOCK_END_CPU1) - main_stack_start_addr[core];
#if NR_CPUS > 2
    } else if (core == CPU2_CORE_ID) {
        main_stack_start_addr[core] = (uint32_t)(&CMB_CSTACK_BLOCK_START_CPU2);
        main_stack_size[core] = (uint32_t)(&CMB_CSTACK_BLOCK_END_CPU2) - main_stack_start_addr[core];
#endif // NR_CPUS
    }
#else // !CONFIG_FREERTOS_SMP
    main_stack_start_addr[core] = (uint32_t)(&CMB_CSTACK_BLOCK_START);
    main_stack_size[core] = (uint32_t)(&CMB_CSTACK_BLOCK_END) - main_stack_start_addr[core];
#endif // CONFIG_FREERTOS_SMP
    code_start_addr = (uint32_t)(&CMB_CODE_SECTION_START);
    code_size = (uint32_t)(&CMB_CODE_SECTION_END) - code_start_addr;
#else
    #error "not supported compiler"
#endif

    if (main_stack_size[core] == 0) {
        cmb_println(print_info[PRINT_MAIN_STACK_CFG_ERROR]);
        return;
    }

    init_ok[core] = true;
}

/**
 * print firmware information, such as: firmware name, hardware version, software version
 */
void cm_backtrace_firmware_info(void) {
    int core = cm_backtrace_get_core();

    cmb_println(print_info[PRINT_FIRMWARE_INFO], fw_name[core], hw_ver[core], sw_ver[core]);
}

#ifdef CMB_USING_OS_PLATFORM
/**
 * Get current thread stack information
 *
 * @param sp stack current pointer
 * @param start_addr stack start address
 * @param size stack size
 */
static void get_cur_thread_stack_info(uint32_t sp, uint32_t *start_addr, size_t *size) {
    CMB_ASSERT(start_addr);
    CMB_ASSERT(size);

#if (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_RTT)
    *start_addr = (uint32_t) rt_thread_self()->stack_addr;
    *size = rt_thread_self()->stack_size;
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSII)
    extern OS_TCB *OSTCBCur;

    *start_addr = (uint32_t) OSTCBCur->OSTCBStkBottom;
    *size = OSTCBCur->OSTCBStkSize * sizeof(OS_STK);
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSIII)
    extern OS_TCB *OSTCBCurPtr;

    *start_addr = (uint32_t) OSTCBCurPtr->StkBasePtr;
    *size = OSTCBCurPtr->StkSize * sizeof(CPU_STK_SIZE);
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_FREERTOS)
    *start_addr = (uint32_t)vTaskStackAddr();
    *size = vTaskStackSize() * sizeof( StackType_t );
#endif
}

/**
 * Get current thread name
 */
static const char *get_cur_thread_name(void) {
#if (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_RTT)
    return rt_thread_self()->name;
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSII)
    extern OS_TCB *OSTCBCur;

#if OS_TASK_NAME_SIZE > 0 || OS_TASK_NAME_EN > 0
        return (const char *)OSTCBCur->OSTCBTaskName;
#else
        return NULL;
#endif /* OS_TASK_NAME_SIZE > 0 || OS_TASK_NAME_EN > 0 */

#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSIII)
    extern OS_TCB *OSTCBCurPtr;

    return (const char *)OSTCBCurPtr->NamePtr;
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_FREERTOS)
    return vTaskName();
#endif
}

#endif /* CMB_USING_OS_PLATFORM */

#ifdef CMB_USING_DUMP_STACK_INFO
/**
 * dump current stack information
 */
static void dump_stack(uint32_t stack_start_addr, size_t stack_size, uint32_t *stack_pointer) {
    int core = cm_backtrace_get_core();

    if (stack_is_overflow[core]) {
        if (on_thread_before_fault[core]) {
            cmb_println(print_info[PRINT_THREAD_STACK_OVERFLOW], stack_pointer);
        } else {
            cmb_println(print_info[PRINT_MAIN_STACK_OVERFLOW], stack_pointer);
        }
        if ((uint32_t) stack_pointer < stack_start_addr) {
            stack_pointer = (uint32_t *) stack_start_addr;
        } else if ((uint32_t) stack_pointer > stack_start_addr + stack_size) {
            stack_pointer = (uint32_t *) (stack_start_addr + stack_size);
        }
    }
    cmb_println(print_info[PRINT_THREAD_STACK_INFO]);
    for (; (uint32_t) stack_pointer < stack_start_addr + stack_size; stack_pointer++) {
        cmb_println("  addr: %08x    data: %08x", stack_pointer, *stack_pointer);
    }
    cmb_println("====================================");
}
#endif /* CMB_USING_DUMP_STACK_INFO */

/* check the disassembly instruction is 'BL' or 'BLX' */
static bool disassembly_ins_is_bl_blx(uint32_t addr) {
    uint16_t ins1 = *((uint16_t *)addr);
    uint16_t ins2 = *((uint16_t *)(addr + 2));

#define BL_INS_MASK         0xF800
#define BL_INS_HIGH         0xF800
#define BL_INS_LOW          0xF000
#define BLX_INX_MASK        0xFF00
#define BLX_INX             0x4700

    if ((ins2 & BL_INS_MASK) == BL_INS_HIGH && (ins1 & BL_INS_MASK) == BL_INS_LOW) {
        return true;
    } else if ((ins2 & BLX_INX_MASK) == BLX_INX) {
        return true;
    } else {
        return false;
    }
}

/**
 * backtrace function call stack
 *
 * @param buffer call stack buffer
 * @param size buffer size
 * @param sp stack pointer
 *
 * @return depth
 */
size_t cm_backtrace_call_stack(uint32_t *buffer, size_t size, uint32_t sp) {
    int core = cm_backtrace_get_core();
    uint32_t stack_start_addr = main_stack_start_addr[core], pc;
    size_t depth = 0, stack_size = main_stack_size[core];
    bool regs_saved_lr_is_valid = false;

    if (on_fault[core]) {
        if (!stack_is_overflow[core]) {
            /* first depth is PC */
            buffer[depth++] = regs[core].saved.pc;
            /* fix the LR address in thumb mode */
            pc = regs[core].saved.lr - 1;
            if ((pc >= code_start_addr) && (pc <= code_start_addr + code_size) && (depth < CMB_CALL_STACK_MAX_DEPTH)
                    && (depth < size)) {
                buffer[depth++] = pc;
                regs_saved_lr_is_valid = true;
            }
        }

#ifdef CMB_USING_OS_PLATFORM
        /* program is running on thread before fault */
        if (on_thread_before_fault[core]) {
            get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
        }
    } else {
        /* OS environment */
        if (cmb_get_sp() == cmb_get_psp()) {
            get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
        }
#endif /* CMB_USING_OS_PLATFORM */

    }

    if (stack_is_overflow[core]) {
        if (sp < stack_start_addr) {
            sp = stack_start_addr;
        } else if (sp > stack_start_addr + stack_size) {
            sp = stack_start_addr + stack_size;
        }
    }

    /* copy called function address */
    for (; sp < stack_start_addr + stack_size; sp += sizeof(size_t)) {
        /* the *sp value may be LR, so need decrease a word to PC */
        pc = *((uint32_t *) sp) - sizeof(size_t);
        /* the Cortex-M using thumb instruction, so the pc must be an odd number */
        if (pc % 2 == 0) {
            continue;
        }
        /* fix the PC address in thumb mode */
        pc = *((uint32_t *) sp) - 1;
        if ((pc >= code_start_addr + sizeof(size_t)) && (pc <= code_start_addr + code_size) && (depth < CMB_CALL_STACK_MAX_DEPTH)
                /* check the the instruction before PC address is 'BL' or 'BLX' */
                && disassembly_ins_is_bl_blx(pc - sizeof(size_t)) && (depth < size)) {
            /* the second depth function may be already saved, so need ignore repeat */
            if ((depth == 2) && regs_saved_lr_is_valid && (pc == buffer[1])) {
                continue;
            }
            buffer[depth++] = pc;
        }
    }

    return depth;
}

/**
 * dump function call stack
 *
 * @param sp stack pointer
 */
static void print_call_stack(uint32_t sp) {
    size_t i, cur_depth = 0;
    uint32_t call_stack_buf[CMB_CALL_STACK_MAX_DEPTH] = {0};
    int core = cm_backtrace_get_core();

    cur_depth = cm_backtrace_call_stack(call_stack_buf, CMB_CALL_STACK_MAX_DEPTH, sp);

    for (i = 0; i < cur_depth; i++) {
        sprintf(call_stack_info[core] + i * (8 + 1), "%08lx", (unsigned long)call_stack_buf[i]);
        call_stack_info[core][i * (8 + 1) + 8] = ' ';
    }

    if (cur_depth) {
        cmb_println(print_info[PRINT_CALL_STACK_INFO], fw_name[core], CMB_ELF_FILE_EXTENSION_NAME, cur_depth * (8 + 1),
                call_stack_info[core]);
    } else {
        cmb_println(print_info[PRINT_CALL_STACK_ERR]);
    }
}

/**
 * backtrace for assert
 *
 * @param sp the stack pointer when on assert occurred
 */
void cm_backtrace_assert(uint32_t sp) {
    int core = cm_backtrace_get_core();

    CMB_ASSERT(init_ok[core]);

#ifdef CMB_USING_OS_PLATFORM
    uint32_t cur_stack_pointer = cmb_get_sp();
#endif

    cmb_println("");
    cm_backtrace_firmware_info();

#ifdef CMB_USING_OS_PLATFORM
    /* OS environment */
    if (cur_stack_pointer == cmb_get_msp()) {
        cmb_println(print_info[PRINT_ASSERT_ON_HANDLER]);

#ifdef CMB_USING_DUMP_STACK_INFO
        dump_stack(main_stack_start_addr[core], main_stack_size[core], (uint32_t *) sp);
#endif /* CMB_USING_DUMP_STACK_INFO */

    } else if (cur_stack_pointer == cmb_get_psp()) {
        cmb_println(print_info[PRINT_ASSERT_ON_THREAD], get_cur_thread_name());

#ifdef CMB_USING_DUMP_STACK_INFO
        uint32_t stack_start_addr;
        size_t stack_size;
        get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
        dump_stack(stack_start_addr, stack_size, (uint32_t *) sp);
#endif /* CMB_USING_DUMP_STACK_INFO */

    }

#else

    /* bare metal(no OS) environment */
#ifdef CMB_USING_DUMP_STACK_INFO
    dump_stack(main_stack_start_addr[core], main_stack_size[core], (uint32_t *) sp);
#endif /* CMB_USING_DUMP_STACK_INFO */

#endif /* CMB_USING_OS_PLATFORM */

    print_call_stack(sp);

}

#if (CMB_CPU_PLATFORM_TYPE != CMB_CPU_ARM_CORTEX_M0)
/**
 * fault diagnosis then print cause of fault
 */
static void fault_diagnosis(void) {
    int core = cm_backtrace_get_core();

    if (regs[core].hfsr.bits.VECTBL) {
        cmb_println(print_info[PRINT_HFSR_VECTBL]);
    }
    if (regs[core].hfsr.bits.FORCED) 
    {
		cmb_println("Forced Hardfault:\r\n");
    }
	{
        /* Memory Management Fault */
        if (regs[core].mfsr.value) {
            if (regs[core].mfsr.bits.IACCVIOL) {
                cmb_println(print_info[PRINT_MFSR_IACCVIOL]);
            }
            if (regs[core].mfsr.bits.DACCVIOL) {
                cmb_println(print_info[PRINT_MFSR_DACCVIOL]);
            }
            if (regs[core].mfsr.bits.MUNSTKERR) {
                cmb_println(print_info[PRINT_MFSR_MUNSTKERR]);
            }
            if (regs[core].mfsr.bits.MSTKERR) {
                cmb_println(print_info[PRINT_MFSR_MSTKERR]);
            }

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
    (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
            if (regs[core].mfsr.bits.MLSPERR) {
                cmb_println(print_info[PRINT_MFSR_MLSPERR]);
            }
#endif

            if (regs[core].mfsr.bits.MMARVALID) {
               // if (regs[core].mfsr.bits.IACCVIOL || regs[core].mfsr.bits.DACCVIOL)
				{
                    cmb_println(print_info[PRINT_MMAR], regs[core].mmar);
                }
            }
        }
        /* Bus Fault */
        if (regs[core].bfsr.value) {
            if (regs[core].bfsr.bits.IBUSERR) {
                cmb_println(print_info[PRINT_BFSR_IBUSERR]);
            }
            if (regs[core].bfsr.bits.PRECISERR) {
                cmb_println(print_info[PRINT_BFSR_PRECISERR]);
            }
            if (regs[core].bfsr.bits.IMPREISERR) {
                cmb_println(print_info[PRINT_BFSR_IMPREISERR]);
            }
            if (regs[core].bfsr.bits.UNSTKERR) {
                cmb_println(print_info[PRINT_BFSR_UNSTKERR]);
            }
            if (regs[core].bfsr.bits.STKERR) {
                cmb_println(print_info[PRINT_BFSR_STKERR]);
            }

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
    (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
            if (regs[core].bfsr.bits.LSPERR) {
                cmb_println(print_info[PRINT_BFSR_LSPERR]);
            }
#endif

            if (regs[core].bfsr.bits.BFARVALID) {
              //  if (regs[core].bfsr.bits.PRECISERR)
				{
                    cmb_println(print_info[PRINT_BFAR], regs[core].bfar);
                }
            }

        }
        /* Usage Fault */
        if (regs[core].ufsr.value) {
            if (regs[core].ufsr.bits.UNDEFINSTR) {
                cmb_println(print_info[PRINT_UFSR_UNDEFINSTR]);
            }
            if (regs[core].ufsr.bits.INVSTATE) {
                cmb_println(print_info[PRINT_UFSR_INVSTATE]);
            }
            if (regs[core].ufsr.bits.INVPC) {
                cmb_println(print_info[PRINT_UFSR_INVPC]);
            }
            if (regs[core].ufsr.bits.NOCP) {
                cmb_println(print_info[PRINT_UFSR_NOCP]);
            }
#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
            if (regs[core].ufsr.bits.STKOF) {
                cmb_println(print_info[PRINT_UFSR_STKOF]);
            }
#endif
            if (regs[core].ufsr.bits.UNALIGNED) {
                cmb_println(print_info[PRINT_UFSR_UNALIGNED]);
            }
            if (regs[core].ufsr.bits.DIVBYZERO0) {
                cmb_println(print_info[PRINT_UFSR_DIVBYZERO0]);
            }
        }
    }
    /* Debug Fault */
    if (regs[core].hfsr.bits.DEBUGEVT) 
    {
		cmb_println("Debugfault:\r\n");
    }
	{
        if (regs[core].dfsr.value) {
            if (regs[core].dfsr.bits.HALTED) {
                cmb_println(print_info[PRINT_DFSR_HALTED]);
            }
            if (regs[core].dfsr.bits.BKPT) {
                cmb_println(print_info[PRINT_DFSR_BKPT]);
            }
            if (regs[core].dfsr.bits.DWTTRAP) {
                cmb_println(print_info[PRINT_DFSR_DWTTRAP]);
            }
            if (regs[core].dfsr.bits.VCATCH) {
                cmb_println(print_info[PRINT_DFSR_VCATCH]);
            }
            if (regs[core].dfsr.bits.EXTERNAL) {
                cmb_println(print_info[PRINT_DFSR_EXTERNAL]);
            }
        }
    }
}
#endif /* (CMB_CPU_PLATFORM_TYPE != CMB_CPU_ARM_CORTEX_M0) */

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
    (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
static uint32_t statck_del_fpu_regs(uint32_t fault_handler_lr, uint32_t sp) {
    int core = cm_backtrace_get_core();

    statck_has_fpu_regs[core] = (fault_handler_lr & (1UL << 4)) == 0 ? true : false;

    /* the stack has S0~S15 and FPSCR registers when statck_has_fpu_regs is true, double word align */
    return statck_has_fpu_regs[core] == true ? sp + sizeof(size_t) * 18 : sp;
}
#endif



/**
 * backtrace for fault
 * @note only call once
 *
 * @param fault_handler_lr the LR register value on fault handler
 * @param fault_handler_sp the stack pointer on fault handler
 */
void cm_backtrace_fault(uint32_t fault_handler_lr, uint32_t fault_handler_sp) {

    uint32_t stack_pointer = fault_handler_sp, saved_regs_addr = stack_pointer;
    const char *regs_name[] = { "R0 ", "R1 ", "R2 ", "R3 ", "R12", "LR ", "PC ", "PSR" };
    int core = cm_backtrace_get_core();
#if CONFIG_FREERTOS_SMP
    uint32_t flags;
#endif

#ifdef CMB_USING_DUMP_STACK_INFO
    uint32_t stack_start_addr = main_stack_start_addr[core];
    size_t stack_size = main_stack_size[core];
#endif

    CMB_ASSERT(init_ok[core]);
    /* only call once */
    CMB_ASSERT(!on_fault[core]);

#if CONFIG_FREERTOS_SMP
    /* print in sync with multi-cores */
    flags = rtos_enter_critical();
#endif

    on_fault[core] = true;

    cmb_println("");
    cm_backtrace_firmware_info();

#ifdef CMB_USING_OS_PLATFORM
    on_thread_before_fault[core] = fault_handler_lr & (1UL << 2);
    /* check which stack was used before (MSP or PSP) */
    if (on_thread_before_fault[core]) {
        cmb_println(print_info[PRINT_FAULT_ON_THREAD], get_cur_thread_name() != NULL ? get_cur_thread_name() : "NO_NAME");
        saved_regs_addr = stack_pointer = cmb_get_psp();

#ifdef CMB_USING_DUMP_STACK_INFO
        get_cur_thread_stack_info(stack_pointer, &stack_start_addr, &stack_size);
#endif /* CMB_USING_DUMP_STACK_INFO */

    } else {
        cmb_println(print_info[PRINT_FAULT_ON_HANDLER]);
    }
#else
    /* bare metal(no OS) environment */
    cmb_println(print_info[PRINT_FAULT_ON_HANDLER]);
#endif /* CMB_USING_OS_PLATFORM */

    /* delete saved R0~R3, R12, LR,PC,xPSR registers space */
    stack_pointer += sizeof(size_t) * 8;

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
    (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
    stack_pointer = statck_del_fpu_regs(fault_handler_lr, stack_pointer);
#endif /* (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) */

#ifdef CMB_USING_DUMP_STACK_INFO
    /* check stack overflow */
    if (stack_pointer < stack_start_addr || stack_pointer > stack_start_addr + stack_size) {
        stack_is_overflow[core] = true;
    }
    /* dump stack information */
    dump_stack(stack_start_addr, stack_size, (uint32_t *) stack_pointer);
#endif /* CMB_USING_DUMP_STACK_INFO */

    /* the stack frame may be get failed when it is overflow  */
    if (!stack_is_overflow[core]) {
        /* dump register */
        cmb_println(print_info[PRINT_REGS_TITLE]);

        regs[core].saved.r0        = ((uint32_t *)saved_regs_addr)[0];  // Register R0
        regs[core].saved.r1        = ((uint32_t *)saved_regs_addr)[1];  // Register R1
        regs[core].saved.r2        = ((uint32_t *)saved_regs_addr)[2];  // Register R2
        regs[core].saved.r3        = ((uint32_t *)saved_regs_addr)[3];  // Register R3
        regs[core].saved.r12       = ((uint32_t *)saved_regs_addr)[4];  // Register R12
        regs[core].saved.lr        = ((uint32_t *)saved_regs_addr)[5];  // Link register LR
        regs[core].saved.pc        = ((uint32_t *)saved_regs_addr)[6];  // Program counter PC
        regs[core].saved.psr.value = ((uint32_t *)saved_regs_addr)[7];  // Program status word PSR

        cmb_println("  %s: %08x  %s: %08x  %s: %08x  %s: %08x", regs_name[0], regs[core].saved.r0,
                                                                regs_name[1], regs[core].saved.r1,
                                                                regs_name[2], regs[core].saved.r2,
                                                                regs_name[3], regs[core].saved.r3);
        cmb_println("  %s: %08x  %s: %08x  %s: %08x  %s: %08x", regs_name[4], regs[core].saved.r12,
                                                                regs_name[5], regs[core].saved.lr,
                                                                regs_name[6], regs[core].saved.pc,
                                                                regs_name[7], regs[core].saved.psr.value);
        cmb_println("==============================================================");
    }

    /* the Cortex-M0 is not support fault diagnosis */
#if (CMB_CPU_PLATFORM_TYPE != CMB_CPU_ARM_CORTEX_M0)
    regs[core].syshndctrl.value = CMB_SYSHND_CTRL;  // System Handler Control and State Register
    regs[core].mfsr.value       = CMB_NVIC_MFSR;    // Memory Fault Status Register
    regs[core].mmar             = CMB_NVIC_MMAR;    // Memory Management Fault Address Register
    regs[core].bfsr.value       = CMB_NVIC_BFSR;    // Bus Fault Status Register
    regs[core].bfar             = CMB_NVIC_BFAR;    // Bus Fault Manage Address Register
    regs[core].ufsr.value       = CMB_NVIC_UFSR;    // Usage Fault Status Register
    regs[core].hfsr.value       = CMB_NVIC_HFSR;    // Hard Fault Status Register
    regs[core].dfsr.value       = CMB_NVIC_DFSR;    // Debug Fault Status Register
    regs[core].afsr             = CMB_NVIC_AFSR;    // Auxiliary Fault Status Register

    fault_diagnosis();
#endif

    print_call_stack(stack_pointer);

#if CONFIG_FREERTOS_SMP
    rtos_exit_critical(flags);
#endif

}
