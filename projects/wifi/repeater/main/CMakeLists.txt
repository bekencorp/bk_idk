set(incs .)
set(srcs
    "repeater_main.c"
	"wifi_repeater_cli.c"
    )



armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

