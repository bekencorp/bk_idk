#ifndef __FLASH_BYPASS_OTP_H__
#define __FLASH_BYPASS_OTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <common/bk_include.h>
#include <components/log.h>

#if CONFIG_FLASH_BYPASS_OTP_V2

#define FLASH_BYPASS_OTP_TAG "flash_bypass_otp"
#define FLASH_BYPASS_OTP_LOGI(...) BK_LOGI(FLASH_BYPASS_OTP_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_OTP_LOGW(...) BK_LOGW(FLASH_BYPASS_OTP_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_OTP_LOGE(...) BK_LOGE(FLASH_BYPASS_OTP_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_OTP_LOGD(...) BK_LOGD(FLASH_BYPASS_OTP_TAG, ##__VA_ARGS__)

typedef enum {
    FLASH_BYPASS_OTP_OK                             = 0,

    FLASH_BYPASS_OTP_ERR_GET_CFG_FAIL               = 1,
    FLASH_BYPASS_OTP_ERR_PARAM                      = 2,
    FLASH_BYPASS_OTP_ERR_NO_MEM                     = 3,

    FLASH_BYPASS_OTP_ERR_OTP_IDX_OUT_OF_RANGE       = 4,
    FLASH_BYPASS_OTP_ERR_LEN_ZERO_NOT_ALLOWED       = 5,
    FLASH_BYPASS_OTP_ERR_OFFSET_OUT_OF_RANGE        = 6,
    FLASH_BYPASS_OTP_ERR_RANGE_CROSS_BLOCK          = 7,
    FLASH_BYPASS_OTP_ERR_WRITE_LEN_OVERFLOW         = 8,
    FLASH_BYPASS_OTP_ERR_WRITE_RANGE_CROSS_BLOCK    = 9,

    FLASH_BYPASS_OTP_ERR_FLASH_ID_READ              = 10,
    FLASH_BYPASS_OTP_ERR_FLASH_ID_ZERO              = 11,
    FLASH_BYPASS_OTP_ERR_FLASH_ID_READ_FAIL         = 12,

    FLASH_BYPASS_OTP_ERR_SR_READ_FAIL               = 15,
    FLASH_BYPASS_OTP_ERR_SR_WRITE_FAIL              = 16,
    FLASH_BYPASS_OTP_ERR_WIP_TIMEOUT                = 17,
    FLASH_BYPASS_OTP_ERR_LOCK_SET_FAIL              = 18,

    FLASH_BYPASS_OTP_ERR_DATA_ERASE_FAIL            = 19,
    FLASH_BYPASS_OTP_ERR_DATA_WRITE_FAIL            = 20,
    FLASH_BYPASS_OTP_ERR_SPLIT_WRITE_FAIL           = 21,
    FLASH_BYPASS_OTP_ERR_DATA_READ_FAIL             = 22,

    FLASH_BYPASS_OTP_ERR_WEL_TIMEOUT                = 23,
} flash_bypass_otp_err_t;

bk_err_t flash_bypass_otp_test(void);
flash_bypass_otp_err_t flash_bypass_otp_test_read(uint8_t otp_idx, uint32_t addr_offset, uint8_t *buf, uint32_t len);
flash_bypass_otp_err_t flash_bypass_otp_test_write(uint8_t otp_idx, uint32_t addr_offset, uint8_t *buf, uint32_t len);
flash_bypass_otp_err_t flash_bypass_otp_test_erase(uint8_t otp_idx);
flash_bypass_otp_err_t flash_bypass_otp_test_set_lock(uint8_t otp_idx);
flash_bypass_otp_err_t flash_bypass_otp_test_is_lock(uint8_t otp_idx, bool *locked);
flash_bypass_otp_err_t flash_bypass_otp_test_read_sr(uint32_t *sr_value);
flash_bypass_otp_err_t flash_bypass_otp_test_write_sr(uint32_t sr_value);

#endif /* CONFIG_FLASH_BYPASS_OTP_V2 */

#ifdef __cplusplus
}
#endif
#endif /* __FLASH_BYPASS_OTP_H__ */

