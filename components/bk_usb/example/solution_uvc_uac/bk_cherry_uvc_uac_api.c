#include <os/os.h>
#include <os/mem.h>
#include <components/usb.h>
#include <components/usb_types.h>
#include "usbh_video.h"
#include "usbh_audio.h"
#include "usb_driver.h"
#include <driver/media_types.h>
#include <driver/audio_ring_buff.h>
#include "bk_uvc_uac_api.h"

#include <driver/gpio.h>
#if CONFIG_CHERRY_USB
#include "usbh_core.h"

typedef struct
{
	struct usbh_video *uvc_dev;
	struct usbh_audio *uac_dev;
	bk_usb_driver_comprehensive_ops *usb_driver;
	bk_uvc_uac_app_callback *uvc_all_cb;
	bk_uvc_uac_app_callback *uac_all_cb;
	bk_uvc_config_t *uvc_config;
	struct usbh_urb *uvc_video_urb;
	camera_packet_t *uvc_packet;
	uint8_t uvc_bulk_count;

	camera_packet_control_t *usb_uvc_buffer_ops_funcs;
	struct audio_packet_control_t *usb_uac_buffer_ops_funcs;
	bk_uac_config_t *uac_config;
	struct usbh_urb *uac_mic_urb;
	bool uac_mic_rx_msg_send_flag;
	struct usbh_urb *uac_spk_urb;
	bool uac_spk_tx_msg_send_flag;
	RingBufferContext *audio_mic_rb;
	uint8_t *mic_ring_buffer;
	uint8_t *mic_shuttle_buffer;
	RingBufferContext *audio_spk_rb;
	uint8_t *spk_ring_buffer;
	uint8_t *spk_shuttle_buffer;
} bk_uvc_uac_fusion_device;

static bk_uvc_uac_fusion_device *s_uvc_uac_device = {0};
static bool s_uvc_uac_sw_init_status = 0;

static bk_err_t bk_uvc_uac_updata_video_stream_buffer(bk_uvc_uac_fusion_device *uvc_uac_device);
static bk_err_t bk_uvc_uac_updata_audio_stream_rx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device);
static bk_err_t bk_uvc_uac_updata_audio_stream_tx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device);

static int bk_uvc_uac_demo_connect(struct usbh_hubport *hport, uint8_t intf)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	bk_usb_driver_comprehensive_ops *usb_driver = uvc_uac_device->usb_driver;

	usb_driver->hport = hport;

	bk_usb_drv_send_msg(USB_DRV_PRASE_DESCRIPTOR, (void *)usb_driver);
	bk_usb_drv_send_msg(USB_DRV_CONNECT_HANDLE, (void *)usb_driver);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return 0;

}

static void bk_uvc_uac_flush_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return;
	}

	if(uvc_uac_device->usb_uvc_buffer_ops_funcs) {
		if(uvc_uac_device->usb_uvc_buffer_ops_funcs->free && uvc_uac_device->uvc_packet) {
			uvc_uac_device->usb_uvc_buffer_ops_funcs->free(uvc_uac_device->uvc_packet);
			uvc_uac_device->uvc_packet = NULL;
		}
	}
	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

}

void bk_usb_uvc_uac_free_enumerate_resources()
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	struct usbh_video *uvc_dev = uvc_uac_device->uvc_dev;
	struct usbh_audio *uac_dev = uvc_uac_device->uac_dev;

	bk_uvc_uac_flush_buffer(uvc_uac_device);

	uvc_uac_device->usb_driver->usbh_class_start_status = 0x0;
	uvc_uac_device->usb_driver->usbh_class_connect_status = 0x0;

	if(uvc_dev)
		bk_usbh_video_sw_deinit(uvc_dev->hport, uvc_dev->ctrl_intf, 0x01);
	if(uac_dev)
		bk_usbh_audio_sw_deinit(uac_dev->hport, uac_dev->ctrl_intf, 0x01);

}

static int bk_uvc_uac_demo_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;

	uvc_uac_device->usb_driver->usbh_class_start_status = 0x0;
	uvc_uac_device->usb_driver->usbh_class_connect_status = 0x0;

#if 0
	bk_uvc_uac_flush_buffer(uvc_uac_device);

	struct usbh_video *uvc_dev = uvc_uac_device->uvc_dev;
	struct usbh_audio *uac_dev = uvc_uac_device->uac_dev;

	if(uvc_dev)
		bk_usbh_video_sw_deinit(uvc_dev->hport, uvc_dev->ctrl_intf, 0x01);
	if(uac_dev)
		bk_usbh_audio_sw_deinit(uac_dev->hport, uac_dev->ctrl_intf, 0x01);
#endif
	bk_usb_drv_send_msg_front(USB_DRV_DISCONNECT_HANDLE, (void *)uvc_uac_device->usb_driver);

	return 0;

}

const struct usbh_class_driver s_video_class_ctrl_intf_driver = {
    .driver_name = "video",
    .connect = bk_uvc_uac_demo_connect,
    .disconnect = bk_uvc_uac_demo_disconnect
};

const struct usbh_class_info s_video_audio_intf_driver = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS | USB_CLASS_MATCH_INTF_SUBCLASS | USB_CLASS_MATCH_INTF_PROTOCOL,
    .class = USB_DEVICE_CLASS_VIDEO,
    .subclass = VIDEO_SC_VIDEOCONTROL,
    .protocol = VIDEO_PC_PROTOCOL_UNDEFINED,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &s_video_class_ctrl_intf_driver
};

