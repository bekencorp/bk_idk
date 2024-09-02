// Copyright 2020-2021 Beken
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

#include <common/bk_include.h>
#include <driver/pwr_clk.h>
#if CONFIG_MAILBOX
#include <driver/mailbox_channel.h>
#endif
#include <modules/pm.h>
#include "sys_driver.h"
#if CONFIG_PSRAM
#include <driver/psram.h>
#endif
#include <os/mem.h>
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <driver/psram.h>
#include <components/system.h>
#include "bk_pm_internal_api.h"
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"

/*=====================DEFINE  SECTION  START=====================*/
#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms
#define PM_BOOT_CP1_WAITING_TIEM             (2000) // 2s
#define PM_CP1_RECOVERY_DEFAULT_VALUE        (0xFFFFFFFFFFFFFFFF)
#define PM_OPEN_CP1_TIMEOUT                  (20000) //20s
#define PM_SEMA_WAIT_FOREVER                 (0xFFFFFFFF)    /*Wait Forever*/
/*=====================DEFINE  SECTION  END=====================*/

/*=====================VARIABLE  SECTION  START=================*/
#if CONFIG_SYS_CPU1
static volatile  pm_mailbox_communication_state_e s_pm_cp1_pwr_finish            = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_clk_finish            = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_sleep_finish          = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_cpu_freq_finish       = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_init                  = 0;
static volatile  pm_mailbox_communication_state_e s_pm_external_ldo_ctrl         = 0;
static volatile  pm_mailbox_communication_state_e s_pm_psram_power_ctrl          = 0;
#elif CONFIG_SYS_CPU0
static volatile  pm_mailbox_communication_state_e s_pm_cp1_boot_ready            = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_psram_malloc_state    = 0;
static volatile  uint32_t                         s_pm_cp1_psram_malloc_count    = 0;
static volatile  uint64_t                         s_pm_cp1_module_recovery_state = PM_CP1_RECOVERY_DEFAULT_VALUE;
#if (CONFIG_CPU_CNT > 1)
static beken_semaphore_t                          s_sync_cp1_open_sema           = NULL;
#endif
static volatile  uint32_t                         s_pm_cp1_closing               = 0;
static volatile  uint32_t                         s_pm_cp1_sema_count            = 0;
#else
static volatile  pm_mailbox_communication_state_e s_pm_cp1_boot_ready            = 0;
#endif

/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/

#if CONFIG_SYS_CPU1 && CONFIG_MAILBOX
static void pm_cp1_mailbox_init();
bk_err_t pm_cp1_mailbox_response(uint32_t cmd, int ret);
bk_err_t bk_pm_cp1_ctrl_state_set(pm_mailbox_communication_state_e state);
pm_mailbox_communication_state_e bk_pm_cp1_ctrl_state_get();
static void pm_cp1_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3);
#endif

#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
#if CONFIG_MAILBOX
static void pm_cp0_mailbox_init();
#endif
static void pm_module_shutdown_cpu1(pm_power_module_name_e module);
bk_err_t bk_pm_cp1_recovery_module_state_ctrl(pm_cp1_prepare_close_module_name_e module,pm_cp1_module_recovery_state_e state);
#endif
/*================FUNCTION DECLARATION  SECTION  END========*/

pm_lpo_src_e bk_clk_32k_customer_config_get(void)
{
#if CONFIG_LPO_MP_A_FORCE_USE_EXT32K
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK))
	{
		return PM_LPO_SRC_X32K;
	}
	else
	{
		#if CONFIG_EXTERN_32K
			return PM_LPO_SRC_X32K;
		#elif CONFIG_LPO_SRC_26M32K
			return PM_LPO_SRC_DIVD;
		#else
			return PM_LPO_SRC_ROSC;
		#endif
	}
#else
	#if CONFIG_EXTERN_32K
		return PM_LPO_SRC_X32K;
	#elif CONFIG_LPO_SRC_26M32K
		return PM_LPO_SRC_DIVD;
	#else
		return PM_LPO_SRC_ROSC;
	#endif
