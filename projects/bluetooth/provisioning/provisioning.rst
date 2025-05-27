WiFi provisioning over BLE
==============================

:link_to_translation:`zh_CN:[中文]`

1. Overview
--------------------------
	This project is used to introduce WiFi provision over BLE.This function mainly defines the use of the BLE GATT Service to realize the SSID and Password required for the WiFi connection during the network configuration process, so that the unconfigured device can obtain the information of the connected network, and then can connect to WIFI.

1.1 Specification
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

    * ble:
        * gap
        * gatt server

1.2 Code Path and build cmd
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	demo：``./projects/bluetooth/provisioning``

	build cmd：``make bk72xx PROJECT=bluetooth/provisioning``

2. Provisioning Process
---------------------------------

2.1 Introduction
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

	The main design ideas of provisioning demo are as follows:

	| 1) Bluetooth device sends out ble provisioning advertising.
	| 2) The mobile phone APP searches for ble provisioning  advertising to find device and creates connection.
	| 3) The moblie phone APP sends the WiFi SSID and PASSWORD to the device to be connected through the GATT Service.
	| 4) After the Bluetooth device obtains the SSID and PASSWORD required for the WiFi connection, the WiFi connection is made, and the provisioning is successful.

2.2 Flowchart
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

	provisioning process is shown in the figure below：

.. figure:: ../../../../_static/wifi_provisioning_over_ble_workflow.png
    :align: center
    :alt: wifi provisioning over ble workflow
    :figclass: align-center

    Figure 1. wifi provisioning over ble workflow

2.3 Instructions
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

	| 1) Open the nRF Connect APP on the mobile phone to scan, find the device named BKPROV_XXXXXX and click the connect button.
	| 2) After the device is connected, find the service whose service UUID is 0xFA00 and click it.
	| 3) Find the characteristic whose UUID is 0xEA05 under the service, write the hex data of the WiFi SSID required for the provisioning and click send.
	| 4) Find the characteristic whose UUID is 0xEA06 under the service, write the hex data of the WiFi PASSWORD required for provisioning and click send.
	| 5) Find the characteristic whose UUID is 0xEA02 under the service, write the command(0x0100) to connect to the AP and click send.
	| 6) If the log``BK STA connected prov_test`` is displayed, the provisioning is successful.
 
	the specific instructions are shown in the figure below：

.. figure:: ../../../../_static/ble_prov_instruction.png
    :align: center
    :alt: wifi provisioning over ble instruction
    :figclass: align-center

    Figure 2. wifi provisioning over ble instruction

3. Code Explanation
---------------------------------

3.1 Add ble provisioning service
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

	| In the project,the api ble_boarding_init is used to add ble provisioning service,the UUIDs of the service and related characteristics are defined by the following macros：
	| 1) Service UUID(BOARDING_SERVICE_UUID):  0xFA00
	| 2) SSID characteristic UUID(BOARDING_CHARA_SSID_UUID):  0xEA05
	| 3) PASSWORD characteristic UUID(BOARDING_CHARA_PASSWORD_UUID):  0xEA06
	| 4) OPERATION characteristic UUID(BOARDING_CHARA_OPERATION_UUID):  0xEA02
	
.. note::
	User can modify the above UUID value as needed.

