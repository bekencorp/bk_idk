#include "command_ate.h"
#include "command_line.h"

#ifdef CONFIG_ATE_TEST
#include "bk_uart.h"
#include "bk_phy_ate.h"
#include "bk_wifi.h"
//#include "phy.h"
//#include "bk_rf_ps.h"
//#include "bk_tx_evm.h"
//#include "bk_rx_sensitivity.h"
#include <os/mem.h>
#include <driver/otp.h>
#include <driver/gpio.h>
#include <driver/efuse.h>
#include <driver/psram.h>
#include <components/ate.h>

#include "bk_phy.h"
//#include "scan.h"
#include "gpio_hal.h"
#include "sys_hal.h"
//#include "bk_cal.h"
#include "bk_rf_internal.h"
#include "aon_pmu_hal.h"
//#include "reg_mdm_cfg.h"
//#if (CONFIG_SOC_BK7256XX)
//#include "rc_driver.h"
//#endif

#include <driver/flash.h>
#include "sys_driver.h"
#include "CheckSumUtils.h"

#define RXSENS_BANDWIDTH_20MHZ                 (0)
#define RXSENS_BANDWIDTH_40MHZ                 (1)

enum mac_chan_bandwidth {
	PHY_CHNL_BW_20,
	PHY_CHNL_BW_40,
	PHY_CHNL_BW_80,
	PHY_CHNL_BW_160,
	PHY_CHNL_BW_80P80,
	PHY_CHNL_BW_OTHER,
};

#define RF_PLL_WIFI       0x1
#define RF_PLL_BT         0x2
#define RF_PLL_BT_LO5G    0x3
#define RF_PLL_MASK       0xF

#define RF_MODE_IQ        0x00  // as default
#define RF_MODE_POLAR     0x10
#define RF_MODE_MASK      0xF0

#define RF_ROLE_WIFI      0x100
#define RF_ROLE_BLE       0x200
#define RF_ROLE_BT        0x400
#define RF_ROLE_THREAD    0x800
#define RF_ROLE_MASK      0xF00
typedef enum {
    RF_WIFI_WIFIPLL_MODE   = RF_ROLE_WIFI   | RF_PLL_WIFI,  // wifi high power
    RF_WIFI_BTPLL_MODE     = RF_ROLE_WIFI   | RF_PLL_BT,    // wifi low power
    RF_BLE_POLAR_MODE      = RF_ROLE_BLE    | RF_PLL_BT   | RF_MODE_POLAR,
    RF_BLE_WIFIPLL_MODE    = RF_ROLE_BLE    | RF_PLL_WIFI,
    RF_BLE_BTPLL_MODE      = RF_ROLE_BLE    | RF_PLL_BT,
    RF_BT_POLAR_MODE       = RF_ROLE_BT     | RF_PLL_BT   | RF_MODE_POLAR,
    RF_BT_WIFIPLL_MODE     = RF_ROLE_BT     | RF_PLL_WIFI,
    RF_BT_BTPLL_MODE       = RF_ROLE_BT     | RF_PLL_BT,
    RF_THREAD_WIFIPLL_MODE = RF_ROLE_THREAD | RF_PLL_WIFI,
    RF_THREAD_BTPLL_MODE   = RF_ROLE_THREAD | RF_PLL_BT,
} RF_MODE;

#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
#define ATE_GPIO_ID                     (14)// GPIO9 use for extern32k
#else
#define ATE_GPIO_ID                     (9)//according the test to select the gpio id
#endif
#define MAC_ADDR_LEN 6

typedef unsigned int                clock_time_t;
extern UINT32 g_rxsens_start;
extern UINT32 g_single_carrier;
extern UINT32 g_rate;
beken2_timer_t rx_sens_ate_tmr      = {0};

#define TX_ENCRPYT_EN_PIN(x)        //(*((uint32_t *)(0x00802800 + TX_ENCRPYT_EN*4))     = (x))
#define TX_ENCRPYT_RESULT_PIN(x)    tx_verify_test_result(x);
#define AX_DEFUALT_GI_TYPE          (0x1)
#define DEFUALT_GI_TYPE             (0)
static UINT8 hw_key_idx             = 0;

UINT32 device_id = 0;

#if ((CONFIG_SOC_BK7256XX) ||(CONFIG_SOC_BK7235) ||(CONFIG_SOC_BK7237))
#define REG_SYS_CTRL_CHIP_ID         (sys_hal_get_chip_id())
#define REG_SYS_CTRL_DEVICE_ID       (aon_pmu_ll_get_reg7c_value())

#define ATE_GPIO_DEV_UART1_RXD       10
#define ATE_GPIO_DEV_UART1_TXD       11

#define RCB_POWER_TABLE_ADDR         (0x4980C400)
#elif (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#define REG_SYS_CTRL_CHIP_ID         (sys_hal_get_chip_id())
#define REG_SYS_CTRL_DEVICE_ID       (aon_pmu_hal_get_chipid())
#if CONFIG_UART_PRINT_PORT_ATE
#define ATE_GPIO_DEV_UART1_RXD       16
#define ATE_GPIO_DEV_UART1_TXD       15
#else
#define ATE_GPIO_DEV_UART1_RXD       10
#define ATE_GPIO_DEV_UART1_TXD       11
#endif
#else
#define SOC_SYSTEM_REG_BASE          (0x00800000)
#define REG_SYS_CTRL_CHIP_ID_ADDR    (SOC_SYSTEM_REG_BASE + 0x0 * 4)
#define REG_SYS_CTRL_CHIP_ID         (*((volatile unsigned long *) REG_SYS_CTRL_CHIP_ID_ADDR))
#define REG_SYS_CTRL_DEVICE_ID_ADDR  (SOC_SYSTEM_REG_BASE + 0x1 * 4)
#define REG_SYS_CTRL_DEVICE_ID       (*((volatile unsigned long *) REG_SYS_CTRL_DEVICE_ID_ADDR))

#define RC_BEKEN_BASE                0x01050000
#define RCB_POWER_TABLE_ADDR         0x01050200
#endif

#define GPIO_NUM_MAX                 (SOC_GPIO_NUM)

extern bk_err_t uart_write_byte(uart_id_t id, uint8_t data);
extern void rwnx_cal_update_pregain(INT32 new_pwr_gain_bias);
extern void sctrl_cali_dpll(UINT8 flag);
extern void manual_cal_show_txpwr_tab(void);
extern void bk7011_max_rxsens_setting(void);
extern int bk7011_reduce_vdddig_for_rx(int reduce);
extern void rwnx_cal_set_bw_i2v(int enable);
extern void mpb_set_txdelay(UINT32 delay_us);
extern uint32_t bk_wifi_get_mpb_ctrl(void);
extern void bk_wifi_set_mpb_ctrl(uint32_t value);
extern void evm_stop_bypass_mac_for_ate (void);
extern void evm_start_bypass_mac_for_ate(void);
extern void evm_bypass_mac_test_with_frequency(UINT32 frequency, wifi_band_t band, UINT32 bandwidth);
extern void evm_set_pwr_for_he_tb(UINT32 format);
extern UINT32 evm_get_auto_hetb_tx_len(UINT32 ru, UINT32 rate, UINT32 guard_i_type);
extern UINT32 evm_frequency_to_channel(UINT32 frequency, wifi_band_t band);

extern void evm_bypass_mac_test(UINT32 channel, wifi_band_t band, UINT32 bandwidth);
extern UINT32 rs_test(UINT32 channel, wifi_band_t band, UINT32 mode);
extern UINT32 rs_test_for_mbp(UINT32 channel, wifi_band_t band, UINT32 mode);
extern UINT32 evm_channel_to_frequency(UINT32 channel, wifi_band_t band);
extern UINT32 rs_test_for_mbp_with_frequency(UINT32 frequency, wifi_band_t band, UINT32 mode);
extern RF_MODE rwnx_cal_set_rfband(wifi_band_t rf_band, RF_MODE rf_mode);
extern int bk_set_spectial_channel_for_ate(uint32_t frequency);
extern void bk_revert_spectial_channel_for_ate(void);

extern UINT32 evm_bypass_mac_set_tx_data_length_for_ate(UINT32 modul_format, \
                                                   UINT32 len, UINT32 rate, \
                                                   UINT32 bandwidth, UINT32 need_change);
extern UINT32 evm_bypass_mac_set_rate_mformat_for_ate(UINT32 ppdu_rate, UINT32 m_format);
extern uint8_t evm_add_key(void);
extern void evm_del_key(uint8_t key_idx);
extern void ble_dut_start(uint8_t uart_id);

UINT8 phy_closed = 0;
extern int _phy_power_ctrl_wrapper(uint8_t power_state);
extern void rwnxl_wakeup();
extern void rwnxl_sleep();

extern uint32_t bk_otp_fully_flow_test();

extern void bk_delay_us(UINT32 us);

#if (!CONFIG_SOC_BK7256XX)
extern void sctrl_dpll_int_open(void);
#endif

extern uint32_t bk_rfpll_ate_test(uint32_t *bandmin_wifi, uint32_t *bandmax_wifi, uint32_t *bandmin_lp, uint32_t *bandmax_lp);
extern void sys_hal_set_cpu_cp_test_config(uint32_t cfg_id);

#if CONFIG_MAC802154_ENABLE
extern uint8_t mac80154_ate(uint8_t * cmd,uint8_t len,uint8_t *out_rsp,uint8_t *out_size);
#endif

extern uint32_t bk_otp_partical_clean_check_customer(void);

bk_err_t core_mem_wr_test(uint32_t start_addr)
{
    uint32_t original_values[10] = {0};
    uint32_t flags = rtos_disable_int();

    for (int i = 0; i < 10; i++) {
        original_values[i] = os_get_word(start_addr + i * 4);
    }

    for (int j = 0; j < 10; j++) {
        uint32_t test_value = 0xABCDEEEF + j;
        os_write_word(start_addr + j * sizeof(uint32_t), test_value);
        uint32_t readback_value = os_get_word(start_addr + j * sizeof(uint32_t));

        if (readback_value != test_value) {
            for (int k = 0; k <= j; k++) {
                os_write_word(start_addr + k * 4, original_values[k]);
            }
            rtos_enable_int(flags);
            return BK_FAIL;
        }
    }
    for (int j = 0; j < 10; j++) {
        os_write_word(start_addr + j * 4, original_values[j]);
    }
    rtos_enable_int(flags);
    return BK_OK;
}

