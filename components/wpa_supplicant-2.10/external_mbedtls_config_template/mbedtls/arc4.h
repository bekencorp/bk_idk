/**
 * @file arc4.h
 * @brief Stub header for mbedtls 3.x compatibility
 *
 * ARC4 was removed in mbedtls 3.x. This stub header ensures compilation
 * succeeds. wpa_supplicant will use its local rc4.c implementation
 * when MBEDTLS_ARC4_C is not defined.
 */

#ifndef MBEDTLS_ARC4_H
#define MBEDTLS_ARC4_H

/* ARC4 is not available in mbedtls 3.x */
/* wpa_supplicant will use its local rc4.c implementation */

#endif /* MBEDTLS_ARC4_H */

