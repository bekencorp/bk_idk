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

#include "sys_hal.h"
#include "sys_driver.h"
#include "sys_driver_common.h"
#include "bk_phy_internal.h"

void sys_drv_thread_interrupt_ctrl(bool en)
{
    uint32_t int_level = sys_drv_enter_critical();
    sys_hal_thread_interrupt_ctrl(en);
    sys_drv_exit_critical(int_level);
}

void sys_drv_thread_rf_ctrl(bool en)
{
    static uint8_t rf_save;

    if (en) {
        rf_save = sys_hal_rf_ctrl_type_get();
        sys_hal_rf_ctrl(RF_CTRL_THREAD);
        rfconfig_enter_thread_mode();
        rwnx_cal_set_rfconfig_thread_mode();
    } else {
        rfconfig_exit_thread_mode();
        rwnx_cal_set_rfconfig_WIFIPLL();
        sys_hal_rf_ctrl(rf_save);
    }
}

void sys_set_thread_tx_mode_trxconfig(void)
{
    rfconfig_set_thread_tx_mode_trxconfig();
}
