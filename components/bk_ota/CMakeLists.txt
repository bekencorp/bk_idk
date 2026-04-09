set(incs include)
set(srcs)

if (CONFIG_BK_OTA)
	list(APPEND srcs
		ota.c
		ota_common.c
		ota_confirm.c
		ota_verify.c
		ota_ow.c
		ota_xip.c
		ota_bl2.c
		)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common psa_mbedtls
	)
