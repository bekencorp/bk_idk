set(incs)
set(srcs)

if (CONFIG_PSA_MBEDTLS)
include(sw.cmake)
include(hw.cmake)
include(tfm.cmake)
include(cust.cmake)

list(APPEND incs
	# include te/cust/port dir first
	${hw_incs}
	${cust_incs}
	mbedtls_port/inc
	mbedtls_port/mbedtls/include
	mbedtls_port/mbedtls/library
	mbedtls_port/configs
	mbedtls_ui

	mbedtls/include
	mbedtls/library
	mbedtls/tests/src
	mbedtls/tests/include
)

list(APPEND srcs
	${sw_srcs}
	${hw_srcs}
	${tfm_srcs}
	${cust_srcs}

	mbedtls_port/src/tls_hardware.c
	mbedtls_port/src/tls_mem.c
	mbedtls_port/src/tls_connect.c
	mbedtls_port/src/tls_certificate.c
	mbedtls_port/mbedtls/library/net_sockets.c
	mbedtls_port/mbedtls/library/platform.c
	mbedtls_port/mbedtls/library/threading.c
	mbedtls_port/mbedtls/programs/test/selftest.c
	mbedtls_ui/sl_tls.c

	mbedtls/tests/src/certs.c

	# src_x509
	mbedtls/library/pkcs7.c
	mbedtls/library/x509.c
	mbedtls/library/x509_create.c
	mbedtls/library/x509_crl.c
	mbedtls/library/x509_crt.c
	mbedtls/library/x509_csr.c
	mbedtls/library/x509write.c
	mbedtls/library/x509write_crt.c
	mbedtls/library/x509write_csr.c

	# src_tls
	mbedtls/library/debug.c
	mbedtls/library/mps_reader.c
	mbedtls/library/mps_trace.c
	mbedtls/library/ssl_cache.c
	mbedtls/library/ssl_ciphersuites.c
	mbedtls/library/ssl_client.c
	mbedtls/library/ssl_cookie.c
	mbedtls/library/ssl_debug_helpers_generated.c
	mbedtls/library/ssl_msg.c
	mbedtls/library/ssl_ticket.c
	mbedtls/library/ssl_tls.c
	mbedtls/library/ssl_tls12_client.c
	mbedtls/library/ssl_tls12_server.c
	mbedtls/library/ssl_tls13_keys.c
	mbedtls/library/ssl_tls13_server.c
	mbedtls/library/ssl_tls13_client.c
	mbedtls/library/ssl_tls13_generic.c

	# src_crypto
	mbedtls/library/aes.c
	mbedtls/library/aesni.c
	mbedtls/library/aria.c
	mbedtls/library/asn1parse.c
	mbedtls/library/asn1write.c
	mbedtls/library/base64.c
	mbedtls/library/bignum_core.c
	mbedtls/library/camellia.c
	mbedtls/library/ccm.c
	mbedtls/library/chacha20.c
	mbedtls/library/chachapoly.c
	mbedtls/library/constant_time.c
	mbedtls/library/des.c
	mbedtls/library/ecjpake.c
	mbedtls/library/ecp_curves_new.c
	mbedtls/library/entropy.c
	mbedtls/library/error.c
	mbedtls/library/gcm.c
	mbedtls/library/hkdf.c
	mbedtls/library/hmac_drbg.c
	mbedtls/library/md5.c
	mbedtls/library/nist_kw.c
	mbedtls/library/ripemd160.c
	mbedtls/library/rsa_alt_helpers.c
	mbedtls/library/sha1.c
	mbedtls/library/sha256.c
	mbedtls/library/sha512.c
	mbedtls/library/sha3.c
	mbedtls/library/pem.c
	mbedtls/library/pk.c
	mbedtls/library/pk_wrap.c
	mbedtls/library/pkcs12.c
	mbedtls/library/pkcs5.c
	mbedtls/library/pkwrite.c
	mbedtls/library/platform_util.c
	mbedtls/library/poly1305.c
	mbedtls/library/psa_util.c
	mbedtls/library/cmac.c
	mbedtls/library/psa_crypto_cipher.c
	mbedtls/library/psa_crypto_slot_management.c
	mbedtls/library/psa_crypto_rsa.c
	mbedtls/library/psa_crypto_ffdh.c
	mbedtls/library/psa_crypto_ecp.c
	mbedtls/library/psa_crypto_driver_wrappers_no_static.c
	mbedtls/library/psa_crypto_hash.c
	mbedtls/library/psa_crypto_aead.c
	mbedtls/library/psa_crypto_mac.c
	mbedtls/library/psa_crypto_pake.c
	mbedtls/library/psa_crypto_se.c
	mbedtls/library/psa_crypto_storage.c
	mbedtls/library/version.c
)

