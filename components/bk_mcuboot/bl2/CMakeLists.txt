cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake)
include(components/config/set_config.cmake)
include(${BL2_PATH}/tools/cmake/toolchain.cmake)

set(CMAKE_PROJECT_INCLUDE_BEFORE ${CMAKE_SOURCE_DIR}/tools/cmake/disable_compiler_detection.cmake)

project("BL2" VERSION 2.1.0 LANGUAGES C ASM)
bl2_toolchain_reload_compiler()

# Synchronise the install path variables. If CMAKE_INSTALL_PREFIX is manually
# set then set both to the value of that, else set both to the value of
# TFM_INSTALL_PATH. This has to be done after the call to `project()`.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX  ${TFM_INSTALL_PATH} CACHE PATH  "" FORCE)
else()
    set(TFM_INSTALL_PATH  ${CMAKE_INSTALL_PREFIX} CACHE PATH  "Path to which to install TF-M files" FORCE)
endif()

#TODO 
add_executable(bl2 components/mcuboot/src/bl2_main.c)

add_subdirectory(components/mcuboot)
add_subdirectory(components/mbedtls)
add_subdirectory(middleware)
add_subdirectory(components/download)

set_target_properties(bl2
    PROPERTIES
        SUFFIX ".axf"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_options(bl2 PRIVATE ${BL2_COMPILER_CP_FLAG})

target_link_options(bl2
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wl,-Map=${CMAKE_BINARY_DIR}/bin/bl2.map>
        $<$<C_COMPILER_ID:ARMClang>:--map>
        $<$<C_COMPILER_ID:IAR>:--map\;${CMAKE_BINARY_DIR}/bin/bl2.map>
)

if(CONFIG_OTA_CONFIRM_UPDATE)
    target_compile_definitions(bl2
        PUBLIC
            CONFIG_OTA_CONFIRM_UPDATE
    )
endif()

if(CONFIG_OTA_VERIFY_TRUST_CHAIN)
    target_compile_definitions(bl2
        PUBLIC
            CONFIG_OTA_VERIFY_TRUST_CHAIN
    )
endif()

if(CONFIG_OTA_UPDATE_PUBKEY)
    target_compile_definitions(bl2
        PUBLIC
            CONFIG_OTA_UPDATE_PUBKEY
    )
endif()

add_convert_to_bin_target(bl2)

install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/ DESTINATION ${TFM_INSTALL_PATH}/outputs)