bk_err_t bk_usb_uvc_uac_sw_init(void *set_config)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(s_uvc_uac_sw_init_status) {
		USB_DRIVER_LOGD("%s INITED\r\n", __func__);
		return BK_FAIL;
	}
	usbh_register_class_driver(USB_HOST_MODE, (void *)&s_video_audio_intf_driver);

	bool malloc_success_flag = 0;

	do{
		s_uvc_uac_device = (bk_uvc_uac_fusion_device *)os_malloc(sizeof(bk_uvc_uac_fusion_device));
		if(!s_uvc_uac_device) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device, 0x0, sizeof(bk_uvc_uac_fusion_device));

		s_uvc_uac_device->usb_driver = (bk_usb_driver_comprehensive_ops *)os_malloc(sizeof(bk_usb_driver_comprehensive_ops));
		if(!s_uvc_uac_device->usb_driver) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->usb_driver, 0x0, sizeof(bk_usb_driver_comprehensive_ops));

		for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
		{
			s_uvc_uac_device->usb_driver->usbh_connect_cb[dev] = NULL;
		}

		s_uvc_uac_device->uvc_config = (bk_uvc_config_t *)os_malloc(sizeof(bk_uvc_config_t));
		if(!s_uvc_uac_device->uvc_config) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uvc_config, 0x0, sizeof(bk_uvc_config_t));

		s_uvc_uac_device->uvc_all_cb = (bk_uvc_uac_app_callback *)os_malloc(sizeof(bk_uvc_uac_app_callback));
		if(!s_uvc_uac_device->uvc_all_cb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uvc_all_cb, 0x0, sizeof(bk_uvc_uac_app_callback));

		s_uvc_uac_device->uac_config = (bk_uac_config_t *)os_malloc(sizeof(bk_uac_config_t));
		if(!s_uvc_uac_device->uac_config) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uac_config, 0x0, sizeof(bk_uac_config_t));

		s_uvc_uac_device->uac_all_cb = (bk_uvc_uac_app_callback *)os_malloc(sizeof(bk_uvc_uac_app_callback));
		if(!s_uvc_uac_device->uac_all_cb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->uac_all_cb, 0x0, sizeof(bk_uvc_uac_app_callback));

		s_uvc_uac_device->audio_mic_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
		if(!s_uvc_uac_device->audio_mic_rb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->audio_mic_rb, 0x0, sizeof(RingBufferContext));

		s_uvc_uac_device->audio_spk_rb = (RingBufferContext *)os_malloc(sizeof(RingBufferContext));
		if(!s_uvc_uac_device->audio_spk_rb) {
			malloc_success_flag = 1;
			break;
		} else
			os_memset((void *)s_uvc_uac_device->audio_spk_rb, 0x0, sizeof(RingBufferContext));		
	} while(0);

	if(malloc_success_flag) {

		if(s_uvc_uac_device->uvc_config) {
			os_free(s_uvc_uac_device->uvc_config);
			s_uvc_uac_device->uvc_config = NULL;
		}

		if(s_uvc_uac_device->uac_config) {
			os_free(s_uvc_uac_device->uac_config);
			s_uvc_uac_device->uac_config = NULL;
		}

		if(s_uvc_uac_device->usb_driver) {
			os_free(s_uvc_uac_device->usb_driver);
			s_uvc_uac_device->usb_driver = NULL;
		}

		if(s_uvc_uac_device->uvc_all_cb) {
			os_free(s_uvc_uac_device->uvc_all_cb);
			s_uvc_uac_device->uvc_all_cb = NULL;
		}

		if(s_uvc_uac_device->uac_all_cb) {
			os_free(s_uvc_uac_device->uac_all_cb);
			s_uvc_uac_device->uac_all_cb = NULL;
		}
		if(s_uvc_uac_device->audio_mic_rb) {
			os_free(s_uvc_uac_device->audio_mic_rb);
			s_uvc_uac_device->audio_mic_rb = NULL;
		}
		if(s_uvc_uac_device->audio_spk_rb) {
			os_free(s_uvc_uac_device->audio_spk_rb);
			s_uvc_uac_device->audio_spk_rb = NULL;
		}
		if(s_uvc_uac_device) {
			os_free(s_uvc_uac_device);
			s_uvc_uac_device = NULL;
		}
		return BK_FAIL;
	}
	s_uvc_uac_sw_init_status = 1;

	USB_DRIVER_LOGD("[-]%s malloc_success_flag:%d\r\n",__func__, malloc_success_flag);

	return BK_OK;

}

bk_err_t bk_usb_uvc_uac_sw_deinit()
{
	if(!s_uvc_uac_sw_init_status) {
		USB_DRIVER_LOGD("%s DEINITED\r\n", __func__);
		return BK_FAIL;
	}

	if(s_uvc_uac_device->uvc_config) {
		os_free(s_uvc_uac_device->uvc_config);
		s_uvc_uac_device->uvc_config = NULL;
	}

	if(s_uvc_uac_device->uac_config) {
		os_free(s_uvc_uac_device->uac_config);
		s_uvc_uac_device->uac_config = NULL;
	}

	if(s_uvc_uac_device->usb_driver) {
		os_free(s_uvc_uac_device->usb_driver);
		s_uvc_uac_device->usb_driver = NULL;
	}

	if(s_uvc_uac_device->uvc_all_cb) {
		os_free(s_uvc_uac_device->uvc_all_cb);
		s_uvc_uac_device->uvc_all_cb = NULL;
	}

	if(s_uvc_uac_device->uac_all_cb) {
		os_free(s_uvc_uac_device->uac_all_cb);
		s_uvc_uac_device->uac_all_cb = NULL;
	}
	if(s_uvc_uac_device->audio_mic_rb) {
		os_free(s_uvc_uac_device->audio_mic_rb);
		s_uvc_uac_device->audio_mic_rb = NULL;
	}
	if(s_uvc_uac_device->audio_spk_rb) {
		os_free(s_uvc_uac_device->audio_spk_rb);
		s_uvc_uac_device->audio_spk_rb = NULL;
	}
	if(s_uvc_uac_device) {
		os_free(s_uvc_uac_device);
		s_uvc_uac_device = NULL;
	}
	s_uvc_uac_sw_init_status = 0;

	return BK_OK;

}

bk_err_t bk_uvc_uac_register_callback(bk_uvc_uac_app_callback *uvc_uac_all_cb)
{
	bk_uvc_uac_fusion_device *device = s_uvc_uac_device;
	bk_usb_driver_comprehensive_ops *usb_driver = device->usb_driver;
	bk_uvc_uac_app_callback *cb = uvc_uac_all_cb;

	switch (cb->dev) {
		case USB_UVC_DEVICE:
			os_memcpy((void *)device->uvc_all_cb, cb, sizeof(bk_uvc_uac_app_callback));
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_DISCONNECT, cb->dev, cb->uvc_uac_disconnect_cb);
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_CONNECT, cb->dev, cb->uvc_uac_connect_cb);
			break;
		case USB_UAC_MIC_DEVICE:
		case USB_UAC_SPEAKER_DEVICE:
			os_memcpy((void *)device->uac_all_cb, cb, sizeof(bk_uvc_uac_app_callback));
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_DISCONNECT, cb->dev, cb->uvc_uac_disconnect_cb);
			bk_usb_register_connection_status_notification_callback(usb_driver, USB_DEVICE_CONNECT, cb->dev, cb->uvc_uac_connect_cb);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t bk_usb_uvc_register_disconnect_callback(void *param)
{
	bk_uvc_uac_app_callback *uvc_all_cb = s_uvc_uac_device->uvc_all_cb;
	uvc_all_cb->dev = USB_UVC_DEVICE;
	uvc_all_cb->uvc_uac_disconnect_cb = param;
	bk_uvc_uac_register_callback(uvc_all_cb);

	return BK_OK;
}

bk_err_t bk_usb_uvc_register_connect_callback(void *param)
{
	bk_uvc_uac_app_callback *uvc_all_cb = s_uvc_uac_device->uvc_all_cb;
	uvc_all_cb->dev = USB_UVC_DEVICE;
	uvc_all_cb->uvc_uac_connect_cb = param;
	bk_uvc_uac_register_callback(uvc_all_cb);

	return BK_OK;
}

void bk_usb_updata_video_interface(void *hport, uint8_t bInterfaceNumber, uint8_t interface_sub_class)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;

	bk_usbh_video_sw_init((struct usbh_hubport *)hport, bInterfaceNumber, interface_sub_class);
	uvc_uac_device->uvc_dev = (struct usbh_video *)usbh_find_class_instance("/dev/video0");
    if (uvc_uac_device->uvc_dev == NULL) {
        USB_DRIVER_LOGW("do not find /dev/video0\r\n");
        return;
    }
