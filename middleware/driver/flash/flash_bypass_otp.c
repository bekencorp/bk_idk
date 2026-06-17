#include <stdbool.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include <common/bk_assert.h>
#include <common/sys_config.h>
#include "os/os.h"
#include "bk_arm_arch.h"
#include "bk_misc.h"
#include "driver/flash.h"
#include "flash_bypass.h"
#include "flash_bypass_otp.h"
#include "sys_driver.h"

#if CONFIG_FLASH_BYPASS_OTP_V2

extern flash_line_mode_t bk_flash_bypass_otp_enter(void);
extern void bk_flash_bypass_otp_exit(flash_line_mode_t old_mode);

#define ID_RETRY_MAX      (3)
#define POLL_DELAY_US     (100)
#define POLL_MAX_LOOP     (2000)
#define FLASH_BYPASS_OTP_SR_CMD_MAX (4)

typedef struct {
    uint8_t cmd_read[FLASH_BYPASS_OTP_SR_CMD_MAX];
    uint8_t cmd_write[FLASH_BYPASS_OTP_SR_CMD_MAX];
    uint8_t sr_bytes;
    uint8_t wip_bit_pos;
    uint8_t wel_bit_pos;
    uint8_t lock_bit_base;
    uint8_t lock_all_together;
} flash_bypass_otp_sr_config_t;

typedef struct {
    uint32_t flash_id;
    uint16_t otp_block_size;
    uint16_t max_write_len;
    uint8_t otp_blocks;
    uint8_t otp_idx_min;
    uint8_t otp_idx_max;
    uint8_t otp_addr_idx_offset;
    uint8_t cmd_read_otp;
    uint8_t cmd_prog_otp;
    uint8_t cmd_erase_otp;
    uint8_t cmd_read_id;
    uint8_t cmd_wr_en;
    flash_bypass_otp_sr_config_t sr_config;
} flash_bypass_otp_config_t;

static const flash_bypass_otp_config_t s_flash_bypass_otp_configs[] = {
    /* TH25Q32UBWFAK */
    { 0xCD6016, 1024,  256, 3, 1, 3, 12, 0x48, 0x42, 0x44, 0x9F, 0x06, {{0x05, 0x35, 0x00, 0x00}, {0x01, 0x00, 0x00, 0x00}, 2, 0, 1, 11, 0} },
    /* GD25WQ32E */
    { 0xC86516, 1024,  256, 3, 1, 3, 12, 0x48, 0x42, 0x44, 0x9F, 0x06, {{0x05, 0x35, 0x15, 0x00}, {0x01, 0x31, 0x11, 0x00}, 3, 0, 1, 11, 0} },
    /* P25Q64SUFWF */
    { 0x856017, 1024, 1024, 3, 1, 3, 12, 0x48, 0x42, 0x44, 0x9F, 0x06, {{0x05, 0x35, 0x15, 0x00}, {0x01, 0x31, 0x11, 0x00}, 3, 0, 1, 11, 0} },
    /* Default fallback */
    { 0x000000, 1024,  256, 1, 1, 1, 12, 0x48, 0x42, 0x44, 0x9F, 0x06, {{0x05, 0x35, 0x00, 0x00}, {0x01, 0x00, 0x00, 0x00}, 2, 0, 1, 11, 0} },
};

static uint32_t s_cached_flash_id = 0;
static flash_bypass_otp_config_t s_cached_config = {0};
static bool s_cached_config_valid = false;
static flash_bypass_otp_err_t flash_bypass_otp_wait_wel(void);
static flash_bypass_otp_err_t flash_bypass_otp_wait_wip(void);

typedef struct {
    uint32_t int_status;
    uint32_t spi_int_was_enabled;
} flash_bypass_otp_irq_guard_t;

static __attribute__((section(".iram"))) flash_bypass_otp_irq_guard_t flash_bypass_otp_irq_guard_enter(void)
{
    flash_bypass_otp_irq_guard_t guard = {0};
    uint32_t spi_int_en_saved = 0;

    guard.int_status = rtos_disable_int();
    spi_int_en_saved = (uint32_t)sys_drv_int_disable(SPI_INTERRUPT_CTRL_BIT);
    guard.spi_int_was_enabled = spi_int_en_saved & SPI_INTERRUPT_CTRL_BIT;

    return guard;
}

