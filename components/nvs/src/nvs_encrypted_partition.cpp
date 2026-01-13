/*
 * SPDX-FileCopyrightText: 2019-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 /*
* Change Logs:
* Date			 Author 	  Notes
* 2024-04-11	 Beken	  adapter to Beken sdk
*/

#include <cstring>
#include "nvs_encrypted_partition.hpp"
#include "nvs_types.hpp"

namespace nvs {

NVSEncryptedPartition::NVSEncryptedPartition(const bk_logic_partition_t *partition)
    : NVSPartition(partition) { }

bk_err_t NVSEncryptedPartition::init(nvs_sec_cfg_t* cfg)
{
    uint8_t* eky = reinterpret_cast<uint8_t*>(cfg);

    mbedtls_aes_xts_init(&mEctxt);
    mbedtls_aes_xts_init(&mDctxt);

    if (mbedtls_aes_xts_setkey_enc(&mEctxt, eky, 2 * NVS_KEY_SIZE * 8) != 0) {
        return ESP_ERR_NVS_XTS_CFG_FAILED;
    }

    if (mbedtls_aes_xts_setkey_dec(&mDctxt, eky, 2 * NVS_KEY_SIZE * 8) != 0) {
        return ESP_ERR_NVS_XTS_CFG_FAILED;
    }

    return ESP_OK;
}

NVSEncryptedPartition::~NVSEncryptedPartition()
{	
    mbedtls_aes_xts_free(&mEctxt);
    mbedtls_aes_xts_free(&mDctxt);
}


bk_err_t NVSEncryptedPartition::read(size_t src_offset, void* dst, size_t size)
{
        /** Currently upper layer of NVS reads entries one by one even for variable size
    * multi-entry data types. So length should always be equal to size of an entry.*/
    if (size != sizeof(Item)) 
		return ESP_ERR_INVALID_SIZE;

    // read data
    bk_err_t read_result = bk_flash_partition_read(partition_id, (uint8_t *)dst, src_offset, size);
    if (read_result != ESP_OK) {
        return read_result;
    }

    // decrypt data
    //sector num required as an arr by mbedtls. Should have been just uint64/32.
    uint8_t data_unit[16];
    uint32_t relAddr = src_offset;

    memset(data_unit, 0, sizeof(data_unit));
    memcpy(data_unit, &relAddr, sizeof(relAddr));

    uint8_t *destination = reinterpret_cast<uint8_t*>(dst);

    if (mbedtls_aes_crypt_xts(&mDctxt, MBEDTLS_AES_DECRYPT, size, data_unit, destination, destination) != 0)  {
        return ESP_ERR_NVS_XTS_DECR_FAILED;
    }

    return ESP_OK;
}

bk_err_t NVSEncryptedPartition::write(size_t addr, const void* src, size_t size)
{
        if (size % ESP_ENCRYPT_BLOCK_SIZE != 0) 
		return ESP_ERR_INVALID_SIZE;
	
    // copy data to buffer for encryption
    uint8_t* buf = new (std::nothrow) uint8_t [size];
    if (!buf) 
		return ESP_ERR_NO_MEM;

    memcpy(buf, src, size);

    // encrypt data
    uint8_t entrySize = sizeof(Item);

    //sector num required as an arr by mbedtls. Should have been just uint64/32.
    uint8_t data_unit[16];

    /* Use relative address instead of absolute address (relocatable), so that host-generated
     * encrypted nvs images can be used*/
    uint32_t relAddr = addr;

    memset(data_unit, 0, sizeof(data_unit));

    for(uint8_t entry = 0; entry < (size/entrySize); entry++)
    {
        uint32_t offset = entry * entrySize;
        uint32_t *addr_loc = (uint32_t*) &data_unit[0];

        *addr_loc = relAddr + offset;
        if (mbedtls_aes_crypt_xts(&mEctxt,
                                  MBEDTLS_AES_ENCRYPT,
                                  entrySize,
                                  data_unit,
                                  buf + offset,
                                  buf + offset) != 0)  {
            delete [] buf;
            return ESP_ERR_NVS_XTS_ENCR_FAILED;
        }
    }

    // write data
    bk_err_t result = bk_flash_partition_write(partition_id, buf, addr, size);

    delete [] buf;

    return result;
}

} // nvs
