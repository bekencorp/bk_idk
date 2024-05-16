/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <stdio.h>
#include "cmsis.h"
#include "target_cfg.h"
#include "tfm_hal_platform.h"
#include "tfm_plat_defs.h"
#include "uart_stdout.h"
#include "core_star.h"
#include "security.h"
#include "prro.h"
#include "partitions.h"
#if CONFIG_TFM_BK7236_V5
#include "tfm_hal_ppc.h"
#endif


extern const struct memory_region_limits memory_regions;
/*
	MCLK:26MHz, delay(1): about 25us
				delay(10):about 125us
				delay(100):about 850us
 */
void delay(uint32_t num)
{
	volatile uint32_t i, j;

	for (i = 0; i < num; i ++) {
		for (j = 0; j < 100; j ++)
			;
	}
}

#ifdef TFM_FIH_PROFILE_ON
fih_int tfm_hal_platform_init(void)
#else
enum tfm_hal_status_t tfm_hal_platform_init(void)
#endif
{
    enum tfm_plat_err_t plat_err = TFM_PLAT_ERR_SYSTEM_ERR;
#ifdef TFM_FIH_PROFILE_ON
    fih_int fih_rc = FIH_FAILURE;
#endif

    plat_err = enable_fault_handlers();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

    plat_err = system_reset_cfg();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

#ifdef TFM_FIH_PROFILE_ON
    FIH_CALL(init_debug, fih_rc);
    if (fih_not_eq(fih_rc, fih_int_encode(TFM_PLAT_ERR_SUCCESS))) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }
#else
    plat_err = init_debug();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }
#endif

    __enable_irq();
    stdio_init();
    bk_prro_driver_init();

    plat_err = nvic_interrupt_target_state_cfg();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

    plat_err = nvic_interrupt_enable();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

	bk_flash_driver_init();
	load_partition_from_flash();

    FIH_RET(fih_int_encode(TFM_HAL_SUCCESS));
}

uint32_t tfm_hal_get_ns_VTOR(void)
{
    return memory_regions.non_secure_code_start;
}

uint32_t tfm_hal_get_ns_MSP(void)
{
    return *((uint32_t *)memory_regions.non_secure_code_start);
}

extern uint32_t piece_address(uint8_t *array,uint32_t index);
void tfm_hal_ppc_init_from_flash(uint32_t* reg)
{
	for(int i=0;i<12;i++){
		*((volatile uint32_t *)(0x41040000 + (4+i) * 4)) = reg[i];
	}
}