#endif
	return PM_LPO_SRC_ROSC;
}
bk_err_t bk_pm_mailbox_init()
{
#if CONFIG_SYS_CPU1
	/*cp1 mailbox init*/
#if CONFIG_MAILBOX
	pm_cp1_mailbox_init();

#endif
#endif

#if CONFIG_SYS_CPU0
	/*cp0 mailbox init*/
	#if CONFIG_MAILBOX && (CONFIG_CPU_CNT > 1)
	pm_cp0_mailbox_init();
	#endif //CONFIG_MAILBOX

#endif
	return BK_OK;
}

#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX
bk_err_t bk_pm_cp1_boot_ok_response_set()
{
	if(bk_pm_cp1_ctrl_state_get() == 0x0)
	{
		bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
		pm_cp1_mailbox_response(PM_CPU1_BOOT_READY_CMD, 0x1);
	}
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_pwr_ctrl_state_get()
{
	return s_pm_cp1_pwr_finish;
}
bk_err_t bk_pm_cp1_pwr_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_pwr_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_clk_ctrl_state_get()
{
	return s_pm_cp1_clk_finish;
}
 bk_err_t bk_pm_cp1_clk_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_clk_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_sleep_ctrl_state_get()
{
	return s_pm_cp1_sleep_finish;
}
bk_err_t bk_pm_cp1_sleep_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_sleep_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_cpu_freq_ctrl_state_get()
{
	return s_pm_cp1_cpu_freq_finish;
}
bk_err_t bk_pm_cp1_cpu_freq_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_cpu_freq_finish = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_external_ldo_ctrl_state_get()
{
	return s_pm_external_ldo_ctrl;
}
bk_err_t bk_pm_cp1_external_ldo_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_external_ldo_ctrl = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_psram_power_state_get()
{
	return s_pm_psram_power_ctrl;
}
bk_err_t bk_pm_cp1_psram_power_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_psram_power_ctrl = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_ctrl_state_get()
{
	return s_pm_cp1_init;
}
bk_err_t bk_pm_cp1_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_init = state;
	return BK_OK;
}
bk_err_t bk_pm_cp1_recovery_response(uint32_t cmd, pm_cp1_prepare_close_module_name_e module_name,pm_cp1_module_recovery_state_e state)
{
	pm_cp1_mailbox_send_data(cmd,module_name,state,0);
	return BK_OK;
}
static void pm_cp1_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3)
{
	bk_err_t ret = BK_OK;
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = param1;
	mb_cmd.param2 = param2;
	mb_cmd.param3 = param3;
	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
	os_printf("%s %d\r\n",__func__, ret);
}
bk_err_t pm_cp1_mailbox_response(uint32_t cmd, int ret)
{
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = ret;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
	return BK_OK;
}
static void pm_cp1_mailbox_tx_cmpl_isr(int *pm_mb, mb_chnl_ack_t *cmd_buf)
{
}
static void pm_cp1_mailbox_rx_isr(int *pm_mb, mb_chnl_cmd_t *cmd_buf)
{
	bk_err_t ret = BK_OK;
	uint32_t used_count;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	switch(cmd_buf->hdr.cmd) {
		case PM_POWER_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_pwr_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CLK_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_clk_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		 case PM_SLEEP_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_sleep_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		 case PM_CPU_FREQ_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_cpu_freq_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CTRL_EXTERNAL_LDO_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_external_ldo_ctrl = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CTRL_PSRAM_POWER_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_psram_power_ctrl = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CP1_PSRAM_MALLOC_STATE_CMD:
			used_count = bk_psram_heap_get_used_count();
			pm_cp1_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,0x1,used_count,0);
			//os_printf("cp1 bk_psram_heap_get_used_count[%d]\r\n", bk_psram_heap_get_used_count());
			break;
		case PM_CP1_DUMP_PSRAM_MALLOC_INFO_CMD:
			bk_psram_heap_get_used_state();
			break;
		case PM_CP1_RECOVERY_CMD:
			stop_cpu1_handle_notifications();
			bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
			break;
		default:
			break;
	}
	GLOBAL_INT_RESTORE();

	if(ret != BK_OK)
	{
		os_printf("cp1 response: cp0 handle msg error\r\n");
	}
	//if(pm_debug_mode()&0x2)
	{
      if(cmd_buf->hdr.cmd != PM_CP1_PSRAM_MALLOC_STATE_CMD)
      {
		os_printf("enter cp1_mailbox_rx_isr %d %d %d \r\n",cmd_buf->hdr.cmd,cmd_buf->param1,cmd_buf->param2);
      }
	}
}
static void pm_cp1_mailbox_tx_isr(int *pm_mb)
{
}

