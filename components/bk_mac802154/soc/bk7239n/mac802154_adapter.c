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

#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <soc/bk7239n/int_types.h>
#include <driver/hal/hal_int_types.h>
#include <driver/int.h>
#include <modules/pm.h>
#include <components/system.h>
#include "sys_driver.h"
#include "mac802154_adapter.h"
#include "bk_rf_internal.h"

static bk_err_t init_queue_wrapper(void **queue, const char *name, uint32_t message_size, uint32_t number_of_messages)
{
    return rtos_init_queue(queue, name, message_size, number_of_messages);
}

static bk_err_t deinit_queue_wrapper(void **queue)
{
    return rtos_deinit_queue(queue);
}

static bk_err_t pop_from_queue_wrapper(void **queue, void *message, uint32_t timeout_ms)
{
    return rtos_pop_from_queue(queue, message, timeout_ms);
}

static bk_err_t push_to_queue_wrapper(void **queue, void *message, uint32_t timeout_ms)
{
    return rtos_push_to_queue(queue, message, timeout_ms);
}

static bk_err_t push_to_queue_front_wrapper(beken_queue_t *queue, void *message, uint32_t timeout_ms)
{
    return rtos_push_to_queue_front(queue, message, timeout_ms);
}

static bool is_queue_empty_wrapper(void **queue)
{
    return rtos_is_queue_empty(queue);
}

static bk_err_t create_thread_wrapper(void **thread, uint8_t priority, const char *name,
                                      void *function, uint32_t stack_size, void *arg)
{
    return rtos_create_thread(thread, priority, name, function, stack_size, arg);
}

static bk_err_t delete_thread_wrapper(void **thread)
{
    return rtos_delete_thread(thread);
}

static int thread_join(void **thread)
{
    return rtos_thread_join(thread);
}

static void *current_thread_wrapper(void)
{
    return rtos_get_current_thread();
}

static int32_t init_mutex(void **mutex)
{
    return rtos_init_mutex((beken_mutex_t *)mutex);
}

static int32_t lock_mutex(void **mutex)
{
    return rtos_lock_mutex((beken_mutex_t *)mutex);
}

static int32_t unlock_mutex(void **mutex)
{
    return rtos_unlock_mutex((beken_mutex_t *)mutex);
}

static int32_t deinit_mutex(void **mutex)
{
    return rtos_deinit_mutex((beken_mutex_t *)mutex);
}

static int32_t init_semaphore(void **semaphore)
{
    return rtos_init_semaphore((beken_semaphore_t *)semaphore, 1);
}

static int32_t init_semaphore_ext(void **semaphore, int32_t max_count, int32_t init_count)
{
    return rtos_init_semaphore_ex((beken_semaphore_t *)semaphore, max_count, init_count);
}

static int32_t set_semaphore(void **semaphore)
{
    return rtos_set_semaphore((beken_semaphore_t *)semaphore);
}

static int32_t get_semaphore(void **semaphore, uint32_t timeout_ms)
{
    return rtos_get_semaphore((beken_semaphore_t *)semaphore, timeout_ms);
}

static int32_t deinit_semaphore(void **semaphore)
{
    return rtos_deinit_semaphore((beken_semaphore_t *)semaphore);
}

static uint32_t get_time_wrapper(void)
{
    return rtos_get_time();
}

static void *malloc_wrapper(size_t size)
{
    return os_malloc(size);
}

static void free_wrapper(void *ptr)
{
    os_free(ptr);
}

static bool is_timer_init_wrapper(void *timer)
{
    return rtos_is_timer_init((beken_timer_t *)timer);
}

static int32_t init_timer_wrapper(void **timer, uint32_t time_ms, void *function, void *arg)
{
    beken_timer_t *tmp = (beken_timer_t *)os_malloc(sizeof(beken_timer_t));

    if (tmp == NULL) {
        return -1;
    }

    if (rtos_init_timer(tmp,time_ms,function,arg)) {
        os_free(tmp);
        return -1;
    }

    *timer = (void *)tmp;

    return 0;
}

static int32_t start_timer_wrapper(void *timer)
{
    return rtos_start_timer((beken_timer_t *)timer);
}

static int32_t stop_timer_wrapper(void *timer)
{
    return rtos_stop_timer((beken_timer_t *)timer);
}

static int32_t deinit_timer_wrapper(void *timer)
{
    rtos_deinit_timer((beken_timer_t *)timer);
    os_free(timer);
    return 0;
}

static int32_t timer_change_period(void *timer, uint32_t time_ms)
{
    return rtos_change_period((beken_timer_t *)timer,time_ms);
}

static int delay_milliseconds(uint32_t num_ms)
{
    return rtos_delay_milliseconds(num_ms);
}

static uint32_t get_ms_per_tick(void)
{
    return beken_ms_per_tick();
}

static int int_isr_register_wrapper(void *isr, void *arg)
{
    return bk_int_isr_register(INT_SRC_THREAD, (int_group_isr_t)isr, arg);
}

