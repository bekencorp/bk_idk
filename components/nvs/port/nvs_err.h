/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 /*
* Change Logs:
* Date			 Author 	  Notes
* 2024-04-11	 Beken	  adapter to Beken sdk
*/
#pragma once

#include <common/bk_err.h>

/* Definitions for error constants. */
#define ESP_OK          0       /*!< bk_err_t value indicating success (no error) */
#define ESP_FAIL        -1      /*!< Generic bk_err_t code indicating failure */

#define ESP_ERR_NO_MEM              0x101   /*!< Out of memory */
#define ESP_ERR_INVALID_ARG         0x102   /*!< Invalid argument */
#define ESP_ERR_INVALID_STATE       0x103   /*!< Invalid state */
#define ESP_ERR_INVALID_SIZE        0x104   /*!< Invalid size */
#define ESP_ERR_NOT_FOUND           0x105   /*!< Requested resource not found */
#define ESP_ERR_NOT_SUPPORTED       0x106   /*!< Operation or feature not supported */
#define ESP_ERR_TIMEOUT             0x107   /*!< Operation timed out */
#define ESP_ERR_INVALID_RESPONSE    0x108   /*!< Received response was invalid */
#define ESP_ERR_INVALID_CRC         0x109   /*!< CRC or checksum was invalid */
#define ESP_ERR_INVALID_VERSION     0x10A   /*!< Version was invalid */
#define ESP_ERR_INVALID_MAC         0x10B   /*!< MAC address was invalid */
#define ESP_ERR_NOT_FINISHED        0x10C   /*!< Operation has not fully completed */
#define ESP_ERR_NOT_ALLOWED         0x10D   /*!< Operation is not allowed */

#define ESP_ERR_WIFI_BASE           0x3000  /*!< Starting number of WiFi error codes */
#define ESP_ERR_MESH_BASE           0x4000  /*!< Starting number of MESH error codes */
#define ESP_ERR_FLASH_BASE          0x6000  /*!< Starting number of flash error codes */
#define ESP_ERR_HW_CRYPTO_BASE      0xc000  /*!< Starting number of HW cryptography module error codes */
#define ESP_ERR_MEMPROT_BASE        0xd000  /*!< Starting number of Memory Protection API error codes */

#define ESP_ERR_FLASH_OP_FAIL    (ESP_ERR_FLASH_BASE + 1)
#define ESP_ERR_FLASH_OP_TIMEOUT (ESP_ERR_FLASH_BASE + 2)

