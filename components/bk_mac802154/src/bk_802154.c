// Copyright 2020-2025 Beken
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

#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include "lw_mac802154_interface.h"
#include "common/bk_err.h"
#include "components/system.h"

#include "FreeRTOSConfig.h"
#include "portmacro.h"

#if CONFIG_OPENTHREAD
#include "bk_openthread.h"
#include "bk_ot_alarm.h"
#endif

//#include "driver/aon_rtc.h"
#include "bk_802154.h"
#include "assert.h"
#include "bk_ieee802154_ack.h"

#include "driver/hal/hal_timer_types.h"
#include "driver/timer_types.h"
#include "driver/trng.h"

#include <components/system.h>
#include "sys_driver.h"
#include "bk_rf_internal.h"

#include "mac802154_adapter.h"
#include <modules/pm.h>

#include "cmsis_gcc.h"

#define BK_SELF_TEST_ENABLE 0

#define BK_ED_DURATION                      8      //number of symbol duration

#define BK_MAC_MIN_BE                       3
#define BK_MAC_MAX_BE                       5
#define BK_MAC_MAX_CSMA_BACKOFFS            4

#define ACK_TIMER_ID                        3


enum {
    TIMER_NO_PENDING = 0,
    TIMER_TX_PENDING = 1,
    TIMER_RX_PENDING = 2
};

#if CONFIG_OPENTHREAD
static uint8_t s_rx_frame_index = 0;
static bk_802154_frame_t s_frame_buf[BK_FRAME_BUFFER_MAX_SIZE+1] = {0};
static bk_802154_frame_t *next_rx_frame_buffer = NULL;

