/**
 ****************************************************************************************
 *
 * @file bk_csi_ble_provisioning.c
 *
 * @brief BK WIFI csi module
 *
 *
 * Copyright (C) BEKEN corperation 2021-2024
 *
 ****************************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "ble_boarding.h"
#include "bk_csi_ble_provision.h"
#include "bk_ble.h"
#include "bk_dm_ble.h"
#include "bk_dm_bluetooth.h"
#include "mem.h"
#include "bk_csi_common.h"


#define ADV_MAX_SIZE (251)
#define ADV_NAME_HEAD "bk_csi_demo"

#define ADV_TYPE_FLAGS                      (0x01)
#define ADV_TYPE_LOCAL_NAME                 (0x09)
#define ADV_TYPE_SERVICE_UUIDS_16BIT        (0x14)
#define ADV_TYPE_SERVICE_DATA               (0x16)
#define ADV_TYPE_MANUFACTURER_SPECIFIC      (0xFF)
#define NOTIFY_PAYLOAD_LENGTH 247 /// 255 -3 -5

#define BEKEN_COMPANY_ID                    (0x05F0)

#define BOARDING_UUID                       (0xFE01)

typedef struct bus_message {
	uint32_t type;
	uint32_t arg;
	uint32_t len;
	void *sema;
	void *cb;
	void *param;
} BUS_MSG_T;


#define SEG_BUF_NUM 16
static uint32_t *seg_buf[SEG_BUF_NUM] = {NULL};
static uint8_t seg_len[SEG_BUF_NUM] = {0};


ble_boarding_info_t *bk_wifi_csi_ble_boarding_info = NULL;
beken_thread_t bk_csi_ble_provisioning_thread = NULL;
beken_queue_t bk_csi_ble_provisioning_queue = NULL;
beken_semaphore_t bk_csi_ble_provisioning_sema;
uint8_t bk_csi_ble_send_flag = false;
uint8_t adv_data[ADV_MAX_SIZE] = {0};
uint8_t adv_index = 0;

static int bk_csi_ble_provisioning_send_msg(int type, uint32_t arg, uint32_t len, void *param)
{
	BUS_MSG_T msg;
	int ret;

	msg.type = type;
	msg.arg = (uint32_t)arg;
	msg.len = len;
	msg.sema = NULL;

	if (len)
	{
		msg.param = os_malloc(len);

		if (msg.param)
		{
			os_memset((uint8_t *)msg.param, 0, len);
			os_memcpy((uint8_t *)msg.param, (uint8_t *)param, len);
		}
		else
		{
			CSI_LOGI("%s: msg %d alloc fail \n", __func__, type);
			return BK_FAIL;
		}
	}
	else
		msg.param = (void *)param;
	
	ret = rtos_push_to_queue(&bk_csi_ble_provisioning_queue, &msg, BEKEN_NO_WAIT);
	if (ret)
	{
		CSI_LOGI("%s: msg %d push queue fail \n", __func__, type);

		if (len)
		os_free(msg.param);
	}

	return ret;
}

void bk_wifi_csi_ble_disconnect_ind(void)
{
	bk_csi_ble_provisioning_send_msg(BK_CSI_BLE_DISCONNECT_IND, 0, 0, NULL);
}

static int bk_wifi_csi_disconnect_ind_handler(void)
{
	CSI_LOGI("bk_wifi_csi_disconnect_ind_handler\r\n");
	extern int ble_boarding_adv_only_start_csi(void);
	ble_boarding_adv_only_start_csi();
	return BK_OK;
}

void bk_wifi_csi_ble_data_send(uint8_t *payload, uint16_t data_len)
{
	CSI_LOGI("wifi csi provisioning send data, len %d\r\n", data_len);
	bk_csi_ble_provisioning_send_msg(BK_CSI_BLE_SEND_PKT, 0, data_len, payload);
}

static int bk_wifi_csi_data_send_to_ble(uint16_t seg_info, uint8_t *payload, uint16_t data_len)
{
	uint16_t opcode = (0x5a | ((seg_info << 8) & 0xff00));
	uint8_t status = 0;
	int ret = 0;
	/// 252 is limited by ble phy
	uint8_t data[NOTIFY_PAYLOAD_LENGTH+5] =
	{
		opcode & 0xFF, opcode >> 8,     /* opcode           */
		status & 0xFF,                  /* status           */
		data_len & 0xFF, data_len >> 8,     /* payload length   */
	};

	if (data_len > NOTIFY_PAYLOAD_LENGTH)
	{
		CSI_LOGI("size %d over flow\n", data_len);
		return BK_FAIL;
	}

	os_memcpy(&data[5], payload, data_len);

	ret = ble_boarding_notify(data, data_len + 5);
	return ret;
}

