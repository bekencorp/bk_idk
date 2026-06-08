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

#define LEDC_R_BASE                     (SOC_LEDC_REG_BASE)

#define LEDC_R_CTRL                     (LEDC_R_BASE + 4 * 0x0)
#define LEDC_R_SEND_NUMBER_L            (LEDC_R_BASE + 4 * 0x1)
#define LEDC_R_SEND_NUMBER_H            (LEDC_R_BASE + 4 * 0x2)
#define LEDC_R_STATUS                   (LEDC_R_BASE + 4 * 0x3)
#define LEDC_R_NEED_READ_THR            (LEDC_R_BASE + 4 * 0x4)
#define LEDC_R_NEED_WRITE_THR           (LEDC_R_BASE + 4 * 0x5)
#define LEDC_R_FIFO_WDATA               (LEDC_R_BASE + 4 * 0x6)
#define LEDC_R_INT_EN                   (LEDC_R_BASE + 4 * 0x7)
#define LEDC_R_INT_STATUS               (LEDC_R_BASE + 4 * 0x8)
#define LEDC_R_RESET_PERIOD             (LEDC_R_BASE + 4 * 0x9)
#define LEDC_R_CODE_PERIOD              (LEDC_R_BASE + 4 * 0xA)
#define LEDC_R_T0_TIMING                (LEDC_R_BASE + 4 * 0xB)
#define LEDC_R_T1_TIMING                (LEDC_R_BASE + 4 * 0xC)

#define LEDC_F_LED_START                (BIT(0))
#define LEDC_F_LED_START_M              0x1
#define LEDC_F_LED_START_V              0x1
#define LEDC_F_LED_START_S              0

#define LEDC_F_FIFO_ENABLE              (BIT(4))
#define LEDC_F_FIFO_ENABLE_M            0x1
#define LEDC_F_FIFO_ENABLE_V            0x1
#define LEDC_F_FIFO_ENABLE_S            4

#define LEDC_F_SEND_NUMBER_L            (BIT(0))
#define LEDC_F_SEND_NUMBER_L_M          0xFF
#define LEDC_F_SEND_NUMBER_L_V          0xFF
#define LEDC_F_SEND_NUMBER_L_S          0

#define LEDC_F_SEND_NUMBER_H            (BIT(0))
#define LEDC_F_SEND_NUMBER_H_M          0xF
#define LEDC_F_SEND_NUMBER_H_V          0xF
#define LEDC_F_SEND_NUMBER_H_S          0

#define LEDC_F_FIFO_FULL                (BIT(0))
#define LEDC_F_FIFO_FULL_M              0x1
#define LEDC_F_FIFO_FULL_V              0x1
#define LEDC_F_FIFO_FULL_S              0

#define LEDC_F_FIFO_EMPTY               (BIT(1))
#define LEDC_F_FIFO_EMPTY_M             0x1
#define LEDC_F_FIFO_EMPTY_V             0x1
#define LEDC_F_FIFO_EMPTY_S             1

#define LEDC_F_NEED_READ_FLAG           (BIT(2))
#define LEDC_F_NEED_READ_FLAG_M         0x1
#define LEDC_F_NEED_READ_FLAG_V         0x1
#define LEDC_F_NEED_READ_FLAG_S         2

#define LEDC_F_NEED_WRITE_FLAG          (BIT(3))
#define LEDC_F_NEED_WRITE_FLAG_M        0x1
#define LEDC_F_NEED_WRITE_FLAG_V        0x1
#define LEDC_F_NEED_WRITE_FLAG_S        3

#define LEDC_F_LED_OVER_INT             (BIT(4))
#define LEDC_F_LED_OVER_INT_M           0x1
#define LEDC_F_LED_OVER_INT_V           0x1
#define LEDC_F_LED_OVER_INT_S           4