static void pm_cp1_mailbox_init()
{
	mb_chnl_open(MB_CHNL_PWC, NULL);
	if (pm_cp1_mailbox_rx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_RX_ISR, pm_cp1_mailbox_rx_isr);
	if (pm_cp1_mailbox_tx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_ISR, pm_cp1_mailbox_tx_isr);
	if (pm_cp1_mailbox_tx_cmpl_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_CMPL_ISR, pm_cp1_mailbox_tx_cmpl_isr);
}
#endif //CONFIG_MAILBOX
#endif//CONFIG_SYS_CPU1


#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
pm_mailbox_communication_state_e bk_pm_cp1_work_state_get()
{
	return s_pm_cp1_boot_ready;
}
bk_err_t bk_pm_cp1_work_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_boot_ready = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp0_psram_malloc_state_get()
{
	return s_pm_cp1_psram_malloc_state;
}
bk_err_t bk_pm_cp0_psram_malloc_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_psram_malloc_state = state;
	return BK_OK;
}
#if CONFIG_MAILBOX
static void pm_cp0_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3)
{
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = param1;
	mb_cmd.param2 = param2;
	mb_cmd.param3 = param3;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
}

static void pm_cp0_mailbox_response(uint32_t cmd, int ret)
{
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = ret;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
}

static void pm_cp0_mailbox_tx_cmpl_isr(int *pm_mb, mb_chnl_ack_t *cmd_buf)
{
}

