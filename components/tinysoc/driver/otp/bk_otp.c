// Copyright 2024-2025 Beken
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

#include "otp_driver.h"
#include "common_interfaces.h"

bool is_secureboot_enabled(void)
{
    return false;
}

bool is_spe_debug_enabled(void)
{
    return false;
}

bool is_info_log_enabled(void)
{
    return true;
}

bool is_err_log_enabled(void)
{
    return true;
}

bool is_secure_download_enabled(void)
{
    return false;
}