static void bk_wifi_csi_ble_handle_data_send(BUS_MSG_T *msg)
{
	uint8_t *payload = (uint8 *)(msg->param);
	uint32_t data_len = msg->len;
	uint8_t data_seg = (data_len+(NOTIFY_PAYLOAD_LENGTH-1))/NOTIFY_PAYLOAD_LENGTH;
	uint8_t i;
	int32_t ret;
	uint16_t seg_info;
	CSI_LOGI("wifi csi provisioning send data to ble, len %d, seg %d\r\n",data_len, data_seg);
	for(i=0; i<data_seg; i++)
	{
		seg_info = 0;
		if (i < (data_seg - 1))
		{
			seg_info = (((data_seg - 1)<<4) | i);
			data_len -= NOTIFY_PAYLOAD_LENGTH;
			CSI_LOGI("wifi csi provisioning send data to ble, len %d, seg %d\r\n",NOTIFY_PAYLOAD_LENGTH, i);
			bk_csi_ble_send_flag = true;
			ret = bk_wifi_csi_data_send_to_ble(seg_info, payload+(i*NOTIFY_PAYLOAD_LENGTH), NOTIFY_PAYLOAD_LENGTH);
			if(ret)
			{
				bk_csi_ble_send_flag = false;
				CSI_LOGI("csi send data to ble error1\n");
				break;
			}
		}
		else
		{
			data_len %= NOTIFY_PAYLOAD_LENGTH; 
			data_len = (data_len == 0) ? NOTIFY_PAYLOAD_LENGTH:data_len;
			CSI_LOGI("wifi csi provisioning send data to ble, len %d, seg %d\r\n",data_len, i);
			seg_info = (((data_seg - 1)<<4) | i);
			bk_csi_ble_send_flag = true;
			ret = bk_wifi_csi_data_send_to_ble(seg_info, payload+(data_seg - 1)*NOTIFY_PAYLOAD_LENGTH, data_len);
			if (ret)
			{
				bk_csi_ble_send_flag = false;
				CSI_LOGI("csi send data to ble error2\n");
				break;
			}
		}
		//BK_csi_LOGI("wifi csi provisioning send data to ble, len %d, seg %d\r\n",data_len, i);

		if (rtos_get_semaphore(&bk_csi_ble_provisioning_sema, 6000))
		{
			CSI_LOGI("wifi csi get ble prov sema timeout\r\n");
			if(i < (data_seg - 1))
			{
				CSI_LOGI("csi send data to ble error3\n");
				break;
			}
		}
			CSI_LOGI("wifi csi provisioning send to ble seg %d, len %d\r\n", i, data_len);
			bk_csi_ble_send_flag = false;
	}
}

static void bk_csi_seg_buf_clear(uint8_t seg_num)
{
	uint8_t i;
	for(i=0; i<seg_num; i++)
	{
		if (seg_buf[i] != NULL)
		{
			os_free(seg_buf[i]);
			seg_buf[i] = NULL;
		}
		seg_len[i] = 0;
	}
}

