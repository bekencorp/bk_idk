#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include <driver/pwr_clk.h>
#include <components/log.h>
#include <driver/mailbox_channel.h>


#define TAG  "APP_MBOX"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);

#if (CONFIG_SYS_CPU0 || CONFIG_SYS_CPU1)
/* select the logical channel */
#define  APP_MAILBOX_TEST_CHANNEL  MB_CHNL_MIPC_SYNC
/* customised transmission commands */
#define APP_MBOX_SEND_CMD          			 (0x1)
#define APP_MBOX_RSP_CMD            		 (0x2)

/* application data structure */
typedef struct
{
    beken_semaphore_t	send_sema;
    beken_semaphore_t	rsp_sema;
    void *data;
    uint32_t seq;
    uint32_t count;
} app_mailbox_data_t;

#if CONFIG_SYS_CPU0
static app_mailbox_data_t g_cpu0_mailbox_data;  
static const char *cpu0_send_msg= {">CPU0 SEND MSG TO CPU1<"};
static const char *cpu0_rsp_msg={"|CPU0 ACK CPU1 MSG|"};  
#elif CONFIG_SYS_CPU1
static app_mailbox_data_t g_cpu1_mailbox_data;   
// static const char *cpu1_send_msg= {">CPU1 SEND MSG TO CPU0<"};
static const char *cpu1_rsp_msg={"CPU1 APP MAILBOX RESPONSE MSG"};  
#endif

static void app_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
#if CONFIG_SYS_CPU0
    switch(cmd_buf->hdr.cmd) {
        case APP_MBOX_SEND_CMD:

            break;

        case APP_MBOX_RSP_CMD:
            g_cpu0_mailbox_data.data = (void *)cmd_buf->param1;
            g_cpu0_mailbox_data.seq = cmd_buf->param2;
            rtos_set_semaphore(g_cpu0_mailbox_data.rsp_sema);
            break;
        
        default:
            break;

    }
    
#elif CONFIG_SYS_CPU1
    switch(cmd_buf->hdr.cmd) {
        case APP_MBOX_SEND_CMD:
            g_cpu1_mailbox_data.data = (void *)cmd_buf->param1;
            g_cpu1_mailbox_data.seq = cmd_buf->param2;
            rtos_set_semaphore(g_cpu1_mailbox_data.rsp_sema);
            break;

        case APP_MBOX_RSP_CMD:
            g_cpu1_mailbox_data.data = (void *)cmd_buf->param1;
            g_cpu1_mailbox_data.seq = cmd_buf->param2;
            rtos_set_semaphore(g_cpu1_mailbox_data.rsp_sema);
            break;
        
        default:
            break;

    }

#endif
    
}

static void app_mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
#if CONFIG_SYS_CPU0
    rtos_set_semaphore(&g_cpu0_mailbox_data.send_sema);
#elif CONFIG_SYS_CPU1
    rtos_set_semaphore(&g_cpu1_mailbox_data.send_sema);
#endif
}

void app_mailbox_send_msg(uint32_t cmd, uint8_t * app_data, uint32_t count)
{
    mb_chnl_cmd_t mb_cmd;
    int ret = BK_OK;
    uint8_t channel = 0;

    channel = APP_MAILBOX_TEST_CHANNEL;

    mb_cmd.hdr.cmd = cmd;
    mb_cmd.param1 = (uint32_t)app_data;
    mb_cmd.param2 = count;
    mb_cmd.param3 = 0;

    ret = mb_chnl_write(channel, &mb_cmd);

    if (ret != BK_OK)
    {
        LOGE("mb_chnl_write failed\n");
    }
}

void app_mailbox_config(uint8_t channel)
{
    bk_err_t	ret;

    LOGI("open channel: %d on CPU\n", channel);
    /* reigster a mailbox logical channel */
    ret = mb_chnl_open(channel, NULL);
    if (ret != BK_OK) {
        LOGE(" mb_chnl_open open fail \r\n");
        return;
    }  
    /* register mailbox logical channel rx callbcak */
    mb_chnl_ctrl(channel, MB_CHNL_SET_RX_ISR, app_mailbox_rx_isr);
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_ISR, NULL);
    /* register mailbox logical channel rx complete callback */
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_CMPL_ISR, app_mailbox_tx_cmpl_isr);
}

