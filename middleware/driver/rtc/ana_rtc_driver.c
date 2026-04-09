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

#include <common/bk_include.h>
#include <os/mem.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include "string.h"

#include <driver/int.h>
#include "sys_driver.h"
#include "sys_hal.h"
#include "aon_pmu_hal.h"
#include "sys/time.h"
#include <driver/aon_rtc.h>
#include <driver/wakeup.h>
#include "ana_rtc_driver.h"
#include <driver/rosc_32k.h>

typedef struct
{
    bool inited;

    // Alarm list
    alarm_node_t *alarm_head_p;
    uint32_t alarm_node_cnt;
} ana_rtc_driver_t;

typedef struct
{
    aon_rtc_isr_t callback;
    void *isr_param;
} ana_rtc_callback_t;

typedef struct
{
    alarm_node_t nodes[AON_RTC_MAX_ALARM_CNT];
    uint64_t busy_bits;
} ana_rtc_nodes_memory_t;

static ana_rtc_driver_t s_ana_rtc = {0};
static ana_rtc_callback_t s_ana_rtc_tick_isr = {NULL};
static ana_rtc_nodes_memory_t *s_ana_rtc_nodes_p = NULL;

static void aon_rtc_interrupt_disable(aon_rtc_id_t id);
static void aon_rtc_register_deepsleep_cb(void);
static void aon_rtc_compute_boot_timeofday(void);

#define ANA_RTC_SAMPLE_SEL_MASK        (0x1)
#define ANA_RTC_SAMPLE_POSITIVE_EDGE    (0x1)
#define ANA_RTC_SAMPLE_NEGATIVE_EDGE    (0x0)

#define ANA_RTC_GET_SET_TIME_RTC_ID   (AON_RTC_ID_1)
#define ANA_RTC_CURRENT_TICK        bk_aon_rtc_get_current_tick(ANA_RTC_GET_SET_TIME_RTC_ID)
static uint64_t s_boot_time_us       = 0; // timeofday value

#define ANA_RTC_OPS_SAFE_DELAY_US (125)
static uint64_t s_ana_rtc_base_tick = 0; // use for rtc reset
static uint64_t s_time_base_us      = 0; // use for freq switch
static uint64_t s_time_base_tick    = 0; // use for freq switch
static uint32_t s_low_tick          = 0;
static uint32_t s_high_tick         = 0;
#if !CONFIG_ANA_RTC_WAKUP_BY_RTC
static bool s_rtc_updated           = true;
#endif

__IRAM_SEC float bk_rtc_get_ms_tick_count(void)
{
    return (float)bk_rtc_get_clock_freq() / 1000.0f;
}

__IRAM_SEC uint32_t bk_rtc_get_clock_freq(void)
{
    #if CONFIG_EXTERN_32K
    return AON_RTC_EXTERN_32K_CLOCK_FREQ;
    #else
    return bk_rosc_32k_get_freq();
    #endif
}

void bk_rtc_set_clock_freq(uint32_t clock_freq)
{
    bk_aon_rtc_get_us(); //update base time to compensate the frequency change
}

__IRAM_SEC uint64_t bk_aon_rtc_tick_to_us(uint64_t tick)
{
    return (tick * 1000000) / bk_rtc_get_clock_freq();
}

__IRAM_SEC uint64_t bk_aon_rtc_us_to_tick(uint64_t us)
{
    return (us * bk_rtc_get_clock_freq()) / 1000000;
}

__IRAM_SEC uint64_t bk_aon_rtc_get_us(void)
{
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    uint64_t current_tick = ANA_RTC_CURRENT_TICK;
    uint64_t delta_us = bk_aon_rtc_tick_to_us(current_tick - s_time_base_tick);

    s_time_base_tick = current_tick;
    s_time_base_us += delta_us;
    GLOBAL_INT_RESTORE();
    return s_time_base_us;
}

__IRAM_SEC uint64_t bk_aon_rtc_get_ms(void)
{
    return bk_aon_rtc_get_us()/1000;
}

#if CONFIG_ROSC_COMPENSATION
__IRAM_SEC uint64_t bk_aon_rtc_get_current_tick_with_compensation(aon_rtc_id_t id)
{
    return bk_aon_rtc_get_us();
}
#endif

static void alarm_dump_node(alarm_node_t *node_p)
{
#if CONFIG_ANA_RTC_DEBUG
    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    ANA_RTC_LOGD("node_p=0x%x\r\n", node_p);
    if (node_p)
    {
        ANA_RTC_LOGD("next=0x%x\r\n", node_p->next);
        ANA_RTC_LOGD("name=%s\r\n", node_p->name);
        ANA_RTC_LOGD("period_tick=0x%x\r\n", node_p->period_tick);
        ANA_RTC_LOGD("period_cnt=%d\r\n", node_p->period_cnt);
        ANA_RTC_LOGD("start_tick=0x%x\r\n", node_p->start_tick);
        ANA_RTC_LOGD("expired_tick=0x%x\r\n", node_p->expired_tick);
    }

    ANA_RTC_LOGD("%s[-]\r\n", __func__);
#endif
}

static void alarm_dump_list(alarm_node_t *head_p)
{
#if CONFIG_ANA_RTC_DEBUG
    alarm_node_t *cur_p = head_p;
    uint32_t count = 0;
    uint32_t int_level = 0;

    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    int_level = rtos_disable_int();
    while (cur_p)
    {
        alarm_dump_node(cur_p);
        count++;

        cur_p = cur_p->next;
    }
    rtos_enable_int(int_level);

    ANA_RTC_LOGD("node cnt=%d\r\n", count);

    ANA_RTC_LOGD("%s[-]\r\n", __func__);
#endif
}

static alarm_node_t *ana_rtc_request_node(aon_rtc_id_t id)
{
    uint32_t i = 0;

    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    for (i = 0; i < AON_RTC_MAX_ALARM_CNT; i++)
    {
        if ((s_ana_rtc_nodes_p->busy_bits & (0x1 << i)) == 0)
        {
            ANA_RTC_LOGD("%s[-]:node[%d]=0x%x\r\n", __func__, i, &s_ana_rtc_nodes_p->nodes[i]);
            s_ana_rtc_nodes_p->busy_bits |= (0x1 << i);
            return &s_ana_rtc_nodes_p->nodes[i];
        }
    }

    return NULL;
}

