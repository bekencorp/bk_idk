/*
 * Copyright (c) 2009-2020 ARM Limited
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
 *
 * This file is derivative of CMSIS V5.00 startup_ARMCM33.S
 */

#include "region_defs.h"
#include "os/os.h"
#include "cmsis_gcc.h"
#include "sdkconfig.h"
#include "partitions_gen.h"
#include "sys_persist_config.h"
#include "sys_hal.h"

#define ENTRY_SECTION                    __attribute__((section(".fix.reset_entry")))
#define SYSTEM_BASE_ADDR                 (0x44010000)
#define OTP_APB_BASE_ADDRESSS            (0x4b100000)
#define MEM_CHECK_BASE_ADDRESSS          (0x44890000)
#define GET_BIT_VAL(val, bit)            (((val) >> (bit)) & 1)
#define SET_BIT_VAL(val, bit, new_val)   (((val) & (~(1 << (bit)))) | ((new_val) << (bit)))

#define AON_PMU_BASE_ADDR                (0x44000000)
#define AON_PMU_R7B_REG                  (*(volatile uint32_t *)(AON_PMU_BASE_ADDR + 0x7b * 4))
#define DLV_STARTUP_BIT                  (1 << 23)

#define SOC_FLASH_BASE_ADDR     (0x44030000)
#define FLASH_OFFSET_BEGIN_REG  (*(volatile uint32_t *)(SOC_FLASH_BASE_ADDR + 0x16 * 4))
#define FLASH_OFFSET_END_REG    (*(volatile uint32_t *)(SOC_FLASH_BASE_ADDR + 0x17 * 4))
#define FLASH_ADDR_OFFSET_REG   (*(volatile uint32_t *)(SOC_FLASH_BASE_ADDR + 0x18 * 4))
#define FLASH_CTRL_REG          (*(volatile uint32_t *)(SOC_FLASH_BASE_ADDR + 0x19 * 4))
#define FLASH_OFFSET_EN_BIT     (0x1)

#define FLASH_OP_CTRL_REG       (*(volatile uint32_t *)(SOC_FLASH_BASE_ADDR + 0x04 * 4))
#define FLASH_DATA_RD_REG       (*(volatile uint32_t *)(SOC_FLASH_BASE_ADDR + 0x06 * 4))
#define FLASH_OP_CMD_REG        (*(volatile uint32_t *)(SOC_FLASH_BASE_ADDR + 0x15 * 4))
#define FLASH_OP_SW_BIT         (1u << 29)
#define FLASH_BUSY_BIT          (1u << 31)
#define FLASH_CMD_READ          (5u)

#define SCB_VTOR_REG            (*(volatile uint32_t *)0xE000ED08)

#define BL2_APP_VT_ADDR  \
    (0x02000000 + CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_START + CONFIG_SYS_PERSIST_SIZE)

#define BL2_DLV_STACK_TOP       (BL2_DLV_IRAM_START + BL2_DLV_IRAM_SIZE)

extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;
extern __NO_RETURN void __PROGRAM_START(void);
extern void SystemInit(void);

typedef void (*VECTOR_TABLE_Type)(void);

__NO_RETURN void Reset_Handler(void);

void NMI_Handler(void) __attribute__((weak));
void HardFault_Handler(void) __attribute__((weak));
void MemManage_Handler(void) __attribute__((weak));
void BusFault_Handler(void) __attribute__((weak));
void UsageFault_Handler(void) __attribute__((weak));
void SecureFault_Handler(void) __attribute__((weak));
void SVC_Handler(void) __attribute__((weak));
void DebugMon_Handler(void) __attribute__((weak));
void PendSV_Handler(void) __attribute__((weak));
void SysTick_Handler(void) __attribute__((weak));
void UART_InterruptHandler(void) __attribute__((weak));

