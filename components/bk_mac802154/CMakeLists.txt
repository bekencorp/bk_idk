set(incs)
set(srcs)

armino_build_get_property(target ARMINO_SOC)

if (CONFIG_MAC802154_ENABLE)

    set(middleware ../../middleware)
    set(bk_inc ../../include)

    list(APPEND incs
        ${middleware}/soc/common/hal
        ${bk_inc}/os
        include/private
        include
    )

    list(APPEND srcs
        src/mac802154.c
    )

    list(APPEND srcs
        soc/${target}/mac802154_adapter.c
    )
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_phy)

