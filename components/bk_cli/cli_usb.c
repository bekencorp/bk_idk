#include <common/bk_include.h>
#include "sdkconfig.h"
#include "stdarg.h"
#include <os/mem.h>
#include "sys_rtos.h"
#include <os/os.h>
#include <common/bk_kernel_err.h>
#include "bk_sys_ctrl.h"
#include "bk_cli.h"
#include "bk_uart.h"
#include <os/str.h>
#include <components/log.h>
#include <driver/media_types.h>
#include "drv_model_pub.h"
#include "cli.h"
#include <components/usb.h>
#include <components/usb_types.h>
#include <driver/audio_ring_buff.h>

#if CONFIG_USB
#if CONFIG_USB_MSD
#include "ff.h"
#include "diskio.h"
#endif

static void cli_usb_help(void)
{
	CLI_LOGD("cli_usb_help!\r\n");
}

#if CONFIG_USB_MSD

static FATFS *pfs = NULL;
uint8_t mount_flag = 0;
extern void usbh_msc_register();

static void udisk_test_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "register") == 0)
	{
		os_printf("usbh_msc_register \r\n");
		usbh_msc_register();
	}

	if (os_strcmp(argv[1], "read") == 0)
	{
		FIL file;
		const TCHAR *file_name = argv[2];
		UINT read_len = os_strtoul(argv[3], NULL, 10);
		UINT rd_pos = os_strtoul(argv[4], NULL, 10);

		uint8_t print_buf[64];
		UINT print_size = 0;

		BK_LOG_ON_ERR(f_open(&file, file_name, FA_OPEN_APPEND | FA_READ));
		BK_LOG_ON_ERR(f_lseek(&file, rd_pos));
		BK_LOG_ON_ERR(f_read(&file, print_buf, read_len, &print_size));
		BK_LOG_ON_ERR(f_close(&file));
		print_hex_dump(NULL, print_buf, read_len);
	}
	if (os_strcmp(argv[1], "write") == 0)
	{
		FIL file;
		const TCHAR *file_name = argv[2];
		UINT write_len = os_strtoul(argv[3], NULL, 10);
		UINT st_value = os_strtoul(argv[4], NULL, 10);
		UINT wr_pos = os_strtoul(argv[5], NULL, 10);

		uint8_t write_buf[64];
		// init a array to write
		for (int i = 0; i < 64; i++)
		{
			write_buf[i] = st_value & 0xFF;
		}
		UINT wr_size = 0;

		BK_LOG_ON_ERR(f_open(&file, file_name, FA_OPEN_APPEND | FA_WRITE));
		BK_LOG_ON_ERR(f_lseek(&file, wr_pos));
		BK_LOG_ON_ERR(f_write(&file, write_buf, write_len, &wr_size));
		BK_LOG_ON_ERR(f_close(&file));
	}
	if (os_strcmp(argv[1], "mkdir") == 0)
	{
		const TCHAR *file_name = argv[2];
		BK_LOG_ON_ERR(f_mkdir(file_name));
	}
}


void usb_mount_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	char sys_path[10];
	int number = DISK_NUMBER_UDISK;

	if (pfs)
		os_free(pfs);

	pfs = os_malloc(sizeof(FATFS));
	if (NULL == pfs) {
		CLI_LOGD("f_mount malloc failed!\r\n");
		return;
	}

	os_memset(sys_path, 0, sizeof(sys_path));
	sprintf(sys_path, "%d:", number);
	fr = f_mount(pfs, sys_path, 1);
	if (fr != FR_OK) {
		mount_flag = 0;
		CLI_LOGI("usb mount failed:%d\r\n", fr);
	} else {
		mount_flag = 1;
		CLI_LOGI("usb mount OK!\r\n");
	}
}