static void ana_rtc_release_node(alarm_node_t *node_p)
{
    uint32_t i = 0;

    ANA_RTC_LOGD("%s[+]\r\n", __func__);
    if ((NULL == s_ana_rtc_nodes_p) || (NULL == node_p))
    {
        ANA_RTC_LOGE("s_ana_rtc_nodes_p or node_p is NULL\r\n");
        return;
    }

    for (i = 0; i < AON_RTC_MAX_ALARM_CNT; i++)
    {
        if (&s_ana_rtc_nodes_p->nodes[i] == node_p)
        {
            s_ana_rtc_nodes_p->busy_bits &= ~(0x1 << i);
            os_memset(&s_ana_rtc_nodes_p->nodes[i], 0, sizeof(alarm_node_t));
            ANA_RTC_LOGD("%s[-]:node[%d]=0x%x\r\n", __func__, i, &s_ana_rtc_nodes_p->nodes[i]);
            break;
        }
    }

    if (i >= AON_RTC_MAX_ALARM_CNT)
    {
        ANA_RTC_LOGE("release node err\r\n");
    }
}

static int32_t alarm_insert_node(aon_rtc_id_t id, alarm_node_t *node_p)
{
    alarm_node_t *cur_p = NULL;
    alarm_node_t *next_p = NULL;
    uint32_t int_level = 0;

    ANA_RTC_LOGD("%s[+]cnt=%d\r\n", __func__, s_ana_rtc.alarm_node_cnt);

    alarm_dump_list(s_ana_rtc.alarm_head_p);

    int_level = rtos_disable_int();

    // check whether the same name
    cur_p = s_ana_rtc.alarm_head_p;
    while (cur_p)
    {
        if (strncmp((const char *)cur_p->name, (const char *)node_p->name, ALARM_NAME_MAX_LEN) == 0)
        {
            ANA_RTC_LOGE("name=%s has registered\r\n", node_p->name);
            rtos_enable_int(int_level);
            return -1;
        }

        cur_p = cur_p->next;
    }

    // search the node position
    cur_p = s_ana_rtc.alarm_head_p;

    // no node
    if (cur_p == NULL)
    {
        s_ana_rtc.alarm_head_p = node_p;
        s_ana_rtc.alarm_node_cnt++;

        ANA_RTC_LOGD("insert first node 0x%x,name=%s\r\n", node_p, node_p->name);

        rtos_enable_int(int_level);
        return 0;
    }

    // only one node
    next_p = cur_p->next;
    if (next_p == NULL)
    {
        if (cur_p->expired_tick <= node_p->expired_tick)
            cur_p->next = node_p;
        else
        {
            node_p->next = cur_p;
            s_ana_rtc.alarm_head_p = node_p;
        }
        s_ana_rtc.alarm_node_cnt++;
        rtos_enable_int(int_level);

        // TODO:log debug
        ANA_RTC_LOGD("list total has two nodes\r\n");

        return 0;
    }

    // more then 2 nodes
    while (next_p)
    {
        if (cur_p->expired_tick <= node_p->expired_tick) // move after cur_p
        {
            if (next_p->expired_tick <= node_p->expired_tick) // search next
            {
                cur_p = next_p;
                next_p = next_p->next;
                continue;
            }
            else // insert
            {
                node_p->next = next_p;
                cur_p->next = node_p;
                s_ana_rtc.alarm_node_cnt++;
                rtos_enable_int(int_level);
                return 0;
            }
        }
        else // insert before cur_p, means the first node, head
        {
            node_p->next = cur_p;
            s_ana_rtc.alarm_head_p = node_p;
            s_ana_rtc.alarm_node_cnt++;
            rtos_enable_int(int_level);
            return 0;
        }
    }

    // the last one
    cur_p->next = node_p;
    s_ana_rtc.alarm_node_cnt++;
    rtos_enable_int(int_level);

    // dump list info
    alarm_dump_list(s_ana_rtc.alarm_head_p);

    ANA_RTC_LOGD("%s[-]cnt=%d\r\n", __func__, s_ana_rtc.alarm_node_cnt);

    return 0;
}

static alarm_node_t *alarm_remove_node(aon_rtc_id_t id, uint8_t *name_p)
{
    alarm_node_t *cur_p = NULL;
    alarm_node_t *previous_p = NULL;
    alarm_node_t *remove_node_p = NULL;
    uint32_t int_level = 0;
    uint32_t node_cnt = 0;

    ANA_RTC_LOGD("%s[+]cnt=%d\r\n", __func__, s_ana_rtc.alarm_node_cnt);

    int_level = rtos_disable_int();
    //
    previous_p = cur_p = s_ana_rtc.alarm_head_p;
    while (cur_p)
    {
        // double check pointer is valid
        node_cnt++;
        BK_ASSERT(node_cnt <= AON_RTC_MAX_ALARM_CNT); /* ASSERT VERIFIED */

        if (strncmp((const char *)cur_p->name, (const char *)name_p, ALARM_NAME_MAX_LEN) == 0)
        {
            // first one
            if (previous_p == cur_p)
            {
                remove_node_p = s_ana_rtc.alarm_head_p;
                s_ana_rtc.alarm_head_p = cur_p->next;
                s_ana_rtc.alarm_node_cnt--;

                ANA_RTC_LOGD("free=0x%x,name=%s\r\n", cur_p, cur_p->name);
                ana_rtc_release_node(cur_p);
                break;
            }
            else
            {
                remove_node_p = cur_p;
                previous_p->next = cur_p->next;
                s_ana_rtc.alarm_node_cnt--;
                ANA_RTC_LOGD("free=0x%x,name=%s\r\n", cur_p, cur_p->name);
                ana_rtc_release_node(cur_p);
                break;
            }
        }

        previous_p = cur_p;
        cur_p = cur_p->next;
    }

    rtos_enable_int(int_level);

    if (remove_node_p == NULL)
    {
        ANA_RTC_LOGD("%s:can't find %s alarm\r\n", __func__, name_p);
    }

    // dump list info
    alarm_dump_list(s_ana_rtc.alarm_head_p);

    ANA_RTC_LOGD("%s[-]cnt=%d\r\n", __func__, s_ana_rtc.alarm_node_cnt);

    return remove_node_p;
}

