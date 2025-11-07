if (BL2_DOWNLOAD)
	add_library(download 
		bl_bk_hci_protocol.c
		bl_extflash.c
		bl_spi.c
		bl_tra_hcit.c
		bl_uart.c
		bootloader.c
		crc32.c
		system.c
	)
if (CONFIG_ENCRYPT_PRIVATE_KEY)
	target_sources(download PRIVATE bl_key_injection.c)
endif()

	string(REGEX REPLACE "beken/" "" SOC "${TFM_PLATFORM}")

if(DEFINED SOC_PATH)
	string(REGEX MATCH "[^/]+$" SOC "${SOC_PATH}")
endif()

if(NOT DEFINED TFM_SOC)
	set(TFM_SOC "${SOC}")
endif()

	set(download_inc
		.
		./inc
		./inc/pal
		./hal/inc
		${TFM_BEKEN}/${TFM_SOC}/armino/driver/hal
		../${TFM_SOC}/cmsis_core
		${TFM_BEKEN}/retarget
		${TFM_BEKEN}/partition
		../armino/include
		${IDK_PATH}/components/tinysoc/${TFM_SOC}/config
		./../../../cmsis/
		${ARMINO_MIDDLEWARE_REPO_PATH}/boards/${SOC}/partitions
		${PARTITION_HDR_DIR}
		${SECURITY_HDR_DIR}
		${IDK_PATH}/include
		${IDK_PATH}/include/common
		${IDK_PATH}/include/driver
		${IDK_PATH}/components/bk_mcuboot/bl2/components/cmsis
		${IDK_PATH}/components/bk_mcuboot/bl2/middleware/soc/${SOC}/cmsis_core
		${IDK_PATH}/middleware/soc/${SOC}/hal

	)

	target_compile_definitions(download
		PUBLIC
			CONFIG_BL2_DOWNLOAD
	)

	if(TFM_SOC STREQUAL "bk7239n")
		target_compile_definitions(download
		PUBLIC
			CHIP_BK7239
		)
	elseif(TFM_SOC STREQUAL "bk7236n")
		target_compile_definitions(download
		PUBLIC
			CHIP_BK7236N
		)
	else()
		target_compile_definitions(download
		PUBLIC
			CHIP_BK7236
		)
	endif()

	if(CONFIG_BL2_UPDATE_WITH_PC)
		target_compile_definitions(download
			PUBLIC
				CONFIG_BL2_UPDATE_WITH_PC
		)
	endif()

	if (CONFIG_RANDOM_AES_UPGRADE_BL2 STREQUAL "y")
		target_compile_definitions(download
			PUBLIC
				CONFIG_RANDOM_AES_UPGRADE_BL2
		)
	endif()

	if(CONFIG_ENCRYPT_PRIVATE_KEY)
		target_compile_definitions(download
			PUBLIC
				CONFIG_ENCRYPT_PRIVATE_KEY
	)

	target_link_libraries(download
        PUBLIC
			crypto_service_mbedcrypto
			# bl2_crypto_hw
    )
	endif()

	target_compile_options(download PUBLIC ${BL2_COMPILER_CP_FLAG})
	target_include_directories(download PRIVATE ${download_inc})
	target_link_libraries(bl2 PUBLIC download)
	link_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()


