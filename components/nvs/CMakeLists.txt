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
	"src/nvs_test.c"
	"src/nvs_thread_safety_test.c"
	"port/rom_crc.cpp"
	"port/platform.cpp"
	"port/locks.c"
	"port/nvs_sec_provider.c"
)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES include mbedtls psa_mbedtls tfm_mbedtls os_source
)
