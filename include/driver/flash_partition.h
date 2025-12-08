/**
 *********************************************************************************
 * @file flash_partition.h
 * @brief This file provides all the headers of Flash operation functions..
 *********************************************************************************
 *
 *Copyright 2020-2025 Beken
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 *********************************************************************************
*/
#pragma once
#include <common/bk_include.h>
#include <partitions.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (!CONFIG_FLASH_ORIGIN_API)

#define PAR_OPT_READ_POS     (0)
#define PAR_OPT_READ_DIS     (0x0u << PAR_OPT_READ_POS)
#define PAR_OPT_READ_EN     (0x1u << PAR_OPT_READ_POS)

#define PAR_OPT_WRITE_POS     (1)
#define PAR_OPT_WRITE_DIS     (0x0u << PAR_OPT_WRITE_POS)
#define PAR_OPT_WRITE_EN     (0x1u << PAR_OPT_WRITE_POS)

#define PAR_OPT_EXECUTE_POS     (2)
#define PAR_OPT_EXECUTE_DIS     (0x0u << PAR_OPT_EXECUTE_POS)
#define PAR_OPT_EXECUTE_EN     (0x1u << PAR_OPT_EXECUTE_POS)

#endif

typedef enum
{
    BK_FLASH_EMBEDDED = 0,
    BK_FLASH_SPI = 1,
    BK_FLASH_MAX = 2,
    BK_FLASH_NONE = 3,
}bk_flash_t;

typedef uint32_t bk_partition_t;
typedef uint16_t bk_partition_type_t;

enum {
	PARTITION_TYPE_DATA = 0,
	PARTITION_TYPE_APP,
};

typedef uint16_t bk_partition_subtype_t;

//TODO auto generate partition type
enum {
	PARTITION_SUBTYPE_DEFAULT          = 0,
	PARTITION_SUBTYPE_OW_ACTIVE        = 1,
	PARTITION_SUBTYPE_OW_OTA           = 2,
	PARTITION_SUBTYPE_XIP_A            = 3,
	PARTITION_SUBTYPE_XIP_B            = 4,
	PARTITION_SUBTYPE_MERGE_OVERWRITE  = 5,
	PARTITION_SUBTYPE_MERGE_XIP_A      = 6,
	PARTITION_SUBTYPE_MERGE_XIP_B      = 7,
	PARTITION_SUBTYPE_BL1_CTRL         = 8,
	PARTITION_SUBTYPE_BL1_MANIFEST     = 9,
	PARTITION_SUBTYPE_NVS              = 10,
	PARTITION_SUBTYPE_PARTITION        = 11,
	PARTITION_SUBTYPE_OW_OTA_CONTROL   = 12,
	PARTITION_SUBTYPE_XIP_OTA_CONTROL  = 13,
};

#define PARTITION_FLAGS_DBUS_READ            (1<<0)
#define PARTITION_FLAGS_DBUS_WRITE           (1<<1)
#define PARTITION_FLAGS_EXECUTE              (1<<2)
#define PARTITION_FLAGS_CBUS_READ            (1<<3)
#define PARTITION_FLAGS_CBUS_WRITE           (1<<4)
#define PARTITION_FLAGS_OTA_WRITE_CBUS       (1<<5)
#define PARTITION_FLAGS_DOWNLOAD_WRITE_CBUS  (1<<6)
#define PARTITION_FLAGS_OTA_READ_CBUS        (1<<7)
#define PARTITION_FLAGS_CBUS_NS              (1<<8)
#define PARTITION_FLAGS_DBUS_NS              (1<<9)

typedef struct
{
    bk_flash_t partition_owner;			 	/**< flash partition owners */
    const char *partition_description;		/**< flash partition description */
    uint32_t partition_start_addr;			/**< flash partition start address */
    uint32_t partition_length;				/**< flash partition length */
    uint32_t partition_options;				/**< flash partition options */
}bk_logic_partition_t;

/**
 * @brief   Get the infomation of the specified flash area
 *
 * @param partition:  The target flash logical partition
 *
 * @return bk flash logic partition struct
 */
bk_logic_partition_t *bk_flash_partition_get_info(bk_partition_t partition);

