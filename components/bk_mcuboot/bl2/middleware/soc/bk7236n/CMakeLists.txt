cmake_policy(SET CMP0076 NEW)

set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

target_sources(bl2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/startup/bl2_startup.c)
target_add_scatter_file(bl2 ${CMAKE_CURRENT_SOURCE_DIR}/link/bl2.ld)

set(platform_bl2_src
	PRIVATE
		cmsis_core/system_cmsdk_bk7236n.c
		cmsis_core/mpu_cfg.c
)

target_compile_definitions(platform_bl2
	PUBLIC
		-DMCUBOOT_IMAGE_ACCESS_HOOKS=1
		-DCRYPTO_HW_ACCELERATOR
		-DCONFIG_ENABLE_MCUBOOT_BL2=1
		-DCONFIG_FLASH_FAST_ERASE=1
		-DCONFIG_BL2_REG_BASE=1
		-DCONFIG_UART_PRINT_BAUD_RATE=${CONFIG_UART_PRINT_BAUD_RATE}
		__EN_ICACHE
		__EN_DCACHE
	)

if (CONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE)
	target_compile_definitions(platform_bl2 PUBLIC
		-DCONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE)
endif()

set(platform_bl2_inc
	PUBLIC
		flash_layout
		cmsis_core
		${PARTITION_HDR_DIR}
		${SECURITY_HDR_DIR}
		${BL2_COMPONENTS_PATH}/bl2
		${SYS_PERSIST_CONFIG_DIR}

	PRIVATE
		.
		${PLATFORM_DIR}/..
		${BL2_COMPONENTS_PATH}/native_drivers
		${BL2_COMPONENTS_PATH}/bk_mcuboot/
		${BL2_COMPONENTS_PATH}/bk_mcuboot/include
		${TE200}/dubhe_alt/inc
)

if(CRYPTO_HW_ACCELERATOR)
	list(APPEND platform_bl2_inc
		${BL2_COMPONENTS_PATH}/common/hal/inc/
		${TE200}/spe/inc/hal
		${TE200}/spe/inc/pal
		${TE200}/dubhe_driver/inc/crypto
		${BL2_COMPONENTS_PATH}/armino/driver/sys_ctrl/
		./armino/driver/include/
		${BL2_COMPONENTS_PATH}/armino/include/driver/
	)
endif()

if(CONFIG_BL2_WDT)
	target_compile_definitions(platform_bl2
		PUBLIC
			CONFIG_BL2_WDT
	)
endif()

if(CONFIG_DECOMPRESS_RESUME)
	target_compile_definitions(platform_bl2
		PUBLIC
			CONFIG_DECOMPRESS_RESUME
	)
endif()

if(CONFIG_BL2_DOWNLOAD)
	target_compile_definitions(platform_bl2
		PUBLIC
		CONFIG_BL2_DOWNLOAD
	)
endif()

if(CONFIG_BL2_WDT_PERIOD)
	target_compile_definitions(platform_bl2
		PUBLIC
			CONFIG_BL2_WDT_PERIOD
	)
endif()

if(CONFIG_AON_PMU_REG0_REFACTOR_DEV)
	target_compile_definitions(platform_bl2
		PUBLIC
			CONFIG_AON_PMU_REG0_REFACTOR_DEV
	)
endif()

if(MCUBOOT_UPGRADE_STRATEGY STREQUAL OVERWRITE_ONLY)
	list(APPEND platform_bl2_src
		${IDK_PATH}/components/compress/decompress.c
		${IDK_PATH}/components/compress/lzma.c
		${IDK_PATH}/components/compress/lzma1900/LzmaDec.c
		${TINYSOC_PATH}/driver/flash/flash_driver.c
	)

	list(APPEND platform_bl2_inc
		${IDK_PATH}/components/compress/include
		${IDK_PATH}/components/compress/lzma1900
		${TINYSOC_PATH}/driver/flash
	)
endif()


add_library(bk_mcuboot_config INTERFACE)
target_sources(bk_mcuboot_config INTERFACE 
	${IDK_PATH}/components/compress/decompress.c
	${IDK_PATH}/components/compress/lzma.c
	${IDK_PATH}/components/compress/lzma1900/LzmaDec.c
	${TINYSOC_PATH}/driver/flash/flash_driver.c
)
target_include_directories(bk_mcuboot_config INTERFACE
	${BL2_COMPONENTS_PATH}/bl2/include
	${IDK_PATH}/components/compress/include
	${IDK_PATH}/components/compress/lzma1900
	${TINYSOC_PATH}/driver/flash
)

target_include_directories(platform_bl2 PUBLIC ${platform_bl2_inc})
target_sources(platform_bl2 PRIVATE ${platform_bl2_src})
add_subdirectory(tinysoc)
