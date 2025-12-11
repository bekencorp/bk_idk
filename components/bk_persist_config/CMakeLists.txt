set(incs)
set(srcs)

if(CONFIG_PERSIST_CONFIG)
	list(APPEND srcs $ENV{SYS_PERSIST_CONFIG_DIR}/sys_persist_config.c)
endif()

if(CONFIG_PERSIST_CONFIG_CLI)
	list(APPEND srcs $ENV{SYS_PERSIST_CONFIG_DIR}/sys_persist_config_cli.c)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common)
