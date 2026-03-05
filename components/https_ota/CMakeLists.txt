set(incs include)
set(srcs)

if (CONFIG_BK_OTA)
	list(APPEND srcs ota_download.c)
	list(APPEND srcs https_ota.c)
endif()

if (CONFIG_OTA_TEST)
	list(APPEND srcs cli_https_ota.c)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES bk_common bk_https psa_mbedtls bk_ota
	)
