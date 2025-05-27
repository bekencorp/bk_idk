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

#include <stdlib.h>
#include <string.h>
#include <os/mem.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_driver.h"
#include "flash_hal.h"

#if CONFIG_OVERRIDE_FLASH_PARTITION
#include "vendor_flash_partition.h"
#endif

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX)
#include "partitions_gen.h"
#endif

#include <ctype.h>
//#if (CONFIG_PSA_MBEDTLS) || (CONFIG_MBEDTLS_ACCELERATOR) || (CONFIG_MBEDTLS)
//#include "mbedtls/aes.h"
//#endif

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

#if CONFIG_TFM_READ_FLASH_NSC
#if (CONFIG_TFM_FWU)
#include "tfm_flash_nsc.h"
#endif

#if CONFIG_NVS_ENCRYPTION
/**/
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

#endif

enum {
	PARTITION_PRIMARY_ALL = 0,
	PARTITION_SECONDARY_ALL,
	PARTITION_OTA,
	PARTITION_PARTITION,
	PARTITION_SPE,
	PARTITION_TFM_NS,
	PARTITION_NSPE,
	PARTITION_OTP_NV,
	PARTITION_PS,
	PARTITION_ITS,
	PARTITION_CNT,
};

const char *s_partition_name[PARTITION_CNT] = {
	// "sys_rf",
	// "sys_net",
	// "easyflash",
	// "primary_tfm_s",
	// "primary_cpu0_app",
	// "ota",
	// "user_config"
		"primary_all",
	"secondary_all",
	"ota",
	"partition",
	"primary_tfm_s",
	"primary_tfm_ns",
	"primary_cpu0_app",
	"sys_otp_nv",
	"sys_ps",
	"sys_its"
};
#if (CONFIG_TFM_READ_FLASH_NSC)
	#define PARTITION_PARTITION_PHY_OFFSET   CONFIG_PARTITION_PHY_PARTITION_OFFSET
	#define PARTITION_PPC_OFFSET             0x400
	#define PARTITION_NAME_LEN               20
	#define PARTITION_ENTRY_LEN              32
	#define PARTITION_OFFSET_OFFSET          22
	#define PARTITION_SIZE_OFFSET            26
	#define PARTITION_FLAGS_OFFSET           30

	typedef struct {
		uint32_t phy_offset;
		uint32_t phy_size;
		uint32_t phy_flags;
	} partition_config_t;

	typedef struct {
		bk_partition_t partition;
		const char *name;
	} partition_map_t;

	typedef struct {
		const char *name;
		uint32_t offset;
		uint32_t size;
	} partition_info_t;

	const partition_info_t partition_map_by_gen[] = PARTITION_MAP;

	bk_logic_partition_t logic_partitions[sizeof(partition_map_by_gen) / sizeof(partition_map_by_gen[0])];
	static int is_initialized = 0;

    static struct {
        char name[PARTITION_NAME_LEN + 1];
        uint32_t phy_flags;
    } cachedPartitions[PARTITION_AMOUNT];



const partition_map_t partition_map[] = {
    {BK_PARTITION_BOOTLOADER, "bl2"},
    {BK_PARTITION_APPLICATION, "primary_cpu0_app"},
    {BK_PARTITION_OTA, "ota"},
    {BK_PARTITION_APPLICATION1, "primary_cpu0_app1"},
    {BK_PARTITION_MATTER_FLASH, "matter"},
    {BK_PARTITION_RF_FIRMWARE, "sys_rf"},
    {BK_PARTITION_NET_PARAM, "sys_net"},
    {BK_PARTITION_USR_CONFIG, "user_config"},
    {BK_PARTITION_OTA_FINA_EXECUTIVE, "ota2"},
    {BK_PARTITION_APPLICATION2, "primary_cpu0_app2"},
    {BK_PARTITION_EASYFLASH, "easyflash"},
    {BK_PARTITION_NVS, "nvs"},
    {BK_PARTITION_NVS_KEY, "nvs_key"},
    {BK_PARTITION_WIZ_MFR, "wiz_mfr"},
};

const size_t partition_map_size = sizeof(partition_map) / sizeof(partition_map[0]);

#endif

#if CONFIG_FLASH_ORIGIN_API
#define PAR_OPT_READ_POS      (0)
#define PAR_OPT_WRITE_POS     (1)

