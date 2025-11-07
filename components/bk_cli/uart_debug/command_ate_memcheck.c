#include "command_ate.h"
#include <string.h>

#if CONFIG_ATE_TEST
#include <driver/otp.h>
#include <os/mem.h>
#include "aon_pmu_hal.h"
bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value);

int mcheck_section(uint32_t* reg_addr, uint16_t* output, int reg_points_num, int output_points_num, int addr_len, int singe_repair_points)
{
    int reg_index, output_index;
    uint32_t reg_value;

    for (output_index=0, reg_index=0; reg_index< reg_points_num; reg_index++)
    {
        // keep the low part of register
        reg_value = reg_addr[reg_index] & 0x0000FFFF;

        // confirm whether the address is valid
        if ((reg_value >> (addr_len-1)) % 2 == 1)
        {
            if (output_index >= output_points_num)
            {
                //printf("MCHECK fail! stop register index:%d\r\n", reg_index);
                return 1;
            }
            else
            {
                // if address < 16 bits, need to convert to OTP address
                if(addr_len < 16)
                {
                    reg_value |= 0x00008000;              //firstly set the highest bit
                    reg_value &= ~(1 << (addr_len-1));    // clear the original valid bit
                    reg_value |=  (reg_index / singe_repair_points) << (addr_len-1);    // calculate the index and set corresbonding bits
                }

                output[output_index] = (int16_t) reg_value;
                output_index += 1;
            }
        }
    }

    return 0;
}

int mcheck_ate(uint16_t check_res[48])
{
    uint32_t* reg_start_addr;
    int res = -1;

    reg_start_addr = (uint32_t*)SOC_MEM_CHECK_REG_BASE;

    // SMEM2-5 and h265, overall 20 points, no share
    res = mcheck_section(reg_start_addr + 8, check_res + 0, 20, 20, 16, 4);

    // SMEM0-1 overall 8 points, share 4 points
    res += mcheck_section(reg_start_addr + 28, check_res + 20, 8, 4, 15, 4);

   // cpu0_0->cpu2_0 overall 12 points, share 6 points
    res += mcheck_section(reg_start_addr + 36, check_res + 24, 12, 6, 14, 4);

   // cpu0_1->cpu1_3 overall 16 points, share 8 points
    res += mcheck_section(reg_start_addr + 48, check_res + 30, 16, 8, 13, 2);

   // cpu2_3->pram overall 14 points, share 8 points
    res += mcheck_section(reg_start_addr + 64, check_res + 38, 14, 8, 13, 2);

   // usb2->dmad overall 4 points, share 2 points
    res += mcheck_section(reg_start_addr + 78, check_res + 46, 4, 2, 12, 2);

    return res;
}

int cmd_save_memcheck(void)
{
    uint16_t check_res[48];// TODO: bk7236q memcheck size 56
    int res;
    int ret = -1;

	memset((void *)check_res, 0x0, sizeof(check_res));
    res = mcheck_ate(check_res);
    if (res > 0) {
        return -1;
    }

#if CONFIG_OTP
    ret = bk_otp_apb_update(OTP_MEMORY_CHECK_MARK, (uint8_t *)check_res, sizeof(check_res));
#endif
#if CONFIG_FLASH_OTP
    extern void sys_hal_set_efstoflash_otp(uint8_t load, uint8_t shaddr, uint8_t en);
    extern void bk_flash_lock_otp(uint8_t block_index);
    bk_flash_otp_init(1);
    uint32_t magic_word = 0x50544f43;
    sys_hal_set_efstoflash_otp(0, 0, 1);
    ret = bk_flash_otp_update(OTP_MEMORY_CHECK_MAGIC, (uint8_t *)&magic_word, sizeof(magic_word))
                || bk_flash_otp_update(OTP_MEMORY_CHECK_MARK, (uint8_t *)check_res, sizeof(check_res));
    // bk_flash_lock_otp(1); //TODO: confirm the lock at the appropriate time
#endif
    return ret;
}