static void alarm_update_expeired_nodes(aon_rtc_id_t id)
{
    alarm_node_t *cur_p = NULL;
    alarm_node_t *next_p = NULL;
    uint32_t node_cnt = 0;
    uint64_t cur_tick = 0;
    uint32_t int_level = 0;

    ANA_RTC_LOGD("%s[+]cnt=%d\r\n", __func__, s_ana_rtc.alarm_node_cnt);

    alarm_dump_list(s_ana_rtc.alarm_head_p);

    int_level = rtos_disable_int();

    // search the node position
    cur_p = s_ana_rtc.alarm_head_p;
    while (cur_p)
    {
        alarm_dump_node(cur_p);
        alarm_dump_node(cur_p->next);

        // double check pointer is valid
        node_cnt++;
        BK_ASSERT(node_cnt <= AON_RTC_MAX_ALARM_CNT); /* ASSERT VERIFIED */

        next_p = cur_p->next;

        cur_tick = bk_aon_rtc_get_current_tick(id);
        // maybe callback runs too much time,so assume the time in bk_rtc_get_ms_tick_count() means has expired
        if (cur_p->expired_tick <= cur_tick + AON_RTC_PRECISION_TICK)
        {
            if (cur_p->callback)
            {
                cur_p->callback(id, cur_p->name, cur_p->cb_param_p);
            }

            // last time alarm
            if (cur_p->period_cnt == 1)
            {
                cur_p->period_cnt = 0;
                s_ana_rtc.alarm_head_p = cur_p->next; // head move to next
                s_ana_rtc.alarm_node_cnt--;
/*
 * WARNING:As freertos doesn't support free memory in ISR context.
 * The chip no needs to use a task for AON RTC which wastes some memory.
 * so the APPLIACTION who calls bk_alarm_register should release the memory
 * returns by bk_alarm_register.
 */
                ana_rtc_release_node(cur_p);
            }
            // loop timer
            else
            {
                if (cur_p->period_cnt != ALARM_LOOP_FOREVER)
                {
                    cur_p->period_cnt--;
                    ANA_RTC_LOGD("%s left %d times \r\n", cur_p->name, cur_p->period_cnt);
                }

                // has next
                if (next_p) // move to switable position
                {
                    s_ana_rtc.alarm_head_p = cur_p->next; // head move to next
                    cur_p->expired_tick += cur_p->period_tick;
                    cur_p->next = NULL;            // cur_p is removed
                    s_ana_rtc.alarm_node_cnt--; // it will ++ in alarm_insert_node
                    if (alarm_insert_node(id, cur_p) != 0)
                    {
                        ANA_RTC_LOGE("alarm name=%s insert fail\r\n", cur_p->name);
                        rtos_enable_int(int_level);
                        return;
                    }
                }
                else // only self
                {
                    // just update self expired time
                    cur_p->expired_tick += cur_p->period_tick;
                    ANA_RTC_LOGD("%s update next expired time %d \r\n", cur_p->name, cur_p->expired_tick);
                }
            }
        }
        else // no expired
        {
            break;
        }

        cur_p = next_p; // TODO:maybe cur_p offset is too small and calback excutes too much time, here can't switch to next NODE.

        alarm_dump_list(s_ana_rtc.alarm_head_p);
    }

    rtos_enable_int(int_level);

    alarm_dump_list(s_ana_rtc.alarm_head_p);

    ANA_RTC_LOGD("%s[-]cnt=%d\r\n", __func__, s_ana_rtc.alarm_node_cnt);
}

bk_err_t bk_aon_rtc_register_tick_isr(aon_rtc_id_t id, aon_rtc_isr_t isr, void *param)
{
    // AON_RTC_RETURN_ON_INVALID_ID(id);
    uint32_t int_level = rtos_disable_int();
    s_ana_rtc_tick_isr.callback = isr;
    s_ana_rtc_tick_isr.isr_param = param;
    rtos_enable_int(int_level);
    return BK_OK;
}

/*
 * aon rtc set tick uses 3 cycles of 32k in ASIC,
 * cpu should check whether set tick success.
 * If twice set tick in 3/32 ms, the second time set tick value will be failed.
 */
static void ana_rtc_set_tick(uint64_t val)
{
    uint64_t start_tick = 0, cur_tick = 0;
#if CONFIG_ANA_RTC_WAKUP_BY_RTC
    if (val == AON_RTC_ROUND_TICK + 1)
    {
        sys_hal_set_ana_reg11_rtcsel((uint32_t)(AON_RTC_ROUND_TICK >> 32));
        sys_hal_set_ana_reg13_rtcsel((uint32_t)AON_RTC_ROUND_TICK - 1);
        return;
    }

    val -= s_ana_rtc_base_tick; // convert rtc tick

    uint32_t val_h = val >> 32, val_l = (uint32_t)val;

    sys_hal_set_ana_reg11_rtcsel(val_h);
    sys_hal_set_ana_reg13_rtcsel(val_l);

    ANA_RTC_LOGD("set tick: %d %d %d %d\r\n", (uint32_t)(s_ana_rtc_base_tick>>32), (uint32_t)s_ana_rtc_base_tick, val_h, val_l);

    start_tick = ANA_RTC_CURRENT_TICK;
    while ((sys_hal_get_ana_reg13_rtcsel() != val_l) || (sys_hal_get_ana_reg11_rtcsel() != val_h))
    {
        cur_tick = ANA_RTC_CURRENT_TICK;
        if (cur_tick - start_tick > ANA_RTC_OPS_SAFE_DELAY_US)
        {
            ANA_RTC_LOGE("%s:set tick timeout,set_tick=0x%llx, lpo_tick=0x%x %x\r\n", __func__, val,
                         ((uint64_t)sys_hal_get_ana_reg13_rtcsel() << 32) + sys_hal_get_ana_reg11_rtcsel());
            break;
        }
        sys_hal_set_ana_reg11_rtcsel(val_h);
        sys_hal_set_ana_reg13_rtcsel(val_l);
    }
#else
    if (val == AON_RTC_ROUND_TICK + 1)
    {
        sys_hal_set_ana_reg11_timersel((uint32_t)(AON_RTC_ROUND_TICK >> 32));
        sys_hal_set_ana_reg12_timersel((uint32_t)AON_RTC_ROUND_TICK - 1);
        return;
    }

    val -= s_ana_rtc_base_tick; // convert rtc tick

    uint32_t val_h = val >> 32, val_l = (uint32_t)val;

    sys_hal_set_ana_reg11_timersel(val_h);
    sys_hal_set_ana_reg12_timersel(val_l);

    ANA_RTC_LOGD("set tick: %d %d %d %d\r\n", (uint32_t)(s_ana_rtc_base_tick>>32), (uint32_t)s_ana_rtc_base_tick, val_h, val_l);

    start_tick = ANA_RTC_CURRENT_TICK;
    while ((sys_hal_get_ana_reg12_timersel() != val_l) || (sys_hal_get_ana_reg11_timersel() != val_h))
    {
        cur_tick = ANA_RTC_CURRENT_TICK;
        if (cur_tick - start_tick > ANA_RTC_OPS_SAFE_DELAY_US)
        {
            ANA_RTC_LOGE("%s:set tick timeout,set_tick=0x%llx, lpo_tick=0x%x %x\r\n", __func__, val,
                         ((uint64_t)sys_hal_get_ana_reg12_timersel() << 32) + sys_hal_get_ana_reg11_timersel());
            break;
        }
        sys_hal_set_ana_reg11_timersel(val_h);
        sys_hal_set_ana_reg12_timersel(val_l);
    }
#endif
    sys_hal_enable_ana_rtc_int();
    #if !CONFIG_ANA_RTC_WAKUP_BY_RTC
    s_rtc_updated = false;
    #endif
}

