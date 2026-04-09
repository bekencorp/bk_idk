/*
 * Copyright (c) 2009-2019 Arm Limited.
 * Copyright (c) 2019-2020 ArmChina.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "components/log.h"
#include "components/system.h"
#include "armstar.h"
#include "aon_pmu_driver.h"
#include "sys_driver.h"
#include "driver/uart.h"
#include "bk_pm_internal_api.h"
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "sdkconfig.h"
#include "system_star.h"

#if CONFIG_CM_BACKTRACE
#include "cm_backtrace.h"
#endif

#if (CONFIG_INT_WDT || CONFIG_TASK_WDT)
#include "wdt_driver.h"
#endif

#if CONFIG_MPU
#include "mpu.h"
#endif

#include "stack_base.h"

#include <driver/wdt.h>
#include <os/os.h>
#include "bk_aon_wdt.h"
#include <driver/uart.h>
#include <reset_reason.h>
#include "arch_interrupt.h"

#if CONFIG_DEEP_LV
#include "deep_lv.h"
#endif

#if CONFIG_SUPPORT_PSRAM_XIP
#include <driver/psram.h>
#endif

#define TAG "arch"

#define SYSTEM_BASE_ADDRESS              SOC_SYS_REG_BASE
#define OTP_APB_BASE_ADDRESS             SOC_OTP_REG_BASE
#define MEM_CHECK_BASE_ADDRESS           SOC_MEM_CHECK_REG_BASE
#define BIT_ID(num)                      (num)
#define GET_BIT_VAL(val, bit)            ((val >> bit) & 1)
#define SET_BIT_VAL(val, bit, new_val)   ((val & (~ (1 << bit)))|(new_val << bit))

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;

extern __NO_RETURN void __PROGRAM_START(void);
extern void entry_main(void);

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler  (void);
            void Default_Handler(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Exceptions */
void NMI_Handler            (void) __attribute__ ((weak));
void HardFault_Handler      (void) __attribute__ ((weak));
void MemManage_Handler      (void) __attribute__ ((weak));
void BusFault_Handler       (void) __attribute__ ((weak));
void UsageFault_Handler     (void) __attribute__ ((weak));
void SecureFault_Handler    (void) __attribute__ ((weak));
void SVC_Handler            (void) __attribute__ ((weak, alias("Default_Handler")));
void DebugMon_Handler       (void) __attribute__ ((weak));
void PendSV_Handler         (void) __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler        (void) __attribute__ ((weak, alias("Default_Handler")));

/* Interrupt Handler */
void DMA0_NSEC_Handler               (void) __attribute__ ((weak));
void ENCP_SEC_Handler                (void) __attribute__ ((weak));
void ENCP_NSEC_Handler               (void) __attribute__ ((weak));
void TIMER0_Handler                  (void) __attribute__ ((weak));
void UART0_Handler                   (void) __attribute__ ((weak));
void PWM0_Handler                    (void) __attribute__ ((weak));
void I2C0_Handler                    (void) __attribute__ ((weak));
void SPI0_Handler                    (void) __attribute__ ((weak));
void SARADC_Handler                  (void) __attribute__ ((weak));
void UART3_Handler                   (void) __attribute__ ((weak));
void SDIO_Handler                    (void) __attribute__ ((weak));
void GDMA_Handler                    (void) __attribute__ ((weak));
void LA_Handler                      (void) __attribute__ ((weak));
void TIMER1_Handler                  (void) __attribute__ ((weak));
void I2C1_Handler                    (void) __attribute__ ((weak));
void UART1_Handler                   (void) __attribute__ ((weak));
void UART2_Handler                   (void) __attribute__ ((weak));
void SPI1_Handler                    (void) __attribute__ ((weak));
void LED_Handler                     (void) __attribute__ ((weak));
void Anti_Tamper_Handler             (void) __attribute__ ((weak));
void CKMN_Handler                    (void) __attribute__ ((weak));
void I2S0_Handler                    (void) __attribute__ ((weak));
void PHY_MBP_Handler                 (void) __attribute__ ((weak));
void PHY_RIU_Handler                 (void) __attribute__ ((weak));
void MAC_INT_TX_RX_TIMER_Handler     (void) __attribute__ ((weak));
void MAC_INT_TX_RX_MISC_Handler      (void) __attribute__ ((weak));
void MAC_INT_RX_TRIGGER_Handler      (void) __attribute__ ((weak));
void MAC_INT_TX_TRIGGER_Handler      (void) __attribute__ ((weak));
void MAC_INT_PORT_TRIGGER_Handler    (void) __attribute__ ((weak));
void MAC_INT_GEN_Handler             (void) __attribute__ ((weak));
void HSU_Handler                     (void) __attribute__ ((weak));
void INT_MAC_WAKEUP_Handler          (void) __attribute__ ((weak));
void BTDM_Handler                    (void) __attribute__ ((weak));
void BLE_Handler                     (void) __attribute__ ((weak));
void BT_Handler                      (void) __attribute__ ((weak));
void QSPI0_Handler                   (void) __attribute__ ((weak));
void THREAD_Handler                  (void) __attribute__ ((weak));
void OTP_Handler                     (void) __attribute__ ((weak));
void DPLL_UNLOCK_Handler             (void) __attribute__ ((weak));
void GPIO_Handler                    (void) __attribute__ ((weak));
void GPIO_NS_Handler                 (void) __attribute__ ((weak));
void ANA_GPIO_Handler                (void) __attribute__ ((weak));
void ANA_RTC_Handler                 (void) __attribute__ ((weak));
void ABNORMAL_GPIO_Handler           (void) __attribute__ ((weak));
void ABNORMAL_RTC_Handler            (void) __attribute__ ((weak));
/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/

