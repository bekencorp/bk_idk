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
/*
 *  The SHA-256 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

#ifndef _SHA256_ALT_H_
#define _SHA256_ALT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_SHA256_ALT)
#include "dubhe_hash.h"

/**
 * \brief          SHA-1 context structure
 */
typedef arm_ce_hash_context_t mbedtls_sha256_context;

#define MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED                -0x0037  /**< SHA-256 hardware accelerator failed */

#if defined(ARM_CE_DUBHE_HASH)
#define MBEDTLS_ERR_SHA256_INPUT_LEN    -0x00E1
#define MBEDTLS_ERR_SHA256_NOMEM        -0x00E2
#define MBEDTLS_ERR_SHA256_CE_INIT      -0x00E3
#define MBEDTLS_ERR_SHA256_CTX_NULL     -0x00E4
#endif

void dubhe_sha256_init( mbedtls_sha256_context *ctx );
void dubhe_sha256_free( mbedtls_sha256_context *ctx );
void dubhe_sha256_clone( mbedtls_sha256_context *dst, const mbedtls_sha256_context *src );
int dubhe_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 );
int dubhe_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen );
int dubhe_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32] );
int dubhe_internal_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[64] );
int dubhe_internal_sha224_process( mbedtls_sha256_context *ctx, const unsigned char data[64] );

#if !defined( MBEDTLS_DEPRECATED_REMOVED )
int dubhe_sha256_starts( mbedtls_sha256_context *ctx, int is224 );
int dubhe_sha256_update( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen );
int dubhe_sha256_finish( mbedtls_sha256_context *ctx, unsigned char *output );
void dubhe_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[64] );
#endif

void mbedtls_sha256_init( mbedtls_sha256_context *ctx );
void mbedtls_sha256_free( mbedtls_sha256_context *ctx );
void mbedtls_sha256_clone( mbedtls_sha256_context *dst, const mbedtls_sha256_context *src );
int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 );
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen );
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32] );
int mbedtls_internal_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[64] );
int mbedtls_internal_sha224_process( mbedtls_sha256_context *ctx, const unsigned char data[64] );

#if !defined( MBEDTLS_DEPRECATED_REMOVED )
int mbedtls_sha256_starts( mbedtls_sha256_context *ctx, int is224 );
int mbedtls_sha256_update( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen );
int mbedtls_sha256_finish( mbedtls_sha256_context *ctx, unsigned char *output );
void mbedtls_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[64] );
#endif

#endif /*MBEDTLS_SHA256_ALT*/

#ifdef __cplusplus
}
#endif

#endif /*_SHA256_ALT_H_*/