#define PAR_OPT_READ_DIS      (0x0u << PAR_OPT_READ_POS)
#define PAR_OPT_READ_EN       (0x1u << PAR_OPT_READ_POS)
#define PAR_OPT_WRITE_DIS     (0x0u << PAR_OPT_WRITE_POS)
#define PAR_OPT_WRITE_EN      (0x1u << PAR_OPT_WRITE_POS)
#endif

#define PARTITION_IRAM         __attribute__((section(".iram")))

/// TODO: use bk_flash_partitions name for all, every soc can define self config at:
///             middleware/boards/bk7256/vnd_flash/vnd_flash.c
/// Custom can override bk_flash_partitions in project, For example:
///             projects/customization/bk7256_config2/main/vendor_flash.c
/* Logic partition on flash devices */
#if (CONFIG_SOC_BK7256XX)
#if CONFIG_OVERRIDE_FLASH_PARTITION
extern const bk_logic_partition_t bk_flash_partitions[BK_PARTITION_MAX_USER];
#else
extern const bk_logic_partition_t bk_flash_partitions[BK_PARTITION_MAX];
#endif
#elif (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX)
#if CONFIG_OVERRIDE_FLASH_PARTITION
extern const bk_logic_partition_t bk_flash_partitions[BK_PARTITION_MAX_USER];
#else
extern const bk_logic_partition_t bk_flash_partitions[BK_PARTITION_MAX];
#endif
#else
#include "partitions.h"
static const bk_logic_partition_t bk_flash_partitions[BK_PARTITION_MAX] = {
	[BK_PARTITION_BOOTLOADER] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "Bootloader",
		.partition_start_addr      = 0x00000000,
		.partition_length          = 0x0F000,
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
	[BK_PARTITION_APPLICATION] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "Primary Application",
		.partition_start_addr      = 0x11000,
#if CONFIG_SUPPORT_MATTER	|| 	 CONFIG_FLASH_SIZE_4M
		.partition_length          = 0x1A9000,
#else
		.partition_length          = 0x143000,
#endif
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
	[BK_PARTITION_OTA] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "ota",
#if CONFIG_FLASH_SIZE_4M
		.partition_start_addr      = 0x1BA000,
		.partition_length          = 0x1A9000, //1700KB
#elif CONFIG_SUPPORT_MATTER
		.partition_start_addr      = 0x1BA000,
		.partition_length          = 0x11000, //68KB
#else
		.partition_start_addr      = 0x132000,
		.partition_length          = 0xAE000, //696KB
#endif
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
#if CONFIG_SUPPORT_MATTER
	[BK_PARTITION_MATTER_FLASH] =
	{
		.partition_owner		   = BK_FLASH_EMBEDDED,
		.partition_description	   = "Matter",
		#if CONFIG_FLASH_SIZE_4M
		.partition_start_addr	   = 0x363000,
		#else
		partition_start_addr	   = 0x1CB000,
		#endif
		.partition_length		   = 0x15000, //84KB
		.partition_options		   = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
#endif
	[BK_PARTITION_RF_FIRMWARE] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "RF Firmware",
#if (CONFIG_FLASH_SIZE_4M)
		.partition_start_addr      = 0x3FE000,
#else
		.partition_start_addr      = CONFIG_SYS_RF_PHY_PARTITION_OFFSET,// for rf related info
#endif
		.partition_length          = CONFIG_SYS_RF_PHY_PARTITION_SIZE,
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
	[BK_PARTITION_NET_PARAM] =
	{
		.partition_owner           = BK_FLASH_EMBEDDED,
		.partition_description     = "NET info",
#if (CONFIG_FLASH_SIZE_4M)
		.partition_start_addr      = 0x3FF000,
#else
		.partition_start_addr      = CONFIG_SYS_NET_PHY_PARTITION_OFFSET,// for net related info
#endif
		.partition_length          = CONFIG_SYS_NET_PHY_PARTITION_SIZE,
		.partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
};
#endif


static bool flash_partition_is_valid(bk_partition_t partition)
{
	if ((partition >= BK_PARTITION_BOOTLOADER)
		&& (partition < ARRAY_SIZE(bk_flash_partitions))) {
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

int toHex(char ch) {
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

uint8_t toHexByte(const char* c) {
	return 16 * toHex(c[0]) + toHex(c[1]);
}

void toHexStream(const char* src, uint8_t* dest, uint32_t* dest_len) {
	uint32_t cnt = 0;
	const char* p = src;
	while (*p != '\0' && *(p + 1) != '\0') {
		dest[cnt++] = toHexByte(p);
		p += 2;
	}
	*dest_len = cnt;
}

void generate_iv(uint8_t *iv, size_t unit_num) {
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


#if CONFIG_TFM_READ_FLASH_NSC

void initialize_logic_partitions() {
    if (is_initialized) return;

    for (size_t i = 0; i < sizeof(partition_map_by_gen) / sizeof(partition_map_by_gen[0]); ++i) {
        logic_partitions[i].partition_owner = BK_FLASH_EMBEDDED;
        logic_partitions[i].partition_description = partition_map_by_gen[i].name;
        logic_partitions[i].partition_start_addr = partition_map_by_gen[i].offset;
        logic_partitions[i].partition_length = partition_map_by_gen[i].size;
        logic_partitions[i].partition_options = PAR_OPT_EXECUTE_DIS | PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS;
    }

    is_initialized = 1;
}

static void get_partition_name(bk_partition_t partition, char *name, size_t name_len) {
    for (size_t i = 0; i < sizeof(partition_map) / sizeof(partition_map[0]); ++i) {
        if (partition_map[i].partition == partition) {
            strncpy(name, partition_map[i].name, name_len - 1);
            name[name_len - 1] = '\0';
            return;
        }
    }
    strncpy(name, "unknown", name_len - 1);
    name[name_len - 1] = '\0';
}

bk_logic_partition_t * get_partition_info(bk_partition_t partition) {
    const char *label = NULL;

    for (size_t i = 0; i < sizeof(partition_map) / sizeof(partition_map[0]); ++i) {
        if (partition_map[i].partition == partition) {
            label = partition_map[i].name;
            break;
        }
    }

    if (label) {
        return get_partition_info_by_name(label);
    }

    return NULL;
}

bk_logic_partition_t * get_partition_info_by_name(const char *label) {
	if (!is_initialized) {
		initialize_logic_partitions();
	}

	for (size_t i = 0; i < sizeof(partition_map_by_gen) / sizeof(partition_map_by_gen[0]); ++i) {
		if (strcmp(partition_map_by_gen[i].name, label) == 0) {
			return &logic_partitions[i];
		}
	}
	return NULL;
}

uint32_t get_partition_index(const bk_logic_partition_t* partition) {
    uint32_t iRet = 0;
    for (size_t i = 0; i < partition_map_size; ++i) {
        if (strcmp(partition->partition_description,partition_map[i].name) == 0) {
            iRet = i;
            break;
        }
    }
    return iRet;
}

static bool partitionIsEncrypt(bk_logic_partition_t *partition_info)
{
    static bool isInitialized = false;

    if (!isInitialized) {
        uint8_t* buf = (uint8_t*)malloc(PARTITION_ENTRY_LEN * sizeof(uint8_t));
        if (buf == NULL) {
            BK_LOGE(TAG, "memory malloc fails.\r\n");
            return false;
        }

        uint32_t partition_start = PARTITION_PARTITION_PHY_OFFSET + PARTITION_PPC_OFFSET;

        for (uint32_t i = 0; i < PARTITION_AMOUNT; ++i) {
            #if (CONFIG_TFM_FWU)
                psa_flash_read_bytes(partition_start + PARTITION_ENTRY_LEN * i, buf, PARTITION_ENTRY_LEN);
            #else
                bk_flash_read_bytes(partition_start + PARTITION_ENTRY_LEN * i, buf, PARTITION_ENTRY_LEN);
            #endif

            if (is_alpha(buf[0]) == 0) {
                break;
            }

            int j;
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
        isInitialized = true;
    }

    for (uint32_t i = 0; i < PARTITION_AMOUNT; ++i) {
        if (strcmp(partition_info->partition_description, cachedPartitions[i].name) == 0) {
            return (cachedPartitions[i].phy_flags & 1) != 0;
        }
    }

    return false;
}

#endif

static bk_logic_partition_t * flash_partition_get_info_by_addr(uint32_t addr)
{
	const bk_logic_partition_t *pt;
	
	for(int i = 0; i < ARRAY_SIZE(bk_flash_partitions); i++)
	{
		pt = &bk_flash_partitions[i];
		
		if(addr < pt->partition_start_addr)
			continue;
		if(addr >= (pt->partition_start_addr + pt->partition_length))
			continue;

		return (bk_logic_partition_t *)pt;
	}

	return NULL;
}

bk_logic_partition_t *bk_flash_partition_get_info(bk_partition_t partition)
{
	bk_logic_partition_t *pt = NULL;

	BK_ASSERT(BK_PARTITION_BOOTLOADER < BK_PARTITION_MAX);

	if (flash_partition_is_valid(partition)) {
#if (CONFIG_SOC_BK7256XX)
		pt = (bk_logic_partition_t *)&bk_flash_partitions[partition];
#elif (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX)
#if CONFIG_TFM_READ_FLASH_NSC
		pt = get_partition_info(partition);
#else
		pt = (bk_logic_partition_t *)&bk_flash_partitions[partition];
#endif
#else
		pt = (bk_logic_partition_t *)&bk_flash_partitions[partition];
#endif
	}
	return pt;
}

static bk_err_t flash_partition_addr_check(bk_logic_partition_t *partition_info, uint32_t offset, uint32_t size)
{
#if (CONFIG_FLASH_PARTITION_CHECK_VALID)
	if ( (offset >= partition_info->partition_length) 
		|| (size > partition_info->partition_length)
		|| (offset + size > partition_info->partition_length) )
	{
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
#endif

	return BK_OK;
}

/*********************************************
 *  this function MUST reside in the flash.
 *  it will use itself address to check partition write permission.
 */
static bk_err_t flash_partition_write_perm_check(bk_logic_partition_t *partition_info)
{
#if (CONFIG_FLASH_PARTITION_CHECK_VALID)
	if((partition_info->partition_options & PAR_OPT_WRITE_EN) == 0)
		return BK_FAIL;

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
#else
	return BK_OK;
#endif

}

static bk_err_t bk_flash_partition_erase_internal(bk_logic_partition_t *partition_info, uint32_t offset, uint32_t size)
{
    if (size == 0)
		return BK_OK;

    uint32_t erase_addr = 0;
    uint32_t start_sector, end_sector = 0;

    start_sector = offset >> FLASH_SECTOR_SIZE_OFFSET; /* offset / FLASH_SECTOR_SIZE */
    end_sector = (offset + size - 1) >> FLASH_SECTOR_SIZE_OFFSET;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        erase_addr = partition_info->partition_start_addr + (i << FLASH_SECTOR_SIZE_OFFSET);

        bk_flash_erase_sector(erase_addr);
    }

    return BK_OK;
}

bk_err_t bk_flash_partition_erase(bk_partition_t partition, uint32_t offset, uint32_t size)
{
    bk_logic_partition_t *partition_info = bk_flash_partition_get_info(partition);
	
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

static bk_err_t bk_flash_partition_write_internal(bk_logic_partition_t *partition_info, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(buffer);

    uint32_t start_addr;

    start_addr = partition_info->partition_start_addr + offset;
    if ((offset + buffer_len) > partition_info->partition_length) {
        FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

#if (CONFIG_TFM_READ_FLASH_NSC)
    uint8_t *dest_hex = os_malloc(buffer_len);
    if (dest_hex == NULL) {
        FLASH_LOGW("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(dest_hex, buffer, buffer_len);

    if (partitionIsEncrypt(partition_info)) {
        uint8_t eky_hex[2 * NVS_KEY_SIZE];
        uint8_t ptxt_hex[32], ctxt_hex[32], TweakValue[16];
        bk_err_t ret = BK_FAIL;

        mbedtls_aes_xts_context ectx[1];
        mbedtls_aes_xts_init(ectx);

#if CONFIG_NVS_ENCRYPTION
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
                return ret;
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

    if ((offset + buffer_len) <= partition_info->partition_length) {
#if (CONFIG_TFM_READ_FLASH_NSC)
        bk_flash_write_bytes(start_addr, dest_hex, buffer_len);
#else
        bk_flash_write_bytes(start_addr, buffer, buffer_len);
#endif
    }

#if (CONFIG_TFM_READ_FLASH_NSC)
    if (dest_hex) {
        os_free(dest_hex);
        dest_hex = NULL;
    }
#endif

    return BK_OK;
}

bk_err_t bk_flash_partition_write(bk_partition_t partition, const uint8_t *buffer, uint32_t offset, uint32_t buffer_len)
{
    bk_logic_partition_t *partition_info = bk_flash_partition_get_info(partition);
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

static bk_err_t bk_flash_partition_read_internal(bk_logic_partition_t *partition_info, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
    BK_RETURN_ON_NULL(out_buffer);

    uint32_t start_addr;

    start_addr = partition_info->partition_start_addr + offset;
    if ((offset + buffer_len) > partition_info->partition_length) {
        FLASH_LOGE("partition overlap. offset(%d),len(%d)\r\n", offset, buffer_len);
        return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
    }

    bk_flash_read_bytes(start_addr, out_buffer, buffer_len);

#if (CONFIG_TFM_READ_FLASH_NSC)
    if (partitionIsEncrypt(partition_info)) {
        uint8_t eky_hex[2 * NVS_KEY_SIZE];
        uint8_t ptxt_hex[32], ctxt_hex[32], TweakValue[16];
        bk_err_t ret = BK_FAIL;

        mbedtls_aes_xts_context dctx[1];

        uint8_t* dest_hex = os_malloc(buffer_len);
        if (dest_hex == NULL) {
            FLASH_LOGW("%s malloc failed\r\n", __func__);
            return BK_ERR_NO_MEM;
        }

        os_memset(dest_hex, 0xff, buffer_len);
        mbedtls_aes_xts_init(dctx);

#if CONFIG_NVS_ENCRYPTION
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
                return ret;
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

        if (dest_hex) {
            os_free(dest_hex);
            dest_hex = NULL;
        }
    }
#endif

    return BK_OK;
}

bk_err_t bk_flash_partition_read(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
	bk_logic_partition_t *partition_info = bk_flash_partition_get_info(partition);
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

#if CONFIG_TFM_READ_FLASH_NSC
bk_err_t bk_flash_partition_erase_all(const char *label)
{
	bk_logic_partition_t *partition_info = get_partition_info_by_name(label);
    if (partition_info == NULL) {
        return BK_ERR_FLASH_PARTITION_NOT_FOUND;
    }
	if (flash_partition_write_perm_check(partition_info) != BK_OK )
	{
		return BK_FAIL;
	}

    return bk_flash_partition_erase_internal(partition_info, 0, partition_info->partition_length);
}

bk_err_t bk_flash_partition_erase_by_name(const char* label, uint32_t offset, uint32_t size)
{
    bk_logic_partition_t *partition_info = get_partition_info_by_name(label);
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
    bk_logic_partition_t *partition_info = get_partition_info_by_name(label);

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
    bk_logic_partition_t *partition_info = get_partition_info_by_name(label);

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

#endif

extern void * bk_memcpy_4w(void *dst, const void *src, unsigned int size);

#if CONFIG_FLASH_CBUS_RW
bk_err_t bk_flash_partition_read_cbus(bk_partition_t partition, uint8_t *out_buffer, uint32_t offset, uint32_t buffer_len)
{
	BK_RETURN_ON_NULL(out_buffer);

	uint32_t in_ptr;
	uint32_t start_addr, flash_base;
	bk_logic_partition_t *partition_info;

	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partiion not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

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

	uint32_t wr_ptr;
	uint32_t start_addr, flash_base;
	bk_logic_partition_t *partition_info;

	GLOBAL_INT_DECLARATION();
	FLASH_LOGI("bk_flash_partition_write_enhanced:0x%x\r\n", buffer_len);
	partition_info = bk_flash_partition_get_info(partition);
	if (NULL == partition_info) {
		FLASH_LOGW("%s partition not found\r\n", __func__);
		return BK_ERR_FLASH_PARTITION_NOT_FOUND;
	}

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
#endif

bk_err_t bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code)
{
	if(magic_code != FLASH_API_MAGIC_CODE)
		return BK_FAIL;
	
	bk_logic_partition_t *partition_info = flash_partition_get_info_by_addr(addr);

	if (NULL == partition_info) {
		return BK_FAIL;
	}

	uint32_t   offset = addr - partition_info->partition_start_addr;

	bk_err_t   ret_val = flash_partition_addr_check(partition_info, offset, size);

	if(ret_val != BK_OK)
		return ret_val;

	return flash_partition_write_perm_check(partition_info);
}

