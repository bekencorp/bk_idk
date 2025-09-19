set(CRYPTO_HW_ACCELERATOR            ON         CACHE BOOL      "Whether to enable the crypto hardware accelerator on supported platforms")
set(CRYPTO_NV_SEED                   OFF        CACHE BOOL      "Use stored NV seed to provide entropy")
set(BL2                              ON         CACHE BOOL      "Whether to build BL2")
set(DEFAULT_MCUBOOT_FLASH_MAP        OFF        CACHE BOOL      "Whether to use the default flash map defined by TF-M project")

set(MCUBOOT_DATA_SHARING             OFF         CACHE BOOL      "Enable Data Sharing")
set(MCUBOOT_MEASURED_BOOT            OFF         CACHE BOOL      "Add boot measurement values to boot status. Used for initial attestation token")

set(MCUBOOT_IMAGE_NUMBER             1          CACHE STRING    "Whether to combine S and NS into either 1 image, or sign each seperately")

set(PLATFORM_DEFAULT_ROTPK           OFF        CACHE BOOL      "Use default root of trust public key.")

set(PLATFORM_DEFAULT_NV_COUNTERS     OFF          CACHE BOOL      "Use default nv counter implementation.")

set(PLATFORM_DEFAULT_OTP             OFF        CACHE BOOL      "Use trusted on-chip flash to implement OTP memory")
set(PLATFORM_DEFAULT_OTP_WRITEABLE   ON         CACHE BOOL      "Use OTP memory with write support")
set(TFM_DUMMY_PROVISIONING           OFF        CACHE BOOL      "Provision with dummy values. NOT to be used in production")
set(PS_NUM_ASSETS                    "20"      CACHE STRING    "The maximum number of assets to be stored in the Protected Storage area")
set(PS_MAX_ASSET_SIZE                "1024"      CACHE STRING    "The maximum asset size to be stored in the Protected Storage area")
set(ITS_MAX_ASSET_SIZE               512       CACHE STRING    "The maximum asset size to be stored in the Internal Trusted Storage area")
set(ITS_BUF_SIZE                     1024      CACHE STRING    "Size of the ITS internal data transfer buffer (defaults to ITS_MAX_ASSET_SIZE if not set)")
set(ITS_NUM_ASSETS                   "20"        CACHE STRING    "The maximum number of assets to be stored in the Internal Trusted Storage area")

set(BL2_HEADER_SIZE  0x1000 CACHE STRING "Header size")
set(BL2_TRAILER_SIZE 0x1000 CACHE STRING "Trailer size")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DCONFIG_BUILD_DEBUG)
endif()
