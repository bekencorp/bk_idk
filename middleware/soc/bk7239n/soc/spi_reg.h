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

#define SPI_R_BASE(_id)          (SOC_SPI_REG_BASE + _id * 0x1010000)

#define SPI_R_GLOBAL_CTRL(_id)   (SPI_R_BASE(_id) + 2 * 0x04)
#define SPI_F_SOFT_RESET         (BIT(0))
#define SPI_F_SOFT_RESET_M       (0x1)
#define SPI_F_SOFT_RESET_V       (0x1)
#define SPI_F_SOFT_RESET_S       (0)

#define SPI_R_CTRL(_id)          (SPI_R_BASE(_id) + 4 * 0x04)

#define SPI_F_CLK_DIV            (BIT(8))
#define SPI_F_CLK_DIV_M          (0xFF)
#define SPI_F_CLK_DIV_V          (0xFF)
#define SPI_F_CLK_DIV_S          (8)

#define SPI_F_MASTER_EN          (BIT(22))
#define SPI_F_MASTER_EN_M        (0x1)
#define SPI_F_MASTER_EN_V        (0x1)
#define SPI_F_MASTER_EN_S        (22)

#define SPI_F_SPI_EN             (BIT(23))
#define SPI_F_SPI_EN_M           (0x1)
#define SPI_F_SPI_EN_V           (0x1)
#define SPI_F_SPI_EN_S           (23)

#define SPI_R_CFG(_id)           (SPI_R_BASE(_id) + 4 * 0x05)

#define SPI_F_TX_TRANS_LEN       (BIT(8))
#define SPI_F_TX_TRANS_LEN_M     (0xFFF)
#define SPI_F_TX_TRANS_LEN_V     (0xFFF)
#define SPI_F_TX_TRANS_LEN_S     (8)

#define SPI_F_RX_TRANS_LEN       (BIT(20))
#define SPI_F_RX_TRANS_LEN_M     (0xFFF)
#define SPI_F_RX_TRANS_LEN_V     (0xFFF)
#define SPI_F_RX_TRANS_LEN_S     (20)

#define SPI_F_RX_FIN_INT_EN      (BIT(3))
#define SPI_F_RX_FIN_INT_EN_M    (0x1)
#define SPI_F_RX_FIN_INT_EN_V    (0x1)
#define SPI_F_RX_FIN_INT_EN_S    (3)

#define SPI_F_TX_FIN_INT_EN      (BIT(2))
#define SPI_F_TX_FIN_INT_EN_M    (0x1)
#define SPI_F_TX_FIN_INT_EN_V    (0x1)
#define SPI_F_TX_FIN_INT_EN_S    (2)

#define SPI_F_RX_EN              (BIT(1))
#define SPI_F_RX_EN_M            (0x1)
#define SPI_F_RX_EN_V            (0x1)
#define SPI_F_RX_EN_S            (1)

#define SPI_F_TX_EN              (BIT(0))
#define SPI_F_TX_EN_M            (0x1)
#define SPI_F_TX_EN_V            (0x1)
#define SPI_F_TX_EN_S            (0)

#define SPI_R_INT_STATUS(_id)    (SPI_R_BASE(_id) + 4 * 0x06)

#define SPI_F_TXFIOF_WR_READY    (BIT(1))
#define SPI_F_TXFIOF_WR_READY_M  (0x1)
#define SPI_F_TXFIOF_WR_READY_V  (0x1)
#define SPI_F_TXFIOF_WR_READY_S  (1)

#define SPI_F_RX_FIOF_RD_READY   (BIT(2))
#define SPI_F_RX_FIOF_RD_READY_M (0x1)
#define SPI_F_RX_FIOF_RD_READY_V (0x1)
#define SPI_F_RX_FIOF_RD_READY_S (2)

#define SPI_F_TXFIFO_INT         (BIT(8))
#define SPI_F_TXFIFO_INT_M       (0x1)
#define SPI_F_TXFIFO_INT_V       (0x1)
#define SPI_F_TXFIFO_INT_S       (8)

#define SPI_F_RXFIFO_INT         (BIT(9))
#define SPI_F_RXFIFO_INT_M       (0x1)
#define SPI_F_RXFIFO_INT_V       (0x1)
#define SPI_F_RXFIFO_INT_S       (9)

#define SPI_F_SLV_RELEASE_INT    (BIT(10))
#define SPI_F_SLV_RELEASE_INT_M  (0x1)
#define SPI_F_SLV_RELEASE_INT_V  (0x1)
#define SPI_F_SLV_RELEASE_INT_S  (10)

#define SPI_F_TXUDF              (BIT(11))
#define SPI_F_TXUDF_M            (0x1)
#define SPI_F_TXUDF_V            (0x1)
#define SPI_F_TXUDF_S            (11)

#define SPI_F_RXOVF              (BIT(12))
#define SPI_F_RXOVF_M            (0x1)
#define SPI_F_RXOVF_V            (0x1)
#define SPI_F_RXOVF_S            (12)

#define SPI_F_TX_FINISH_INT      (BIT(13))
#define SPI_F_TX_FINISH_INT_M    (0x1)
#define SPI_F_TX_FINISH_INT_V    (0x1)
#define SPI_F_TX_FINISH_INT_S    (13)

#define SPI_F_RX_FINISH_INT      (BIT(14))
#define SPI_F_RX_FINISH_INT_M    (0x1)
#define SPI_F_RX_FINISH_INT_V    (0x1)
#define SPI_F_RX_FINISH_INT_S    (14)

#define SPI_F_TXFIFO_CLR         (BIT(16))
#define SPI_F_TXFIFO_CLR_M       (0x1)
#define SPI_F_TXFIFO_CLR_V       (0x1)
#define SPI_F_TXFIFO_CLR_S       (16)

#define SPI_F_RXFIFO_CLR         (BIT(17))
#define SPI_F_RXFIFO_CLR_M       (0x1)
#define SPI_F_RXFIFO_CLR_V       (0x1)
#define SPI_F_RXFIFO_CLR_S       (17)

#define SPI_R_DATA(_id)          (SPI_R_BASE(_id) + 4 * 0x07)

#ifdef __cplusplus
}
#endif

