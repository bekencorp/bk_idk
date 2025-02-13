#include <os/os.h>
#include <os/mem.h>

#include "usb_driver.h"
#include <driver/media_types.h>

#include "bk_cherry_usb_cdc_acm_api.h"
#include "usbh_cdc_acm.h"
#include "usbh_core.h"
#include <driver/gpio.h>
#include <modules/pm.h>

#include "mb_ipc_cmd.h"

#if (USB_CDC_CP1_IPC)
extern void ipc_cdc_send_cmd(u8 cmd, u8 *cmd_buf, u16 cmd_len, u8 * rsp_buf, u16 rsp_buf_len);
void bk_usb_cdc_upload_data(IPC_CDC_DATA_t *ipc_data)
{
	ipc_cdc_send_cmd(IPC_CPU1_UPLOAD_USB_CDC_DATA, (uint8_t *)ipc_data, ipc_data->cmd_len, NULL, 0);
}

void bk_usb_cdc_update_state_notify(uint8_t state)
{
	ipc_cdc_send_cmd(IPC_CPU1_UPDATE_USB_CDC_STATE, (uint8_t *)&state, sizeof(state), NULL, 0);
}

#else
void bk_usb_cdc_upload_data(IPC_CDC_DATA_t *ipc_data)
{
	ipc_data->bk_cdc_acm_bulkin_cb();
}

extern void (*usb_cdc_state_cb)(uint32_t);
void bk_usb_cdc_update_state_notify(uint8_t state)
{
	if(usb_cdc_state_cb != NULL)
		usb_cdc_state_cb(state);
}

#endif


static uint32_t io_read_flag = 0;

static beken_queue_t acm_msg_queue   = NULL;
static beken_thread_t acm_class_task = NULL;
//static beken_semaphore_t acm_class_task_sem  = NULL;


static uint8_t cdc_tx_state  = 0;
static uint8_t cdc_tx_finish = 0;

#define MAX_BULK_TRS_SIZE (1024)
#define CDC_TX_MAX_SIZE     510

IPC_CDC_DATA_t *g_ipc_cdc_data = NULL;


static uint8_t buffer[512] = {0};

struct usbh_cdc_acm * acm_device = NULL;
usb_osal_sem_t acm_event_wait;

uint8_t g_cdc_acm_buf[6] = {0xEF,0xAA, 0x30, 0x00, 0x00, 0x30};

static uint8_t g_bInterfaceNumber = 0;
static uint8_t g_interface_sub_class = 0;
static struct usbh_hubport * g_u_hport = NULL;

bk_err_t bk_cdc_acm_demo(void);
void bk_cdc_acm_bulkin_callback(void *arg, int nbytes);
void bk_cdc_acm_bulkout_callback(void *arg, int nbytes);

/*********************************************************************************************************/
static bk_err_t acm_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = kNoErr;
	acm_msg_t msg;

	if (acm_msg_queue)
	{
		msg.type = type;
		msg.data = param;

		ret = rtos_push_to_queue(&acm_msg_queue, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			return kNoResourcesErr;
		}
		return ret;
	}
	return kGeneralErr;
}

int32_t bk_usbh_cdc_acm_register_in_transfer_callback(struct usbh_cdc_acm *cdc_acm_class, void *callback, void *arg)
{
//	if (cdc_acm_class)
	{
		struct usbh_urb *urb = &cdc_acm_class->bulkin_urb;
		urb->complete = (usbh_complete_callback_t)callback;
		urb->arg = arg;
	}
	return 0;
}

int32_t bk_usbh_cdc_acm_register_out_transfer_callback(struct usbh_cdc_acm *cdc_acm_class, void *callback, void *arg)
{
//	if (cdc_acm_class)
	{
		struct usbh_urb *urb = &cdc_acm_class->bulkout_urb;
		urb->complete = (usbh_complete_callback_t)callback;
		urb->arg = arg;
	}
	return 0;
}