typedef struct {
    bk_802154_frame_t rx_frame[BK_FRAME_BUFFER_MAX_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} rx_pkt_ringbuffer;

static bool s_rx_when_idle = false;
static uint8_t s_timer_pending = TIMER_NO_PENDING;
volatile bk_802154_state_t s_802154_state = BK_802154_STATE_IDLE;
bool is_frame_pending = false;
static int8_t s_recent_rssi = 0;
static bool s_cca_channel_clear = false;
static bk_802154_frame_t s_ack_frame;
static uint8_t s_pending_tx_frame[BK_FRAME_LENGTH];
static uint8_t *ptr_pending_tx_frame;
static bool s_pending_tx_cca;
static bk_802154_frame_t* rx_pkt_with_enh_ack=NULL;

static rx_pkt_ringbuffer g_rx_buffer;
static bool b_needs_next_operation = false;
#define SET_NEEDS_NEXT_OPERATION(a) do{b_needs_next_operation = a;} while(0)
extern uint64_t bk_aon_rtc_get_us(void);
extern bk_err_t bk_timer_stop(timer_id_t timer_id);
extern bk_err_t bk_timer_delay_with_callback(timer_id_t timer_id, uint64_t time_us, timer_isr_t callback);

#if CONFIG_MAC802154_RX_SFD_EN
static volatile uint64_t uRxSfdTimeStamp = 0;
#endif

bk_802154_frame_t* bk_ieee802154_get_ack_buffer()
{
    return &s_ack_frame;
}

uint8_t* bk_ieee802154_get_pending_tx_frame()
{
    return s_pending_tx_frame;
}

static uint32_t bk_802154_gen_random_with_limitation(uint32_t limit)
{
    uint32_t rand_val = bk_rand();
    rand_val = rand_val & limit;

    if (0x00000000 == rand_val)
    {
        rand_val = limit;
    }

    return rand_val;
}

static void bk_802154_energy_detected(int8_t result)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    bk_802154_energy_detected_ex(result);
    xTaskNotifyFromISR(bk_ot_get_thread_main_hdl(), OT_MAINLOOP_PROC_ED, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void bk_802154_received_done(bk_802154_frame_t *data_p)
{
    if (s_rx_frame_index != BK_FRAME_BUFFER_MAX_SIZE)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        __STRBT(true, &data_p->used); //data_p->used = true;
        bk_802154_ot_received_done(data_p);
        xTaskNotifyFromISR(bk_ot_get_thread_main_hdl(), OT_MAINLOOP_PROC_RADIO, eSetBits, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

}

static void ieee802154_timer_stop(void)
{
    // user remember to add some timers stop function
}
static void select_next_rx_buffer(void)
{
    uint8_t index = 0;
    next_rx_frame_buffer = NULL;

    if (s_rx_frame_index == BK_FRAME_BUFFER_MAX_SIZE)
        s_rx_frame_index = 0;
    for (uint8_t i = 0; i < BK_FRAME_BUFFER_MAX_SIZE; i++)
    {
        index = (s_rx_frame_index + i) % BK_FRAME_BUFFER_MAX_SIZE;
        if(__LDRBT(&s_frame_buf[index].used))//(s_frame_buf[index].used == true)
        {
            continue;
        }
        else
        {
            s_rx_frame_index = index;
            next_rx_frame_buffer = &s_frame_buf[index];
            break;
        }
    }
    // rx_frame_buffer full
    if (next_rx_frame_buffer == NULL)
    {
        s_rx_frame_index = BK_FRAME_BUFFER_MAX_SIZE;
        next_rx_frame_buffer = &s_frame_buf[BK_FRAME_BUFFER_MAX_SIZE];
        MAC802154_LOGE("rx buffer full\n");
    }
}

static bool stop_rx(void)
{
    uint16_t uIntrEnReg = 0;
    uIntrEnReg = lw_mac802154_get_register_thread_intr_en();
    lw_mac802154_set_register_thread_intr_en((uIntrEnReg & 0xFFEF));
    lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
    lw_mac802154_set_register_thread_intr_en(uIntrEnReg);

    return true;
}
static bool stop_rx_ack(void)
{
    uint16_t uIntrEnReg = 0;
    uIntrEnReg = lw_mac802154_get_register_thread_intr_en();
    lw_mac802154_set_register_thread_intr_en((uIntrEnReg & 0xFFEF));
    lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
    lw_mac802154_set_register_thread_intr_en(uIntrEnReg);

    return true;
}
static bool stop_tx(void)
{

    return true;
}

static bool stop_tx_ack(void)
{

    return true;
}

static bool stop_tx_cca(void)
{

    lw_mac802154_lw_macl_stop_hw_timer_pl();
    return true;
}

static bool stop_ed(void)
{
    uint16_t uIntrEnReg = 0;
    uIntrEnReg = lw_mac802154_get_register_thread_intr_en();
    lw_mac802154_set_register_thread_intr_en((uIntrEnReg & 0xFFEF));
    lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
    lw_mac802154_set_register_thread_intr_en(uIntrEnReg);
    return true;
}

static bool stop_cca(void)
{
    uint16_t uIntrEnReg = 0;
    uIntrEnReg = lw_mac802154_get_register_thread_intr_en();
    lw_mac802154_set_register_thread_intr_en((uIntrEnReg & 0xFFEF));
    lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
    lw_mac802154_set_register_thread_intr_en(uIntrEnReg);
    return true;
}
static void bk_802154_state_set_new_state(bk_802154_state_t sState)
{
    s_802154_state = sState;
}
static bool stop_current_operation(void)
{
    ieee802154_timer_stop();
    switch(bk_ieee802154_state_get())
    {
        case BK_802154_STATE_DISABLE:
            break;
        case BK_802154_STATE_IDLE:
            // lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
            // lw_mac802154_thread_tx_stop();
            break;
        case BK_802154_STATE_SLEEP:
            // Do noting
            break;
        case BK_802154_STATE_RECEIVE:
            stop_rx();
            break;
        case BK_802154_STATE_RECEIVE_ACK:
            break;
        case BK_802154_STATE_TRANSMIT:
        case BK_802154_STATE_TRANSMIT_ACK:
        case BK_802154_STATE_TRANSMIT_ENH_ACK:
            stop_tx();
            break;
        case BK_802154_STATE_TRANSMIT_CCA:
            stop_tx_cca();
            break;
        case BK_802154_STATE_ED:
            stop_ed();
            break;
        case BK_802154_STATE_CCA:
            stop_cca();
            break;
        default:
            break;
    }
    return true;
}
static void init_frame_buffer(void)
{
    os_memset(&g_rx_buffer, 0, sizeof(rx_pkt_ringbuffer));
}

static void ieee802154_timer0_start(void)
{
    lw_mac802154_lw_timer0_start();
}

static void ieee802154_timer0_stop(void)
{
    lw_mac802154_lw_timer0_stop();
}

static void ieee802154_timer0_set_max_value(uint32_t value)
{
    lw_mac802154_lw_timer0_set_max_value(value);
}

static void isr_handle_delay_tx(void)
{
    MAC802154_LOGD("handle delay tx\n");
    s_timer_pending = TIMER_NO_PENDING;
    if ((bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_CCA) && (!s_cca_channel_clear)){
        MAC802154_LOGD("cca tx failed as no clear channel\n");
        bk_802154_transmit_failed(BK_802154_TX_ERR_CCA_BUSY);
    } else {
        bk_ieee802154_tx_mode_trxconfig();
        lw_mac802154_lw_macl_tx_frame_pl((uint8_t*)(s_pending_tx_frame + 1), s_pending_tx_frame[0] - 2, 0);
    }
}

static void isr_handle_delay_rx(void)
{
    MAC802154_LOGD("handle delay rx\n");
    s_timer_pending = TIMER_NO_PENDING;
    lw_mac802154_lw_macl_rx_config_pl(LW_TRUE, 1);
}

static int8_t bk_802154_convert_lqi_to_rssi(uint8_t lqi_val)
{
    int8_t rssi_val;
    if (lqi_val == 0) {
        rssi_val = LW_MAC_RSSI_LOWER_LIMIT;
    } else if (lqi_val == 0xFF) {
        rssi_val = LW_MAC_RSSI_UPPER_LIMIT;
    } else {
        rssi_val = (lqi_val / LW_MAC_RSSI_TO_LQI_OFFSET_MULTIPLIER) - LW_MAC_RSSI_TO_LQI_OFFSET;
    }
    return rssi_val;
}

static void bk_timer_overflow_cb(timer_id_t id)
{
    if (s_timer_pending == TIMER_TX_PENDING) {
       isr_handle_delay_tx();
    }
    if (s_timer_pending == TIMER_RX_PENDING) {
       s_timer_pending = TIMER_NO_PENDING;
       isr_handle_delay_rx();
    }
    ieee802154_timer_stop();
}
static void do_next_operation(void)
{
    if (ptr_pending_tx_frame != NULL)
    {
        stop_current_operation();
        if (s_pending_tx_cca)
        {
            MAC802154_LOGD("cca mode\n");
            bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT_CCA);
            lw_mac802154_lw_macl_start_cca_pl(BK_ED_DURATION);
        }
        else
        {
            bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT);
            bk_ieee802154_tx_mode_trxconfig();
            lw_mac802154_lw_macl_tx_frame_pl((uint8_t *)s_pending_tx_frame + 1, s_pending_tx_frame[0]-2, 1);
        }
        ptr_pending_tx_frame = NULL;
        s_pending_tx_cca = false;
    }
    else
    {
        if (bk_ieee802154_get_rx_when_idle())
        {
            stop_current_operation();
            lw_mac802154_lw_macl_rx_config_pl_ext(LW_TRUE);
            bk_802154_state_set_new_state(BK_802154_STATE_RECEIVE);
        }
        else
        {
            lw_mac802154_thread_tx_stop();
            lw_mac802154_thread_ed_scan_stop();
            lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
            lw_mac802154_thread_timer_stop();
            bk_802154_enter_sleep();
        }
    }
}

bk_err_t bk_802154_enter_sleep(void)
{
    if (bk_ieee802154_state_get() != BK_802154_STATE_SLEEP)
    {
        stop_current_operation();
        bk_802154_state_set_new_state(BK_802154_STATE_SLEEP);
    }
    return BK_OK;
}
static bool thread_is_trans_ack_enhack(void)
{
    return ((bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ACK) || (bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ENH_ACK));
}

static bool thread_is_trans_ack(void)
{
    return (bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ACK);
}

static bool thread_is_trans_enhack(void)
{
    return (bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ENH_ACK);
}

static __IRAM_SEC void macl_isr_rx_frame_enh_ack(uint8_t event_type,  void *data, uint16_t datalen)
{
    OT_UNUSED_VARIABLE(data);
    OT_UNUSED_VARIABLE(datalen);
    bk_802154_received_done(rx_pkt_with_enh_ack);
    SET_NEEDS_NEXT_OPERATION(true);
}

static __IRAM_SEC void macl_isr_tx_complete_cb(uint8_t event_type,  void *data, uint16_t datalen)
{
    uint8_t *data_buf = (uint8_t *)data;

    if(thread_is_trans_ack_enhack())
    {//transmit ack or enhance-ack
        macl_isr_rx_frame_enh_ack(event_type, data, datalen);
        return;
    }
    if ((bk_ieee802154_state_get() != BK_802154_STATE_TRANSMIT) && (bk_ieee802154_state_get() != BK_802154_STATE_TRANSMIT_CCA))
        return;
    if ((datalen == 2) && (data_buf[0] == LW_MAC_ENUM_NO_ACK) && (data_buf[1] == 0))
    {
        bk_802154_transmit_failed(BK_802154_TX_ERR_NO_ACK);
        SET_NEEDS_NEXT_OPERATION(true);
    }
    else
    {
        if (ieee802154_frame_get_type(&s_pending_tx_frame[1]) == 0x02) //Acknowledgement Frame type
        {
            MAC802154_LOGD("ack tx done...\n");
            SET_NEEDS_NEXT_OPERATION(false);
            return; //Do nothing.
        }
        bk_802154_ot_transmit_done_no_ack();
        SET_NEEDS_NEXT_OPERATION(true);
    }

}
static __IRAM_SEC void macl_isr_rx_ack_cb(uint8_t event_type, void *data, uint16_t datalen)
{
    if ((bk_ieee802154_state_get() != BK_802154_STATE_TRANSMIT) && (bk_ieee802154_state_get() != BK_802154_STATE_TRANSMIT_CCA))
        return;
    uint8_t *data_buf = (uint8_t *)data;
    select_next_rx_buffer();
    bk_802154_frame_t *rx_ack = next_rx_frame_buffer;

    rx_ack->frame[0] = datalen + 1;  //add 1 byte for crc, 1 byte is LQI which is already added.

    MAC802154_LOGD("received ack for tx.....\n");
    os_memcpy(&rx_ack->frame[1], data, datalen);
    rx_ack->pending = data_buf[0] & 0x10;
    rx_ack->lqi = data_buf[datalen-1];
    rx_ack->rssi = bk_802154_convert_lqi_to_rssi(data_buf[datalen-1]);
    rx_ack->channel = bk_ieee802154_channel_get();
    MAC802154_LOGD("lqi:%d, rssi:%d\n", rx_ack->lqi, rx_ack->rssi);
#if CONFIG_MAC802154_RX_SFD_EN
    rx_ack->time = uRxSfdTimeStamp; //otPlatTimeGet() - datalen * 32 - 120;
#else
    rx_ack->time = otPlatTimeGet() - datalen * 32 - 120;
#endif
    __STRBT(true, &rx_ack->used);//rx_ack->used = true;
    bk_802154_transmit_done_with_ack(next_rx_frame_buffer);
    SET_NEEDS_NEXT_OPERATION(true);
}

static __IRAM_SEC void macl_isr_rx_frame_cb(uint8_t event_type, void *data, uint16_t datalen)
{
    uint8_t *data_buf = (uint8_t *)data;
    if(bk_ieee802154_state_get() != BK_802154_STATE_RECEIVE)
        return;
    if (datalen > IEEE802154_FRAME_MAX_LEN)
    {
        SET_NEEDS_NEXT_OPERATION(true);
        return;
    }
    select_next_rx_buffer();
    next_rx_frame_buffer->enh_ack = false;
    next_rx_frame_buffer->frame[0] = datalen + 1;  //add 1 byte for crc, 1 byte is LQI which is already added.
    os_memcpy(&next_rx_frame_buffer->frame[1], data_buf, datalen);
#if CONFIG_MAC802154_RX_SFD_EN
    next_rx_frame_buffer->time = uRxSfdTimeStamp;
#else
    next_rx_frame_buffer->time = otPlatTimeGet() - datalen * 32 - 120;
#endif
    next_rx_frame_buffer->lqi = data_buf[datalen - 1];
    next_rx_frame_buffer->rssi = bk_802154_convert_lqi_to_rssi(next_rx_frame_buffer->lqi);
    next_rx_frame_buffer->channel = bk_ieee802154_channel_get();
    s_recent_rssi = next_rx_frame_buffer->rssi;
    if (data_buf[0] & 0x20)//Acknoledge Requset=True
    {
        next_rx_frame_buffer->pending = ieee802154_ack_config_pending_bit(data_buf);
        bk_ieee802154_tx_mode_trxconfig();
        if(ieee802154_frame_get_version(data_buf) < IEEE802154_FRAME_VERSION_2)
        {
            uint8_t * ack_data = bk_ieee802154_imm_ack_generator_create(data_buf, next_rx_frame_buffer->pending);
            lw_mac802154_lw_macl_tx_frame_pl(ack_data, IEEE802154_IMM_ACK_LENGTH - 2, 0);
            bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT_ACK);
        }
        else
        {
#if 0//CONFIG_MAC802154_TX_SFD_EN
            extern uint8_t* bk_802154_send_enh_ack(bk_802154_frame_t *data_p);
            uint8_t * enh_ack = bk_802154_send_enh_ack(next_rx_frame_buffer);// generate enh-ack not process
            if(enh_ack != NULL)
            {
                lw_mac802154_lw_macl_tx_frame_pl(enh_ack + 1, enh_ack[0] - 2, 0);
            }

#else
            lw_mac802154_lw_macl_tx_frame_pl_ack_start_tx((uint8_t*)next_rx_frame_buffer, next_rx_frame_buffer->frame[0], 0);
            extern uint8_t* bk_802154_send_enh_ack(bk_802154_frame_t *data_p);
            uint8_t * enh_ack = bk_802154_send_enh_ack(next_rx_frame_buffer);// generate enh-ack not process
            if(enh_ack != NULL)
            {
                lw_mac802154_lw_macl_tx_frame_pl_ack_payload(enh_ack + 1, enh_ack[0] - 2, 0);
            }
#endif
            bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT_ENH_ACK);
            next_rx_frame_buffer->enh_ack = true;
        }
        rx_pkt_with_enh_ack = next_rx_frame_buffer;
        SET_NEEDS_NEXT_OPERATION(false);
        return;
    }
    else
    {
        next_rx_frame_buffer->pending = false;
    }
    bk_802154_received_done(next_rx_frame_buffer);
    SET_NEEDS_NEXT_OPERATION(true);
}

