armino_component_register(INCLUDE_DIRS .
	modules/securityip
	modules/freetype
)

target_include_directories(${COMPONENT_LIB} INTERFACE $ENV{SECURITY_PREBUILD_DIR} $ENV{SYS_PERSIST_CONFIG_DIR} $ENV{PARTITIONS_DIR})
