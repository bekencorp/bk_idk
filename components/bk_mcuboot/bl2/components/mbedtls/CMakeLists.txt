cmake_minimum_required(VERSION 3.15)

add_library(bl2_mbedcrypto_config INTERFACE)

target_compile_definitions(bl2_mbedcrypto_config
	INTERFACE
		$<$<STREQUAL:${MCUBOOT_SIGNATURE_TYPE},RSA>:MCUBOOT_SIGN_RSA>
		$<$<STREQUAL:${MCUBOOT_SIGNATURE_TYPE},RSA>:MCUBOOT_SIGN_RSA_LEN=${MCUBOOT_SIGNATURE_KEY_LEN}>
		MBEDTLS_CONFIG_FILE="${MCUBOOT_MBEDCRYPTO_CONFIG_FILEPATH}"
		# Workaround for https://github.com/ARMmbed/mbedtls/issues/1077
		$<$<OR:$<STREQUAL:${TFM_SYSTEM_ARCHITECTURE},armv8-m.base>,$<STREQUAL:${TFM_SYSTEM_ARCHITECTURE},armv6-m>>:MULADDC_CANNOT_USE_R7>
)

cmake_policy(SET CMP0079 NEW)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(ENABLE_TESTING OFF)
set(ENABLE_PROGRAMS OFF)
set(MBEDTLS_FATAL_WARNINGS OFF)
set(ENABLE_DOCS OFF)
set(INSTALL_MBEDTLS_HEADERS OFF)
set(LIB_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/mbedcrypto/install)

# Set the prefix to be used by mbedTLS targets
set(MBEDTLS_TARGET_PREFIX bl2_)

# Mbedcrypto is quite a large lib, and it uses too much memory for it to be
# reasonable to build it in debug info. As a compromise, if `debug` build type
# is selected mbedcrypto will build under `relwithdebinfo` which preserved debug
# symbols whild optimizing space.
set(SAVED_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE ${MBEDCRYPTO_BUILD_TYPE})
add_subdirectory(${MBEDCRYPTO_PATH} ${CMAKE_CURRENT_BINARY_DIR}/mbedcrypto EXCLUDE_FROM_ALL)
set(CMAKE_BUILD_TYPE ${SAVED_BUILD_TYPE} CACHE STRING "Build type: [Debug, Release, RelWithDebInfo, MinSizeRel]" FORCE)

if(NOT TARGET ${MBEDTLS_TARGET_PREFIX}mbedcrypto)
    message(FATAL_ERROR "Target ${MBEDTLS_TARGET_PREFIX}mbedcrypto does not exist. Have the patches in ${CMAKE_SOURCE_DIR}/lib/ext/mbedcrypto been applied to the mbedcrypto repo at ${MBEDCRYPTO_PATH} ?
    Hint: The command might be `cd ${MBEDCRYPTO_PATH} && git apply ${CMAKE_SOURCE_DIR}/lib/ext/mbedcrypto/*.patch`")
endif()

target_link_libraries(${MBEDTLS_TARGET_PREFIX}mbedcrypto
    PUBLIC
        bl2_mbedcrypto_config
)

target_include_directories(${MBEDTLS_TARGET_PREFIX}mbedcrypto
    PUBLIC
        ${MBEDCRYPTO_PATH}/library
)

target_compile_options(bl2_mbedcrypto
    PRIVATE
        ${BL2_COMPILER_CP_FLAG}
)

target_compile_options(bl2_mbedtls
    PRIVATE
        ${BL2_COMPILER_CP_FLAG}
)

target_compile_options(bl2_mbedx509
    PRIVATE
        ${BL2_COMPILER_CP_FLAG}
)

target_link_libraries(bl2_mbedcrypto
	PUBLIC
		bl2_crypto_hw
)

target_compile_definitions(bl2_mbedcrypto
	PUBLIC
		CRYPTO_HW_ACCELERATOR
		CRYPTO_HW_ACCELERATOR_OTP_PROVISIONING
)

target_compile_options(bl2_mbedcrypto
	PRIVATE
		-Wno-unused-parameter
)

include(${TINYSOC_PATH}/driver/te200/bl2.cmake)