const VECTOR_TABLE_Type __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
    (VECTOR_TABLE_Type)(&__INITIAL_SP),       /*     Initial Stack Pointer */
    /* Core interrupts */
    Reset_Handler,                            /*     Reset Handler */
    NMI_Handler,                              /* -14 NMI Handler */
    HardFault_Handler,                        /* -13 Hard Fault Handler */
    MemManage_Handler,                        /* -12 MPU Fault Handler */
    BusFault_Handler,                         /* -11 Bus Fault Handler */
    UsageFault_Handler,                       /* -10 Usage Fault Handler */
    SecureFault_Handler,                      /*  -9 Secure Fault Handler */
    0,                                        /*  -8 */
    0,                                        /*  -7 */
    0,                                        /*  -6 */
    SVC_Handler,                              /*  -5 SVCall Handler */
    DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
    0,                                        /*  -3 */
    PendSV_Handler,                           /*  -2 PendSV Handler */
    SysTick_Handler,                          /*  -1 SysTick Handler */
    /* External interrupts */
    0,                                        /* Interrupt 0 */
    0,                                        /* Interrupt 1 */
    0,                                        /* Interrupt 2 */
    0,                                        /* Interrupt 3 */
    UART_InterruptHandler,                    /* Interrupt 4 */
    0,                                        /* Interrupt 5 */
    0,                                        /* Interrupt 6 */
    0,                                        /* Interrupt 7 */
    0,                                        /* Interrupt 8 */
    0,                                        /* Interrupt 9 */
    0,                                        /* Interrupt 10 */
    0,                                        /* Interrupt 11 */
    0,                                        /* Interrupt 12 */
    0,                                        /* Interrupt 13 */
    0,                                        /* Interrupt 14 */
    0,                                        /* Interrupt 15 */
    0,                                        /* Interrupt 16 */
    0,                                        /* Interrupt 17 */
    0,                                        /* Interrupt 18 */
    0,                                        /* Interrupt 19 */
    0,                                        /* Interrupt 20 */
    0,                                        /* Interrupt 21 */
    0,                                        /* Interrupt 22 */
    0,                                        /* Interrupt 23 */
    0,                                        /* Interrupt 24 */
    0,                                        /* Interrupt 25 */
    0,                                        /* Interrupt 26 */
    0,                                        /* Interrupt 27 */
    0,                                        /* Interrupt 28 */
    (void (*)(void))0x140,                    /* Default Jump BIN offset  */
    (void (*)(void))0x100,                    /* Default Jump BIN length */
    0,                                        /* Interrupt 31 */
    0,                                        /* Interrupt 32 */
    0,                                        /* Interrupt 33 */
    0,                                        /* Interrupt 34 */
    0,                                        /* Interrupt 35 */
    0,                                        /* Interrupt 36 */
    0,                                        /* Interrupt 37 */
    0,                                        /* Interrupt 38 */
    0,                                        /* Interrupt 39 */
    0,                                        /* Interrupt 40 */
    0,                                        /* Interrupt 41 */
    0,                                        /* Interrupt 42 */
    0,                                        /* Interrupt 43 */
    0,                                        /* Interrupt 44 */
    0,                                        /* Interrupt 45 */
    0,                                        /* Interrupt 46 */
    0,                                        /* Interrupt 47 */
    (void (*)(void))0x32374B42,               /* offset 0x100, magic code: BK7236 */
    (void (*)(void))0x00003633,
    /* Reserve 32 bytes to protect magic code */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

const VECTOR_TABLE_Type __VECTOR_IRAM[] __VECTOR_IRAM_ATTRIBUTE = {
    (VECTOR_TABLE_Type)(&__INITIAL_SP),       /*     Initial Stack Pointer */
    /* Core interrupts */
    Reset_Handler,                            /*     Reset Handler */
    NMI_Handler,                              /* -14 NMI Handler */
    HardFault_Handler,                        /* -13 Hard Fault Handler */
    MemManage_Handler,                        /* -12 MPU Fault Handler */
    BusFault_Handler,                         /* -11 Bus Fault Handler */
    UsageFault_Handler,                       /* -10 Usage Fault Handler */
    SecureFault_Handler,                      /*  -9 Secure Fault Handler */
    0,                                        /*  -8 */
    0,                                        /*  -7 */
    0,                                        /*  -6 */
    SVC_Handler,                              /*  -5 SVCall Handler */
    DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
    0,                                        /*  -3 */
    PendSV_Handler,                           /*  -2 PendSV Handler */
    SysTick_Handler,                          /*  -1 SysTick Handler */
    /* External interrupts */
    0,                                        /* Interrupt 0 */
    0,                                        /* Interrupt 1 */
    0,                                        /* Interrupt 2 */
    0,                                        /* Interrupt 3 */
    UART_InterruptHandler,                    /* Interrupt 4 */
};