#if CONFIG_ANA_RTC_DEBUG
#define ANA_RTC_ISR_DEBUG_MAX_CNT (256)
static uint32_t s_isr_cnt = 0;
static uint64_t s_isr_debug_in_tick[ANA_RTC_ISR_DEBUG_MAX_CNT];
static uint64_t s_isr_debug_out_tick[ANA_RTC_ISR_DEBUG_MAX_CNT];
static uint64_t s_isr_debug_set_tick[ANA_RTC_ISR_DEBUG_MAX_CNT];
#endif

static bk_err_t ana_rtc_isr_handler(void *args)
{
    aon_rtc_id_t id = ANA_RTC_GET_SET_TIME_RTC_ID;
    uint32_t int_level = rtos_disable_int();

#if CONFIG_ANA_RTC_DEBUG
    s_isr_debug_in_tick[(s_isr_cnt) % ANA_RTC_ISR_DEBUG_MAX_CNT] = bk_aon_rtc_get_current_tick(id);
#endif

    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    sys_hal_disable_ana_rtc_int();
    alarm_update_expeired_nodes(id);

    // reset the timer tick
    if (s_ana_rtc.alarm_head_p)
    {
        //+1:to assume set it valid,maybe aon rtc add 1 tick when set the value now.
        // BK_ASSERT(bk_aon_rtc_get_current_tick(id) + 1/*AON_RTC_PRECISION_TICK*/ < s_ana_rtc.alarm_head_p->expired_tick);    //4:reserve enough time to set the tick
        if ((bk_aon_rtc_get_current_tick(id) + 1 > s_ana_rtc.alarm_head_p->expired_tick))
        {
            ANA_RTC_LOGE("next expired tick is invalid\r\n");
            rtos_enable_int(int_level);
            return BK_FAIL;
        }
        ana_rtc_set_tick(s_ana_rtc.alarm_head_p->expired_tick);
#if CONFIG_AON_RTC_DEBUG
        s_isr_debug_set_tick[(s_isr_cnt) % AON_RTC_ISR_DEBUG_MAX_CNT] = s_ana_rtc.alarm_head_p->expired_tick;
#endif
        ANA_RTC_LOGD("next tick=0x%x, cur_tick=0x%x\r\n", (uint32_t)s_ana_rtc.alarm_head_p->expired_tick, (uint32_t)bk_aon_rtc_get_current_tick(id));
    }
    else
    {
        ana_rtc_set_tick(AON_RTC_ROUND_TICK + 1);
        ANA_RTC_LOGD("no alarm:cur_tick=0x%x\r\n", (uint32_t)bk_aon_rtc_get_current_tick(id));
    }

    ANA_RTC_LOGD("%s[-]\r\n", __func__);
#if CONFIG_ANA_RTC_DEBUG
    s_isr_debug_out_tick[(s_isr_cnt) % ANA_RTC_ISR_DEBUG_MAX_CNT] = bk_aon_rtc_get_current_tick(id);
    s_isr_cnt++;
#endif
    rtos_enable_int(int_level);

    return BK_OK;
}

__IRAM_SEC static void ana_rtc1_isr_handler(void)
{
    ana_rtc_isr_handler(AON_RTC_ID_1);
}

__IRAM_SEC bk_err_t ana_rtc_wakeup_config(void *args)
{
    // protect sleep time check
    uint64_t current_tick;
    uint64_t expired_tick = AON_RTC_ROUND_TICK;
    int64_t sleep_tick;

#if CONFIG_ANA_RTC_WAKUP_BY_RTC
    expired_tick = sys_hal_get_ana_reg13_rtcsel() + ((uint64_t)sys_hal_get_ana_reg11_rtcsel() << 32);
    current_tick = aon_pmu_hal_rtc_tick_l_get() + ((uint64_t)aon_pmu_hal_rtc_tick_h_get() << 32);
#else
    if (s_ana_rtc.alarm_head_p)
        expired_tick = s_ana_rtc.alarm_head_p->expired_tick;
    current_tick = ANA_RTC_CURRENT_TICK;
#endif

    sleep_tick = (int64_t)expired_tick - (int64_t)current_tick;

    if (sleep_tick < AON_RTC_MS_TICK_CNT) // close to 1ms
    {
        return BK_FAIL;
    }

    return BK_OK;
}

__IRAM_SEC bk_err_t ana_rtc_deep_ps_wakeup_check(uint64_t *p_expired_tick)
{
    alarm_node_t *deep_ps_node = NULL, *node = NULL;

    if (p_expired_tick == NULL)
    {
        return BK_ERR_PARAM;
    }

    uint32_t int_level = rtos_disable_int();

    node = s_ana_rtc.alarm_head_p;
    while (node)
    {
        // deep sleep rtc node name is "deep_ps"
        if (strncmp((const char *)node->name, (const char *)"deep_ps", ALARM_NAME_MAX_LEN) == 0)
        {
            deep_ps_node = node;
            break;
        }
        node = node->next;
    }

    if(deep_ps_node)
    {
        uint64_t current_tick;
        uint64_t expired_tick;

        expired_tick = deep_ps_node->expired_tick;
        current_tick = ANA_RTC_CURRENT_TICK;

        int64_t sleep_tick = (int64_t)expired_tick - (int64_t)current_tick;
        if (sleep_tick < AON_RTC_MS_TICK_CNT) // close to 1ms
        {
            ANA_RTC_LOGE("deep_ps rtc sleep time is too short\r\n");
            rtos_enable_int(int_level);
            return BK_FAIL;
        }
        // force set deep ps rtc tick, ignore other alarm nodes in the alarm list
        //ana_rtc_set_tick(expired_tick);
        *p_expired_tick = deep_ps_node->expired_tick;
        //ANA_RTC_LOGD("deep_ps rtc set tick:0x%x\r\n", (uint32_t)deep_ps_node->expired_tick);
    }
    else
    {
        rtos_enable_int(int_level);

        /* No "deep_ps" node: do not change RTC compare; caller skips apply when *p_expired_tick==0. */
        ANA_RTC_LOGW("deep_ps alarm not found, keep HW RTC compare as-is\r\n");
        return ana_rtc_wakeup_config(NULL);
    }
    rtos_enable_int(int_level);
    return BK_OK;
}

