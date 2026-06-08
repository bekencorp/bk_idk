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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_driver.h"
#include "flash_hal.h"
#include <ctype.h>

#define TAG    "partition"

#define NVS_KEY_SIZE        32 // AES-256
#define DATAUNIT_SIZE       32

#define PARTITION_AMOUNT    50

#define FLASH_PHYSICAL_ADDR_UNIT_SIZE     34
#define FLASH_LOGICAL_ADDR_UNIT_SIZE      32

#define FLASH_PHY_ADDR_VALID(addr)    (((addr) % FLASH_PHYSICAL_ADDR_UNIT_SIZE) < FLASH_LOGICAL_ADDR_UNIT_SIZE)
#define FLASH_PHY_2_LOGICAL(addr)     ((((addr) / FLASH_PHYSICAL_ADDR_UNIT_SIZE) * FLASH_LOGICAL_ADDR_UNIT_SIZE) + ((addr) % FLASH_PHYSICAL_ADDR_UNIT_SIZE))
#define FLASH_LOGICAL_2_PHY(addr)     ((((addr) / FLASH_LOGICAL_ADDR_UNIT_SIZE) * FLASH_PHYSICAL_ADDR_UNIT_SIZE) + ((addr) % FLASH_LOGICAL_ADDR_UNIT_SIZE))

#define SOC_FLASH_BASE_ADDR           0x02000000
#define FLASH_LOGICAL_BASE_ADDR       SOC_FLASH_BASE_ADDR
#if defined(CONFIG_NVS_PHY_PARTITION_OFFSET)
#define FLASH_PROTECT_FREE_END_ADDR   (CONFIG_NVS_PHY_PARTITION_OFFSET - 1U)
#else
#define FLASH_PROTECT_FREE_END_ADDR   (FLASH_MAX_SIZE- 1U)
#endif

#if (CONFIG_TFM_FWU)
#include "tfm_flash_nsc.h"
#endif

#if CONFIG_NVS_ENCRYPTION
#include "nvs_flash.h"
#include "nvs_sec_provider.h"
#else
char eky[4 * NVS_KEY_SIZE + 1] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
};
#endif

uint32_t flash_partition_get_index(const bk_logic_partition_t *partition_ptr);

#define PARTITION_PARTITION_PHY_OFFSET   CONFIG_PARTITION_PHY_PARTITION_OFFSET
#define PARTITION_PPC_OFFSET             0x400
#define PARTITION_NAME_LEN               32
#define PARTITION_ENTRY_LEN              44
#define PARTITION_OFFSET_OFFSET          34
#define PARTITION_SIZE_OFFSET            38
#define PARTITION_FLAGS_OFFSET           42
static struct {
    char name[PARTITION_NAME_LEN + 1];
    uint32_t phy_flags;
} cachedPartitions[PARTITION_AMOUNT];

#if CONFIG_FLASH_ORIGIN_API
#define PAR_OPT_READ_POS      (0)
#define PAR_OPT_WRITE_POS     (1)

#define PAR_OPT_READ_DIS      (0x0u << PAR_OPT_READ_POS)
#define PAR_OPT_READ_EN       (0x1u << PAR_OPT_READ_POS)
#define PAR_OPT_WRITE_DIS     (0x0u << PAR_OPT_WRITE_POS)
#define PAR_OPT_WRITE_EN      (0x1u << PAR_OPT_WRITE_POS)
#endif

#define PARTITION_IRAM         __attribute__((section(".iram")))

static const bk_logic_partition_t bk_flash_partitions[] = BK_FLASH_PARTITIONS_MAP;

struct bk_partition_check_context {
    bk_logic_partition_t *partition_info;
    uint32_t start_addr;
    uint32_t length;
    bool need_encrypt;
};

uint32_t pt_get_partition_count(void)
{
    return ARRAY_SIZE(bk_flash_partitions);
}

static bool pt_partition_id_is_valid(bk_partition_t partition)
{
    if ((partition >= BK_PARTITION_BOOTLOADER)
        && (partition < (bk_partition_t)pt_get_partition_count())) {
        return true;
    } else {
        return false;
    }
}