#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

const VECTOR_TABLE_Type __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
  (VECTOR_TABLE_Type)(&__INITIAL_SP),       /*     Initial Stack Pointer */
  Reset_Handler,                            /*     Reset Handler */
  NMI_Handler,                              /* -14 NMI Handler */
  HardFault_Handler,                        /* -13 Hard Fault Handler */
  MemManage_Handler,                        /* -12 MPU Fault Handler */
  BusFault_Handler,                         /* -11 Bus Fault Handler */
  UsageFault_Handler,                       /* -10 Usage Fault Handler */
  SecureFault_Handler,                      /*  -9 Secure Fault Handler */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  SVC_Handler,                              /*  -5 SVCall Handler */
  DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
  0,                                        /*     Reserved */
  PendSV_Handler,                           /*  -2 PendSV Handler */
  SysTick_Handler,                          /*  -1 SysTick Handler */

  /* Interrupts */
  DMA0_NSEC_Handler,                        /* 0 DMA0_NSEC_Handler */
  ENCP_SEC_Handler,                         /* 1 ENCP_SEC_Handler */
  ENCP_NSEC_Handler,                        /* 2 ENCP_NSEC_Handler */
  TIMER0_Handler,                           /* 3 TIMER0_Handler */
  UART0_Handler,                            /* 4 UART0_Handler */
  PWM0_Handler,                             /* 5 PWM0_Handler */
  I2C0_Handler,                             /* 6 I2C0_Handler */
  SPI0_Handler,                             /* 7 SPI0_Handler */
  SARADC_Handler,                           /* 8 SARADC_Handler */
  UART3_Handler,                            /* 9 UART3_Handler */
  SDIO_Handler,                             /* 10 SDIO_Handler */
  GDMA_Handler,                             /* 11 GDMA_Handler */
  LA_Handler,                               /* 12 LA_Handler */
  TIMER1_Handler,                           /* 13 TIMER1_Handler */
  I2C1_Handler,                             /* 14 I2C1_Handler */
  UART1_Handler,                            /* 15 UART1_Handler */
  UART2_Handler,                            /* 16 UART2_Handler */
  SPI1_Handler,                             /* 17 SPI1_Handler */
  LED_Handler,                              /* 18 LED_Handler */
  Anti_Tamper_Handler,                      /* 19 Anti_Tamper_Handler*/
  0,                                        /* 20 Reserved */
  CKMN_Handler,                             /* 21 CKMN_Handler */
  0,                                        /* 22 Reserved */
  0,                                        /* 23 Reserved */
  I2S0_Handler,                             /* 24 I2S0_Handler */
  0,                                        /* 25 Reserved */
  0,                                        /* 26 Reserved */
  0,                                        /* 27 Reserved */
  0,                                        /* 28 Reserved */
  PHY_MBP_Handler,                          /* 29 PHY_MBP_Handler */
  PHY_RIU_Handler,                          /* 30 PHY_RIU_Handler */
  MAC_INT_TX_RX_TIMER_Handler,              /* 31 MAC_INT_TX_RX_TIMER_Handler */
  MAC_INT_TX_RX_MISC_Handler,               /* 32 MAC_INT_TX_RX_MISC_Handler */
  MAC_INT_RX_TRIGGER_Handler,               /* 33 MAC_INT_RX_TRIGGER_Handler */
  MAC_INT_TX_TRIGGER_Handler,               /* 34 MAC_INT_TX_TRIGGER_Handler */
  MAC_INT_PORT_TRIGGER_Handler,             /* 35 MAC_INT_PORT_TRIGGER_Handler */
  MAC_INT_GEN_Handler,                      /* 36 MAC_INT_GEN_Handler */
  HSU_Handler,                              /* 37 HSU_Handler */
  INT_MAC_WAKEUP_Handler,                   /* 38 INT_MAC_WAKEUP_Handler */
  BTDM_Handler,                             /* 39 BTDM_Handler */
  BLE_Handler,                              /* 40 BLE_Handler */
  BT_Handler,                               /* 41 BT_Handler */
  QSPI0_Handler,                            /* 42 QSPI0_Handler */
  0,                                        /* 43 Reserved */
  0,                                        /* 44 Reserved */
  0,                                        /* 45 Reserved */
  0,                                        /* 46 Reserved */
  0,                                        /* 47 Reserved */
  /* BK7236 legacy download mode requires that the flash offset 0x100 is 'BK7236'.*/
  (void (*)(void))0x32374B42,
  (void (*)(void))0x00003633,
  OTP_Handler,                              /* 50 OTP_Handler */
  DPLL_UNLOCK_Handler,                      /* 51 DPLL_UNLOCK_Handler */
  0,                                        /* 52 Reserved */
  0,                                        /* 53 Reserved */
  0,                                        /* 54 Reserved */
  GPIO_Handler,                             /* 55 GPIO_S_Handler */
  GPIO_NS_Handler,                          /* 56 GPIO_NS_Handler */
  0,                                        /* 57 Reserved */
  ANA_GPIO_Handler,                         /* 58 ANA_GPIO_Handler */
  ANA_RTC_Handler,                          /* 59 ANA_RTC_Handler */
  ABNORMAL_GPIO_Handler,                    /* 60 ABNORMAL_GPIO_Handler */
  ABNORMAL_RTC_Handler,                     /* 61 ABNORMAL_RTC_Handler */
  0,                                        /* 62 Reserved */
  0,                                        /* 63 Reserved */
  /* Interrupts 64 .. 480 are left out */
};

