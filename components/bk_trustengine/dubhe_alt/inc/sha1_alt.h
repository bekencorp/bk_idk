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
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */
#ifndef _SHA1_ALT_H_
#define _SHA1_ALT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_SHA1_ALT)
#include "dubhe_hash.h"

#define MBEDTLS_ERR_SHA1_HW_ACCEL_FAILED                  -0x0035  /**< SHA-1 hardware accelerator failed */

#if defined(ARM_CE_DUBHE_HASH)
#define MBEDTLS_ERR_SHA1_CTX_NULL       -0x00EC
#define MBEDTLS_ERR_SHA1_INPUT_LEN      -0x00ED
#define MBEDTLS_ERR_SHA1_NOMEM          -0x00EE
#define MBEDTLS_ERR_SHA1_CE_INIT        -0x00EF
#endif

/**
 * \brief          SHA-1 context structure
 */
typedef arm_ce_hash_context_t mbedtls_sha1_context;

void dubhe_sha1_init( mbedtls_sha1_context *ctx );
void dubhe_sha1_free( mbedtls_sha1_context *ctx );
void dubhe_sha1_clone( mbedtls_sha1_context *dst, const mbedtls_sha1_context *src );
int dubhe_sha1_starts_ret( mbedtls_sha1_context *ctx );
int dubhe_sha1_update_ret( mbedtls_sha1_context *ctx, const unsigned char *input, size_t ilen );
int dubhe_sha1_finish_ret( mbedtls_sha1_context *ctx, unsigned char output[20] );
int dubhe_internal_sha1_process( mbedtls_sha1_context *ctx, const unsigned char data[64] );

#if !defined( MBEDTLS_DEPRECATED_REMOVED )
int dubhe_sha1_starts( mbedtls_sha1_context *ctx );
int dubhe_sha1_update( mbedtls_sha1_context *ctx, const unsigned char *input, size_t ilen );
int dubhe_sha1_finish( mbedtls_sha1_context *ctx, unsigned char output[20] );
void dubhe_sha1_process( mbedtls_sha1_context *ctx, const unsigned char data[64] );
#endif

void mbedtls_sha1_init( mbedtls_sha1_context *ctx );
void mbedtls_sha1_free( mbedtls_sha1_context *ctx );
void mbedtls_sha1_clone( mbedtls_sha1_context *dst, const mbedtls_sha1_context *src );
int mbedtls_sha1_starts_ret( mbedtls_sha1_context *ctx );
int mbedtls_sha1_update_ret( mbedtls_sha1_context *ctx, const unsigned char *input, size_t ilen );
int mbedtls_sha1_finish_ret( mbedtls_sha1_context *ctx, unsigned char output[20] );
int mbedtls_internal_sha1_process( mbedtls_sha1_context *ctx, const unsigned char data[64] );

#if !defined( MBEDTLS_DEPRECATED_REMOVED )
int mbedtls_sha1_starts( mbedtls_sha1_context *ctx );
int mbedtls_sha1_update( mbedtls_sha1_context *ctx, const unsigned char *input, size_t ilen );
int mbedtls_sha1_finish( mbedtls_sha1_context *ctx, unsigned char output[20] );
void mbedtls_sha1_process( mbedtls_sha1_context *ctx, const unsigned char data[64] );
#endif

#endif /*MBEDTLS_SHA1_ALT */

#ifdef __cplusplus
}
#endif

#endif /*_SHA1_ALT_H_*/