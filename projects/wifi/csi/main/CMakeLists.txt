
set(incs .
	)
set(srcs
	armino_main.c
	)

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_init
)