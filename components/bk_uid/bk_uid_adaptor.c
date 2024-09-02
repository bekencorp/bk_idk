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
#include "sha256.h"
#include <modules/uidlib.h>

#define UID_OSI_VERSION              0x00000001

static void *malloc_wrapper(size_t size)
{
    return os_malloc(size);
}

static void free_wrapper(void *ptr)
{
    os_free(ptr);
}

void bk_uid_mbedtls_sha256_init(void *ctx)
{
    mbedtls_sha256_init((mbedtls_sha256_context *)ctx);
}

int bk_uid_mbedtls_sha256_starts_ret(void *ctx, int is224)
{
    return mbedtls_sha256_starts_ret((mbedtls_sha256_context *)ctx, is224);
}

int bk_uid_mbedtls_sha256_update_ret(void *ctx, uint8_t *input, uint32_t ilen)
{
    return mbedtls_sha256_update_ret((mbedtls_sha256_context *)ctx, input, ilen);
}

int bk_uid_mbedtls_sha256_finish_ret(void *ctx, unsigned char output[32])
{
    return mbedtls_sha256_finish_ret((mbedtls_sha256_context *)ctx, output);
}

void bk_uid_mbedtls_sha256_free(void *ctx)
{
    mbedtls_sha256_free((mbedtls_sha256_context *)ctx);
}

static const struct uid_osi_funcs_t uid_osi_funcs = {
    ._version = UID_OSI_VERSION,
    .size = sizeof(struct uid_osi_funcs_t), 
    .sha256_ctx_size = sizeof(struct mbedtls_sha256_context),
     /* mem */
    ._malloc = malloc_wrapper,
    ._free = free_wrapper,
    /* sha256 */
    ._bk_uid_mbedtls_sha256_init = bk_uid_mbedtls_sha256_init,
    ._bk_uid_mbedtls_sha256_starts_ret = bk_uid_mbedtls_sha256_starts_ret,
    ._bk_uid_mbedtls_sha256_update_ret = bk_uid_mbedtls_sha256_update_ret,
    ._bk_uid_mbedtls_sha256_finish_ret = bk_uid_mbedtls_sha256_finish_ret,
    ._bk_uid_mbedtls_sha256_free = bk_uid_mbedtls_sha256_free,
    /* log */
    ._log = bk_printf_ext,
};

bk_err_t bk_uid_adaptor_init(void)
{
    bk_err_t ret = BK_OK;

    if (uid_osi_funcs_init((void*)&uid_osi_funcs) != 0)
    {
        return BK_FAIL;
    }

    return ret;
}

