// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
// Copyright 2024 Beken Corporation
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
* Change Logs:
* Date			 Author 	  Notes
* 2024-04-11	 Beken	  adapter to Beken sdk
*/

#pragma once

#include "nvs_err.h"
#include "lock.h"

namespace nvs
{
    class Lock
    {
    public:
        Lock();
        ~Lock();
        static bk_err_t init();
        static void uninit();
    private:
        static _lock_t mSemaphore;
    };
} // namespace nvs
