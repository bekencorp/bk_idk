// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t sdio_host_unit_t; /**< sdio host uint id */

#if (CONFIG_SDIO_V2P0)
typedef enum {
	SDIO_HOST_CLK_XTAL_20M = 1,	 /**< approximately 40M/2 division of crystal clock frequency */
	SDIO_HOST_CLK_XTAL_13M = 2,	 /**< approximately 40M/3 division of crystal clock frequency */
	SDIO_HOST_CLK_XTAL_312K = 3,	 /**< approximately 40M/128 division of crystal clock frequency */

	SDIO_HOST_CLK_60M = 8,	 /**< 60M/1 division of clock frequency */
	SDIO_HOST_CLK_30M = 9,	 /**< 60M/2 division of clock frequency */
	SDIO_HOST_CLK_20M = 10,	 /**< 60M/3 division of clock frequency */

	SDIO_HOST_CLK_80M = 12,	 /**< 80M/1 division of clock frequency */
	SDIO_HOST_CLK_40M = 13,	 /**< 80M/2 division of clock frequency */

} sdio_host_clock_freq_t;
#else
typedef enum {
	SDIO_HOST_CLK_26M = 26000000,    /**< 1 division of clock frequency */
	SDIO_HOST_CLK_13M = 13000000,    /**< 2 division of clock frequency */
	SDIO_HOST_CLK_6_5M = 6500000,    /**< 4 division of clock frequency */
	SDIO_HOST_CLK_3_2_5M  = 3250000, /**< 8 division of clock frequency */
	SDIO_HOST_CLK_1_6M = 1600000,    /**< 16 division of clock frequency */
	SDIO_HOST_CLK_800K = 800000,     /**< 32 division of clock frequency */
	SDIO_HOST_CLK_400K = 400000,     /**< 64 division of clock frequency */
	SDIO_HOST_CLK_200K = 200000,     /**< 128 division of clock frequency */
	SDIO_HOST_CLK_100K = 100000,     /**< 256 division of clock frequency */
} sdio_host_clock_freq_t;
#endif

typedef enum {
	SDIO_HOST_CMD_RSP_NONE = 0, /**< sdio host not need slave respond the command */
	SDIO_HOST_CMD_RSP_SHORT,    /**< sdio host need slave long response */
	SDIO_HOST_CMD_RSP_LONG,     /**< sdio host need slave short response */
} sdio_host_cmd_rsp_t;

typedef enum {
	SDIO_HOST_BUS_WIDTH_1LINE = 0, /**< sdio host bus width 1 line */
	SDIO_HOST_BUS_WIDTH_4LINE,     /**< sdio host bus width 4 line */
} sdio_host_bus_width_t;

typedef enum {
	SDIO_HOST_RSP0 = 0, /**< sdio host response regiseter 0 */
	SDIO_HOST_RSP1,     /**< sdio host response regiseter 1 */
	SDIO_HOST_RSP2,     /**< sdio host response regiseter 2 */
	SDIO_HOST_RSP3,     /**< sdio host response regiseter 3 */
} sdio_host_response_t;

typedef struct {
	uint32_t cmd_index;           /**< sdio command index, between 0 and 63 */
	uint32_t argument;            /**< sdio command argument */
	sdio_host_cmd_rsp_t response; /**< sdio command response type */
	uint32_t wait_rsp_timeout;    /**< sdio host wait for slave command response timeout */
	bool crc_check;			      /**< sdio host whether need to check slave response crc value */
} sdio_host_cmd_cfg_t;

#ifdef __cplusplus
}
#endif