/*
	if(((struct usbh_hubport *)hport)->config.intf[bInterfaceNumber].priv)
		uvc_uac_device->uvc_dev = (struct usbh_video *)(((struct usbh_hubport *)hport)->config.intf[bInterfaceNumber].priv);
*/
	uvc_uac_device->usb_driver->usbh_class_connect_status |= (0x1 << USB_UVC_DEVICE);
	struct usbh_video *uvc_device = uvc_uac_device->uvc_dev;
	uint8_t frame_index = 0;
	uint8_t fps_index = 0;
	uint32_t frame_interval = 0;
	for(int index = 0; index < 3; index++)
	{
		switch(uvc_device->format[index].format_type)
		{
			case USBH_VIDEO_FORMAT_UNCOMPRESSED:
			case USBH_VIDEO_FORMAT_MJPEG:
			case USBH_VIDEO_FORMAT_H264:
			case USBH_VIDEO_FORMAT_H265:
				for(frame_index = 0; frame_index < uvc_device->format[index].num_of_frames; frame_index++)
				{
					for(fps_index = 0; fps_index < uvc_device->format[index].frame[frame_index].fps_num; fps_index++)
					{
						frame_interval = uvc_device->format[index].frame[frame_index].fps[fps_index];
						if(frame_interval > UVC_MAX_FPS) {
							uvc_device->format[index].frame[frame_index].fps[fps_index] = UVC_CALCULATE_FRAME_RATE_VALUE_NUM/frame_interval;
						}
					}
				}
				break;
			default:
				break;
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
	return;

}

bk_err_t bk_usb_uvc_get_param(bk_uvc_device_brief_info_t *uvc_device_info)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(BK_OK != bk_usb_check_device_supported(USB_UVC_DEVICE)) {
		USB_DRIVER_LOGE("%s unsupprted UVC\r\n", __func__);
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;
	}

	if(!uvc_device_info)
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;

	struct usb_device_descriptor_t dev_descriptor;
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	struct usbh_video *uvc_device = uvc_uac_device->uvc_dev;

	if(!uvc_device) {
		USB_DRIVER_LOGE("%s uvc_device is NULL\r\n",__func__);
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;
	}

	bk_usb_get_device_descriptor(&dev_descriptor);

	uvc_device_info->vendor_id = dev_descriptor.idVendor;
	uvc_device_info->product_id = dev_descriptor.idProduct;
	uvc_device_info->device_bcd = dev_descriptor.bcdDevice;
	uvc_device_info->support_devs = uvc_uac_device->usb_driver->usbh_class_connect_status;

	for(int index = 0; index < 3; index++)
	{
		switch(uvc_device->format[index].format_type)
		{
			case USBH_VIDEO_FORMAT_UNCOMPRESSED:
				uvc_device_info->all_frame.yuv_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.yuv_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.yuv_format_index = index + 1;
				break;
			case USBH_VIDEO_FORMAT_MJPEG:
				uvc_device_info->all_frame.mjpeg_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.mjpeg_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.mjpeg_format_index = index + 1;
				break;
			case USBH_VIDEO_FORMAT_H264:
				uvc_device_info->all_frame.h264_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.h264_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.h264_format_index = index + 1;
				break;
			case USBH_VIDEO_FORMAT_H265:
				uvc_device_info->all_frame.h265_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.h265_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.h265_format_index = index + 1;
				break;
			default:
				break;
		}
	}

	for (uint8_t i = 0; i < uvc_device->num_of_intf_altsettings; i++) {
		uvc_device_info->endpoints_num = uvc_device->hport->config.intf[uvc_device->data_intf].altsetting[i].intf_desc.bNumEndpoints;
		uvc_device_info->ep_desc = (struct s_bk_usb_endpoint_descriptor *)&uvc_device->hport->config.intf[uvc_device->data_intf].altsetting[i].ep[0].ep_desc;
	}
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uvc_set_param(bk_uvc_config_t *uvc_config)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(BK_OK != bk_usb_check_device_supported(USB_UVC_DEVICE)) {
		USB_DRIVER_LOGE("%s unsupprted UVC\r\n", __func__);
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;
	}

	if(!uvc_config)
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;

	bk_uvc_uac_fusion_device *device = s_uvc_uac_device;

	device->uvc_config->vendor_id = uvc_config->vendor_id;
	device->uvc_config->product_id = uvc_config->product_id;
	device->uvc_config->width = uvc_config->width;
	device->uvc_config->height = uvc_config->height;
	device->uvc_config->fps = uvc_config->fps;
	device->uvc_config->frame_index = uvc_config->frame_index;
	device->uvc_config->format_index = uvc_config->format_index;
	device->uvc_config->ep_desc = uvc_config->ep_desc;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uvc_register_transfer_buffer_ops(void *ops)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	s_uvc_uac_device->usb_uvc_buffer_ops_funcs = (camera_packet_control_t *)ops;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

static void bk_uvc_video_stream_rxed_callback(void *pCompleteParam, int nbytes)
{
	USB_DRIVER_LOGD("[+]%s \r\n", __func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	struct usbh_video *uvc_device = uvc_uac_device->uvc_dev;
	struct usbh_urb *video_urb = (struct usbh_urb *)pCompleteParam;
	camera_packet_t *packet = uvc_uac_device->uvc_packet;

	if(uvc_uac_device->usb_uvc_buffer_ops_funcs == NULL)
		return;

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		if(uvc_uac_device->usb_uvc_buffer_ops_funcs->free) {
			uvc_uac_device->usb_uvc_buffer_ops_funcs->free(uvc_uac_device->uvc_packet);
			uvc_uac_device->uvc_packet = NULL;
		}
		return;
	}

	if(nbytes >= 0) {
		if((uvc_uac_device->uvc_config->ep_desc->bmAttributes & 0x3) == USB_ENDPOINT_ISOCH_TRANSFER) {//ISOCH
			bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
		} else if((uvc_uac_device->uvc_config->ep_desc->bmAttributes & 0x3) == USB_ENDPOINT_BULK_TRANSFER) {//BULK
			video_urb->iso_packet[uvc_uac_device->uvc_bulk_count].actual_length = video_urb->actual_length;
			video_urb->iso_packet[uvc_uac_device->uvc_bulk_count].errorcode = 0;
			//USB_DRIVER_LOGE("%s nbytes:%d actual_length:%d uvc_bulk_count:%d\r\n",__func__, nbytes, video_urb->actual_length, uvc_uac_device->uvc_bulk_count);
			uvc_uac_device->uvc_bulk_count++;
			if(uvc_uac_device->uvc_bulk_count >= video_urb->num_of_iso_packets) {
				bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
				video_urb->transfer_buffer_length = 0;
			} else {
				video_urb->transfer_buffer = video_urb->iso_packet[uvc_uac_device->uvc_bulk_count].transfer_buffer;
				video_urb->transfer_buffer_length = (packet->data_buffer_size/packet->num_packets);
				video_urb->actual_length = 0;
			}
		}
	} else {
		USB_DRIVER_LOGE("%s error: %d\r\n",__func__, nbytes);
		bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
	}


	USB_DRIVER_LOGD("[-]%s \r\n", __func__);

	return;
}

static bk_err_t bk_uvc_uac_updata_video_stream_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{	
	USB_DRIVER_LOGD("[+]%s \r\n", __func__);

	if(uvc_uac_device->usb_uvc_buffer_ops_funcs) {
		if(uvc_uac_device->uvc_packet == NULL)
			uvc_uac_device->uvc_packet = uvc_uac_device->usb_uvc_buffer_ops_funcs->malloc();
	} else
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	camera_packet_t *packet = uvc_uac_device->uvc_packet;
	struct usbh_video *uvc_device = uvc_uac_device->uvc_dev;

	if(!packet) {
		USB_DRIVER_LOGE("%s packet is NULL\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	} else
		uvc_uac_device->uvc_bulk_count = 0;

	if(!uvc_uac_device->uvc_video_urb) {
		uvc_uac_device->uvc_video_urb = usb_malloc(sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet) * packet->num_packets);
		if (uvc_uac_device->uvc_video_urb == NULL) {
			USB_DRIVER_LOGE("No memory to alloc urb\r\n");
			return BK_ERR_USB_OPERATION_NULL_POINTER;
		} else
			memset(uvc_uac_device->uvc_video_urb, 0, sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet) * packet->num_packets);
		
		uvc_uac_device->uvc_video_urb->pipe = uvc_device->isoin;
		uvc_uac_device->uvc_video_urb->complete = (usbh_complete_callback_t)bk_uvc_video_stream_rxed_callback;
		uvc_uac_device->uvc_video_urb->arg = uvc_uac_device->uvc_video_urb;

		if((uvc_uac_device->uvc_config->ep_desc->bmAttributes & 0x3) == USB_ENDPOINT_ISOCH_TRANSFER) {//ISOCH
			if((uvc_uac_device->uvc_dev->hport->speed) == USB_SPEED_HIGH) {
				uvc_uac_device->uvc_video_urb->timeout = 150;//150us
			}
			if((uvc_uac_device->uvc_dev->hport->speed) == USB_SPEED_FULL) {
				uvc_uac_device->uvc_video_urb->timeout = 1000;//1ms
			}
		} else if((uvc_uac_device->uvc_config->ep_desc->bmAttributes & 0x3) == USB_ENDPOINT_BULK_TRANSFER) {//BULK
			uvc_uac_device->uvc_video_urb->timeout = 0;
		}

	}
	struct usbh_urb *video_urb = uvc_uac_device->uvc_video_urb;
	uint8_t *tmp_buf = packet->data_buffer;
	
	video_urb->transfer_buffer = packet->data_buffer;
	video_urb->transfer_buffer_length = (packet->data_buffer_size/packet->num_packets);
	video_urb->num_of_iso_packets = packet->num_packets;
	for (uint32_t i = 0; i < packet->num_packets; i++) {
		video_urb->iso_packet[i].transfer_buffer = tmp_buf;
		video_urb->iso_packet[i].transfer_buffer_length = (packet->data_buffer_size/packet->num_packets);
		tmp_buf+=(packet->data_buffer_size/packet->num_packets);/* enable this when use ram larger than 4M */
	}

	USB_DRIVER_LOGD("[-]%s \r\n", __func__);
	return BK_OK;
}

bk_err_t bk_uvc_start(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s uvc disconnect \r\n",__func__);
		return BK_FAIL;
	}

	if(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UVC_DEVICE)) {
		USB_DRIVER_LOGE("%s is start\r\n",__func__);
		return BK_FAIL;
	}
	struct usbh_video *uvc_device = uvc_uac_device->uvc_dev;

	uvc_uac_device->usb_driver->usbh_class_start_status |= (0x1 << USB_UVC_DEVICE);
	bk_usb_drv_send_msg(USB_DRV_VIDEO_START, (void *)uvc_device);
	//bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

