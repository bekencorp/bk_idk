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
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


/* adapter function */
struct uid_osi_funcs_t {
    uint32_t _version;
    uint32_t size;
    uint32_t sha256_ctx_size;
    /* mem */
    void *(*_malloc)(unsigned int size);
    void (*_free)(void *p);
    /* sha256 */
    void (*_bk_uid_mbedtls_sha256_init)(void *ctx);
    int (*_bk_uid_mbedtls_sha256_starts_ret)(void *ctx, int is224);
    int (*_bk_uid_mbedtls_sha256_update_ret)(void *ctx, uint8_t *input, uint32_t ilen);
    int (*_bk_uid_mbedtls_sha256_finish_ret)(void *ctx, unsigned char output[32]);
    void (*_bk_uid_mbedtls_sha256_free)(void *ctx);
    /* log */
    void (*_log)(int level, char *tag, const char *fmt, ...);
};

