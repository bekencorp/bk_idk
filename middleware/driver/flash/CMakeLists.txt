if (CONFIG_FLASH)
	list(APPEND incs flash)
	list(APPEND srcs
		flash/flash_bypass.c
		flash/flash_notify.c
		flash/CheckSumUtils.c
	)

	if (CONFIG_SUPPORT_PARTITIONS_PARTITION)
		list(APPEND srcs
			flash/flash_partition_v1_1.c
		)
	else()
		list(APPEND srcs
			flash/flash_partition.c
		)
	endif()

	if (CONFIG_SYS_CPU0)
		list(APPEND srcs
			flash/flash_driver.c
			flash/flash_driver_ext.c)

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
	if (CONFIG_FLASH_SVR_TEST)
		list(APPEND srcs flash/flash_svr_test.c)
	endif()

	if (CONFIG_FLASH_RACE_TEST)
		list(APPEND srcs flash/flash_thread_safety_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
