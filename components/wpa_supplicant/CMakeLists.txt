set(incs 
	.
	crypto
	utils)
if(CONFIG_WPA_SUPPLICANT)
set(srcs)
	list(APPEND srcs
		"crypto/aes-siv.c"
		"crypto/aes-unwrap.c"
		"crypto/aes-wrap.c"
		"crypto/dh_group5.c"
		"crypto/dh_groups.c"
		"crypto/sha1-prf.c"
		"crypto/sha256-kdf.c"
		"crypto/sha256-prf.c"
		"crypto/sha384-prf.c"
		"crypto/sha512-prf.c"
		"crypto/sha384-kdf.c"
		"crypto/sha512-kdf.c"
		"crypto/tls_none.c"
		"utils/os_none.c"
	)
	if (CONFIG_USE_MBEDTLS)
		# if mbedtls is enabled, use crypto_mbedtls
		list(APPEND srcs "crypto/crypto_mbedtls.c")
	else()
		# if mbedtls is not enabled, use wpa_supplicant's internal crypto implementation
		list(APPEND srcs
			"crypto/aes-cbc.c"
			"crypto/aes-ctr.c"
			"crypto/aes-internal-dec.c"
			"crypto/aes-internal-enc.c"
			"crypto/aes-internal.c"
			"crypto/aes-omac1.c"
			"crypto/crypto_internal-modexp.c"
			"crypto/md5-internal.c"
			"crypto/md5.c"
			"crypto/rc4.c"
			"crypto/sha1-internal.c"
			"crypto/sha1-pbkdf2.c"
			"crypto/sha1.c"
		)

		# WPA3 will use crypto_wolfssl who has sha256 implemented
		if (NOT CONFIG_WPA3)
		list(APPEND srcs 
			"crypto/sha256-internal.c"
			"crypto/sha256.c"
		)
		endif()
	endif()
endif()
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES mbedtls tfm_mbedtls bk_wifi bk_common lwip_intf_v2_1
	)
