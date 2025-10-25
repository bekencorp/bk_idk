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

/* System includes */
#include "bk_private/bk_init.h"    /**< Beken system initialization */
#include <components/system.h>      /**< System components */
#include <os/os.h>                  /**< Operating system interface */
#include "sys_ll.h"                 /**< System low-level functions */
#include "modules/pm.h"             /**< Power management module */

/* External function declarations */
extern void rtos_set_user_app_entry(beken_thread_function_t entry); /**< Set RTOS user application entry point */
extern void flash_pressure_test_start(void);                        /**< Start Flash pressure test */
extern void cli_resettable_wifi_start(void);                       /**< Start CLI resettable WiFi functionality */

/**
 * @brief User application main function
 * 
 * This function is called after system initialization is complete, responsible for:
 * - Configure CPU frequency to 160MHz
 * - Set Flash clock frequency to 80MHz
 * - Start Flash pressure test
 * - Start CLI resettable WiFi functionality
 * 
 * @note This function runs in RTOS environment as the main entry point for user application
 */
void user_app_main(void)
{
	/* Set CPU frequency to 120MHz */
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_120M);

	/* Configure Flash clock frequency to 80MHz */
	sys_ll_set_cpu_clk_div_mode2_ckdiv_flash(0);  /* Set division factor to 0 */
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(2);  /* Select clock source: 0/1:40MHz, 2:80MHz, 3:120MHz */

	/* Start Flash pressure test */
	flash_pressure_test_start();
	
	/* Start CLI resettable WiFi functionality */
	cli_resettable_wifi_start();
}

/**
 * @brief Application main entry function
 * 
 * Main entry point during system startup, responsible for:
 * - Performance monitoring initialization (for BK7236 series chips)
 * - Set RTOS user application entry function (CPU0)
 * - Call system initialization function
 * 
 * @return int Always returns 0, indicating normal exit
 * 
 * @note This function is called during system startup, after basic initialization
 *       the user application will run through RTOS task scheduling mechanism
 */
int main(void)
{
	/* Set RTOS user application entry function */
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);

	/* Execute system initialization */
	bk_init();

	return 0;
}