bk_err_t core_mem_wr_all_test(void)
{
    bk_err_t ret;
    uint32_t smem0 = 0x2806F000;
    uint32_t smem1 = 0x28000FFF;
    uint32_t smem2 = 0x28020FFF;
    uint32_t smem3 = 0x28040FFF;
    uint32_t smem4 = 0x28070FFF;

    ret = core_mem_wr_test(smem0);
    if (ret == BK_OK) {
    } else {
        return BK_FAIL;
    }

    ret = core_mem_wr_test(smem1);
    if (ret != BK_OK) {
        return BK_FAIL;
    }

    ret = core_mem_wr_test(smem2);
    if (ret != BK_OK) {
        return BK_FAIL;
    }

    ret = core_mem_wr_test(smem3);
    if (ret != BK_OK) {
        return BK_FAIL;
    }

    ret = core_mem_wr_test(smem4);
    if (ret != BK_OK) {
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t core_cp_test(void)
{
    #define CPU_CLK_240_PASS             (BK_OK)
    #define CPU_CLK_240_FAIL_160_PASS    (0x1)
    #define CPU_CLK_240_FAIL_160_FAIL    (0x2)

    bk_err_t ret;
    /* cpu_core_clock = 240MHz */
    sys_hal_set_cpu_cp_test_config(1);
    /* if cpu240M test pass, then no need to test cpu160M */
    ret = core_mem_wr_all_test();
    if (ret == BK_OK) {
        return CPU_CLK_240_PASS;
    }
    /* cpu_core_clock = 160MHz */
    sys_hal_set_cpu_cp_test_config(0);
    /* if cpu240M test fail and cpu160M test pass, then return "0x55 CC 01" */
    ret = core_mem_wr_all_test();
    if (ret == BK_OK) {
        return CPU_CLK_240_FAIL_160_PASS;
    }
    /* if cpu240M test fail and cpu160M test fail too, then return "0x55 CC 02" */
    return CPU_CLK_240_FAIL_160_FAIL;
}

void uart_send_byte_for_ate(UINT8 data)
{
	uart_write_byte(bk_get_printf_port(), data);
}

void uart_send_bytes_for_ate(UINT8* pData, UINT8 cnt)
{
    int i;
    for(i = 0; i < cnt; i ++)
    {
        uart_send_byte_for_ate(pData[i]);
    }
}

void ate_time_delay(volatile uint32_t times)
{
    while (times--) ;
}

void tx_verify_test_result(UINT8 res)
{
    UINT8 tx_buffer[2];

    tx_buffer[0] = 0x55;
    if(res == 2)
        tx_buffer[1] = 0x33;
    else
        tx_buffer[1] = 0xCC;
    uart_send_bytes_for_ate(tx_buffer, 2);
}

const UINT8 tx_golden[] = {
    0x80,0x01,0x0F,0x00,0xFF,0x00,
    0x00,0x00,0x00,0x00,0x88,0x40,0x00,0x00,0x6C,0xE8,0x5C,0xAA,0x6C,0x8A,0xC8,0x47,
    0x8C,0x08,0xAB,0x49,0xC8,0x47,0x8C,0x08,0xAB,0x49,0x00,0x00,0x00,0x00,0x01,0x00,
    0x00,0x20,0x00,0x00,0x00,0x00,0xDF,0xAF,0xCA,0x91,0xCF,0xB2,0xDC,0x8E,0x3D,0x8C,
    0x73,0x3F,0x1A,0x6E,0xD1,0x69,0x2E,0xF5,0x7D,0xE8,0x43,0xD5,0x59,0xC5,0xD6,0x4C,
    0xF9,0xAC,0x8B,0xFB,0xD0,0x41,0x0D,0x41,0x1D,0xCB,0xAB,0x98,0xEE,0x07,0xEB,0xDB,
    0x90,0xDC,0x01,0xAB,0xA3,0xAD,0x40,0xA1,0x15,0x6A,0x78,0x4A,0x81,0xAE,0x4F,0x91,
    0x22,0xF6,0x77,0xC1,0x19,0xEF,0x46,0x17,0x62,0x88,0xFC,0xFE,0x39,0x81,0x89,0x29,
    0xDE,0xA3,0x9C,0x58,0x51,0xD5,0xFB,0x29,0x7F,0xB2,0x0D,0xA8,0x08,0xE5,0xA1,0x62,
    0x7F,0xCF,0x8B,0xD0,0xD9,0x68,0x30,0x19,0x0A,0x00,0x7F,0xCD,0xEC,0xDA,0xED,0x9B,
    0xD9,0xF4,0x67,0xFD,0x43,0xC6,0xD3,0x75,0x5E,0x01,0xCE,0xD5,0x1E,0xBC,0x53,0x37,
    0xDE,0xDC,0x81,0xE7,0x04,0x3C,0x0D,0xF6,0x9B,0x90,0x0A,0x62,0x7D,0xE1,0x78,0x50,
    0xB0,0x7A,0xF3,0x6C,0xCE,0xA2,0x02,0xE1,0x08,0xE4,0x39,0xF9,0xC0,0x4A,0xE4,0x6C,
    0x13,0x44,0xC8,0xFA,0xE8,0x41,0xE0,0xB3,0x72,0xF0,0xBA,0x00,0xBD,0x16,0x53,0xEC,
    0x50,0x30,0xE3,0x03,0xF2,0x19,0xF8,0x72,0x62,0x49,0x5D,0x97,0xC4,0x0D,0xCA,0x92,
    0x3F,0x54,0xCC,0x05,0x4B,0x86,0x5D,0xD8,0xAB,0x9D,0xDE,0x8E,0xB4,0xFB,0x3E,0xD0,
    0x29,0x23,0xEA,0xE9,0xEC,0xCA,0xFF,0x2F,0xF6,
};

void send_compile_time(void)
{
    uint8_t build_time[] = __DATE__ " " __TIME__;

    uart_send_bytes_for_ate(build_time, sizeof(build_time));
}

void send_chip_id(void)
{
    unsigned long chip_id;
    UINT8 tx_buffer[16];
    chip_id = REG_SYS_CTRL_CHIP_ID;
    tx_buffer[0] = (chip_id >> 24) & 0x00FF;
    tx_buffer[1] = (chip_id >> 16) & 0x00FF;
    tx_buffer[2] = (chip_id >> 8) & 0x00FF;
    tx_buffer[3] = chip_id & 0x00FF;
    uart_send_bytes_for_ate(tx_buffer, 4);

}

void send_flash_id(void)
{
    UINT8 tx_buffer[8];
    unsigned long ulFlashID;

    ulFlashID = bk_flash_get_id();
    tx_buffer[0] = (ulFlashID >> 24) & 0x00FF;
    tx_buffer[1] = (ulFlashID >> 16) & 0x00FF;
    tx_buffer[2] = (ulFlashID >> 8) & 0x00FF;
    tx_buffer[3] = ulFlashID & 0x00FF;
    uart_send_bytes_for_ate(tx_buffer, 4);
}

void send_compile_time_hex(void)
{
    const char *date = __DATE__;  // e.g. "jul 23 2025"
    const char *time = __TIME__;  // e.g. "14:37:12"

    static const char month_table[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    uint8 month = 0;
    for (size_t i = 0; i < 12; i++) {
        if (os_memcmp(date, month_table[i], 3) == 0) {
            month = i + 1;
            month = month / 10 * 16 + month % 10;
            break;
        }
    }

    uint8_t year = 0;
    for (size_t i = 9; i < 11; i++) {
        if (date[i] >= '0' && date[i] <= '9') {
            year = year * 10 + (date[i] - '0');
        }
    }
    year = year / 10 * 16 + year % 10;

    uint8_t day = 0;
    for (size_t i = 4; i < 6; i++) {
        if (date[i] >= '0' && date[i] <= '9') {
            day = day * 10 + (date[i] - '0');
        }
    }
    day = day / 10 * 16 + day % 10;

    uint8_t hour = (time[0] - '0') * 10 + (time[1] - '0');
    uint8_t min  = (time[3] - '0') * 10 + (time[4] - '0');
    uint8_t sec  = (time[6] - '0') * 10 + (time[7] - '0');

    hour = hour / 10 * 16 + hour % 10;
    min  = min  / 10 * 16 + min  % 10;
    sec  = sec  / 10 * 16 + sec  % 10;

    UINT8 tx_buffer[16];
    tx_buffer[0] = (0x00  >> 0) & 0xFF;
    tx_buffer[1] = (0x00  >> 0) & 0xFF;
    tx_buffer[2] = (year  >> 0) & 0xFF;
    tx_buffer[3] = (month >> 0) & 0xFF;
    tx_buffer[4] = (day   >> 0) & 0xFF;
    tx_buffer[5] = (hour  >> 0) & 0xFF;
    tx_buffer[6] = (min   >> 0) & 0xFF;
    tx_buffer[7] = (sec   >> 0) & 0xFF;
    uart_send_bytes_for_ate(tx_buffer, 8);
}

void wifi_cali_dpll_ate(void)
{
    UINT8 flag = 0;
    UINT8 tx_buffer[2];

#if CONFIG_DPLL_UNLOCK_VERIFY_ENABLE
    /* use cpu clock 26M, do not use dpll */
    sys_ll_set_cpu_clk_div_mode1_cksel_core(0);
    sys_ll_set_cpu_clk_div_mode1_clkdiv_core(0);
#endif

    for(UINT8 i = 0; i < 3; i++)
    {
        if(sys_drv_cali_dpll_ate() != 0)
        {
            flag += 1;
        }
    }
    tx_buffer[0] = 0x55;
    if(flag < 2)
    {
        tx_buffer[1] = 0x33;
    }
    else
    {
        tx_buffer[1] = 0xCC;
    }
    uart_send_bytes_for_ate(tx_buffer, 2);
}

void wifi_cali_rfpll_ate(void)
{
    #define PRINT_RESULT_ENABLE  1
    UINT8 tx_buffer[16];
    __maybe_unused uint32_t bandmin_wifi = 0;
    __maybe_unused uint32_t bandmax_wifi = 0;
    __maybe_unused uint32_t bandmin_lp = 0;
    __maybe_unused uint32_t bandmax_lp = 0;

    tx_buffer[0] = 0x55;

    uint32_t err_mask = bk_rfpll_ate_test(&bandmin_wifi, &bandmax_wifi, &bandmin_lp, &bandmax_lp);

    if(err_mask == 0)
    {
        tx_buffer[1] = 0x33;
#if (PRINT_RESULT_ENABLE)
        tx_buffer[2] = (bandmin_wifi >> 8) & 0x00FF;
        tx_buffer[3] = bandmin_wifi & 0x00FF;
        tx_buffer[4] = (bandmax_wifi >> 8) & 0x00FF;
        tx_buffer[5] = bandmax_wifi & 0x00FF;
        tx_buffer[6] = (bandmin_lp >> 8) & 0x00FF;
        tx_buffer[7] = bandmin_lp & 0x00FF;
        tx_buffer[8] = (bandmax_lp >> 8) & 0x00FF;
        tx_buffer[9] = bandmax_lp & 0x00FF;
        uart_send_bytes_for_ate(tx_buffer, 10);
#else
        uart_send_bytes_for_ate(tx_buffer, 2);
#endif
    }
    else
    {
        tx_buffer[1] = 0xCC;
        tx_buffer[2] = (0x00 | (err_mask & 0x0F));
#if (PRINT_RESULT_ENABLE)
        tx_buffer[3] = (bandmin_wifi >> 8) & 0x00FF;
        tx_buffer[4] = bandmin_wifi & 0x00FF;
        tx_buffer[5] = (bandmax_wifi >> 8) & 0x00FF;
        tx_buffer[6] = bandmax_wifi & 0x00FF;
        tx_buffer[7] = (bandmin_lp >> 8) & 0x00FF;
        tx_buffer[8] = bandmin_lp & 0x00FF;
        tx_buffer[9] = (bandmax_lp >> 8) & 0x00FF;
        tx_buffer[10] = bandmax_lp & 0x00FF;
        uart_send_bytes_for_ate(tx_buffer, 11);
#else
        uart_send_bytes_for_ate(tx_buffer, 3);
#endif
    }
}

void send_device_id(void)
{

    UINT8 tx_buffer[16];

    tx_buffer[0] = (device_id >> 24) & 0x00FF;
    tx_buffer[1] = (device_id >> 16) & 0x00FF;
    tx_buffer[2] = (device_id >> 8) & 0x00FF;
    tx_buffer[3] = device_id & 0x00FF;
    uart_send_bytes_for_ate(tx_buffer, 4);
}

void gpio_voltage_output_high(void)
{
	uint32_t  i = 0;

	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD)
			continue;
		bk_gpio_set_value(i, 0x02000002);
	}
}

void gpio_voltage_output_low(void)
{
	uint32_t  i = 0;

	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD)
			continue;
		bk_gpio_set_value(i, 0x02000000);
	}
}

void gpio_voltage_input_high(void)
{
	uint32_t  i = 0;
	uint32_t val = 0;
	uint8_t *data = (uint8_t *)os_zalloc(GPIO_NUM_MAX);
	uint8_t *restore_reg = (uint8_t *)os_zalloc(GPIO_NUM_MAX);

	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD)
			continue;
		restore_reg[i] = bk_gpio_get_value(i);
		// input enable, pull enable, pull = pulldown
		bk_gpio_set_value(i, 0x01000020);
	}

	//uart_write_byte_for_ate(0, (uint8_t *)restore_reg, GPIO_NUM_MAX);
	bk_delay_us(1000);

	data[0] = 0x04;
	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD) {
			continue;
		} else {
			val = bk_gpio_get_value(i);
			if(val & 0x1) {
				data[(1 + i/8)] |= (0x1 << (i % 8));
			} else {
				data[(1 + i/8)] &= ~(0x1 << (i % 8));
			}
		}
	}
	uart_write_byte_for_ate(0, (uint8_t *)data, (1 + GPIO_NUM_MAX/8));
	if (data) {
		os_free(data);
	}

	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD)
			continue;
		bk_gpio_set_value(i, restore_reg[i]);
	}
	if (restore_reg) {
		os_free(restore_reg);
	}

}

void gpio_voltage_input_low(void)
{
	uint32_t  i = 0;
	uint32_t val = 0;
	uint8_t *data = (uint8_t *)os_zalloc(GPIO_NUM_MAX);
	uint8_t *restore_reg = (uint8_t *)os_zalloc(GPIO_NUM_MAX);

	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD)
			continue;
		restore_reg[i] = bk_gpio_get_value(i);
		// input enable, pull enable, pull = pullup
		bk_gpio_set_value(i, 0x01000030);
	}

	//uart_write_byte_for_ate(0, (uint8_t *)restore_reg, GPIO_NUM_MAX);
	bk_delay_us(1000);

	data[0] = 0x05;
	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD) {
			continue;
		} else {
			val = bk_gpio_get_value(i);
			if(val & 0x1) {
				data[(1 + i/8)] |= (0x1 << (i % 8));
			} else {
				data[(1 + i/8)] &= ~(0x1 << (i % 8));
			}
		}
	}
	uart_write_byte_for_ate(0, (uint8_t *)data, (1 + GPIO_NUM_MAX/8));
	if (data) {
		os_free(data);
	}

	for(i = 0; i < GPIO_NUM_MAX; i ++)
	{
		if(i == ATE_GPIO_DEV_UART1_RXD || i == ATE_GPIO_DEV_UART1_TXD)
			continue;
		bk_gpio_set_value(i, restore_reg[i]);
	}
	if (restore_reg) {
		os_free(restore_reg);
	}
}

void get_device_id(void)
{
    device_id = REG_SYS_CTRL_DEVICE_ID;
}

void mpb_tx_mode_close_for_ate(void)
{
	UINT32 reg;
	reg = bk_wifi_get_mpb_ctrl();
	reg &= 0x00;
	bk_wifi_set_mpb_ctrl(reg);
}

typedef struct {
    UINT8 channel;
    UINT8 pwr_idx;
    UINT8 pwr_dBm_x10;
} cal_point_t;

static cal_point_t g_2g_11b_cal_points[3] = {
    {1, 0, 0},   // ch1
    {7, 0, 0},   // ch7
    {13, 0, 0}   // ch13
};

static cal_point_t g_2g_11g_cal_points[3] = {
    {1, 0, 0},   // ch1
    {7, 0, 0},   // ch7
    {13, 0, 0}   // ch13
};

static cal_point_t g_5g_11a_cal_points[5] = {
    {36, 0, 0},   // ch36
    {64, 0, 0},   // ch64
    {100, 0, 0},  // ch100
    {132, 0, 0},  // ch132
    {165, 0, 0}   // ch165
};

static int find_valid_cal_points(cal_point_t *cal_points, int num_points, cal_point_t *valid_points, int *num_valid)
{
    int count = 0;
    for (int i = 0; i < num_points; i++) {
        if (cal_points[i].pwr_idx != 0 && cal_points[i].pwr_dBm_x10 != 0) {
            valid_points[count] = cal_points[i];
            count++;
        }
    }
    *num_valid = count;
    return count;
}

static UINT8 linear_interpolate(UINT8 x1, UINT8 y1, UINT8 x2, UINT8 y2, UINT8 x)
{
    if (x1 == x2) {
        return y1;
    }

    INT32 result = y1 + ((INT32)(y2 - y1) * (INT32)(x - x1)) / (INT32)(x2 - x1);
    if (result < 0) {
        bk_printf("Warning: linear_interpolate() result < 0, clamped to 0\n");
        return 0;
    }
    if (result > 255) {
        bk_printf("Warning: linear_interpolate() result > 255, clamped to 255\n");
        return 255;
    }
    return (UINT8)result;
}