static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static uint32_t piece_address(uint8_t *array,uint32_t index)
{
    return ((uint32_t)(array[index]) << 24 | (uint32_t)(array[index+1])  << 16 | (uint32_t)(array[index+2])  << 8 | (uint32_t)((array[index+3])));
}

static uint16_t short_address(uint8_t *array,uint16_t index)
{
    return ((uint16_t)(array[index]) << 8 | (uint16_t)(array[index+1]));
}

int toHex(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    } else {
        return 0;
    }
}

uint8_t toHexByte(const char* c)
{
    return 16 * toHex(c[0]) + toHex(c[1]);
}

void toHexStream(const char* src, uint8_t* dest, uint32_t* dest_len)
{
    uint32_t cnt = 0;
    const char* p = src;
    while (*p != '\0' && *(p + 1) != '\0') {
        dest[cnt++] = toHexByte(p);
        p += 2;
    }
    *dest_len = cnt;
}

void generate_iv(uint8_t *iv, size_t unit_num)
{
    memset(iv, 0, 16);
    for (int k = 0; k < 16 && k < DATAUNIT_SIZE; k++) {
        if (8 * k >= 32) continue;
        iv[15 - k] = (unit_num >> (8 * k)) & 0xFF;
    }
    for (int j = 0; j < 8; j++) {
        u8 temp = iv[j];
        iv[j] = iv[15 - j];
        iv[15 - j] = temp;
    }
}

static void get_partition_name(bk_partition_t partition, char *name, size_t name_len)
{
    const bk_logic_partition_t *logic_pt_item;
    uint32_t is_valid_id = pt_partition_id_is_valid(partition);

    if (is_valid_id && name && (name_len > 0)) {
        logic_pt_item = &bk_flash_partitions[partition];
        if (NULL != logic_pt_item->partition_description) {
            os_strlcpy(name, logic_pt_item->partition_description, name_len);
            return;
        }
    }
    os_strlcpy(name, "unknown", name_len);
}

const bk_logic_partition_t * pt_get_partition_info(bk_partition_t partition)
{
    const bk_logic_partition_t *logic_pt_item = NULL;
    uint32_t is_valid_id = pt_partition_id_is_valid(partition);

    if (is_valid_id) {
        logic_pt_item = &bk_flash_partitions[partition];
    }

    return logic_pt_item;
}

const bk_logic_partition_t * get_partition_info_by_name(const char *label)
{
    const bk_logic_partition_t *pt = NULL;

    for (size_t i = 0; i < pt_get_partition_count(); ++i) {
        /* if null pointer, maybe memfault will be triggered */
        if ((label)
            && (bk_flash_partitions[i].partition_description)
            && strcmp(bk_flash_partitions[i].partition_description, label) == 0) {
                pt = &bk_flash_partitions[i];
                break;
        }
    }

    return pt;
}

uint32_t pt_get_partition_index(const bk_logic_partition_t* partition)
{
    uint32_t iRet = 0;
    for (size_t i = 0; i < pt_get_partition_count(); ++i) {
        if ((NULL != partition->partition_description)
            && (NULL != bk_flash_partitions[i].partition_description)
            && strcmp(partition->partition_description, bk_flash_partitions[i].partition_description) == 0) {
            iRet = i;
            break;
        }
    }
    return iRet;
}