static __IRAM_SEC void macl_isr_ch_ed_cb(uint8_t event_type, void *data, uint16_t datalen)
{
    MAC802154_LOGD("ed done...\n");
    s_recent_rssi = bk_802154_convert_lqi_to_rssi(*((uint8_t *)data));
    bk_802154_energy_detected(s_recent_rssi);
    SET_NEEDS_NEXT_OPERATION(true);
}

static __IRAM_SEC void macl_isr_ch_cca_cb(uint8_t event_type, void *data, uint16_t datalen)
{
    static uint8_t cca_nb = 0;
    static uint8_t cca_be = BK_MAC_MIN_BE;
    uint32_t cca_random_delay = 0;

    uint8_t channel_state;
    API_RESULT ret;
    MAC802154_LOGD("cca done...\n");
    if (bk_ieee802154_state_get() == BK_802154_STATE_CCA)
    {
        stop_current_operation();
        bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT_CCA);
        channel_state = ((uint8_t *)data)[0];
        if (LW_FALSE == channel_state)
        {
            MAC802154_LOGD("channel is free\n");
            cca_nb = 0;
            cca_be = BK_MAC_MIN_BE;
            bk_ieee802154_tx_mode_trxconfig();
            ret = lw_mac802154_lw_macl_tx_frame_pl((uint8_t*)(s_pending_tx_frame + 1), s_pending_tx_frame[0] - 2, 0);
            if (API_FAILURE_NO_ACK == ret)
            {
                MAC802154_LOGE("Wait ACK failed\n");
                bk_802154_transmit_failed(BK_802154_TX_ERR_NO_ACK);
            }
            else if (API_SUCCESS != ret)
            {
                MAC802154_LOGE("channel access failed\n");
                s_cca_channel_clear = false;
                bk_802154_transmit_failed(BK_802154_TX_ERR_CCA_BUSY);
            }
        }
        else
        {
            s_cca_channel_clear = false;
            cca_nb += 1;
            cca_be += 1;
            if (cca_be > BK_MAC_MAX_BE)
            {
                cca_be = BK_MAC_MAX_BE;
            }
            if (cca_nb > BK_MAC_MAX_CSMA_BACKOFFS)
            {
                MAC802154_LOGE("failed:exceed max number of CSMA retries...\n");
                cca_nb = 0;
                cca_be = BK_MAC_MIN_BE;
                bk_802154_state_set_new_state(BK_802154_STATE_IDLE);
                bk_802154_transmit_failed(BK_802154_TX_ERR_CCA_BUSY);
            }
            else
            {
                cca_random_delay = bk_802154_gen_random_with_limitation((1<<cca_be) - 1);
                lw_mac802154_lw_macl_start_hw_timer_pl(cca_random_delay * LW_MAC_A_UNIT_BACKOFF_PERIOD,
                                          0x00,
                                          0x01);
            }
        }
        SET_NEEDS_NEXT_OPERATION(false);
    }

}