static void pm_cp0_mailbox_rx_isr(int *pm_mb, mb_chnl_cmd_t *cmd_buf)
{
	bk_err_t ret = BK_OK;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	switch(cmd_buf->hdr.cmd) {
		case PM_POWER_CTRL_CMD:
			ret = bk_pm_module_vote_power_ctrl(cmd_buf->param1,cmd_buf->param2);

			pm_cp0_mailbox_response(PM_POWER_CTRL_CMD,ret);

			break;
		case PM_CLK_CTRL_CMD:
			ret = bk_pm_clock_ctrl(cmd_buf->param1,cmd_buf->param2);
			pm_cp0_mailbox_response(PM_CLK_CTRL_CMD,ret);
			break;
		case PM_SLEEP_CTRL_CMD:
			ret = bk_pm_module_vote_sleep_ctrl(cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			pm_cp0_mailbox_response(PM_SLEEP_CTRL_CMD,ret);
			break;
		case PM_CPU_FREQ_CTRL_CMD:
			ret = bk_pm_module_vote_cpu_freq(cmd_buf->param1,cmd_buf->param2);
			pm_cp0_mailbox_response(PM_CPU_FREQ_CTRL_CMD,ret);
			break;
		case PM_CTRL_EXTERNAL_LDO_CMD:
			ret = bk_pm_module_vote_ctrl_external_ldo(cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			pm_cp0_mailbox_response(PM_CTRL_EXTERNAL_LDO_CMD,ret);
			break;
		case PM_CTRL_PSRAM_POWER_CMD:
			ret = bk_pm_module_vote_psram_ctrl(cmd_buf->param1,cmd_buf->param2);
			pm_cp0_mailbox_response(PM_CTRL_PSRAM_POWER_CMD,ret);
			break;
		case PM_CPU1_BOOT_READY_CMD:
			if(cmd_buf->param1 == 0x1)
			{
				s_pm_cp1_boot_ready = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			//if(pm_debug_mode()&0x2)//for temp debug
				os_printf("cpu0 receive the cpu1 boot success event [%d]\r\n",cmd_buf->param1);
			break;
		case PM_CP1_PSRAM_MALLOC_STATE_CMD:
			if(cmd_buf->param1 == 0x1)
			{
				bk_pm_cp0_psram_malloc_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
				s_pm_cp1_psram_malloc_count = cmd_buf->param2;
			}
			break;
		case PM_CP1_RECOVERY_CMD:
			bk_pm_cp1_recovery_module_state_ctrl(cmd_buf->param1,cmd_buf->param2);
			os_printf("%s PM_CP1_RECOVERY_CMD\r\n",__func__);
			break;
		default:
			break;
	}
	GLOBAL_INT_RESTORE();
	if(ret != BK_OK)
	{
		os_printf("cp0 handle cp1 message error\r\n");
	}

	//if(pm_debug_mode()&0x2)
	{
		if(cmd_buf->hdr.cmd != PM_CP1_PSRAM_MALLOC_STATE_CMD)
		{
			os_printf("enter cp0_mailbox_rx_isr %d %d %d %d %d\r\n",cmd_buf->hdr.cmd,cmd_buf->param1,cmd_buf->param2,cmd_buf->param3,ret);
		}
	}

}
static void pm_cp0_mailbox_tx_isr(int *pm_mb)
{
}
static void pm_cp0_mailbox_init()
{
	mb_chnl_open(MB_CHNL_PWC, NULL);
	if (pm_cp0_mailbox_rx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_RX_ISR, pm_cp0_mailbox_rx_isr);
	if (pm_cp0_mailbox_tx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_ISR, pm_cp0_mailbox_tx_isr);
	if (pm_cp0_mailbox_tx_cmpl_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_CMPL_ISR, pm_cp0_mailbox_tx_cmpl_isr);
}
#endif //CONFIG_MAILBOX
#endif //CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)


#if CONFIG_SYS_CPU0&& (CONFIG_CPU_CNT > 1)
static uint32_t s_pm_cp1_ctrl_state           = 0;
extern void start_cpu1_core(void);
extern void stop_cpu1_core(void);

bk_err_t bk_pm_cp1_recovery_module_state_ctrl(pm_cp1_prepare_close_module_name_e module,pm_cp1_module_recovery_state_e state)
{
	if(state == PM_CP1_MODULE_RECOVERY_STATE_INIT)
	{
		s_pm_cp1_module_recovery_state &= ~(0x1ULL << module);
	}
	else
	{
		s_pm_cp1_module_recovery_state |= (0x1ULL << module);
	}
	os_printf("%s 0x%llx %d %d %d\r\n",__func__,s_pm_cp1_module_recovery_state,bk_pm_cp1_work_state_get(),bk_pm_cp1_recovery_all_state_get(),s_pm_cp1_ctrl_state);
	if(bk_pm_cp1_recovery_all_state_get())
	{
		bk_pm_module_check_cp1_shutdown();
	}
	return BK_OK;
}

bool bk_pm_cp1_recovery_all_state_get()
{
	bool cp1_all_module_recovery = false;
	if(bk_pm_cp1_work_state_get())
	{
		cp1_all_module_recovery = (s_pm_cp1_module_recovery_state == PM_CP1_RECOVERY_DEFAULT_VALUE);
	}
	return cp1_all_module_recovery;
}

static void pm_module_bootup_cpu1(pm_power_module_name_e module)
{
	uint64_t previous_tick = 0;
	uint64_t current_tick   = 0;
	if(PM_POWER_MODULE_STATE_OFF == sys_drv_module_power_state_get(module))
	{
		if(module == PM_POWER_MODULE_NAME_CPU1)
		{
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_CPU1, 0, 0);
            bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_ON);
            start_cpu1_core();

			previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
			current_tick = previous_tick;
			while((current_tick - previous_tick) < (PM_BOOT_CP1_WAITING_TIEM*AON_RTC_MS_TICK_CNT))
			{
				if (bk_pm_cp1_work_state_get()) // wait the cp1 response
				{
					break;
				}
				current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
			}

			if(!bk_pm_cp1_work_state_get())
			{
				os_printf("cp0 boot cp1 time out, boot cp1 fail!!!\r\n");
			}
		}
	}
}
bk_err_t bk_pm_module_check_cp1_shutdown()
{
	if(0x0 == s_pm_cp1_ctrl_state)
	{
		pm_module_shutdown_cpu1(PM_POWER_MODULE_NAME_CPU1);
	}
    return BK_OK;
}
static void pm_module_shutdown_cpu1(pm_power_module_name_e module)
{
	bk_err_t ret = BK_OK;
	GLOBAL_INT_DECLARATION();
	if(PM_POWER_MODULE_STATE_ON == sys_drv_module_power_state_get(module))
	{
		if(module == PM_POWER_MODULE_NAME_CPU1)
		{
            stop_cpu1_core();
		    bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_OFF);

			GLOBAL_INT_DISABLE();
			s_pm_cp1_boot_ready = 0;
			s_pm_cp1_closing = 0;
			GLOBAL_INT_RESTORE();
			ret = rtos_set_semaphore(&s_sync_cp1_open_sema);
			if(s_pm_cp1_sema_count == 0)
			{
				rtos_deinit_semaphore(&s_sync_cp1_open_sema);
			}

			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_CPU1, 1, 0);
			os_printf("%s [%d][%d][%d]\r\n",__func__,s_pm_cp1_closing,ret,s_pm_cp1_sema_count);
		}
	}
}