static UINT8 fit_power_index_generic(UINT8 target_channel, UINT8 target_pwr_dBm_x10, cal_point_t *cal_points, int num_points)
{
    cal_point_t valid_points[5];
    int num_valid = 0;

    find_valid_cal_points(cal_points, num_points, valid_points, &num_valid);

    if (num_valid == 0) {
        return 0;
    }

    // Base values (idx and power) at target channel from calibration data
    UINT8 base_idx;
    UINT8 base_dbm_x10;

    if (num_valid == 1) {
        base_idx = valid_points[0].pwr_idx;
        base_dbm_x10 = valid_points[0].pwr_dBm_x10;
    } else {
        // Clamp to edges if out of range, otherwise interpolate between neighbors
        if (target_channel <= valid_points[0].channel) {
            base_idx = valid_points[0].pwr_idx;
            base_dbm_x10 = valid_points[0].pwr_dBm_x10;
        } else if (target_channel >= valid_points[num_valid - 1].channel) {
            base_idx = valid_points[num_valid - 1].pwr_idx;
            base_dbm_x10 = valid_points[num_valid - 1].pwr_dBm_x10;
        } else {
            cal_point_t left_point = valid_points[0];
            cal_point_t right_point = valid_points[num_valid - 1];
            for (int i = 0; i < num_valid - 1; i++) {
                if (target_channel >= valid_points[i].channel && target_channel <= valid_points[i + 1].channel) {
                    left_point = valid_points[i];
                    right_point = valid_points[i + 1];
                    break;
                }
            }
            base_idx = linear_interpolate(left_point.channel, left_point.pwr_idx,
                                          right_point.channel, right_point.pwr_idx,
                                          target_channel);
            base_dbm_x10 = linear_interpolate(left_point.channel, left_point.pwr_dBm_x10,
                                              right_point.channel, right_point.pwr_dBm_x10,
                                              target_channel);
        }
    }

    // Adjust index by target power difference: 10 (1.0dBm) -> idx delta 4
    INT32 delta_dbm_x10 = (INT32)target_pwr_dBm_x10 - (INT32)base_dbm_x10;
    INT32 idx_adjust = (delta_dbm_x10 * 4) / 10;
    INT32 final_idx = (INT32)base_idx + idx_adjust;

    if (final_idx < 0) {
        bk_printf("Warning: final_idx < 0 (%d), clamp to 0\r\n", final_idx);
        final_idx = 0;
    } else if (final_idx > 255) {
        bk_printf("Warning: final_idx > 255 (%d), clamp to 255\r\n", final_idx);
        final_idx = 255;
    }

    return (UINT8)final_idx;
}

static void evm_test_save_pwr(UINT32 rate, UINT32 frequency, uint8_t pwr_dbm)
{
    uint8_t tx_buffer[3];

    UINT8 pwr_idx = rwnx_cal_get_pwr_idx();
    bk_printf("frequency=%d,rate=%d,pwr_idx=%d,pwr_dbm=%d\r\n", frequency, rate, pwr_idx, pwr_dbm);

    if (rate == 11) {
        if (frequency == 2412) {
            g_2g_11b_cal_points[0].pwr_idx = pwr_idx;
            g_2g_11b_cal_points[0].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 2442) {
            g_2g_11b_cal_points[1].pwr_idx = pwr_idx;
            g_2g_11b_cal_points[1].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 2472) {
            g_2g_11b_cal_points[2].pwr_idx = pwr_idx;
            g_2g_11b_cal_points[2].pwr_dBm_x10 = pwr_dbm;
        }
    } else if (rate == 54) {
        if (frequency == 2412) {
            g_2g_11g_cal_points[0].pwr_idx = pwr_idx;
            g_2g_11g_cal_points[0].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 2442) {
            g_2g_11g_cal_points[1].pwr_idx = pwr_idx;
            g_2g_11g_cal_points[1].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 2472) {
            g_2g_11g_cal_points[2].pwr_idx = pwr_idx;
            g_2g_11g_cal_points[2].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 5180) {
            g_5g_11a_cal_points[0].pwr_idx = pwr_idx;
            g_5g_11a_cal_points[0].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 5320) {
            g_5g_11a_cal_points[1].pwr_idx = pwr_idx;
            g_5g_11a_cal_points[1].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 5500) {
            g_5g_11a_cal_points[2].pwr_idx = pwr_idx;
            g_5g_11a_cal_points[2].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 5660) {
            g_5g_11a_cal_points[3].pwr_idx = pwr_idx;
            g_5g_11a_cal_points[3].pwr_dBm_x10 = pwr_dbm;
        } else if (frequency == 5825) {
            g_5g_11a_cal_points[4].pwr_idx = pwr_idx;
            g_5g_11a_cal_points[4].pwr_dBm_x10 = pwr_dbm;
        }
    }

    tx_buffer[0] = 0x55;
    tx_buffer[1] = 0x33;
    tx_buffer[2] = pwr_dbm;

    uart_send_bytes_for_ate(tx_buffer, 3);
}

/**
 * rate        : reference to evm_translate_tx_rate
 * channel     : [2400, 2655]
 * bandwidth   : reference to mac_chan_bandwidth
 * modul_format: reference to ModulationFormat
 */
static bool g_evm_test_special_channel = false;
static void evm_test(UINT32 rate, UINT32 frequency, UINT32 bandwidth, UINT32 modul_format, UINT8 pwr_dbm, wifi_band_t band)
{
    UINT32 packet_len;
    UINT32 duty_cycle;
    UINT32 gi;

    if (g_evm_test_special_channel == true) {
        bk_revert_spectial_channel_for_ate();
        g_evm_test_special_channel = false;
    }

    evm_stop_bypass_mac();

    mdm_set_scramblerctrl(0x83); //change from 0x85 to 0x83 by cunliang
    //set channel before init macbypass, for bk7236 need it.
    evm_bypass_mac_test_with_frequency(frequency, band, bandwidth);
    evm_init_bypass_mac();

#if(CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
    evm_set_pwr_for_he_tb(modul_format);
#endif

    if (frequency < 2400 && g_evm_test_special_channel == false) {
        bk_set_spectial_channel_for_ate(frequency);
        g_evm_test_special_channel = true;
    }

    mdm_set_txstartdelay(1023);
    mdm_set_txcheckack(14400);

    if (rate <= 54) {
        modul_format = 0;
    }

    // packet_len = evm_get_auto_tx_len(rate, modul_format, bandwidth, 0);
    packet_len = 1;
    packet_len = packet_len * 100;

    evm_bypass_mac_set_rate_mformat(rate, modul_format);
    evm_set_bandwidth(bandwidth);

    if ((modul_format == 5) || (modul_format == 8))
        gi = AX_DEFUALT_GI_TYPE;
    else
        gi = DEFUALT_GI_TYPE;

    if (modul_format == 8) {
        if (gi == 0x1 || gi == 0x2)
            packet_len = evm_get_auto_hetb_tx_len(0, rate, gi);
        bk_printf("HE-TB packet_len : %d\n", packet_len);
    }
    if (packet_len != 0)
        evm_bypass_mac_set_tx_data_length(modul_format, packet_len, rate, bandwidth, 0);
    else {
        packet_len = evm_get_auto_tx_len(rate, modul_format, bandwidth, 0);
        evm_bypass_mac_set_tx_data_length(modul_format, packet_len, rate, bandwidth, 1);
    }

    evm_bypass_mac_set_guard_i_type(gi);
    if ((modul_format >= 5) && (bandwidth == 1)) {
        //HE-40MHz support LDPC only
        evm_bypass_mac_set_fec_coding(1);
    }

    rwnx_cal_en_extra_txpa();
    rf_module_vote_ctrl(RF_OPEN,RF_BY_TEMP_BIT);
    rwnx_cal_set_txpwr_by_rate(evm_translate_tx_rate(rate), modul_format, (UINT32)-1, 0, 0, bandwidth);
    UINT8 pwr_idx = rwnx_cal_get_pwr_idx();
    bk_printf("frequency=%d,rate=%d,pwr_idx=%d\r\n", frequency, rate, pwr_idx);

    rf_module_vote_ctrl(RF_CLOSE,RF_BY_TEMP_BIT);
    //evm_bypass_mac_set_txdelay(txdelay);

    evm_start_bypass_mac();

    bk7011_stop_tx_pattern();

    bk7011_cal_dpll();
    uint32_t duty_in_us;
    if (rate < 128)
    {
        /* long preamble always on since MSB of verctor4 in macbypass */
        duty_in_us = hal_machw_frame_duration_ate(bandwidth, modul_format, evm_translate_tx_rate(rate), 1, gi, packet_len);
    }
    else
        duty_in_us = hal_machw_frame_duration_ate(bandwidth, modul_format, rate - 128, 1, gi, packet_len);
    /* keep duty cycle 50% */
    duty_cycle = 50;
    mpb_set_txdelay(duty_in_us * (100 - duty_cycle) / duty_cycle);
}

static void evm_single_test(UINT32 rate, UINT32 channel, UINT32 bandwidth)
{
    UINT32 modul_format = 0;
    UINT32 packet_len;
    UINT32 gi;
    UINT32 duty_in_us;

    evm_stop_bypass_mac_for_ate();
    rwnx_no_use_tpc_set_pwr();

    mdm_set_scramblerctrl(0x83); //change from 0x85 to 0x83 by cunliang

    //set channel before init macbypass, for bk7236 need it.
    evm_bypass_mac_test(channel, IEEE80211_BAND_2GHZ, bandwidth);
    evm_init_bypass_mac();

    if (rate <= 54)
        modul_format = 0;
    else if (modul_format < 2) {
        //BK_LOGD(NULL, "HT/VHT/HE pkts modul_format >= 2\r\n");
        modul_format = 2;
    }

    packet_len = evm_get_auto_tx_len(rate, modul_format, bandwidth, 0);
    evm_bypass_mac_set_tx_data_length_for_ate(modul_format, packet_len, rate, bandwidth, 1);

    evm_bypass_mac_set_rate_mformat_for_ate(rate, modul_format);
    evm_set_bandwidth(bandwidth);

    if (modul_format == 5)
        gi = AX_DEFUALT_GI_TYPE;
    else
        gi = DEFUALT_GI_TYPE;

    evm_bypass_mac_set_guard_i_type(gi);

    rwnx_cal_en_extra_txpa();
    // set g_single_carrier ahead of set txpwr, for bk7236 need it
    g_single_carrier = 1;

    rf_module_vote_ctrl(RF_OPEN,RF_BY_TEMP_BIT);
    rwnx_cal_set_txpwr_by_rate(evm_translate_tx_rate(rate), 0, (UINT32)-1, 0, 0, 0);
    rf_module_vote_ctrl(RF_CLOSE,RF_BY_TEMP_BIT);;
    //evm_bypass_mac_set_txdelay(txdelay);

    evm_start_bypass_mac_for_ate();

    // bk7236,no need do it anymore
    evm_bypass_set_single_carrier(rate, IEEE80211_BAND_2GHZ, bandwidth, channel);

    bk7011_cal_dpll();

    if (rate < 128)
    {
        /* long preamble always on since MSB of verctor4 in macbypass */
        duty_in_us = hal_machw_frame_duration_ate(bandwidth, modul_format, evm_translate_tx_rate(rate), 1, gi, packet_len);
    }
    else
        duty_in_us = hal_machw_frame_duration_ate(bandwidth, modul_format, rate - 128, 1, gi, packet_len);
    /* keep duty cycle 10% */
    mpb_set_txdelay(duty_in_us * 9);
}

void tx_verify_test_call_back(void)
{
    uint32_t ret, i, *word_ptr, frame_len = 0, golden_size;
    UINT8* sample_buf = NULL, sample, golden;

    evm_del_key(hw_key_idx);

    ret = bk7011_get_sample_mac_out_for_ate(&sample_buf, &frame_len);
    if(ret != 0)
    {
        //os_printf("get sample fail %d\r\n", ret);
        bk7011_clear_sample_mac_out_for_ate();
        TX_ENCRPYT_RESULT_PIN(0);
        return;
    }

    word_ptr = (uint32_t *)sample_buf;
    #if 0
    for(i = 0; i < 6; i ++)
    {
        os_printf("%02x,", (word_ptr[i] >> 13) & 0xff );
    }
    os_printf("\r\n");

    for(i = 6; i < frame_len; i ++)
    {
        os_printf("%02x,", (word_ptr[i] >> 13) & 0xff );
        if((i - 6 + 1) % 16 == 0)
            os_printf("\r\n");
    }

    bk7011_clear_sample_mac_out();
    TX_ENCRPYT_EN_PIN(0);
    #else
    golden_size = sizeof(tx_golden);
    ret = 0;

    if(golden_size != frame_len)
    {
        //os_printf("len not match:%d, %d\r\n", golden_size, frame_len);
        ret |= 1;
    }

    for(i = 0; i < golden_size; i ++)
    {
        sample = ((word_ptr[i] >> 24) & 0xff);
        golden = tx_golden[i];

        if((sample != golden) && (i < 12 || i > 13))
        {
            ret |= 2;
            //os_printf("data not match:%d, %02x, %02x\r\n", i, sample, golden);
            break;
        }
    }

    bk7011_clear_sample_mac_out_for_ate();
    if(ret != 0)
    {
        //os_printf("tx verify fail:%01x\r\n", ret);
        TX_ENCRPYT_RESULT_PIN(0);
    }
    else
    {
        //os_printf("tx verify pass\r\n");
        TX_ENCRPYT_RESULT_PIN(2);
    }
    TX_ENCRPYT_EN_PIN(0);
    #endif
}

static void tx_verify_test_2442(void)
{
    uint32_t frame_len, ret;

    TX_ENCRPYT_EN_PIN(2);
    frame_len = 255;

    evm_phy_init(2462, IEEE80211_BAND_2GHZ, PHY_CHNL_BW_20);

    hw_key_idx = evm_add_key();
    phy_close_cca();

    ret = bk7011_set_sample_mac_out_for_ate(frame_len);
    if(ret != 0)
    {
        //os_printf("set sample fail %d\r\n", ret);
        TX_ENCRPYT_EN_PIN(0);
        return;
    }

    ret = evm_req_tx_for_ate(frame_len);
    if(ret != 0)
    {
        //os_printf("req tx fail %d\r\n", ret);
        bk7011_clear_sample_mac_out_for_ate();
        TX_ENCRPYT_EN_PIN(0);
        return;
    }
}

void rxsens_ct_hdl_for_ate(void *param)
{
	bk_err_t err;
	rx_get_rx_result_end_for_ate(param, NULL);
	rx_get_rx_result_begin_for_ate();

	if (rx_sens_ate_tmr.handle != NULL) {
		err = rtos_oneshot_reload_timer(&rx_sens_ate_tmr);
		BK_ASSERT(kNoErr == err);
	}
}