static __IRAM_SEC void macl_isr_hw_timeout_cb(uint8_t event_type, void *data, uint16_t datalen)
{
    MAC802154_LOGD("time out...\n");
    if ((bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_CCA) && (!s_cca_channel_clear))
    {
        MAC802154_LOGD("cca backoff...\n");
        stop_current_operation();
        bk_802154_state_set_new_state(BK_802154_STATE_CCA);
        lw_mac802154_lw_macl_start_cca_pl (BK_ED_DURATION);
    }
    SET_NEEDS_NEXT_OPERATION(false);
}

static __IRAM_SEC void macl_isr_err_cb(uint8_t event_type, void *data, uint16_t datalen)
{
    __maybe_unused_var(event_type);
    __maybe_unused_var(data);
    __maybe_unused_var(datalen);
    MAC802154_LOGD("[Warning]macl_cb error type:0x%x state:%d\n", event_type, s_802154_state);
}
static __IRAM_SEC void macl_isr_rx_sfd_cb(uint8_t event_type, void *data, uint16_t datalen)
{
#if CONFIG_MAC802154_RX_SFD_EN
    uRxSfdTimeStamp = (uint64_t)otPlatTimeGet();
#endif
    // MAC802154_LOGD("[Del]%s: rxSfd:%llu\n", __func__, uRxSfdTimeStamp);
}

static __IRAM_SEC void macl_isr_tx_sfd_cb(uint8_t event_type, void *data, uint16_t datalen)
{
#if CONFIG_MAC802154_TX_SFD_EN
    if((s_802154_state == BK_802154_STATE_TRANSMIT) || (s_802154_state == BK_802154_STATE_TRANSMIT_CCA))
        bk_ot_tx_frame_ack_sfd_process(0);
    else if (s_802154_state == BK_802154_STATE_TRANSMIT_ENH_ACK)
        bk_ot_tx_frame_ack_sfd_process(1);
    // MAC802154_LOGD("[Del]%s\n", __func__);
#endif
}

/**
 * @brief x:event_type y:s_802154_state
 *
 */