bk_err_t bk_pm_module_vote_boot_cp1_ctrl(pm_boot_cp1_module_name_e module,pm_power_module_state_e power_state)
{
	bk_err_t ret = BK_OK;
	GLOBAL_INT_DECLARATION();

	os_printf("%s %d %d 0x%x [%d][0x%x]enter 1\r\n",__func__, module, power_state,s_pm_cp1_ctrl_state,s_pm_cp1_closing,&s_sync_cp1_open_sema);
	if (NULL == s_sync_cp1_open_sema)
	{
		rtos_init_semaphore(&s_sync_cp1_open_sema, 1);
	}
	if(s_pm_cp1_closing)
	{
		GLOBAL_INT_DISABLE();
		s_pm_cp1_sema_count++;
		GLOBAL_INT_RESTORE();
		os_printf("%s get semaphore[%d][0x%x]\r\n",__func__,s_pm_cp1_sema_count,&s_sync_cp1_open_sema);

		/*add protect:init again when the s_sync_cp1_open_sema free*/
		if (NULL == s_sync_cp1_open_sema)
		{
			rtos_init_semaphore(&s_sync_cp1_open_sema, 1);
		}
		ret = rtos_get_semaphore(&s_sync_cp1_open_sema, PM_OPEN_CP1_TIMEOUT);

		GLOBAL_INT_DISABLE();
		s_pm_cp1_sema_count--;
		GLOBAL_INT_RESTORE();
		if(ret == kTimeoutErr)
		{
			os_printf("%s [%d]0x%llx %d %d %d\r\n",__func__,ret,s_pm_cp1_module_recovery_state,bk_pm_cp1_work_state_get(),bk_pm_cp1_recovery_all_state_get(),s_pm_cp1_ctrl_state);
			if(bk_pm_cp1_recovery_all_state_get())
			{
				bk_pm_module_check_cp1_shutdown();
			}
		}
	}
	os_printf("%s %d %d 0x%x [%d]enter 2\r\n",__func__, module, power_state,s_pm_cp1_ctrl_state,ret);
    if(power_state == PM_POWER_MODULE_STATE_ON)//power on
    {
        GLOBAL_INT_DISABLE();
        s_pm_cp1_ctrl_state |= 0x1 << (module);
        GLOBAL_INT_RESTORE();
        pm_module_bootup_cpu1(PM_POWER_MODULE_NAME_CPU1);
    }
    else //power down
    {
    	if(s_pm_cp1_ctrl_state&(0x1 << (module)))
		{
			GLOBAL_INT_DISABLE();
			s_pm_cp1_ctrl_state &= ~(0x1 << (module));
			GLOBAL_INT_RESTORE();
			if(0x0 == s_pm_cp1_ctrl_state)
			{
				s_pm_cp1_closing = 1;
				os_printf("%s %d %d close 0x%llx  %d\r\n",__func__, module, power_state,s_pm_cp1_module_recovery_state,s_pm_cp1_boot_ready);
				pm_cp0_mailbox_send_data(PM_CP1_RECOVERY_CMD,0,0,0);
				//pm_module_shutdown_cpu1(PM_POWER_MODULE_NAME_CPU1);
			}
    	}
    }

    return BK_OK;
}

