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

#ifndef __COEX_INTERNAL_H_
#define __COEX_INTERNAL_H_

#include <stdio.h>
#include <stdint.h>

struct coex_osi_funcs_t
{
    uint32_t _version;
    void (*_log)(int level, char *tag, const char *fmt, ...);
    void (*_log_raw)(int level, char * tag, const char *fmt, ...);
    void (*_rf_select_wifi)(void);
    void (*_rf_select_bt)(void);
    void (*_rf_select_pta)(void);
    uint8_t (*_rf_get_type)(void);
    void (*_wifi_pta_enable_set)(bool en);
    uint32_t (*_coex_get_time)(void);
    void (*_gpio)(uint32_t io,uint8_t type);
    void (*_gpio_init)(uint32_t io);
    void *(*_malloc)(unsigned int size);
    void (*_free)(void *p);
    void (*_timer_start)(uint32_t ms,void *cb);
    void (*_timer_stop)(void);
    uint32_t (*_disable_int)(void);
    void (*_enable_int)(uint32_t int_level);
};

typedef void (*coex_cb)(uint16_t duration);

void softcoex_init(void *osi_funcs);
void softcoex_deinit(void);
void softcoex_bt_request(uint32_t event,uint8_t mode,uint32_t duration);
void softcoex_bt_release(uint32_t event);
void softcoex_wifi_request(uint32_t event,uint8_t mode,uint32_t duration);
void softcoex_wifi_release(uint32_t event);
void softcoex_wifi_register_cb(coex_cb cb);
uint32_t softcoex_wifi_event_get(void);
#endif
