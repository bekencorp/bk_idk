set(incs .)
set(srcs
    app_main.c
    )

if (CONFIG_HTOL_RELIABILITY_TEST)
    list(APPEND srcs htol_test.c)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