static void bk_uvc_video_clear(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	if(uvc_uac_device->uvc_video_urb) {
		struct usbh_urb *uvc_video_urb = uvc_uac_device->uvc_video_urb;
		uvc_uac_device->uvc_video_urb = NULL;
		os_free(uvc_video_urb);
	}

	uvc_uac_device->uvc_packet = NULL;
}

void bk_usbh_video_start_handle(struct usbh_video *uvc_device)
{
	if(!uvc_device)
		return;

	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	bk_uvc_config_t *config = uvc_uac_device->uvc_config;
	uint8_t formatindex = config->format_index;
	uint8_t frameindex = config->frame_index;
	uint32_t dwMaxVideoFrameSize = (config->width) * (config->height) * 2;
	uint32_t dwMaxPayloadTransferSize = config->ep_desc->wMaxPacketSize;
	uint8_t altsettings = 0;
	int ret = 0;

#if 0
	uint32_t version_buffer_len = 0;
	if(uvc_device->bcdVDC == 0x0100)
		version_buffer_len = 26;
	else if(uvc_device->bcdVDC == 0x0101)
		version_buffer_len = 34;
	else
		version_buffer_len = 48;
#endif

	usbh_videostreaming_get_cur_probe(uvc_device);
	dwMaxVideoFrameSize = uvc_device->probe.dwMaxVideoFrameSize;
	dwMaxPayloadTransferSize = uvc_device->probe.dwMaxPayloadTransferSize;
	uvc_device->probe.dwFrameInterval = UVC_CALCULATE_FRAME_RATE_VALUE_NUM/config->fps;
	usbh_videostreaming_set_cur_probe(uvc_device, formatindex, frameindex, dwMaxVideoFrameSize, dwMaxPayloadTransferSize);

	usbh_videostreaming_get_cur_probe(uvc_device);
	dwMaxVideoFrameSize = uvc_device->probe.dwMaxVideoFrameSize;
	dwMaxPayloadTransferSize = uvc_device->probe.dwMaxPayloadTransferSize;
	usbh_videostreaming_set_cur_commit(uvc_device, formatindex, frameindex, dwMaxVideoFrameSize, dwMaxPayloadTransferSize); /* select resolution from list */
	usbh_videostreaming_get_cur_probe(uvc_device);

	for (uint8_t i = 1; i < uvc_device->num_of_intf_altsettings; i++) {
		altsettings = uvc_device->hport->config.intf[uvc_device->data_intf].altsetting[i].intf_desc.bAlternateSetting;
	}

	ret = usbh_video_open(uvc_device, altsettings); /* select ep mps from altsettings ,just for reference now */
	if(ret < 0) {
		USB_DRIVER_LOGE("usbh_video_open:%d\r\n", ret);
		ret = usbh_video_open(uvc_device, altsettings);
		if(ret < 0) USB_DRIVER_LOGE("CHECK VIDEO OPEN FAIL\n");
	}

	bk_uvc_video_clear(uvc_uac_device);

	bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
}

bk_err_t bk_uvc_trigger_video_stream_rx(void *uvc_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!uvc_device) {
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		return BK_FAIL;
	}
	struct usbh_urb *video_urb = uvc_uac_device->uvc_video_urb;
	if((uvc_uac_device->uvc_packet != NULL) && (video_urb != NULL)) {

		if(!(uvc_uac_device->uvc_packet->actual_num_byte)) {
			return BK_FAIL;
		}
		if(!(uvc_uac_device->uvc_packet->state)) {
			return BK_FAIL;
		}

		for (uint32_t i = 0; i < video_urb->num_of_iso_packets; i++) {
			uvc_uac_device->uvc_packet->actual_num_byte[i] = video_urb->iso_packet[i].actual_length;
			uvc_uac_device->uvc_packet->state[i] = video_urb->iso_packet[i].errorcode;
			//USB_DRIVER_LOGE("%s i:%d actual_num_byte:%d state:%d\r\n",__func__, i, uvc_uac_device->uvc_packet->actual_num_byte[i], uvc_uac_device->uvc_packet->state[i]);
		}

		if(video_urb->errorcode) {
			uvc_uac_device->uvc_packet->packet_state = CAM_STREAM_ERROR;
		}
		else{
			uvc_uac_device->uvc_packet->packet_state = CAM_STREAM_READY;
		}

		uvc_uac_device->usb_uvc_buffer_ops_funcs->push(uvc_uac_device->uvc_packet);
		uvc_uac_device->uvc_packet = NULL;
	}
	int ret = 0;
	ret = bk_uvc_uac_updata_video_stream_buffer(uvc_uac_device);
	if(ret != BK_OK) {
		return ret;
	}

	ret = usbh_submit_urb(uvc_uac_device->uvc_video_urb);
	if (ret < 0) {
		switch(-ret)
		{
			case EBUSY:
				USB_DRIVER_LOGE("%s Urb is busy\r\n", __func__);
				bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
				break;
			case ENODEV:
				USB_DRIVER_LOGE("%s Please check device connect\r\n", __func__);
				break;
			case EINVAL:
				USB_DRIVER_LOGE("%s Please check pipe or urb\r\n", __func__);
				bk_usb_drv_send_msg(USB_DRV_VIDEO_START, (void *)uvc_device);
				break;
			case ESHUTDOWN:
				USB_DRIVER_LOGE("%s Check device Disconnect\r\n", __func__);
				break;
			case ETIMEDOUT:
				USB_DRIVER_LOGE("%s Timeout wait\r\n", __func__);
				bk_usb_drv_send_msg(USB_DRV_VIDEO_RX, (void *)uvc_device);
				break;
			default:
				USB_DRIVER_LOGE("%s Fail to submit urb:%d\r\n", __func__, ret);
				break;
		}
	}

	USB_DRIVER_LOGD("[-]%s ret:%d\r\n",__func__, ret);

	return ret;
}

