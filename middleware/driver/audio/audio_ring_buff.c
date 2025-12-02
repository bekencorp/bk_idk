// Copyright 2020-2025 Beken
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

#include <driver/dma.h>
#include <bk_general_dma.h>
#include <driver/audio_ring_buff.h>
#include <stdlib.h>
#include <string.h>

//#define RWP_SAFE_INTERVAL   (4)
#define RWP_SAFE_INTERVAL   (0)

void ring_buffer_init(RingBufferContext* rb, uint8_t* addr, uint32_t capacity, dma_id_t dma_id, uint32_t dma_type)
{
    rb->address  = addr;
    rb->capacity = capacity;
    rb->wp       = 0;
    rb->rp       = 0;
    rb->dma_id   = dma_id;
    rb->dma_type = dma_type;

    if(DMA_ID_MAX == dma_id)
    {
        return;
    }

    if(rb->dma_type == RB_DMA_TYPE_READ)
    {
        dma_set_src_pause_addr(dma_id, (uint32_t)addr);
    }
    else if(rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        dma_set_dst_pause_addr(dma_id, (uint32_t)addr + capacity);
    }
}

void ring_buffer_clear(RingBufferContext* rb)
{
    rb->wp = 0;
    rb->rp = 0;

    if(rb->dma_id == DMA_ID_MAX)
    {
        return;
    }

    if(RB_DMA_TYPE_READ == rb->dma_type)
    {
        /* Ring buffer is cleared, so set the read pause address to the ring buffer start address.
         * This causes DMA to pause immediately until new data is written.
         */
        dma_set_src_pause_addr(rb->dma_id, (uint32_t)rb->address);
    }
    else if(RB_DMA_TYPE_WRITE == rb->dma_type)
    {
        /* Ring buffer is cleared, so set the write pause address to the ring buffer end address.
         * This allows DMA to fill the entire ring buffer from the start.
         */
        dma_set_dst_pause_addr(rb->dma_id, (uint32_t)rb->address + rb->capacity);
    }
}

uint32_t ring_buffer_read(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t required_bytes = size;
    uint32_t read_bytes;
    uint32_t remain_bytes;
    uint32_t wp;

    if((rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_WRITE))
    {
        //读取DMA寄存器[dma1_dest_wr_addr],获取此时已经读到的地址
        wp = rb->wp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_dest_write_addr(rb->dma_id) - (uint32_t)rb->address : 0;
    }
    else
    {
        wp = rb->wp;
    }

    if(required_bytes == 0) return 0;

    if(wp >= rb->rp)
    {
        remain_bytes = wp - rb->rp;

        if(required_bytes > remain_bytes)
        {
            read_bytes = remain_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            rb->rp = (rb->rp + read_bytes) % rb->capacity;
        }
        else
        {
            read_bytes = required_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            rb->rp = (rb->rp + read_bytes) % rb->capacity;
        }
    }
    else
    {
        remain_bytes = rb->capacity - rb->rp;

        if(required_bytes > remain_bytes)
        {
            read_bytes = remain_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);

            if(required_bytes - read_bytes > wp)
            {
                memcpy(buffer + read_bytes, &rb->address[0], wp);
                rb->rp = wp;
                read_bytes += wp;
            }
            else
            {
                memcpy(buffer + read_bytes, &rb->address[0], required_bytes - read_bytes);
                rb->rp = required_bytes - read_bytes;
                read_bytes = required_bytes;
            }
        }
        else
        {
            read_bytes = required_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            //rb->rp += read_bytes;
            rb->rp = (rb->rp + read_bytes) % rb->capacity;
        }
    }

    #if 0
    if(rb->dma && rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        dma_channel_dst_curr_address_set(rb->dma, (uint32_t)&rb->address[rb->rp]);
    }
    #endif

    return read_bytes;
}

uint32_t ring_buffer_write(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t remain_bytes, rp;
    uint32_t write_bytes = size;

    if(0 == write_bytes) 
    {
        return 0;
    }

    if((DMA_ID_MAX != rb->dma_id) && (RB_DMA_TYPE_READ == rb->dma_type))
    {
        /* Read DMA register [dma_src_rd_addr] to get the current read address.
         * Convert the absolute address to an offset relative to the ring buffer start address.
         * If DMA is not enabled, set rp to 0.
         */
        rp = rb->rp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_src_read_addr(rb->dma_id) - (uint32_t)rb->address : 0;
    }
    else
    {
        rp = rb->rp;
    }

    if(rb->wp >= rp)
    {
        remain_bytes = rb->capacity - rb->wp + rp;
        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            remain_bytes = rb->capacity - rb->wp;

            if(remain_bytes >= write_bytes)
            {
                memcpy(&rb->address[rb->wp], buffer, write_bytes);
                rb->wp = (rb->wp + write_bytes) % rb->capacity;
            }
            else
            {
                memcpy(&rb->address[rb->wp], buffer, remain_bytes);
                rb->wp = write_bytes - remain_bytes;
                memcpy(&rb->address[0], &buffer[remain_bytes], rb->wp);
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        remain_bytes = rp - rb->wp;
        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            memcpy(&rb->address[rb->wp], buffer, write_bytes);
            rb->wp = (rb->wp + write_bytes) % rb->capacity;
        }
        else
        {
            return 0;
        }
    }

    if(rb->wp >= rb->capacity && rb->rp)
    {
        rb->wp = 0;
    }

    if((rb->dma_id != DMA_ID_MAX) && (rb->dma_type == RB_DMA_TYPE_READ))
    {
        /* When writing data to a ring buffer that DMA reads from, after writing data,
         * we need to set the write pointer (wp) address value to [dma_src_pause_addr].
         * This ensures that DMA will pause data transfer when it reaches this address.
         */
        dma_set_src_pause_addr(rb->dma_id, (uint32_t)&rb->address[rb->wp]);
    }

    return write_bytes;
}

uint32_t ring_buffer_get_fill_size(RingBufferContext* rb)
{
    uint32_t rp, wp;
    uint32_t fill_size;

    if(rb->dma_id != DMA_ID_MAX)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            rp = rb->rp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_src_read_addr(rb->dma_id) - (uint32_t)rb->address : 0;
            wp = rb->wp;
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            rp = rb->rp;
            wp = rb->wp = bk_dma_get_enable_status(rb->dma_id) ? dma_get_dest_write_addr(rb->dma_id) - (uint32_t)rb->address : 0;
        }
        else
        {
            rp = rb->rp;
            wp = rb->wp;
        }
    }
    else
    {
        rp = rb->rp;
        wp = rb->wp;
    }

    fill_size = wp >= rp ? wp - rp : rb->capacity - rp + wp;

    return fill_size;
}

uint32_t ring_buffer_get_free_size(RingBufferContext* rb)
{
    uint32_t free_size;

    free_size = rb->capacity - ring_buffer_get_fill_size(rb);

    return free_size > RWP_SAFE_INTERVAL ? free_size - RWP_SAFE_INTERVAL : 0;
}

