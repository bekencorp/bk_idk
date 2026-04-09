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

#include <os/os.h>
#include "cli.h"
#include <driver/wdt.h>
#include "wdt_driver.h"
#include <bk_wdt.h>
#include "sys_hal.h"
#if CONFIG_SYSTEM_CTRL
#include "aon_pmu_driver.h"
#include "sys_types.h"
#endif

static void cli_wdt_help(void)
{
	CLI_LOGI("wdt_driver init\n");
	CLI_LOGI("wdt_driver deinit\n");
	CLI_LOGI("wdt start [timeout]\n");
	CLI_LOGI("wdt stop\n");
	CLI_LOGI("wdt feed\n");
	CLI_LOGI("wdt clkdiv [get|<0-3>]  (sys reg0xa ckdiv_wdt: 0:/2 1:/4 2:/8 3:/16, src LPO_16K)\n");
	CLI_LOGI("wdt feed_stop / feed_resume  (stop TIMER_ID2 auto-feed; need wdt started first)\n");
#if CONFIG_SYSTEM_CTRL
	CLI_LOGI("wdt pmu_r2 get | set <0-7> | default  (AON PMU R2[2:0]: ana,top,aon)\n");
#endif
}

static void cli_wdt_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_wdt_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_wdt_driver_init());
		CLI_LOGI("wdt driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_wdt_driver_deinit());
		CLI_LOGI("wdt driver deinit\n");
	} else {
		cli_wdt_help();
		return;
	}
}

static void cli_wdt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_wdt_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		uint32_t timeout = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_wdt_start(timeout));
		CLI_LOGI("wdt start, timeout=%d\n", timeout);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		BK_LOG_ON_ERR(bk_wdt_stop());
#if (CONFIG_TASK_WDT)
		bk_task_wdt_stop();
#endif
		CLI_LOGI("wdt stop\n");
	}else if (os_strcmp(argv[1], "feed") == 0) {
		BK_LOG_ON_ERR(bk_wdt_feed());
		CLI_LOGI("wdt feed\n");
	}else if (os_strcmp(argv[1], "disable") == 0) {
		bk_wdt_stop();
#if (CONFIG_TASK_WDT)
		bk_task_wdt_stop();
#endif
		CLI_LOGI("wdt debug disabled\n");
	}else if (os_strcmp(argv[1], "enable") == 0) {
		extern void wdt_init(void);
		wdt_init();
		CLI_LOGI("wdt debug enabled\n");
	} else if (os_strcmp(argv[1], "clkdiv") == 0) {
		if (argc < 3 || os_strcmp(argv[2], "get") == 0) {
			uint32_t v = sys_hal_nmi_wdt_get_clk_div();
			uint32_t div = 2u << v;

			CLI_LOGI("nmi_wdt ckdiv_wdt=%u -> wdt_clk = LPO_16K / %u\n", v, div);
		} else {
			uint32_t v = (uint32_t)os_strtoul(argv[2], NULL, 0);

			if (v > 3) {
				CLI_LOGE("clkdiv must be 0..3 (0:/2 1:/4 2:/8 3:/16)\n");
				return;
			}
			sys_hal_nmi_wdt_set_clk_div(v);
			CLI_LOGI("nmi_wdt ckdiv_wdt=%u -> wdt_clk = LPO_16K / %u\n", v, 2u << v);
		}
	} else if (os_strcmp(argv[1], "feed_stop") == 0) {
		BK_LOG_ON_ERR(bk_wdt_timer_feed_stop());
		CLI_LOGI("WDT auto-feed timer stopped (will reset if wdt running and not fed)\n");
	} else if (os_strcmp(argv[1], "feed_resume") == 0) {
		BK_LOG_ON_ERR(bk_wdt_timer_feed_resume());
		CLI_LOGI("WDT auto-feed timer resumed\n");
#if CONFIG_SYSTEM_CTRL
	} else if (os_strcmp(argv[1], "pmu_r2") == 0) {
		if (argc < 3 || os_strcmp(argv[2], "get") == 0) {
			uint32_t v = aon_pmu_drv_reg_get(PMU_REG2);

			CLI_LOGI("AON PMU R2=0x%08x  wdt_rst_ana=%u top=%u aon=%u\n",
				 v, (unsigned)((v >> 0) & 1u), (unsigned)((v >> 1) & 1u),
				 (unsigned)((v >> 2) & 1u));
		} else if (os_strcmp(argv[2], "default") == 0) {
			aon_pmu_drv_wdt_rst_dev_enable();
			CLI_LOGI("restored driver default (top+aon like wdt_rst_dev_enable)\n");
		} else if (os_strcmp(argv[2], "set") == 0) {
			uint32_t m;

			if (argc < 4) {
				CLI_LOGE("usage: wdt pmu_r2 set <0-7>  (bit0=ana bit1=top bit2=aon)\n");
				return;
			}
			m = (uint32_t)os_strtoul(argv[3], NULL, 0);
			if (m > 7u) {
				CLI_LOGE("mask must be 0..7\n");
				return;
			}
			{
				uint32_t v = aon_pmu_drv_reg_get(PMU_REG2);

				v = (v & ~7u) | (m & 7u);
				aon_pmu_drv_reg_set(PMU_REG2, v);
				v = aon_pmu_drv_reg_get(PMU_REG2);
				CLI_LOGI("R2 low3=0x%x full=0x%08x  ana=%u top=%u aon=%u\n",
					 (unsigned)(m & 7u), v,
					 (unsigned)((v >> 0) & 1u), (unsigned)((v >> 1) & 1u),
					 (unsigned)((v >> 2) & 1u));
			}
		} else {
			CLI_LOGE("usage: wdt pmu_r2 get | set <0-7> | default\n");
		}
#endif
	}else if (os_strcmp(argv[1], "while") == 0) {
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		CLI_LOGI("wdt enter while1\n");
		while(1);
		GLOBAL_INT_RESTORE();
	} else {
		cli_wdt_help();
		return;
	}
}

#define WDT_CMD_CNT (sizeof(s_wdt_commands) / sizeof(struct cli_command))
static const struct cli_command s_wdt_commands[] = {
	{"wdt_driver", "{init|deinit}", cli_wdt_driver_cmd},
#if CONFIG_SYSTEM_CTRL
	{"wdt", "wdt {start|stop|feed|clkdiv|feed_stop|feed_resume|pmu_r2} [...]", cli_wdt_cmd}
#else
	{"wdt", "wdt {start|stop|feed|clkdiv|feed_stop|feed_resume} [...]", cli_wdt_cmd}
#endif
};

int cli_wdt_init(void)
{
	BK_LOG_ON_ERR(bk_wdt_driver_init());
	return cli_register_commands(s_wdt_commands, WDT_CMD_CNT);
}