#define LEDC_F_NEED_WRITE_INT           (BIT(5))
#define LEDC_F_NEED_WRITE_INT_M         0x1
#define LEDC_F_NEED_WRITE_INT_V         0x1
#define LEDC_F_NEED_WRITE_INT_S         5

#define LEDC_F_NEED_READ_INT_STATUS     (BIT(6))
#define LEDC_F_NEED_READ_INT_STATUS_M   0x1
#define LEDC_F_NEED_READ_INT_STATUS_V   0x1
#define LEDC_F_NEED_READ_INT_STATUS_S   6

#define LEDC_F_BUSY_NOW                 (BIT(7))
#define LEDC_F_BUSY_NOW_M               0x1
#define LEDC_F_BUSY_NOW_V               0x1
#define LEDC_F_BUSY_NOW_S               7

#define LEDC_F_SEND_FINISH_INTEN        (BIT(0))
#define LEDC_F_SEND_FINISH_INTEN_M      0x1
#define LEDC_F_SEND_FINISH_INTEN_V      0x1
#define LEDC_F_SEND_FINISH_INTEN_S      0

#define LEDC_F_NEED_WR_INTEN            (BIT(1))
#define LEDC_F_NEED_WR_INTEN_M          0x1
#define LEDC_F_NEED_WR_INTEN_V          0x1
#define LEDC_F_NEED_WR_INTEN_S          1

#define LEDC_F_NEED_READ_INTEN          (BIT(2))
#define LEDC_F_NEED_READ_INTEN_M        0x1
#define LEDC_F_NEED_READ_INTEN_V        0x1
#define LEDC_F_NEED_READ_INTEN_S        2

#define LEDC_F_SEND_FINISH_INT          (BIT(0))
#define LEDC_F_SEND_FINISH_INT_M        0x1
#define LEDC_F_SEND_FINISH_INT_V        0x1
#define LEDC_F_SEND_FINISH_INT_S        0

#define LEDC_F_NEED_WR_INT              (BIT(1))
#define LEDC_F_NEED_WR_INT_M            0x1
#define LEDC_F_NEED_WR_INT_V            0x1
#define LEDC_F_NEED_WR_INT_S            1

#define LEDC_F_NEED_READ_INT            (BIT(2))
#define LEDC_F_NEED_READ_INT_M          0x1
#define LEDC_F_NEED_READ_INT_V          0x1
#define LEDC_F_NEED_READ_INT_S          2

/*
 * Default timing values (clk_period = 32ns).
 * REG_0x09 reset_period_number: reset line low duration, n * 64 * 32ns
 * REG_0x0A code_period_number:  logic-0/1 low duration (T0L/T1L), n * 32ns
 * REG_0x0B t0_high_number:      logic-0 high duration (T0H), n * 32ns
 * REG_0x0C t1_high_number:      logic-1 high duration (T1H), n * 32ns
 * REG_0x05 need_write_thr:      assert need_write interrupt when FIFO free space >= n bytes
 */
#define LEDC_V_RESET_PERIOD_DEFAULT     0x0a
#define LEDC_V_CODE_PERIOD_DEFAULT      0x14
#define LEDC_V_T0_HIGH_DEFAULT          0x05
#define LEDC_V_T1_HIGH_DEFAULT          0x0f
#define LEDC_V_NEED_WRITE_THR_DEFAULT   50

/* WS2812 timing (clk_period = 32ns), from ledc reference design */
#define LEDC_V_WS2812_RESET_PERIOD      0x48  /* reset: n * 64 * 32ns (~147us) */
#define LEDC_V_WS2812_CODE_PERIOD       0x27  /* T0L/T1L: n * 32ns (~1.25us) */
#define LEDC_V_WS2812_T0_HIGH           0x0a  /* T0H: n * 32ns (320ns) */
#define LEDC_V_WS2812_T1_HIGH           0x1d  /* T1H: n * 32ns (928ns) */
#define LEDC_V_WS2812_WRITE_THR         20    /* need_write interrupt threshold (bytes) */

#ifdef __cplusplus
}
#endif