void cmd_start_softreset(uint8_t vcore, uint8_t wdt_delay)
{
	sys_hal_ctrl_vdddig_h_vol(vcore);
	aon_pmu_hal_reg_set(PMU_REG2, 0x1FE);

    //softreset mode
    REG_SET(SOC_WDT_REG_BASE + 4 * 2, 0, 0, 1);
    uart_send_byte_for_ate(0x55);
    uart_send_byte_for_ate(0x33);
#if CONFIG_SUPPORT_AON_WDT
    //disable always on wdt to avoid dog time when MBIST
    REG_WRITE(SOC_AON_WDT_REG_BASE, 0x5A0000);
    REG_WRITE(SOC_AON_WDT_REG_BASE, 0xA50000);
#endif
	//(*(volatile uint32_t *)(SOC_AON_GPIO_REG_BASE + 18 * 4)) = 2; //for test by xiaodi
    REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0x5A0000 | wdt_delay);
    REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0xA50000 | wdt_delay);
    while(1);
}
#endif

#if CONFIG_ATE_TEST && CONFIG_RESET_REASON
#include <components/system.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include "reset_reason.h"

#define RESET_SOURCE_RAM_TEST 0x5A

static int memcheck(uint32_t *aligned4_ptr, uint32_t bytes_length, uint32_t magic)
{
    uint32_t *aligned4_end = aligned4_ptr + (bytes_length >> 2);
    memset((void *)aligned4_ptr, (uint8_t)magic, bytes_length);
    for (; aligned4_ptr < aligned4_end; aligned4_ptr++)
    {
        if (*aligned4_ptr != magic)
        {
            return -1;
        }
    }

    return 0;
}

int cmd_do_memcheck(void)
{
    UINT32 start_type;

    reset_reason_init();

    start_type = bk_misc_get_reset_reason();
    if ((start_type & 0xFF) != RESET_SOURCE_RAM_TEST)
    {
        return 0;
    }

    bk_gpio_driver_init();
    bk_uart_driver_init();

    uart_send_byte_for_ate(0x55);
    uart_send_byte_for_ate(0x33);
    while(1);
    return 1;
}

void cmd_start_memcheck(void)
{
    bk_reboot_ex(RESET_SOURCE_RAM_TEST);
}

/* ============================================================================
 * MACRO DEFINITIONS
 * ============================================================================ */

#define MEMCHK_OK                      0
#define MEMCHK_ERR_SAVE_CFG1          -1
#define MEMCHK_ERR_CHECK_CONTENT_CRC  -2
#define MEMCHK_ERR_PARSE_OR_CHECK     -3
#define MEMCHK_ERR_CHECK_OTP_CRC      -4
#define MEMCHK_ERR_UNKNOWN            -5

#define MEMCHECK_HEADER_LEN           2
#define MEMCHECK_PAYLOAD_LEN          139
#define MEMCHECK_CRC_LEN              1
#define MEMCHECK_EXPECTED_LEN         (MEMCHECK_HEADER_LEN + MEMCHECK_PAYLOAD_LEN + MEMCHECK_CRC_LEN)

#define MEMCHECK_POINT_COUNT          34
#define MEMCHECK_DATA1_OFFSET         0
#define MEMCHECK_DATA2_OFFSET         68
#define MEMCHECK_BGCAL_INDEX          136
#define MEMCHECK_VCOREHSEL_INDEX      137
#define MEMCHECK_CLK_DIV_INDEX        138

#define MEMCHECK_OTP_MARK_SIZE        60
#define MEMCHECK_OTP_EXTRA_SIZE       8
#define MEMCHECK_OTP_VDDDIG_SIZE      4
#define MEMCHECK_OTP_TOTAL_SIZE       (MEMCHECK_OTP_MARK_SIZE + MEMCHECK_OTP_EXTRA_SIZE + MEMCHECK_OTP_VDDDIG_SIZE)
#define MEMCHECK_OTP_DATA_SIZE        (MEMCHECK_OTP_TOTAL_SIZE - 1)

/* ============================================================================
 * STRUCTURE DEFINITIONS
 * ============================================================================ */

