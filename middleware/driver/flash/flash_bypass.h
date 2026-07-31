// Copyright 2020-2026 Beken
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

#include <components/log.h>
#include "sys_reg.h"

#define FLASH_BYPASS_TAG "flash_bypass"
#define FLASH_BYPASS_LOGI(...) BK_LOGI(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_LOGW(...) BK_LOGW(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_LOGE(...) BK_LOGE(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_LOGD(...) BK_LOGD(FLASH_BYPASS_TAG, ##__VA_ARGS__)

#define FLASH_BYPASS_RAWI(...) BK_RAW_LOGI(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_RAWW(...) BK_RAW_LOGW(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_RAWE(...) BK_RAW_LOGE(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_RAWD(...) BK_RAW_LOGD(FLASH_BYPASS_TAG, ##__VA_ARGS__)

#define CURRENT_ROUTINE_TEXT_SIZE       (0x400)

/*Write Enable for Status Register*/
#define FLASH_CMD_WR_EN_SR              (0x6)
#define FLASH_CMD_WR_DISABLE            (0x04)

/*Write Status Register*/
#define FLASH_CMD_WR_SR                 (0x31)

/*Read Status Register*/
#define FLASH_CMD_RD_SR                 (0x05)

/*Others*/
#define FLASH_CMD_QUAD_IO_FAST_READ     (0xEB)
#define FLASH_GD25Q32C_SR_QUAD_EN       (0x2)
#define FLASH_CMD_ENTER_DEEP_PWR_DW     (0xB9)
#define FLASH_CMD_EXIT_DEEP_PWR_DW      (0xAB)
#define FLASH_SR_WIP_MASK               (0x01)
#define FLASH_CMD_READ_UID              (0x4B)
#define FLASH_P25Q64SU_PAGE_ERASE       (0x81)
#define FLASH_DUMMY_BYTE                (0)

/*SYS regs define*/
#define FLASH_BYPASS_SPI0_CKEN_BIT      (SYS_CPU_DEVICE_CLK_ENABLE_SPI0_CKEN_MASK << SYS_CPU_DEVICE_CLK_ENABLE_SPI0_CKEN_POS)
#define FLASH_BYPASS_SPI0_CLKSEL_MASK   (SYS_CPU_26M_WDT_CLK_DIV_CLKSEL_SPI0_MASK << SYS_CPU_26M_WDT_CLK_DIV_CLKSEL_SPI0_POS)
#define FLASH_BYPASS_FLASH_SEL_BIT      (SYS_CPU_STORAGE_CONNECT_OP_SELECT_FLASH_SEL_MASK << SYS_CPU_STORAGE_CONNECT_OP_SELECT_FLASH_SEL_POS)

/*SPI0 regs define*/
#define SPI_CFG_TRX_LEN_MASK            (SPI_F_TX_TRANS_LEN_M)
#define SPI_CFG_TX_TRAHS_LEN_POSI       (SPI_F_TX_TRANS_LEN_S)
#define SPI_CFG_RX_TRAHS_LEN_POSI       (SPI_F_RX_TRANS_LEN_S)
#define SPI_CFG_RX_FIN_INT_EN           (SPI_F_RX_FIN_INT_EN)
#define SPI_CFG_TX_FIN_INT_EN           (SPI_F_TX_FIN_INT_EN)
#define SPI_CFG_RX_EN                   (SPI_F_RX_EN)
#define SPI_CFG_TX_EN                   (SPI_F_TX_EN)
#define SPI_CFG_TX_EN_ONE_BYTE          (((1) << SPI_CFG_TX_TRAHS_LEN_POSI) | SPI_CFG_RX_FIN_INT_EN | SPI_CFG_TX_FIN_INT_EN | SPI_CFG_TX_EN)
#define SPI_CFG_TX_EN_TWO_BYTE          (((2) << SPI_CFG_TX_TRAHS_LEN_POSI) | SPI_CFG_RX_FIN_INT_EN | SPI_CFG_TX_FIN_INT_EN | SPI_CFG_TX_EN)
#define SPI_STATUS_TXFIFO_INT           (SPI_F_TXFIFO_INT)
#define SPI_STATUS_RXFIFO_INT           (SPI_F_RXFIFO_INT)
#define SPI_STATUS_SLV_RELEASE_INT      (SPI_F_SLV_RELEASE_INT)
#define SPI_STATUS_TXUDF                (SPI_F_TXUDF)
#define SPI_STATUS_RXOVF                (SPI_F_RXOVF)
#define SPI_STATUS_TX_FINISH_INT        (SPI_F_TX_FINISH_INT)
#define SPI_STATUS_RX_FINISH_INT        (SPI_F_RX_FINISH_INT)
#define SPI_STATUS_TXFIFO_WR_READY      (SPI_F_TXFIOF_WR_READY)
#define SPI_STATUS_RXFIFO_RD_READY      (SPI_F_RX_FIOF_RD_READY)
#define SPI_STATUS_IRQ_BUSY_MASK        (SPI_STATUS_TXFIFO_INT      | \
                                         SPI_STATUS_RXFIFO_INT      | \
                                         SPI_STATUS_SLV_RELEASE_INT | \
                                         SPI_STATUS_TXUDF           | \
                                         SPI_STATUS_RXOVF           | \
                                         SPI_STATUS_TX_FINISH_INT   | \
                                         SPI_STATUS_RX_FINISH_INT)

#define FLASH_ID_GD25Q32C               (0xC84016)
#define FLASH_ID_TH25Q64                (0xCD6017)
#define QE_RETRY_TIMES                  (10)