/*Get the cp1 heap malloc count*/
uint32_t bk_pm_get_cp1_psram_malloc_count()
{
	uint64_t previous_tick = 0;
	uint64_t current_tick   = 0;
	if(s_pm_cp1_boot_ready)
	{
		s_pm_cp1_psram_malloc_count = 0;
		bk_pm_cp0_psram_malloc_state_set(PM_MAILBOX_COMMUNICATION_INIT);
		pm_cp0_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,0,0,0);

		previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
		current_tick = previous_tick;
		while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*AON_RTC_MS_TICK_CNT))
		{
			if (bk_pm_cp0_psram_malloc_state_get()) // wait the cp1 response
			{
				break;
			}
			current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
		}
		if(!bk_pm_cp0_psram_malloc_state_get())
		{
			os_printf("cp0 get the psram malloc state time out > 100ms\r\n");
		}

	    return s_pm_cp1_psram_malloc_count;
	}
	else
	{
		return 0;
	}
}

/*trigger the cp1 heap malloc dump*/
bk_err_t bk_pm_dump_cp1_psram_malloc_info()
{
	if(s_pm_cp1_boot_ready)
	{
		pm_cp0_mailbox_send_data(PM_CP1_DUMP_PSRAM_MALLOC_INFO_CMD,0,0,0);
	}
    return BK_OK;
}

#endif

#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 2)
static volatile  pm_mailbox_communication_state_e s_pm_cp2_boot_ready        = 0;
static uint32_t s_pm_cp2_ctrl_state           = 0;
extern void start_cpu2_core(void);
extern void stop_cpu2_core(void);
static void pm_module_bootup_cpu2(pm_power_module_name_e module)
{
	if(PM_POWER_MODULE_STATE_OFF == sys_drv_module_power_state_get(module))
	{
		if(module == PM_POWER_MODULE_NAME_CPU2)
		{
            bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_ON);
            start_cpu2_core();
            //while(!s_pm_cp2_boot_ready);
		}
	}
}
static void pm_module_shutdown_cpu2(pm_power_module_name_e module)
{
	GLOBAL_INT_DECLARATION();
	if(PM_POWER_MODULE_STATE_ON == sys_drv_module_power_state_get(module))
	{
		if(module == PM_POWER_MODULE_NAME_CPU2)
		{
            stop_cpu2_core();
		    bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_OFF);
			GLOBAL_INT_DISABLE();
			s_pm_cp2_boot_ready = 0;
			GLOBAL_INT_RESTORE();
		}
	}
}
bk_err_t bk_pm_module_vote_boot_cp2_ctrl(pm_boot_cp2_module_name_e module,pm_power_module_state_e power_state)
{
	GLOBAL_INT_DECLARATION();

    if(power_state == PM_POWER_MODULE_STATE_ON)//power on
    {
        GLOBAL_INT_DISABLE();
        s_pm_cp2_ctrl_state |= 0x1 << (module);
        GLOBAL_INT_RESTORE();
        pm_module_bootup_cpu2(PM_POWER_MODULE_NAME_CPU2);
    }
    else //power down
    {
		GLOBAL_INT_DISABLE();
		s_pm_cp2_ctrl_state &= ~(0x1 << (module));
		GLOBAL_INT_RESTORE();
		if(0x0 == s_pm_cp2_ctrl_state)
		{
			pm_module_shutdown_cpu2(PM_POWER_MODULE_NAME_CPU2);
		}
    }
    return BK_OK;
}
#endif

