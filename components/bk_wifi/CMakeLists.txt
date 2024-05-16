set(incs include include/bk_private)
set(srcs)
if(CONFIG_WIFI_ENABLE)
list(APPEND srcs
	src/bk_wifi_adapter.c
	src/wifi_api_wrapper.c
)
if(CONFIG_AT)
list(APPEND srcs
	src/wifi_at.c
)
set(components ../../components)
set(IDK_INC ../../include)
list(APPEND incs
	${components}/at_server
	${components}/at_server/_at_server
	${components}/at_server/_at_server_port
	${components}/bk_net/include
	${components}/bk_event
	${IDK_INC}/modules
	)
endif()
endif()
if(CONFIG_NET_PARAM)
list(APPEND srcs
	src/net_param.c
)
endif()
armino_component_register(SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common wpa_supplicant mbedtls wolfssl lwip_intf_v2_1 uart_debug bk_phy
    REQUIRES bk_netif)


if(CONFIG_WIFI_ENABLE)

	if (CONFIG_TZ AND NOT CONFIG_SPE)
		if(CONFIG_MULTI_STA_IN_SAP)
			set (wifi_elem libwifi_multi_sta_nspe.a)
		else()
			if(CONFIG_WIFI_CSI_EN)
				set (wifi_elem libwifi_csi_nspe.a)
			else()
				set (wifi_elem libwifi_nspe.a)
			endif()
		endif()

	else()
		if(CONFIG_MULTI_STA_IN_SAP)
			set (wifi_elem libwifi_multi_sta.a)
		else()
			if(CONFIG_WIFI_CSI_EN)
				set (wifi_elem libwifi_csi.a)
			else()
				set (wifi_elem libwifi.a)
			endif()
		endif()
	endif()

	foreach(lib_name ${wifi_elem})
		#message("WARINIG lib_name ${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}_$ENV{PROJECT_LIBS}/libs/${lib_name}")
		add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
		target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
	endforeach()

endif()

armino_component_register_call_subdirs()

