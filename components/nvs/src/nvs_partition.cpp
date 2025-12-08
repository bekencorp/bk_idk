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


#include <cstdlib>
#include "nvs_partition.hpp"

namespace nvs {

NVSPartition::NVSPartition(const bk_logic_partition_t* partition)
    : mESPPartition(partition)
{
	partition_id = (bk_partition_t)flash_partition_get_index(partition);
	
    // ensure the class is in a valid state
    if (partition == nullptr) {
        std::abort();
    }
}

const char *NVSPartition::get_partition_name()
{
    return mESPPartition->partition_description;
}

bk_err_t NVSPartition::read_raw(size_t src_offset, void* dst, size_t size)
{
    return bk_flash_partition_read(partition_id, (uint8_t *)dst, (uint32_t)src_offset, (uint32_t)size);
}

bk_err_t NVSPartition::read(size_t src_offset, void* dst, size_t size)
{
    if (size % ESP_ENCRYPT_BLOCK_SIZE != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return bk_flash_partition_read(partition_id, (uint8_t *)dst, (uint32_t)src_offset, (uint32_t)size);
}

bk_err_t NVSPartition::write_raw(size_t dst_offset, const void* src, size_t size)
{
    return bk_flash_partition_write(partition_id, (uint8_t *)src, (uint32_t)dst_offset, (uint32_t)size);
}

bk_err_t NVSPartition::write(size_t dst_offset, const void* src, size_t size)
{
    *(volatile uint32_t *)(0x44000400 + 15 * 4) = 0;
    *(volatile uint32_t *)(0x44000400 + 15 * 4) = 2;

    if (size % ESP_ENCRYPT_BLOCK_SIZE != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return bk_flash_partition_write(partition_id, (uint8_t *)src, (uint32_t)dst_offset, (uint32_t)size);
}

bk_err_t NVSPartition::erase_range(size_t dst_offset, size_t size)
{
    return bk_flash_partition_erase(partition_id, (uint32_t)dst_offset, (uint32_t)size);
}

uint32_t NVSPartition::get_address()
{
    return mESPPartition->partition_start_addr;
}

uint32_t NVSPartition::get_size()
{
    return mESPPartition->partition_length;
}

} // nvs