#if CONFIG_SYS_CPU0
#if CONFIG_PSRAM
static uint32_t s_pm_psram_ctrl_state     = 0;
#endif
static bk_err_t pm_psram_power_ctrl(pm_power_psram_module_name_e module,pm_power_module_state_e power_state)
{
#if CONFIG_PSRAM
	GLOBAL_INT_DECLARATION();
	//os_printf("%s %d %d 0x%x\r\n",__func__, module, power_state,s_pm_psram_ctrl_state);
    if(power_state == PM_POWER_MODULE_STATE_ON)//power on
    {
        GLOBAL_INT_DISABLE();
        s_pm_psram_ctrl_state |= 0x1 << (module);
        GLOBAL_INT_RESTORE();

		bk_psram_init();
    }
    else //power down
    {
		GLOBAL_INT_DISABLE();
		s_pm_psram_ctrl_state &= ~(0x1 << (module));
		GLOBAL_INT_RESTORE();
		if(0x0 == s_pm_psram_ctrl_state)
		{
			bk_psram_deinit();
		}
    }
#endif
 	return BK_OK;
}

bk_err_t pm_debug_pwr_clk_state()
{
#if CONFIG_SYS_CPU0
#if CONFIG_PSRAM
	os_printf("pm_psram_ctrl_state:0x%x 0x%x\r\n",s_pm_psram_ctrl_state,bk_psram_heap_init_flag_get());
#endif
#if (CONFIG_CPU_CNT > 1)
	os_printf("pm_cp1_ctrl_state:0x%x \r\n",s_pm_cp1_ctrl_state);
#endif
	os_printf("pm_cp1_boot_ready:0x%x 0x%x\r\n",s_pm_cp1_boot_ready,s_pm_cp1_module_recovery_state);
#endif
	return BK_OK;
}

#endif
bk_err_t bk_pm_module_vote_psram_ctrl(pm_power_psram_module_name_e module,pm_power_module_state_e power_state)
{
#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;

	bk_err_t ret = BK_OK;
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_psram_power_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_CTRL_PSRAM_POWER_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = power_state;
	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
	os_printf("cpu1 vote psram power ctrl enter %d\r\n",ret);
	while(ret != BK_OK)
	{
		ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
		rtos_delay_milliseconds(4);
	}

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_psram_power_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_psram_power_state_get())
	{
	    os_printf("cp1 get psram state time out\r\n");
	}

	os_printf("cpu1 vote psram power ctrl \r\n");
#endif
	return BK_OK;
#elif CONFIG_SYS_CPU0
	pm_psram_power_ctrl(module,power_state);
	return BK_OK;
#else
	return BK_OK;
#endif
}

bk_err_t bk_pm_module_vote_ctrl_external_ldo(gpio_ctrl_ldo_module_e module,gpio_id_t gpio_id,gpio_output_state_e value)
{
#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;

	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_external_ldo_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_CTRL_EXTERNAL_LDO_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = gpio_id;
	mb_cmd.param3 = value;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_BOOT_CP1_WAITING_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_external_ldo_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_external_ldo_ctrl_state_get())
	{
	    os_printf("cp1 ctrl external ldo time out\r\n");
	}

	os_printf("cpu1 vote ctrl_external_ldo \r\n");
#endif
	return BK_OK;
#elif CONFIG_SYS_CPU0
	bk_gpio_ctrl_external_ldo(module,gpio_id,value);
	return BK_OK;
#else
	return BK_OK;
#endif
}

