/**
* @if copyright_display
 *      Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 *      All rights reserved
 *
 *      The content of this file or document is CONFIDENTIAL and PROPRIETARY
 *      to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 *      License Agreement between Licensee and Arm Technology (China) Co., Ltd
 *      restricting among other things, the use, reproduction, distribution
 *      and transfer.  Each of the embodiments, including this information and
 *      any derivative work shall retain this copyright notice.
* @endif
 */

#ifndef AES_ALT_H
#define AES_ALT_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_AES_ALT)
#include "dubhe_sca.h"
typedef arm_ce_sca_context_t mbedtls_aes_context;

#ifndef MBEDTLS_PRIVATE
#define MBEDTLS_PRIVATE(member) member
#endif

typedef struct mbedtls_aes_xts_context
{
    mbedtls_aes_context MBEDTLS_PRIVATE(crypt); /*!< The AES context to use for AES block
                                        encryption or decryption. */
    mbedtls_aes_context MBEDTLS_PRIVATE(tweak); /*!< The AES context used for tweak
                                        computation. */
} mbedtls_aes_xts_context;

#define MBEDTLS_ERR_AES_INVALID_PARAM                -0x002F
#define MBEDTLS_ERR_AES_GENERIC                      -0x0030

#define MBEDTLS_AES_DERIVED_MODEL_KEY       0
#define MBEDTLS_AES_DERIVED_DEVICE_ROOT_KEY 1

void dubhe_aes_init( mbedtls_aes_context *ctx );
void dubhe_aes_free( mbedtls_aes_context *ctx );
int dubhe_aes_setkey_enc( mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits );
int dubhe_aes_setkey_dec( mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits );
int dubhe_aes_crypt_ecb( mbedtls_aes_context *ctx, int mode, const unsigned char input[16], unsigned char output[16] );
int dubhe_aes_crypt_cbc( mbedtls_aes_context *ctx, int mode, size_t length, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int dubhe_aes_crypt_cfb128( mbedtls_aes_context *ctx, int mode, size_t length, size_t *iv_off, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int dubhe_aes_crypt_cfb8( mbedtls_aes_context *ctx, int mode, size_t length, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int dubhe_aes_crypt_ctr( mbedtls_aes_context *ctx, size_t length, size_t *nc_off, unsigned char nonce_counter[16], unsigned char stream_block[16], const unsigned char *input, unsigned char *output );
int dubhe_aes_crypt_ofb( mbedtls_aes_context *ctx, size_t length, size_t *iv_off, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int dubhe_internal_aes_encrypt( mbedtls_aes_context *ctx, const unsigned char input[16], unsigned char output[16] );
int dubhe_internal_aes_decrypt( mbedtls_aes_context *ctx, const unsigned char input[16], unsigned char output[16] );
int dubhe_aes_set_derived_key( mbedtls_aes_context *ctx, int key_type, int mode, const unsigned char *ek1, const unsigned char *ek2, const unsigned char *ek3, unsigned int ek1bits );

#if defined(MBEDTLS_CIPHER_MODE_XTS)
void dubhe_aes_xts_init( mbedtls_aes_xts_context *ctx );
void dubhe_aes_xts_free( mbedtls_aes_xts_context *ctx );
int dubhe_aes_xts_setkey_enc( mbedtls_aes_xts_context *ctx, const unsigned char *key, unsigned int keybits );
int dubhe_aes_xts_setkey_dec( mbedtls_aes_xts_context *ctx, const unsigned char *key, unsigned int keybits );
int dubhe_aes_crypt_xts( mbedtls_aes_xts_context *ctx, int mode, size_t length, const unsigned char data_unit[16], const unsigned char *input, unsigned char *output );
#endif


void mbedtls_aes_init( mbedtls_aes_context *ctx );
void mbedtls_aes_free( mbedtls_aes_context *ctx );
int mbedtls_aes_setkey_enc( mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits );
int mbedtls_aes_setkey_dec( mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits );
int mbedtls_aes_crypt_ecb( mbedtls_aes_context *ctx, int mode, const unsigned char input[16], unsigned char output[16] );
int mbedtls_aes_crypt_cbc( mbedtls_aes_context *ctx, int mode, size_t length, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int mbedtls_aes_crypt_cfb128( mbedtls_aes_context *ctx, int mode, size_t length, size_t *iv_off, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int mbedtls_aes_crypt_cfb8( mbedtls_aes_context *ctx, int mode, size_t length, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int mbedtls_aes_crypt_ctr( mbedtls_aes_context *ctx, size_t length, size_t *nc_off, unsigned char nonce_counter[16], unsigned char stream_block[16], const unsigned char *input, unsigned char *output );
int mbedtls_aes_crypt_ofb( mbedtls_aes_context *ctx, size_t length, size_t *iv_off, unsigned char iv[16], const unsigned char *input, unsigned char *output );
int mbedtls_internal_aes_encrypt( mbedtls_aes_context *ctx, const unsigned char input[16], unsigned char output[16] );
int mbedtls_internal_aes_decrypt( mbedtls_aes_context *ctx, const unsigned char input[16], unsigned char output[16] );
int mbedtls_aes_set_derived_key( mbedtls_aes_context *ctx, int key_type, int mode, const unsigned char *ek1, const unsigned char *ek2, const unsigned char *ek3, unsigned int ek1bits );

#if defined(MBEDTLS_CIPHER_MODE_XTS)
void mbedtls_aes_xts_init( mbedtls_aes_xts_context *ctx );
void mbedtls_aes_xts_free( mbedtls_aes_xts_context *ctx );
int mbedtls_aes_xts_setkey_enc( mbedtls_aes_xts_context *ctx, const unsigned char *key, unsigned int keybits );
int mbedtls_aes_xts_setkey_dec( mbedtls_aes_xts_context *ctx, const unsigned char *key, unsigned int keybits );
int mbedtls_aes_crypt_xts( mbedtls_aes_xts_context *ctx, int mode, size_t length, const unsigned char data_unit[16], const unsigned char *input, unsigned char *output );
#endif

#endif /*end MBEDTLS_AES_ALT */
#ifdef __cplusplus
}
#endif
#endif /* end AES_ALT_H */
