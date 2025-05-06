/**
 ****************************************************************************************
 *
 * @file bk_csi_ble_provisioning.h
 *
 * @brief BK WIFI csi module
 *
 *
 * Copyright (C) BEKEN corperation 2021-2024
 *
 ****************************************************************************************
 */

#ifndef _BK_WIFI_CSI_BLE_PROVISONING_H_
#define _BK_WIFI_CSI_BLE_PROVISONING_H_

#pragma once

#include <common/sys_config.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os.h>

//#include "bk_csi_internal.h"

#ifdef __cplusplus
extern "C" {
#endif
#define BLE_MAX_SEND_LIST 10 /// 2500bytes


#define CSI_BOARDING_OP_START_CSI_CONFIG	(17)
#define CSI_BOARDING_OP_CSI_DATA_IND		(18)


enum bk_csi_ble_provisioning_cmd
{
	/// BK csi BLE send pkt
	BK_CSI_BLE_SEND_PKT,
	BK_CSI_BLE_RCV_PKT,
	BK_CSI_BLE_DISCONNECT_IND,
};

extern bk_err_t bk_wifi_csi_ble_provisioning_init(void);
extern void bk_wifi_csi_ble_provisioning_dinit(void);
extern void bk_wifi_csi_ble_data_send(uint8_t *payload, uint16_t data_len);

#ifdef __cplusplus
}
#endif

#endif