#define ERROR_2380_LOCK_FAILED                    (-1)
#define ERROR_FREQUENCY_LOCK_FAILED               (-2)
#define ERROR_FREQUENCY_LOCK_FAILED_FIRST_TIME    (-3)
static int per_test(uint32_t bandwidth, uint32_t frequency, uint32_t duration, uint32_t arg, wifi_band_t band, RF_MODE rf_mode)
{
    UINT8 ret;
    int err;
    UINT32 stat_reset = 1;
    UINT8 tx_buffer[8] = {0};

    rwnx_cal_set_rfband(band, rf_mode);

    ret = rs_test_for_mbp_with_frequency(frequency, band, bandwidth);
    if (ret)
        return 1;

    if (frequency < 2400) {
        ret = bk_set_spectial_channel_for_ate(frequency);
        if (ret < 0) {
            goto exit;
        }
    }

    // TODO: BK7236 phy karst
    if (bandwidth == RXSENS_BANDWIDTH_40MHZ)
        rwnx_cal_set_40M_extra_setting(1);
    else
        rwnx_cal_set_40M_extra_setting(0);

    g_rxsens_start = 1;

    if (duration)
    {
        if (rx_sens_ate_tmr.handle != NULL)
        {
            err = rtos_deinit_oneshot_timer(&rx_sens_ate_tmr);
            BK_ASSERT(kNoErr == err);
            rx_sens_ate_tmr.handle = NULL;
        }

        err = rtos_init_oneshot_timer(&rx_sens_ate_tmr, duration, (timer_2handler_t)rx_get_rx_result_end_for_mbp_ate, (void *)arg, (void *)stat_reset);
        BK_ASSERT(kNoErr == err);
        err = rtos_start_oneshot_timer(&rx_sens_ate_tmr);
        BK_ASSERT(kNoErr == err);
    }
    else
    {
        if (rx_sens_ate_tmr.handle != NULL)
        {
            err = rtos_deinit_oneshot_timer(&rx_sens_ate_tmr);
            BK_ASSERT(kNoErr == err);
            rx_sens_ate_tmr.handle = NULL;
        }
    }
exit:
    if (frequency < 2400) {
        bk_revert_spectial_channel_for_ate();

        if (ret == ERROR_2380_LOCK_FAILED) {
            tx_buffer[0] = 0x55;
            tx_buffer[1] = 0xCC;
            tx_buffer[2] = 0x01;
            uart_send_bytes_for_ate(tx_buffer, 3);
        } else if (ret == ERROR_FREQUENCY_LOCK_FAILED) {
            tx_buffer[0] = 0x55;
            tx_buffer[1] = 0xCC;
            tx_buffer[2] = 0x02;
            uart_send_bytes_for_ate(tx_buffer, 3);
        } else if (ret == ERROR_FREQUENCY_LOCK_FAILED_FIRST_TIME) {
            tx_buffer[0] = 0x55;
            tx_buffer[1] = 0xCC;
            tx_buffer[2] = 0x03;
            uart_send_bytes_for_ate(tx_buffer, 3);
        }
    }
    return 0;
}

static void ate_read_otp(const uint8_t *content, int cnt, uint8_t *tx_buffer1)
{
    uint32_t length;
    UINT8 tx_buffer[100];
    int ret = 0;

    if (content[2] == 0x01) {
        /* otp1 use apb_read */
        if (content[3] == OTP_MEMORY_CHECK_MARK) {
            // No area for crc
            length = 60;
        } else if (content[3] == OTP_EFUSE) {
            // No area for crc
            length = 4;
        } else if (content[3] == OTP_MEMORY_CHECK_MARK_EXTRA) {
            // No area for crc
            length = 8;
        } else {
            length = 0;
        }
    } else if (content[2] == 0x02) {
        /* otp2 use ahb_read */
        if (content[3] == OTP_VDDDIG_BANDGAP) {
            length = 1;
        } else if (content[3] == OTP_DIA) {
            // 2G PA(1 byte) + 5G PA(1 byte)
            length = 2;
        } else if (content[3] == OTP_GADC_CALIBRATION) {
            // saradc_self_cali(16 * 4 bytes) + saradc_volt_cali(3 * 2 bytes)
            length = 70;
        } else if (content[3] == OTP_DEVICE_ID) {
            // Note: "DEVICE_ID" = "UID"
            // Batch_Num(2 bytes) + Wafer_ID(1 byte) + X(1 byte) + Y(1 byte)
            length = 5;
        } else if (content[3] == OTP_MEMORY_CHECK_VDDDIG) {
            // No area for crc
            length = 4;
        } else if (content[3] == OTP_GADC_TEMPERATURE) {
            // temp code1(2 bytes) + temp code2(2 bytes)
            length = 4;
        } else if (content[3] == OTP_PHY_PWR1) {
            // 2.4G CH1 pwr_idx = 1(11b_11M) + 1(11g_54M) + 2(res)
            // 2.4G CH7 pwr_idx = 1(11b_11M) + 1(11g_54M) + 2(res)
            // 2.4G CH13 pwr_idx = 1(11b_11M) + 1(11g_54M) + 2(res)
            // 2.4G CH1 pwr_dBm_x10 = 1(11b_11M) + 1(11g_54M) + 2(res)
            // 2.4G CH7 pwr_dBm_x10 = 1(11b_11M) + 1(11g_54M) + 2(res)
            // 2.4G CH13 pwr_dBm_x10 = 1(11b_11M) + 1(11g_54M)+ 2(res)
            // 5G 11a 54M CH36 pwr = 1(pwr_idx) + 1(pwr_dBm_x10) + 2(res)
            // 5G 11a 54M CH64 pwr = 1(pwr_idx) + 1(pwr_dBm_x10) + 2(res)
            // 5G 11a 54M CH100 pwr = 1(pwr_idx) + 1(pwr_dBm_x10) + 2(res)
            // 5G 11a 54M CH132 pwr = 1(pwr_idx) + 1(pwr_dBm_x10) + 2(res)
            // 5G 11a 54M CH165 pwr = 1(pwr_idx) + 1(pwr_dBm_x10) + 2(res)
            // Firmware Version = 1(tbl_ver) + 1(ana_ver)
            length = 46;
        } else if (content[3] == OTP_PHY_PWR2) {
            // OTP_PHY_PWR1 = OTP_PHY_PWR2
            length = 46;
        } else if (content[3] == OTP_PRODUCT_ID) {
            // Product_ID(15 bytes)
            length = 15;
        } else if (content[3] == OTP_FACTORY_ID) {
            // Factory_ID(3 bytes)
            length = 3;
        } else {
            length = 0;
        }
    } else {
        length = 0;
    }

    if (length) {
        tx_buffer[0] = 0x0E;
        os_memset(tx_buffer + 1, 0x00, length);
        if (content[2] == 0x01) {
            ret = bk_otp_apb_read(content[3], tx_buffer + 1, length);
        } else {
            // here just for content[2] == 0x02
            ret = bk_otp_ahb_read(content[3], tx_buffer + 1, length);
        }
        if (ret != BK_OK) {
            // otp read fail
            tx_buffer[0] = 0x55;
            tx_buffer[1] = 0xBB;
            uart_send_bytes_for_ate(tx_buffer, 2);
        } else {
            uart_send_bytes_for_ate(tx_buffer, length + 1);
        }
    }

}

#if (CONFIG_OTP)
typedef enum {
    OTP_OP_READ  = 0x01u,
    OTP_OP_WRITE = 0x02u,
} ate_otp_op_t;

static bool otp_check_crc8(uint8_t* buf, size_t buflen)
{
    CRC8_Context crc8;
    uint8_t cal_value;
    uint8_t store_value = buf[buflen - 1];
    CRC8_Init(&crc8);
    CRC8_Update(&crc8, buf, buflen - 1);
    CRC8_Final(&crc8, &cal_value);
    if (cal_value == store_value) {
        return true;
    } else {
        return false;
    }
}

static void ate_otp_write_common(uint32_t otp_addr, const uint8_t *content, uint32_t content_len)
{
    uint8_t tx_local[2 + 32] = {0};

    tx_local[0] = 0x55;
    tx_local[1] = 0xCC;

    // CRC check with debug output
    uint32_t data_len = content_len - 1;  // data length (excluding CRC byte)

    if (!otp_check_crc8((uint8_t*)content, content_len)) {
        bk_printf("ate_otp_write_common: crc check failed total_len=%d\r\n", content_len);
        goto fail;
    }
    if (bk_otp_ahb_update(otp_addr, (uint8_t *)content, content_len) != BK_OK) {
        bk_printf("ate_otp_write_common: ahb_update failed addr=0x%X len=%d\r\n", otp_addr, content_len);
        goto fail;
    }
    /* success: update already verifies internally, no extra readback needed */
    tx_local[1] = 0x33;
    bk_otp_ahb_read(otp_addr, tx_local + 2, data_len);
    uart_send_bytes_for_ate(tx_local, 2 + data_len);
    return;

fail:
    tx_local[0] = 0x55;
    tx_local[1] = 0xCC;
    uart_send_bytes_for_ate(tx_local, 2);
}

static void ate_otp_read_common(uint32_t otp_addr, uint32_t read_len)
{
    uint8_t tx_local[1 + 32] = {0};
    int ret;

    tx_local[0] = 0x0E;

    ret = bk_otp_ahb_read(otp_addr, tx_local + 1, read_len);
    if (ret != BK_OK) {
        bk_printf("ate_otp_read_common: ahb_read failed addr=0x%X read_len=%d ret=%x\r\n", otp_addr, read_len, ret);
        goto fail;
    }

    uart_send_bytes_for_ate(tx_local, read_len + 1);
    return;

fail:
    tx_local[0] = 0x55;
    tx_local[1] = 0xBB;
    uart_send_bytes_for_ate(tx_local, 2);
}

static void ate_otp_rw_common(uint32_t otp_addr, const uint8_t *content, uint32_t len, ate_otp_op_t op)
{
    if (op == OTP_OP_READ) {
        ate_otp_read_common(otp_addr, len);
    }
    if (op == OTP_OP_WRITE) {
        ate_otp_write_common(otp_addr, content, len);
    }
}
#endif

static void ate_pm_gpio_callback(gpio_id_t gpio_id)
{
	#if CONFIG_SYS_CPU0
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,100000000);
	bk_ate_ctrl(1);
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_ON);
	#if (CONFIG_CPU_CNT > 1)
	extern void start_cpu1_core(void);
	start_cpu1_core();
	#endif
	#endif
}
static void ate_pm_exit_lvsleep(uint64_t sleep_time, void *args)
{

}
int ate_enter_low_voltage()
{
#if (CONFIG_SYS_CPU0)
	pm_cb_conf_t exit_config = {
	.cb = (pm_cb)ate_pm_exit_lvsleep,
	.args = NULL
	};
    	bk_ate_ctrl(0);
    bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_DEFAULT, NULL, &exit_config);

	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_BTSP,0x1,100000000);
	// bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_WIFIP_MAC,0x1,100000000);

	// bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_OFDM, PM_POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_ENCP, PM_POWER_MODULE_STATE_OFF);
	// bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_BAKP,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BTSP,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_THREAD,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_WIFIP_MAC,POWER_MODULE_STATE_OFF);

	///close wifi pll
	rf_pll_ctrl(CMD_RF_WIFIPLL_HOLD_BIT_CLR,RF_WIFIPLL_HOLD_BY_WIFI_BIT);
	rf_pll_ctrl(CMD_RF_WIFIPLL_HOLD_BIT_CLR,RF_WIFIPLL_HOLD_BY_BLE_BIT);

	///close phy power domain
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_PHY_WIFI,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_PHY_BT,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_PHY_RF,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_PHY_THREAD,POWER_MODULE_STATE_OFF);

	///close rf
	rf_module_vote_ctrl(RF_CLOSE, RF_BY_WIFI_BIT);
	rf_module_vote_ctrl(RF_CLOSE, RF_BY_BLE_BIT);
	rf_module_vote_ctrl(RF_CLOSE, RF_BY_ATE_WIFI_BIT);

#if (CONFIG_CPU_CNT > 1)
	extern void stop_cpu1_core(void);
	stop_cpu1_core();
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_OFF);
#endif

#if (CONFIG_CPU_CNT > 2)
	extern void stop_cpu2_core(void);
	stop_cpu2_core();
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_OFF);
#endif

#if CONFIG_SOC_BK7236XX
#else
	bk_pm_lpo_src_set(PM_LPO_SRC_ROSC);
#endif
	bk_gpio_register_isr(ATE_GPIO_ID, ate_pm_gpio_callback);
	bk_gpio_register_wakeup_source(ATE_GPIO_ID,GPIO_INT_TYPE_HIGH_LEVEL);
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);

	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,100000000);
#endif
	return 0;
}
int ate_enter_deep_sleep()
{
	bk_ate_ctrl(0);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BTSP,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_THREAD,POWER_MODULE_STATE_OFF);
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_WIFIP_MAC,POWER_MODULE_STATE_OFF);

#if (CONFIG_CPU_CNT > 1)
	extern void stop_cpu1_core(void);
	stop_cpu1_core();
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_OFF);
#endif

#if (CONFIG_CPU_CNT > 2)
	extern void stop_cpu2_core(void);
	stop_cpu2_core();
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_OFF);
#endif

	bk_gpio_register_wakeup_source(ATE_GPIO_ID,GPIO_INT_TYPE_HIGH_LEVEL);
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);

	bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

	return 0;
}

#if CONFIG_MAILBOX
#include <driver/mailbox.h>

#define mailbox_write_data(addr,val)                 *((volatile uint32_t *)(addr)) = val
#define mailbox_get_addr_data(addr)                  *((volatile uint32_t *)(addr))

static int ate_test_cpu1_by_mailbox_cb_flag = 0;
static void ate_test_cpu1_by_mailbox_cb(mailbox_data_t *ate_mbx_data)
{
    if(!ate_test_cpu1_by_mailbox_cb_flag)
        ate_test_cpu1_by_mailbox_cb_flag = 1;

    return;
}

static void ate_test_cpu1_by_mailbox_init(void)
{
    bk_mailbox_init();
    bk_mailbox_recv_callback_register(MAILBOX_CPU1, MAILBOX_CPU0, (mailbox_callback_t)ate_test_cpu1_by_mailbox_cb);
    ate_test_cpu1_by_mailbox_cb_flag = 0;
}

static void ate_test_cpu1_by_mailbox_deinit(void)
{
    bk_mailbox_deinit();
    bk_mailbox_recv_callback_unregister(MAILBOX_CPU1, MAILBOX_CPU0);
}

static void ate_test_cpu1_by_mailbox_case(void)
{
    char tx_buffer[8];

    if(ate_test_cpu1_by_mailbox_cb_flag) {
        tx_buffer[0]='C';//43
        tx_buffer[1]='P';//50
        tx_buffer[2]='U';//55
        tx_buffer[3]='1';//31
        tx_buffer[4]='\0';
        uart_write_string(0, tx_buffer);
    } else
        return;
}

