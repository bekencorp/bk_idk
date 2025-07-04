set(TE200 "${IDK_PATH}/components/tinysoc/driver/te200")

add_library(bl2_crypto_hw STATIC)

target_compile_options(bl2_crypto_hw
	PUBLIC
		${BL2_COMPILER_CP_FLAG}
)

target_sources(bl2_crypto_hw
	PRIVATE
		${TE200}/dubhe_driver/src/common/dubhe_driver.c
		${TE200}/dubhe_driver/src/common/dubhe_event.c
		${TE200}/dubhe_driver/src/common/dubhe_intr_handler.c
		${TE200}/dubhe_driver/src/common/dubhe_sram_alloc.c
		${TE200}/dubhe_driver/src/crypto/aca/dubhe_aca.c
		${TE200}/dubhe_driver/src/crypto/aca/dubhe_aca_hw.c
		${TE200}/dubhe_driver/src/crypto/aead/dubhe_ccm.c
		${TE200}/dubhe_driver/src/crypto/aead/dubhe_gcm.c
		${TE200}/dubhe_driver/src/crypto/hash/dubhe_hash.c
		${TE200}/dubhe_driver/src/crypto/mac/dubhe_cbcmac.c
		${TE200}/dubhe_driver/src/crypto/mac/dubhe_cmac.c
		${TE200}/dubhe_driver/src/crypto/mac/dubhe_ghash.c
		${TE200}/dubhe_driver/src/crypto/otp/dubhe_otp.c
		${TE200}/dubhe_driver/src/crypto/sca/dubhe_sca.c
		${TE200}/dubhe_driver/src/crypto/trng/dubhe_trng.c
		${TE200}/dubhe_driver/src/crypto/dbg/dubhe_dbg.c
		${TE200}/dubhe_driver/src/hal_pal/pal_heap.c
		${TE200}/dubhe_driver/src/hal_pal/pal_log.c
		${TE200}/dubhe_driver/src/hal_pal/pal_mutex.c
		${TE200}/dubhe_driver/src/hal_pal/pal_preempt.c
		${TE200}/dubhe_driver/src/hal_pal/pal_semaphore.c
		${TE200}/dubhe_driver/src/hal_pal/pal_string.c
		${TE200}/dubhe_driver/src/hal_pal/pal_time.c
)

target_include_directories(bl2_crypto_hw
	PRIVATE
		${TE200}/dubhe_driver/inc/common
		${TE200}/dubhe_driver/inc/crypto
		${TE200}/hal/inc
		${TE200}/interface
		${BL2_COMPONENTS_PATH}/cmsis
		${BL2_COMPONENTS_PATH}/common
)

target_sources(bl2_crypto_hw
	PRIVATE
		${TE200}/dubhe_alt/src/aes.c
		${TE200}/dubhe_alt/src/bignum_wrap.c
		${TE200}/dubhe_alt/src/ccm.c
		${TE200}/dubhe_alt/src/cmac.c
		${TE200}/dubhe_alt/src/gcm.c
		${TE200}/dubhe_alt/src/otp.c
		${TE200}/dubhe_alt/src/sha256.c
)

target_include_directories(bl2_crypto_hw
	PRIVATE
		${TE200}/dubhe_alt/inc
)

target_sources(bl2_crypto_hw
	PRIVATE
		${TE200}/dubhe_lib/bignum.c
		${TE200}/dubhe_lib/bignum_delta.c
		${TE200}/dubhe_lib/cipher.c
		${TE200}/dubhe_lib/cipher_wrap.c
		${TE200}/dubhe_lib/ctr_drbg.c
		${TE200}/dubhe_lib/dhm.c
		${TE200}/dubhe_lib/ecdh.c
		${TE200}/dubhe_lib/ecdsa.c
		${TE200}/dubhe_lib/ecp.c
		${TE200}/dubhe_lib/ecp_curves.c
		${TE200}/dubhe_lib/entropy_poll.c
		${TE200}/dubhe_lib/md.c
		${TE200}/dubhe_lib/oid.c
		${TE200}/dubhe_lib/pkparse.c
		${TE200}/dubhe_lib/rsa.c
)

target_sources(bl2_crypto_hw
	PRIVATE
		${TE200}/spe/src/misc.c
		${TE200}/crypto_hw.c
		${TE200}/hal/crypto/hal_crypto_aes.c
		${TE200}/hal/crypto/hal_crypto_rsa.c
		${TE200}/hal/crypto/hal_crypto_hash.c
		${TE200}/hal/crypto/hal_crypto_ecdsa.c
)

target_include_directories(bl2_crypto_hw
	PRIVATE
		${TE200}/spe/inc
		${TE200}/spe/inc/hal
		${TE200}/spe/inc/pal
)

target_include_directories(bl2_crypto_hw
	PUBLIC
		${TE200}/config
		${TE200}/include
		${MBEDCRYPTO_PATH}/library
		${MBEDCRYPTO_PATH}/include
		${IDK_PATH}/components/tinysoc/${SOC}/config
		${CMAKE_SOURCE_DIR}/interface/include
		${CMAKE_BINARY_DIR}/generated/interface/include
)

target_compile_definitions(bl2_crypto_hw
	PRIVATE
		ARM_CE_DUBHE
		DUBHE_SECURE
		ARM_CE_DUBHE_SCA
		ARM_CE_DUBHE_TRNG
		ARM_CE_DUBHE_OTP
		ARM_CE_DUBHE_HASH
		ARM_CE_DUBHE_ACA
		CONFIG_ENABLE_MCUBOOT_BL2
)

target_link_libraries(bl2_crypto_hw
	PRIVATE
		bl2_mbedcrypto
)

target_include_directories(bl2_mbedcrypto
	PUBLIC
		${TE200}/config
		${TE200}/dubhe_driver/inc/common
		${TE200}/dubhe_driver/inc/crypto
		${TE200}/dubhe_alt/inc
)
