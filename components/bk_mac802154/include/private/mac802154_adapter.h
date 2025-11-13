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

#ifndef __802154_ADAPTER_H
#define __802154_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>

struct mac802154_osi_funcs_t {
    uint32_t _version;
    int (*_init_queue)(void **queue, const char *name, uint32_t message_size, uint32_t number_of_messages);
    int (*_deinit_queue)(void **queue);
    int (*_pop_from_queue)(void **queue, void *message, uint32_t timeout_ms);
    int (*_push_to_queue)(void **queue, void *message, uint32_t timeout_ms);
    int (*_push_to_queue_front)(void **queue, void *message, uint32_t timeout_ms);
    bool (*_is_queue_empty)(void **queue);
    int (*_create_thread)(void **thread, uint8_t priority, const char *name,
                          void *function, uint32_t stack_size, void *arg);
    int (*_delete_thread)(void **thread);
    int (*_thread_join)(void **thread);
    void *(*_current_thread)(void);
    int32_t (*_init_mutex)(void **mutex);
    int32_t (*_lock_mutex)(void **mutex);
    int32_t (*_unlock_mutex)(void **mutex);
    int32_t (*_deinit_mutex)(void **mutex);
    int32_t (*_init_semaphore)(void **semaphore);
    int32_t (*_init_semaphore_ext)(void **semaphore, int32_t max_count, int32_t init_count);
    int32_t (*_set_semaphore)(void **semaphore);
    int32_t (*_get_semaphore)(void **semaphore, uint32_t timeout_ms);
    int32_t (*_deinit_semaphore)(void **semaphore);
    int32_t (*_init_timer)(void **timer, uint32_t time_ms, void *function, void *arg);
    int32_t (*_deinit_timer)(void *timer);
    int32_t (*_stop_timer)(void *timer);
    int32_t (*_timer_change_period)(void *timer, uint32_t time_ms);
    bool (*_is_timer_init)(void *timer);
    int32_t (*_start_timer)(void *timer);
    bool (*_is_timer_running)(void *timer);
    void (*_log)(int level, char *tag, const char *fmt, ...);
    void (*_log_raw)(int level, char *tag, const char *fmt, ...);
    uint32_t (*_get_time)(void);
    void *(*_malloc)(unsigned int size);
    void (*_free)(void *p);
    int (*_delay_milliseconds)(uint32_t num_ms);
    uint32_t (*_get_ms_per_tick)(void);
    int (*_int_isr_register)(void *isr, void *arg);
    void (*_interrupt_ctrl)(bool en);
    bool (*_ate_is_enabled)(void);
    void (*_mac_clock_ctrl)(bool clock_state);
    void (*_drv_thread_rf_ctrl)(bool en);
    void (*_vote_rf_ctrl)(uint8_t cmd);
    void (*_thread_power_ctrl)(uint8_t en);

    int32_t (*_os_memcmp)(const void *s, const void *s1, uint32_t n);
    void *(*_os_memset)(void *b, int c, uint32_t len);
    uint32_t (*_os_strtoul)(const char *nptr, char **endptr, int base);
    int (*_os_strcasecmp)(const char *s1, const char *s2);
    int32_t (*_os_strcmp)(const char *s1, const char *s2);
    int32_t (*_os_strncmp)(const char *s1, const char *s2, const uint32_t n);
    void *(*_os_memcpy)(void *out, const void *in, uint32_t n);
};

int mac802154_osi_init(void *osi_funcs);
uint8_t mac802154_mac_init(void);
void mac802154_mac_deinit(void);

#endif