bk_err_t bk_uvc_stop(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s uvc disconnect\r\n",__func__);
		return BK_FAIL;
	}

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UVC_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		return BK_FAIL;
	}
	uvc_uac_device->usb_driver->usbh_class_start_status &= ~(0x1 << USB_UVC_DEVICE);

	struct usbh_video *uvc_device = uvc_uac_device->uvc_dev;
	bk_usb_drv_send_msg(USB_DRV_VIDEO_STOP, (void *)uvc_device);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
	return BK_OK;
}

void bk_usbh_video_stop_handle(struct usbh_video *uvc_device)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	usbh_video_close(uvc_device);
	bk_uvc_video_clear(uvc_uac_device);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

bk_err_t bk_usb_uac_register_disconnect_callback(void *param)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_uvc_uac_app_callback *uac_all_cb = s_uvc_uac_device->uac_all_cb;
	uac_all_cb->dev = USB_UAC_MIC_DEVICE;
	uac_all_cb->uvc_uac_disconnect_cb = param;
	bk_uvc_uac_register_callback(uac_all_cb);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_uac_register_connect_callback(void *param)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_uvc_uac_app_callback *uac_all_cb = s_uvc_uac_device->uac_all_cb;
	uac_all_cb->dev = USB_UAC_MIC_DEVICE;
	uac_all_cb->uvc_uac_connect_cb = param;
	bk_uvc_uac_register_callback(uac_all_cb);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

void bk_usb_updata_audio_interface(void *hport, uint8_t bInterfaceNumber, uint8_t interface_sub_class)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;

	bk_usbh_audio_sw_init((struct usbh_hubport *)hport, bInterfaceNumber, interface_sub_class);

	uvc_uac_device->uac_dev = (struct usbh_audio *)usbh_find_class_instance("/dev/audio0");
    if (uvc_uac_device->uac_dev == NULL) {
        USB_DRIVER_LOGW("do not find /dev/audio0\r\n");
        return;
    }
	/*
	if(((struct usbh_hubport *)hport)->config.intf[bInterfaceNumber].priv)
		uvc_uac_device->uac_dev = (struct usbh_audio *)(((struct usbh_hubport *)hport)->config.intf[bInterfaceNumber].priv);
	*/

	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		uvc_uac_device->usb_driver->usbh_class_connect_status |= (0x1 << USB_UAC_SPEAKER_DEVICE);
		usbh_audio_open(uvc_uac_device->uac_dev, "speaker", 8000);
		usbh_audio_close(uvc_uac_device->uac_dev, "speaker");
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		uvc_uac_device->usb_driver->usbh_class_connect_status |= (0x1 << USB_UAC_MIC_DEVICE);
		usbh_audio_open(uvc_uac_device->uac_dev, "mic", 8000);
		usbh_audio_close(uvc_uac_device->uac_dev, "mic");
	}

}

static void bk_usbh_audio_stream_rxed_callback(void *pCompleteParam, int nbytes)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	struct usbh_audio *uac_device = uvc_uac_device->uac_dev;
	struct usbh_urb *mic_urb = uvc_uac_device->uac_mic_urb;

	if(mic_urb == NULL)
		return;

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		uvc_uac_device->uac_mic_urb->transfer_buffer_length = 0;
		os_free(uvc_uac_device->mic_ring_buffer);
		uvc_uac_device->mic_ring_buffer = NULL;
		return;
	}
	/*write ring buffer*/
	ring_buffer_write(uvc_uac_device->audio_mic_rb, uvc_uac_device->uac_mic_urb->transfer_buffer, uvc_uac_device->uac_config->mic_ep_desc->wMaxPacketSize);
	if(ring_buffer_get_free_size(uvc_uac_device->audio_mic_rb) < (uvc_uac_device->audio_mic_rb->capacity / 2))
	{
		if(uvc_uac_device->uac_mic_rx_msg_send_flag) return;
		bk_usb_drv_send_msg(USB_DRV_AUDIO_RX, (void *)uac_device);
		uvc_uac_device->uac_mic_rx_msg_send_flag = true;
	}

	return;
}

static bk_err_t bk_uvc_uac_updata_audio_stream_rx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	audio_packet_t packet;

	USB_DRIVER_LOGD("[+]%s \r\n", __func__);
	if(!(s_uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE))) return BK_FAIL;

	if(ring_buffer_get_free_size(uvc_uac_device->audio_mic_rb) < (uvc_uac_device->audio_mic_rb->capacity / 2))
	{
		ring_buffer_read(uvc_uac_device->audio_mic_rb, uvc_uac_device->mic_shuttle_buffer, (uvc_uac_device->audio_mic_rb->capacity / 2));
		packet.data_buffer = uvc_uac_device->mic_shuttle_buffer;
		packet.data_buffer_size = (uvc_uac_device->audio_mic_rb->capacity / 2);
		
		uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_push(&packet);
	}

	struct usbh_audio *uac_device = uvc_uac_device->uac_dev;

	if(!uvc_uac_device->uac_mic_urb) {
		uvc_uac_device->mic_ring_buffer = (uint8_t *)os_malloc(uvc_uac_device->uac_config->mic_ep_desc->wMaxPacketSize);

		uvc_uac_device->uac_mic_urb = usb_malloc(sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet));
		if (uvc_uac_device->uac_mic_urb == NULL) {
			USB_DRIVER_LOGE("No memory to alloc urb\r\n");
			return BK_ERR_USB_OPERATION_NULL_POINTER;
		} else
			memset(uvc_uac_device->uac_mic_urb, 0, sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet));

		struct usbh_urb *mic_urb = uvc_uac_device->uac_mic_urb;

		mic_urb->transfer_buffer = uvc_uac_device->mic_ring_buffer;//packet->data_buffer;
		mic_urb->transfer_buffer_length = uvc_uac_device->uac_config->mic_ep_desc->wMaxPacketSize;//packet->data_buffer_size;
		mic_urb->pipe = uac_device->isoin;
		mic_urb->num_of_iso_packets = 1;//packet->num_packets;
		mic_urb->timeout = 0;//0xffffffff;
		mic_urb->complete = (usbh_complete_callback_t)bk_usbh_audio_stream_rxed_callback;
		mic_urb->iso_packet[0].transfer_buffer = uvc_uac_device->mic_ring_buffer;
		mic_urb->iso_packet[0].transfer_buffer_length = uvc_uac_device->uac_config->mic_ep_desc->wMaxPacketSize;

		mic_urb->arg = mic_urb;
	}

	uvc_uac_device->uac_mic_rx_msg_send_flag = false;

	USB_DRIVER_LOGD("[-]%s \r\n", __func__);
	return BK_OK;
}

