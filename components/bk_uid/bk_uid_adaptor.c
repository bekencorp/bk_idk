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

#include "os/os.h"
#include "bk_uid_adaptor.h"
#include <modules/uidlib.h>
#include <driver/otp.h>

#define UID_OSI_VERSION              0x00000001

static int uid_otp_apb_read_wrapper(uint32_t item, uint8_t *buf, uint32_t size)
{
#if CONFIG_OTP
    return (int)bk_otp_apb_read((otp1_id_t)item, buf, size);
#else
    (void)item;
    (void)buf;
    (void)size;
    return (int)BK_ERR_NOT_SUPPORT;
#endif
}
static int uid_otp_ahb_read_wrapper(uint32_t item, uint8_t *buf, uint32_t size)
{
#if CONFIG_OTP
    return (int)bk_otp_ahb_read((otp2_id_t)item, buf, size);
#else
    (void)item;
    (void)buf;
    (void)size;
    return (int)BK_ERR_NOT_SUPPORT;
#endif
}

static struct uid_osi_vars_t uid_osi_vars = {
#if CONFIG_OTP
    ._OTP_DEVICE_ID = OTP_DEVICE_ID,
#else
    ._OTP_DEVICE_ID = 0,
#endif
};

static const struct uid_osi_funcs_t uid_osi_funcs = {
    ._version = UID_OSI_VERSION,
    .size = sizeof(struct uid_osi_funcs_t),
    /* log */
    ._log = bk_printf_ext,
    /* otp read */
    ._bk_otp_apb_read = uid_otp_apb_read_wrapper,
    ._bk_otp_ahb_read = uid_otp_ahb_read_wrapper,
};

bk_err_t bk_uid_adaptor_init(void)
{
    bk_err_t ret = BK_OK;

    if (uid_osi_funcs_init((void *)&uid_osi_funcs, (void *)&uid_osi_vars) != 0)
    {
        return BK_FAIL;
    }

    return ret;
}