#if CONFIG_SOC_BK7236XX
__attribute__((section(".iram"))) int flash_bypass_op_write(uint8_t *op_code, uint8_t *tx_buf, uint32_t tx_len);
__attribute__((section(".iram"))) int flash_bypass_op_write_iram(uint8_t *op_code, uint8_t *tx_buf, uint32_t tx_len);
#else
__attribute__((section(".itcm_sec_code"))) void flash_bypass_quad_enable(void);
__attribute__((section(".itcm_sec_code"))) void flash_bypass_quad_test(uint32_t quad_enable, uint32_t delay_cycle1, uint32_t delay_cycle2);
#endif

#if CONFIG_FLASH_BYPASS_OTP_V2
/* These functions must be defined in ITCM or IRAM section.
 * They are actually defined in flash_bypass.c with .iram section. */
__attribute__((section(".iram"))) int flash_bypass_op_read(uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_len);
__attribute__((section(".iram"))) int flash_bypass_op_write(uint8_t *op_code, uint8_t *tx_buf, uint32_t tx_len);
__attribute__((section(".iram"))) int flash_bypass_op_write_combin(uint8_t *op_code, uint8_t *tx_buf1, uint32_t tx_len1, uint8_t *tx_buf2, uint32_t tx_len2);
#endif

#if CONFIG_FLASH_BYPASS_PAGE_ERASE
__attribute__((section(".iram"))) bk_err_t flash_bypass_page_erase(uint32_t address);
#endif

#if CONFIG_FLASH_BYPASS_READ_UID
__attribute__((section(".iram"))) bk_err_t flash_bypass_read_uid(uint8_t *buf, uint32_t len);
#endif

#if CONFIG_FLASH_BYPASS_OTP_OPERATION
#define FLASH_BYPASS_OTP_BLOCK_START_OFFSET    (0)
#define FLASH_BYPASS_OTP_BLOCK_LENGTH          (1024)

#define FLASH_BYPASS_OTP_PAGE_LENGTH           (256)

#define FLASH_BYPASS_OTP_BLOCK_NUM             (3)

#define FLASH_BYPASS_OTP_IDX_X_ADDR(x)         ((x) << 12)

#define CMD_FLASH_BYPASS_WR_EN                 (0x06)
#define CMD_FLASH_BYPASS_WR_DI                 (0x04)

#define CMD_FLASH_BYPASS_OTP_READ_TASK_LEN     (5)
#define CMD_FLASH_BYPASS_OTP_READ              (0x48)

#define CMD_FLASH_BYPASS_OTP_EARSE_TASK_LEN    (4)
#define CMD_FLASH_BYPASS_OTP_EARSE             (0x44)

#define CMD_FLASH_BYPASS_OTP_WRITE_HEAD_LEN    (4)
#define CMD_FLASH_BYPASS_OTP_WRITE_TASK_LEN    (CMD_FLASH_BYPASS_OTP_WRITE_HEAD_LEN + FLASH_BYPASS_OTP_BLOCK_LENGTH)
#define CMD_FLASH_BYPASS_OTP_WRITE             (0x42)

#define CMD_FLASH_STA_REG_1_READ               (0x05)
#define CMD_FLASH_STA_REG_2_READ               (0x35)
#define CMD_FLASH_STA_REG_3_READ               (0x15)
#define CMD_FLASH_STA_REG_1_WRITE              (0x01)
#define CMD_FLASH_STA_REG_2_WRITE              (0x31)
#define CMD_FLASH_STA_REG_3_WRITE              (0x11)

#define CMD_FLASH_STA_REG_READ_TASK_LEN        (1)
#define CMD_FLASH_STA_REG_WRITE_TASK_LEN       (2)

#define FLASH_STA_REG_LOCK_OTP_OFFSET          (10)
#define FLASH_STA_REG_WIP_BIT                  (1 << 0)
#define FLASH_STA_REG_QUAD_ENABLE_BIT          (1 << 9)

#define FLASH_BYPASS_OTP_READ_RETRY_MAX        (3)
#define FLASH_BYPASS_OTP_EARSE_RETRY_MAX       (3)
#define FLASH_BYPASS_OTP_LOCK_RETRY_MAX        (3)

typedef enum
{
	FLASH_BYPASS_OTP_READ = 1,
	FLASH_BYPASS_OTP_EARSE,
	FLASH_BYPASS_OTP_WRITE,
	FLASH_BYPASS_OTP_LOCK,
	FLASH_BYPASS_GET_UID,
} flash_bypass_otp_cmd_t;

typedef enum
{
	FLASH_BYPASS_OTP_IDX_1 = 1,
	FLASH_BYPASS_OTP_IDX_2,
	FLASH_BYPASS_OTP_IDX_3,

	FLASH_BYPASS_OTP_IDX_MAX,
} flash_bypass_otp_idx_t;

typedef enum
{
	FLASH_STA_REG_1 = 0,
	FLASH_STA_REG_2,
	FLASH_STA_REG_3,

	FLASH_STA_REG_MAX,
} flash_bypass_otp_reg_idx_t;

typedef struct {
	flash_bypass_otp_idx_t otp_idx;
	uint32_t               addr_offset;
	uint8_t                *read_buf;
	uint32_t               read_len;
	uint8_t                *write_buf;
	uint32_t               write_len;
} flash_bypass_otp_ctrl_t;

bk_err_t flash_bypass_otp_operation(flash_bypass_otp_cmd_t cmd, flash_bypass_otp_ctrl_t *param);
#endif

#ifdef __cplusplus
}
#endif
// eof
