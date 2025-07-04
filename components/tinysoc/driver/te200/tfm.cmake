target_sources(crypto_service_crypto_hw
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
        ${TE200}/dubhe_driver/src/hal_pal/pal_heap.c
        ${TE200}/dubhe_driver/src/hal_pal/pal_log.c
        ${TE200}/dubhe_driver/src/hal_pal/pal_mutex.c
        ${TE200}/dubhe_driver/src/hal_pal/pal_preempt.c
        ${TE200}/dubhe_driver/src/hal_pal/pal_semaphore.c
        ${TE200}/dubhe_driver/src/hal_pal/pal_signal.c
        ${TE200}/dubhe_driver/src/hal_pal/pal_string.c
        ${TE200}/dubhe_driver/src/hal_pal/pal_time.c
)

target_include_directories(crypto_service_crypto_hw
    PRIVATE
		${TE200}/dubhe_driver/inc/common
		${TE200}/dubhe_driver/inc/crypto
		${PLATFORM_DIR}/ext/target/beken/common/hal/inc
)

target_sources(crypto_service_crypto_hw
    PRIVATE
        ${TE200}/dubhe_alt/src/aes.c
        ${TE200}/dubhe_alt/src/bignum_wrap.c
        ${TE200}/dubhe_alt/src/ccm.c
        ${TE200}/dubhe_alt/src/cmac.c
        ${TE200}/dubhe_alt/src/gcm.c
        ${TE200}/dubhe_alt/src/otp.c
        ${TE200}/dubhe_alt/src/sha256.c
)

target_include_directories(crypto_service_crypto_hw
    PRIVATE
        ${TE200}/dubhe_alt/inc
        ${IDK_PATH}/components/tinysoc/${TFM_SOC}/config
        ${CMAKE_SOURCE_DIR}/interface/include
        ${CMAKE_BINARY_DIR}/generated/interface/include
)

target_sources(crypto_service_crypto_hw
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

target_sources(crypto_service_crypto_hw
    PRIVATE
		${TE200}/spe/src/misc.c
		${TE200}/crypto_hw.c
		${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_aes.c
		${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_rsa.c
		${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_hash.c
		${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_ecdsa.c
)

target_include_directories(crypto_service_crypto_hw
    PRIVATE
        ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}
        ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}/cmsis_core
        ${TE200}/spe/inc
        ${TE200}/spe/inc/hal
        ${TE200}/spe/inc/pal
)

target_include_directories(crypto_service_crypto_hw
    PUBLIC
        ${TE200}/config
        ${PLATFORM_DIR}/include
        ${MBEDCRYPTO_PATH}/library
        ${MBEDCRYPTO_PATH}/include
        ${CMAKE_SOURCE_DIR}/../../cmsis/CMSIS_5/Device/ArmChina/STAR/Include
        ${PLATFORM_DIR}/../secure_fw/spm/include
        ${PLATFORM_DIR}/ext/common
)

target_link_libraries(crypto_service_crypto_hw
    PRIVATE
        crypto_service_mbedcrypto
)


target_compile_options(crypto_service_crypto_hw
    PUBLIC
        ${COMPILER_CP_FLAG}
)

target_compile_definitions(crypto_service_crypto_hw
    PRIVATE
        TEE_M
        ARM_CE_DUBHE
        DUBHE_SECURE
        ARM_CE_DUBHE_SCA
        ARM_CE_DUBHE_TRNG
        ARM_CE_DUBHE_OTP
        ARM_CE_DUBHE_HASH
        ARM_CE_DUBHE_ACA
)

if (NOT BL2)
target_compile_definitions(crypto_service_crypto_hw
    PRIVATE
        TEE_M
        DUBHE_FOR_RUNTIME
)
endif()
endif()

if (CONFIG_BUILD_TFM)
target_include_directories(crypto_service_mbedcrypto
    PUBLIC
        ${TE200}/config
        ${TE200}/dubhe_driver/inc/common
        ${TE200}/dubhe_driver/inc/crypto
        ${TE200}/dubhe_alt/inc
)

target_compile_definitions(crypto_service_mbedcrypto
    PRIVATE
        TEE_M
        ARM_CE_DUBHE
        DUBHE_SECURE
        ARM_CE_DUBHE_SCA
        ARM_CE_DUBHE_TRNG
        ARM_CE_DUBHE_OTP
        ARM_CE_DUBHE_HASH
        ARM_CE_DUBHE_ACA
        DUBHE_FOR_RUNTIME
)