__IRAM_SEC bk_err_t ana_rtc_deep_ps_wakeup_apply(uint64_t expired_tick)
{
    uint32_t int_level = rtos_disable_int();
    uint64_t current_tick = ANA_RTC_CURRENT_TICK;

    int64_t sleep_tick = (int64_t)expired_tick - (int64_t)current_tick;
    if (sleep_tick < AON_RTC_MS_TICK_CNT) // close to 1ms
    {
        ANA_RTC_LOGE("deep_ps rtc sleep time is too short\r\n");
        rtos_enable_int(int_level);
        return BK_FAIL;
    }

    ana_rtc_set_tick(expired_tick);
    ANA_RTC_LOGD("deep_ps rtc set tick:0x%x\r\n", (uint32_t)expired_tick);
    rtos_enable_int(int_level);
    return BK_OK;
}

static bk_err_t ana_rtc_sw_init(void)
{
    os_memset(&s_ana_rtc, 0, sizeof(s_ana_rtc));
    os_memset(&s_ana_rtc_tick_isr, 0, sizeof(s_ana_rtc_tick_isr));

    s_high_tick = 0;
    s_low_tick = 0;

    s_ana_rtc_nodes_p = os_zalloc(sizeof(ana_rtc_nodes_memory_t));
    if (s_ana_rtc_nodes_p == NULL)
    {
        return BK_ERR_NO_MEM;
    }

#if 0
    wakeup_cb_conf_t wkup_cfg = {
        .cb = (wakeup_cb)ana_rtc_isr_handler,
        .args = ANA_RTC_GET_SET_TIME_RTC_ID,
    };
    wakeup_cb_conf_t src_cfg = {
        .cb = (wakeup_cb)ana_rtc_wakeup_config,
        .args = ANA_RTC_GET_SET_TIME_RTC_ID,
    };
    bk_wakeup_register_cb(WAKEUP_SOURCE_INT_RTC, wkup_cfg, src_cfg);
#endif
#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
	aon_rtc_register_deepsleep_cb();
#endif

    return BK_OK;
}

void ana_rtc_update(void)
{
    #if !CONFIG_ANA_RTC_WAKUP_BY_RTC
    if (!s_rtc_updated)
    {
        uint32_t high_tick = aon_pmu_hal_rtc_tick_h_get();
        // indicate rtc_value reset by ana_timer
        if (high_tick == 0)
        {
            if (s_ana_rtc.alarm_head_p)
                s_ana_rtc_base_tick = s_ana_rtc.alarm_head_p->expired_tick;
            s_rtc_updated = true;
            ana_rtc_set_tick(AON_RTC_ROUND_TICK + 1);
            return;
        }
    }
    // indicate rtc_value > 2^32,overflow happened
    s_high_tick++;
    #endif
}

static void ana_rtc_hw_init(void)
{
    uint32_t tick_h = (uint32_t)(AON_RTC_ROUND_TICK >> 32);
    uint32_t tick_l = (uint32_t)AON_RTC_ROUND_TICK;

    // rebase tick value
#if !CONFIG_AON_RTC_KEEP_TIME_SUPPORT
    s_ana_rtc_base_tick = aon_pmu_hal_rtc_tick_l_get();
#endif
    s_low_tick = 0;

    sys_hal_set_ana_reg8_spi_latch1v(1);
#if !CONFIG_AON_RTC_KEEP_TIME_SUPPORT
    sys_hal_set_ana_reg9_spi_timerwken(0);
#endif
    sys_hal_set_ana_reg5_pwd_rosc_spi(0);
    sys_hal_set_ana_reg5_rosc_disable(0);
    sys_hal_set_ana_reg9_spi_byp32pwd(1);

#ifdef CONFIG_EXTERN_32K
    sys_hal_set_ana_reg7_clk_sel(1);
#else
    sys_hal_set_ana_reg7_clk_sel(0);
#endif

    // set upper tick value
    sys_hal_set_ana_reg11_timersel(tick_h);
    sys_hal_set_ana_reg12_timersel(tick_l);

    // set wakeup tick value to make sure wakeup success
    sys_hal_set_ana_reg11_rtcsel(tick_h);
    sys_hal_set_ana_reg13_rtcsel(tick_l - 1);

    // rtc value sample select neg edge
    aon_pmu_hal_rtc_samp_sel(ANA_RTC_SAMPLE_NEGATIVE_EDGE);

    // rtc tick start
#if !CONFIG_AON_RTC_KEEP_TIME_SUPPORT
    sys_hal_set_ana_reg9_spi_timerwken(1);
#endif
    sys_hal_set_ana_reg8_spi_latch1v(0);

    // wait until rtc tick reset ok, will cost about 32us
#if !CONFIG_AON_RTC_KEEP_TIME_SUPPORT
    while (1)
    {
        if (aon_pmu_hal_rtc_tick_l_get() < s_ana_rtc_base_tick)
        {
            break;
        }
    }
#endif

    bk_int_isr_register(INT_SRC_ANA_RTC, ana_rtc1_isr_handler, NULL);

    ANA_RTC_LOGD("%s[-]cur_tick=%d\r\n", __func__, (uint32_t)ANA_RTC_CURRENT_TICK);
}

bk_err_t bk_aon_rtc_driver_init(void)
{
    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    // TOTO: Enter critical protect
    if (!s_ana_rtc.inited)
    {
        ana_rtc_sw_init();
        ana_rtc_hw_init();
        #if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
        aon_rtc_compute_boot_timeofday();
        #endif
        s_ana_rtc.inited = true;
    }

    // TOTO: exit critical protect
    ANA_RTC_LOGD("%s[-]\r\n", __func__);

#if CONFIG_AON_RTC_TEST
    int bk_ano_rtc_register_cli_test_feature(void);
    bk_ano_rtc_register_cli_test_feature();
#endif

    return BK_OK;
}

bk_err_t bk_aon_rtc_driver_deinit(void)
{
    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    if (s_ana_rtc.inited)
    {
        sys_hal_set_ana_reg5_pwd_rosc_spi(1);
        sys_hal_set_ana_reg5_rosc_disable(1);
        sys_hal_set_ana_reg9_spi_byp32pwd(1);

        bk_int_isr_unregister(INT_SRC_ANA_RTC);
        if (s_ana_rtc_nodes_p != NULL)
        {
            os_free(s_ana_rtc_nodes_p);
            s_ana_rtc_nodes_p = NULL;
        }

        s_ana_rtc.inited = false;
    }

    ANA_RTC_LOGD("%s[-]\r\n", __func__);
    return BK_OK;
}