static bool partition_is_encrypt(bk_logic_partition_t *partition_info)
{
    uint8_t* buf = NULL;
    uint32_t partition_start, i, j;
    static bool isInitialized = false;

    if (!isInitialized) {
        buf = (uint8_t*)malloc(PARTITION_ENTRY_LEN * sizeof(uint8_t));
        if (buf == NULL) {
            BK_LOGE(TAG, "memory malloc fails.\r\n");
            goto is_encrypt_exit;
        }

#if CONFIG_SUPPORT_PARTITIONS_PARTITION
        partition_start = PARTITION_PARTITION_PHY_OFFSET + PARTITION_PPC_OFFSET;
#else
        /* TODO: support partition encryption */
        BK_LOGE(TAG, "partition encryption not supported.\r\n");
        goto is_encrypt_exit;
#endif
        for (i = 0; i < PARTITION_AMOUNT; ++i) {
            bk_flash_read_bytes(partition_start + PARTITION_ENTRY_LEN * i, buf, PARTITION_ENTRY_LEN);

            if (is_alpha(buf[0]) == 0) {
                break;
            }

            for (j = 0; j < PARTITION_NAME_LEN; ++j) {
                if (buf[j] == 0xFF) {
                    break;
                }
                cachedPartitions[i].name[j] = buf[j];
            }
            cachedPartitions[i].name[j] = '\0';
            cachedPartitions[i].phy_flags = short_address(buf, PARTITION_FLAGS_OFFSET);
        }

        free(buf);
        buf = NULL;

        isInitialized = true;
    }

    if (partition_info && partition_info->partition_description) {
        for (uint32_t i = 0; i < PARTITION_AMOUNT; ++i) {
            if (cachedPartitions[i].name[0] == '\0') {
                continue;
            }
            if (strcmp(partition_info->partition_description, cachedPartitions[i].name) == 0) {
                return (cachedPartitions[i].phy_flags & 1) != 0;
            }
        }
    }

is_encrypt_exit:
    if (buf) {
        free(buf);
        buf = NULL;
    }

    return false;
}

static bool bk_partition_build_context(bk_partition_t partition, struct bk_partition_check_context *ctx)
{
    if (!ctx) {
        return false;
    }

    if (!pt_partition_id_is_valid(partition)) {
        return false;
    }

    ctx->partition_info = bk_flash_partition_get_info(partition);
    if (ctx->partition_info == NULL) {
        return false;
    }

    ctx->start_addr = ctx->partition_info->partition_start_addr;
    ctx->length = ctx->partition_info->partition_length;
#if CONFIG_PARTITION_ENCRYPTION
    ctx->need_encrypt = partition_is_encrypt(ctx->partition_info);
#endif

    return true;
}

static bk_logic_partition_t * flash_partition_get_info_by_addr(uint32_t addr)
{
    const bk_logic_partition_t *pt;
    uint32_t target_addr = addr;
    uint32_t start, length;
    size_t partition_count = pt_get_partition_count();

    for (size_t i = 0; i < partition_count; ++i) {
        pt = &bk_flash_partitions[i];
        start = pt->partition_start_addr;
        length = pt->partition_length;

        if (length == 0) {
            continue;
        }

        uint64_t end = start + length;
        if (end <= start) {
            continue;
        }

        if (target_addr >= start && target_addr < end) {
            return (bk_logic_partition_t *)pt;
        }
    }

    return NULL;
}

bk_logic_partition_t *bk_flash_partition_get_info(bk_partition_t partition)
{
    bk_logic_partition_t *pt = NULL;
    uint32_t is_valid_id = pt_partition_id_is_valid(partition);

    if (is_valid_id) {
        pt = (bk_logic_partition_t*) pt_get_partition_info(partition);
    }

    return pt;
}