__attribute__((used, section(".vectors_iram"))) \
const VECTOR_TABLE_Type __VECTOR_TABLE_IRAM[] = {
  (VECTOR_TABLE_Type)(&__INITIAL_SP),       /*     Initial Stack Pointer */
  Reset_Handler,                            /*     Reset Handler */
  NMI_Handler,                              /* -14 NMI Handler */
  HardFault_Handler,                        /* -13 Hard Fault Handler */
  MemManage_Handler,                        /* -12 MPU Fault Handler */
  BusFault_Handler,                         /* -11 Bus Fault Handler */
  UsageFault_Handler,                       /* -10 Usage Fault Handler */
  SecureFault_Handler,                      /*  -9 Secure Fault Handler */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  SVC_Handler,                              /*  -5 SVCall Handler */
  DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
  0,                                        /*     Reserved */
  PendSV_Handler,                           /*  -2 PendSV Handler */
  SysTick_Handler,                          /*  -1 SysTick Handler */

  /* Interrupts */
  DMA0_NSEC_Handler,                        /* 0 DMA0_NSEC_Handler */
  ENCP_SEC_Handler,                         /* 1 ENCP_SEC_Handler */
  ENCP_NSEC_Handler,                        /* 2 ENCP_NSEC_Handler */
  TIMER0_Handler,                           /* 3 TIMER0_Handler */
  UART0_Handler,                            /* 4 UART0_Handler */
  PWM0_Handler,                             /* 5 PWM0_Handler */
  I2C0_Handler,                             /* 6 I2C0_Handler */
  SPI0_Handler,                             /* 7 SPI0_Handler */
  SARADC_Handler,                           /* 8 SARADC_Handler */
  UART3_Handler,                            /* 9 UART3_Handler */
  SDIO_Handler,                             /* 10 SDIO_Handler */
  GDMA_Handler,                             /* 11 GDMA_Handler */
  LA_Handler,                               /* 12 LA_Handler */
  TIMER1_Handler,                           /* 13 TIMER1_Handler */
  I2C1_Handler,                             /* 14 I2C1_Handler */
  UART1_Handler,                            /* 15 UART1_Handler */
  UART2_Handler,                            /* 16 UART2_Handler */
  SPI1_Handler,                             /* 17 SPI1_Handler */
  LED_Handler,                              /* 18 LED_Handler */
  Anti_Tamper_Handler,                      /* 19 Anti_Tamper_Handler*/
  0,                                        /* 20 Reserved */
  CKMN_Handler,                             /* 21 CKMN_Handler */
  0,                                        /* 22 Reserved */
  0,                                        /* 23 Reserved */
  I2S0_Handler,                             /* 24 I2S0_Handler */
  0,                                        /* 25 Reserved */
  0,                                        /* 26 Reserved */
  0,                                        /* 27 Reserved */
  0,                                        /* 28 Reserved */
  PHY_MBP_Handler,                          /* 29 PHY_MBP_Handler */
  PHY_RIU_Handler,                          /* 30 PHY_RIU_Handler */
  MAC_INT_TX_RX_TIMER_Handler,              /* 31 MAC_INT_TX_RX_TIMER_Handler */
  MAC_INT_TX_RX_MISC_Handler,               /* 32 MAC_INT_TX_RX_MISC_Handler */
  MAC_INT_RX_TRIGGER_Handler,               /* 33 MAC_INT_RX_TRIGGER_Handler */
  MAC_INT_TX_TRIGGER_Handler,               /* 34 MAC_INT_TX_TRIGGER_Handler */
  MAC_INT_PORT_TRIGGER_Handler,             /* 35 MAC_INT_PORT_TRIGGER_Handler */
  MAC_INT_GEN_Handler,                      /* 36 MAC_INT_GEN_Handler */
  HSU_Handler,                              /* 37 HSU_Handler */
  INT_MAC_WAKEUP_Handler,                   /* 38 INT_MAC_WAKEUP_Handler */
  BTDM_Handler,                             /* 39 BTDM_Handler */
  BLE_Handler,                              /* 40 BLE_Handler */
  BT_Handler,                               /* 41 BT_Handler */
  QSPI0_Handler,                            /* 42 QSPI0_Handler */
  0,                                        /* 43 Reserved */
  0,                                        /* 44 Reserved */
  0,                                        /* 45 Reserved */
  0,                                        /* 46 Reserved */
  0,                                        /* 47 Reserved */
  THREAD_Handler,                           /* 48 THREAD_Handler */
  0,                                        /* 49 Reserved */
  OTP_Handler,                              /* 50 OTP_Handler */
  DPLL_UNLOCK_Handler,                      /* 51 DPLL_UNLOCK_Handler */
  0,                                        /* 52 Reserved */
  0,                                        /* 53 Reserved */
  0,                                        /* 54 Reserved */
  GPIO_Handler,                             /* 55 GPIO_S_Handler */
  GPIO_NS_Handler,                          /* 56 GPIO_NS_Handler */
  0,                                        /* 57 Reserved */
  ANA_GPIO_Handler,                         /* 58 ANA_GPIO_Handler */
  ANA_RTC_Handler,                          /* 59 ANA_RTC_Handler */
  ABNORMAL_GPIO_Handler,                    /* 60 ABNORMAL_GPIO_Handler */
  ABNORMAL_RTC_Handler,                     /* 61 ABNORMAL_RTC_Handler */
  0,                                        /* 62 Reserved */
  0,                                        /* 63 Reserved */
  /* Interrupts 64 .. 480 are left out */
};


