/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NVS_INTERNAL_H
#define NVS_INTERNAL_H

/**
 * @brief Partition subtype
 *
 * @note These ESP-IDF-defined partition subtypes apply to partitions of type ESP_PARTITION_TYPE_APP
 * and ESP_PARTITION_TYPE_DATA.
 *
 * Application-defined partition types (0x40-0xFE) can set any numeric subtype value.
 *
 * @internal Keep this enum in sync with PartitionDefinition class gen_esp32part.py @endinternal
 */
typedef enum {
    ESP_PARTITION_SUBTYPE_APP_FACTORY = 0x00,                                 //!< Factory application partition
    ESP_PARTITION_SUBTYPE_APP_OTA_MIN = 0x10,                                 //!< Base for OTA partition subtypes
    ESP_PARTITION_SUBTYPE_APP_TEST = 0x20,                                    //!< Test application partition

    ESP_PARTITION_SUBTYPE_DATA_OTA = 0x00,                                    //!< OTA selection partition
    ESP_PARTITION_SUBTYPE_DATA_PHY = 0x01,                                    //!< PHY init data partition
    ESP_PARTITION_SUBTYPE_DATA_NVS = 0x02,                                    //!< NVS partition
    ESP_PARTITION_SUBTYPE_DATA_COREDUMP = 0x03,                               //!< COREDUMP partition
    ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS = 0x04,                               //!< Partition for NVS keys
    ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM = 0x05,                               //!< Partition for emulate eFuse bits
    ESP_PARTITION_SUBTYPE_DATA_UNDEFINED = 0x06,                              //!< Undefined (or unspecified) data partition

    ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD = 0x80,                               //!< ESPHTTPD partition
    ESP_PARTITION_SUBTYPE_DATA_FAT = 0x81,                                    //!< FAT partition
    ESP_PARTITION_SUBTYPE_DATA_SPIFFS = 0x82,                                 //!< SPIFFS partition
    ESP_PARTITION_SUBTYPE_DATA_LITTLEFS = 0x83,                               //!< LITTLEFS partition

    ESP_PARTITION_SUBTYPE_ANY = 0xff,                                         //!< Used to search for partitions with any subtype
} esp_partition_subtype_t;

/**
 * @brief Partition type
 *
 * @note Partition types with integer value 0x00-0x3F are reserved for partition types defined by ESP-IDF.
 * Any other integer value 0x40-0xFE can be used by individual applications, without restriction.
 *
 * @internal Keep this enum in sync with PartitionDefinition class gen_esp32part.py @endinternal
 *
 */
typedef enum {
    ESP_PARTITION_TYPE_APP = 0x00,       //!< Application partition type
    ESP_PARTITION_TYPE_DATA = 0x01,      //!< Data partition type

    ESP_PARTITION_TYPE_ANY = 0xff,       //!< Used to search for partitions with any type
} esp_partition_type_t;

#if CONFIG_NVS_ASSERT_ERROR_CHECK
#define NVS_ASSERT_OR_RETURN(condition, retcode) assert(condition);
#else
#define NVS_ASSERT_OR_RETURN(condition, retcode) \
    if (!(condition)) {                          \
        return retcode;                          \
    }
#endif // CONFIG_NVS_ASSERT_ERROR_CHECK

#endif // NVS_INTERNAL_H