#if CONFIG_SYS_CPU0
static void _cpu0_app_thread_handle(beken_thread_arg_t arg)
{
    (void) arg;

    for(;;) {
        /* send mailbox msg */
        if ((g_cpu0_mailbox_data.count % 2) == 0) {
            app_mailbox_send_msg(APP_MBOX_SEND_CMD, (uint8_t *)cpu0_send_msg, g_cpu0_mailbox_data.count);
        } else {
            app_mailbox_send_msg(APP_MBOX_RSP_CMD, (uint8_t *)cpu0_rsp_msg, g_cpu0_mailbox_data.count);
        }
        
        /* wait send semaphore */
        rtos_get_semaphore(&g_cpu0_mailbox_data.send_sema, BEKEN_WAIT_FOREVER);

        /* wait in a rotating way */
        rtos_delay_milliseconds(500);  
        LOGI("cpu0 recv cpu1 msg seq:[%d]- info:[%s]\r\n",g_cpu0_mailbox_data.seq, g_cpu0_mailbox_data.data);

        g_cpu0_mailbox_data.count++;
        if (g_cpu0_mailbox_data.count >= 1000) {
            g_cpu0_mailbox_data.count = 0;
        }

        
    }
}

#elif CONFIG_SYS_CPU1
static void _cpu1_app_thread_handle(beken_thread_arg_t arg)
{
    (void) arg;

    for(;;) {

        /* wait recv semaphore and output recviced message */
        rtos_get_semaphore(&g_cpu1_mailbox_data.rsp_sema, BEKEN_WAIT_FOREVER);
        LOGI("cpu1 recv cpu0 msg seq:[%d]- info:[%s]\r\n",g_cpu1_mailbox_data.seq, g_cpu1_mailbox_data.data);

        /* send recv msg back */
        app_mailbox_send_msg(APP_MBOX_RSP_CMD, (uint8_t *)cpu1_rsp_msg, g_cpu1_mailbox_data.count);

        /* wait send semaphore */
        rtos_get_semaphore(&g_cpu1_mailbox_data.send_sema, BEKEN_WAIT_FOREVER);

        g_cpu1_mailbox_data.count++;
        if (g_cpu1_mailbox_data.count >= 1000) {
            g_cpu1_mailbox_data.count = 0;
        }
        rtos_delay_milliseconds(500);
    }
}

#endif
#endif

void app_mailbox_v1_test(void)
{
#if CONFIG_SYS_CPU0
    /* create a task */
    beken_thread_t cpu0_thread_handle;
    bk_err_t ret;

    ret = rtos_create_thread(&cpu0_thread_handle,
        BEKEN_DEFAULT_WORKER_PRIORITY,
         "cpu0_mailbox_thread",
        (beken_thread_function_t)_cpu0_app_thread_handle,
        1024*2,
        0);
    
    if (ret != kNoErr) {
        LOGE("cpu0 create thread fail!\r\n");
    }
    
    /* register a log channel */
    app_mailbox_config(APP_MAILBOX_TEST_CHANNEL);

    /* init semaphore */
    ret = rtos_init_semaphore(&g_cpu0_mailbox_data.send_sema, 1);
    if (ret != BK_OK) {
        LOGE("cpu0 init send_sema fail!\r\n");
    }

    ret = rtos_init_semaphore(&g_cpu0_mailbox_data.rsp_sema, 1);
    if (ret != BK_OK) {
        LOGE("cpu0 init rsp_sema fail!\r\n");
    }

    /* power on cpu1 */
    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP, PM_POWER_MODULE_STATE_ON);

#elif CONFIG_SYS_CPU1
    /* create a task */
    beken_thread_t cpu1_thread_handle;
    bk_err_t ret;

    ret = rtos_create_thread(&cpu1_thread_handle,
            BEKEN_DEFAULT_WORKER_PRIORITY,
            "cpu1_mailbox_thread",
            (beken_thread_function_t)_cpu1_app_thread_handle,
            1024*2,
            0);
    if (ret != kNoErr) {
        LOGE("cpu0 create thread fail!\r\n");
    }

    /* register a log channel */
    app_mailbox_config(APP_MAILBOX_TEST_CHANNEL);

    /* init semaphore */
    ret = rtos_init_semaphore(&g_cpu1_mailbox_data.send_sema, 1);
    if (ret != BK_OK) {
        LOGE("cpu1 init send_sema fail!\r\n");
    }

    ret = rtos_init_semaphore(&g_cpu1_mailbox_data.rsp_sema, 1);
    if (ret != BK_OK) {
        LOGE("cpu1 init rsp_sema fail!\r\n");
    }

#endif
}

void user_app_main(void){

}

int main(void)
{
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7236)
	// STARTUP_PERF(14);
#endif
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();

    app_mailbox_v1_test();

	return 0;
}