#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#if CONFIG_CM_BACKTRACE
#define FIREWARE_NAME    "app"
#define HARDWARE_VERSION "V1.0.0"
#define SOFTWARE_VERSION "V1.0.0"
#endif

#define ENTRY_SECTION  __attribute__((section(".fix.reset_entry")))

void set_reboot_tag(uint32_t tag)
{
    REG_WRITE(REBOOT_TAG_ADDR, tag);
}

uint32_t sys_get_reboot_tag(void)
{
    return REG_READ(REBOOT_TAG_ADDR);
}

/* sets mem-repair module use OTP param if it has been set by CP-TEST. */
static inline void boot_mem_check(void)
{
    if(GET_BIT_VAL(REG_READ(SYSTEM_BASE_ADDRESS + 0x10*4), BIT_ID(3)) != 0)
    {
        REG_WRITE((SYSTEM_BASE_ADDRESS + 0x10*4), SET_BIT_VAL(REG_READ(SYSTEM_BASE_ADDRESS + 0x10*4), BIT_ID(3), 0));
    }

    if(GET_BIT_VAL(REG_READ(SYSTEM_BASE_ADDRESS + 0xC*4), BIT_ID(15)) != 1)
    {
        REG_WRITE((SYSTEM_BASE_ADDRESS + 0xC*4), SET_BIT_VAL(REG_READ(SYSTEM_BASE_ADDRESS + 0xC*4), BIT_ID(15), 1));
    }

    if((GET_BIT_VAL(REG_READ(OTP_APB_BASE_ADDRESS + 0xB2*4), BIT_ID(0)) != 1)||(GET_BIT_VAL(REG_READ(OTP_APB_BASE_ADDRESS + 0xB2*4), BIT_ID(1)) != 1))
    {
        REG_WRITE((OTP_APB_BASE_ADDRESS + 0xB2*4), SET_BIT_VAL(REG_READ(OTP_APB_BASE_ADDRESS + 0xB2*4), BIT_ID(0), 3));
    }

    if(GET_BIT_VAL(REG_READ(OTP_APB_BASE_ADDRESS + 0xB1*4), BIT_ID(0)) == 0)
    {
        if((REG_READ(OTP_APB_BASE_ADDRESS + 0x7c8) & 0xF ) == 0x7)
        {
            REG_WRITE((MEM_CHECK_BASE_ADDRESS + 0x2*4), 0x7);
        }
    }
}

