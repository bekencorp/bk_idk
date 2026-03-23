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

#if CONFIG_PSA_MBEDTLS
#include <mbedtls/sha256.h>
#include "mbedtls/ecdsa.h"
#include "mbedtls/oid.h"
#include "mbedtls/asn1.h"
#endif
#include <driver/otp.h>
#define BK_ERR_OTA_VALIDATE_TLV_FAIL                    (BK_ERR_OTA_BASE - 6)
#define BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL               (BK_ERR_OTA_BASE - 7)
#define BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL            (BK_ERR_OTA_BASE - 8)
#define BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL                (BK_ERR_OTA_BASE - 9)
#define BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL              (BK_ERR_OTA_BASE - 10)
#define BK_ERR_OTA_CBUS_READ_CRC_FAIL                   (BK_ERR_OTA_BASE - 11)

#define TLV_TOTAL_SIZE 1024
#define NUM_ECC_BYTES (256 / 8)
#define IMAGE_TLV_INFO_MAGIC        0x6907
#define IMAGE_TLV_PROT_INFO_MAGIC   0x6908

#define IMAGE_TLV_PUBKEY               0x02   /* public key */
#define IMAGE_TLV_SHA256               0x10   /* SHA256 of image hdr and body */
#define IMAGE_TLV_ECDSA256             0x22   /* ECDSA of hash output */
#define IMAGE_TLV_SEC_CNT              0x50   /* security counter */
#define IMAGE_TLV_CUSTOM               0xA0   /* custom public key (new key for app signing) */
#define IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY 0xA1   /* root pubkey*/
#define IMAGE_TLV_CUSTOM_PUBKEY_SIG    0xA2   /* signature of new pubkey hash (signed by root key) */

#define MAX_PUBKEY_LEN              128

typedef struct image_version {
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
} image_version_t;

typedef struct image_header {
    uint32_t ih_magic;
    uint32_t ih_load_addr;
    uint16_t ih_hdr_size;           /* Size of image header (bytes). */
    uint16_t ih_protect_tlv_size;   /* Size of protected TLV area (bytes). */
    uint32_t ih_img_size;           /* Does not include header. */
    uint32_t ih_flags;              /* IMAGE_F_[...]. */
    image_version_t ih_ver;
    uint32_t _pad1;
    uint16_t crc;
} image_header_t;

typedef struct image_tlv_info {
    uint16_t it_magic;
    uint16_t it_tlv_tot;  /* size of TLV area (including tlv_info header) */
} image_tlv_info_t;

/** Image trailer TLV format. All fields in little endian. */
typedef struct image_tlv {
    uint16_t it_type;   /* IMAGE_TLV_[...]. */
    uint16_t it_len;    /* Data length (not including TLV header). */
} image_tlv_t;

#if CONFIG_PSA_MBEDTLS
#define MBEDTLS_CONTEXT_MEMBER(X) MBEDTLS_PRIVATE(X)
#define NUM_ECC_BYTES (256 / 8)
#define CRYPTO_ECDSA_P256_HASH_SIZE (32)

struct bootutil_key {
    uint8_t *key;
    unsigned int *len;
};
#endif

typedef struct ota_verify
{
    bool is_verify_supported;
#if CONFIG_PSA_MBEDTLS
    mbedtls_sha256_context ctx;
	bool ctx_inited;
#endif
    uint32_t validate_offset;
    uint32_t hash_size;
    uint8_t hash_result[32];
    struct image_header hdr;
    uint8_t tlv_total[TLV_TOTAL_SIZE];
} ota_verify_t;

bk_err_t ota_check_crc(const bk_logic_partition_t *partition);
ota_verify_t* ota_verify_init(void);
bk_err_t ota_verify_receive_data(ota_verify_t* ota_verify, const void* data, int len);
bk_err_t ota_verify_image(const bk_logic_partition_t *partition, ota_verify_t* ota_verify);
bk_err_t read_pubkey_from_primary(const bk_logic_partition_t *partition, uint8_t* pubkey, uint16_t* pubkey_size);
void ota_verify_deinit(ota_verify_t* ota_verify);
#if CONFIG_OTA_UPDATE_PUBKEY
bk_err_t ota_verify_pubkey(const bk_logic_partition_t *app_partition,
    ota_verify_t *ota_verify, const uint8_t *ota_outer_pubkey, uint16_t ota_outer_pubkey_len);
#endif
bk_err_t ota_update_back_pubkey_from_primary(const bk_logic_partition_t *app_partition, const bk_logic_partition_t *key_partition);
#if CONFIG_PSA_MBEDTLS
int ota_mbedtls_ecdsa_verify(mbedtls_ecdsa_context *ctx,
								uint8_t *pk, size_t pk_len,
								uint8_t *hash, size_t hash_len,
								uint8_t *sig, size_t sig_len);
bk_err_t ota_verify_signature(uint8_t *hash, uint32_t hash_len,
                               uint8_t *pubkey, uint32_t pubkey_len,
                               uint8_t *signature, uint32_t signature_len);
bk_err_t ota_read_tlv_from_data(uint8_t *tlv_data, uint32_t tlv_data_size, uint16_t tlv_type,
                                 uint8_t *tlv_value, uint32_t *tlv_value_len);
bk_err_t ota_read_tlv_from_partition(const bk_logic_partition_t *partition, uint16_t tlv_type,
                                      uint8_t *tlv_value, uint16_t *tlv_value_len);
#endif