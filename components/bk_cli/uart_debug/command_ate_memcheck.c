#include "command_ate.h"
#include <string.h>

#if CONFIG_ATE_TEST
#include <driver/otp.h>
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
#endif // CFG_ATE_TEST