if (NOT (CONFIG_TRUSTENGINE OR CONFIG_SW_CRYPTO))
	message("use software crypto by default")
	list(APPEND srcs
		mbedtls_port/mbedtls/library/ecp_curves.c
		mbedtls/library/bignum.c
		mbedtls/library/cipher.c
		mbedtls/library/cipher_wrap.c
		mbedtls/library/ctr_drbg.c
		mbedtls/library/dhm.c
		mbedtls/library/ecdh.c
		mbedtls/library/ecdsa.c
		mbedtls/library/ecp.c
		mbedtls/library/entropy_poll.c
		mbedtls/library/md.c
		mbedtls/library/oid.c
		mbedtls/library/rsa.c
		mbedtls/library/pkparse.c
		mbedtls/library/psa_crypto.c
		mbedtls/library/psa_crypto_client.c
	)
endif()

if(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
	list(APPEND srcs "esp_crt_bundle/esp_crt_bundle.c")
	list(APPEND incs "esp_crt_bundle/include")
endif()

if (CONFIG_PSA_MBEDTLS_TEST)
	list(APPEND incs
		cli/psa_tests
		#mbedtls/tests/src
		#mbedtls/tests/include
	)

	list(APPEND srcs
		cli/psa_tests/aes_cbc.c
		cli/psa_tests/aes_gcm.c
		cli/psa_tests/cli_crypto.c
		cli/psa_tests/ecdh.c
		cli/psa_tests/ecdsa.c
		cli/psa_tests/hmac.c
		cli/psa_tests/psa_tls_client.c
		cli/psa_tests/sha256.c
		cli/psa_tests/sha384.c
		cli/psa_tests/key_manage.c
		cli/psa_tests/platform_misc.c
	)
endif()

if (CONFIG_TRUSTENGINE_TEST)
	list(APPEND incs
		cli/mbedtls_tests
	)

	list(APPEND srcs
		cli/mbedtls_tests/mbedtls_sha_test.c
		cli/mbedtls_tests/mbedtls_aes_test.c
		cli/mbedtls_tests/cli_mbedtls.c
		cli/mbedtls_tests/mbedtls_ecdsa_test.c
		cli/mbedtls_tests/mbedtls_rsa_test.c
		cli/mbedtls_tests/mbedtls_multi_thread_test.c
		#cli/mbedtls_tests/psa_tls_client.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common tfm lwip_intf_v2_1
)

if (CONFIG_PSA_MBEDTLS)
set(PSA_MBEDTLS_CONFIG_FILE_PATH  "${COMPONENT_DIR}/mbedtls_port/configs/mbedtls_psa_crypto_config.h")
target_compile_definitions(${COMPONENT_LIB}
	PUBLIC
		MBEDTLS_CONFIG_FILE="${PSA_MBEDTLS_CONFIG_FILE_PATH}"
		${hw_defs}
)
endif()

if(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
	set(bundle_name "x509_crt_bundle")
	set(DEFAULT_CRT_DIR ${COMPONENT_DIR}/esp_crt_bundle)

	# Generate custom certificate bundle using the generate_cert_bundle utility
	set(GENERATE_CERT_BUNDLEPY ${python} ${COMPONENT_DIR}/esp_crt_bundle/gen_crt_bundle.py)

	if(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL)
		list(APPEND crt_paths ${DEFAULT_CRT_DIR}/cacrt_all.pem ${DEFAULT_CRT_DIR}/cacrt_local.pem)
	elseif(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN)
		list(APPEND crt_paths ${DEFAULT_CRT_DIR}/cacrt_all.pem ${DEFAULT_CRT_DIR}/cacrt_local.pem)
		list(APPEND args --filter ${DEFAULT_CRT_DIR}/cmn_crt_authorities.csv)
	endif()

	# Add deprecated root certs if enabled. This config is not visible if the default cert
	# bundle is not selected
	if(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEPRECATED_LIST)
		list(APPEND crt_paths ${DEFAULT_CRT_DIR}/cacrt_deprecated.pem)
	endif()

	if(CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE)
		get_filename_component(custom_bundle_path
		${CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE_PATH} ABSOLUTE BASE_DIR "${project_dir}")
		list(APPEND crt_paths ${custom_bundle_path})

	endif()
	list(APPEND args --input ${crt_paths} -q)

	get_filename_component(crt_bundle
		${bundle_name}
		ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

	# Generate bundle according to config
	add_custom_command(OUTPUT ${crt_bundle}
		COMMAND ${GENERATE_CERT_BUNDLEPY} ${args}
		DEPENDS ${custom_bundle_path}
		VERBATIM)

	add_custom_target(custom_bundle DEPENDS ${cert_bundle})
	add_dependencies(${COMPONENT_LIB} custom_bundle)


		target_add_binary_data(${COMPONENT_LIB} ${crt_bundle} BINARY)
	set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		APPEND PROPERTY ADDITIONAL_CLEAN_FILES
		"${crt_bundle}")
endif()