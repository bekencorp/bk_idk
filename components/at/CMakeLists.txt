set(incs)
set(srcs)

if ((CONFIG_AT_CMD OR CONFIG_AT) AND (CONFIG_OTA_HTTP OR (CONFIG_OTA_HTTPS AND CONFIG_OTA_TEST)))
	list(APPEND incs
		include
		)
	list(APPEND srcs
		"src/ota_at_command.c"
        )
endif()

if ((CONFIG_AT_CMD OR CONFIG_AT) AND CONFIG_OTA_HTTP)
	list(APPEND incs
		include
		)
	list(APPEND srcs
		"src/http_at_command.c"
        )
	if (CONFIG_HTTP)
		list(APPEND priv_req_list http)
	endif()
endif()

if (CONFIG_AT_CMD)
    list(APPEND incs
		include
		)
	list(APPEND srcs
		"src/at_common.c"
        )

	if (CONFIG_WIFI_ENABLE)
		list(APPEND srcs
			"src/wifi_at_command.c")
	endif()

	if (CONFIG_BT)
	if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
		list(APPEND srcs
			"src/bt_at_command.c")
	endif()
	endif()

	if (CONFIG_BLE)
	if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
                list(APPEND srcs
                        "src/ble_at_command.c"
						"src/ble_at_gatt.c"
									)
	endif()
	endif()

endif()

set(priv_req_list ali_mqtt bk_bluetooth bt lwip_intf_v2_1)
if ((CONFIG_AT_CMD OR CONFIG_AT) AND (CONFIG_OTA_HTTP OR (CONFIG_OTA_HTTPS AND CONFIG_OTA_TEST)))
	if (CONFIG_OTA_HTTP)
		list(APPEND priv_req_list ota)
	endif()
	if (CONFIG_OTA_HTTPS AND CONFIG_OTA_TEST)
		list(APPEND priv_req_list https_ota)
	endif()
	if (CONFIG_OTA_HTTP AND CONFIG_WIFI_ENABLE)
		list(APPEND priv_req_list bk_wifi)
	endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}"
                        REQUIRES bk_common bk_log
                        PRIV_REQUIRES ${priv_req_list})
