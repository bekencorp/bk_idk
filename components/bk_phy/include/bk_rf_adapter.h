#include <common/bk_include.h>
#include "driver/hal/hal_adc_types.h"
#include "driver/flash_partition.h"
#include "driver/flash.h"
#include "drv_model_pub.h"
#include "bk_wifi_adapter.h"
#include "bk_phy_adapter.h"

typedef struct {
    uint32_t (*_sys_drv_modem_bus_clk_ctrl)(bool clk_en);
    uint32_t (*_sys_drv_modem_clk_ctrl)(bool clk_en);
    void (*_phy_exit_dsss_only)(void);
    void (*_phy_enter_dsss_only)(void);
    uint32_t (*_rtos_disable_int)(void);
    void (*_rtos_enable_int)(uint32_t int_level);
    void (*_rwnx_cal_mac_sleep_rc_recover)(void);

} rf_control_funcs_t;

void bk_rf_adapter_init(void);
extern const rf_control_funcs_t g_rf_control_funcs;
extern void rf_adapter_init(const void * phy_funcs);
extern void rf_cntrl_init(void);