::

	//the data structure of ble provisioning service
	ble_attm_desc_t boarding_service_db[BOARDING_IDX_NB] =
	{
		//Service Declaration
		[BOARDING_IDX_SVC]        = {{BOARDING_SERVICE_UUID & 0xFF, BOARDING_SERVICE_UUID >> 8}, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},

		[BOARDING_IDX_CHAR_DECL]  = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
		// Characteristic Value
		[BOARDING_IDX_CHAR_VALUE] = {{BOARDING_CHARA_PROPERTIES_UUID & 0xFF, BOARDING_CHARA_PROPERTIES_UUID >> 8}, BK_BLE_PERM_SET(NTF, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
		//Client Characteristic Configuration Descriptor
		[BOARDING_IDX_CHAR_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},

		//opreation
		[BOARDING_IDX_CHAR_OPERATION_DECL]  = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
		[BOARDING_IDX_CHAR_OPERATION_VALUE] = {{BOARDING_CHARA_OPERATION_UUID & 0xFF, BOARDING_CHARA_OPERATION_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

		//ssid
		[BOARDING_IDX_CHAR_SSID_DECL]    = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
		[BOARDING_IDX_CHAR_SSID_VALUE]   = {{BOARDING_CHARA_SSID_UUID & 0xFF, BOARDING_CHARA_SSID_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

		//password
		[BOARDING_IDX_CHAR_PASSWORD_DECL]    = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
		[BOARDING_IDX_CHAR_PASSWORD_VALUE]   = {{BOARDING_CHARA_PASSWORD_UUID & 0xFF, BOARDING_CHARA_PASSWORD_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
	};

	int ble_boarding_init(ble_boarding_info_t *info)
	{
		bt_err_t ret = BK_FAIL;
		struct bk_ble_db_cfg ble_db_cfg;

		ble_boarding_info = info;

		ret = rtos_init_semaphore(&ble_boarding_sema, 1);
		if (ret != BK_OK)
		{
			goto error;
		}

		//create the service database
		ble_db_cfg.att_db = (ble_attm_desc_t *)boarding_service_db;
		ble_db_cfg.att_db_nb = BOARDING_IDX_NB;
		ble_db_cfg.prf_task_id = PRF_TASK_ID_BOARDING;
		ble_db_cfg.start_hdl = 0;
		ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
		ble_db_cfg.uuid[0] = BOARDING_SERVICE_UUID & 0xFF;
		ble_db_cfg.uuid[1] = BOARDING_SERVICE_UUID >> 8;

		bk_ble_set_notice_cb(ble_at_legacy_notice_cb);

		ret = bk_ble_create_db(&ble_db_cfg);

		if (ret != BK_ERR_BLE_SUCCESS)
		{
			LOGE("create gatt db failed %d\n", ret);
			goto error;
		}

		ret = rtos_get_semaphore(&ble_boarding_sema, AT_SYNC_CMD_TIMEOUT_MS);

		if (ret != BK_OK)
		{
			LOGE("wait semaphore failed at %d, %d\n", ret, __LINE__);
			goto error;
		}
		else
		{
			LOGI("create gatt db success\n");
		}

		return ret;

	error:

		return BK_FAIL;
	}

3.2 Start ble provisioning advertising
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

	In the project, ble_boarding_adv_start is called to start ble provisioning advertising,the advertising data is configured as follows.The ble device name(starting with ``BKPROV_``) displays in the serial monitor log(eg.BKPROV_56A7F2).

::

	void wifi_prov_over_ble_init(void)
	{
		...

		/* flags */
		len_index = adv_index;
		adv_data[adv_index++] = 0x00;
		adv_data[adv_index++] = ADV_TYPE_FLAGS;
		adv_data[adv_index++] = 0x06;
		adv_data[len_index] = 2;

		/* local name */
		bk_bluetooth_get_address(mac);

		len_index = adv_index;
		adv_data[adv_index++] = 0x00;
		adv_data[adv_index++] = ADV_TYPE_LOCAL_NAME;

		ret = sprintf((char *)&adv_data[adv_index], "%s_%02X%02X%02X",
					  ADV_NAME_HEAD, mac[2], mac[1], mac[0]);

		adv_index += ret;
		adv_data[len_index] = ret + 1;

		/* 16bit service uuid */
		len_index = adv_index;
		adv_data[adv_index++] = 0x00;
		adv_data[adv_index++] = ADV_TYPE_SERVICE_DATA;
		adv_data[adv_index++] = PROV_UUID & 0xFF;
		adv_data[adv_index++] = PROV_UUID >> 8;
		adv_data[len_index] = 3;

		/* manufacturer */
		len_index = adv_index;
		adv_data[adv_index++] = 0x00;
		adv_data[adv_index++] = ADV_TYPE_MANUFACTURER_SPECIFIC;
		adv_data[adv_index++] = BEKEN_COMPANY_ID & 0xFF;
		adv_data[adv_index++] = BEKEN_COMPANY_ID >> 8;
		adv_data[len_index] = 3;

		...
		//start adv
		ble_boarding_adv_start(adv_data, adv_index);

		...
	}

3.3 Receive provisioning information
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

::

	static void ble_at_legacy_notice_cb(ble_notice_t notice, void *param)
	{
		switch (notice)
		{
			...
			case BLE_5_WRITE_EVENT:
			{
				...

				if (bk_ble_get_controller_stack_type() == BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2
					&& w_req->prf_id == PRF_TASK_ID_BOARDING)
				{
					switch (w_req->att_idx)
					{
						...
						//receive the ssid from phone app
						case BOARDING_IDX_CHAR_SSID_VALUE:
						{
							if (ble_boarding_info == NULL)
							{
								LOGE("ble_boarding_info should not be NULL\n");
								break;
							}

							if (ble_boarding_info->ssid_value != NULL)
							{
								os_free(ble_boarding_info->ssid_value);
								ble_boarding_info->ssid_value = NULL;
							}

							ble_boarding_info->ssid_length = w_req->len;
							ble_boarding_info->ssid_value = os_malloc(ble_boarding_info->ssid_length + 1);


							os_memset((uint8_t *)ble_boarding_info->ssid_value, 0, ble_boarding_info->ssid_length + 1);
							os_memcpy((uint8_t *)ble_boarding_info->ssid_value, w_req->value, ble_boarding_info->ssid_length);

							LOGI("write ssid: %s, length: %d\n", ble_boarding_info->ssid_value, ble_boarding_info->ssid_length);
						}
						break;

						...
						//receive the password from phone app
						case BOARDING_IDX_CHAR_PASSWORD_VALUE:
						{
							if (ble_boarding_info == NULL)
							{
								LOGE("ble_boarding_info should not be NULL\n");
								break;
							}

							if (ble_boarding_info->password_value != NULL)
							{
								os_free(ble_boarding_info->password_value);
								ble_boarding_info->password_value = NULL;
							}

							ble_boarding_info->password_length = w_req->len;
							ble_boarding_info->password_value = os_malloc(ble_boarding_info->password_length + 1);

							os_memset((uint8_t *)ble_boarding_info->password_value, 0, ble_boarding_info->password_length + 1);
							os_memcpy((uint8_t *)ble_boarding_info->password_value, w_req->value, ble_boarding_info->password_length);

							LOGI("write password: %s, length: %d\n", ble_boarding_info->password_value, ble_boarding_info->password_length);
						}
						break;
						...
					}
				}

				break;
			}
		...
	}

3.4 Connect to WiFi AP
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

::

	static void ble_boarding_operation_handle(uint16_t opcode, uint16_t length, uint8_t *data)
	{
		LOGE("%s, opcode: %04X, length: %04X\n", __func__, opcode, length);

		switch (opcode)
		{
			case BOARDING_OP_STATION_START:
			{
	#if CONFIG_WIFI_ENABLE
				//enable STA to connect to AP
				demo_sta_app_init((char *)ble_boarding_info->ssid_value, (char *)ble_boarding_info->password_value);
	#endif
			}
			break;

		}
	}

	static void ble_at_legacy_notice_cb(ble_notice_t notice, void *param)
	{
		switch (notice)
		{
			...
			case BLE_5_WRITE_EVENT:
			{
				...

				if (bk_ble_get_controller_stack_type() == BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2
					&& w_req->prf_id == PRF_TASK_ID_BOARDING)
				{
					switch (w_req->att_idx)
					{
						...
						//receive the commond from phone app
						case BOARDING_IDX_CHAR_OPERATION_VALUE:
						{
							uint16_t opcode = 0, length = 0;
							uint8_t *data = NULL;

							if (w_req->len < 2)
							{
								LOGE("error input: operation code length: " + w_req->len);
								break;
							}

							opcode = w_req->value[0] | w_req->value[1] << 8;

							if (w_req->len >= 4)
							{
								length = w_req->value[2] | w_req->value[3] << 8;
							}

							if (w_req->len > 4)
							{
								data = &w_req->value[4];
							}
							//perfom corresponding operation on the received command
							ble_boarding_operation_handle(opcode, length, data);

						}
						break;

						...
					}
				}

				break;
			}
		...
	}