bk_err_t bk_uac_start_mic()
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		return BK_FAIL;
	}
	if(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE)) {
		return BK_FAIL;
	}

	struct usbh_audio *uac_device = uvc_uac_device->uac_dev;

	uvc_uac_device->usb_driver->usbh_class_start_status |= (0x1 << USB_UAC_MIC_DEVICE);
	bk_usb_drv_send_msg(USB_DRV_AUDIO_MIC_START, (void *)uac_device);
	//bk_usb_drv_send_msg(USB_DRV_AUDIO_RX, (void *)uac_device);

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

static void bk_uac_mic_clear(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	if(uvc_uac_device->uac_mic_urb) {
		struct usbh_urb *uac_mic_urb = uvc_uac_device->uac_mic_urb;
		uvc_uac_device->uac_mic_urb = NULL;
		os_free(uac_mic_urb);
	}

	ring_buffer_clear(uvc_uac_device->audio_mic_rb);
	if(uvc_uac_device->audio_mic_rb->address) {
		os_free(uvc_uac_device->audio_mic_rb->address);
		uvc_uac_device->audio_mic_rb->address = NULL;
	}

	if(uvc_uac_device->mic_shuttle_buffer) {
		os_free(uvc_uac_device->mic_shuttle_buffer);
		uvc_uac_device->mic_shuttle_buffer = NULL;
	}

	if(uvc_uac_device->mic_ring_buffer) {
		os_free(uvc_uac_device->mic_ring_buffer);
		uvc_uac_device->mic_ring_buffer = NULL;
	}
	uvc_uac_device->uac_mic_rx_msg_send_flag = false;
}

void bk_uac_start_mic_handle(struct usbh_audio *uac_device)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	bk_uac_mic_clear(uvc_uac_device);
	bk_uac_config_t *uac_config = uvc_uac_device->uac_config;
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	audio_packet_t *packet;
	if(!(uvc_uac_device->usb_uac_buffer_ops_funcs)) return;
	packet = uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_malloc(USB_UAC_MIC_DEVICE);

	if(!packet) return;
	uint32_t pool_buffer_size = packet->data_buffer_size;
	USB_DRIVER_LOGI("%s pool_buffer_size:%d\r\n",__func__,pool_buffer_size);
	
	uint8_t *mic_ring_buffer = (uint8_t *)os_malloc(pool_buffer_size);
	uvc_uac_device->mic_shuttle_buffer = (uint8_t *)os_malloc((pool_buffer_size / 2));
	
	ring_buffer_init(uvc_uac_device->audio_mic_rb, mic_ring_buffer, pool_buffer_size, DMA_ID_MAX, 0);

	usbh_audio_open(uac_device, "mic", uac_config->mic_samples_frequence);

	int ret = 0;
	bk_uvc_uac_updata_audio_stream_rx_buffer(uvc_uac_device);

	ret = usbh_submit_urb(uvc_uac_device->uac_mic_urb);
	if (ret < 0) {
		USB_DRIVER_LOGD("%s Fail to submit urb:%d\r\n", __func__, ret);
	}
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

bk_err_t bk_uac_trigger_audio_stream_rx(void *uac_device)
{
	if(!(s_uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE))) return BK_FAIL;

	if(!uac_device) {
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	return bk_uvc_uac_updata_audio_stream_rx_buffer(uvc_uac_device);
}

bk_err_t bk_uac_stop_mic()
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		return BK_FAIL;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_MIC_DEVICE))) {
		return BK_FAIL;
	}
	uvc_uac_device->usb_driver->usbh_class_start_status &= ~(0x1 << USB_UAC_MIC_DEVICE);
	if(uvc_uac_device->uac_mic_urb)
		uvc_uac_device->uac_mic_urb->transfer_buffer_length = 0;

	bk_usb_drv_send_msg(USB_DRV_AUDIO_MIC_STOP, (void *)uvc_uac_device->uac_dev);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

void bk_uac_stop_mic_handle(struct usbh_audio *uac_device)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	usbh_audio_close(uac_device, "mic");
	bk_uac_mic_clear(uvc_uac_device);
}

static void bk_usbh_audio_stream_txed_callback(void *pCompleteParam, int nbytes)
{
	USB_DRIVER_LOGD("[+]%s \r\n", __func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	struct usbh_audio *uac_device = uvc_uac_device->uac_dev;
	struct usbh_urb *spk_urb = uvc_uac_device->uac_spk_urb;

	if(spk_urb == NULL)
		return;

	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		uvc_uac_device->uac_spk_urb->transfer_buffer_length = 0;
		os_free(uvc_uac_device->spk_ring_buffer);
		uvc_uac_device->spk_ring_buffer = NULL;
		return;
	}

	if(!(ring_buffer_read(uvc_uac_device->audio_spk_rb, uvc_uac_device->spk_ring_buffer, uvc_uac_device->uac_config->spk_ep_desc->wMaxPacketSize) > 0))
		memset(uvc_uac_device->spk_ring_buffer, 0, uvc_uac_device->uac_config->spk_ep_desc->wMaxPacketSize);

	if(ring_buffer_get_free_size(uvc_uac_device->audio_spk_rb) > (uvc_uac_device->audio_spk_rb->capacity / 2))
	{
		if(uvc_uac_device->uac_spk_tx_msg_send_flag) return;
		bk_usb_drv_send_msg(USB_DRV_AUDIO_TX, (void *)uac_device);
		uvc_uac_device->uac_spk_tx_msg_send_flag = true;
	}

	USB_DRIVER_LOGD("[-]%s \r\n", __func__);

	return;
}