static void interrupt_ctrl_wrapper(bool en)
{
    sys_drv_thread_interrupt_ctrl(en);
}

static bool ate_is_enabled_wrapper(void)
{
    extern bool ate_is_enabled(void);
    return ate_is_enabled();
}

static void mac_clock_ctrl_wrapper(bool clock_state)
{
    bk_pm_clock_ctrl(CLK_PWR_ID_THREAD,(clock_state) ? 1 : 0);
}

static void drv_thread_rf_ctrl_wrapper(bool en)
{
    sys_drv_thread_rf_ctrl(en);
}

static void vote_rf_ctrl(uint8_t cmd)
{
    rf_module_vote_ctrl(cmd, RF_BY_THREAD_BIT);
}

static void thread_power_ctrl(uint8_t en)
{
    if (en) {
        //Power UP thread and PHY
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_THREAD,PM_POWER_MODULE_STATE_ON);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_PHY_THREAD,PM_POWER_MODULE_STATE_ON);
        //Switch High Power PLL
        rf_pll_ctrl(0, RF_WIFIPLL_HOLD_BY_THREAD_BIT);
    } else {
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_THREAD,PM_POWER_MODULE_STATE_OFF);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_PHY_THREAD,PM_POWER_MODULE_STATE_OFF);
        rf_pll_ctrl(1, RF_WIFIPLL_HOLD_BY_THREAD_BIT);
    }
}

static int32_t os_memcmp_wrapper(const void *s, const void *s1, uint32_t n)
{
    return os_memcmp(s, s1, (unsigned int)n);
}
static void *os_memset_wrapper(void *b, int c, uint32_t len)
{
    return (void *)os_memset(b, c, (unsigned int)len);
}
static uint32_t os_strtoul_wrapper(const char *nptr, char **endptr, int base)
{
    return os_strtoul(nptr, endptr, base);
}
static int os_strcasecmp_wrapper(const char *s1, const char *s2)
{
    return os_strcasecmp(s1, s2);
}
static int32_t os_strcmp_wrapper(const char *s1, const char *s2)
{
    return os_strcmp(s1, s2);
}
static int32_t os_strncmp_wrapper(const char *s1, const char *s2, const uint32_t n)
{
    return os_strncmp(s1, s2, n);
}
static void *os_memcpy_wrapper(void *out, const void *in, uint32_t n)
{
    return (void *)os_memcpy(out, in, n);
}

struct mac802154_osi_funcs_t g_mac802154_os_funcs = {
    ._init_queue = init_queue_wrapper,
    ._deinit_queue = deinit_queue_wrapper,
    ._pop_from_queue = pop_from_queue_wrapper,
    ._push_to_queue = push_to_queue_wrapper,
    ._push_to_queue_front = push_to_queue_front_wrapper,
    ._is_queue_empty = is_queue_empty_wrapper,
    ._create_thread = create_thread_wrapper,
    ._delete_thread = delete_thread_wrapper,
    ._thread_join = thread_join,
    ._current_thread = current_thread_wrapper,
    ._log = bk_printf_ext,
    ._log_raw = bk_printf_raw,
    ._get_time = get_time_wrapper,
    ._malloc = malloc_wrapper,
    ._free = free_wrapper,
    ._is_timer_init = is_timer_init_wrapper,
    ._init_timer = init_timer_wrapper,
    ._start_timer = start_timer_wrapper,
    ._stop_timer = stop_timer_wrapper,
    ._deinit_timer = deinit_timer_wrapper,
    ._timer_change_period = timer_change_period,
    ._init_mutex = init_mutex,
    ._lock_mutex = lock_mutex,
    ._unlock_mutex = unlock_mutex,
    ._deinit_mutex = deinit_mutex,
    ._init_semaphore = init_semaphore,
    ._init_semaphore_ext = init_semaphore_ext,
    ._set_semaphore = set_semaphore,
    ._get_semaphore = get_semaphore,
    ._deinit_semaphore = deinit_semaphore,
    ._delay_milliseconds = delay_milliseconds,
    ._get_ms_per_tick = get_ms_per_tick,
    ._int_isr_register = int_isr_register_wrapper,

    ._interrupt_ctrl = interrupt_ctrl_wrapper,
    ._ate_is_enabled = ate_is_enabled_wrapper,
    ._mac_clock_ctrl = mac_clock_ctrl_wrapper,
    ._drv_thread_rf_ctrl= drv_thread_rf_ctrl_wrapper,
    ._vote_rf_ctrl   = vote_rf_ctrl,
    ._thread_power_ctrl  = thread_power_ctrl,

    ._os_memcmp      = os_memcmp_wrapper,
    ._os_memset      = os_memset_wrapper,
    ._os_strtoul     = os_strtoul_wrapper,
    ._os_strcasecmp  = os_strcasecmp_wrapper,
    ._os_strcmp      = os_strcmp_wrapper,
    ._os_strncmp     = os_strncmp_wrapper,
    ._os_memcpy      = os_memcpy_wrapper,
};

