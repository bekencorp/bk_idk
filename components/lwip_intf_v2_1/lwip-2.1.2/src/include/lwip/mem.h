/**
 * @file
 * Heap API
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_MEM_H
#define LWIP_HDR_MEM_H

#include "lwip/opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MEM_LIBC_MALLOC

#include "lwip/arch.h"

typedef size_t mem_size_t;
#define MEM_SIZE_F SZT_F

#elif MEM_USE_POOLS

typedef u16_t mem_size_t;
#define MEM_SIZE_F U16_F

#else

/* MEM_SIZE would have to be aligned, but using 64000 here instead of
 * 65535 leaves some room for alignment...
 */
#if MEM_SIZE > 64000L
typedef u32_t mem_size_t;
#define MEM_SIZE_F U32_F
#else
typedef u16_t mem_size_t;
#define MEM_SIZE_F U16_F
#endif /* MEM_SIZE > 64000 */
#endif

#if MEM_TRX_DYNAMIC_EN
#define MEM_TYPE_TX        1
#define MEM_TYPE_RX        2
#define MEM_TYPE_POOL      3

struct mem_with_limit_t {
  mem_size_t size;
  u8_t type;
};
#endif
struct mem {
  /** index (-> ram[next]) of the next struct */
  mem_size_t next;
  /** index (-> ram[prev]) of the previous struct */
  mem_size_t prev;
  /** 1: this area is used; 0: this area is unused */
  u8_t used;
#if MEM_TRX_DYNAMIC_EN
  u8_t type;
#endif
#if MEM_OVERFLOW_CHECK
  /** this keeps track of the user allocation size for guard checks */
  mem_size_t user_size;
#endif
};

void  mem_init(void);
void *mem_trim(void *mem, mem_size_t size);
#if MEM_TRX_DYNAMIC_EN
void *mem_malloc_trx(mem_size_t size, u8_t type);
#define mem_malloc(size_in)     mem_malloc_trx(size_in, MEM_TYPE_TX)
#define mem_malloc_rx(size_in)  mem_malloc_trx(size_in, MEM_TYPE_RX)
#else
void *mem_malloc(mem_size_t size);
#endif
void *mem_calloc(mem_size_t count, mem_size_t size);
void  mem_free(void *mem);
u32_t get_mem_size(void *rmem);
uint32_t mem_sanity_check(void *mem);
#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_MEM_H */