#define APP_PARTITION_NAME    "application"
static bk_err_t flash_partition_addr_check(const bk_logic_partition_t *partition_info, uint32_t offset, uint32_t size)
{
#if CONFIG_OTA_POSITION_INDEPENDENT_AB
    if(strcmp(partition_info->partition_description, APP_PARTITION_NAME) == 0)
    {
        return BK_OK;
    }
#endif

    if ( (offset >= partition_info->partition_length)
        || (size > partition_info->partition_length)
        || (offset + size > partition_info->partition_length) )
    {
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    {
        uint32_t flash_sz = bk_flash_get_current_total_size();

        if (flash_sz != 0) {
            uint64_t span_end = (uint64_t)partition_info->partition_start_addr
                + (uint64_t)offset + (uint64_t)size;

            if (span_end > (uint64_t)flash_sz) {
                return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
            }
        }
    }


    return BK_OK;
}

/*********************************************
 *  this function MUST reside in the flash.
 *  it will use itself address to check partition write permission.
 */
static bk_err_t flash_partition_write_perm_check(const bk_logic_partition_t *partition_info)
{
    if((partition_info->partition_options & PAR_OPT_WRITE_EN) == 0)
        return BK_FAIL;

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
    return BK_OK;
#else
    // flash ctrl only can read/write 16MB.
    uint32_t   fun_flash_logical_addr = ((uint32_t)flash_partition_write_perm_check) & (FLASH_MAX_SIZE - 1) ;
    uint32_t   fun_flash_phy_addr = FLASH_LOGICAL_2_PHY(fun_flash_logical_addr);

    if(fun_flash_phy_addr < partition_info->partition_start_addr)
    {
        return BK_OK;  // not write current running partition.
    }

    if(fun_flash_phy_addr > (partition_info->partition_start_addr + partition_info->partition_length))
    {
        return BK_OK;  // not write current running partition.
    }

    return BK_FAIL;  // not permit to write current running partition.
#endif
}

static bk_err_t bk_flash_partition_erase_internal(const bk_logic_partition_t *partition_info, uint32_t offset, uint32_t size)
{
    if (size == 0)
        return BK_OK;

    uint32_t erase_addr = 0;
    uint32_t start_sector, end_sector = 0;

    start_sector = offset >> FLASH_SECTOR_SIZE_OFFSET; /* offset / FLASH_SECTOR_SIZE */
    end_sector = (offset + size - 1) >> FLASH_SECTOR_SIZE_OFFSET;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        bk_err_t ret;

        erase_addr = partition_info->partition_start_addr + (i << FLASH_SECTOR_SIZE_OFFSET);

    #if CONFIG_FLASH_V1P1
        GLOBAL_INT_DISABLE();
    #endif

        if (partition_info->partition_start_addr <= FLASH_PROTECT_FREE_END_ADDR){
            bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        }

        ret = bk_flash_erase_sector(erase_addr);

        if (partition_info->partition_start_addr <= FLASH_PROTECT_FREE_END_ADDR){
            bk_flash_set_protect_type(FLASH_PROTECT_ALL);
        }
    #if CONFIG_FLASH_V1P1
        GLOBAL_INT_RESTORE();
    #endif

        if (ret != BK_OK) {
            return ret;
        }
    }


    return BK_OK;
}