/**
 * @brief   Erase an area on a Flash logical partition
 *
 * @note    Erase on an address will erase all data on a sector that the
 *          address is belonged to, this function does not save data that
 *          beyond the address area but in the affected sector, the data
 *          will be lost.
 *
 * @param  partition: The target flash logical partition which should be erased
 * @param  offset: Start address of the erased flash area
 * @param  size: Size of the erased flash area
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_flash_partition_erase(bk_partition_t partition, uint32_t offset, uint32_t size);

/**
 * @brief   Erase an area on a Flash logical partition
 *
 * @note    Erase on an address will erase all data on a sector that the
 *          address is belonged to, this function does not save data that
 *          beyond the address area but in the affected sector, the data
 *          will be lost.
 *
 * @param  label: The target flash logical partition which should be erased
 * @param  offset: Start address of the erased flash area
 * @param  size: Size of the erased flash area
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_flash_partition_erase_by_name(const char *label, uint32_t offset, uint32_t size);

/**
 * @brief   Erase an area on a Flash logical partition
 *
 * @note    Erase on an address will erase all data on a sector that the
 *          address is belonged to, this function does not save data that
 *          beyond the address area but in the affected sector, the data
 *          will be lost.
 *
 * @param  label: The target flash logical partition which should be erased all
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_flash_partition_erase_all(const char *label);

/**
 * @brief  Write data to an area on a Flash logical partition
 *
 * @param  partition: The target flash logical partition which should be written
 * @param  buffer: Pointer to the data buffer that will be written to flash
 * @param  off_set: The offset of write address
 * @param  buffer_len: The length of the buffer
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_flash_partition_write(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len);

/**
 * @brief  Write data to an area on a Flash logical partition
 *
 * @param  label: The target flash logical partition which should be written
 * @param  buffer: Pointer to the data buffer that will be written to flash
 * @param  off_set: The offset of write address
 * @param  buffer_len: The length of the buffer
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_flash_partition_write_by_name(const char *label, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len);

/**
 * @brief    Read data from an area on a Flash to data buffer in RAM
 *
 * @param    partition: The target flash logical partition which should be read
 * @param    out_buffer: Pointer to the data buffer that stores the data read from flash
 * @param    offsets: The offset of read address
 * @param    buffer_len: The length of the buffer
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_flash_partition_read(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len);

/**
 * @brief    Read data from an area on a Flash to data buffer in RAM
 *
 * @param    label: The target flash logical partition which should be read
 * @param    out_buffer: Pointer to the data buffer that stores the data read from flash
 * @param    offsets: The offset of read address
 * @param    buffer_len: The length of the buffer
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_flash_partition_read_by_name(const char *label, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len);

bk_err_t bk_flash_partition_read_cbus(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len);

/**
 * @brief    Write a block to flash via the control (CBUS) channel.
 *
 * @param    partition: Target logical partition.
 * @param    buffer: Pointer to source data to be written.
 * @param    offset: Offset inside the partition where writing starts.
 * @param    buffer_len: Number of bytes to write.
 *
 * @return
 *    - BK_OK on success
 *    - other bk_err_t codes on failure
 */
bk_err_t bk_flash_partition_write_cbus(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len);

/**
 * @brief    Lookup the logical partition metadata by its name label.
 *
 * @param    label: Null-terminated name of the desired partition.
 *
 * @return   Pointer to partition descriptor or NULL if not found.
 */
const bk_logic_partition_t * get_partition_info_by_name(const char *label);

/**
 * @brief    Retrieve partition metadata from the enum identifier.
 *
 * @param    partition: Enum value defined in bk_partition_t.
 *
 * @return   Pointer to matching partition descriptor, or NULL when invalid.
 */
const bk_logic_partition_t * get_partition_info(bk_partition_t partition);

/**
 * @brief    Redundant helper matching `get_partition_info_by_name`, exported for legacy compatibility.
 *
 * @param    label: Name of the partition to query.
 *
 * @return   Partition descriptor pointer or NULL when not found.
 */
const bk_logic_partition_t * bk_flash_partition_get_info_by_name(const char *label);

/**
 * @brief    Convert a partition descriptor into its table index.
 *
 * @param    partition: Pointer to the logic partition returned by other helpers.
 *
 * @return   Index (0-based) inside the flash partition table.
 */
uint32_t get_partition_index(const bk_logic_partition_t* partition);

/**
 * @brief     Write data to flash (only operating 4k flash space)
 *
 * @param partition partition to write  (eg:partition BK_PARTITION_SYS_RF)
 * @param user_buf the pointer to data which is to write
 * @param size size to write
 * @param offset offset to write
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_FLASH_ADDR_OUT_OF_RANGE: flash address is out of range
 *    - others: other errors.
 */
bk_err_t bk_spec_flash_write_bytes(bk_partition_t partition, const uint8_t *user_buf, uint32_t size,uint32_t offset);

/**
 * @brief    Extract the stored type (first byte) from partition options.
 *
 * @param    partition: Pointer to the partition descriptor.
 *
 * @return   Type field encoded into bits [31:24] of partition_options.
 */
static inline uint32_t bk_flash_partition_get_type(const bk_logic_partition_t* partition)
{
	return (partition->partition_options >> 24) & 0xFF;
}

/**
 * @brief    Extract the option flags (lower 24 bits) from partition_options.
 *
 * @param    partition: Pointer to the partition descriptor.
 *
 * @return   Option bitmap stored in bits [23:0] of partition_options.
 */
static inline uint32_t bk_flash_partition_get_options(const bk_logic_partition_t* partition)
{
	return partition->partition_options & 0xFFFFFF;
}

#ifdef __cplusplus
}
#endif