void usb_unmount_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	char sys_path[10];
	int number = DISK_NUMBER_UDISK;

	if (mount_flag != 1) {
		CLI_LOGD("usb hasn't initialization!\r\n");
		return;
	}

	os_memset(sys_path, 0, sizeof(sys_path));
	sprintf(sys_path, "%d:", number);
	fr = f_mount(NULL, sys_path, 1);
	if (fr != FR_OK) {
		CLI_LOGD("unmount %s fail.\r\n", sys_path);
		return;
	}

	if (pfs) {
		os_free(pfs);
		pfs = NULL;
	}

	mount_flag = 0;
	CLI_LOGD("usb unmount OK!\r\n");
}

static FRESULT usb_scan_files(char *path)
{
	FRESULT fr;
	DIR dir;
	FILINFO fno;
	char path_temp[255];

	fr = f_opendir(&dir, path);
	if (fr == FR_OK) {
		while (1) {
			fr = f_readdir(&dir, &fno);
			if (fr != FR_OK)
				break;
			if (fno.fname[0] == 0)
				break;
			if (fno.fattrib & AM_DIR) {
				/* It is a directory */
				os_memset(path_temp, 0, sizeof(path_temp));
				sprintf(path_temp, "%s/%s", path, fno.fname);
				fr = usb_scan_files(path_temp);
				if (fr != FR_OK) break;
			} else {
				/* It is a file. */
				CLI_LOGI("%s/%s\r\n", path, fno.fname);
			}
		}
		f_closedir(&dir);
	} else
		CLI_LOGD("f_opendir failed\r\n");

	return fr;
}

void usb_ls_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	char sys_path[10];
	int number = DISK_NUMBER_UDISK;

	if (mount_flag != 1) {
		CLI_LOGD("usb hasn't initialization!\r\n");
		return;
	}

	os_memset(sys_path, 0, sizeof(sys_path));
	sprintf(sys_path, "%d:", number);
	fr = usb_scan_files(sys_path);
	if (fr != FR_OK)
		CLI_LOGD("scan_files failed!\r\n");
}
#endif

#if CONFIG_USB_UVC_DEBUG
void uvc_view_support_attribute(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	E_UVC_ATTRIBUTE_T attribute;
	for(attribute = UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION; attribute < UVC_ATTRIBUTE_NUM; attribute++)
	{
        switch(attribute)
        {
            case UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT BACKLIGHT_COMPENSATION: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT BACKLIGHT_COMPENSATION: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_BRIGHTNESS:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT BRIGHTNESS: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT BRIGHTNESS: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_CONTRAST:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT CONTRAST: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT CONTRAST: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_GAIN:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT GAIN: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT GAIN: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_POWER_LINE_FREQUENCY:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT POWER_LINE_FREQUENCY: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT POWER_LINE_FREQUENCY: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_HUE:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT HUE: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT HUE: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_SATURATION:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT SATURATION: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT SATURATION: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_SHARPNESS:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT SHARPNESS: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT SHARPNESS: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_GAMMA:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT GAMMA: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT GAMMA: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_TEMPERATURE: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_TEMPERATURE: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE_AUTO:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_TEMPERATURE_AUTO: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_TEMPERATURE_AUTO: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_COMPONENT: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_COMPONENT: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT_AUTO:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_COMPONENT_AUTO: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT WHITE_BALANCE_COMPONENT_AUTO: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_DIGITAL_MULTIPLIER:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT DIGITAL_MULTIPLIER: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT DIGITAL_MULTIPLIER: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_DIGITAL_MULTIPLIER_LIMIT:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT DIGITAL_MULTIPLIER_LIMIT: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT DIGITAL_MULTIPLIER_LIMIT: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_HUE_AUTO:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT HUE_AUTO: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT HUE_AUTO: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_ANALOG_LOCK_STATUS:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT ANALOG_LOCK_STATUS: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT ANALOG_LOCK_STATUS: 1  YES\r\n");
                break;
            case UVC_ATTRIBUTE_ANALOG_VIDEO_STANDARD:
                if(bk_usb_uvc_check_support_attribute(attribute))
                    CLI_LOGD("UVC SUPPORT ANALOG_VIDEO_STANDARD: 0  NO\r\n");
                else
                    CLI_LOGD("UVC SUPPORT ANALOG_VIDEO_STANDARD: 1  YES\r\n");
                break;
            default:
                break;
        }
	}
}

