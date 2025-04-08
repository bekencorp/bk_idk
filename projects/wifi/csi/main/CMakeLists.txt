
set(incs .
    )
set(srcs
    armino_main.c
    bk_csi_demo.c
    )


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")