void bk_usb_update_cdc_interface(void *hport, uint8_t bInterfaceNumber, uint8_t interface_sub_class)
{
	if (interface_sub_class != CDC_SUBCLASS_ACM || !hport)
		return;
	USB_CDC_LOGD("[+]%s\r\n", __func__);
	struct usbh_hubport * u_hport = (struct usbh_hubport *)hport;
	g_u_hport = (struct usbh_hubport *)hport;
	g_bInterfaceNumber = bInterfaceNumber;
	g_interface_sub_class = interface_sub_class;

	bk_usbh_cdc_sw_init(u_hport, bInterfaceNumber, interface_sub_class);

	acm_device = (struct usbh_cdc_acm *)usbh_find_class_instance(u_hport->config.intf[bInterfaceNumber].devname);
	if (acm_device == NULL)
		USB_LOG_ERR("don't find /dev/ttyACM0\r\n");

	USB_CDC_LOGD("[-]%s, %s\r\n", __func__, u_hport->config.intf[bInterfaceNumber].devname);

	bk_cdc_acm_demo();
}



void bk_acm_trigger_rx(void)
{
	acm_device->bulkin_urb.complete = bk_cdc_acm_bulkin_callback;
	acm_device->bulkin_urb.pipe     = acm_device->bulkin;
	acm_device->bulkin_urb.transfer_buffer = &buffer[0];
	acm_device->bulkin_urb.transfer_buffer_length = sizeof(buffer)*sizeof(uint8_t);
	acm_device->bulkin_urb.timeout = 0;
	acm_device->bulkin_urb.actual_length = 0;
}

void bk_acm_trigger_tx(void)
{
	acm_device->bulkout_urb.complete = bk_cdc_acm_bulkout_callback;
	acm_device->bulkout_urb.pipe     = acm_device->bulkout;
	acm_device->bulkout_urb.transfer_buffer = &g_cdc_acm_buf[0];
	acm_device->bulkout_urb.transfer_buffer_length = sizeof(g_cdc_acm_buf)*sizeof(uint8_t);
	acm_device->bulkout_urb.timeout = 100;
	acm_device->bulkout_urb.actual_length = 0;
}


void bk_cdc_acm_bulkin_callback(void *arg, int nbytes)
{
	USB_CDC_LOGI("[+]%s, %d\r\n", __func__, nbytes);

	for (uint32_t i = 0; i < nbytes; i++) {
		USB_CDC_LOGD("%02x ", buffer[i]);
	}
	USB_CDC_LOGD("\n");

	if (nbytes >= 0) {
		*((uint32_t *)g_ipc_cdc_data->rx_len) = nbytes;
		os_memcpy((uint8_t *)(g_ipc_cdc_data->rx_data), &buffer[0], nbytes);
		bk_acm_trigger_rx();
		acm_device->bulkin_urb.actual_length = 0;
		memset(buffer, 0x00, sizeof(buffer));
		acm_send_msg(ACM_UPLOAD_IND, 0);
	}
	else 
		USB_CDC_LOGI("Error bulkin status\n");

}

void bk_cdc_acm_bulkout_callback(void *arg, int nbytes)
{
	USB_CDC_LOGI("[+]%s, nbytes: %d %d\r\n", __func__, nbytes, cdc_tx_finish);

	bk_acm_trigger_tx();
	cdc_tx_state = 1;
	if(cdc_tx_finish)
	{
		acm_send_msg(ACM_BULKIN_IND, 0);
		cdc_tx_finish = 0;
	}
}

int32_t bk_cdc_acm_io_read(void)//(unsigned char *buffer, uint32_t length)
{
	USB_CDC_LOGI("[+]%s\n", __func__);
	int32_t ret = 0;
	uint32_t timeout = 0;

	if (acm_device == NULL)
	{
		USB_CDC_LOGE("acm_device is NULL!\r\n");
		return -1;
	}
	bk_acm_trigger_rx();
	usbh_cdc_acm_bulk_in_transfer(acm_device, &buffer[0], sizeof(buffer)*sizeof(uint8_t), timeout);

	return ret;
}

