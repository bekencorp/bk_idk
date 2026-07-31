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


/*
 * DEFINES
 ****************************************************************************************
 */

/* CMD_RF_HOLD_BIT_SET/_CLR*/
///thread control rf
#define RF_BY_THREAD_BIT                  (1 << 7)

///saradc control rf
#define RF_BY_SARADC_BIT                  (1 << 6)

///temp control rf
#define RF_BY_TEMP_BIT                  (1 << 5)
///ate control rf
#define RF_BY_ATE_BT_BIT                   (1 << 4)
#define RF_BY_ATE_WIFI_BIT                   (1 << 3)
///bkreg
#define RF_BY_BKREG_BIT                   (1 << 2)
///ble control rf
#define RF_BY_BLE_BIT                   (1 << 1)
///wifi control rf
#define RF_BY_WIFI_BIT                  (1 << 0)
/* END*/


/* CMD_PLL_HOLD_BIT_SET/_CLR*/
#define RF_WIFIPLL_HOLD_BY_THREAD_BIT                (2 << 1)
#define RF_WIFIPLL_HOLD_BY_BLE_BIT                   (1 << 1)
#define RF_WIFIPLL_HOLD_BY_WIFI_BIT                  (1 << 0)
/* END*/


/*
 * ENUM
 ****************************************************************************************
 */

enum {
    RF_CLOSE,
    RF_OPEN,
};

enum {
    CMD_RF_WIFIPLL_HOLD_BIT_SET,
    CMD_RF_WIFIPLL_HOLD_BIT_CLR,
};

#if CONFIG_RF_COEX_JUDGE
enum MODULE_TYPE_E {
    MODULE_TYPE_BLE_BT,
    MODULE_TYPE_WIFI,
    MODULE_TYPE_THREAD,
    MODULE_TYPE_TEMP,
    MODULE_TYPE_MAX,
};

enum RF_OPERATION_E {
    RF_OPERATION_INVALID,
    RF_OPERATION_FREE,
    RF_OPERATION_APPLY,
    RF_OPERATION_MAX,
};

enum RF_PATH_E {
    RF_PATH_WIFI_IQ,
    RF_PATH_BT_IQ,
    RF_PATH_BT_POLAR,
    RF_PATH_THREAD_IQ,
    RF_PATH_MAX,
};

enum RF_PRIORITY_E {
    RF_PRIORITY_TEMP_HIGHEST,
    RF_PRIORITY_BLE_BT_HIGH,
    RF_PRIORITY_WIFI_HIGH,
    RF_PRIORITY_THREAD_HIGH,
    RF_PRIORITY_TEMP_HIGH,
    RF_PRIORITY_BLE_BT_NORMAL,
    RF_PRIORITY_WIFI_NORMAL,
    RF_PRIORITY_THREAD_NORMAL,
    RF_PRIORITY_TEMP_LOW,
    RF_PRIORITY_MAX,
};

enum RF_PLL_E {
    RF_PLL_LOW,
    RF_PLL_HIGH,
    RF_PLL_MAX,
};

enum RF_TASK_TYPE_E {
    RF_TASK_TYPE_WIFI_BEGIN,
    RF_TASK_TYPE_WIFI_INIT,
    RF_TASK_TYPE_WIFI_PLL_CHANGE,
    RF_TASK_TYPE_WIFI_FREE_ALL,
    RF_TASK_TYPE_WIFI_SCAN,
    RF_TASK_TYPE_WIFI_AUTH,
    RF_TASK_TYPE_WIFI_ASSOC,
    RF_TASK_TYPE_WIFI_EAPOL,
    RF_TASK_TYPE_WIFI_DHCP,
    RF_TASK_TYPE_WIFI_BEACON,
    RF_TASK_TYPE_WIFI_DATA,
    RF_TASK_TYPE_WIFI_CONNECT,
    RF_TASK_TYPE_WIFI_DISCONNECT,
    RF_TASK_TYPE_WIFI_AP,
    RF_TASK_TYPE_WIFI_END,

    RF_TASK_TYPE_BLE_BEGIN,
    RF_TASK_TYPE_BLE_INIT,
    RF_TASK_TYPE_BLE_FREE_ALL, //14
    RF_TASK_TYPE_BLE_SCAN, //15
    RF_TASK_TYPE_BLE_SCAN_AUX, //16
    RF_TASK_TYPE_BLE_ADV,  //17
    RF_TASK_TYPE_BLE_CONNECT, //18
    RF_TASK_TYPE_BLE_INITIALING, //19
    RF_TASK_TYPE_BLE_NORMAL, //20
    RF_TASK_TYPE_BLE_DUT, //21
    RF_TASK_TYPE_BLE_END,

