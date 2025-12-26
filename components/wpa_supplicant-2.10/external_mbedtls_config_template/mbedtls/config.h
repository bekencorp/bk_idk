/**
 * @file config.h
 * @brief Template mbedtls config.h for external mbedtls integration
 *
 * Copy this file to your external mbedtls config directory and modify as needed.
 * Set CONFIG_EXTERNAL_MBEDTLS_CONFIG_PATH to the directory containing this file.
 *
 * This template is designed for mbedtls 3.x compatibility with wpa_supplicant.
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/*===========================================================================*/
/* mbedtls 3.x compatibility settings                                       */
/*===========================================================================*/

/* Use legacy ECDH context for wpa_supplicant compatibility */
#define MBEDTLS_ECDH_LEGACY_CONTEXT

/*===========================================================================*/
/* System support                                                            */
/*===========================================================================*/
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HAVE_TIME
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY

/*===========================================================================*/
/* Basic crypto modules                                                      */
/*===========================================================================*/
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BASE64_C

/*===========================================================================*/
/* Hash algorithms                                                           */
/*===========================================================================*/
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA384_C
#define MBEDTLS_SHA512_C
#define MBEDTLS_RIPEMD160_C

/*===========================================================================*/
/* Symmetric ciphers                                                         */
/*===========================================================================*/
#define MBEDTLS_AES_C
#define MBEDTLS_DES_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CCM_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_MODE_CTR
#define MBEDTLS_CIPHER_MODE_CFB
#define MBEDTLS_CIPHER_MODE_OFB

/*
 * NOTE: The following features require corresponding functions in your mbedtls library.
 * If your external mbedtls lacks these, comment them out and wpa_supplicant will
 * use its own implementations (aes-wrap.c, aes-omac1.c).
 */

/* Uncomment if your mbedtls has mbedtls_cipher_cmac_* functions */
/* #define MBEDTLS_CMAC_C */

/* Uncomment if your mbedtls has mbedtls_nist_kw_* functions */
/* #define MBEDTLS_NIST_KW_C */

/*===========================================================================*/
/* Random number generation                                                  */
/*===========================================================================*/
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_HKDF_C

/*===========================================================================*/
/* ECC - required for WPA3/SAE                                               */
/*===========================================================================*/
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_DP_SECP192R1_ENABLED
#define MBEDTLS_ECP_DP_SECP224R1_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_SECP521R1_ENABLED
#define MBEDTLS_ECP_DP_BP256R1_ENABLED
#define MBEDTLS_ECP_DP_BP384R1_ENABLED
#define MBEDTLS_ECP_DP_BP512R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED
#define MBEDTLS_ECP_DP_CURVE448_ENABLED

/*===========================================================================*/
/* Public key                                                                */
/*===========================================================================*/
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PKCS1_V21
#define MBEDTLS_DHM_C
#define MBEDTLS_PKCS5_C
#define MBEDTLS_PKCS12_C

/*===========================================================================*/
/* PEM support                                                               */
/*===========================================================================*/
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_WRITE_C

/*===========================================================================*/
/* X.509                                                                     */
/*===========================================================================*/
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CRL_PARSE_C
#define MBEDTLS_X509_CSR_PARSE_C

/*===========================================================================*/
/* mbedtls 3.x removed features                                              */
/*===========================================================================*/

/* ARC4 was removed in mbedtls 3.x - wpa_supplicant uses local rc4.c */
/* #define MBEDTLS_ARC4_C */

/*===========================================================================*/
/* Version info (adjust to match your external mbedtls version)              */
/*===========================================================================*/
#define MBEDTLS_VERSION_MAJOR 3
#define MBEDTLS_VERSION_MINOR 0
#define MBEDTLS_VERSION_PATCH 0
#define MBEDTLS_VERSION_NUMBER 0x03000000
#define MBEDTLS_VERSION_STRING "3.0.0"
#define MBEDTLS_VERSION_STRING_FULL "mbed TLS 3.0.0"

/*===========================================================================*/
/* Memory allocation                                                         */
/*===========================================================================*/
#define MBEDTLS_PLATFORM_CALLOC_MACRO     calloc
#define MBEDTLS_PLATFORM_FREE_MACRO       free

#endif /* MBEDTLS_CONFIG_H */