typedef void (*macl_isr_func_t)(uint8_t event_type, void *data, uint16_t datalen);
static macl_isr_func_t  macl_cb_event_state_matrix[][9] = {
    //MACL_ISR_TX_COMPLETE       MACL_ISR_RX_FRAME          MACL_ISR_CH_ED             MACL_ISR_CH_CCA            MACL_ISR_HW_TIMEOUT     MACL_ISR_RX_ACK       MACL_ISR_RX_SFD         MACL_ISR_TX_SFD
    {macl_isr_err_cb,            macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_err_cb,        macl_isr_err_cb,   }, //BK_802154_STATE_DISABLE
    {macl_isr_err_cb,            macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_err_cb,        macl_isr_err_cb,   }, //BK_802154_STATE_IDLE
    {macl_isr_err_cb,            macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_err_cb,        macl_isr_err_cb,   }, //BK_802154_STATE_SLEEP
    {macl_isr_err_cb,            macl_isr_rx_frame_cb,      macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_rx_sfd_cb,     macl_isr_err_cb,   }, //BK_802154_STATE_RECEIVE
    {macl_isr_err_cb,            macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_rx_sfd_cb,     macl_isr_err_cb,   }, //BK_802154_STATE_RECEIVE_ACK
    {macl_isr_tx_complete_cb,    macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_hw_timeout_cb, macl_isr_rx_ack_cb,   macl_isr_rx_sfd_cb,     macl_isr_tx_sfd_cb,}, //BK_802154_STATE_TRANSMIT
    {macl_isr_tx_complete_cb,    macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_err_cb,        macl_isr_tx_sfd_cb,}, //BK_802154_STATE_TRANSMIT_ACK
    {macl_isr_tx_complete_cb,    macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_err_cb,        macl_isr_tx_sfd_cb,}, //BK_802154_STATE_TRANSMIT_ENH_ACK
    {macl_isr_tx_complete_cb,    macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_hw_timeout_cb, macl_isr_rx_ack_cb,   macl_isr_rx_sfd_cb,     macl_isr_tx_sfd_cb,}, //BK_802154_STATE_TRANSMIT_CCA
    {macl_isr_err_cb,            macl_isr_err_cb,           macl_isr_ch_ed_cb,         macl_isr_err_cb,           macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_rx_sfd_cb,     macl_isr_err_cb,   }, //BK_802154_STATE_ED
    {macl_isr_err_cb,            macl_isr_err_cb,           macl_isr_err_cb,           macl_isr_ch_cca_cb,        macl_isr_err_cb,        macl_isr_err_cb,      macl_isr_rx_sfd_cb,     macl_isr_err_cb,   }, //BK_802154_STATE_CCA
};
static char *eventype2string(uint8_t event)
{
    switch(event)
    {
       case MACL_ISR_TX_COMPLETE:
        return "MACL_ISR_TX_COMPLETE";
       case MACL_ISR_RX_FRAME:
        return "MACL_ISR_RX_FRAME";
       case MACL_ISR_CH_ED:
        return "MACL_ISR_CH_ED";
       case MACL_ISR_CH_CCA:
        return "MACL_ISR_CH_CCA";
       case MACL_ISR_HW_TIMEOUT:
        return "MACL_ISR_HW_TIMEOUT";
       case MACL_ISR_RX_ACK:
        return "MACL_ISR_RX_ACK";
       case MACL_ISR_RX_SFD:
        return "MACL_ISR_RX_SFD";
       case MACL_ISR_TX_SFD:
        return "MACL_ISR_TX_SFD";
       default:
        return "MACL_ISR_ERR";
    }
}
static char * macState2String(bk_802154_state_t st)
{
    switch(st)
    {
        case BK_802154_STATE_DISABLE:
            return "BK_802154_STATE_DISABLE";
        case BK_802154_STATE_IDLE:
            return "BK_802154_STATE_IDLE";
        case BK_802154_STATE_SLEEP:
            return "BK_802154_STATE_SLEEP";
        case BK_802154_STATE_RECEIVE:
            return "BK_802154_STATE_RECEIVE";
        case BK_802154_STATE_RECEIVE_ACK:
            return "BK_802154_STATE_RECEIVE_ACK";
        case BK_802154_STATE_TRANSMIT:
            return "BK_802154_STATE_TRANSMIT";
        case BK_802154_STATE_TRANSMIT_ACK:
            return "BK_802154_STATE_TRANSMIT_ACK";
        case BK_802154_STATE_TRANSMIT_ENH_ACK:
            return "BK_802154_STATE_TRANSMIT_ENH_ACK";
        case BK_802154_STATE_TRANSMIT_CCA:
            return "BK_802154_STATE_TRANSMIT_CCA";
        case BK_802154_STATE_ED:
            return "BK_802154_STATE_ED";
        case BK_802154_STATE_CCA:
            return "BK_802154_STATE_CCA" ;
        default:
            return "BK_802154_STATE_ERR";
    }
}
static API_RESULT macl_cb(uint8_t event_type, uint16_t event_result, void *data, uint16_t datalen)
{
    MAC802154_LOGD("[Del]macl cb type:%s state:%s\n", eventype2string(event_type), macState2String(s_802154_state));
    macl_cb_event_state_matrix[bk_ieee802154_state_get()][event_type-MACL_ISR_TX_COMPLETE](event_type, data, datalen);
    if(b_needs_next_operation)
    {
        do_next_operation();
        b_needs_next_operation = false;
    }
    return 0;
}
#endif

void mac802154_mac_reset(void)
{
    lw_mac802154_thread_tx_stop();
    lw_mac802154_lw_macl_rx_config_pl_ext(false);
    lw_mac802154_write_command_register_cont_rx_stop();
}

static void mac802154_mac_enable(void)
{
    sys_drv_thread_interrupt_ctrl(1);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_THREAD, PM_POWER_MODULE_STATE_ON);
    rtos_delay_milliseconds(1);
    bk_pm_clock_ctrl(CLK_PWR_ID_THREAD, 1);
    rtos_delay_milliseconds(1);
    sys_drv_thread_rf_ctrl(1);
    rtos_delay_milliseconds(1);
    rf_module_vote_ctrl(1, RF_BY_THREAD_BIT);
    rtos_delay_milliseconds(1);
    // lw_mac802154_thread_soft_reset();
    lw_mac802154_lw_macl_hw_isr_register();
}

static void mac802154_mac_disable(void)
{
    sys_drv_thread_interrupt_ctrl(0);
    bk_pm_clock_ctrl(CLK_PWR_ID_THREAD, 0);
    sys_drv_thread_rf_ctrl(0);
    rf_module_vote_ctrl(0, RF_BY_THREAD_BIT);
}