static bk_err_t bk_uvc_uac_updata_audio_stream_tx_buffer(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	USB_DRIVER_LOGD("[+]%s \r\n", __func__);
	if(!(s_uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) return BK_FAIL;

	struct usbh_audio *uac_device = uvc_uac_device->uac_dev;

	audio_packet_t packet;
	if(ring_buffer_get_free_size(uvc_uac_device->audio_spk_rb) > (uvc_uac_device->audio_spk_rb->capacity / 2))
	{
		packet.data_buffer = uvc_uac_device->spk_shuttle_buffer;
		packet.data_buffer_size = (uvc_uac_device->audio_spk_rb->capacity / 2);

		uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_free(&packet);

		ring_buffer_write(uvc_uac_device->audio_spk_rb, uvc_uac_device->spk_shuttle_buffer, (uvc_uac_device->audio_spk_rb->capacity / 2));
	}

	if(!uvc_uac_device->uac_spk_urb) {
		uvc_uac_device->spk_ring_buffer = (uint8_t *)os_malloc(uvc_uac_device->uac_config->spk_ep_desc->wMaxPacketSize);

		uvc_uac_device->uac_spk_urb = usb_malloc(sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet));
		if (uvc_uac_device->uac_spk_urb == NULL) {
			USB_DRIVER_LOGE("No memory to alloc urb\r\n");
			return BK_ERR_USB_OPERATION_NULL_POINTER;
		} else
			memset(uvc_uac_device->uac_spk_urb, 0, sizeof(struct usbh_urb) + sizeof(struct usbh_iso_frame_packet));

		struct usbh_urb *spk_urb = uvc_uac_device->uac_spk_urb;
		if(!(ring_buffer_read(uvc_uac_device->audio_spk_rb, uvc_uac_device->spk_ring_buffer, uvc_uac_device->uac_config->spk_ep_desc->wMaxPacketSize) > 0))
			memset(uvc_uac_device->spk_ring_buffer, 0, uvc_uac_device->uac_config->spk_ep_desc->wMaxPacketSize);

		spk_urb->transfer_buffer = uvc_uac_device->spk_ring_buffer;//packet->data_buffer;
		spk_urb->transfer_buffer_length = uvc_uac_device->uac_config->spk_ep_desc->wMaxPacketSize;//packet->data_buffer_size;
		spk_urb->pipe = uac_device->isoout;
		spk_urb->num_of_iso_packets = 1;//packet->num_packets;
		spk_urb->timeout = 0;//0xffffffff;
		spk_urb->complete = (usbh_complete_callback_t)bk_usbh_audio_stream_txed_callback;
		spk_urb->iso_packet[0].transfer_buffer = uvc_uac_device->spk_ring_buffer;
		spk_urb->iso_packet[0].transfer_buffer_length = uvc_uac_device->uac_config->spk_ep_desc->wMaxPacketSize;

		spk_urb->arg = spk_urb;
	}

	uvc_uac_device->uac_spk_tx_msg_send_flag = false;

	USB_DRIVER_LOGD("[-]%s \r\n", __func__);
	return BK_OK;
}

bk_err_t bk_uac_start_speaker(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	struct usbh_audio *uac_device = uvc_uac_device->uac_dev;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		USB_DRIVER_LOGE("%s Mic disconnect\r\n",__func__);
		return BK_FAIL;
	}
	if(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE)) {
		USB_DRIVER_LOGE("%s is start\r\n",__func__);
		return BK_FAIL;
	}

	uvc_uac_device->usb_driver->usbh_class_start_status |= (0x1 << USB_UAC_SPEAKER_DEVICE);
	bk_usb_drv_send_msg(USB_DRV_AUDIO_SPK_START, (void *)uac_device);
	//bk_usb_drv_send_msg(USB_DRV_AUDIO_TX, (void *)uac_device);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

static void bk_uac_speaker_clear(bk_uvc_uac_fusion_device *uvc_uac_device)
{
	if(uvc_uac_device->uac_spk_urb) {
		struct usbh_urb *uac_spk_urb =uvc_uac_device->uac_spk_urb;
		uvc_uac_device->uac_spk_urb = NULL;
		os_free(uac_spk_urb);
	}

	ring_buffer_clear(uvc_uac_device->audio_spk_rb);
	if(uvc_uac_device->audio_spk_rb->address) {
		os_free(uvc_uac_device->audio_spk_rb->address);
		uvc_uac_device->audio_spk_rb->address = NULL;
	}
	if(uvc_uac_device->spk_shuttle_buffer) {
		os_free(uvc_uac_device->spk_shuttle_buffer);
		uvc_uac_device->spk_shuttle_buffer = NULL;
	}
	
	if(uvc_uac_device->spk_ring_buffer) {
		os_free(uvc_uac_device->spk_ring_buffer);
		uvc_uac_device->spk_ring_buffer = NULL;
	}
	uvc_uac_device->uac_spk_tx_msg_send_flag = false;
}

void bk_uac_start_speaker_handle(struct usbh_audio *uac_device)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	bk_uac_speaker_clear(uvc_uac_device);

	bk_uac_config_t *uac_config = uvc_uac_device->uac_config;
	audio_packet_t *packet;
	if(!(uvc_uac_device->usb_uac_buffer_ops_funcs)) return;
	packet = uvc_uac_device->usb_uac_buffer_ops_funcs->_uac_malloc(USB_UAC_SPEAKER_DEVICE);

	if(!packet) return;
	uint32_t pool_buffer_size = packet->data_buffer_size;
	USB_DRIVER_LOGI("%s pool_buffer_size:%d\r\n",__func__,pool_buffer_size);

	uint8_t *spk_ring_buffer = (uint8_t *)os_malloc(pool_buffer_size);
	uvc_uac_device->spk_shuttle_buffer = (uint8_t *)os_malloc((pool_buffer_size / 2));

	ring_buffer_init(uvc_uac_device->audio_spk_rb, spk_ring_buffer, pool_buffer_size, DMA_ID_MAX, 0);

	usbh_audio_open(uac_device, "speaker", uac_config->spk_samples_frequence);

	bk_uvc_uac_updata_audio_stream_tx_buffer(uvc_uac_device);
	int ret = 0;
	ret = usbh_submit_urb(uvc_uac_device->uac_spk_urb);
	if (ret < 0) {
		USB_DRIVER_LOGE("%s Fail to submit urb:%d\r\n", __func__, ret);
	}
}

bk_err_t bk_uac_trigger_audio_stream_tx(void *uac_device)
{
	if(!(s_uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) return BK_FAIL;
	if(!uac_device) return BK_ERR_USB_OPERATION_NULL_POINTER;
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	return bk_uvc_uac_updata_audio_stream_tx_buffer(uvc_uac_device);
}

bk_err_t bk_uac_stop_speaker(void)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;
	if(!uvc_uac_device) {
		USB_DRIVER_LOGE("%s check s_uvc_uac_device is NULL ?\r\n",__func__);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_connect_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		USB_DRIVER_LOGE("%s Spk disconnect\r\n",__func__);
		return BK_FAIL;
	}
	if(!(uvc_uac_device->usb_driver->usbh_class_start_status & (0x1 << USB_UAC_SPEAKER_DEVICE))) {
		USB_DRIVER_LOGE("%s is stop\r\n",__func__);
		return BK_FAIL;
	}
	uvc_uac_device->usb_driver->usbh_class_start_status &= ~(0x1 << USB_UAC_SPEAKER_DEVICE);
	if(uvc_uac_device->uac_spk_urb)
		uvc_uac_device->uac_spk_urb->transfer_buffer_length = 0;

	bk_usb_drv_send_msg(USB_DRV_AUDIO_SPK_STOP, (void *)uvc_uac_device->uac_dev);
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
	return BK_OK;

}

void bk_uac_stop_speaker_handle(struct usbh_audio *uac_device)
{
	bk_uvc_uac_fusion_device *uvc_uac_device = s_uvc_uac_device;

	usbh_audio_close(uac_device, "speaker");
	bk_uac_speaker_clear(uvc_uac_device);
}