static __attribute__((section(".iram"))) void flash_bypass_otp_irq_guard_exit(const flash_bypass_otp_irq_guard_t *guard)
{
    if (guard->spi_int_was_enabled) {
        sys_drv_int_enable(SPI_INTERRUPT_CTRL_BIT);
    }
    rtos_enable_int(guard->int_status);
}

static __attribute__((section(".iram"))) void flash_bypass_otp_read_flash_id(void)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();

    if (s_cached_flash_id) {
        goto exit;
    }

    size_t count = sizeof(s_flash_bypass_otp_configs) / sizeof(s_flash_bypass_otp_configs[0]);
    const flash_bypass_otp_config_t *cfg_default = &s_flash_bypass_otp_configs[count - 1];
    uint8_t tx_buf[1] = {cfg_default->cmd_read_id};
    uint8_t rx_buf[3] = {0};

    for (int retry = 0; retry < ID_RETRY_MAX; retry++) {
        if (flash_bypass_op_read(tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf)) != sizeof(rx_buf)) {
            continue;
        }

        uint32_t id = ((uint32_t)rx_buf[0] << 16) |
                      ((uint32_t)rx_buf[1] << 8)  |
                      ((uint32_t)rx_buf[2]);
        if (id != 0) {
            s_cached_flash_id = id;
            break;
        }
    }

exit:
    flash_bypass_otp_irq_guard_exit(&guard);
}

static __attribute__((section(".iram"))) uint32_t flash_bypass_otp_get_flash_id(void)
{
    flash_bypass_otp_read_flash_id();
    return s_cached_flash_id;
}

static __attribute__((section(".iram"))) void flash_bypass_otp_find_config(void)
{
    if (s_cached_config_valid) {
        return;
    }

    uint32_t flash_id = flash_bypass_otp_get_flash_id();
    size_t count = sizeof(s_flash_bypass_otp_configs) / sizeof(s_flash_bypass_otp_configs[0]);
    s_cached_config = s_flash_bypass_otp_configs[count - 1];

    for (size_t i = 0; i < count; i++) {
        if (s_flash_bypass_otp_configs[i].flash_id == flash_id) {
            s_cached_config = s_flash_bypass_otp_configs[i];
            break;
        }
    }

    s_cached_config_valid = true;
}

