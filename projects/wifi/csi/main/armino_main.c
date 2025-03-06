#include <common/sys_config.h>
#include <components/log.h>
#include <modules/wifi.h>
#include <components/event.h>
#include <components/netif.h>
#include <string.h>

#include "bk_private/bk_init.h"
#include "bk_private/bk_wifi.h"
#include "modules/wifi_types.h"
//#include "bk_csi_demo.h"

#if (CONFIG_SYS_CPU0)
int csi_wifi_event_alg_cb(void *arg, event_module_t event_module,
					int event_id, void *event_data)
{
	struct wifi_csi_alg_ind *csi_data_info = (struct wifi_csi_alg_ind *)event_data;
	// TODO data handle
	BK_LOG_RAW("--------------out test v1=%d v2=%d\r\n",csi_data_info->v1,csi_data_info->v2);
	
	return BK_OK;
}

static void csi_event_handler_init(void)
{
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_CSI_ALG_IND,
								   csi_wifi_event_alg_cb, NULL));

}
/**
 ****************************************************************************************
 * @brief wifi_csi_rx_cb_demo
 * move csi info pointer to next 
 *
 * @param[in] info     Pointer to the csi info buffer
 * @param[in] flag     flag for getting buffer or write buffer finish
 ****************************************************************************************
 */
void wifi_csi_rx_cb_demo(struct wifi_csi_info_t *info)
{
	if(info != NULL)
	{
		if(info->data_type == 0)// iqdata
		{
			BK_LOG_RAW("SPCSIINFO %d \r\n",info->len);
			for(int i=0;i<info->len;i++)
			{
				BK_LOG_RAW("%x \r\n",info->data.buf[i]);
			}
			BK_LOG_RAW("EPCSIINFO \r\n");
		}
		else if(info->data_type == 1)//abs
		{
			BK_LOG_RAW("SPCSIINFO ABS:%d,[",info->len);
			for(int i=0;i<info->len;i++)
			{
				BK_LOG_RAW("%f,",info->data.abs[i]);
			}
			BK_LOG_RAW("]\r\n");
		}
	}
}

void wifi_csi_init(void)
{
	// bit 0 : send csi data to host
	// bit 1 : print csi data to uart
	// bit 2 : return csi data handle result
	uint8_t csi_work_mode = 0x03;

	if(csi_work_mode&0x01)
	{
		csi_event_handler_init();
	}

	bk_wifi_csi_info_cb_register(wifi_csi_rx_cb_demo);

	// example:
	//   |<--gap-->|<--gap-->|<--gap-->|<--gap-->|<--gap-->|
	//   |<------------------------------interval------------------------------>|
	//   interval time = 200ms
	//   gap = 20ms
	//   gap num = 5
	// bk_wifi_csi_start_req(csi_work_mode,5,200,20);

	//bk_wifi_csi_start_req(csi_work_mode,0,0,0);
	//bk_wifi_csi_start_req(csi_work_mode,0,15,0,1000);

}
#endif
int main(void)
{
	bk_init();

	#if (CONFIG_SYS_CPU0)
	// PS CLOSE
	bk_wifi_ps_config(WIFI_PS_CMD_CLOSE,0,0);
	// capa tx_ampdu 0
	bk_wifi_capa_config(WIFI_CAPA_ID_TX_AMPDU_EN, 0);
	// capa rx_ampdu 0
	bk_wifi_capa_config(WIFI_CAPA_ID_RX_AMPDU_EN, 0);
	// csi init
	wifi_csi_init();
	#endif

	return 0;
}

