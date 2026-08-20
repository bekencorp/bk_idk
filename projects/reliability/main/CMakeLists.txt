set(incs .)
set(srcs app_main.c)

if (CONFIG_WIFI_RELIABILITY_TEST)
	list(APPEND srcs wifi_test.c)
endif()

if (CONFIG_FLASH_RELIABILITY_TEST)
	list(APPEND srcs flash_test.c)
endif()

if (CONFIG_FLASH_IMPORT_TEST)
	list(APPEND srcs flash_import/flash_import_case_endurance.c)
	list(APPEND srcs flash_import/flash_import_case_zero_read.c)
	list(APPEND srcs flash_import/flash_import_case_driver.c)
	list(APPEND srcs flash_import/flash_import_case_loop.c)
	list(APPEND srcs flash_import/flash_import_case_program_disturb.c)
	list(APPEND srcs flash_import/flash_import_case_soft_erase.c)
	if (CONFIG_FLASH_IMPORT_SLEEP_TEST)
		list(APPEND srcs flash_import/flash_import_case_sleep.c)
	endif()
	if (CONFIG_CLI)
		list(APPEND srcs flash_import/cli_flash_import.c)
	endif()
endif()

if (CONFIG_WIFI_RELIABILITY_TEST OR CONFIG_FLASH_RELIABILITY_TEST)
	list(APPEND srcs crc32.c)
	list(APPEND srcs statis.c)
	if (CONFIG_CLI)
		list(APPEND srcs cli_statis.c)
	endif()
endif()

if (CONFIG_FLASH_IMPORT_TEST)
	list(APPEND srcs crc32.c)
endif()

if (CONFIG_PSRAM_RELIABILITY_TEST)
	list(APPEND srcs psram_test.c)
endif()

if (CONFIG_HTOL_RELIABILITY_TEST)
	list(APPEND srcs htol_test.c)
endif()

set(main_priv_requires "")
if (CONFIG_CLI AND (CONFIG_WIFI_RELIABILITY_TEST OR CONFIG_FLASH_RELIABILITY_TEST))
	list(APPEND main_priv_requires bk_cli)
endif()

if (CONFIG_CLI AND CONFIG_FLASH_IMPORT_TEST)
	list(APPEND main_priv_requires bk_cli)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES ${main_priv_requires})
