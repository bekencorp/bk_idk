if (CONFIG_FLASH)
	list(APPEND incs flash)
	list(APPEND srcs
		flash/flash_partition.c
		flash/flash_bypass.c
		flash/flash_notify.c
	)
	if (CONFIG_SYS_CPU0)
		list(APPEND srcs flash/flash_driver.c flash/flash_driver_ext.c)
		if ((CONFIG_CPU_CNT GREATER 1))
			list(APPEND srcs flash/flash_server.c)
		endif()
	endif()
	if (NOT CONFIG_SYS_CPU0)
		list(APPEND srcs flash/flash_client.c)
	endif()
	if (CONFIG_SYS_CPU1)
		list(APPEND srcs flash/flash_client.c flash/flash_notify.c)
	endif()
	if (CONFIG_FLASH_TEST)
		list(APPEND srcs flash/flash_svr_test.c)
	endif()

	if(CONFIG_FLASH_API_TEST)
		list(APPEND srcs flash/cli_flash_api.c)
	endif()
	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