bk_err_t bk_alarm_register(aon_rtc_id_t id, alarm_info_t *alarm_info_p)
{
    alarm_node_t *node_p = NULL;
    uint32_t int_level = 0;

    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    if (id >= AON_RTC_ID_MAX)
    {
        ANA_RTC_LOGE("%s:id=%d\r\n", __func__, id);
        return BK_ERR_PARAM;
    }

    if (alarm_info_p == NULL)
    {
        return BK_ERR_PARAM;
    }

    if (alarm_info_p->period_tick < AON_RTC_PRECISION_TICK) // in protect area to reduce consume time before set tick.
    {
        ANA_RTC_LOGD("period_tick should not smaller then %d\r\n", AON_RTC_PRECISION_TICK);
        return BK_FAIL;
    }

    int_level = rtos_disable_int();

    if (s_ana_rtc.alarm_node_cnt >= AON_RTC_MAX_ALARM_CNT)
    {
        rtos_enable_int(int_level);
        ANA_RTC_LOGE("alarm registered too much:%d\r\n", AON_RTC_MAX_ALARM_CNT);
        return BK_FAIL;
    }

    // request a node
    node_p = ana_rtc_request_node(id);
    if (node_p == NULL)
    {
        rtos_enable_int(int_level);
        ANA_RTC_LOGE("alarm registered:no memory\r\n");
        return BK_ERR_NO_MEM;
    }

    memset(node_p, 0, sizeof(alarm_node_t));
    node_p->callback = alarm_info_p->callback;
    node_p->cb_param_p = alarm_info_p->param_p;
    memcpy(&node_p->name[0], alarm_info_p->name, ALARM_NAME_MAX_LEN);
    node_p->name[ALARM_NAME_MAX_LEN] = 0;
    node_p->start_tick = bk_aon_rtc_get_current_tick(id); // tick
    node_p->period_tick = alarm_info_p->period_tick;
    // BK_ASSERT(alarm_info_p->period_cnt);
    if (alarm_info_p->period_cnt == 0)
    {
        rtos_enable_int(int_level);
        ANA_RTC_LOGE("no set period cnt\r\n");
        return BK_ERR_PARAM;
    }

    node_p->period_cnt = alarm_info_p->period_cnt;
    node_p->expired_tick = node_p->start_tick + (alarm_info_p->period_tick);

    // push to alarm list
    if (alarm_insert_node(id, node_p) != 0)
    {
        ANA_RTC_LOGE("alarm name=%s has registered\r\n", alarm_info_p->name);
        ana_rtc_release_node(node_p);
        rtos_enable_int(int_level);
        return BK_FAIL;
    }

    // reset the timer tick
    if (node_p == s_ana_rtc.alarm_head_p) // insert node is the first one, should reset tick val
    {
        //+1:to assume set it valid,maybe aon rtc add 1 tick when set the value now.
        // BK_ASSERT(bk_aon_rtc_get_current_tick(id) + 1/*AON_RTC_PRECISION_TICK*/ < s_ana_rtc.alarm_head_p->expired_tick);    //4:reserve enough time to set the tick
        if ((bk_aon_rtc_get_current_tick(id) + 1 > s_ana_rtc.alarm_head_p->expired_tick))
        {
            rtos_enable_int(int_level);
            ANA_RTC_LOGE("next expired tick is invalid\r\n");
            return BK_FAIL;
        }

        ana_rtc_set_tick(s_ana_rtc.alarm_head_p->expired_tick);
    }

    ANA_RTC_LOGD("next tick=0x%x, cur_tick=0x%x\r\n", (uint32_t)s_ana_rtc.alarm_head_p->expired_tick, (uint32_t)bk_aon_rtc_get_current_tick(id));

    rtos_enable_int(int_level);

    ANA_RTC_LOGD("%s[-]\r\n", __func__);

    return BK_OK;
}

// the timer isn't expired, but app un-register it.
bk_err_t bk_alarm_unregister(aon_rtc_id_t id, uint8_t *name_p)
{
    alarm_node_t *remove_node_p = NULL;
    alarm_node_t *previous_head_node_p = NULL;
    uint32_t int_level = 0;

    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    if (id >= AON_RTC_ID_MAX)
    {
        ANA_RTC_LOGE("%s:id=%d\r\n", __func__, id);
        return BK_ERR_PARAM;
    }

    int_level = rtos_disable_int();
    previous_head_node_p = s_ana_rtc.alarm_head_p;
    remove_node_p = alarm_remove_node(id, name_p);

    // reset the timer tick
    if (previous_head_node_p == remove_node_p) // the previous head is removed
    {
        if (s_ana_rtc.alarm_head_p) // new head exist
        {
            //+1:to assume set it valid,maybe aon rtc add 1 tick when set the value now.
            // BK_ASSERT(bk_aon_rtc_get_current_tick(id) + 1/*AON_RTC_PRECISION_TICK*/ < s_ana_rtc.alarm_head_p->expired_tick);    //reserve enough time to set the tick
            if ((bk_aon_rtc_get_current_tick(id) + 1 > s_ana_rtc.alarm_head_p->expired_tick))
            {
                rtos_enable_int(int_level);
                ANA_RTC_LOGE("next expired tick is invalid\r\n");
                return BK_FAIL;
            }

            ana_rtc_set_tick(s_ana_rtc.alarm_head_p->expired_tick);
            ANA_RTC_LOGD("next tick=0x%x, cur_tick=0x%x\r\n", s_ana_rtc.alarm_head_p->expired_tick, bk_aon_rtc_get_current_tick(id));
        }
        else // has no nodes now
        {
            // aon_rtc_hal_disable_tick_int(&s_ana_rtc.hal);        //If the ISR at enable status, and the previous set tick time come, it will produce an Interrupt and maybe wakeup system.
            ana_rtc_set_tick(AON_RTC_ROUND_TICK + 1);
            ANA_RTC_LOGD("no alarm:cur_tick=0x%x\r\n", bk_aon_rtc_get_current_tick(id));
        }
    }

    rtos_enable_int(int_level);

    ANA_RTC_LOGD("%s[-]\r\n", __func__);
    return BK_OK;
}

