
#include "cli.h"
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include <common/bk_include.h>

#include "mb_ipc_cmd.h"
#include "bk_usb_cdc_demo.h"

#define TAG "cdc_demo"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

uint32_t ipc_cdc_rx_len = 0;
uint8_t ipc_rxdata_dbg[128] = {0};
IPC_CDC_DATA_t dbg_cdc_ipc;

static beken_queue_t cdc_msg_queue = NULL;
static beken_thread_t cdc_demo_task = NULL;

static uint8_t tx_buf[6] = {0xEF,0xAA,0x30,0x00,0x00,0x30};

void (*usb_cdc_state_cb)(uint32_t);
void bk_usb_cdc_connect_init_cb(void (*cb)(uint32_t))
{
	usb_cdc_state_cb = cb;
}

#if (USB_CDC_CP0_IPC)
extern void ipc_cdc_send_cmd(u8 cmd, u8 *cmd_buf, u16 cmd_len, u8 * rsp_buf, u16 rsp_buf_len);

void bk_usb_cdc_open(IPC_CDC_DATA_t *ipc_cdc)
{
	ipc_cdc_send_cmd(IPC_CPU0_OPEN_USB_CDC, (uint8_t *)ipc_cdc, ipc_cdc->cmd_len, NULL, 0);
}

void bk_usb_cdc_close(void)
{
	ipc_cdc_send_cmd(IPC_CPU0_CLOSE_USB_CDC, NULL, 0, NULL, 0);
}

void bk_cdc_acm_bulkout(IPC_CDC_DATA_t *ipc_cdc)
{
	ipc_cdc_send_cmd(IPC_CPU0_SET_USB_CDC_CMD, (uint8_t *)ipc_cdc, ipc_cdc->cmd_len, NULL, 0);
}

void bk_usb_cdc_param_init(IPC_CDC_DATA_t *ipc_cdc)
{
	ipc_cdc_send_cmd(IPC_CPU0_INIT_USB_CDC_PARAM, (uint8_t *)ipc_cdc, ipc_cdc->cmd_len, NULL, 0);
}

#else
extern void bk_cdc_acm_bulkout(IPC_CDC_DATA_t *ipc_cdc);
extern void bk_usb_cdc_param_init(IPC_CDC_DATA_t *p_cdc_data);

#endif

static bk_err_t cdc_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = kNoErr;
	cdc_msg_t msg;

	if (cdc_msg_queue)
	{
		msg.type = type;
		msg.data = param;

		ret = rtos_push_to_queue(&cdc_msg_queue, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			return kNoResourcesErr;
		}
		return ret;
	}
	return kGeneralErr;
}

static void bk_cdc_acm_bulkin_cb(void)
{
	LOGI("[+]%s\r\n", __func__);
	LOGI("getdata 0x%x 0x%x %d\n", dbg_cdc_ipc.rx_data, dbg_cdc_ipc.rx_len, ipc_cdc_rx_len);
	uint8_t *temp = (uint8_t *)dbg_cdc_ipc.rx_data;
	for (uint32_t i = 0; i < ipc_cdc_rx_len; i++)
		LOGD("%x\n", temp[i]);
	os_memset(temp, 0x00, ipc_cdc_rx_len);
	ipc_cdc_rx_len = 0;

	cdc_send_msg(CDC_STATUS_OPEN, 0);
}

static void bk_cdc_acm_bulkout_cb(void *p)
{
	LOGI("[+]%s\r\n", __func__);
	IPC_CDC_DATA_t * p_cdc = (IPC_CDC_DATA_t *)p;
	bk_cdc_acm_bulkout(p_cdc);
}


static void bk_cdc_acm_init(void)
{
	LOGI("[+]%s", __func__);
	memset(&dbg_cdc_ipc,0,sizeof(dbg_cdc_ipc));
	dbg_cdc_ipc.rx_data = (uint32_t)(&ipc_rxdata_dbg[0]);
	dbg_cdc_ipc.rx_len  = (uint32_t)(&ipc_cdc_rx_len);
	dbg_cdc_ipc.cmd_len = sizeof(IPC_CDC_DATA_t);
	dbg_cdc_ipc.bk_cdc_acm_bulkin_cb  = bk_cdc_acm_bulkin_cb;
	dbg_cdc_ipc.bk_cdc_acm_bulkout_cb = bk_cdc_acm_bulkout_cb;
	bk_usb_cdc_param_init(&dbg_cdc_ipc);
}


static void bk_cdc_acm_state_cb(uint32_t state)
{
	LOGI("[+]%s %d\r\n", __func__, state);
	cdc_send_msg(state, 0);
}


static void bk_cdc_demo_task(beken_thread_arg_t arg)
{
	int ret = BK_OK;
	cdc_msg_t msg;
	while (1)
	{
		ret = rtos_pop_from_queue(&cdc_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		LOGD("%s, type %d\n", __func__, msg.type);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case CDC_STATUS_INIT:
					bk_cdc_acm_init();
					break;
				case CDC_STATUS_OPEN:
					{
						rtos_delay_milliseconds(2000);
						dbg_cdc_ipc.tx_data = (uint32_t)tx_buf;
						dbg_cdc_ipc.tx_len = 6;
						dbg_cdc_ipc.cmd_len = sizeof(IPC_CDC_DATA_t);

						bk_cdc_acm_bulkout(&dbg_cdc_ipc);
					}
					break;
				default:
					break;
			}

		}
	}
}

void bk_usb_cdc_demo(void)
{
	int ret = kNoErr; __maybe_unused_var(ret);
	bk_usb_cdc_connect_init_cb(bk_cdc_acm_state_cb);

	if (cdc_msg_queue == NULL)
	{
		ret = rtos_init_queue(&cdc_msg_queue, "cdc_msg_queue", sizeof(cdc_msg_t), 10);
		if (ret != kNoErr)
		{
			LOGE("init cdc_msg_queue failed\r\n");
			goto error;
		}
	}


	if (cdc_demo_task == NULL)
	{
		ret = rtos_create_thread(&cdc_demo_task,
							BEKEN_APPLICATION_PRIORITY,
							"cdc_demo_task",
							(beken_thread_function_t)bk_cdc_demo_task,
							2*1024,
							NULL);

		if (ret != kNoErr)
		{
			goto error;
		}
	}
	return;
error:
	if (cdc_msg_queue)
	{
		rtos_deinit_queue(&cdc_msg_queue);
		cdc_msg_queue = NULL;
	}

	if (cdc_demo_task)
	{
		cdc_demo_task = NULL;
		rtos_delete_thread(NULL);
	}
}