void tfm_s_2_ns_hook(void)
{
	uint8_t* buf = (uint8_t*)malloc(48*sizeof(uint8_t));
	uint32_t* reg = (uint32_t*)malloc(12*sizeof(uint32_t));
	if(buf == NULL || reg == NULL){
		printf("memory malloc fails.\r\n");
	}
	bk_flash_read_bytes(CONFIG_PARTITION_PHY_PARTITION_OFFSET,buf,48);
	for(int i=0;i<12;i++){
		reg[i] = piece_address(buf,0+i*4);
	}
	
	/* dma secure attributes only are configured at the spe mode */
	*((volatile uint32_t *)(0x45020000 + 2 * 4)) = 0;
	*((volatile uint32_t *)(0x45020000 + 2 * 4)) = 1; /* soft reset dma0 module */
	*((volatile uint32_t *)(0x45020000 + 5 * 4)) = 0xFFF;
	*((volatile uint32_t *)(0x45020000 + 4 * 4)) = 0;

	*((volatile uint32_t *)(0x45030000 + 2 * 4)) = 0;
	*((volatile uint32_t *)(0x45030000 + 2 * 4)) = 1; /* soft reset dma1 module */
	*((volatile uint32_t *)(0x45030000 + 5 * 4)) = 0xFFF;
	*((volatile uint32_t *)(0x45030000 + 4 * 4)) = 0;

	*((volatile uint32_t *)(0x41040000 + 2 * 4)) = 1; /* soft reset ppro module */

#if CONFIG_TFM_BK7236_V5
	// *((volatile uint32_t *)(0x41040000 + 4 * 4)) = 0xFFFFFFFC; /* gpio_nonsec0  gpio0/1:uart1*/
	// *((volatile uint32_t *)(0x41040000 + 5 * 4)) = 0xFFFFFFFF; /* gpio_nonsec1 */

    //     *((volatile uint32_t *)(0x41040000 + 6 * 4)) = 0x080FFF6F;
    //     *((volatile uint32_t *)(0x41040000 + 7 * 4)) = 0x187;
    //     *((volatile uint32_t *)(0x41040000 + 8 * 4)) = 0x10003FFB;
    //     *((volatile uint32_t *)(0x41040000 + 9 * 4)) = 0x9E03F;
    //     *((volatile uint32_t *)(0x41040000 + 10 * 4)) = 0x21F9;
    //     *((volatile uint32_t *)(0x41040000 + 11 * 4)) = 0x2E00FF;
    //     *((volatile uint32_t *)(0x41040000 + 12 * 4)) = 0x13;
    //     *((volatile uint32_t *)(0x41040000 + 13 * 4)) = 0xB9;
    //     *((volatile uint32_t *)(0x41040000 + 14 * 4)) = 0x1;
    //     *((volatile uint32_t *)(0x41040000 + 15 * 4)) = 0x1FFF;

	tfm_hal_ppc_init_from_flash(reg);
	os_free(reg);
	os_free(buf);
#else
	*((volatile uint32_t *)(0x41040000 + 4 * 4)) = 0; /* privilege setting*/
	*((volatile uint32_t *)(0x41040000 + 5 * 4)) = 0xFFFFFFFF;
	*((volatile uint32_t *)(0x41040000 + 8 * 4)) = 0xFFFFFEFF; /*[2]uart1 for spe*/
	*((volatile uint32_t *)(0x41040000 + 10 * 4)) = (0xF);     /*[3:0], bit3:wdt*/
	*((volatile uint32_t *)(0x41040000 + 11 * 4)) = 0xFFFFFFFC;/*gpio_nonsec0  gpio0/1:uart1*/
	*((volatile uint32_t *)(0x41040000 + 12 * 4)) = 0xFFFFFFFF;/*gpio_nonsec1*/

	*((volatile uint32_t *)(0x41040000 + 13 * 4)) = 0x01      ;/*jp enc*/
	*((volatile uint32_t *)(0x41040000 + 14 * 4)) = 0x01      ;/*jp dec*/
	*((volatile uint32_t *)(0x41040000 + 15 * 4)) = 0x01      ;/*dma d_m*/
	*((volatile uint32_t *)(0x41040000 + 16 * 4)) = 0x01      ;/*mac_m*/
	*((volatile uint32_t *)(0x41040000 + 17 * 4)) = 0x01      ;/*hsu_m*/
	*((volatile uint32_t *)(0x41040000 + 18 * 4)) = 0x01      ;/*btdm_m*/
	*((volatile uint32_t *)(0x41040000 + 19 * 4)) = 0x01      ;/*la_m*/
	*((volatile uint32_t *)(0x41040000 + 20 * 4)) = 0x01      ;/*disp_m*/
	*((volatile uint32_t *)(0x41040000 + 21 * 4)) = 0x00      ;/*enc shanghai*/
#endif
	bk_flash_set_base_addr(0x54030000);
	sys_drv_set_base_addr(0x54010000);
	/* excepions target the Non-secure hardfault exception.BusFault, HardFault,
	 * and NMI Non-secure enable.
	 * SCB_AIRCR_PRIS_Msk, if bit[14]=0, invis fault. TODO wangzhilei
	 */
	uint32_t reg_val = SCB->AIRCR;
	reg_val &= (~(uint32_t)SCB_AIRCR_VECTKEYSTAT_Msk);
	reg_val |= (uint32_t)((0x5FAUL << SCB_AIRCR_VECTKEY_Pos)
						| SCB_AIRCR_BFHFNMINS_Msk
						| SCB_AIRCR_PRIS_Msk);
	SCB->AIRCR = reg_val;

	/* Permit Non-secure access to the Floating-point Extension.
	* Note: It is still necessary to set CPACR_NS to enable the FP Extension
	* in the NSPE. This configuration is left to NS privileged software.
	*/
	SCB->NSACR |= SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk;
	printf("SCB->AIRCR:0x%x\r\n", SCB->AIRCR);
}

uint32_t tfm_hal_get_ns_entry_point(void)
{
#if CONFIG_REG_ACCESS_NSC
	void psa_reg_nsc_stub(void);
	psa_reg_nsc_stub();
#endif
#if CONFIG_MPC_NSC
	void psa_mpc_nsc_stub(void);
	psa_mpc_nsc_stub();
#endif
#if CONFIG_MPC_NSC
	void psa_flash_nsc_stub(void);
	psa_flash_nsc_stub();
#endif
#if CONFIG_INT_TARGET_NSC
	void psa_int_target_nsc_stub(void);
	psa_int_target_nsc_stub();
#endif
	tfm_s_2_ns_hook();
	return *((uint32_t *)(memory_regions.non_secure_code_start + 4));
}