typedef union {
    struct {
        uint16_t mchk : 15; /* bit[14:00] */
        uint16_t flag : 1;  /* bit[15:15] */
    } bits;
    uint16_t raw;
} memcheck_data_t;

typedef union {
    struct {
        uint32_t bgcal      : 8;  /* bit[07:00] */
        uint32_t vcorehsel1 : 4;  /* bit[11:08] */
        uint32_t vcorehsel2 : 4;  /* bit[15:12] */
        uint32_t clk_div1   : 4;  /* bit[19:16] */
        uint32_t clk_div2   : 4;  /* bit[23:20] */
        uint32_t crc8       : 8;  /* bit[31:24] */
    } bits;
    uint32_t raw;
} memcheck_config_t;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static int memcheck_check_data_pattern(memcheck_data_t *datas, uint8_t offset, uint8_t count, char *data_name, char *group_name)
{
    /* Error1: flag should be 0 (bit15 must be 0 in input data) */
    /* Error2: there is a empty value in the middle of non-empty values */
    /* Error3: duplicate non-empty value */

    uint8_t non_empty_count = 0;

    for (uint8_t idx = 0; idx < count; idx++) {
        memcheck_data_t *item = &datas[offset + idx];

        // except smem2, smem3, smem4
        if (item->bits.flag != 0
            && strcmp(group_name, "smem2") != 0
            && strcmp(group_name, "smem3") != 0
            && strcmp(group_name, "smem4") != 0) {
            bk_printf("[memchk] %s %s error: flag bit should be 0 (value=0x%04X) at index %d\r\n",
                      group_name, data_name, item->raw, idx);
            return -1;
        }

        if (item->raw != 0) {
            if (non_empty_count != idx) {
                bk_printf("[memchk] %s %s error: there is a empty value in the middle of non-empty values\r\n",
                          group_name, data_name);
                return -1;
            }

            for (uint8_t i = 0; i < non_empty_count; i++) {
                if (datas[offset + i].raw == item->raw) {
                    bk_printf("[memchk] %s %s error: duplicate non-empty value 0x%04X at index %d and %d\r\n",
                              group_name, data_name, item->raw, i, idx);
                    return -1;
                }
            }

            non_empty_count++;
        }
    }

    return non_empty_count;
}

static int memcheck_merge_group(char *group_name, uint8_t offset, uint8_t count, memcheck_data_t *data1, memcheck_data_t *data2, memcheck_data_t *out)
{
    int data2_check_ret, data1_check_ret;

    data2_check_ret = memcheck_check_data_pattern(data2, offset, count, "data2", group_name);
    data1_check_ret = memcheck_check_data_pattern(data1, offset, count, "data1", group_name);
    if ((data2_check_ret < 0) || (data1_check_ret < 0)) {
        return MEMCHK_ERR_PARSE_OR_CHECK;
    }

    if ((data2_check_ret == 0) && (data1_check_ret == 0)) {
        bk_printf("[memchk] %s empty, no need to merge\r\n", group_name);
        return 0;
    }

    /* Merge: save data2 first, then add unique data1 values */
    uint8_t used_count = 0;
    uint8_t data1_count = (uint8_t)data1_check_ret;
    uint8_t data2_count = (uint8_t)data2_check_ret;

    /* Step 1: copy all data2 values */
    if (data2_count > 0) {
        memcpy(&out[offset], &data2[offset], data2_count * sizeof(memcheck_data_t));
    }
    used_count = data2_count;
    bk_printf("[memchk] %s data2=%d used=%d\r\n", group_name, data2_check_ret, used_count);

    /* Step 2: add unique data1 values not in data2 */
    for (uint8_t i = 0; i < data1_count; i++) {
        uint16_t data1_value = data1[offset + i].raw;
        bool is_duplicate = false;

        for (uint8_t j = 0; j < used_count; j++) {
            if (out[offset + j].raw == data1_value) {
                is_duplicate = true;
                break;
            }
        }

        if (is_duplicate) {
            continue;
        }

        if (used_count >= count) {
            bk_printf("[memchk] %s overflow: 0x%04X used=%d cap=%d\r\n",
                      group_name, data1_value, used_count, count);
            return MEMCHK_ERR_SAVE_CFG1;
        }

        out[offset + used_count].raw = data1_value;
        used_count++;
    }
    bk_printf("[memchk] %s data1=%d used=%d\r\n", group_name, data1_check_ret, used_count);

    /* Step 3: set flag for all non-empty values */
    for (uint8_t i = 0; i < used_count; i++) {
        out[offset + i].bits.flag = 1;
    }

    return 0;
}