    RF_TASK_TYPE_BT_BEGIN,
    RF_TASK_TYPE_BT_NORMAL,
    RF_TASK_TYPE_BT_END,

    RF_TASK_TYPE_THREAD_BEGIN,
    RF_TASK_TYPE_THREAD_INIT,
    RF_TASK_TYPE_THREAD_FREE_ALL,
    RF_TASK_TYPE_THREAD_TIMER,
    RF_TASK_TYPE_THREAD_RETRY,
    RF_TASK_TYPE_THREAD_ATTACHING,
    RF_TASK_TYPE_THREAD_CONNECTED,
    RF_TASK_TYPE_THREAD_DISABLED,
    RF_TASK_TYPE_THREAD_END,

    RF_TASK_TYPE_TEMP_BEGIN,
    RF_TASK_TYPE_TEMP,
    RF_TASK_TYPE_TEMP_END,
    RF_TASK_TYPE_MAX = 0xffffffffU
};

#define RF_TASK_TYPE_BASE_MASK        (0x0000FFFFU)
#define RF_TASK_TYPE_LINK_SHIFT       (16U)
#define RF_TASK_TYPE_LINK_MASK        (0xFFFF0000U)
#define RF_TASK_TYPE_BLE_LINK_MAX     (32U)
#define RF_TASK_TYPE_MAKE(task, link) ((((uint32_t)(link) & 0xFFFFU) << RF_TASK_TYPE_LINK_SHIFT) | ((uint32_t)(task) & RF_TASK_TYPE_BASE_MASK))

#define RF_BLE_TASK_BITMAP_NUM        (RF_TASK_TYPE_BLE_END - RF_TASK_TYPE_BLE_BEGIN - 1)

struct RF_ARBITRATION_T {
    enum MODULE_TYPE_E module_type;
    enum RF_OPERATION_E operation;
    enum RF_PATH_E rf_path;
    enum RF_PLL_E rf_pll;
    enum RF_PRIORITY_E priority;
    enum RF_TASK_TYPE_E task_type;
    uint32_t task_type_bitmap;
    uint32_t ble_task_link_bitmap[RF_BLE_TASK_BITMAP_NUM];
    bool is_save_when_failed;
    uint32_t time_length_us;
    uint32_t start_time_us;
    bool position_can_adjust;
    bool is_need_switch_rf;
};

enum RF_ARBIT_RESULT_E {
    RF_ARBIT_RESULT_SUCCESS,
    RF_ARBIT_RESULT_CONFLICT,
    RF_ARBIT_RESULT_ERROR,
    RF_ARBIT_RESULT_MAX,
};

typedef struct RF_PLL_CTRL_RESULT_T {
    enum RF_ARBIT_RESULT_E result;
    uint32_t recommend_time_us;
} RF_PLL_CTRL_RESULT_T;
#endif
/*
 * FUNCTION
 ****************************************************************************************
 */

UINT32 rf_pll_ctrl(UINT32 cmd, UINT32 param);
void rf_module_vote_ctrl(uint8_t cmd,uint32_t module);
void phy_clk_close_handler(uint32_t module);
void phy_clk_open_handler(uint32_t module);

#if CONFIG_RF_COEX_JUDGE
RF_PLL_CTRL_RESULT_T rf_pll_ctrl_coex_judge(enum MODULE_TYPE_E module_type, enum RF_OPERATION_E operation,
                                            enum RF_PATH_E rf_path, enum RF_PLL_E rf_pll,
                                            enum RF_PRIORITY_E priority, enum RF_TASK_TYPE_E task_type,
                                            bool is_save_when_failed,uint32_t time_length_us,
                                            bool position_can_adjust);
enum RF_PATH_E get_current_rf_path(void);
enum RF_PLL_E get_current_rf_pll(void);
void rf_cntrl_debug_dump_rf_state(void);
bool rf_cntrl_has_thread_rf_request(void);
uint32_t rf_cntrl_get_task_bitmap(enum MODULE_TYPE_E module_type, enum RF_PRIORITY_E priority);
uint32_t rf_cntrl_get_module_rf_time_us(enum MODULE_TYPE_E module_type);
void rf_cntrl_reset_module_rf_time(void);
void rf_cntrl_update_module_rf_ratio(void);
#endif