static uint32_t relocate_vector_table(void)
{
    uint32_t vtor_addr;

    vtor_addr = (uint32_t) &(__VECTOR_TABLE_IRAM[0]);
    SCB->VTOR = vtor_addr;

    return vtor_addr;
}

uint32_t memory_is_retention(void)
{
	return ((REG_READ(0x2807E400 + (16 << 2)) == 0x5A5A5A5A)
			&& (REG_READ(0x2807E400 + (17 << 2)) == 0xA5A5A5A5));
}

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
__NO_RETURN ENTRY_SECTION void Reset_Handler(void)
{
    #if CONFIG_DEEP_LV
    if (dlv_is_startup() && memory_is_retention()) {
        __set_MSPLIM((uint32_t)(&__STACK_LIMIT));
        /* CMSIS System Initialization */
        SystemInit_iram();
    }
#endif

#if CONFIG_AON_PMU_POR_TIMING_SUPPORT
    aon_pmu_driver_save_time(true);
#endif

    STARTUP_PERF(15);
    __set_MSPLIM((uint32_t)(&__STACK_LIMIT));

    /* CMSIS System Initialization */
    SystemInit();

#if (CONFIG_INT_WDT || CONFIG_TASK_WDT)
    bk_wdt_force_feed();
#endif
    //clear mannully reboot flag
    set_reboot_tag(0);

    sys_drv_early_init();

#if CONFIG_SUPPORT_PSRAM_XIP
    bk_psram_init();
#endif

    /* Enter PreMain (C library entry point) */
    __PROGRAM_START();
}

void _start(void)
{
#if CONFIG_MPU
    mpu_enable();
#endif // CONFIG_MPU

#if CONFIG_DCACHE
    if (SCB->CLIDR & SCB_CLIDR_DC_Msk)
      SCB_EnableDCache();

    SCB_CleanInvalidateDCache();
#endif

#if CONFIG_CM_BACKTRACE
    cm_backtrace_init(FIREWARE_NAME, HARDWARE_VERSION, SOFTWARE_VERSION);
#endif

#if CONFIG_ATE_TEST && CONFIG_RESET_REASON
    extern int cmd_do_memcheck(void);
    cmd_do_memcheck();
#endif

#if CONFIG_SYS_CPU0
    /*power manager init*/
    pm_hardware_init();

    bk_pm_cp1_auto_power_down_state_set(PM_CP1_AUTO_CTRL_DISABLE);
    bk_pm_mem_auto_power_down_state_set(PM_MEM_AUTO_CTRL_DISABLE);
#endif

    relocate_vector_table();

    entry_main();

    while(1){
        BK_LOGW(TAG, "@\r\n");
    };
}