void bk_wifi_csi_ble_handle_data_rcv(BUS_MSG_T *msg)
{
	uint8_t *data = (uint8 *)(msg->param);
	uint16_t data_len = (uint16)msg->len;
	uint16_t opcode = (uint16)(msg->arg & 0xffff);
	static uint8_t seg_count = 0;
	static uint8_t seg_num = 0;    
	static uint16_t seg_total_len = 0;

	if ((opcode & 0xff00) != 0)
	{
		uint8_t cur_seg_num = (uint8_t) ((opcode >> 12) & 0xf);
		uint8_t cur_seg_count = (uint8_t) ((opcode >> 8) & 0xf);
		BK_ASSERT(cur_seg_num >= cur_seg_count);
		if (cur_seg_count == 0)
		{
			bk_csi_seg_buf_clear(seg_count);
			seg_num = cur_seg_num;
			seg_count = 0;
			seg_total_len = 0;
		}

		BK_ASSERT(seg_num == cur_seg_num);
		BK_ASSERT(seg_count == cur_seg_count);
		seg_buf[seg_count] = os_malloc(data_len);
		BK_ASSERT(seg_buf[seg_count]);
		seg_len[seg_count] = data_len;
		seg_total_len += data_len;    
		os_memcpy(seg_buf[seg_count], data, data_len);
		seg_count++;
		
		if (cur_seg_num == cur_seg_count)
		{
			uint8_t i = 0;
			uint16_t offset = 0;
			uint8_t *send_data;
			send_data = os_malloc(seg_total_len);
			for (i=0; i<seg_count; i++)
			{
				os_memcpy(send_data+offset, seg_buf[i], seg_len[i]);
				offset +=  seg_len[i];
			}
			BK_ASSERT(seg_total_len == offset);
			data_len = seg_total_len;

			csi_data_rcv_data_handle((char*)send_data);
			os_free(send_data);
			bk_csi_seg_buf_clear(seg_count);
			seg_count = 0;
		}
		return;
	}
	else if (seg_count != 0)
	{
		bk_csi_seg_buf_clear(seg_count);
		seg_count = 0;
	}

	csi_data_rcv_data_handle((char*)data);

	return;
}

/// receive data from ble
static void bk_wifi_csi_ble_recv_data_handle(uint16_t opcode, uint16_t length, uint8_t *data)
{
	if(((opcode & 0xFF) != 0xa5)||(length > 251))
	{
		CSI_LOGI("receive data from ble,opcode %d, len %d\r\n", opcode, length);
		return;
	}
	CSI_LOGI("wifi csi provisioning receive data from ble,opcode %d, len %d\r\n", opcode, length);

	bk_csi_ble_provisioning_send_msg(BK_CSI_BLE_RCV_PKT, opcode, length, data);
}

static int bk_wifi_csi_ble_boarding_init(void)
{
	uint8_t len_index = 0;
	uint8_t mac[6];
	int ret;


	CSI_LOGI("%s\n", __func__);

	/* flags */
	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_FLAGS;
	adv_data[adv_index++] = 0x06;
	adv_data[len_index] = 2;

	/* local name */
	extern int bk_bluetooth_get_address(uint8_t *addr);
	bk_bluetooth_get_address(mac);

	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_LOCAL_NAME;

	ret = sprintf((char *)&adv_data[adv_index], "%s_%02X%02X%02X",
					ADV_NAME_HEAD, mac[2], mac[1], mac[0]);

	adv_index += ret;
	adv_data[len_index] = ret + 1;

	/* 16bit uuid */
	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_SERVICE_DATA;
	adv_data[adv_index++] = BOARDING_UUID & 0xFF;
	adv_data[adv_index++] = BOARDING_UUID >> 8;
	adv_data[len_index] = 3;

	/* manufacturer */
	len_index = adv_index;
	adv_data[adv_index++] = 0x00;
	adv_data[adv_index++] = ADV_TYPE_MANUFACTURER_SPECIFIC;
	adv_data[adv_index++] = BEKEN_COMPANY_ID & 0xFF;
	adv_data[adv_index++] = BEKEN_COMPANY_ID >> 8;
	adv_data[len_index] = 3;

	if (bk_wifi_csi_ble_boarding_info == NULL)
	{
		bk_wifi_csi_ble_boarding_info = os_malloc(sizeof(ble_boarding_info_t));

		if (bk_wifi_csi_ble_boarding_info == NULL)
		{
			CSI_LOGI("bk_wifi_csi_ble_boarding_info malloc failed\n");

			goto error;
		}

		os_memset(bk_wifi_csi_ble_boarding_info, 0, sizeof(ble_boarding_info_t));
	}

	bk_wifi_csi_ble_boarding_info->cb = bk_wifi_csi_ble_recv_data_handle;

	ble_boarding_init(bk_wifi_csi_ble_boarding_info);
	ble_boarding_adv_start(adv_data, adv_index);

	return BK_OK;
error:
	return BK_FAIL;
}

