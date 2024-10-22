#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <components/bluetooth/bk_dm_ble.h>

#include "cli.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#ifdef CONFIG_CACHE_CUSTOM_SRAM_MAPPING
const unsigned int g_sram_addr_map[4] =
{
    0x38000000,
    0x30020000,
    0x38020000,
    0x30000000
};
#endif


void user_app_main(void)
{
}

int main(void)
{
	//#if (!CONFIG_SOC_BK7256XX)
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    // bk_set_printf_sync(true);
    // shell_set_log_level(BK_LOG_WARN);
#endif

    bk_init();

#if CONFIG_A2DP_SINK_DEMO && !CONFIG_A2DP_SOURCE_DEMO
    //extern int a2dp_sink_demo_init(uint8_t aac_supported);
    //a2dp_sink_demo_init(0);
#elif !CONFIG_A2DP_SINK_DEMO && CONFIG_A2DP_SOURCE_DEMO

#else
    os_printf("%s CONFIG_A2DP_SINK_DEMO CONFIG_A2DP_SOURCE_DEMO can't both init now !\n", __func__);
#endif

    return 0;
//#endif
}
