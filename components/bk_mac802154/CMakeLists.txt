set(incs)
set(srcs)
set(PRIV_REQS)

armino_build_get_property(armino_path ARMINO_PATH)
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
        src/bk_802154.c
        src/bk_ieee802154_ack.c
        src/bk_ieee802154_frame.c
    )

    list(APPEND srcs
        soc/${target}/mac802154_adapter.c
    )
endif()

get_filename_component(PARENT_DIRECTORY "${armino_path}" DIRECTORY)
if(EXISTS "${PARENT_DIRECTORY}/components/openthread")
    list(APPEND PRIV_REQS openthread)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES lwip_intf_v2_1 bk_phy ${PRIV_REQS})
