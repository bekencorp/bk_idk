set(incs include include/bk_private)
set(srcs)

list(APPEND srcs ota.c)
if (CONFIG_OTA_HTTPS)
list(APPEND srcs ota_https.c)
endif()
if (CONFIG_OTA_HASH_FUNCTION)
list(APPEND srcs ota_hash.c)
endif()
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common bk_netif bk_ps fatfs bk_init at bk_wifi compal http bk_https mbedtls tfm
	)