static __attribute__((section(".iram"))) const flash_bypass_otp_config_t *flash_bypass_otp_get_config(void)
{
    flash_bypass_otp_find_config();
    return &s_cached_config;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_check_range(uint8_t otp_idx, uint32_t addr_offset, uint32_t len, bool allow_zero_len)
{
    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    if (otp_idx < cfg->otp_idx_min || otp_idx > cfg->otp_idx_max) {
        return FLASH_BYPASS_OTP_ERR_OTP_IDX_OUT_OF_RANGE;
    }

    if (!allow_zero_len && len == 0) {
        return FLASH_BYPASS_OTP_ERR_LEN_ZERO_NOT_ALLOWED;
    }

    if (addr_offset >= cfg->otp_block_size) {
        return FLASH_BYPASS_OTP_ERR_OFFSET_OUT_OF_RANGE;
    }

    if (addr_offset + len > cfg->otp_block_size) {
        return FLASH_BYPASS_OTP_ERR_RANGE_CROSS_BLOCK;
    }

    return FLASH_BYPASS_OTP_OK;
}

static __attribute__((section(".iram"))) uint8_t flash_bypass_otp_get_sr_valid_cmd_count(const uint8_t *cmds)
{
    if (cmds == NULL) {
        return 0;
    }

    for (uint8_t i = 0; i < FLASH_BYPASS_OTP_SR_CMD_MAX; i++) {
        if (cmds[i] == 0) {
            return i;
        }
    }
    return FLASH_BYPASS_OTP_SR_CMD_MAX;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_read_sr(uint32_t *sr_value)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    if (sr_value == NULL) {
        ret = FLASH_BYPASS_OTP_ERR_PARAM;
        goto exit;
    }

    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    uint8_t read_count = flash_bypass_otp_get_sr_valid_cmd_count(cfg->sr_config.cmd_read);
    uint8_t sr_bytes = cfg->sr_config.sr_bytes;
    if (sr_bytes != read_count) {
        ret = FLASH_BYPASS_OTP_ERR_SR_READ_FAIL;
        goto exit;
    }

    uint32_t value = 0;
    for (uint8_t i = 0; i < read_count; i++) {
        uint8_t tx_buf[1] = {cfg->sr_config.cmd_read[i]};
        uint8_t rx_buf[1] = {0};

        if (flash_bypass_op_read(tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf)) != sizeof(rx_buf)) {
            ret = FLASH_BYPASS_OTP_ERR_SR_READ_FAIL;
            goto exit;
        }

        uint32_t shift = (uint32_t)i * 8U;
        value |= ((uint32_t)rx_buf[0]) << shift;
    }

    *sr_value = value;
exit:
    flash_bypass_otp_irq_guard_exit(&guard);
    return ret;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_write_sr(uint32_t sr_value)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    uint8_t write_count = flash_bypass_otp_get_sr_valid_cmd_count(cfg->sr_config.cmd_write);
    uint8_t sr_bytes = cfg->sr_config.sr_bytes;
    uint8_t wr_en = cfg->cmd_wr_en;
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;

    if (write_count == 1) {
        uint8_t tx_buf[FLASH_BYPASS_OTP_SR_CMD_MAX + 1] = {0};

        tx_buf[0] = cfg->sr_config.cmd_write[0];
        for (uint8_t i = 0; i < sr_bytes; i++) {
            uint32_t shift = (uint32_t)i * 8U;
            tx_buf[i + 1] = (uint8_t)(sr_value >> shift);
        }

        if (flash_bypass_op_write(&wr_en, NULL, 0) < 0) {
            ret = FLASH_BYPASS_OTP_ERR_SR_WRITE_FAIL;
            goto exit;
        }

        ret = flash_bypass_otp_wait_wel();
        if (ret != FLASH_BYPASS_OTP_OK) {
            goto exit;
        }

        if (flash_bypass_op_write(NULL, tx_buf, sr_bytes + 1) < 0) {
            ret = FLASH_BYPASS_OTP_ERR_SR_WRITE_FAIL;
            goto exit;
        }

        ret = flash_bypass_otp_wait_wip();
        if (ret != FLASH_BYPASS_OTP_OK) {
            goto exit;
        }

        goto exit;
    }

    if (write_count != sr_bytes) {
        ret = FLASH_BYPASS_OTP_ERR_SR_WRITE_FAIL;
        goto exit;
    }

    for (uint8_t i = 0; i < write_count; i++) {
        uint32_t shift = (uint32_t)i * 8U;
        uint8_t tx_buf[2] = {
            cfg->sr_config.cmd_write[i],
            (uint8_t)(sr_value >> shift)
        };

        if (flash_bypass_op_write(&wr_en, NULL, 0) < 0) {
            ret = FLASH_BYPASS_OTP_ERR_SR_WRITE_FAIL;
            goto exit;
        }

        ret = flash_bypass_otp_wait_wel();
        if (ret != FLASH_BYPASS_OTP_OK) {
            goto exit;
        }

        if (flash_bypass_op_write(NULL, tx_buf, 2) < 0) {
            ret = FLASH_BYPASS_OTP_ERR_SR_WRITE_FAIL;
            goto exit;
        }

        ret = flash_bypass_otp_wait_wip();
        if (ret != FLASH_BYPASS_OTP_OK) {
            goto exit;
        }
    }

exit:
    flash_bypass_otp_irq_guard_exit(&guard);
    return ret;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_wait_wip(void)
{
    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    uint8_t wip_bit_pos = cfg->sr_config.wip_bit_pos;
    uint32_t mask = 1U << wip_bit_pos;

    for (int loop = 0; loop < POLL_MAX_LOOP; loop++) {
        uint32_t sr_value = 0;
        if (flash_bypass_otp_read_sr(&sr_value) != FLASH_BYPASS_OTP_OK) {
            bk_delay_us(POLL_DELAY_US);
            continue;
        }

        if ((sr_value & mask) == 0U) {
            return FLASH_BYPASS_OTP_OK;
        }

        bk_delay_us(POLL_DELAY_US);
    }

    return FLASH_BYPASS_OTP_ERR_WIP_TIMEOUT;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_wait_wel(void)
{
    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    uint8_t wel_bit_pos = cfg->sr_config.wel_bit_pos;
    const uint32_t wel_mask = 1U << wel_bit_pos;

    for (int loop = 0; loop < POLL_MAX_LOOP; loop++) {
        uint32_t sr_value = 0;
        if (flash_bypass_otp_read_sr(&sr_value) != FLASH_BYPASS_OTP_OK) {
            bk_delay_us(POLL_DELAY_US);
            continue;
        }

        if ((sr_value & wel_mask) != 0U) {
            return FLASH_BYPASS_OTP_OK;
        }

        bk_delay_us(POLL_DELAY_US);
    }

    return FLASH_BYPASS_OTP_ERR_WEL_TIMEOUT;
}

static __attribute__((section(".iram"))) uint8_t flash_bypass_otp_get_lock_bit_pos(uint8_t otp_idx)
{
    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    uint8_t lock_bit_base = cfg->sr_config.lock_bit_base;
    if (cfg->sr_config.lock_all_together) {
        return lock_bit_base;
    }

    uint8_t otp_idx_offset = otp_idx - cfg->otp_idx_min;
    return lock_bit_base + otp_idx_offset;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_get_lock(uint8_t otp_idx, bool *locked)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    if (locked == NULL) {
        ret = FLASH_BYPASS_OTP_ERR_PARAM;
        goto exit;
    }

    ret = flash_bypass_otp_check_range(otp_idx, 0, 0, true);
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    uint32_t sr_value = 0;
    ret = flash_bypass_otp_read_sr(&sr_value);
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    uint8_t lock_bit = flash_bypass_otp_get_lock_bit_pos(otp_idx);
    uint32_t lock_mask = 1U << lock_bit;
    *locked = ((sr_value & lock_mask) != 0U);

exit:
    flash_bypass_otp_irq_guard_exit(&guard);
    return ret;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_set_lock(uint8_t otp_idx)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    flash_bypass_otp_err_t ret = flash_bypass_otp_check_range(otp_idx, 0, 0, true);
    if (ret != FLASH_BYPASS_OTP_OK) {
        flash_bypass_otp_irq_guard_exit(&guard);
        return ret;
    }

    ret = flash_bypass_otp_wait_wip();
    if (ret != FLASH_BYPASS_OTP_OK) {
        flash_bypass_otp_irq_guard_exit(&guard);
        return ret;
    }

    uint32_t sr_value = 0;
    ret = flash_bypass_otp_read_sr(&sr_value);
    if (ret != FLASH_BYPASS_OTP_OK) {
        flash_bypass_otp_irq_guard_exit(&guard);
        return ret;
    }

    uint8_t lock_bit = flash_bypass_otp_get_lock_bit_pos(otp_idx);
    sr_value |= (1U << lock_bit);

    ret = flash_bypass_otp_write_sr(sr_value);
    if (ret != FLASH_BYPASS_OTP_OK) {
        flash_bypass_otp_irq_guard_exit(&guard);
        return ret;
    }

    bool locked = false;
    ret = flash_bypass_otp_get_lock(otp_idx, &locked);
    if (ret != FLASH_BYPASS_OTP_OK) {
        flash_bypass_otp_irq_guard_exit(&guard);
        return ret;
    }

    if (!locked) {
        flash_bypass_otp_irq_guard_exit(&guard);
        return FLASH_BYPASS_OTP_ERR_LOCK_SET_FAIL;
    }

    flash_bypass_otp_irq_guard_exit(&guard);
    return FLASH_BYPASS_OTP_OK;
}

static __attribute__((section(".iram"))) uint32_t flash_bypass_otp_calc_full_addr(uint8_t otp_idx, uint32_t addr_offset)
{
    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    uint32_t base_addr = ((uint32_t)otp_idx) << cfg->otp_addr_idx_offset;
    return base_addr + addr_offset;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_read(uint8_t otp_idx, uint32_t addr_offset, uint8_t *buf, uint32_t len)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;

    if (buf == NULL) {
        ret = FLASH_BYPASS_OTP_ERR_PARAM;
        goto exit;
    }

    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();

    ret = flash_bypass_otp_check_range(otp_idx, addr_offset, len, false);
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    uint32_t otp_addr = flash_bypass_otp_calc_full_addr(otp_idx, addr_offset);
    uint8_t tx_buf[5] = {0};
    tx_buf[0] = cfg->cmd_read_otp;
    tx_buf[1] = (uint8_t)(otp_addr >> 16);
    tx_buf[2] = (uint8_t)(otp_addr >> 8);
    tx_buf[3] = (uint8_t)otp_addr;
    tx_buf[4] = 0x00;

    if (flash_bypass_op_read(tx_buf, sizeof(tx_buf), buf, len) != len) {
        ret = FLASH_BYPASS_OTP_ERR_DATA_READ_FAIL;
        goto exit;
    }

exit:
    flash_bypass_otp_irq_guard_exit(&guard);
    return ret;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_erase(uint8_t otp_idx)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();

    ret = flash_bypass_otp_check_range(otp_idx, 0, cfg->otp_block_size, false);
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    ret = flash_bypass_otp_wait_wip();
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    uint32_t otp_addr = flash_bypass_otp_calc_full_addr(otp_idx, 0);
    uint8_t wr_en = cfg->cmd_wr_en;
    uint8_t tx_buf[4] = {0};
    tx_buf[0] = cfg->cmd_erase_otp;
    tx_buf[1] = (uint8_t)(otp_addr >> 16);
    tx_buf[2] = (uint8_t)(otp_addr >> 8);
    tx_buf[3] = (uint8_t)otp_addr;

    if (flash_bypass_op_write(&wr_en, NULL, 0) < 0) {
        ret = FLASH_BYPASS_OTP_ERR_DATA_ERASE_FAIL;
        goto exit;
    }

    ret = flash_bypass_otp_wait_wel();
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    if (flash_bypass_op_write(NULL, tx_buf, sizeof(tx_buf)) < 0) {
        ret = FLASH_BYPASS_OTP_ERR_DATA_ERASE_FAIL;
        goto exit;
    }

    ret = flash_bypass_otp_wait_wip();
exit:
    flash_bypass_otp_irq_guard_exit(&guard);
    return ret;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_write_single(uint8_t otp_idx, uint32_t addr_offset, uint8_t *buf, uint32_t len)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;

    if (buf == NULL) {
        ret = FLASH_BYPASS_OTP_ERR_PARAM;
        goto exit;
    }

    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();
    if (len > cfg->max_write_len) {
        ret = FLASH_BYPASS_OTP_ERR_WRITE_LEN_OVERFLOW;
        goto exit;
    }

    ret = flash_bypass_otp_wait_wip();
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    uint32_t otp_addr = flash_bypass_otp_calc_full_addr(otp_idx, addr_offset);
    uint8_t wr_en = cfg->cmd_wr_en;
    uint8_t tx_head[4] = {0};

    tx_head[0] = cfg->cmd_prog_otp;
    tx_head[1] = (uint8_t)(otp_addr >> 16);
    tx_head[2] = (uint8_t)(otp_addr >> 8);
    tx_head[3] = (uint8_t)otp_addr;

    if (flash_bypass_op_write(&wr_en, NULL, 0) < 0) {
        ret = FLASH_BYPASS_OTP_ERR_DATA_WRITE_FAIL;
        goto exit;
    }

    ret = flash_bypass_otp_wait_wel();
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    if (flash_bypass_op_write_combin(NULL, tx_head, sizeof(tx_head), buf, len) < 0) {
        ret = FLASH_BYPASS_OTP_ERR_DATA_WRITE_FAIL;
        goto exit;
    }

    ret = flash_bypass_otp_wait_wip();
exit:
    flash_bypass_otp_irq_guard_exit(&guard);
    return ret;
}

static __attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_write(uint8_t otp_idx, uint32_t addr_offset, uint8_t *buf, uint32_t len)
{
    flash_bypass_otp_irq_guard_t guard = flash_bypass_otp_irq_guard_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;

    if (buf == NULL) {
        ret = FLASH_BYPASS_OTP_ERR_PARAM;
        goto exit;
    }

    const flash_bypass_otp_config_t *cfg = flash_bypass_otp_get_config();

    ret = flash_bypass_otp_check_range(otp_idx, addr_offset, len, false);
    if (ret != FLASH_BYPASS_OTP_OK) {
        goto exit;
    }

    if (len <= cfg->max_write_len) {
        ret = flash_bypass_otp_write_single(otp_idx, addr_offset, buf, len);
        goto exit;
    }

    uint32_t written = 0;
    while (written < len) {
        uint32_t write_len = len - written;
        if (write_len > cfg->max_write_len) {
            write_len = cfg->max_write_len;
        }

        ret = flash_bypass_otp_write_single(otp_idx, addr_offset + written, buf + written, write_len);
        if (ret != FLASH_BYPASS_OTP_OK) {
            goto exit;
        }

        written += write_len;
    }

exit:
    flash_bypass_otp_irq_guard_exit(&guard);
    return ret;
}

bk_err_t flash_bypass_otp_test(void)
{
    FLASH_BYPASS_OTP_LOGI("===== OTP Test Start =====\r\n");

    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    bk_err_t ret = BK_OK;
    uint8_t *read_buf = NULL;
    uint8_t *write_buf = NULL;
    const flash_bypass_otp_config_t *cfg = NULL;
    uint32_t blocks_tested = 0;
    uint32_t blocks_skipped_locked = 0;

    uint32_t flash_id = flash_bypass_otp_get_flash_id();
    size_t cfg_idx;
    for (cfg_idx = 0; cfg_idx < sizeof(s_flash_bypass_otp_configs) / sizeof(s_flash_bypass_otp_configs[0]) - 1; cfg_idx++) {
        if (s_flash_bypass_otp_configs[cfg_idx].flash_id == flash_id) {
            break;
        }
    }
    if (cfg_idx == sizeof(s_flash_bypass_otp_configs) / sizeof(s_flash_bypass_otp_configs[0]) - 1) {
        FLASH_BYPASS_OTP_LOGE("unsupported flash id: 0x%06X\r\n", flash_id);
        ret = BK_FAIL;
        goto cleanup;
    }

    cfg = flash_bypass_otp_get_config();

    read_buf = (uint8_t *)os_malloc(cfg->otp_block_size);
    write_buf = (uint8_t *)os_malloc(cfg->otp_block_size);
    if (!read_buf || !write_buf) {
        FLASH_BYPASS_OTP_LOGE("malloc failed\r\n");
        ret = BK_ERR_NO_MEM;
        goto cleanup;
    }
    for (uint32_t i = 0; i < cfg->otp_block_size; i++) {
        write_buf[i] = (uint8_t)(i & 0xFF);
    }

    uint32_t test_cases[] = {
        16,
        cfg->max_write_len,
        cfg->otp_block_size
    };
    uint32_t test_case_count = sizeof(test_cases) / sizeof(test_cases[0]);

    for (uint8_t otp_idx = cfg->otp_idx_min; otp_idx <= cfg->otp_idx_max; otp_idx++) {
        bool locked = false;
        flash_bypass_otp_err_t lock_ret = flash_bypass_otp_get_lock(otp_idx, &locked);
        if (lock_ret != FLASH_BYPASS_OTP_OK) {
            FLASH_BYPASS_OTP_LOGE("OTP%d get lock failed: ret=%d\r\n", otp_idx, lock_ret);
            ret = BK_FAIL;
            goto cleanup;
        }
        if (locked) {
            FLASH_BYPASS_OTP_LOGW("OTP%d is locked, skip test\r\n", otp_idx);
            blocks_skipped_locked++;
            continue;
        }

        for (uint32_t case_idx = 0; case_idx < test_case_count; case_idx++) {
            uint32_t chunk_len = test_cases[case_idx];

            flash_bypass_otp_err_t otp_ret = flash_bypass_otp_erase(otp_idx);
            if (otp_ret != FLASH_BYPASS_OTP_OK) {
                FLASH_BYPASS_OTP_LOGE("OTP%d chunk_len=%u erase failed: ret=%d\r\n",
                                      otp_idx, chunk_len, otp_ret);
                ret = BK_FAIL;
                goto cleanup;
            }

            otp_ret = flash_bypass_otp_read(otp_idx, 0, read_buf, cfg->otp_block_size);
            if (otp_ret != FLASH_BYPASS_OTP_OK) {
                FLASH_BYPASS_OTP_LOGE("OTP%d chunk_len=%u read after erase failed: ret=%d\r\n",
                                      otp_idx, chunk_len, otp_ret);
                ret = BK_FAIL;
                goto cleanup;
            }

            for (uint32_t i = 0; i < cfg->otp_block_size; i++) {
                if (read_buf[i] != 0xFF) {
                    FLASH_BYPASS_OTP_LOGE("OTP%d chunk_len=%u erase verify failed at offset %u: expected=0xFF got=0x%02X\r\n",
                                          otp_idx, chunk_len, i, read_buf[i]);
                    ret = BK_FAIL;
                    goto cleanup;
                }
            }

            for (uint32_t offset = 0; offset < cfg->otp_block_size; offset += chunk_len) {
                uint32_t len = cfg->otp_block_size - offset;
                if (len > chunk_len) {
                    len = chunk_len;
                }
                otp_ret = flash_bypass_otp_write(otp_idx, offset, write_buf + offset, len);
                if (otp_ret != FLASH_BYPASS_OTP_OK) {
                    FLASH_BYPASS_OTP_LOGE("OTP%d chunk_len=%u write failed: offset=%u len=%u ret=%d\r\n",
                                          otp_idx, chunk_len, offset, len, otp_ret);
                    ret = BK_FAIL;
                    goto cleanup;
                }
            }

            otp_ret = flash_bypass_otp_read(otp_idx, 0, read_buf, cfg->otp_block_size);
            if (otp_ret != FLASH_BYPASS_OTP_OK) {
                FLASH_BYPASS_OTP_LOGE("OTP%d chunk_len=%u read after write failed: ret=%d\r\n",
                                      otp_idx, chunk_len, otp_ret);
                ret = BK_FAIL;
                goto cleanup;
            }

            for (uint32_t i = 0; i < cfg->otp_block_size; i++) {
                if (read_buf[i] != write_buf[i]) {
                    FLASH_BYPASS_OTP_LOGE("OTP%d chunk_len=%u verify failed at offset %u: expected=0x%02X got=0x%02X\r\n",
                                          otp_idx, chunk_len, i, write_buf[i], read_buf[i]);
                    ret = BK_FAIL;
                    goto cleanup;
                }
            }
        }

        blocks_tested++;
    }

cleanup:
    // Restore system line mode
    bk_flash_bypass_otp_exit(old_mode);

    // Free dynamically allocated memory
    if (read_buf != NULL) {
        os_free(read_buf);
        read_buf = NULL;
    }
    if (write_buf != NULL) {
        os_free(write_buf);
        write_buf = NULL;
    }

    if (ret == BK_OK && cfg != NULL) {
        FLASH_BYPASS_OTP_LOGI("===== OTP Test PASS =====\r\n");
        FLASH_BYPASS_OTP_LOGI("tested %u block(s), skipped locked %u\r\n",
                              blocks_tested, blocks_skipped_locked);
    } else {
        FLASH_BYPASS_OTP_LOGE("===== OTP Test FAIL (ret=%d) =====\r\n", ret);
    }
    return ret;
}

__attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_test_read(uint8_t otp_idx, uint32_t addr_offset, uint8_t *buf, uint32_t len)
{
    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    ret = flash_bypass_otp_read(otp_idx, addr_offset, buf, len);
    bk_flash_bypass_otp_exit(old_mode);
    return ret;
}

__attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_test_write(uint8_t otp_idx, uint32_t addr_offset, uint8_t *buf, uint32_t len)
{
    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    ret = flash_bypass_otp_write(otp_idx, addr_offset, buf, len);
    bk_flash_bypass_otp_exit(old_mode);
    return ret;
}

__attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_test_erase(uint8_t otp_idx)
{
    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    ret = flash_bypass_otp_erase(otp_idx);
    bk_flash_bypass_otp_exit(old_mode);
    return ret;
}

__attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_test_set_lock(uint8_t otp_idx)
{
    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    ret = flash_bypass_otp_set_lock(otp_idx);
    bk_flash_bypass_otp_exit(old_mode);
    return ret;
}

__attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_test_is_lock(uint8_t otp_idx, bool *locked)
{
    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    ret = flash_bypass_otp_get_lock(otp_idx, locked);
    bk_flash_bypass_otp_exit(old_mode);
    return ret;
}

__attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_test_read_sr(uint32_t *sr_value)
{
    if (sr_value == NULL)
        return FLASH_BYPASS_OTP_ERR_PARAM;
    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    ret = flash_bypass_otp_read_sr(sr_value);
    bk_flash_bypass_otp_exit(old_mode);
    return ret;
}

__attribute__((section(".iram"))) flash_bypass_otp_err_t flash_bypass_otp_test_write_sr(uint32_t sr_value)
{
    flash_line_mode_t old_mode = bk_flash_bypass_otp_enter();
    flash_bypass_otp_err_t ret = FLASH_BYPASS_OTP_OK;
    ret = flash_bypass_otp_write_sr(sr_value);
    bk_flash_bypass_otp_exit(old_mode);
    return ret;
}

#endif /* CONFIG_FLASH_BYPASS_OTP_V2 */