static void ate_test_cpu2_by_mailbox_case(void)
{
    char tx_buffer[8];

    //if(ate_test_cpu2_by_mailbox_cb_flag) {
    if(mailbox_get_addr_data(CONFIG_ATE_CPU2_ADDRESS) == CONFIG_ATE_CPU2_VALUE) {
        tx_buffer[0]='C';//43
        tx_buffer[1]='P';//50
        tx_buffer[2]='U';//55
        tx_buffer[3]='2';//32
        tx_buffer[4]='\0';
        uart_write_string(0, tx_buffer);
    } else
        return;
}

#endif
void ate_test_multiple_cpus_init(void)
{
#if CONFIG_MAILBOX
    ate_test_cpu1_by_mailbox_init();
#endif
}

void ate_test_multiple_cpus_deinit(void)
{
#if CONFIG_MAILBOX
    ate_test_cpu1_by_mailbox_deinit();
#endif
}

static void ate_test_multiple_cpus_case(void)
{
#if CONFIG_MAILBOX
    ate_test_cpu1_by_mailbox_case();
    ate_test_cpu2_by_mailbox_case();
#endif
}

#define write_data(addr,val)                 *((volatile uint32_t *)(addr)) = val
#define get_addr_data(addr)                  *((volatile uint32_t *)(addr))

static void psram_function_test(void)
{
	UINT8 tx_buffer[2];

	tx_buffer[0] = 0x55;
	tx_buffer[1] = 0x32;

#if CONFIG_PSRAM

	bk_psram_init();

	uint32_t base_addr = 0x60000000;
	uint32_t value = 0;
	uint32_t word_len = 1024 * 256;
	uint8_t  flag_error = 0;

	bk_psram_set_clk(PSRAM_120M);

	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x11111111 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x11111111 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

	base_addr = 0x60200000;
	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x22222222 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x22222222 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

	base_addr = 0x60100000;
	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x33333333 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x33333333 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

	base_addr = 0x60300000;
	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x44444444 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x44444444 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

	base_addr = 0x60500000;
	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x55555555 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x55555555 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

	base_addr = 0x60700000;
	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x66666666 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x66666666 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

	base_addr = 0x60400000;
	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x77777777 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x77777777 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

	base_addr = 0x60600000;
	for (uint32_t i = 0; i < word_len; i++) {
		write_data(base_addr + i * 4, 0x88888888 + i);
	}
	for (uint32_t i = 0; i < word_len; i++) {
		value = get_addr_data(base_addr + i * 4);
		if (value != (0x88888888 + i)) {
			flag_error = 1;
			goto exit;
		}
	}

exit:
	if (flag_error) {
		tx_buffer[1] = 0x30;
	} else {
		tx_buffer[1] = 0x31;
	}

#endif

	uart_send_bytes_for_ate(&tx_buffer[0], 2);
}

int sctrl_check_efuse(const unsigned char *content, int cnt, UINT8 *tx_buffer) {
    bk_err_t result = -1;
    int count = 4;

    tx_buffer[0] = 0x0E;
    result = bk_otp_apb_read(OTP_EFUSE, tx_buffer + 1, count);

    if (result != BK_OK) {
        for (int i = 0; i < 4; i++) {
            tx_buffer[i + 1] = 0xFF;
        }
    }

    uart_send_bytes_for_ate(tx_buffer, 1 + count);

    return result;
}

int sctrl_check_otp(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    uint32_t result;

    result = bk_otp_partical_clean_check_customer();

    tx_buffer[0] = 0x0E;
    tx_buffer[1] = result & 0x00FF;
    tx_buffer[2] = (result >> 8) & 0x00FF;
    tx_buffer[3] = (result >> 16) & 0x00FF;
    tx_buffer[4] = (result >> 24) & 0x00FF;
    uart_send_bytes_for_ate(tx_buffer, 5);

    return result;
}

int set_device_id_to_efuse(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    uint8_t device_id_write[EFUSE_DEVICE_ID_BYTE_NUM] = {0};
    uint8_t device_id_buf[8] = {0};
    uint8_t retry_times = 0;
    int ret = 0;

    tx_buffer[0] = 0x55;

    if (cnt == 2) {
        /* return UID or device_id when enter "0x55 99" */
        tx_buffer[1] = 0x88;
        ret = bk_otp_ahb_read(OTP_DEVICE_ID, tx_buffer + 2, EFUSE_DEVICE_ID_BYTE_NUM);
        if (ret != BK_OK) {
            // otp read fail
            tx_buffer[1] = 0x77;
            uart_send_bytes_for_ate(tx_buffer, 2);
            return BK_FAIL;
        } else {
            uart_send_bytes_for_ate(tx_buffer, EFUSE_DEVICE_ID_BYTE_NUM + 2);
            return BK_OK;
        }
    }

    if (cnt < 7) {
        /* return param num error */
        tx_buffer[1] = 0x77;
        tx_buffer[2] = 0x1;
        uart_send_bytes_for_ate(tx_buffer, 3);
        return BK_FAIL;
    }

    for(int iIndex = 0; iIndex < EFUSE_DEVICE_ID_BYTE_NUM; iIndex++) {
        device_id_write[iIndex] = (uint8_t) (content[2 + iIndex] & 0xFF);
    }

    os_memcpy(device_id_buf, device_id_write, EFUSE_DEVICE_ID_BYTE_NUM);

    /* sn2 is out of range */
    if((device_id_write[0] < EFUSE_DEVICE_ID_SN_MINIMUM) || (device_id_write[0] > EFUSE_DEVICE_ID_SN_MAXIMUM)) {
        tx_buffer[1] = 0x77;
        tx_buffer[2] = 0x2;
        uart_send_bytes_for_ate(tx_buffer, 3);
        return BK_FAIL;
    }

    /*  sn1 is out of range */
    if((device_id_write[1] < EFUSE_DEVICE_ID_SN_MINIMUM) || (device_id_write[1] > EFUSE_DEVICE_ID_SN_MAXIMUM)) {
        tx_buffer[1] = 0x77;
        tx_buffer[2] = 0x3;
        uart_send_bytes_for_ate(tx_buffer, 3);
        return BK_FAIL;
    }

    /* wafer id is out of range */
    if((device_id_write[2] < EFUSE_WAFER_ID_MINIMUM) || (device_id_write[2] > EFUSE_WAFER_ID_MAXIMUM)) {
        tx_buffer[1] = 0x77;
        tx_buffer[2] = 0x4;
        uart_send_bytes_for_ate(tx_buffer, 3);
        return BK_FAIL;
    }

    for (; retry_times < 3; retry_times++) {
        ret = bk_otp_ahb_update(OTP_DEVICE_ID, device_id_buf, sizeof(device_id_buf));
        if (ret == BK_OK) {
            break;
        }
    }

    if (ret != BK_OK) {
        tx_buffer[1] = 0x77;

        if (ret == BK_ERR_OTP_NOT_EMPTY) {
            tx_buffer[2] = 0x5;
        } else if(ret == BK_ERR_OTP_ADDR_OUT_OF_RANGE) {
            tx_buffer[2] = 0x6;
        } else if(ret == BK_ERR_NO_WRITE_PERMISSION) {
            tx_buffer[2] = 0x7;
        } else if(ret == BK_ERR_OTP_UPDATE_NOT_EQUAL) {
            tx_buffer[2] = 0x8;
        } else if(ret == BK_ERR_NO_READ_PERMISSION) {
            tx_buffer[2] = 0x9;
        } else {
            tx_buffer[2] = 0xff;
        }

        bk_otp_ahb_read(OTP_DEVICE_ID, tx_buffer + 3, EFUSE_DEVICE_ID_BYTE_NUM);
        uart_send_bytes_for_ate(tx_buffer, 3 + EFUSE_DEVICE_ID_BYTE_NUM);

        return BK_FAIL;
	}

    tx_buffer[1] = 0x88;
    bk_otp_ahb_read(OTP_DEVICE_ID, tx_buffer + 2, EFUSE_DEVICE_ID_BYTE_NUM);
    uart_send_bytes_for_ate(tx_buffer, 2 + EFUSE_DEVICE_ID_BYTE_NUM);

    return BK_OK;
}
#if 0
int set_mac_address_to_efuse(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    uint8_t mac_address_write[MAC_ADDR_LEN] = {0};
    uint8_t retry_times = 0;
    int ret = 0;

    tx_buffer[0] = 0x55;
    if (cnt < 2 + MAC_ADDR_LEN) {
        /* return param num error */
        tx_buffer[1] = 0x77;
        tx_buffer[2] = 0x1;
        uart_send_bytes_for_ate(tx_buffer, 3);
        return BK_FAIL;
    }

    for(int iIndex = 0; iIndex < MAC_ADDR_LEN; iIndex++) {
        mac_address_write[iIndex] = (uint8_t) (content[2 + iIndex] & 0xFF);
    }

    for (; retry_times < 3; retry_times++) {
	    ret = bk_otp_apb_update(OTP_MAC_ADDRESS, mac_address_write, sizeof(mac_address_write));
		if (ret == BK_OK) {
			break;
		}
    }

	if (ret != BK_OK) {
        tx_buffer[1] = 0x77;

        if (ret == BK_ERR_OTP_NOT_EMPTY) {
            tx_buffer[2] = 0x5;
        } else if(ret == BK_ERR_OTP_ADDR_OUT_OF_RANGE) {
            tx_buffer[2] = 0x6;
        } else if(ret == BK_ERR_NO_WRITE_PERMISSION) {
            tx_buffer[2] = 0x7;
        } else if(ret == BK_ERR_OTP_UPDATE_NOT_EQUAL) {
            tx_buffer[2] = 0x8;
        } else if(ret == BK_ERR_NO_READ_PERMISSION) {
            tx_buffer[2] = 0x9;
        } else {
            tx_buffer[2] = 0xff;
        }

        bk_otp_apb_read(OTP_MAC_ADDRESS, tx_buffer + 3, MAC_ADDR_LEN);
        uart_send_bytes_for_ate(tx_buffer, 3 + MAC_ADDR_LEN);

        return BK_FAIL;
	}

    tx_buffer[1] = 0x88;
    bk_otp_apb_read(OTP_MAC_ADDRESS, tx_buffer + 2, MAC_ADDR_LEN);
    uart_send_bytes_for_ate(tx_buffer, 2 + MAC_ADDR_LEN);

    return BK_OK;
}
#endif
/**
 * command format 55 xx, range of xx:
 * 0x : device test cmd
 * 1x : evm / per / sleep
 * 2x : evm
 * 3x : rfpll
 * 4x : save vdddig / bandgap into efuse/otp
 * 5x : set registers for cali
 * 6x : memory / flash / otp check
 * 7x : ble
 * 9x :
 */
