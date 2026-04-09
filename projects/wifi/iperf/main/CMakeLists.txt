set(incs .)
set(srcs
    iperf_main.c
    wifi_iperf_cli.c
    )



armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