#if CONFIG_OPENTHREAD
bk_err_t bk_ieee802154_enable(void)
{
    init_frame_buffer();

    mac802154_mac_enable();

    /* MACL platform init */
    lw_mac802154_lw_macl_init_pl();
#if CONFIG_MAC802154_RX_SFD_EN && CONFIG_MAC802154_TX_SFD_EN
    lw_mac802154_int_rx_tx_sfd_matched_en();
#elif CONFIG_MAC802154_RX_SFD_EN
    lw_mac802154_int_rx_sfd_matched_en();
#elif CONFIG_MAC802154_TX_SFD_EN
    lw_mac802154_int_tx_sfd_matched_en();
#endif
    /* Register ISR */
    lw_mac802154_lw_macl_register_isr_pl(macl_cb);

    // {
    //     uint16_t value = 0;
    //     value = lw_mac802154_read_register_REG0xCF_AGC_CTRL0();
    //     value = value & 0xFF00;
    //     value = value + 0x0F;
    //     lw_mac802154_write_register_REG0xCF_AGC_CTRL0(value);
    // }

    bk_802154_state_set_new_state(BK_802154_STATE_IDLE);
    return BK_OK;
}

bk_err_t bk_ieee802154_disable(void)
{
    mac802154_mac_disable();
    bk_802154_state_set_new_state(BK_802154_STATE_DISABLE);
    return BK_OK;
}
#endif

uint8_t bk_ieee802154_channel_get(void)
{
    uint8_t value = 0;
    lw_mac802154_lw_mac_get_cur_channel_pl(&value);
    value = value / 5 + 10;
    return value;
}

bk_err_t bk_ieee802154_channel_set(uint8_t channel)
{
    if (channel == bk_ieee802154_channel_get())
    {
        return BK_OK;
    }
    if ((channel < 11) || (channel > 26))
    {
        MAC802154_LOGE("[Error]%s channel%d out of range\r\n", __func__, channel);
        return BK_ERR_PARAM;
    }
    lw_mac802154_lw_mac_set_channel_pl(channel * 5 - 50);
    return BK_OK;
}

void bk_ieee802154_tx_mode_trxconfig(void)
{
    extern void sys_set_thread_tx_mode_trxconfig(void);
    sys_set_thread_tx_mode_trxconfig();
}

int8_t bk_ieee802154_get_txpower(void)
{
    return BK_OK; //TODO
}

bk_err_t bk_ieee802154_set_txpower(int8_t power)
{
    return BK_OK; //TODO
}

bool bk_ieee802154_get_promiscuous(void)
{
    uint16_t val = 0;
    val = lw_mac802154_le_read_THRAD_CTRL_CONFIG();
    val = val & 0x0080;

    return (val == 0x0080 ? true : false);
}

bk_err_t bk_ieee802154_set_promiscuous(bool enable)
{
    uint16_t val;
    val = lw_mac802154_le_read_THRAD_CTRL_CONFIG();
    if (enable)
    {
        /* Set 7th bit position */
        val = (val | (0x0080));
    }
    else
    {
        /* Reset 7th bit position */
        val = (val & 0xFF7F);
    }

    lw_mac802154_le_write_THREAD_CTRL_CONFIG(val);
    return BK_OK;
}

#if CONFIG_OPENTHREAD
bk_err_t bk_ieee802154_receive(void)
{
    stop_current_operation();//
    lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
    bk_802154_state_set_new_state(BK_802154_STATE_RECEIVE);
    lw_mac802154_lw_macl_rx_config_pl(LW_TRUE, 1);
    return BK_OK;
}
bk_err_t bk_ieee802154_receive_handle_done(const uint8_t *data)
{
    uint16 size = data - (uint8_t *)&s_frame_buf[0];
    uint8_t uIdx = 0;
    if ((size % sizeof(bk_802154_frame_t) != 0) ||
        (size / sizeof(bk_802154_frame_t) > BK_FRAME_BUFFER_MAX_SIZE))
    {
        return BK_FAIL;
    }
    uIdx = size / sizeof(bk_802154_frame_t);
    __STRBT(false, &s_frame_buf[uIdx].used);//s_frame_buf[uIdx].used = false;
    return BK_OK;
}

bk_err_t bk_ieee802154_transmit(const uint8_t *frame, bool cca)
{
    if (frame[0] > 127)
        return BK_ERR_PARAM;
    //cache the transmitting frame, in case failure
    os_memcpy(s_pending_tx_frame, frame, frame[0] + 1);
    if ((bk_ieee802154_state_get() == BK_802154_STATE_RECEIVE && (lw_mac802154_get_register_thread_mac_rx_status_rx_cs() >= THREAD_MAC_CURRENT_RX_STATUS_P_LENGTH))//0x05 - P_LENGTH
        || (bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ACK) || (bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ENH_ACK))
    {
        ptr_pending_tx_frame = s_pending_tx_frame;
        s_pending_tx_cca = cca;
        return BK_OK;
    }

    stop_current_operation();

    if (cca)
    {
        MAC802154_LOGD("cca mode\n");
        bk_802154_state_set_new_state(BK_802154_STATE_CCA);//BK_802154_STATE_TRANSMIT_CCA;
        lw_mac802154_lw_macl_start_cca_pl(BK_ED_DURATION);
    }
    else
    {
        bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT);
        bk_ieee802154_tx_mode_trxconfig();
        lw_mac802154_lw_macl_tx_frame_pl((uint8_t *)frame + 1, frame[0] - 2, 1);
    }
    return BK_OK;
}

bk_err_t bk_ieee802154_transmit_ack(const uint8_t *frame)
{
    bk_ieee802154_tx_mode_trxconfig();
    lw_mac802154_lw_macl_tx_frame_pl((uint8_t *)frame + 1, frame[0] - 2, 1);
    bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT_ACK);
    return BK_OK;
}

bk_err_t bk_ieee802154_receive_at(uint32_t time)
{
    uint64_t current_time;
    current_time = otPlatTimeGet();
    stop_current_operation();
    bk_802154_state_set_new_state(BK_802154_STATE_RECEIVE);
    s_timer_pending = TIMER_RX_PENDING;
    ieee802154_timer_stop();
    bk_timer_delay_with_callback(ACK_TIMER_ID,(time > current_time) ? (time - current_time) : 0, bk_timer_overflow_cb);

    return BK_OK;
}