/*********************************************************************************************************/

int32_t bk_cdc_acm_io_write(IPC_CDC_DATA_t *p_cdc_data)
{
	USB_CDC_LOGD("[+]%s\n", __func__);
	int32_t ret = 0;
	uint32_t timeout = 100;

	if (acm_device == NULL)
	{
		USB_CDC_LOGE("acm_device is NULL!\r\n");
		return -1;
	}
	bk_acm_trigger_tx();

	if (p_cdc_data->tx_len > CDC_TX_MAX_SIZE)
	{
		uint32_t sum_len = p_cdc_data->tx_len;
		uint32_t ops = 0;
		uint32_t one_len = 0;
		uint8_t *buf = (uint8_t *)(p_cdc_data->tx_data);
		while(ops < sum_len)
		{
			one_len = CDC_TX_MAX_SIZE;
			if((sum_len - ops) < CDC_TX_MAX_SIZE)
			{
				one_len = sum_len - ops;
				cdc_tx_finish = 1;
			}
			ret = usbh_cdc_acm_bulk_out_transfer(acm_device, buf+ops, one_len, timeout);

			if(ret == 0)
			{
				ops+= one_len;
			}
			else
			{
				rtos_delay_milliseconds(3);
			}
			USB_CDC_LOGD("out ret:%d\r\n", ret);
		//	rtos_get_semaphore(&acm_tx_msg_sem, 1000);
		//	rtos_delay_milliseconds(1);
			while(!cdc_tx_state)
			{
				rtos_delay_milliseconds(2);
			}
			cdc_tx_state = 0;
		}

	}
	else {
		cdc_tx_finish = 1;
		ret = usbh_cdc_acm_bulk_out_transfer(acm_device, (uint8_t *)p_cdc_data->tx_data, p_cdc_data->tx_len, timeout);
	}
	return ret;
}


void bk_cdc_acm_bulkout(IPC_CDC_DATA_t *p_cdc_data)
{
	acm_send_msg(ACM_BULKOUT_IND, (uint32_t)p_cdc_data);
}

/**************************************************************************************/

void bk_cdc_acm_main(void)
{
	USB_CDC_LOGI("[+]%s\n", __func__);
	int32_t ret = BK_OK;
//	rtos_set_semaphore(&acm_class_task_sem);
	bk_usb_cdc_update_state_notify(CDC_STATUS_INIT);
	rtos_delay_milliseconds(400);
	bk_usb_cdc_update_state_notify(CDC_STATUS_OPEN);
//	bk_usb_device_set_using_status(1, USB_CDC_DEVICE);

	while (1)
	{
		acm_msg_t msg;
		ret = rtos_pop_from_queue(&acm_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case ACM_START_IND:
					break;

				case ACM_BULKIN_IND:
					{
						if (!io_read_flag)
						{
							bk_cdc_acm_io_read();
							io_read_flag = 1;
						}
					}
					break;

				case ACM_BULKOUT_IND:
					bk_cdc_acm_io_write((IPC_CDC_DATA_t *)msg.data);
					break;

				case ACM_UPLOAD_IND:
					bk_usb_cdc_upload_data(g_ipc_cdc_data);
					break;

				case ACM_UPDATE_STATE_IND:
					{
						USB_CDC_LOGD("ACM_UPDATE_STATE_IND\n");
						uint32_t state = (uint32_t) msg.data;
						BK_ASSERT(g_ipc_cdc_data);
						g_ipc_cdc_data->state = state;
						if (!state) {
						//	bk_usb_device_set_using_status(0, USB_CDC_DEVICE);
							usbh_cdc_acm_set_line_state(acm_device, false, true);
							USB_CDC_LOGI("bk_usb_close %d", bk_usb_close());
							goto exit;
						}
					}
					break;

				case ACM_EXIT_IND:
					{
						USB_CDC_LOGI("QQQ %d\n", msg.type);
					//	bk_usb_device_set_using_status(0, USB_CDC_DEVICE);
						goto exit;
					}
					break;

				case ACM_STOP_IND:
					{
					//	bk_usb_device_set_using_status(0, USB_CDC_DEVICE);
						bk_usb_cdc_update_state_notify(CDC_STATUS_CLOSE);
						bk_usb_close();
						goto exit;
					}
					break;
				default:
					break;
			}
		}
	}

