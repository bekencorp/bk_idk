
set(incs .
    )
set(srcs
    armino_main.c
    bk_csi_demo.c
	bk_csi_ble_provision.c
    )

set(idk_components  ../../../../components)
list(APPEND incs
		${idk_components}/bluetooth
		${idk_components}/bk_bluetooth/bt/ble_boarding
	)

set(idk_include  ../../../../include)
list(APPEND incs
		${idk_include}/components/bluetooth
		${idk_include}/os
	)

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_init bk_cli bk_bluetooth bt json
)