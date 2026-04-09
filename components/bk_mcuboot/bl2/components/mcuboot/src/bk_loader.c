/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2016-2020 Linaro LTD
 * Copyright (c) 2016-2019 JUUL Labs
 * Copyright (c) 2019-2023 Arm Limited
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2024 Beken
 *
 * Original license:
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "ow_loader.h"
#include "aon_pmu_hal.h"
static struct boot_loader_state boot_data;

void boot_state_clear(struct boot_loader_state *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(struct boot_loader_state));
    } else {
        memset(&boot_data, 0, sizeof(struct boot_loader_state));
    }
}


fih_ret
bk_boot_go(struct boot_rsp *rsp)
{
    FIH_DECLARE(fih_rc, FIH_FAILURE);

    boot_state_clear(NULL);
#if CONFIG_OTA_OVERWRITE || CONFIG_MIXED_OTA
    FIH_CALL(overwrite_boot_go, fih_rc, &boot_data, rsp);
#endif

#if CONFIG_DIRECT_XIP || CONFIG_MIXED_OTA
    FIH_CALL(xip_boot_go, fih_rc, &boot_data, rsp);
#endif
    aon_pmu_hal_set_app_boot_validate_pending(1u);
    fill_rsp(&boot_data, rsp);
    FIH_RET(fih_rc);
}