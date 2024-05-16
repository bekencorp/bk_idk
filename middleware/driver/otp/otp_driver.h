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

#pragma once

#include <components/log.h>

#define OTP_TAG "otp"
#define OTP_LOGI(...) BK_LOGI(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGW(...) BK_LOGW(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGE(...) BK_LOGE(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGD(...) BK_LOGD(OTP_TAG, ##__VA_ARGS__)

typedef struct
{
    const otp_id_t name;
    const uint32_t allocated_size;
    const uint32_t offset;
    otp_privilege_t privilege;
    //bool security;/*TODO*/
} otp_item_t;

typedef struct
{
    const otp2_id_t name;
    const uint32_t allocated_size;
    const uint32_t offset;
    otp_privilege_t privilege;
    //bool security;/*TODO*/
} otp2_item_t;