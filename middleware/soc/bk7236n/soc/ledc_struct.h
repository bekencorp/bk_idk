// Copyright 2025-2026 Beken
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

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile struct {
	/* REG_0x00 */
	union {
		struct {
			uint32_t led_start:      1; /**< bit[0] start transfer when fifo not empty */
			uint32_t reserved_1_3:   3; /**< bit[1:3] reserved */
			uint32_t fifo_enable:    1; /**< bit[4] enable FIFO write */
			uint32_t reserved_5_7:   3; /**< bit[5:7] reserved */
			uint32_t reserved_8_31: 24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} ctrl;

	/* REG_0x01 */
	union {
		struct {
			uint32_t send_number_l:  8; /**< bit[0:7] send length low byte */
			uint32_t reserved:      24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} send_number_l;

	/* REG_0x02 */
	union {
		struct {
			uint32_t send_number_h:  4; /**< bit[0:3] send length high nibble */
			uint32_t reserved:      28; /**< bit[4:31] reserved */
		};
		uint32_t v;
	} send_number_h;

	/* REG_0x03 */
	union {
		struct {
			uint32_t fifo_full:       1; /**< bit[0] FIFO full flag */
			uint32_t fifo_empty:      1; /**< bit[1] FIFO empty flag */
			uint32_t need_read_flag:  1; /**< bit[2] read request flag */
			uint32_t need_write_flag: 1; /**< bit[3] write request flag */
			uint32_t led_over_int:    1; /**< bit[4] transfer done flag */
			uint32_t need_write_int:  1; /**< bit[5] write interrupt status */
			uint32_t need_read_int:   1; /**< bit[6] read interrupt status */
			uint32_t busy_now:        1; /**< bit[7] busy flag */
			uint32_t reserved:       24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} status;

	/* REG_0x04 */
	union {
		struct {
			uint32_t need_read_thr:  8; /**< bit[0:7] read threshold */
			uint32_t reserved:      24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} need_read_thr;

	/* REG_0x05 */
	union {
		struct {
			uint32_t need_write_thr: 8; /**< bit[0:7] write threshold */
			uint32_t reserved:      24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} need_write_thr;

	/* REG_0x06 */
	union {
		struct {
			uint32_t write_data:     8; /**< bit[0:7] FIFO write data */
			uint32_t reserved:      24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} fifo_wdata;

	/* REG_0x07 */
	union {
		struct {
			uint32_t send_finish_inten: 1; /**< bit[0] send finish interrupt enable */
			uint32_t need_wr_inten:     1; /**< bit[1] write interrupt enable */
			uint32_t need_read_inten:   1; /**< bit[2] read interrupt enable */
			uint32_t reserved:         29; /**< bit[3:31] reserved */
		};
		uint32_t v;
	} int_en;

	/* REG_0x08 */
	union {
		struct {
			uint32_t send_finish_int: 1; /**< bit[0] send finish interrupt, W1C */
			uint32_t need_wr_int:     1; /**< bit[1] write interrupt, W1C */
			uint32_t need_read_int:   1; /**< bit[2] read interrupt, W1C */
			uint32_t reserved:         29; /**< bit[3:31] reserved */
		};
		uint32_t v;
	} int_status;

	/* REG_0x09 */
	union {
		struct {
			uint32_t reset_period_number: 8; /**< bit[0:7] reset period */
			uint32_t reserved:             24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} reset_period;

	/* REG_0x0A */
	union {
		struct {
			uint32_t code_period_number: 8; /**< bit[0:7] code bit period */
			uint32_t reserved:            24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} code_period;

	/* REG_0x0B */
	union {
		struct {
			uint32_t t0_high_number: 8; /**< bit[0:7] logic-0 high duration */
			uint32_t reserved:      24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} t0_timing;

	/* REG_0x0C */
	union {
		struct {
			uint32_t t1_high_number: 8; /**< bit[0:7] logic-1 high duration */
			uint32_t reserved:      24; /**< bit[8:31] reserved */
		};
		uint32_t v;
	} t1_timing;
} ledc_hw_t;

#ifdef __cplusplus
}
#endif
