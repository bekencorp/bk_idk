cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0079 NEW)

add_library(mcuboot_config INTERFACE)
add_subdirectory(bootutil bootutil)
target_link_libraries(bootutil PRIVATE bk_mcuboot_config)

target_sources(bl2
	PRIVATE
		src/flash_map.c
		src/bk_flash_map.c
		src/decompress_bl2.c
		#$<$<BOOL:${DEFAULT_MCUBOOT_SECURITY_COUNTERS}>:${CMAKE_CURRENT_SOURCE_DIR}/src/security_cnt.c>
		$<$<BOOL:${DEFAULT_MCUBOOT_FLASH_MAP}>:${CMAKE_CURRENT_SOURCE_DIR}/src/default_flash_map.c>
		$<$<BOOL:${MCUBOOT_DATA_SHARING}>:${CMAKE_CURRENT_SOURCE_DIR}/src/shared_data.c>
		${CMAKE_CURRENT_SOURCE_DIR}/src/bl2_securedebug.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/keys.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/flash_map_extended.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/flash_map_legacy.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/boot_hal_bl2.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/bk_loader.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/ow_loader.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/xip_loader.c
		$<$<STREQUAL:${MCUBOOT_UPGRADE_STRATEGY},DIRECT_XIP>:${CMAKE_CURRENT_SOURCE_DIR}/src/xip_pubkey.c>
		$<$<STREQUAL:${MCUBOOT_UPGRADE_STRATEGY},OVERWRITE_ONLY>:${CMAKE_CURRENT_SOURCE_DIR}/src/ow_pubkey.c>
		${CMAKE_CURRENT_SOURCE_DIR}/src/common_loader.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/hooks_bl2.c
		$<$<NOT:$<BOOL:${CONFIG_DBUS_CHECK_CRC}>>:${CMAKE_CURRENT_SOURCE_DIR}/src/bootutil_public.c>
		${TINYSOC_PATH}/driver/cmsis_drivers/Driver_Flash.c
)

target_include_directories(bl2
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bootutil/src>
		${TINYSOC_PATH}/driver/te200/interface
)

target_include_directories(bootutil
	PUBLIC
		include
		config
)

target_link_libraries(bootutil
	PUBLIC
		mcuboot_config
		platform_bl2
		bl2_mbedcrypto
)

target_compile_options(bootutil
	PRIVATE
		${BL2_COMPILER_CP_FLAG}
)

target_compile_definitions(bootutil
	PRIVATE
		$<$<BOOL:${DEFAULT_MCUBOOT_FLASH_MAP}>:DEFAULT_MCUBOOT_FLASH_MAP>
		$<$<BOOL:${MCUBOOT_DIRECT_XIP}>:CONFIG_DIRECT_XIP>
		$<$<BOOL:${CONFIG_FLASH_CRC_EN}>:CONFIG_FLASH_CRC_EN>
		$<$<BOOL:${CONFIG_OTA_CONFIRM_UPDATE}>:CONFIG_OTA_CONFIRM_UPDATE>
		$<$<BOOL:${CONFIG_BL2_SKIP_VALIDATE}>:CONFIG_BL2_SKIP_VALIDATE>
		$<$<BOOL:${CONFIG_DBUS_CHECK_CRC}>:CONFIG_DBUS_CHECK_CRC>
		$<$<BOOL:${CONFIG_DECOMPRESS_RESUME}>:CONFIG_DECOMPRESS_RESUME>
		$<$<BOOL:${CONFIG_BL2_WDT}>:CONFIG_BL2_WDT>
		$<$<BOOL:${CONFIG_OTA_UPDATE_PUBKEY}>:CONFIG_OTA_UPDATE_PUBKEY>
)

target_include_directories(mcuboot_config
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # for mcuboot_config.h only
)

target_link_libraries(bl2 PUBLIC bootutil)

set(MCUBOOT_ALLOWED_LOG_LEVELS OFF ERROR WARNING INFO DEBUG)
list(FIND MCUBOOT_ALLOWED_LOG_LEVELS ${MCUBOOT_LOG_LEVEL} LOG_LEVEL_ID)

if (MCUBOOT_ALIGN_VAL GREATER 8)
	set (MCUBOOT_BOOT_MAX_ALIGN ${MCUBOOT_ALIGN_VAL})
else()
	set (MCUBOOT_BOOT_MAX_ALIGN 8)
endif()

configure_file(include/mcuboot_config/mcuboot_config.h.in
       ${CMAKE_CURRENT_BINARY_DIR}/mcuboot_config/mcuboot_config.h
       @ONLY)