bk_err_t bk_usb_uac_get_param(bk_uac_device_brief_info_t *uac_device_info)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(BK_OK != bk_usb_check_device_supported(USB_UAC_SPEAKER_DEVICE))
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;

	if(BK_OK != bk_usb_check_device_supported(USB_UAC_MIC_DEVICE))
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;

	if(!uac_device_info)
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;

	struct usb_device_descriptor_t dev_descriptor;
	struct usbh_audio *uac_device = s_uvc_uac_device->uac_dev;

	bk_usb_get_device_descriptor(&dev_descriptor);
	uac_device_info->vendor_id = dev_descriptor.idVendor;
	uac_device_info->product_id = dev_descriptor.idProduct;
	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	const char *mic_name = "mic";
	const char *spk_name = "speaker";
	uint8_t intf = 0xff;
	uint8_t mic_intf = 0xff;
	uint8_t spk_intf = 0xff;
	struct audio_ep_descriptor *audio_ep_desc;
    for (size_t i = 0; i < uac_device->module_num; i++) {
        if (strcmp(mic_name, uac_device->module[i].name) == 0) {
			intf = uac_device->module[i].data_intf;
			uac_device_info->mic_format_tag = uac_device->module[i].altsetting[0].format_type;
			uac_device_info->mic_samples_frequence_num = uac_device->module[i].altsetting[0].sampfreq_num;
			uac_device_info->mic_samples_frequence = (uint8_t *)&(uac_device->module[i].altsetting[0].sampfreq[0]);
			audio_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
			if(audio_ep_desc->bEndpointAddress & 0x80) {
				uac_device_info->mic_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				mic_intf = intf;
			} else {
				uac_device_info->spk_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				spk_intf = intf;
			}
        }
        if (strcmp(spk_name, uac_device->module[i].name) == 0) {
           intf = uac_device->module[i].data_intf;
           uac_device_info->spk_format_tag = uac_device->module[i].altsetting[0].format_type;
           uac_device_info->spk_samples_frequence_num = uac_device->module[i].altsetting[0].sampfreq_num;
           uac_device_info->spk_samples_frequence = (uint8_t *)&(uac_device->module[i].altsetting[0].sampfreq[0]);
		   audio_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
			if(audio_ep_desc->bEndpointAddress & 0x80) {
				uac_device_info->mic_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				mic_intf = intf;
			} else {
				uac_device_info->spk_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				spk_intf = intf;
			}
        }
    }
	for (size_t i = 0; i < uac_device->module_num; i++) {
        if (strcmp(mic_name, uac_device->module[i].name) == 0) {
			uac_device->module[i].data_intf = mic_intf;
		}
		if (strcmp(spk_name, uac_device->module[i].name) == 0) {
			uac_device->module[i].data_intf = spk_intf;
		}
	}
	return BK_OK;

}

bk_err_t bk_usb_uac_set_param(bk_uac_config_t *uac_config)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(BK_OK != bk_usb_check_device_supported(USB_UAC_SPEAKER_DEVICE))
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;

	if(BK_OK != bk_usb_check_device_supported(USB_UAC_MIC_DEVICE))
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;

	if(!uac_config)
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;

	bk_uvc_uac_fusion_device *device = s_uvc_uac_device;
	struct usbh_audio *uac_device = device->uac_dev;
	const char *mic_name = "mic";
	const char *spk_name = "speaker";
	uint8_t intf = 0xff;
	device->uac_config->vendor_id = uac_config->vendor_id;
	device->uac_config->product_id = uac_config->product_id;
	device->uac_config->mic_format_tag = uac_config->mic_format_tag;
	device->uac_config->mic_samples_frequence = uac_config->mic_samples_frequence;

	for (size_t i = 0; i < uac_device->module_num; i++) {
		if (strcmp(mic_name, uac_device->module[i].name) == 0) 
		{
			intf = uac_device->module[i].data_intf;
			if(uac_config->mic_ep_desc) {
				memcpy(&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc, uac_config->mic_ep_desc, 7);
				device->uac_config->mic_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
			}
		}

		if (strcmp(spk_name, uac_device->module[i].name) == 0) 
		{
			intf = uac_device->module[i].data_intf;
			if(uac_config->spk_ep_desc) {
				memcpy(&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc, uac_config->spk_ep_desc, 7);
				device->uac_config->spk_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
			}
		}
		
	}
	device->uac_config->spk_format_tag = uac_config->spk_format_tag;
	device->uac_config->spk_samples_frequence = uac_config->spk_samples_frequence;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);


	return BK_OK;

}

bk_err_t bk_usb_uac_set_mic_param(bk_uac_mic_config_t *uac_mic_config)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(BK_OK != bk_usb_check_device_supported(USB_UAC_MIC_DEVICE))
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;

	if(!uac_mic_config)
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;

	bk_uvc_uac_fusion_device *device = s_uvc_uac_device;
	struct usbh_audio *uac_device = device->uac_dev;
	const char *mic_name = "mic";
	uint8_t intf = 0xff;
	struct audio_ep_descriptor audio_mic_ep_desc;
	device->uac_config->mic_format_tag = uac_mic_config->mic_format_tag;
	device->uac_config->mic_samples_frequence = uac_mic_config->mic_samples_frequence;
	memcpy(&audio_mic_ep_desc, uac_mic_config->mic_ep_desc, 7);
	for (size_t i = 0; i < uac_device->module_num; i++) {
		if (strcmp(mic_name, uac_device->module[i].name) == 0) {
			intf = uac_device->module[i].data_intf;
			memcpy(&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc, &audio_mic_ep_desc, 7);
			device->uac_config->mic_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uac_set_spk_param(bk_uac_spk_config_t *uac_spk_config)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	if(!bk_usb_get_device_connect_status())
		return BK_ERR_USB_NOT_CONNECT;

	if(BK_OK != bk_usb_check_device_supported(USB_UAC_SPEAKER_DEVICE))
		return BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;

	if(!uac_spk_config)
		return BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;

	bk_uvc_uac_fusion_device *device = s_uvc_uac_device;
	struct usbh_audio *uac_device = device->uac_dev;
	const char *spk_name = "speaker";
	uint8_t intf = 0xff;
	struct audio_ep_descriptor audio_spk_ep_desc;
	device->uac_config->spk_format_tag = uac_spk_config->spk_format_tag;
	device->uac_config->spk_samples_frequence = uac_spk_config->spk_samples_frequence;
	memcpy(&audio_spk_ep_desc, uac_spk_config->spk_ep_desc, 7);
	for (size_t i = 0; i < uac_device->module_num; i++) {
		if (strcmp(spk_name, uac_device->module[i].name) == 0) {
			intf = uac_device->module[i].data_intf;
			memcpy(&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc, &audio_spk_ep_desc, 7);
			device->uac_config->spk_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;

}

bk_err_t bk_usb_uac_register_transfer_buffer_ops(void *ops)
{
	s_uvc_uac_device->usb_uac_buffer_ops_funcs = (struct audio_packet_control_t *)ops;

#if 0
	struct bk_uac_ring_buffer_t *uac_rb_t = (struct bk_uac_ring_buffer_t *)ops;
	uac_rb_t->mic_ring_buffer_p = (void *)s_uvc_uac_device->audio_mic_rb;
	uac_rb_t->spk_ring_buffer_p = (void *)s_uvc_uac_device->audio_spk_rb;
#endif
	return BK_OK;
}

bk_err_t bk_usb_uvc_check_support_attribute(uint32_t attribute)
{
	return BK_OK;
}

bk_err_t bk_usb_uvc_attribute_op(E_USB_ATTRIBUTE_OP ops, uint32_t attribute, uint32_t *param)
{

	return BK_OK;
}

bk_err_t bk_usb_uac_check_support_attribute(uint32_t attribute)
{
	return BK_OK;

}

bk_err_t bk_usb_uac_attribute_op(E_USB_ATTRIBUTE_OP ops, uint32_t attribute, uint32_t *param)
{
	return BK_OK;
}
#endif