bk_err_t bk_ieee802154_transmit_at(const uint8_t *frame, bool cca, uint32_t time)
{
    uint64_t current_time;
    os_memcpy(s_pending_tx_frame, frame, frame[0] + 1);

    if ((bk_ieee802154_state_get() == BK_802154_STATE_RECEIVE && (lw_mac802154_get_register_thread_mac_rx_status_rx_cs() >= THREAD_MAC_CURRENT_RX_STATUS_P_LENGTH))
        ||(bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ACK) || (bk_ieee802154_state_get() == BK_802154_STATE_TRANSMIT_ENH_ACK))
    {
        ptr_pending_tx_frame = s_pending_tx_frame;
        s_pending_tx_cca = cca;
        return BK_OK;
    }

    stop_current_operation();

    //uint32_t fire_time;
    current_time = otPlatTimeGet();
    if (cca) {
        //when the time is over
        //need to wait cca to trigger tx
        MAC802154_LOGD("cca mode\n");
        bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT_CCA);
        lw_mac802154_lw_macl_start_cca_pl(BK_ED_DURATION);
    } else {
        //when the time is over
        //start tx
        bk_802154_state_set_new_state(BK_802154_STATE_TRANSMIT);
        //set flag
        s_timer_pending = TIMER_TX_PENDING;
        ieee802154_timer_stop();

        bk_timer_delay_with_callback(ACK_TIMER_ID, (time > current_time) ? (time - current_time) : 0, bk_timer_overflow_cb);

    }
    return BK_OK;
}
#endif

uint16_t bk_ieee802154_get_panid(void)
{
    return lw_mac802154_get_mac_pan_id();
}

bk_err_t bk_ieee802154_set_panid(uint16_t panid)
{
    lw_mac802154_set_mac_pan_id(panid);
    return BK_OK;
}

uint16_t bk_ieee802154_get_short_address(void)
{
    return lw_mac802154_get_short_address();
}

bk_err_t bk_ieee802154_set_short_address(uint16_t short_address)
{
    lw_mac802154_set_short_address(short_address);
    return BK_OK;
}

bk_err_t bk_ieee802154_get_extended_address(uint8_t *ext_addr)
{
    uint16_t temp = 0;
    temp = lw_mac802154_get_mac_extnd_addr0();
    ext_addr[0] = temp & 0x00FF;
    ext_addr[1] = temp >> 8;
    temp = lw_mac802154_get_mac_extnd_addr1();
    ext_addr[2] = temp & 0x00FF;
    ext_addr[3] = temp >> 8;
    temp = lw_mac802154_get_mac_extnd_addr2();
    ext_addr[4] = temp & 0x00FF;
    ext_addr[5] = temp >> 8;
    temp = lw_mac802154_get_mac_extnd_addr3();
    ext_addr[6] = temp & 0x00FF;
    ext_addr[7] = temp >> 8;

    return BK_OK;
}

bk_err_t bk_ieee802154_set_extended_address(const uint8_t *ext_addr)
{
    lw_mac802154_set_mac_extnd_addr0( (ext_addr[0]) | (ext_addr[1] << 8));
    lw_mac802154_set_mac_extnd_addr1( (ext_addr[2]) | (ext_addr[3] << 8));
    lw_mac802154_set_mac_extnd_addr2( (ext_addr[4]) | (ext_addr[5] << 8));
    lw_mac802154_set_mac_extnd_addr3( (ext_addr[6]) | (ext_addr[7] << 8));

    return BK_OK;
}

#if CONFIG_OPENTHREAD
bk_ieee802154_pending_mode_t bk_ieee802154_get_pending_mode(void)
{
    return ieee802154_get_pending_mode();
}

bk_err_t bk_ieee802154_set_pending_mode(bool pending_mode)
{
    ieee802154_set_pending_mode(pending_mode);
    return BK_OK;
}

bk_err_t bk_ieee802154_add_pending_addr(const uint8_t *addr, bool is_short)
{
    return ieee802154_add_pending_addr(addr, is_short);;
}

bk_err_t bk_ieee802154_clear_pending_addr(const uint8_t *addr, bool is_short)
{
    return ieee802154_clear_pending_addr(addr, is_short);;
}

bk_err_t bk_ieee802154_reset_pending_table(bool is_short)
{
    ieee802154_reset_pending_table(is_short);
    return BK_OK;
}

int8_t bk_ieee802154_get_cca_threshold(void)
{
    uint16_t value = lw_mac802154_get_cca_threshold();
    return (int8_t) (value & 0xFF);
}

bk_err_t bk_ieee802154_set_cca_threshold(int8_t cca_threshold)
{
    lw_mac802154_set_cca_threshold( (uint16_t)cca_threshold);
    return BK_OK;
}

bk_err_t bk_ieee802154_set_rx_when_idle(bool enable)
{
    s_rx_when_idle = enable;
    // if (enable) {
    //     lw_mac802154_lw_macl_rx_config_pl_ext(LW_TRUE);
    // } else {
    //     lw_mac802154_lw_macl_rx_config_pl_ext(LW_FALSE);
    // }
    return BK_OK;
}

bool bk_ieee802154_get_rx_when_idle(void)
{
    return s_rx_when_idle;
}

bk_err_t bk_ieee802154_energy_detect(uint32_t duration)
{
    stop_current_operation();
    lw_mac802154_lw_macl_start_ed_pl(duration, 0);
    bk_802154_state_set_new_state(BK_802154_STATE_ED);
    return BK_OK;
}

bk_802154_state_t bk_ieee802154_state_get(void)
{
    return s_802154_state;
}

void bk_ieee802154_energy_detect_done(int8_t power)
{}

int8_t bk_ieee802154_get_recent_rssi(void)
{
    return s_recent_rssi;
}

uint8_t bk_ieee802154_get_recent_lqi(void)
{
    return lw_mac802154_lw_mac_convert_rssi_to_lqi(bk_ieee802154_get_recent_rssi());
}
#endif

uint8_t mac802154_mac_init(void)
{
    mac802154_mac_enable();

    // lp_init();

    lw_mac802154_em_timer_init();

    lw_mac802154_lw_init();

    return BK_OK;
}

#ifdef BK_SELF_TEST_ENABLE
#if CONFIG_OPENTHREAD
//self test
//
//as test code
static void lp_init(void)
{
    // BK7236_TRX_REG.REG0x13->bits.lpfouttsten = 1;
    // BK7236_TRX_REG.REG0x13->bits.entst = 1;
    volatile uint32_t *trx = (volatile uint32_t*)(0x4980c24c);
    *trx |= ((1 <<0 ) | (1 << 4));

    //config p21/p22 9 function
    uint32_t temp = *(volatile uint32_t*)(0x44010000 + 0x32 * 4);
    temp &= ~(0xF << 24 | 0xF << 20);
    temp |= ((9 << 24) | (9 << 20));
    *(volatile uint32_t*)(0x44010000 + 0x32 * 4) = temp;
}