static void bk_csi_ble_provisioning_thread_main(void *args)
{
	int ret;
	BUS_MSG_T msg;

	while (1) 
	{
		ret = rtos_pop_from_queue(&bk_csi_ble_provisioning_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret)
		continue;

		//BK_csi_LOGI("%s: send pkt cmd %d\n", __func__, msg.type);
		switch (msg.type)
		{
			case BK_CSI_BLE_SEND_PKT:
			{
				bk_wifi_csi_ble_handle_data_send(&msg);
			}break;

			case BK_CSI_BLE_RCV_PKT:
			{
				bk_wifi_csi_ble_handle_data_rcv(&msg);
			}break;

			case BK_CSI_BLE_DISCONNECT_IND:
			{
				bk_wifi_csi_disconnect_ind_handler();
			}break;

			default:
			{
				CSI_LOGI("ble_provisioning unkown msg %d\n", msg.type);
			}break;
		}

		if (msg.len)
			os_free(msg.param);
	}

	rtos_delete_thread(&bk_csi_ble_provisioning_thread);
}

bk_err_t bk_wifi_csi_ble_provisioning_init(void)
{
	int ret;
	// Create bk csi thread and initialize bk csi queue.
	if ((bk_csi_ble_provisioning_thread != NULL) || (bk_csi_ble_provisioning_queue != NULL) )
	{
		CSI_LOGI("BK csi ble provisioning queue or thread not deinitialize.\r\n");
		return BK_FAIL;
	}

	// Initialize bk csi queue
	ret = rtos_init_queue(&bk_csi_ble_provisioning_queue, "bk_csi_ble_provisioning_queue", sizeof(BUS_MSG_T), 16);
	if (ret != kNoErr)
	{
		CSI_LOGI("BK csi ble provisioning initialize queue fail.\r\n");
		return BK_FAIL;
	}

	// Create bk csi thread
#if CONFIG_FREERTOS_SMP
	ret = rtos_create_thread_with_affinity(&bk_csi_ble_provisioning_thread, -1, BEKEN_DEFAULT_WORKER_PRIORITY-1, "bk_csi_ble_provisioning",
											bk_csi_ble_provisioning_thread_main, 4096, (beken_thread_arg_t)0);
#else
	ret = rtos_create_thread(&bk_csi_ble_provisioning_thread, BEKEN_DEFAULT_WORKER_PRIORITY-1, "bk_csi_ble_provisioning",
							bk_csi_ble_provisioning_thread_main, 4096, (beken_thread_arg_t)0);
#endif
	if (ret != kNoErr)
	{
		CSI_LOGI("BK csi ble provisioning create thread fail.\r\n");
		return BK_FAIL;
	}

	ret = rtos_init_semaphore(&bk_csi_ble_provisioning_sema, 1);
	BK_ASSERT(0 == ret); /* ASSERT VERIFIED */

	extern bool ate_is_enabled(void);
	if (!ate_is_enabled())
	{
		bk_wifi_csi_ble_boarding_init();
	}
	else
	{
		CSI_LOGI("ATE is enable, ble adv disable!!!!!! \r\n");
	}

	return BK_OK;
}

void bk_wifi_csi_ble_provisioning_dinit(void)
{
	if (bk_wifi_csi_ble_boarding_info)
	{
		os_free(bk_wifi_csi_ble_boarding_info);
		bk_wifi_csi_ble_boarding_info = NULL;
	}

	if (bk_csi_ble_provisioning_queue)
	{
		rtos_deinit_queue(&bk_csi_ble_provisioning_queue);
		bk_csi_ble_provisioning_queue = NULL;
	}
	if (bk_csi_ble_provisioning_thread)
	{
		rtos_delete_thread(&bk_csi_ble_provisioning_thread);
		bk_csi_ble_provisioning_thread = NULL;
	}

	rtos_deinit_semaphore(&bk_csi_ble_provisioning_sema);
	bk_csi_ble_send_flag = false;
}
