set(incs .
    include)

set(srcs
	wapi_main.c
	)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