__attribute__((section(".dlv_iram")))
static uint32_t flash_spi_read_word_iram(uint32_t phy_addr)
{
    while (FLASH_OP_CTRL_REG & FLASH_BUSY_BIT) {;}

    FLASH_OP_CMD_REG = (FLASH_CMD_READ << 24) | (phy_addr & 0x00FFFFFF);
    FLASH_OP_CTRL_REG |= FLASH_OP_SW_BIT;

    while (FLASH_OP_CTRL_REG & FLASH_BUSY_BIT) {;}

    return FLASH_DATA_RD_REG;
}

__attribute__((section(".dlv_iram")))
__attribute__((noinline))
__attribute__((noreturn))
static void bl2_deep_lv_boot(void)
{
    sys_hal_enable_high_freq_iram();

#if CONFIG_DIRECT_XIP || CONFIG_MIXED_OTA
    uint32_t slot_val =
        flash_spi_read_word_iram(CONFIG_XIP_OTA_CONTROL_PHY_PARTITION_OFFSET);

    FLASH_OFFSET_BEGIN_REG = CONFIG_APP_A_PHY_PARTITION_OFFSET;
    FLASH_OFFSET_END_REG = CONFIG_APP_A_PHY_PARTITION_OFFSET +
                           CONFIG_APP_A_PHY_PARTITION_SIZE;
    FLASH_ADDR_OFFSET_REG = CONFIG_APP_B_PHY_PARTITION_OFFSET -
                            CONFIG_APP_A_PHY_PARTITION_OFFSET;

    if (slot_val != 0xFFFFFFFF) {
        FLASH_CTRL_REG |= FLASH_OFFSET_EN_BIT;
    } else {
        FLASH_CTRL_REG &= ~FLASH_OFFSET_EN_BIT;
    }
#endif

    volatile uint32_t *vt = (volatile uint32_t *)BL2_APP_VT_ADDR;
    uint32_t app_msp = vt[0];
    uint32_t app_reset = vt[1];

    SCB_VTOR_REG = BL2_APP_VT_ADDR;

    __asm__ volatile(
        "msr    msplim, %[zero]         \n"
        "msr    msp, %[msp]            \n"
        "mov    r7, %[reset]            \n"
        "movs   r0, #0                  \n"
        "mov    r1, r0                  \n"
        "mov    r2, r0                  \n"
        "mov    r3, r0                  \n"
        "mov    r4, r0                  \n"
        "mov    r5, r0                  \n"
        "mov    r6, r0                  \n"
        "mov    r8, r0                  \n"
        "mov    r9, r0                  \n"
        "mov    r10, r0                 \n"
        "mov    r11, r0                 \n"
        "mov    r12, r0                 \n"
        "mov    lr, r0                  \n"
        "bx     r7                      \n"
        :
        : [reset] "r" (app_reset), [msp] "r" (app_msp), [zero] "r" (0)
        : "memory"
    );

    while (1) {}
}

static uint32_t memory_is_retention(void)
{
	return ((REG_READ(0x2807E400 + (16 << 2)) == 0x5A5A5A5A)
		    && (REG_READ(0x2807E400 + (17 << 2)) == 0xA5A5A5A5));
}

__NO_RETURN ENTRY_SECTION __attribute__((naked)) void Reset_Handler(void)
{
    if ((AON_PMU_R7B_REG & DLV_STARTUP_BIT) && memory_is_retention()) {
        __set_MSP(BL2_DLV_STACK_TOP);
        __set_MSPLIM(BL2_DLV_IRAM_START);

        wdt_hal_force_feed();
        timer_hal_us_init_iram();
        sys_hal_enable_pll_iram();

        bl2_deep_lv_boot();
    }

    /* Normal boot path */
    __set_MSPLIM((uint32_t)(&__STACK_LIMIT));

    /* CMSIS System Initialization */
    SystemInit();

    sys_drv_early_init();

    /* Enter PreMain (C library entry point) */
    __PROGRAM_START();
}

/* Naked fault handlers: use asm branch-to-self to halt without stack/prologue.
   Do not return from fault to avoid handing control to non-secure or undefined state. */
#define FAULT_HANDLER_HALT()  __ASM volatile("b .")

#define DEFINE_NAKED_FAULT_HANDLER(handler) \
    __attribute__((naked)) void handler(void) { FAULT_HANDLER_HALT(); }

DEFINE_NAKED_FAULT_HANDLER(MemManage_Handler)
DEFINE_NAKED_FAULT_HANDLER(BusFault_Handler)
DEFINE_NAKED_FAULT_HANDLER(UsageFault_Handler)
DEFINE_NAKED_FAULT_HANDLER(SecureFault_Handler)