__IRAM_SEC uint64_t bk_aon_rtc_get_current_tick(aon_rtc_id_t id)
{
    uint32_t low_tick, int_level = 0;

    if (id >= AON_RTC_ID_MAX)
    {
        ANA_RTC_LOGE("%s:id=%d\r\n", __func__, id);
        return 0;
    }

    /*analog rtc sample edge should be negative edge*/
    BK_ASSERT(ANA_RTC_SAMPLE_NEGATIVE_EDGE == aon_pmu_hal_rtc_get_samp_sel());
    int_level = rtos_disable_int();

    low_tick = aon_pmu_hal_rtc_tick_l_get();
#if CONFIG_ANA_RTC_SUPPORT_64BIT
    if (low_tick < s_low_tick)
    {
        ANA_RTC_LOGD("rtc carrying:%x %x %x\n", s_low_tick, low_tick, aon_pmu_hal_rtc_tick_l_get());
        #if !CONFIG_ANA_RTC_WAKUP_BY_RTC
        ana_rtc_update();
        #else
        s_high_tick++;
        #endif
    }
#else
    s_high_tick = aon_pmu_hal_rtc_tick_h_get();
#endif
    s_low_tick = low_tick;
    rtos_enable_int(int_level);
    return ((uint64_t)s_high_tick << 32) + s_low_tick + s_ana_rtc_base_tick;
}

void bk_aon_rtc_dump(aon_rtc_id_t id)
{
#if CONFIG_ANA_RTC_DEBUG
    uint32_t i = 0, index = 0;

    for (i = s_isr_cnt - ANA_RTC_ISR_DEBUG_MAX_CNT; i < s_isr_cnt; i++)
    {
        index = i % ANA_RTC_ISR_DEBUG_MAX_CNT;

        for (volatile uint32_t j = 0; j < 1800; j++)
            ; // confirm log output normarlly

        ANA_RTC_LOGI("isr_in[%d]=0x%llx,out=0x%llx,set=0x%llx\r\n", index, s_isr_debug_in_tick[index], s_isr_debug_out_tick[index], s_isr_debug_set_tick[index]);
    }
#endif
    alarm_dump_list(s_ana_rtc.alarm_head_p);
}

#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
#if CONFIG_EASY_FLASH
#include "bk_ef.h"
#endif
static uint64_t s_rtc_keep_time_offset_us = 0;

/* Serialize uint64_t to hex for env storage; full 64-bit, fixed width for stable parse */
static int ana_rtc_u64_to_hex_buf(char *buf, size_t buflen, uint64_t v)
{
    return snprintf(buf, buflen, "%016llx", (unsigned long long)v);
}

static bool bk_rtc_get_deepsleep_reset_reason_flag(void)
{
    uint32_t reset_reason = bk_misc_get_reset_reason();
    ANA_RTC_LOGD("reset_reason: %d\r\n", reset_reason);
    /* Deep sleep exit reasons; SUPER_DEEP used on some SoC paths even for RTC wake */
    if (reset_reason == RESET_SOURCE_DEEPPS_GPIO ||
        reset_reason == RESET_SOURCE_DEEPPS_RTC ||
        reset_reason == RESET_SOURCE_DEEPPS_TOUCH ||
        reset_reason == RESET_SOURCE_DEEPPS_USB ||
        reset_reason == RESET_SOURCE_SUPER_DEEP)
    {
        return true;
    }
    return false;
}

static bk_err_t aon_rtc_bake_boot_time_us(void)
{
#if CONFIG_EASY_FLASH
    bool boot_time_us_need_save = true;
    char *boot_time_us_value = bk_get_env("s_boot_time_us");

    if (boot_time_us_value)
    {
        uint64_t temp = strtoull(boot_time_us_value, NULL, 16);

        ANA_RTC_LOGD("%s temp s_boot_time_us 0x%llx\r\n", __func__, (unsigned long long)temp);
        if (temp == s_boot_time_us)
        {
            boot_time_us_need_save = false;
        }
    }
    if (boot_time_us_need_save)
    {
        char value[32];
        os_memset(value, 0, sizeof(value));
        int nw = ana_rtc_u64_to_hex_buf(value, sizeof(value), s_boot_time_us);

        if (nw <= 0 || (size_t)nw >= sizeof(value))
        {
            ANA_RTC_LOGE("%s: fmt s_boot_time_us fail\r\n", __func__);
            return BK_FAIL;
        }
        {
            EfErrCode er = bk_set_env("s_boot_time_us", value);

            ANA_RTC_LOGI("%s s_boot_time_us value:%s ret:%d\n", __func__, value, (int)er);
            if (er != EF_NO_ERR)
            {
                return BK_FAIL;
            }
        }
    }

    if (bk_save_env() != EF_NO_ERR)
    {
        ANA_RTC_LOGE("%s: bk_save_env fail\r\n", __func__);
        return BK_FAIL;
    }
    return BK_OK;
#else
    ANA_RTC_LOGW("%s: CONFIG_EASY_FLASH off, time not persisted\r\n", __func__);
    return BK_FAIL;
#endif
}

static bk_err_t aon_rtc_bake_entry_deepsleep_timeofday(void)
{
    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    uint64_t current_time_us = bk_aon_rtc_get_us();
    ANA_RTC_LOGD("current_time_us=0x%llx\r\n", current_time_us);

#if CONFIG_EASY_FLASH
    char value[32];
    os_memset(value, 0, sizeof(value));
    {
        int nw = ana_rtc_u64_to_hex_buf(value, sizeof(value), current_time_us);

        if (nw <= 0 || (size_t)nw >= sizeof(value))
        {
            ANA_RTC_LOGE("%s: fmt rtc_ds_start_us fail\r\n", __func__);
            return BK_FAIL;
        }
    }
    {
        EfErrCode er = bk_set_env("rtc_ds_start_us", value);

        ANA_RTC_LOGI("%s rtc_ds_start_us value:%s ret:%d\n", __func__, value, (int)er);
        if (er != EF_NO_ERR)
        {
            return BK_FAIL;
        }
    }

    if (bk_save_env() != EF_NO_ERR)
    {
        ANA_RTC_LOGE("%s: bk_save_env fail\r\n", __func__);
        return BK_FAIL;
    }

    // not save boot time, as boot time aready saved once call bk_rtc_settimeofday

#else
    ANA_RTC_LOGW("%s: CONFIG_EASY_FLASH off, time not persisted\r\n", __func__);
    return BK_FAIL;
#endif
    return BK_OK;
}

static bk_err_t aon_rtc_get_deepsleep_start_time(uint64_t *deepsleep_start_time_us)
{
    ANA_RTC_LOGD("%s[+]\r\n", __func__);

#if CONFIG_EASY_FLASH
    char *value;
    if(deepsleep_start_time_us)
    {
        value = bk_get_env("rtc_ds_start_us");
        if(value)
        {
            *deepsleep_start_time_us = strtoull(value, NULL, 16);
            ANA_RTC_LOGD("%s, value:%s\r\n", "rtc_ds_start_us", value);
            return BK_OK;
        }
        else
        {
            ANA_RTC_LOGD("%s value is null\r\n", "rtc_ds_start_us");
        }
    }
#else
    ANA_RTC_LOGW("TODO:%s\n", __func__);
#endif
    return BK_FAIL;
}

