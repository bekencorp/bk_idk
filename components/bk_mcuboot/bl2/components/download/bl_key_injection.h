/*
 * bl_key_injection.h
 *
 *  Created on: 2024-10-12
 *      Author: beken-sw-security
 */

#ifndef BL_KEY_INJECTION_H_
#define BL_KEY_INJECTION_H_

#include <stdint.h>
#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

    uint8_t Transmit_key_injection(uint8_t *rx_param, u16 len);

#ifdef __cplusplus
}
#endif

#endif /* BL_KEY_INJECTION_H_ */
