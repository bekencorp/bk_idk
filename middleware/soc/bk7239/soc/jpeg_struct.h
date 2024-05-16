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

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile struct {
	/* REG_0x00 */
	uint32_t dev_id;

	/* REG_0x01 */
	uint32_t dev_version;

	/* REG_0x02 */
	union {
		struct {
			uint32_t soft_reset:      1; /**< bit[0] soft reset */
			uint32_t clk_gate_bypass: 1; /**< bit[1] bypass uart clock gate */
			uint32_t reserved:       30; /**< bit[2:31] reserved */
		};
		uint32_t v;
	} global_ctrl;

	/* REG_0x03 */
	uint32_t dev_status;

	/* REG_0x04 */
	union {
		struct {
			uint32_t reserved:     16; /**< bit[0:15] */
			uint32_t eof_offset:   16; /**< bit[16:31] */
		};
		uint32_t v;
	} eof_offset;

	/* REG_0x05 */
	uint32_t rx_fifo_data;

	/* REG_0x6 */
	union {
		struct {
			uint32_t int_status:     6; /**< bit[0:5] */
			uint32_t fifo_rd_finish: 1; /**< bit[6]*/
			uint32_t reserved:      25; /**< bit[7:31] */
		};
		uint32_t v;
	} int_status;

	/* REG_0x7 */
	uint32_t byte_count_pfrm;

	/* REG_0x8 */
	union {
		struct {
			uint32_t reserved0:          26; /**< bit[0:25] */
			uint32_t stream_fifo_empty:   1; /**< bit[26] */
			uint32_t stream_fifo_full:    1; /**< bit[27] */
			uint32_t is_data_set:         1; /**< bit[28] */
			uint32_t reserved1:           3; /**< bit[29:31] */
		};
		uint32_t v;
	} fifo_status;

	/* REG_0x9 */
	union {
		struct {
			uint32_t reserved0:          24; /**< bit[0:23] */
			uint32_t y_count:             8; /**< bit[24:31] */
		};
		uint32_t v;
	} y_count;

	/* REG_0xa */
	uint32_t byte_count_every_line;

	/* REG_0xb */
	uint32_t reserved;

	/* REG_0xc */
	union {
		struct {
			uint32_t frame_err_int_en:    1; /**< bit[0] */
			uint32_t head_int_en:         1; /**< bit[1] */
			uint32_t sof_int_en:          1; /**< bit[2] */
			uint32_t eof_int_en:          1; /**< bit[3] */
			uint32_t reserved0:           3; /**< bit[4:6] */
			uint32_t line_clear_int_en:   1;/**< bit[7] */
			uint32_t reserved1:          24; /**< bit[8:31] */
		};
		uint32_t v;
	} int_en;

	/* REG_0xd */
	union {
		struct {
			uint32_t reserved0:          1; /**< bit[0] */
			uint32_t video_byte_reverse: 1; /**< bit[1] */
			uint32_t reserved1:          2; /**< bit[2:3] */
			uint32_t jpeg_enc_en:        1; /**< bit[4] */
			uint32_t reserved2:          3; /**< bit[5:7] */
			uint32_t x_pixel:            8; /**< bit[8:15] */
			uint32_t jpeg_enc_size:      1; /**< bit[16] */
			uint32_t bitrate_ctrl:       1; /**< bit[17] */
			uint32_t bitrate_step:       2; /**< bit[18:19] */
			uint32_t auto_step:          1; /**< bit[20] */
			uint32_t reserved3:          2; /**< bit[21:22] */
			uint32_t bitrate_mode:       1; /**< bit[23] */
			uint32_t y_pixel:            8; /**< bit[24:31] */
		};
		uint32_t v;
	} cfg;

	/* REG_0xe */
	uint32_t target_byte_h;

	/* REG_0xf */
	uint32_t target_byte_l;

	/* REG_0x20 - REG_0x3C */
	uint32_t quat_table[0x1d];
} jpeg_hw_t;

#ifdef __cplusplus
}
#endif