static bk_err_t aon_rtc_get_boot_time_us(uint64_t *boot_time_us)
{
    ANA_RTC_LOGD("%s[+]\r\n", __func__);

#if CONFIG_EASY_FLASH
    char *value;
    if(boot_time_us)
    {
        value = bk_get_env("s_boot_time_us");
        if(value)
        {
            *boot_time_us = strtoull(value, NULL, 16);
            ANA_RTC_LOGD("%s, value:%s\r\n","s_boot_time_us", value);
            return BK_OK;
        }
        else
        {
            ANA_RTC_LOGD("%s s_boot_time_us value is null\r\n", __func__);
        }
    }
#else
    ANA_RTC_LOGW("TODO:%s\n", __func__);
#endif
    return BK_FAIL;
}

static void aon_rtc_compute_boot_timeofday(void)
{
    bk_err_t ret = BK_OK;
    ANA_RTC_LOGD("%s[+]\r\n", __func__);

    //deepsleep boot
    if(bk_rtc_get_deepsleep_reset_reason_flag())
    {
        uint64_t current_time_us = bk_aon_rtc_get_us();
        uint64_t deepsleep_start_time_us = 0;

        ret = aon_rtc_get_deepsleep_start_time(&deepsleep_start_time_us);
        if(ret != BK_OK)
        {
            deepsleep_start_time_us = 0;
        }
        ANA_RTC_LOGD("deepsleep_start_time_us=0x%llx\r\n", deepsleep_start_time_us);
        ANA_RTC_LOGD("current_time_us=0x%llx\r\n", current_time_us);
        if(current_time_us < deepsleep_start_time_us)
        {
            s_rtc_keep_time_offset_us = (uint64_t)(0) - deepsleep_start_time_us + current_time_us;
        }
        else
        {
            s_rtc_keep_time_offset_us = current_time_us - deepsleep_start_time_us;
        }
        ANA_RTC_LOGI("s_rtc_keep_time_offset_us=0x%llx\r\n", s_rtc_keep_time_offset_us);

        uint64_t boot_time_us = 0;
        ret = aon_rtc_get_boot_time_us(&boot_time_us);
        if(ret == BK_OK)
        {
            s_boot_time_us = boot_time_us;
            ANA_RTC_LOGI("s_boot_time_us=0x%llx\r\n", s_boot_time_us);
        }
        ANA_RTC_LOGD("deepsleep boot\r\n");
    }
    else
    {
        // other case, check the boot time from flash
        // for power on, there has two cases:
        // 1. power off long time, rtc was reset. load s_boot_time_us has no sense. still load as it no effect.
        // 2. power off short time, rtc was not reset. s_boot_time_us is valid.
        uint64_t boot_time_us = 0;
        ret = aon_rtc_get_boot_time_us(&boot_time_us);
        if(ret == BK_OK)
        {
            s_boot_time_us = boot_time_us;
            ANA_RTC_LOGI("s_boot_time_us=0x%llx\r\n", s_boot_time_us);
        }
        s_rtc_keep_time_offset_us = 0;
        ANA_RTC_LOGD("hot reboot\r\n");
    }
}

static void bk_rtc_keep_time_enter_deepsleep(void)
{
    ANA_RTC_LOGD("[+]%s\r\n", __func__);

    if (aon_rtc_bake_entry_deepsleep_timeofday() != BK_OK)
    {
        ANA_RTC_LOGE("%s: bake time to flash failed\r\n", __func__);
    }

    ANA_RTC_LOGD("[-]%s\r\n", __func__);
}

static void bk_rtc_keep_time_exit_deepsleep(void)
{
    ANA_RTC_LOGD("%s\r\n", __func__);
}

static void aon_rtc_register_deepsleep_cb(void)
{
    pm_cb_conf_t aon_rtc_enter_config = {
            .cb = (pm_cb)bk_rtc_keep_time_enter_deepsleep,
            .args = NULL
            };
    pm_cb_conf_t aon_rtc_exit_config = {
            .cb = (pm_cb)bk_rtc_keep_time_exit_deepsleep,
            .args = NULL
            };
    bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP, PM_DEV_ID_RTC, &aon_rtc_enter_config, &aon_rtc_exit_config);
}
#endif //CONFIG_AON_RTC_KEEP_TIME_SUPPORT

bk_err_t bk_rtc_get_deepsleep_duration_seconds(uint32_t *deepsleep_seconds)
{
#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
    uint64_t sec64;

    if (deepsleep_seconds == NULL)
    {
        return BK_ERR_NULL_PARAM;
    }
    sec64 = s_rtc_keep_time_offset_us / 1000000ULL;
    if (sec64 > (uint64_t)UINT32_MAX)
    {
        return BK_FAIL;
    }
    *deepsleep_seconds = (uint32_t)sec64;
    return BK_OK;
#else
    (void)deepsleep_seconds;
#endif
    return BK_FAIL;
}

bk_err_t bk_rtc_gettimeofday(struct timeval *tv, void *ptz)
{
    (void)ptz;

    if (tv != NULL)
    {
        uint64_t uCurTimeUs = s_boot_time_us + bk_aon_rtc_get_us();

        tv->tv_sec = uCurTimeUs / 1000000;
        tv->tv_usec = uCurTimeUs % 1000000;
        ANA_RTC_LOGD("s_boot_time_us:h=0x%x,l=0x%x \r\n", s_boot_time_us >> 32, (uint32_t)s_boot_time_us);
        ANA_RTC_LOGD("%s sec=%d,us:%d\n", __func__, tv->tv_sec, tv->tv_usec);
    }
    else
        ANA_RTC_LOGW("%s tv is null \r\n", __func__);

    return BK_OK;
}

bk_err_t bk_rtc_settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    (void)tz;
    if (tv)
    {
        uint64_t setTimeUs = ((uint64_t)tv->tv_sec) * 1000000LL + tv->tv_usec;
        uint64_t getCurTimeUs = bk_aon_rtc_get_us();

        s_boot_time_us = setTimeUs - getCurTimeUs;

#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
        aon_rtc_bake_boot_time_us();
#endif

        ANA_RTC_LOGD("%s:sec=%d us=%d\n", __func__, tv->tv_sec, tv->tv_usec);
        ANA_RTC_LOGD("get us:h=0x%x,l=0x%x\n", getCurTimeUs >> 32, (uint32_t)getCurTimeUs);
    }

    return BK_OK;
}
