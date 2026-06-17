set(incs)
set(srcs)

if (CONFIG_SUPPORT_NVS_FLASH)
list(APPEND incs
	./include
	./port
	$ENV{PARTITIONS_DIR}
)

list(APPEND srcs
	"src/nvs_api.cpp"
	"src/nvs_cxx_api.cpp"
	"src/nvs_item_hash_list.cpp"
	"src/nvs_page.cpp"
	"src/nvs_pagemanager.cpp"
	"src/nvs_storage.cpp"
	"src/nvs_handle_simple.cpp"
	"src/nvs_handle_locked.cpp"
	"src/nvs_partition.cpp"
	"src/nvs_partition_lookup.cpp"
	"src/nvs_partition_manager.cpp"
	"src/nvs_encrypted_partition.cpp"
	"src/nvs_types.cpp"
	"src/nvs_platform.cpp"
	"port/rom_crc.cpp"
	"port/platform.cpp"
	"port/locks.c"
	"port/nvs_sec_provider.c"
)

if (CONFIG_NVS_TEST)
	list(APPEND srcs
		"src/nvs_test.c"
	)
endif()

if (CONFIG_SUPPORT_NVS_THREAD_SAFETY_TEST)
	list(APPEND srcs
		"src/nvs_thread_safety_test.c"
	)
endif()
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES include mbedtls psa_mbedtls tfm_mbedtls os_source
)
