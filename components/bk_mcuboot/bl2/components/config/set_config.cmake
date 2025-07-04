cmake_minimum_required(VERSION 3.15)

set(TINYSOC_PATH "${IDK_PATH}/components/tinysoc")
set(MBEDTLS_PATH "${IDK_PATH}/components/tfm_mbedtls")
set(BL2_PATH "${IDK_PATH}/components/bk_mcuboot/bl2/")
set(BL2_COMPONENTS_PATH "${BL2_PATH}/components")
set(SOC_PATH "${BL2_PATH}/middleware/soc/${SOC}")
set(ARCH_PATH "${BL2_PATH}/middleware/arch/cm33")

include(${SOC_PATH}/config/compile-options.cmake)
include(${SOC_PATH}/config/config.cmake)
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWERCASE)
include(${BL2_COMPONENTS_PATH}/config/build_type/${CMAKE_BUILD_TYPE_LOWERCASE}.cmake)

include(${BL2_COMPONENTS_PATH}/config/config.cmake)
include(${BL2_COMPONENTS_PATH}/config/mcuboot_default_config.cmake)
include(${BL2_COMPONENTS_PATH}/config/config_default.cmake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/components/config/idk_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config/idk_config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/config)