exit:
	io_read_flag = 0;
	g_ipc_cdc_data = NULL;

	/* step 1: free que and delete task */
	rtos_deinit_queue(&acm_msg_queue);
	acm_msg_queue = NULL;
	acm_class_task = NULL;
//	rtos_set_semaphore(&acm_class_task_sem);
	rtos_delete_thread(NULL);
}


bk_err_t bk_cdc_acm_demo(void)
{
	int32_t ret = kNoErr;
//	USB_CDC_LOGI("[+]%s, 0x%x\n", __func__, acm_class_task);
	if (acm_class_task)
		return ret;

	bk_usbh_cdc_acm_register_in_transfer_callback(acm_device, bk_cdc_acm_bulkin_callback, NULL);
	bk_usbh_cdc_acm_register_out_transfer_callback(acm_device, bk_cdc_acm_bulkout_callback, NULL);

	// step 1: init acm_task
//	ret = rtos_init_semaphore(&acm_class_task_sem, 1);
//	if (ret != kNoErr)
//	{
//		goto error;
//	}

	if (acm_msg_queue == NULL) {
		ret = rtos_init_queue(&acm_msg_queue, "acm_class_queue", sizeof(acm_msg_t), 20);
		if (ret != kNoErr)
		{
			goto error;
		}
	}

	if (acm_class_task == NULL) {
		ret = rtos_create_thread(&acm_class_task,
								4,
								"acm_class_task",
								(beken_thread_function_t)bk_cdc_acm_main,
								1024 * 2,
								acm_msg_queue);

		if (ret == kNoErr)
		{
		//	rtos_get_semaphore(&acm_class_task_sem, BEKEN_NEVER_TIMEOUT);
			bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_480M);
			return ret;
		}
	}
error:
//	if (acm_class_task_sem)
//	{
//		rtos_deinit_semaphore(&acm_class_task_sem);
//		acm_class_task_sem = NULL;
//	}

	if (acm_msg_queue)
	{
		rtos_deinit_queue(&acm_msg_queue);
		acm_msg_queue = NULL;
	}

	if (acm_class_task)
	{
		rtos_delete_thread(&acm_class_task);
		acm_class_task = NULL;
	}

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);

	return ret;
}

void bk_usb_cdc_free_enumerate_resources(void)
{
	if(acm_device) {
		bk_usbh_cdc_sw_deinit(acm_device->hport, acm_device->ctrl_intf, CDC_SUBCLASS_ACM);
		acm_device = NULL;
	}
	acm_send_msg(ACM_EXIT_IND, 0);
}


void bk_usb_cdc_open(IPC_CDC_DATA_t *ipc_cdc)
{
	if (!ipc_cdc)
		USB_CDC_LOGE("Invalid Parameter!\n");

	g_ipc_cdc_data = ipc_cdc;

	bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
	bk_usb_open(USB_HOST_MODE);
//	bk_usb_device_set_using_status(1, USB_CDC_DEVICE);

	acm_send_msg(ACM_BULKIN_IND, 0);
}

void bk_usb_cdc_close(void)
{
	acm_send_msg(ACM_STOP_IND, 0);
}

void bk_usb_cdc_exit(void)
{
	acm_send_msg(ACM_EXIT_IND, 0);
}

void bk_usb_cdc_param_init(IPC_CDC_DATA_t *p_cdc_data)
{
	if (!p_cdc_data)
		USB_CDC_LOGE("Invalid Parameter!\n");

	g_ipc_cdc_data = p_cdc_data;
}