void uvc_get_param(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		cli_usb_help();
		return;
	}
	uint32_t attribute = 0x00;
	uint32_t param = 0;

	if (os_strcmp(argv[1], "backlight") == 0) {
		attribute = UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION;
	} else if (os_strcmp(argv[1], "brightness") == 0) {
		attribute = UVC_ATTRIBUTE_BRIGHTNESS;
	} else if (os_strcmp(argv[1], "contrast") == 0) {
		attribute = UVC_ATTRIBUTE_CONTRAST;
	} else if (os_strcmp(argv[1], "gain") == 0) {
		attribute = UVC_ATTRIBUTE_GAIN;
	} else if (os_strcmp(argv[1], "powerlinefre") == 0) {
		attribute = UVC_ATTRIBUTE_POWER_LINE_FREQUENCY;
	} else if (os_strcmp(argv[1], "hue") == 0) {
		attribute = UVC_ATTRIBUTE_HUE;
	} else if (os_strcmp(argv[1], "hueauto") == 0) {
		attribute = UVC_ATTRIBUTE_HUE_AUTO;
	} else if (os_strcmp(argv[1], "saturation") == 0) {
		attribute = UVC_ATTRIBUTE_SATURATION;
	} else if (os_strcmp(argv[1], "sharpness") == 0) {
		attribute = UVC_ATTRIBUTE_SHARPNESS;
	} else if (os_strcmp(argv[1], "gamma") == 0) {
		attribute = UVC_ATTRIBUTE_GAMMA;
	} else if (os_strcmp(argv[1], "wbt") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE;
	} else if (os_strcmp(argv[1], "wbtauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE_AUTO;
	} else if (os_strcmp(argv[1], "wbc") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT;
	} else if (os_strcmp(argv[1], "wbcauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT_AUTO;
	} else if (os_strcmp(argv[1], "digmu") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER;
	} else if (os_strcmp(argv[1], "digmulimit") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER_LIMIT;
	} else if (os_strcmp(argv[1], "analogvideo") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_VIDEO_STANDARD;
	} else if (os_strcmp(argv[1], "analoglock") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_LOCK_STATUS;
	} else {
		cli_usb_help();
		return;
	}

	if (os_strcmp(argv[2], "cur") == 0) {
		CLI_LOGD("uvc cur 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_CUR, attribute, &param));
	} else if (os_strcmp(argv[2], "min") == 0) {
		CLI_LOGD("uvc min 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_MIN, attribute, &param));
	} else if (os_strcmp(argv[2], "max") == 0) {
		CLI_LOGD("uvc max 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_MAX, attribute, &param));
	} else if (os_strcmp(argv[2], "info") == 0) {
		CLI_LOGD("uvc info 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_INFO, attribute, &param));
	} else if (os_strcmp(argv[2], "len") == 0) {
		CLI_LOGD("uvc len 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_LEN, attribute, &param));
	} else if (os_strcmp(argv[2], "res") == 0) {
		CLI_LOGD("uvc res 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_RES, attribute, &param));
	} else if (os_strcmp(argv[2], "def") == 0) {
		CLI_LOGD("uvc def 0x%x!\r\n", bk_usb_uvc_attribute_op(USB_ATTRIBUTE_GET_DEF, attribute, &param));
	} else {
		cli_usb_help();
		return;
	}
	CLI_LOGD("uvc param: 0x%x!\r\n", param);

}

void uvc_set_param(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		cli_usb_help();
		CLI_LOGD("cli_usb_help!\r\n");
		return;
	}
	uint32_t attribute = 0x00;
	uint32_t param = 0;

	if (os_strcmp(argv[1], "backlight") == 0) {
		attribute = UVC_ATTRIBUTE_BACKLIGHT_COMPENSATION;
	} else if (os_strcmp(argv[1], "brightness") == 0) {
		attribute = UVC_ATTRIBUTE_BRIGHTNESS;
	} else if (os_strcmp(argv[1], "contrast") == 0) {
		attribute = UVC_ATTRIBUTE_CONTRAST;
	} else if (os_strcmp(argv[1], "gain") == 0) {
		attribute = UVC_ATTRIBUTE_GAIN;
	} else if (os_strcmp(argv[1], "powerlinefre") == 0) {
		attribute = UVC_ATTRIBUTE_POWER_LINE_FREQUENCY;
	} else if (os_strcmp(argv[1], "hue") == 0) {
		attribute = UVC_ATTRIBUTE_HUE;
	} else if (os_strcmp(argv[1], "hueauto") == 0) {
		attribute = UVC_ATTRIBUTE_HUE_AUTO;
	} else if (os_strcmp(argv[1], "saturation") == 0) {
		attribute = UVC_ATTRIBUTE_SATURATION;
	} else if (os_strcmp(argv[1], "sharpness") == 0) {
		attribute = UVC_ATTRIBUTE_SHARPNESS;
	} else if (os_strcmp(argv[1], "gamma") == 0) {
		attribute = UVC_ATTRIBUTE_GAMMA;
	} else if (os_strcmp(argv[1], "wbt") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE;
	} else if (os_strcmp(argv[1], "wbtauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_TEMPERATURE_AUTO;
	} else if (os_strcmp(argv[1], "wbc") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT;
	} else if (os_strcmp(argv[1], "wbcauto") == 0) {
		attribute = UVC_ATTRIBUTE_WHITE_BALANCE_COMPONENT_AUTO;
	} else if (os_strcmp(argv[1], "digmu") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER;
	} else if (os_strcmp(argv[1], "digmulimit") == 0) {
		attribute = UVC_ATTRIBUTE_DIGITAL_MULTIPLIER_LIMIT;
	} else if (os_strcmp(argv[1], "analogvideo") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_VIDEO_STANDARD;
	} else if (os_strcmp(argv[1], "analoglock") == 0) {
		attribute = UVC_ATTRIBUTE_ANALOG_LOCK_STATUS;
	} else {
		cli_usb_help();
		return;
	}

	param = os_strtoul(argv[2], NULL, 10);
	bk_usb_uvc_attribute_op(USB_ATTRIBUTE_SET_CUR, attribute, &param);

}

static void uvc_disconnect_uvc_configed(void)
{
	CLI_LOGD("fuvc_notify_uvc_disconnect\r\n");
	return;
}

#endif

#if CONFIG_USB_PLUG_IN_OUT
void cli_usb_plug_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		usb_plug_inout_init();
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		usb_plug_inout_deinit();
	} else {
		cli_usb_help();
		return;
	}

}

void cli_usb_plug_inout(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	if (os_strcmp(argv[1], "open") == 0) {
		usb_plug_inout_open();
		CLI_LOGD("cli_usb_plug_inout inout open!\r\n");
	} else if (os_strcmp(argv[1], "close") == 0) {
		usb_plug_inout_close();
		CLI_LOGD("cli_usb_plug_inout inout close!\r\n");
	} else {
		cli_usb_help();
		return;
	}

}
#endif

#if CONFIG_UVC_OTA_DEMO
#include <components/uvc_ota_common.h>
static uint32_t cli_usb_ctrl_sync_callback(void *pContext, void *pControlIrp)
{
	if(!pContext)
		return BK_FAIL;
	s_usb_device_request *pSetup = (s_usb_device_request *)pContext;
	CLI_LOGD("Control_Transfer Done. pSetup->bmRequestType:0x%x\r\n",pSetup->bmRequestType);
	CLI_LOGD("Control_Transfer Done. pSetup->bRequest:0x%x\r\n",pSetup->bRequest);
	CLI_LOGD("Control_Transfer Done. pSetup->wValue:0x%x\r\n",pSetup->wValue);
	CLI_LOGD("Control_Transfer Done. pSetup->wIndex:0x%x\r\n",pSetup->wIndex);
	CLI_LOGD("Control_Transfer Done. pSetup->wLength:0x%x\r\n",pSetup->wLength);
	CLI_LOGD("cli_usb_ctrl_sync_callback Control_Transfer Done.\r\n");
	return BK_OK;
}

void cli_usb_ota_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		CLI_LOGD("cli_usb_ota_ops Please Check the number of paramter.\r\n");
		cli_usb_help();
		return;
	}
	s_usb_device_request *pSetup = {0};
	s_usb_transfer_buffer_info *buffer_info  = {0};
	uint8_t *g_ota_outbuffer = {0};
	uint8_t *g_ota_inbuffer = {0};
	void *parameter = {0};
	char verssion[7] = {'V', '1', '.', '0', '.', '2', 0};
	char firewaredata[6] = {0};
	uint32_t file_len = 0;
	bool usb_transfer_sta = 0;
	bool malloc_status_flag = 0;
	bk_err_t ret;

	if (os_strcmp(argv[1], "cmd_self_test") == 0) {
		if (argc < 8) {
			CLI_LOGD("cli_usb_ota_ops Please Check the number of paramter.\r\n");
			return;
		}

		do{
			pSetup = (s_usb_device_request *)os_malloc(sizeof(s_usb_device_request));
			if(!pSetup) {
				CLI_LOGD("%s pSetup malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}

			buffer_info = (s_usb_transfer_buffer_info *)os_malloc(sizeof(s_usb_transfer_buffer_info));
			if(!buffer_info) {
				CLI_LOGD("%s buffer_info malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}

			g_ota_outbuffer = (uint8_t *)os_malloc(sizeof(uint8_t) * (pSetup->wLength));
			if(!g_ota_outbuffer) {
				CLI_LOGD("%s g_ota_outbuffer malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}

			g_ota_inbuffer = (uint8_t *)os_malloc(sizeof(uint8_t) * (pSetup->wLength));
			if(!g_ota_inbuffer) {
				CLI_LOGD("%s g_ota_inbuffer malloc fail.\r\n", __func__);
				malloc_status_flag = 1;
				break;
			}
		}while(0);

		if(malloc_status_flag) {
			if(g_ota_outbuffer != NULL) {
				os_free(g_ota_outbuffer);
				g_ota_outbuffer = NULL;
			}
			if(g_ota_inbuffer != NULL) {
				os_free(g_ota_inbuffer);
				g_ota_inbuffer = NULL;
			}
			if(pSetup != NULL) {
				os_free(pSetup);
				pSetup = NULL;
			}
			if(buffer_info != NULL) {
				os_free(buffer_info);
				buffer_info = NULL;
			}

			return;
		}

		pSetup->bmRequestType = os_strtoul(argv[2], NULL, 16);
		pSetup->bRequest = os_strtoul(argv[3], NULL, 16);
		pSetup->wValue = os_strtoul(argv[4], NULL, 16);
		pSetup->wIndex = os_strtoul(argv[5], NULL, 16);
		pSetup->wLength = os_strtoul(argv[6], NULL, 16);

		for(int i = 0; i < (pSetup->wLength); i++)
		{
			g_ota_outbuffer[i] = i;
			CLI_LOGD("%s Outbuffer[%d] : %x\r\n", __FUNCTION__, i, g_ota_outbuffer[i]);
		}
		os_memset((void *)g_ota_inbuffer, 0x0, (sizeof(uint8_t) * (pSetup->wLength)));

		buffer_info->pOutBuffer = g_ota_outbuffer;
		buffer_info->dwOutLength = pSetup->wLength;
		buffer_info->pInBuffer = g_ota_inbuffer;
		buffer_info->dwInLength = pSetup->wLength;
		parameter = (void *)cli_usb_ctrl_sync_callback;
		buffer_info->pfIrpComplete = parameter;
		buffer_info->pCompleteParam = pSetup;

		ret = bk_usb_control_transfer(pSetup, buffer_info);
		if(ret) {
			CLI_LOGD("bk_usb_control_transfer fail. ret:%d\r\n", ret);
			os_free(g_ota_outbuffer);
			os_free(g_ota_inbuffer);
			os_free(pSetup);
			os_free(buffer_info);
			return;
		}

		for(int j = 0; j < (buffer_info->dwInLength); j++)
			CLI_LOGD("%s Inbuffer[%d] : %x\r\n", __FUNCTION__, j, g_ota_inbuffer[j]);

		os_free(g_ota_outbuffer);
		os_free(g_ota_inbuffer);
		os_free(pSetup);
		os_free(buffer_info);
		CLI_LOGD("cli_usb_ota_ops cmd_self_test.\r\n");
	} else if (os_strcmp(argv[1], "uvc_ota_init") == 0){
		ret = bk_uvc_ota_demo_init();
		if(ret) {
			CLI_LOGD("bk_uvc_ota_demo_init fail. ret:%d\r\n", ret);
			return;
		}
	} else if (os_strcmp(argv[1], "uvc_ota_deinit") == 0){
		ret = bk_uvc_ota_demo_deinit();
		if(ret) {
			CLI_LOGD("bk_uvc_ota_demo_deinit fail. ret:%d\r\n", ret);
			return;
		}
	} else if (os_strcmp(argv[1], "module_version_check") == 0) {
		firewaredata[0] = 'B';
		/* Fill in reasonable numbers based on the actual situation
		 * and some USB modules cannot be upgraded
		 */
		if (argc > 2) {
			file_len = os_strtoul(argv[2], NULL, 10);
		} else
			file_len = 5546649;/* Self test data length */
		ret = bk_uvc_ota_demo_upgrade_check(verssion, firewaredata, file_len);
		if(ret) {
			CLI_LOGD("bk_uvc_ota_demo_upgrade_check fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGD("uvc_ota_demo_upgrade_check cli_test\r\n");

	} else if (os_strcmp(argv[1], "uvc_ota_done_result") == 0){
		uint32_t result = 0;
		ret = bk_uvc_ota_demo_transmit_result(&result);
		if(ret) {
			CLI_LOGD("bk_uvc_ota_demo_transmit_result fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGD("uvc_ota_done_result result:%d\r\n", result);
	} else if (os_strcmp(argv[1], "uvc_ota_persent") == 0){
		int persent = 0;
		ret = bk_uvc_ota_demo_get_upgrade_persent(&persent);
		if(ret) {
			CLI_LOGD("bk_uvc_ota_demo_get_upgrade_persent fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGD("uvc_ota_persent persent:%d\r\n", persent);
	} else if (os_strcmp(argv[1], "uvc_ota_http_percentage") == 0){
		float percentage = 0;
		ret = bk_uvc_ota_http_to_device_status(&percentage, &usb_transfer_sta);
		if(ret) {
			CLI_LOGD("bk_uvc_ota_http_to_device_status fail. ret:%d\r\n", ret);
			return;
		}
		CLI_LOGD("uvc_ota_http_percentage percentage::(%.2f)%%  usb_transfer_sta:%d\r\n", percentage, usb_transfer_sta);
	} else if (os_strcmp(argv[1], "uvc_ota_http_download") == 0){
		if (argc != 4) {
			CLI_LOGD("Usage:http_ota [url:]\r\n");
			return;
		}
		uint32_t uri_length = os_strlen(argv[2]) + 1;
		uint8_t wait_ms = os_strtoul(argv[3], NULL, 10);

		ret = bk_uvc_ota_http_download(argv[2], uri_length, wait_ms);
		if(ret) {
			CLI_LOGD("bk_uvc_ota_http_download fail. ret:%d\r\n", ret);
			return;
		}
	} else{
		CLI_LOGD("cli_usb_ota_ops Please Check the number of paramter.\r\n");
	}

}
#endif

#if CONFIG_USB_CDC_ACM_DEMO

void cli_usb_cdc_acm_demo_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		CLI_LOGD("%s Please Check the number of paramter.\r\n", __func__);
		cli_usb_help();
		return;
	}
	/* Firstly */
	if (os_strcmp(argv[1], "demo") == 0) {
          #if (CONFIG_SYS_CPU0)
		extern void bk_usb_cdc_demo(void);
		bk_usb_cdc_demo();
          #endif
	}
	/* Secondly */
	else if (os_strcmp(argv[1], "open") == 0) {
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
		bk_usb_open(USB_HOST_MODE);
	}
}

#endif

void cli_usb_base_ops(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_usb_help();
		return;
	}

	uint32_t gpio_id = 0xFF;
	bool poweron = 0;
	uint32_t ret = 0;
	bool use_or_no = 1;
	uint32_t dev = 0;
	if (os_strcmp(argv[1], "driver_init") == 0) {
		ret = bk_usb_driver_init();
		CLI_LOGI("%s driver_init ret:%d\r\n",__func__, ret);
	} else if (os_strcmp(argv[1], "driver_deinit") == 0) {
		ret = bk_usb_driver_deinit();
		CLI_LOGI("%s driver_deinit ret:%d\r\n",__func__, ret);
	} else if (os_strcmp(argv[1], "power") == 0) {
		gpio_id = os_strtoul(argv[2], NULL, 16);
		poweron = os_strtoul(argv[3], NULL, 16);
		ret = bk_usb_power_ops(gpio_id, poweron);
		CLI_LOGI("%s power_ops gpio_id:%d power_ops:%d ret:%d\r\n",__func__,gpio_id, poweron, ret);
	} else if (os_strcmp(argv[1], "open_host") == 0) {
		CLI_LOGI("cli_usb_open host! %d\r\n", bk_usb_open(0));
	} else if (os_strcmp(argv[1], "open_dev") == 0) {
		CLI_LOGD("cli_usb_open device! %d\r\n", bk_usb_open(1));
	} else if (os_strcmp(argv[1], "close") == 0) {
		CLI_LOGI("cli_usb_open host! %d\r\n", bk_usb_close());
	} else if(os_strcmp(argv[1], "set_using") == 0) {
		use_or_no = os_strtoul(argv[2], NULL, 16);
		dev = os_strtoul(argv[3], NULL, 16);
		bk_usb_device_set_using_status(use_or_no, dev);
	} else if(os_strcmp(argv[1], "get_connect") == 0) {
		ret = bk_usb_get_device_connect_status();
		CLI_LOGI("connect %d\r\n", ret);
	} else if(os_strcmp(argv[1], "check_dev") == 0) {
		dev = os_strtoul(argv[2], NULL, 16);
		ret = bk_usb_check_device_supported (dev);
		CLI_LOGI("check device %d\r\n", ret);
	} else {
		cli_usb_help();
		return;
	}

}

const struct cli_command usb_host_clis[] = {

#if CONFIG_USB_MSD
	{"usb_mount", "usb mount", usb_mount_command},
	{"usb_unmount", "usb unmount", usb_unmount_command},
	{"usb_ls", "usb list system", usb_ls_command},
	{"usb_op", "usb_read file length", udisk_test_command},
#endif //CONFIG_USB_MSD

#if CONFIG_USB_PLUG_IN_OUT
	{"usb_plug", "usb plug init|out", cli_usb_plug_init},
	{"usb_plug_inout", "usb open|close", cli_usb_plug_inout},
#endif //CONFIG_USB_PLUG_IN_OUT

#if CONFIG_UVC_OTA_DEMO
	{"usb_ota", "usb_ota cli_usb_ota_ops|uvc_ota_init|uvc_ota_deinit|uvc_ota_http_download", cli_usb_ota_ops},
#endif //CONFIG_UVC_OTA_DEMO

#if CONFIG_USB_CDC_ACM_DEMO
	{"usb_cdc", "usb_cdc_acm_demo ", cli_usb_cdc_acm_demo_ops},
#endif

	{"usb", "usb driver_init|driver_deinit|power[gpio_id ops]|open_host|open_dev|close", cli_usb_base_ops},
};

int cli_usb_init(void)
{
	int ret;
	ret = cli_register_commands(usb_host_clis, sizeof(usb_host_clis) / sizeof(struct cli_command));
	if (ret)
		CLI_LOGD("register usb host commands fail.\r\n");

	return ret;
}
#endif