void user_except_handler_ex(uint32_t reset_reason, uint32_t lr, uint32_t sp);

__STATIC_FORCEINLINE void dump_system_info(uint32_t rr, uint32_t lr, uint32_t sp) {
    user_except_handler_ex(rr, lr, sp);
}

#define dump_fault_info(rr) \
{\
    uint32_t lr = __get_LR();\
    uint32_t sp = __get_MSP();\
\
    __asm volatile \
    (\
        "   push {r4-r11}                               \n"\
    );\
\
    dump_system_info(rr, lr, sp);\
\
    while(1);\
}

void user_nmi_handler(uint32_t lr, uint32_t sp)
{
#if CONFIG_DEBUG_FIRMWARE || CONFIG_DUMP_ENABLE
    if(arch_is_enter_exception())
    {
        //For nmi wdt reset
        aon_pmu_drv_wdt_change_not_rosc_clk();
        aon_pmu_drv_wdt_rst_dev_enable();
        while(1);
    }

    if(REBOOT_TAG_REQ == sys_get_reboot_tag()) {
        while(1);
    }

#if CONFIG_WDT_EN
    bk_wdt_feed();
#endif

    dump_system_info(RESET_SOURCE_NMI_WDT, lr, sp);
#else // nmi wdt without system info dump
    if(REBOOT_TAG_REQ != sys_get_reboot_tag()) {
        bk_misc_set_reset_reason(RESET_SOURCE_NMI_WDT);
    }

    aon_pmu_drv_wdt_change_not_rosc_clk();
    aon_pmu_drv_wdt_rst_dev_enable();
    while(1);

#endif // CONFIG_DEBUG_FIRMWARE || CONFIG_DUMP_ENABLE
}

/*----------------------------------------------------------------------------
  Hard Fault Handler
 *----------------------------------------------------------------------------*/
__attribute__((naked)) void HardFault_Handler(void)
{
    dump_fault_info(RESET_SOURCE_HARD_FAULT);
}

__attribute__((naked, section(".itcm")))  void NMI_Handler(void)
{
    uint32_t lr = __get_LR();
    uint32_t sp = __get_MSP();

    __asm volatile
    (
        "   push {r4-r11}                               \n"
    );

    user_nmi_handler(lr, sp);

    while(1);
}

__attribute__((naked)) void MemManage_Handler(void)
{
    dump_fault_info(RESET_SOURCE_MPU_FAULT);
}

__attribute__((naked)) void BusFault_Handler(void)
{
    dump_fault_info(RESET_SOURCE_BUS_FAULT);
}

__attribute__((naked)) void UsageFault_Handler(void)
{
    dump_fault_info(RESET_SOURCE_USAGE_FAULT);
}

__attribute__((naked)) void SecureFault_Handler(void)
{
    dump_fault_info(RESET_SOURCE_SECURE_FAULT);
}

#if CONFIG_SOC_CORTEX_M_UART_DEBUG
/* debug_monitor_exception.c*/
#include "debug_monitor_exception.h"

void debug_monitor_handler_c(CONTEXT_FRAME_T *frame);

__attribute__((naked)) void DebugMon_Handler(void)
{
  __asm volatile(
      "tst lr, #4 \n"
      "ite eq \n"
      "mrseq r0, msp \n"
      "mrsne r0, psp \n"
      "b debug_monitor_handler_c \n");
}

#else
__attribute__((naked)) void DebugMon_Handler(void)
{
    dump_fault_info(RESET_SOURCE_DEBUG_MONITOR_FAULT);
}
#endif // CONFIG_SOC_CORTEX_M_UART_DEBUG

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
__attribute__((naked)) void Default_Handler(void)
{
    dump_fault_info(RESET_SOURCE_DEFAULT_EXCEPTION);
}
// eof

