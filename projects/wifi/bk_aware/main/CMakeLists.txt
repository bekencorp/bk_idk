set(incs .)
set(srcs
    app_main.c
	bk_aware_crc.c
	bk_aware_demo.c
    )

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES wpa_supplicant-2.10)