#define THREAD_TX_LENGTH1           9
#define THREAD_TX_LENGTH2           10
#define THREAD_TX_LENGTH3           11
#define THREAD_ACK_LENGTH           5
#define TX_FRAME_CONTROL_LSB        0x63
#define TX_FRAME_CONTROL_MSB        0x23
#define TX_SEQUENCE_NUMBER          0x12

uint8_t tx_data[THREAD_TX_LENGTH1+1] = {THREAD_TX_LENGTH1, TX_FRAME_CONTROL_LSB, TX_FRAME_CONTROL_MSB, TX_SEQUENCE_NUMBER, 0x45, 0x14, 0xAA, 0XBB};



void cli_mac802154(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (os_strcmp(argv[1], "tx") == 0) {

        //THREAD_CHANNEL REG hold freq offset value.example THREAD_CHANNEL = 0x5 is 2405Mhz
        uint8_t channel = 0x05;
        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }

        MAC802154_LOGD("tx channel %d\r\n",channel);
        bk_ieee802154_channel_set(channel);
        bk_ieee802154_transmit(tx_data, 0);
    } else if (os_strcmp(argv[1], "rx") == 0) {
        uint8_t channel = 0x05;
        uint8_t rx_mode = 0;
        static uint8_t rx_mode_save = 0;

        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }

        if (argc > 3) {
            rx_mode = (os_strtoul(argv[3], NULL, 10) > 0) ? 1 : 0;
        }

        MAC802154_LOGD("rx channel %d rx_mode %d\r\n", channel, rx_mode);

        if (rx_mode_save == 1) {
            lw_mac802154_write_command_register_cont_rx_stop();
        } else if (rx_mode_save == 2) {
            lw_mac802154_le_write_command_rx_stop();
        }

        lw_mac802154_lw_mac_set_channel_pl(channel);

        lw_mac802154_thread_hw_filter_disable();

        rtos_delay_milliseconds(10);

        if (rx_mode) {
            lw_mac802154_thread_continuous_rx_start();
            rx_mode_save = 1;
        } else {
            lw_mac802154_thread_rx_start();
            rx_mode_save = 2;
        }
    } else if (os_strcmp(argv[1], "ed") == 0) {
        uint8_t channel = 5;
        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }
        lw_mac802154_lw_mac_set_channel_pl(channel);
        lw_mac802154_write_command_register_ed_start();
    } else if (os_strcmp(argv[1], "tx_at") == 0) {
        MAC802154_LOGD("test tramsit at specific time\n");
        uint8_t channel = 0x05;
        uint8_t time = 20;
        if (argc > 2) {
            channel = os_strtoul(argv[2], NULL, 10);
        }

        if (argc > 3) {
            time = os_strtoul(argv[3], NULL, 10);
        }
        MAC802154_LOGD("tx channel %d\r\n",channel);
        bk_ieee802154_channel_set(channel);
        bk_ieee802154_transmit_at(tx_data, 0, time);
    } else if (os_strcmp(argv[1], "set_get_ch") == 0) {
        MAC802154_LOGD("test set and get channel\n");
        uint8_t channel_set = 6;
        bk_ieee802154_channel_set(channel_set);
        uint8_t channel_get = bk_ieee802154_channel_get();
        if (channel_get == channel_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (os_strcmp(argv[1], "set_get_ch") == 0) {
        MAC802154_LOGD("test set and get cca threshold\n");
        int8_t cca_threshold_set = -85;
        bk_ieee802154_set_cca_threshold(cca_threshold_set);
        int8_t cca_threshold_get = bk_ieee802154_get_cca_threshold();
        if(cca_threshold_get == cca_threshold_set){
            MAC802154_LOGD(" test pass \n");
        }else {
            MAC802154_LOGD(" test failed, get threshold %d\n", cca_threshold_get);
        }
    } else if (os_strcmp(argv[1], "set_get_panid") == 0) {
        MAC802154_LOGD("test set and get panid\n");
        uint16_t panid_set = 0x7;
        bk_ieee802154_set_panid(panid_set);
        uint16_t panid_get = bk_ieee802154_get_panid();
        if(panid_get == panid_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (os_strcmp(argv[1], "set_get_short_addr") == 0) {
        MAC802154_LOGD("test set and get short addr\n");
        uint16_t short_addr_set = 0x1234;
        bk_ieee802154_set_short_address(short_addr_set);
        uint16_t short_addr_get = 0;
        short_addr_get = bk_ieee802154_get_short_address();
        if(short_addr_get == short_addr_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (os_strcmp(argv[1], "set_get_extend_addr") == 0) {
        MAC802154_LOGD("test set and get extend_addr\n");
        const uint8_t extend_addr_set[8] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
        bk_ieee802154_set_extended_address(extend_addr_set);
        uint8_t extend_addr_get[8] = { 0 };
        bk_ieee802154_get_extended_address(extend_addr_get);
        if(0 == os_memcmp(extend_addr_get, extend_addr_set, 8)){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (os_strcmp(argv[1], "set_get_promiscuous") == 0) {
        MAC802154_LOGD("test set and get promiscuous\n");
        bool promiscuous_set = true;
        bk_ieee802154_set_promiscuous(promiscuous_set);
        bool promiscuous_get = bk_ieee802154_get_promiscuous();
        if ( promiscuous_get == promiscuous_set){
            MAC802154_LOGD(" test pass \n");
        }
    } else if (os_strcmp(argv[1], "set_get_txpower") == 0) {
        MAC802154_LOGD("test set and get txpower\n");
        int8_t txpower_set = 0;
        bk_ieee802154_set_txpower(txpower_set);
        int8_t txpower_get = bk_ieee802154_get_txpower();
        if ( txpower_get == txpower_set ){
            MAC802154_LOGD(" test pass \n");
        }
        //} else if (os_strcmp(argv[1], "") == 0) {
} else {
    MAC802154_LOGE("cmd not support!!!\r\n");
}
}
#endif
#endif