int bkreg_run_command1(unsigned char *content, int cnt)
{
    UINT8 tx_buffer[16];
    UINT8 addr, data;

    if (cnt < 2)
        return -1;
    if (content[0] == 0x55)
    {
        switch (content[1])
        {
            case 0x00:      // read compilation time
            {
                send_compile_time();
                break;
            }

            case 0x01:      // read chip ID
            {
                send_chip_id();
                send_flash_id();
                send_compile_time_hex();
                break;
            }

            case 0x02:      // Voltage Output High at All GPIO Pins
            {
                gpio_voltage_output_high();
                break;
            }

            case 0x03:      // Voltage Output Low at All GPIO Pins
            {
                gpio_voltage_output_low();
                break;
            }

            case 0x04:      // Input High voltage to some GPIO
            {
                gpio_voltage_input_high();
                break;
            }

            case 0x05:      // Input High voltage to some GPIO
            {
                gpio_voltage_input_low();
                break;
            }
            case 0x06:      // read device ID
            {
                send_device_id();
                break;
            }
            case 0x0D:
            {
                wifi_cali_dpll_ate();
                break;
            }
            case 0x0E:
            {
                wifi_cali_rfpll_ate();
                break;
            }
            case 0x10:      // Pout_Max_2400    135M MCS7 11n 20M
            {
                uint32_t offset = 0;
                if (cnt > 2)
                {
                    offset = (uint8_t)content[2];
                }
                uint32_t frequency = 2400 + offset;
                evm_test(135, frequency, PHY_CHNL_BW_20, 0, 0, IEEE80211_BAND_2GHZ);
                break;
            }
            case 0x11:      // Pout_Max_2400    54M OFDM 11g
            {
                uint32_t offset = 0;
                if (cnt > 2)
                {
                    offset = (uint8_t)content[2];
                }
                uint32_t frequency = 2400 + offset;
                evm_test(54, frequency, PHY_CHNL_BW_20, 0, 0, IEEE80211_BAND_2GHZ);
                break;
            }
            case 0x12:      // Pout_Max_2480    11M DSSS 11b
            {
                uint32_t offset = 80;
                if (cnt > 2)
                {
                    offset = (uint8_t)content[2];
                }
                uint32_t frequency = 2400 + offset;
                evm_test(11, frequency, PHY_CHNL_BW_20, 0, 0, IEEE80211_BAND_2GHZ);
                break;
            }
            case 0x13:      // Pout_Max_2400    135M MCS7 11ax
            {
                uint32_t offset = 0;
                if (cnt > 2)
                {
                    offset = (uint8_t)content[2];
                }
                uint32_t frequency = 2400 + offset;
                evm_test(135, frequency, PHY_CHNL_BW_20, 5, 0, IEEE80211_BAND_2GHZ);
                break;
            }
            case 0x20:      // BER_Min_CHxx_20M: 55 20 xx [ms=50]
            {
                uint32_t duration = 50;
                uint32_t channel = 1;
                uint32_t arg = 0x20;
                if (cnt > 3)
                {
                    duration = (uint8_t)content[3];
                    channel = (uint8_t)content[2];
                }
                else if (cnt > 2)
                {
                    channel = (uint8_t)content[2];
                }
                uint32_t frequency = evm_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
                per_test(PHY_CHNL_BW_20, frequency, duration, arg, IEEE80211_BAND_2GHZ, RF_WIFI_WIFIPLL_MODE);
                break;
            }
            case 0x21:      // BER_Min_CHxx_40M: 55 21 xx [ms=50]
            {
                uint32_t duration = 50;
                uint32_t channel = 1;
                uint32_t arg = 0x21;
                if (cnt > 3)
                {
                    duration = (uint8_t)content[3];
                    channel = (uint8_t)content[2];
                }
                else if (cnt > 2)
                {
                    channel = (uint8_t)content[2];
                }
                uint32_t frequency = evm_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
                per_test(PHY_CHNL_BW_40, frequency, duration, arg, IEEE80211_BAND_2GHZ, RF_WIFI_WIFIPLL_MODE);
                break;
            }
            case 0x22:      // Pout_Max_2442   135, PHY_CHNL_BW_40
            {
                uint32_t offset = 42;
                if (cnt > 2)
                {
                    offset = (uint8_t)content[2];
                }
                uint32_t frequency = 2400 + offset;
                evm_test(135, frequency, PHY_CHNL_BW_40, 0, 0, IEEE80211_BAND_2GHZ);
                break;
            }
            case 0x23:      // // not test in ATE temporary,Pout_Max_2462    tx encrypt verify
            {
                tx_verify_test_2442();
                break;
            }
            case 0x0A:
                /* 55 0A 01 : read efuse 18,19,20,21 first, return value */
                /* 55 0A 10: set 1Volt and read ADC3, return value */
                /* 55 0A 20: set 2Volt and read ADC3, return value */
                /* 55 0A 15: set 1.5Volt and read ADC3, return value */
                /* 55 0A 02: write efuse 18,19,20,21 */
                ate_cali_adc(content, tx_buffer);
                break;
            case 0xB:
                /* 55 0B 01 : read efuse 18,19,20,21 first, return value */
                /* 55 0B 10: set 1Volt and read ADC3, return value */
                /* 55 0B 20: set 2Volt and read ADC3, return value */
                /* 55 0B 15: set 1.5Volt and read ADC3, return value */
                /* 55 0B 02: write efuse 18,19,20,21 */
                break;
            case 0x0C:       // SARADC temperature
                ate_cali_temperature(content, tx_buffer);
                break;
            case 0x14:      // PER_Min_2484: 55 14 [ms=50]
            {
                clock_time_t duration;
                uint32_t arg = 0x16;
                if (cnt > 2)
                {
                    duration = content[2];
                }
                else
                {
                    duration = 50;
                }
                uint32_t frequency = evm_channel_to_frequency(14, IEEE80211_BAND_2GHZ);
                per_test(PHY_CHNL_BW_20, frequency, duration, arg, IEEE80211_BAND_2GHZ, RF_WIFI_WIFIPLL_MODE);
                break;
            }
            #if CONFIG_DEEP_LV
            case 0x15:      //cpu off low voltage
            {
                ate_enter_low_voltage();
                break;
            }
            #endif
            case 0x16:      //low voltage
            {
                ate_enter_low_voltage();
                break;
            }
            case 0x17:      //deep sleep
            {
                ate_enter_deep_sleep();
                break;
            }

            case 0x18:  // CPU Test
            {
                int ret = 0;
                ret = core_cp_test();

                tx_buffer[0] = 0x55;
                if (ret == BK_OK) {
                    tx_buffer[1] = 0x33;
                    uart_send_bytes_for_ate(tx_buffer, 2);
                } else {
                    tx_buffer[1] = 0xCC;
                    tx_buffer[2] = ret & 0x0F;
                    uart_send_bytes_for_ate(tx_buffer, 3);
                }
                break;
            }

            case 0x24:      // PER_Min_2442: 55 24 [ms=50]
            {
                clock_time_t duration;
                uint32_t arg = 0x24;
                if (cnt > 2)
                {
                    duration = content[2];
                }
                else
                {
                    duration = 50;
                }
                uint32_t frequency = evm_channel_to_frequency(7, IEEE80211_BAND_2GHZ);
                per_test(PHY_CHNL_BW_40, frequency, duration, arg, IEEE80211_BAND_2GHZ, RF_WIFI_WIFIPLL_MODE);
                break;
            }
#if CONFIG_AUDIO
            case 0x31:      // audio_loop test: 55 31 [01|02|00]
            {
                uint32_t state;
                state = (uint32_t)content[2];
                audio_loop_test(state);
                audio_adc_mic2_to_dac_test(state);    /* use for bk7258 mic2 test */
                break;
            }
#endif
            case 0x30:      //not test in ATE temporary
            /* 55 3D xx set single tone with channel */
            /* xx channel [00, 7F] */
            {
				//UPDATE PA/PAD
                //addTRX_BEKEN_Reg0x0c  = (addTRX_BEKEN_Reg0x0c & (~(0x0FF0<<0))) | (0x110<<0);
                evm_single_test(54, 2400 + content[2], 0);
                tx_buffer[0] = 0x55;
                ate_time_delay(1000);

				// return tssi, invalid at present
                tx_buffer[1] = 0 & 0x00FF;
                tx_buffer[2] = 0 & 0x00FF;
                uart_send_bytes_for_ate(tx_buffer, 3);
                break;
            }

            case 0x35:  // CP_CMD_PHY_SWITCH
            {
                UINT32 param;
                static UINT32 ana_reg0x7_bak = 0;
                static UINT32 ana_reg0x8_bak = 0;
                static UINT32 ana_reg0x4_bak = 0;

                if ((cnt > 2) && (0 != content[2]))
                {
                    if (phy_closed)
                    {
                        sys_drv_analog_set(ANALOG_REG7, ana_reg0x7_bak);
                        sys_drv_analog_set(ANALOG_REG8, ana_reg0x8_bak);
                        sys_drv_analog_set(ANALOG_REG4, ana_reg0x4_bak);

                        rwnxl_wakeup();
                        _phy_power_ctrl_wrapper(1);
                        rf_module_vote_ctrl(RF_OPEN, RF_BY_ATE_WIFI_BIT);
                        phy_closed = 0;
                    }
                }
                else
                {
                    if (!phy_closed)
                    {
                        ana_reg0x7_bak = sys_drv_analog_get(ANALOG_REG7);
                        ana_reg0x8_bak = sys_drv_analog_get(ANALOG_REG8);
                        ana_reg0x4_bak = sys_drv_analog_get(ANALOG_REG4);

                        // sys_ana_0x48<9>=1
                        param = sys_drv_analog_get(ANALOG_REG8);
                        param |= (0x1 << 9);
                        sys_drv_analog_set(ANALOG_REG8, param);

                        // Measure the voltage of VDDIO(ldoio)  [0x2]：sys_ana_0x47<15:13>=0x2
                        param = sys_drv_analog_get(ANALOG_REG7);
                        param &= ~(0x7 << 13);
                        param |= (0x2 << 13);
                        sys_drv_analog_set(ANALOG_REG7, param);

                        // Measure the voltage of VDDDIG(ldodig)  [0xB]: sys_ana_0x48<19:16>=0xB
                        param = sys_drv_analog_get(ANALOG_REG8);
                        param &= ~(0xF << 16);
                        param |= (0xB << 16);
                        sys_drv_analog_set(ANALOG_REG8, param);

                        // Measure the voltage of VDDA(ldoana)  [0xB]: sys_ana_0x47<9:6>=0xB
                        param = sys_drv_analog_get(ANALOG_REG7);
                        param &= ~(0xF << 6);
                        param |= (0xB << 6);
                        sys_drv_analog_set(ANALOG_REG7, param);

                        // Measure the voltage of VDDRAM(vddram ldo)  [0x0]: sys_ana_0x44<31>=0x0
                        param = sys_drv_analog_get(ANALOG_REG4);
                        param |= (0x1 << 31);
                        // Measure the voltage of VDDRAM(vddram ldo)  [0x0]: sys_ana_0x44<29:28>=0x0
                        param &= ~(0x3 << 28);
                        sys_drv_analog_set(ANALOG_REG4, param);

                        rf_module_vote_ctrl(RF_CLOSE,RF_BY_ATE_WIFI_BIT);
                        _phy_power_ctrl_wrapper(0);
                        rwnxl_sleep();
                        phy_closed = 1;
                    }
                }

                tx_buffer[0] = 0x55;
                tx_buffer[1] = 0x33;
                uart_send_bytes_for_ate(tx_buffer, 2);
                break;
            }

            case 0x3D:      //not test in ATE temporary
            /* 55 3D xx yy set rate and power_gain */
            /* yy rate: 1,2,5,11 for 11B, 6,9,12,18,24,32,48,54 for 11G, 128,...135 for MCS 0-7 */
            /* xx power_gain: [0, 2E] for 11B; [0, 4D] for 11G */
            {
                UINT32 rate = (UINT8)content[2];
                UINT32 power_gain = (UINT8)content[3];

                if ((rate == 1) || (rate == 2) || (rate == 5) || (rate == 11))
                {
                    rate = 11;
                }
                else
                {
                    rate = 54;
                }

                rwnx_cal_set_txpwr(power_gain, rate);
                tx_buffer[0] = 0x55;
                tx_buffer[1] = 0x33;
                uart_send_bytes_for_ate(tx_buffer, 2);
                break;
            }

#if (CONFIG_OTP)
            case 0x3E:  // OTP RFPWR_TBL write and read
            {
                uint32_t length = 46;
                UINT8 tx_buffer[100];
                int ret = 0;
                UINT8 otp_idx = content[3];

                switch (content[2])
                {
                case 0x01:  // OTP RFPWR_TBL read
                {
                    ret = bk_otp_ahb_read(otp_idx, tx_buffer + 1, length);
                    tx_buffer[0] = 0x0E;
                    if (ret != BK_OK) {
                        // otp read fail
                        tx_buffer[0] = 0x55;
                        tx_buffer[1] = 0xBB;
                        uart_send_bytes_for_ate(tx_buffer, 2);
                    } else {
                        uart_send_bytes_for_ate(tx_buffer, length + 1);
                    }
                    break;
                }

                case 0x02:  // OTP RFPWR_TBL write
                {
                    UINT8 tbl_version = CONFIG_ATE_PHY_PWR_TBL_VERSION;
                    UINT8 ana_version = CONFIG_ATE_PHY_PWR_ANA_VERSION;
                    UINT32 pwr_tbl[12] = {0};

                    pwr_tbl[0] = g_2g_11b_cal_points[0].pwr_idx << 24 | g_2g_11g_cal_points[0].pwr_idx << 16;
                    pwr_tbl[1] = g_2g_11b_cal_points[1].pwr_idx << 24 | g_2g_11g_cal_points[1].pwr_idx << 16;
                    pwr_tbl[2] = g_2g_11b_cal_points[2].pwr_idx << 24 | g_2g_11g_cal_points[2].pwr_idx << 16;
                    pwr_tbl[3] = g_2g_11b_cal_points[0].pwr_dBm_x10 << 24 | g_2g_11g_cal_points[0].pwr_dBm_x10 << 16;
                    pwr_tbl[4] = g_2g_11b_cal_points[1].pwr_dBm_x10 << 24 | g_2g_11g_cal_points[1].pwr_dBm_x10 << 16;
                    pwr_tbl[5] = g_2g_11b_cal_points[2].pwr_dBm_x10 << 24 | g_2g_11g_cal_points[2].pwr_dBm_x10 << 16;
                    pwr_tbl[6] = g_5g_11a_cal_points[0].pwr_idx << 24 | g_5g_11a_cal_points[0].pwr_dBm_x10 << 16;
                    pwr_tbl[7] = g_5g_11a_cal_points[1].pwr_idx << 24 | g_5g_11a_cal_points[1].pwr_dBm_x10 << 16;
                    pwr_tbl[8] = g_5g_11a_cal_points[2].pwr_idx << 24 | g_5g_11a_cal_points[2].pwr_dBm_x10 << 16;
                    pwr_tbl[9] = g_5g_11a_cal_points[3].pwr_idx << 24 | g_5g_11a_cal_points[3].pwr_dBm_x10 << 16;
                    pwr_tbl[10] = g_5g_11a_cal_points[4].pwr_idx << 24 | g_5g_11a_cal_points[4].pwr_dBm_x10 << 16;
                    pwr_tbl[11] = tbl_version | ana_version << 8;

                    // "bk_otp_ahb_update" will do read-back check, so do not need to read back again here
                    ret = bk_otp_ahb_update(otp_idx, (uint8_t *)pwr_tbl, sizeof(pwr_tbl));
                    tx_buffer[0] = 0x55;
                    if (ret != BK_OK) {
                        tx_buffer[1] = 0xCC;
                    } else {
                        tx_buffer[1] = 0x33;
                    }
                    uart_send_bytes_for_ate(tx_buffer, 2);
                    break;
                }

                default:
                    break;
                }
                break;
            }

            case 0x3C:  // OTP_FACTORY_ID read and write
            case 0x3F:  // OTP_PRODUCT_ID read and write
            {
                uint32_t id = (content[1] == 0x3C) ? OTP_FACTORY_ID : OTP_PRODUCT_ID;
                ate_otp_op_t op = (ate_otp_op_t)content[2];

                if (op == OTP_OP_READ) {
                    // Read command: content = 0x55 3C/3F 01
                    uint32_t len = (content[1] == 0x3C) ? 3 : 15;
                    ate_otp_rw_common(id, NULL, len, op);
                } else if (op == OTP_OP_WRITE) {
                    // Write command: content = 0x55 3C/3F 02 data[4]/data[16]
                    uint32_t len = (content[1] == 0x3C) ? 4 : 16;
                    ate_otp_rw_common(id, (uint8_t *)content + 3, len, op);
                }
                break;
            }

            case 0x40: //read otp
                ate_read_otp(content, cnt, tx_buffer);
                break;
#endif
#if 0
            case 0x41:      //not test in ATE temporary
            {
                addr = 0;
                data = 0x0A;
                tx_buffer[0] = 0x55;
                if (bk_efuse_write_byte(addr, data) == 0)
                {
                    tx_buffer[1] = 0x33;
                }
                else
                {
                    tx_buffer[1] = 0xCC;
                }
                uart_send_bytes_for_ate(tx_buffer, 2);
                break;
            }
#endif
            case 0x42:
            {
                if (cnt > 2)
                {
                    addr = ((UINT8)content[2] & 0x1F);
                    data = 0;
                    if (addr == 0x11)
                    {
                        //BANDGAP in efuse for legacy
                        bk_otp_ahb_read(OTP_VDDDIG_BANDGAP, &data, (uint32_t)sizeof(data));
                        tx_buffer[0] = 0x0E;
                        tx_buffer[1] = data;
                        uart_send_bytes_for_ate(tx_buffer, 2);
                        break;
                    }
#if CONFIG_EFUSE
                    bk_efuse_read_byte(addr, &data);
#endif
                    uart_send_bytes_for_ate(&data, 1);
                }
                else
                {
#if CONFIG_EFUSE
                    unsigned char i;
                    for (i=0; i<32; i++)
                    {
                        addr = i;
                        data = 0;
                        bk_efuse_read_byte(addr, &data);
                    }
#endif
                }
                break;
            }

            case 0x43:
                /* write 17th bytes in efuse for analog2<28:23> to cali vdddig */
                sctrl_set_bandgap_to_efuse(content, cnt, tx_buffer);
                break;
#if 0
            case 0x44:
                /* write macaddr in efuse/OTP */
                set_mac_address_to_efuse(content, cnt, tx_buffer);
                break;
#endif
            case 0x45:
                /* check all efuse bytes: bits[i]=0 means efuse[i]=0 */
                sctrl_check_efuse((const unsigned char *)content, cnt, tx_buffer);
                break;
            case 0x46:
                /* check partial otp: 0E xx xx xx xx, return first !0 address, 0 means clean */
                sctrl_check_otp((const unsigned char *)content, cnt, tx_buffer);
                break;
            case 0x47:
            {
                /* flash crc bypass enable */
                int ret = 0;
                tx_buffer[0] = 0x55;
                UINT32 efuse_val;

                ret = bk_otp_apb_read(OTP_EFUSE, (UINT8 *)&efuse_val, sizeof(efuse_val));
                if (ret != BK_OK) {
                    tx_buffer[1] = 0xCC;
                    uart_send_bytes_for_ate(tx_buffer, 2);
                    break;
                }

                // flash_crc_bypass = 1
                efuse_val |= (0x1 << 27);

                // "bk_otp_ahb_update" will do read-back check, so do not need to read back again here
                ret = bk_otp_apb_update(OTP_EFUSE, (UINT8 *)&efuse_val, sizeof(efuse_val));
                if (ret != BK_OK) {
                    tx_buffer[1] = 0xCC;
                } else {
                    tx_buffer[1] = 0x33;
                }
                uart_send_bytes_for_ate(tx_buffer, 2);
                break;
            }
#if 0
            case 0x50:
                if ((cnt > 2) && ((unsigned char) content[2] < 80))
                {
                    rwnx_cal_update_pregain(0 - content[2]);
                }
                break;
            case 0x51:
                if ((cnt > 2) && ((unsigned char) content[2] < 80))
                {
                    rwnx_cal_update_pregain(content[2]);
                }
                break;
#endif
            case 0x52:
                /* set analog2<28:23> to cali vdddig */
                /* 55 52 xx */
                sctrl_set_bandgap_to_reg(content, cnt, tx_buffer);
                break;
            case 0x53:
            /* set vdddig to 1.00V */
            {
                tx_buffer[0] = 0x55;
                if (sctrl_set_vdddig_1voltage() == 0)
                {
                    /* success */
                    tx_buffer[1] = 0x33;
                }
                else
                {
                    tx_buffer[1] = 0xCC;
                }
                uart_send_bytes_for_ate(tx_buffer, 2);
                break;
            }
            case 0x54:
                /* set SCTRL_REG0X4 vhsel_ldodig<31:29> */
                sctrl_set_vdddig_to_reg(content, cnt, tx_buffer);
                break;
#if 0
            case 0x55:
            {
                /* check dpll unlock */
                if (cnt > 2)
                {
                    /* success */
                    sctrl_check_dpll_unlock((unsigned char) content[2], tx_buffer);
                }
                else
                {
                    sctrl_check_dpll_unlock(0, tx_buffer);
                }
                uart_print(tx_buffer, 2);
                break;
            }
#endif	//TODO:temp disable.

            case 0x56:
                psram_function_test();
                break;

            case 0x57:
                /* set xtal: 55 57 xx */
                manual_cal_set_xtal((UINT32)content[2]);
                break;

            case 0x58:
                /* set trxA<31:28>_trxB<17:17> to cali Dia */
                /* 55 58 xx */
                sctrl_set_dia_to_reg(content, cnt, tx_buffer);
                break;

            case 0x59:
                /* save trxA<31:28>_trxB<17:17> to otp */
                /* 55 59 [xx] */
                sctrl_set_dia_to_efuse(content, cnt, tx_buffer);
                break;

            case 0x60:
                /* 55 60 memory check, return 55 33 if success, otherwise 55 CC */
#if CONFIG_RESET_REASON
                cmd_start_memcheck();
#endif
                break;

            case 0x61:
                cmd_save_memcheck_from_uart(content, cnt, tx_buffer);
                // tx_buffer[0] = 0x55;
                // data = cmd_save_memcheck();
                // if (data < 0) {
                //     tx_buffer[1] = 0xCC;
                // } else {
                //     /* success */
                //     tx_buffer[1] = 0x33;
                // }
                // uart_send_bytes_for_ate(tx_buffer, 2);
                break;

            case 0x62:
                /* 55 62 softreset for MBIST/ATPG, return 55 33 if success, otherwise 55 CC */
                if (cnt > 3) {
                    cmd_start_softreset((uint8_t)content[2], (uint8_t)content[3]);
                } else if (cnt > 2) {
                    cmd_start_softreset((uint8_t)content[2], 0xA);
                } else {
                    cmd_start_softreset(0x5, 0xA);
                }
                break;

            case 0x63:
            {
                uint32_t result;
                result = bk_otp_fully_flow_test();
                tx_buffer[0] = 0x55;
                tx_buffer[2] = (result & 0xFF00) >> 8;
                tx_buffer[3] = result & 0xFF;
                if (result == BK_OK) {
                    /* success */
                    tx_buffer[1] = 0x33;
                } else if (result & BK_ERR_OTP_OPERATION_ERROR_MASK) {
                    tx_buffer[1] = 0xC1;
                } else if (result & BK_ERR_OTP_OPERATION_FAIL_MASK) {
                    tx_buffer[1] = 0xC2;
                } else if (result & BK_ERR_OTP_INIT_FAIL_MASK) {
                    tx_buffer[1] = 0xC3;
                } else if (result & BK_ERR_OTP_FULL_ERROR_MASK) {
                    /*will not happen normally*/
                    tx_buffer[1] = 0xC4;
                } else {
                    /*auto repair return fail cnt,see tx_buffer[3] second nibble*/
                    tx_buffer[1] = 0xC5;
                }
                uart_send_bytes_for_ate(tx_buffer, 4);
                break;
            }

            case 0x70:
            {
                /* BLE DUT */
                evm_stop_bypass_mac();
                ble_dut_start(CONFIG_UART_PRINT_PORT);
                break;
            }
            #if CONFIG_MAC802154_ENABLE
            case 0x71:
            {
                uint8_t size = 0;
                mac80154_ate(&content[2],cnt-2,tx_buffer,&size);
                uart_send_bytes_for_ate(tx_buffer, size);
                break;
            }
            #endif
#if CONFIG_USB
            case 0x80:
            {
                UINT8 usbbackbuffer[2];
                switch (content[2])
                {
                    case 0x01:
                    {
                        bk_usb_ate_voh_vol_test(1);
                        bk_usb_ate_voh_vol_test(2);
                        break;
                    }

                    case 0x02:
                    {
                        bk_usb_ate_voh_vol_test(1);
                        bk_usb_ate_voh_vol_test(3);
                        break;
                    }

                    case 0x03:
                    {
                        bk_usb_ate_rterm_test(1);
                        bk_usb_ate_rterm_test(2);
                        break;
                    }

                    case 0x04:
                    {
                        bk_usb_ate_rx_dc_input_test(1);
                        bk_usb_ate_rx_dc_input_test(2);
                        if(content[3] == 0x01)
                        {
                            if(bk_usb_ate_rx_dc_input_test(3) == BK_OK)
                            {
                                 usbbackbuffer[0] = 0x55;
                                 usbbackbuffer[1] = 0x33;
                            }else {
                                 usbbackbuffer[0] = 0x55;
                                 usbbackbuffer[1] = 0xCC;
                            }
                        }
                        if(content[3] == 0x02)
                        {
                            if(bk_usb_ate_rx_dc_input_test(3) == BK_OK)
                            {
                                 usbbackbuffer[0] = 0x55;
                                 usbbackbuffer[1] = 0x33;
                            }else {
                                 usbbackbuffer[0] = 0x55;
                                 usbbackbuffer[1] = 0xCC;
                            }
                        }
                        uart_send_bytes_for_ate(usbbackbuffer, 2);
                        break;
                    }
                    case 0x05:
                    {
                        if(bk_usb_ate_bist_test(1) == BK_OK)
                        {
                            usbbackbuffer[0] = 0x55;
                            usbbackbuffer[1] = 0x33;
                        }else {
                            usbbackbuffer[0] = 0x55;
                            usbbackbuffer[1] = 0xCC;
                        }
                        uart_send_bytes_for_ate(usbbackbuffer, 2);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
#endif
            case 0x90:
            /* 0x55 90 [tbl_idx] [vertion] [pwrtbl_dBm_x10] */
            /* 0x55 90 01/02 01 01 BB BB BB GG GG GG AA AA AA AA AA */
            {
                int ret = 0;
                UINT8 tbl_idx = content[2];
                UINT8 tbl_version = content[3];
                UINT8 ana_version = content[4];
                UINT8 pwr_dBm_x10_11b_ch1 = content[5];
                UINT8 pwr_dBm_x10_11b_ch7 = content[6];
                UINT8 pwr_dBm_x10_11b_ch13 = content[7];
                UINT8 pwr_dBm_x10_11g_ch1 = content[8];
                UINT8 pwr_dBm_x10_11g_ch7 = content[9];
                UINT8 pwr_dBm_x10_11g_ch13 = content[10];
                UINT8 pwr_dBm_x10_11a_ch36 = content[11];
                UINT8 pwr_dBm_x10_11a_ch64 = content[12];
                UINT8 pwr_dBm_x10_11a_ch100 = content[13];
                UINT8 pwr_dBm_x10_11a_ch132 = content[14];
                UINT8 pwr_dBm_x10_11a_ch165 = content[15];

                UINT8 pwr_idx_11b_ch1 = g_2g_11b_cal_points[0].pwr_idx;
                UINT8 pwr_idx_11b_ch7 = g_2g_11b_cal_points[1].pwr_idx;
                UINT8 pwr_idx_11b_ch13 = g_2g_11b_cal_points[2].pwr_idx;
                UINT8 pwr_idx_11g_ch1 = g_2g_11g_cal_points[0].pwr_idx;
                UINT8 pwr_idx_11g_ch7 = g_2g_11g_cal_points[1].pwr_idx;
                UINT8 pwr_idx_11g_ch13 = g_2g_11g_cal_points[2].pwr_idx;
                UINT8 pwr_idx_11a_ch36 = g_5g_11a_cal_points[0].pwr_idx;
                UINT8 pwr_idx_11a_ch64 = g_5g_11a_cal_points[1].pwr_idx;
                UINT8 pwr_idx_11a_ch100 = g_5g_11a_cal_points[2].pwr_idx;
                UINT8 pwr_idx_11a_ch132 = g_5g_11a_cal_points[3].pwr_idx;
                UINT8 pwr_idx_11a_ch165 = g_5g_11a_cal_points[4].pwr_idx;

                UINT32 pwr_tbl[12] = {0};

                pwr_tbl[0] = pwr_idx_11b_ch1 << 24 | pwr_idx_11g_ch1 << 16;
                pwr_tbl[1] = pwr_idx_11b_ch7 << 24 | pwr_idx_11g_ch7 << 16;
                pwr_tbl[2] = pwr_idx_11b_ch13 << 24 | pwr_idx_11g_ch13 << 16;
                pwr_tbl[3] = pwr_dBm_x10_11b_ch1 << 24 | pwr_dBm_x10_11g_ch1 << 16;
                pwr_tbl[4] = pwr_dBm_x10_11b_ch7 << 24 | pwr_dBm_x10_11g_ch7 << 16;
                pwr_tbl[5] = pwr_dBm_x10_11b_ch13 << 24 | pwr_dBm_x10_11g_ch13 << 16;
                pwr_tbl[6] = pwr_idx_11a_ch36 << 24 | pwr_dBm_x10_11a_ch36 << 16;
                pwr_tbl[7] = pwr_idx_11a_ch64 << 24 | pwr_dBm_x10_11a_ch64 << 16;
                pwr_tbl[8] = pwr_idx_11a_ch100 << 24 | pwr_dBm_x10_11a_ch100 << 16;
                pwr_tbl[9] = pwr_idx_11a_ch132 << 24 | pwr_dBm_x10_11a_ch132 << 16;
                pwr_tbl[10] = pwr_idx_11a_ch165 << 24 | pwr_dBm_x10_11a_ch165 << 16;
                pwr_tbl[11] = tbl_version << 24 | ana_version << 16;

                if (tbl_idx == 0x01) {
                    ret = bk_otp_ahb_update(OTP_PHY_PWR1, (uint8_t *)pwr_tbl, sizeof(pwr_tbl));
                } else {
                    ret = bk_otp_ahb_update(OTP_PHY_PWR2, (uint8_t *)pwr_tbl, sizeof(pwr_tbl));
                }

                tx_buffer[0] = 0x55;
                if (ret != BK_OK) {
                    tx_buffer[0] = 0xCC;
                } else {
                    tx_buffer[0] = 0x33;
                }

                uart_send_bytes_for_ate(tx_buffer, 2);

                break;
            }

            case 0x99:
            {
                set_device_id_to_efuse(content, cnt, tx_buffer);
                break;
            }

            /*
             * RF Test
             * Cli_CMD: 0x55 [G][X] [H][J] [K][L] [M][N] [P][Q]
             * G[3:0]: "TX/RX Mode",
             *     0xA = TX
             *     0xB = RX
             * X[3:0]: "HT20/HT40",
             *     0x0 = HT20
             *     0x1 = HT40
             * H_J_K_L[15:0]: "Channel",
             *     0x24_12 = 2412MHz
             *     0x24_42 = 2442MHz
             *     0x24_72 = 2472Mhz
             *     ...
             * M[3:0]: "Modulation Format",
             *     0x0 = MOD_FMT_NON_HT,
             *     0x1 = MOD_FMT_NON_HT_DUP_OFDM,
             *     0x2 = MOD_FMT_HT_MM,
             *     0x3 = MOD_FMT_HT_GF,
             *     0x4 = MOD_FMT_VHT,
             *     0x5 = MOD_FMT_HE_SU,
             *     0x6 = MOD_FMT_HE_MU,
             *     0x7 = MOD_FMT_EXT_SU,
             *     0x8 = MOD_FMT_HE_TB,
             *     0xF = MOD_FMT_MASK
             * N_P_Q[11:0]: "Rate",
             *     0x000 = 1Mbps
             *     0x001 = 2Mbps
             *     0x002 = 5.5Mbps
             *     0x003 = 11Mbps
             *     0x004 = 6Mbps
             *     0x005 = 9Mbps
             *     0x006 = 12Mbps
             *     0x007 = 18Mbps
             *     0x008 = 24Mbps
             *     0x009 = 36Mbps
             *     0x00A = 48Mbps
             *     0x00B = 54Mbps
             *     0x128 = MCS0
             *     0x129 = MCS1
             *     0x130 = MCS2
             *     0x131 = MCS3
             *     0x132 = MCS4
             *     0x133 = MCS5
             *     0x134 = MCS6
             *     0x135 = MCS7
             *     0x136 = MCS8
             *     0x137 = MCS9
            */

            #define FUNC_TX_SIGNAL         (0x01)
            #define FUNC_SAVE_TXPWR        (0x02)
            #define FUNC_SET_TARGET_TXPWR  (0x03)

            case 0xA0:  // RF HT20 TX Test
            {
                /* 55 A0 Channel<15:0> Modulation<3:0> Rate<11:0> */
                /* Ep: 0x55 A0 24 12 00 0B, TX Test 11G 54M 2412MHz HT20 */
                uint32_t frequency = 0;
                uint32_t mod = 0;
                uint32_t rate_i = 0;
                uint32_t rate_o = 0;
                uint8_t  pwr_dbm_x10 = 0;
                uint8_t  function = 0;

                if (cnt > 6)
                {
                    frequency = ((content[2] >> 4) & 0x0F) * 1000
                              + ((content[2] >> 0) & 0x0F) * 100
                              + ((content[3] >> 4) & 0x0F) * 10
                              + ((content[3] >> 0) & 0x0F) * 1;

                    mod = (content[4] >> 4) & 0x0F;

                    rate_i = ((content[4] & 0x0F) << 8) | (content[5] & 0xFF);

                    function = content[6];
                }

                if (function != FUNC_TX_SIGNAL) {
                    pwr_dbm_x10 = content[7];
                }

                wifi_band_t band;
                if ((frequency >= 2412) && (frequency <= 2484)) {
                    band = IEEE80211_BAND_2GHZ;
                } else if ((frequency >= 5005) && (frequency <= 5885)) {
                    band = IEEE80211_BAND_5GHZ;
                } else if ((frequency >= 5955) && (frequency <= 7115)) {
                    band = IEEE80211_BAND_6GHZ;
                } else {
                    band = IEEE80211_BAND_2GHZ;
                }

                if (rate_i <= 0xB) {
                    switch (rate_i) {
                    case 0x0: rate_o = 1; break;
                    case 0x1: rate_o = 2; break;
                    case 0x2: rate_o = 5; break;
                    case 0x3: rate_o = 11; break;
                    case 0x4: rate_o = 6; break;
                    case 0x5: rate_o = 9; break;
                    case 0x6: rate_o = 12; break;
                    case 0x7: rate_o = 18; break;
                    case 0x8: rate_o = 24; break;
                    case 0x9: rate_o = 36; break;
                    case 0xA: rate_o = 48; break;
                    case 0xB: rate_o = 54; break;
                    default:  rate_o = rate_i; break;
                    }
                } else if ((rate_i >= 0x128) && (rate_i <= 0x137)) {
                    rate_o = ((rate_i >> 8) & 0xF) * 100
                           + ((rate_i >> 4) & 0xF) * 10
                           + (rate_i & 0xF);
                } else {
                    rate_o = rate_i;
                }

                if (function == FUNC_TX_SIGNAL) {
                    evm_test(rate_o, frequency, PHY_CHNL_BW_20, mod, pwr_dbm_x10, band);
                } else if (function == FUNC_SAVE_TXPWR) {
                    evm_test_save_pwr(rate_o, frequency, pwr_dbm_x10);
                } else if (function == FUNC_SET_TARGET_TXPWR) {
                    if (rate_o >= 128 && rate_o <= 137) {
                        evm_test(rate_o, frequency, PHY_CHNL_BW_20, mod, pwr_dbm_x10, band);
                        UINT32 pwr_idx = 0;
                        if (band == IEEE80211_BAND_2GHZ) {
                            UINT32 channel = evm_frequency_to_channel(frequency, band);
                            pwr_idx = fit_power_index_generic((UINT8)channel, pwr_dbm_x10, g_2g_11g_cal_points, 3);
                        } else if (band == IEEE80211_BAND_5GHZ) {
                            UINT32 channel = evm_frequency_to_channel(frequency, band);
                            pwr_idx = fit_power_index_generic((UINT8)channel, pwr_dbm_x10, g_5g_11a_cal_points, 5);
                        } else {
                            pwr_idx = 0;
                        }
                        rwnx_cal_set_txpwr(pwr_idx, rate_o);
                    }
                }
                break;
            }

            case 0xB0:  // RF HT20 RX Test
            {
                /* 55 B0 Channel<15:0> Modulation<3:0> Rate<11:0> */
                /* Ep: 0x55 B0 24 12, RX Test 2412MHz HT20 */
                uint32_t frequency = 0;
                uint32_t duration = 50;
                uint32_t arg = 0x20;
                uint32_t rfmod = RF_WIFI_WIFIPLL_MODE;

                if (cnt > 3)
                {
                    frequency = ((content[2] >> 4) & 0x0F) * 1000
                              + ((content[2] >> 0) & 0x0F) * 100
                              + ((content[3] >> 4) & 0x0F) * 10
                              + ((content[3] >> 0) & 0x0F) * 1;
                }

                if ((cnt > 4) && (content[4] == 0x00))
                {
                    rfmod = RF_WIFI_BTPLL_MODE;
                }

                wifi_band_t band;
                if ((frequency >= 2300) && (frequency <= 2600)) {
                    band = IEEE80211_BAND_2GHZ;
                } else if ((frequency >= 4500) && (frequency <= 6000)) {
                    band = IEEE80211_BAND_5GHZ;
                } else if ((frequency > 6000) && (frequency <= 7500)) {
                    band = IEEE80211_BAND_6GHZ;
                } else {
                    band = IEEE80211_BAND_2GHZ;
                }

                per_test(PHY_CHNL_BW_20, frequency, duration, arg, band, rfmod);
                break;
            }

            case 0xA1:  // RF HT40 TX Test
            {
                /* 55 A1 Channel<15:0> Modulation<3:0> Rate<11:0> */
                /* Ep: 0x55 A1 24 12 01 28, TX Test 11N MCS0 2412MHz HT40 */
                uint32_t frequency = 0;
                uint32_t mod = 0;
                uint32_t rate_i = 0;
                uint32_t rate_o = 0;
                uint8_t  pwr_dbm_x10 = 0;
                uint8_t  function = 0;

                if (cnt > 6)
                {
                    frequency = ((content[2] >> 4) & 0x0F) * 1000
                              + ((content[2] >> 0) & 0x0F) * 100
                              + ((content[3] >> 4) & 0x0F) * 10
                              + ((content[3] >> 0) & 0x0F) * 1;

                    mod = (content[4] >> 4) & 0x0F;

                    rate_i = ((content[4] & 0x0F) << 8) | (content[5] & 0xFF);

                    function = content[6];
                }

                if (function != FUNC_TX_SIGNAL) {
                    pwr_dbm_x10 = content[7];
                }

                wifi_band_t band;
                if ((frequency >= 2412) && (frequency <= 2484)) {
                    band = IEEE80211_BAND_2GHZ;
                } else if ((frequency >= 5005) && (frequency <= 5885)) {
                    band = IEEE80211_BAND_5GHZ;
                } else if ((frequency >= 5955) && (frequency <= 7115)) {
                    band = IEEE80211_BAND_6GHZ;
                } else {
                    band = IEEE80211_BAND_2GHZ;
                }

                if ((rate_i >= 0x128) && (rate_i <= 0x137)) {
                    rate_o = ((rate_i >> 8) & 0xF) * 100
                           + ((rate_i >> 4) & 0xF) * 10
                           + (rate_i & 0xF);
                } else {
                    rate_o = rate_i;
                }

                if (function == FUNC_TX_SIGNAL) {
                    evm_test(rate_o, frequency, PHY_CHNL_BW_40, mod, pwr_dbm_x10, band);
                } else if (function == FUNC_SAVE_TXPWR) {
                    evm_test_save_pwr(rate_o, frequency, pwr_dbm_x10);
                } else if (function == FUNC_SET_TARGET_TXPWR) {
                    if (rate_o >= 128 && rate_o <= 137) {
                        evm_test(rate_o, frequency, PHY_CHNL_BW_40, mod, pwr_dbm_x10, band);
                        UINT32 pwr_idx = 0;
                        if (band == IEEE80211_BAND_2GHZ) {
                            UINT32 channel = evm_frequency_to_channel(frequency, band);
                            pwr_idx = fit_power_index_generic((UINT8)channel, pwr_dbm_x10, g_2g_11g_cal_points, 3);
                        } else if (band == IEEE80211_BAND_5GHZ) {
                            UINT32 channel = evm_frequency_to_channel(frequency, band);
                            pwr_idx = fit_power_index_generic((UINT8)channel, pwr_dbm_x10, g_5g_11a_cal_points, 5);
                        } else {
                            pwr_idx = 0;
                        }
                        rwnx_cal_set_txpwr(pwr_idx, rate_o);
                    }
                }
                break;
            }

            case 0xB1:  // RF HT40 RX Test
            {
                /* 55 B0 Channel<15:0> Modulation<3:0> Rate<11:0> */
                /* Ep: 0x55 B0 24 12, RX Test 2412MHz HT20 */
                uint32_t frequency = 0;
                uint32_t duration = 50;
                uint32_t arg = 0x21;
                uint32_t rfmod = RF_WIFI_WIFIPLL_MODE;

                if (cnt > 3)
                {
                    frequency = ((content[2] >> 4) & 0x0F) * 1000
                              + ((content[2] >> 0) & 0x0F) * 100
                              + ((content[3] >> 4) & 0x0F) * 10
                              + ((content[3] >> 0) & 0x0F) * 1;
                }

                if ((cnt > 4) && (content[4] == 0x00))
                {
                    rfmod = RF_WIFI_BTPLL_MODE;
                }

                wifi_band_t band;
                if ((frequency >= 2300) && (frequency <= 2600)) {
                    band = IEEE80211_BAND_2GHZ;
                } else if ((frequency >= 4500) && (frequency <= 6000)) {
                    band = IEEE80211_BAND_5GHZ;
                } else if ((frequency > 6000) && (frequency <= 7500)) {
                    band = IEEE80211_BAND_6GHZ;
                } else {
                    band = IEEE80211_BAND_2GHZ;
                }

                per_test(PHY_CHNL_BW_40, frequency, duration, arg, band, rfmod);
                break;
            }

            default:
                break;
        }
    }
    else
    {
        if ((content[0] == 0x01) && (content[1] == 0xE0) && (content[2] == 0xFC))
        {
            if (content[3] == cnt - 4)
            {
                unsigned long ulAddr;
                unsigned long ulData;
                switch (content[4])
                {
                    case 0x00:
                        tx_buffer[0] = 0x04;
                        tx_buffer[1] = 0x0E;
                        tx_buffer[2] = 0x01;
                        tx_buffer[3] = 0x00;
                        uart_send_bytes_for_ate(tx_buffer, 4);
                        break;

                    case 0x01:
                        ulAddr = ((content[5]<<0)  & 0x000000FFUL)
                               | ((content[6]<<8)  & 0x0000FF00UL)
                               | ((content[7]<<16) & 0x00FF0000UL)
                               | ((content[8]<<24) & 0xFF000000UL);
                        ulData = ((content[9]<<0)   & 0x000000FFUL)
                               | ((content[10]<<8)  & 0x0000FF00UL)
                               | ((content[11]<<16) & 0x00FF0000UL)
                               | ((content[12]<<24) & 0xFF000000UL);
                        *(volatile unsigned long *)ulAddr = ulData;
                        tx_buffer[0] = 0x04;
                        tx_buffer[1] = 0x0E;
                        tx_buffer[2] = 0x0C;
                        tx_buffer[3] = 0x01;
                        tx_buffer[4] = 0xE0;
                        tx_buffer[5] = 0xFC;
                        tx_buffer[6] = 0x01;
                        tx_buffer[7] = content[5];
                        tx_buffer[8] = content[6];
                        tx_buffer[9] = content[7];
                        tx_buffer[10] = content[8];
                        tx_buffer[11] = content[9];
                        tx_buffer[12] = content[10];
                        tx_buffer[13] = content[11];
                        tx_buffer[14] = content[12];
                        uart_send_bytes_for_ate(tx_buffer, 15);
                        break;

                    case 0x03:
                        ulAddr = ((content[5]<<0)  & 0x000000FFUL)
                               | ((content[6]<<8)  & 0x0000FF00UL)
                               | ((content[7]<<16) & 0x00FF0000UL)
                               | ((content[8]<<24) & 0xFF000000UL);
                        ulData = *(volatile unsigned long *)ulAddr;
                        tx_buffer[0] = 0x04;
                        tx_buffer[1] = 0x0E;
                        tx_buffer[2] = 0x0C;
                        tx_buffer[3] = 0x01;
                        tx_buffer[4] = 0xE0;
                        tx_buffer[5] = 0xFC;
                        tx_buffer[6] = 0x03;
                        tx_buffer[7] = content[5];
                        tx_buffer[8] = content[6];
                        tx_buffer[9] = content[7];
                        tx_buffer[10] = content[8];
                        tx_buffer[11] = (ulData >> 0)  & 0x000000FFUL;
                        tx_buffer[12] = (ulData >> 8)  & 0x000000FFUL;
                        tx_buffer[13] = (ulData >> 16) & 0x000000FFUL;
                        tx_buffer[14] = (ulData >> 24) & 0x000000FFUL;
                        uart_send_bytes_for_ate(tx_buffer, 15);
                        break;
#if (CONFIG_AUDIO)
                    case 0x37:
                        audio_ap_test_for_ate(content[5], content[6], content[7], content[8], content[9],  content[10]);
                        break;
#endif
                    case 0x77:
                        manual_cal_show_txpwr_tab();
                        break;
                    default:
                        break;
                }
            }
        }
    }
    if (content[0] == 0x4D && content[1] == 0x42)
		ate_test_multiple_cpus_case();

    return 0;
}
#else
int bkreg_run_command1(const unsigned char *content, int cnt)
{
    return 0;
}
#endif