bk_err_t bk_flash_partition_erase(bk_partition_t partition, uint32_t offset, uint32_t size)
{
    struct bk_partition_check_context ctx = { 0 };
    if (!bk_partition_build_context(partition, &ctx)) {
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
    bk_logic_partition_t *partition_info = ctx.partition_info;
    if (flash_partition_addr_check(partition_info, offset, size) != BK_OK )
    {
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }
    if (flash_partition_write_perm_check(partition_info) != BK_OK )
    {
        return BK_FAIL;
    }


    return bk_flash_partition_erase_internal(partition_info, offset, size);
}

static bk_err_t bk_flash_partition_write_internal(const bk_logic_partition_t *partition_info, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(buffer);

    uint32_t start_addr;
    bk_err_t ret = BK_OK;
    GLOBAL_INT_DECLARATION();

    start_addr = partition_info->partition_start_addr + offset;
    if ((offset + buffer_len) > partition_info->partition_length) {
        FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

#if (CONFIG_PARTITION_ENCRYPTION)
    uint8_t *dest_hex = os_malloc(buffer_len);
    if (dest_hex == NULL) {
        FLASH_LOGW("%s malloc failed\r\n", __func__);
        ret = BK_ERR_NO_MEM;
        goto write_internal_exit;
    }

    os_memcpy(dest_hex, buffer, buffer_len);

    if (partition_is_encrypt(partition_info)) {
        uint8_t eky_hex[2 * NVS_KEY_SIZE];
        uint8_t ptxt_hex[32], ctxt_hex[32], TweakValue[16];
        ret = BK_FAIL;

        mbedtls_aes_xts_context ectx[1];
        mbedtls_aes_xts_init(ectx);

#if CONFIG_NVS_ENCRYPTION
        nvs_sec_cfg_t cfg = {{0x72, 0x36}, {0x20, 0x24}};
        ret = nvs_flash_read_cfg_by_partId(BK_PARTITION_NVS_KEY, &cfg);
        if (ret == ESP_ERR_NVS_KEYS_NOT_INITIALIZED) {
            BK_LOGI(TAG, "NVS key partition empty, generating keys");
            const bk_logic_partition_t *key_part = bk_partition_find_first(
                ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NVS_KEY_PARTITION_LABEL);
            if (key_part == NULL) {
                BK_LOGE(TAG, "CONFIG_NVS_ENCRYPTION is enabled, but no partition with subtype nvs_keys found in the partition table.");
                ret = ESP_ERR_NOT_FOUND;
                goto write_internal_exit;
            }

            ret = nvs_flash_generate_keys(key_part, &cfg);
            if (ret != BK_OK) {
                BK_LOGE(TAG, "Failed to generate keys: [0x%02X]", ret);
                goto write_internal_exit;
            }
        } else if (ret != BK_OK) {
            BK_LOGE(TAG, "Failed to read NVS security cfg: [0x%02X]", ret);
            goto write_internal_exit;
        }

        os_memcpy(eky_hex, cfg.eky, NVS_KEY_SIZE);
        os_memcpy(eky_hex + NVS_KEY_SIZE, cfg.tky, NVS_KEY_SIZE);
#else
        uint32_t byteArrayLen = 0;
        toHexStream(eky, eky_hex, &byteArrayLen);
#endif // CONFIG_NVS_ENCRYPTION

        mbedtls_aes_xts_setkey_enc(ectx, eky_hex, 2 * NVS_KEY_SIZE * 8);

        for (size_t i = 0; i < buffer_len; i += DATAUNIT_SIZE) {
            os_memset(TweakValue, 0, 16);
            os_memset(ptxt_hex, 0xff, 32);

            generate_iv(TweakValue, (i + offset) / DATAUNIT_SIZE);

            const uint8_t* tab_addr = &buffer[i];

            if ((i + DATAUNIT_SIZE) < buffer_len)
                os_memcpy(ptxt_hex, tab_addr, DATAUNIT_SIZE);
            else {
                // last copy
                uint32_t len = buffer_len - i;
                os_memcpy(ptxt_hex, tab_addr, len);
            }

            ret = mbedtls_aes_crypt_xts(ectx, MBEDTLS_AES_ENCRYPT, 32, TweakValue, ptxt_hex, ctxt_hex);
            if (ret != BK_OK) {
                mbedtls_aes_xts_free(ectx);
                BK_LOGE(TAG, "Failed to mbedtls_aes_crypt_xts_encrypt: [0x%02X]", ret);
                goto write_internal_exit;
            }

            if ((i + DATAUNIT_SIZE) < buffer_len)
                os_memcpy(&dest_hex[i], ctxt_hex, DATAUNIT_SIZE);
            else {
                // last copy
                uint32_t len = buffer_len - i;
                os_memcpy(&dest_hex[i], ctxt_hex, len);
            }
        }
        mbedtls_aes_xts_free(ectx);
    }
#endif

    GLOBAL_INT_DISABLE();

    if (partition_info->partition_start_addr <= FLASH_PROTECT_FREE_END_ADDR)
    {
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    }
    if ((offset + buffer_len) <= partition_info->partition_length) {
#if (CONFIG_PARTITION_ENCRYPTION)
        bk_flash_write_bytes(start_addr, dest_hex, buffer_len);
#else
        bk_flash_write_bytes(start_addr, buffer, buffer_len);
#endif
    }

    if (partition_info->partition_start_addr <= FLASH_PROTECT_FREE_END_ADDR)
    {
        bk_flash_set_protect_type(FLASH_PROTECT_ALL);
    }
    GLOBAL_INT_RESTORE();

#if (CONFIG_PARTITION_ENCRYPTION)
write_internal_exit:
    if (dest_hex) {
        os_free(dest_hex);
        dest_hex = NULL;
    }
#endif

    return ret;
}

bk_err_t bk_flash_partition_write(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    struct bk_partition_check_context ctx = { 0 };
    if (!bk_partition_build_context(partition, &ctx)) {
        FLASH_LOGW("%s partition not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
    bk_logic_partition_t *partition_info = ctx.partition_info;
    if (flash_partition_addr_check(partition_info, offset, buffer_len) != BK_OK )
    {
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }
    if (flash_partition_write_perm_check(partition_info) != BK_OK )
    {
        return BK_FAIL;
    }

    return bk_flash_partition_write_internal(partition_info, buffer, offset, buffer_len);
}

static bk_err_t bk_flash_partition_read_internal(const bk_logic_partition_t *partition_info, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(out_buffer);

    uint32_t start_addr;
    bk_err_t ret = BK_OK;
    GLOBAL_INT_DECLARATION();

    start_addr = partition_info->partition_start_addr + offset;
    if ((offset + buffer_len) > partition_info->partition_length) {
        FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    GLOBAL_INT_DISABLE();
    bk_flash_read_bytes(start_addr, out_buffer, buffer_len);
    GLOBAL_INT_RESTORE();

#if (CONFIG_PARTITION_ENCRYPTION)
    if (0 == partition_is_encrypt(partition_info)) {
        goto read_internal_exit;
    }

    uint8_t eky_hex[2 * NVS_KEY_SIZE];
    uint8_t ptxt_hex[32], ctxt_hex[32], TweakValue[16];
    ret = BK_FAIL;

    mbedtls_aes_xts_context dctx[1];

    uint8_t* dest_hex = os_malloc(buffer_len);
    if (dest_hex == NULL) {
        FLASH_LOGW("%s malloc failed\r\n", __func__);
        ret = BK_ERR_NO_MEM;
        goto read_internal_exit;
    }

    os_memset(dest_hex, 0xff, buffer_len);
    mbedtls_aes_xts_init(dctx);

#if CONFIG_NVS_ENCRYPTION
    nvs_sec_cfg_t cfg = {{0x72, 0x36}, {0x20, 0x24}};
    ret = nvs_flash_read_cfg_by_partId(BK_PARTITION_NVS_KEY, &cfg);
    if (ret == ESP_ERR_NVS_KEYS_NOT_INITIALIZED) {
        BK_LOGI(TAG, "NVS key partition empty, generating keys");
        const bk_logic_partition_t *key_part = bk_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NVS_KEY_PARTITION_LABEL);
        if (key_part == NULL) {
            BK_LOGE(TAG, "CONFIG_NVS_ENCRYPTION is enabled, but no partition with subtype nvs_keys found in the partition table.");
            ret = ESP_ERR_NOT_FOUND;
            goto read_internal_exit;
        }

        ret = nvs_flash_generate_keys(key_part, &cfg);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "Failed to generate keys: [0x%02X]", ret);
            goto read_internal_exit;
        }
    } else if (ret != BK_OK) {
        BK_LOGE(TAG, "Failed to read NVS security cfg: [0x%02X]", ret);
        goto read_internal_exit;
    }

    os_memcpy(eky_hex, cfg.eky, NVS_KEY_SIZE);
    os_memcpy(eky_hex + NVS_KEY_SIZE, cfg.tky, NVS_KEY_SIZE);
#else
    uint32_t byteArrayLen = 0;
    toHexStream(eky, eky_hex, &byteArrayLen);
#endif // CONFIG_NVS_ENCRYPTION

    mbedtls_aes_xts_setkey_dec(dctx, eky_hex, 2 * NVS_KEY_SIZE * 8);

    for (size_t i = 0; i < buffer_len; i += DATAUNIT_SIZE) {
        os_memset(TweakValue, 0, 16);
        generate_iv(TweakValue, (i + offset) / DATAUNIT_SIZE);

        uint8_t* tab_addr = &out_buffer[i];

        if ((i + DATAUNIT_SIZE) < buffer_len)
            os_memcpy(ctxt_hex, tab_addr, DATAUNIT_SIZE);
        else {
            // last copy
            uint32_t len = buffer_len - i;
            os_memcpy(ctxt_hex, tab_addr, len);
        }

        ret = mbedtls_aes_crypt_xts(dctx, MBEDTLS_AES_DECRYPT, 32, TweakValue, ctxt_hex, ptxt_hex);
        if (ret != BK_OK) {
            mbedtls_aes_xts_free(dctx);
            BK_LOGE(TAG, "Failed to mbedtls_aes_crypt_xts_decrypt: [0x%02X]", ret);
            goto read_internal_exit;
        }

        if ((i + DATAUNIT_SIZE) < buffer_len)
            os_memcpy(&dest_hex[i], ptxt_hex, DATAUNIT_SIZE);
        else {
            // last copy
            uint32_t len = buffer_len - i;
            os_memcpy(&dest_hex[i], ptxt_hex, len);
        }
    }
    mbedtls_aes_xts_free(dctx);

    os_memset(out_buffer, 0xff, buffer_len);
    os_memcpy(out_buffer, dest_hex, buffer_len);

read_internal_exit:
    if (dest_hex) {
        os_free(dest_hex);
        dest_hex = NULL;
    }
#endif

    return ret;
}

bk_err_t bk_flash_partition_read(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
    struct bk_partition_check_context ctx = { 0 };
    if (!bk_partition_build_context(partition, &ctx)) {
        FLASH_LOGW("%s partition not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
    bk_logic_partition_t *partition_info = ctx.partition_info;
    if (flash_partition_addr_check(partition_info, offset, buffer_len) != BK_OK )
    {
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    uint32_t aligned_offset = offset & ~0x1F;
    uint32_t aligned_end = (offset + buffer_len + 31) & ~0x1F;

    uint32_t aligned_length = aligned_end - aligned_offset;
    uint8_t *aligned_buffer = (uint8_t *)malloc(aligned_length);
    if (aligned_buffer == NULL) {
        return BK_ERR_NO_MEM;
    }

    bk_err_t read_status = bk_flash_partition_read_internal(partition_info, aligned_buffer, aligned_offset, aligned_length);

    if (read_status != BK_OK) {
        free(aligned_buffer);
        return read_status;
    }
    memcpy(out_buffer, aligned_buffer + (offset - aligned_offset), buffer_len);
    free(aligned_buffer);

    return BK_OK;
}

bk_err_t bk_flash_partition_erase_all(const char *label)
{
    const bk_logic_partition_t *partition_info = get_partition_info_by_name(label);

    if (partition_info == NULL) {
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }

    if (flash_partition_write_perm_check(partition_info) != BK_OK ) {
        return BK_FAIL;
    }

    return bk_flash_partition_erase_internal(partition_info, 0, partition_info->partition_length);
}

bk_err_t bk_flash_partition_erase_by_name(const char* label, uint32_t offset, uint32_t size)
{
    const bk_logic_partition_t *partition_info = get_partition_info_by_name(label);
    if (partition_info == NULL) {
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
    if (flash_partition_addr_check(partition_info, offset, size) != BK_OK )
    {
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }
    if (flash_partition_write_perm_check(partition_info) != BK_OK )
    {
        return BK_FAIL;
    }

    return bk_flash_partition_erase_internal(partition_info, offset, size);
}

bk_err_t bk_flash_partition_write_by_name(const char *label, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    const bk_logic_partition_t *partition_info = get_partition_info_by_name(label);

    if (NULL == partition_info) {
        FLASH_LOGW("%s partition not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
    if (flash_partition_addr_check(partition_info, offset, buffer_len) != BK_OK )
    {
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }
    if (flash_partition_write_perm_check(partition_info) != BK_OK )
    {
        return BK_FAIL;
    }

    return bk_flash_partition_write_internal(partition_info, buffer, offset, buffer_len);
}

bk_err_t bk_flash_partition_read_by_name(const char *label, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
    const bk_logic_partition_t *partition_info = get_partition_info_by_name(label);

    if (NULL == partition_info) {
        FLASH_LOGW("%s partition not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
    if (flash_partition_addr_check(partition_info, offset, buffer_len) != BK_OK )
    {
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    uint32_t aligned_offset = offset & ~0x1F;
    uint32_t aligned_end = (offset + buffer_len + 31) & ~0x1F;

    uint32_t aligned_length = aligned_end - aligned_offset;
    uint8_t *aligned_buffer = (uint8_t *)malloc(aligned_length);
    if (aligned_buffer == NULL) {
        return BK_ERR_NO_MEM;
    }

    bk_err_t read_status = bk_flash_partition_read_internal(partition_info, aligned_buffer, aligned_offset, aligned_length);

    if (read_status != BK_OK) {
        free(aligned_buffer);
        return read_status;
    }
    memcpy(out_buffer, aligned_buffer + (offset - aligned_offset), buffer_len);
    free(aligned_buffer);

    return BK_OK;
}

extern void * bk_memcpy_4w(void *dst, const void *src, unsigned int size);

bk_err_t bk_flash_partition_read_cbus(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(out_buffer);
    struct bk_partition_check_context ctx = { 0 };
    if (!bk_partition_build_context(partition, &ctx)) {
        FLASH_LOGW("%s partiion not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
    bk_logic_partition_t *partition_info = ctx.partition_info;

    uint32_t in_ptr;
    uint32_t start_addr, flash_base;

    flash_base = FLASH_LOGICAL_BASE_ADDR;
    start_addr = partition_info->partition_start_addr;
    if(!FLASH_PHY_ADDR_VALID(start_addr))
        return BK_FAIL;

    int ret = flash_partition_addr_check(partition_info, FLASH_LOGICAL_2_PHY(offset), FLASH_LOGICAL_2_PHY(buffer_len));

    if(ret != BK_OK)
        return ret;

    in_ptr = flash_base + FLASH_PHY_2_LOGICAL(start_addr) + offset;

    memcpy(out_buffer, (void *)in_ptr, buffer_len);

    return BK_OK;
}

bk_err_t bk_flash_partition_write_cbus(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(buffer);
    struct bk_partition_check_context ctx = { 0 };
    if (!bk_partition_build_context(partition, &ctx)) {
        FLASH_LOGW("%s partition not found\r\n", __func__);
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }

    uint32_t wr_ptr;
    uint32_t start_addr, flash_base;
    bk_logic_partition_t *partition_info = ctx.partition_info;

    GLOBAL_INT_DECLARATION();
    FLASH_LOGI("bk_flash_partition_write_cbus:0x%x\r\n", buffer_len);

    flash_base = SOC_FLASH_DATA_BASE;
    start_addr = partition_info->partition_start_addr;
    wr_ptr = flash_base + FLASH_PHY_2_LOGICAL(start_addr) + offset;
    if((offset + buffer_len) > partition_info->partition_length) {
        FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    GLOBAL_INT_DISABLE();

    flash_protect_type_t  partition_type = bk_flash_get_protect_type();
    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    if((offset + buffer_len) <= partition_info->partition_length) {
        bk_memcpy_4w((void *)wr_ptr, buffer, buffer_len);
    }
    bk_flash_set_protect_type(partition_type);

    GLOBAL_INT_RESTORE();

    return BK_OK;
}

bk_err_t bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code)
{
#if CONFIG_BK_OTA
    //TODO
    return BK_OK;
#endif

#if !CONFIG_FLASH_NO_PARTITION
    if(magic_code != FLASH_API_MAGIC_CODE)
        return BK_FAIL;

#if (!CONFIG_SPE) && CONFIG_TFM
    //TODO fix me by add permission in partitions.csv of secure project
    return BK_OK;
#else
    bk_logic_partition_t *partition_info = flash_partition_get_info_by_addr(addr);

    if (NULL == partition_info) {
        return BK_FAIL;
    }

    uint32_t   offset = addr - partition_info->partition_start_addr;
    bk_err_t   ret_val = flash_partition_addr_check(partition_info, offset, size);
    if(ret_val != BK_OK)
        return ret_val;

    return flash_partition_write_perm_check(partition_info);
#endif
#endif
#if CONFIG_FLASH_NO_PARTITION
return BK_OK;
#endif
}

const bk_logic_partition_t *bk_flash_partition_get_info_by_name(const char *label)
{
    return get_partition_info_by_name(label);
}