/* ============================================================================
 * MAIN FUNCTIONS
 * ============================================================================ */

void cmd_save_memcheck_from_uart(uint8_t *content, int cnt, uint8_t *tx_buffer)
{
    int ret = MEMCHK_OK;
    memcheck_data_t *memcheck_points_data1 = NULL;
    memcheck_data_t *memcheck_points_data2 = NULL;
    memcheck_data_t *result = NULL;
    uint8_t *check_buffer = NULL;
    uint8_t *check_extra_buffer = NULL;
    uint8_t *check_vdddig_buffer = NULL;

    /* STEP 1: crc check */
    if ((content == NULL) || (cnt != MEMCHECK_EXPECTED_LEN)) {
        bk_printf("[memchk] payload check failed with cnt=%d\r\n", cnt);
        ret = MEMCHK_ERR_UNKNOWN;
        goto cleanup;
    }

    uint8_t *payload = content + MEMCHECK_HEADER_LEN; /* points to content[2] */
    extern uint8_t bk7011_crc8_poly0x31(uint8_t *buf, uint32_t length, uint8_t crc);
    uint8_t crc_calc = bk7011_crc8_poly0x31((uint8_t *)payload, MEMCHECK_PAYLOAD_LEN, 0);
    uint8_t crc_recv = content[MEMCHECK_HEADER_LEN + MEMCHECK_PAYLOAD_LEN];
    if (crc_calc != crc_recv) {
        bk_printf("[memchk] crc check failed\r\n");
        ret = MEMCHK_ERR_CHECK_CONTENT_CRC;
        goto cleanup;
    }

    /* STEP 2: parse data */
    memcheck_config_t memcheck_config = {0};
    memcheck_points_data1 = (memcheck_data_t *)os_malloc(MEMCHECK_POINT_COUNT * sizeof(memcheck_data_t));
    memcheck_points_data2 = (memcheck_data_t *)os_malloc(MEMCHECK_POINT_COUNT * sizeof(memcheck_data_t));
    if ((memcheck_points_data1 == NULL) || (memcheck_points_data2 == NULL)) {
        ret = MEMCHK_ERR_UNKNOWN;
        bk_printf("[memchk] malloc memcheck points failed\r\n");
        goto cleanup;
    }

    // Parse memcheck_points (copy from payload to structures)
    memcpy(memcheck_points_data1, payload + MEMCHECK_DATA1_OFFSET, MEMCHECK_POINT_COUNT * sizeof(memcheck_data_t));
    memcpy(memcheck_points_data2, payload + MEMCHECK_DATA2_OFFSET, MEMCHECK_POINT_COUNT * sizeof(memcheck_data_t));

    // Parse bgcal (byte 136)
    memcheck_config.bits.bgcal = payload[MEMCHECK_BGCAL_INDEX];

    // Parse vcorehsel and clk_div (bytes 137 and 138)
    uint8_t vcorehsel = payload[MEMCHECK_VCOREHSEL_INDEX];
    uint8_t clk_div   = payload[MEMCHECK_CLK_DIV_INDEX];
    memcheck_config.bits.vcorehsel1 = (uint8_t)(vcorehsel & 0x0F);
    memcheck_config.bits.vcorehsel2 = (uint8_t)((vcorehsel >> 4) & 0x0F);
    memcheck_config.bits.clk_div1   = (uint8_t)(clk_div & 0x0F);
    memcheck_config.bits.clk_div2   = (uint8_t)((clk_div >> 4) & 0x0F);

    /* STEP 3: merge data */
    result = (memcheck_data_t *)os_malloc(MEMCHECK_POINT_COUNT * sizeof(memcheck_data_t));
    if (result == NULL) {
        ret = MEMCHK_ERR_UNKNOWN;
        bk_printf("[memchk] malloc result failed\r\n");
        goto cleanup;
    }
    memset(result, 0, MEMCHECK_POINT_COUNT * sizeof(memcheck_data_t));
    memcheck_data_t *data1 = memcheck_points_data1;
    memcheck_data_t *data2 = memcheck_points_data2;

    ret = memcheck_merge_group("smem0", 0, 4, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("smem1", 4, 4, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("smem2", 8, 4, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("smem3", 12, 4, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("smem4", 16, 4, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("cpu0_0", 20, 4, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("cpu0_1", 24, 2, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("encp", 26, 2, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("psram", 28, 2, data1, data2, result);
    if (ret < 0) goto cleanup;
    ret = memcheck_merge_group("wifi", 30, 4, data1, data2, result);
    if (ret < 0) goto cleanup;

    /* STEP 4: add crc8 (chained) */
    uint8_t crc8 = 0;
    crc8 = bk7011_crc8_poly0x31((uint8_t *)result, MEMCHECK_POINT_COUNT * sizeof(result[0]), crc8);
    crc8 = bk7011_crc8_poly0x31((uint8_t *)&memcheck_config, sizeof(memcheck_config) - 1, crc8); /* exclude crc8 byte */
    memcheck_config.bits.crc8 = crc8;

    /* STEP 5: save data */
    check_buffer = (uint8_t *)os_malloc(MEMCHECK_OTP_MARK_SIZE);
    check_extra_buffer = (uint8_t *)os_malloc(MEMCHECK_OTP_EXTRA_SIZE);
    check_vdddig_buffer = (uint8_t *)os_malloc(MEMCHECK_OTP_VDDDIG_SIZE);
    if ((check_buffer == NULL) || (check_extra_buffer == NULL) || (check_vdddig_buffer == NULL)) {
        ret = MEMCHK_ERR_UNKNOWN;
        bk_printf("[memchk] malloc check buffers failed\r\n");
        goto cleanup;
    }
    memset(check_buffer, 0, MEMCHECK_OTP_MARK_SIZE);
    memset(check_extra_buffer, 0, MEMCHECK_OTP_EXTRA_SIZE);
    memset(check_vdddig_buffer, 0, MEMCHECK_OTP_VDDDIG_SIZE);
    if (bk_otp_apb_read(OTP_MEMORY_CHECK_MARK, check_buffer, MEMCHECK_OTP_MARK_SIZE) != BK_OK) {
        bk_printf("[memchk] read memcheck mark failed\r\n");
        ret = MEMCHK_ERR_UNKNOWN;
        goto cleanup;
    }
    if (bk_otp_apb_read(OTP_MEMORY_CHECK_MARK_EXTRA, check_extra_buffer, MEMCHECK_OTP_EXTRA_SIZE) != BK_OK) {
        bk_printf("[memchk] read memcheck extra failed\r\n");
        ret = MEMCHK_ERR_UNKNOWN;
        goto cleanup;
    }
    if (bk_otp_ahb_read(OTP_MEMORY_CHECK_VDDDIG, check_vdddig_buffer, MEMCHECK_OTP_VDDDIG_SIZE) != BK_OK) {
        bk_printf("[memchk] read memcheck vdddig failed\r\n");
        ret = MEMCHK_ERR_UNKNOWN;
        goto cleanup;
    }

    bool any_non_empty = false;
    for (size_t i = 0; i < MEMCHECK_OTP_MARK_SIZE && !any_non_empty; i++) {
        if (check_buffer[i] != 0x00) {
            any_non_empty = true;
            break;
        }
    }
    for (size_t i = 0; i < MEMCHECK_OTP_EXTRA_SIZE && !any_non_empty; i++) {
        if (check_extra_buffer[i] != 0x00) {
            any_non_empty = true;
            break;
        }
    }
    for (size_t i = 0; i < MEMCHECK_OTP_VDDDIG_SIZE && !any_non_empty; i++) {
        if (check_vdddig_buffer[i] != 0x00) {
            any_non_empty = true;
            break;
        }
    }

    if (any_non_empty) {
        uint8_t *exist_result = (uint8_t *)os_malloc(MEMCHECK_OTP_TOTAL_SIZE);
        if (exist_result == NULL) {
            ret = MEMCHK_ERR_UNKNOWN;
            bk_printf("[memchk] malloc exist_result failed\r\n");
            goto cleanup;
        }
        memcpy(exist_result, check_buffer, MEMCHECK_OTP_MARK_SIZE);
        memcpy(exist_result + MEMCHECK_OTP_MARK_SIZE, check_extra_buffer, MEMCHECK_OTP_EXTRA_SIZE);
        memcpy(exist_result + MEMCHECK_OTP_MARK_SIZE + MEMCHECK_OTP_EXTRA_SIZE, check_vdddig_buffer, MEMCHECK_OTP_VDDDIG_SIZE);
        uint8_t crc_exist = bk7011_crc8_poly0x31(exist_result, MEMCHECK_OTP_DATA_SIZE, 0);
        if (crc_exist != exist_result[MEMCHECK_OTP_DATA_SIZE]) {
            bk_printf("[memchk] existing memcheck crc8 error: calc=0x%02X stored=0x%02X\r\n", crc_exist, exist_result[MEMCHECK_OTP_DATA_SIZE]);
            os_free(exist_result);
            ret = MEMCHK_ERR_CHECK_OTP_CRC;
            goto cleanup;
        }
        bk_printf("[memchk] existing memcheck valid, skip update\r\n");
        os_free(exist_result);
        ret = MEMCHK_OK;
        goto cleanup;
    }

    if (bk_otp_apb_update(OTP_MEMORY_CHECK_MARK, (uint8_t *)result, MEMCHECK_OTP_MARK_SIZE) != BK_OK) {
        bk_printf("[memchk] update memcheck mark failed\r\n");
        ret = MEMCHK_ERR_UNKNOWN;
        goto cleanup;
    }
    if (bk_otp_apb_update(OTP_MEMORY_CHECK_MARK_EXTRA, ((uint8_t *)result) + MEMCHECK_OTP_MARK_SIZE, MEMCHECK_OTP_EXTRA_SIZE) != BK_OK) {
        bk_printf("[memchk] update memcheck extra failed\r\n");
        ret = MEMCHK_ERR_UNKNOWN;
        goto cleanup;
    }
    if (bk_otp_ahb_update(OTP_MEMORY_CHECK_VDDDIG, (uint8_t *)&memcheck_config, MEMCHECK_OTP_VDDDIG_SIZE) != BK_OK) {
        bk_printf("[memchk] update memcheck vdddig failed\r\n");
        ret = MEMCHK_ERR_UNKNOWN;
        goto cleanup;
    }
    ret = MEMCHK_OK;

cleanup:
    if (check_vdddig_buffer)
        os_free(check_vdddig_buffer);
    if (check_extra_buffer)
        os_free(check_extra_buffer);
    if (check_buffer)
        os_free(check_buffer);
    if (result)
        os_free(result);
    if (memcheck_points_data2)
        os_free(memcheck_points_data2);
    if (memcheck_points_data1)
        os_free(memcheck_points_data1);

    /* Send response */
    tx_buffer[0] = 0x55;
    if (ret == MEMCHK_OK) {
        tx_buffer[1] = 0x33;
        uart_send_bytes_for_ate(tx_buffer, 2);
    } else if (ret == MEMCHK_ERR_UNKNOWN) {
        tx_buffer[1] = 0xCC;
        uart_send_bytes_for_ate(tx_buffer, 2);
    } else {
        tx_buffer[1] = 0xCC;
        tx_buffer[2] = (uint8_t)(-ret);
        uart_send_bytes_for_ate(tx_buffer, 3);
    }
}
#endif // CFG_ATE_TEST
