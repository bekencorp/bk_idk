#include "https_ota.h"
#include "cli.h"
#include "modules/bk_ota.h"
// ota_verify.h is already included via https_ota.h -> ota_download.h
#include "driver/flash.h"
#include "driver/flash_partition.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#if CONFIG_OTA_UPDATE_PUBKEY
#if CONFIG_PSA_MBEDTLS
#include <mbedtls/sha256.h>
#include "mbedtls/ecdsa.h"
#include "mbedtls/asn1.h"
#include "mbedtls/error.h"
#include "mbedtls/bignum.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#endif
#endif

#define TAG "HTTPS_OTA_TEST"

static beken_thread_t ota_thread_handle = NULL;
// static ota_download_t* s_ota_handle = NULL;
https_ota_t* s_ota_config = NULL;
char *https_url = NULL;

#if CONFIG_OTA_UPDATE_PUBKEY
extern bk_err_t bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size);
extern uint32_t flash_get_excute_enable(void);

// Helper function to import DER format public key (similar to ota_import_key in ota_verify.c)
static int import_der_pubkey(uint8_t **cp, uint8_t *end)
{
#if CONFIG_PSA_MBEDTLS
	size_t len;
	mbedtls_asn1_buf alg;
	mbedtls_asn1_buf param;
	
	static const uint8_t ec_pubkey_oid[] = MBEDTLS_OID_EC_ALG_UNRESTRICTED;
	static const uint8_t ec_secp256r1_oid[] = MBEDTLS_OID_EC_GRP_SECP256R1;
	
	if (mbedtls_asn1_get_tag(cp, end, &len,
		MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
		return -1;
	}
	end = *cp + len;
	
	/* ECParameters (RFC5480) */
	if (mbedtls_asn1_get_alg(cp, end, &alg, &param)) {
		return -2;
	}
	/* id-ecPublicKey (RFC5480) */
	if (alg.MBEDTLS_CONTEXT_MEMBER(len) != sizeof(ec_pubkey_oid) - 1 ||
		memcmp(alg.MBEDTLS_CONTEXT_MEMBER(p), ec_pubkey_oid, sizeof(ec_pubkey_oid) - 1)) {
		return -3;
	}
	/* namedCurve (RFC5480) */
	if (param.MBEDTLS_CONTEXT_MEMBER(len) != sizeof(ec_secp256r1_oid) - 1 ||
		memcmp(param.MBEDTLS_CONTEXT_MEMBER(p), ec_secp256r1_oid, sizeof(ec_secp256r1_oid) - 1)) {
		return -4;
	}
	/* ECPoint (RFC5480) */
	if (mbedtls_asn1_get_bitstring_null(cp, end, &len)) {
		return -6;
	}
	if (*cp + len != end) {
		return -7;
	}
	
	if (len != 2 * NUM_ECC_BYTES + 1) {
		return -8;
	}
	
	return 0;
#else
	return -1;
#endif
}

// Helper function to read pubkey from primary partition (root key)
static bk_err_t read_root_pubkey_from_primary(const bk_logic_partition_t *partition, uint8_t* pubkey, uint16_t* pubkey_size)
{
	uint32_t key_len = 0;
	image_header_t primary_hdr;

	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition->partition_start_addr));
	bk_flash_read_cbus(fa_off, &primary_hdr, sizeof(image_header_t));
	uint32_t tlv_begin = primary_hdr.ih_hdr_size + primary_hdr.ih_img_size;
	uint32_t offset = 0;
	image_tlv_t tlv;
	uint8_t tlv_value_buf[256];  // Buffer to read TLV value

	// First, try to find 0xA1 TLV (root pubkey) in primary partition
	while(offset < TLV_TOTAL_SIZE) {
		bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));

		if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
			offset += sizeof(image_tlv_t);
		} else if (tlv.it_type == IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY) {
			// Found 0xA1 TLV: root pubkey
			offset += sizeof(image_tlv_t);
			if (tlv.it_len > sizeof(tlv_value_buf) || tlv.it_len > MAX_PUBKEY_LEN) {
				return BK_FAIL;
			}
			bk_flash_read_cbus(fa_off + tlv_begin + offset, pubkey, tlv.it_len);
			key_len = tlv.it_len;
			if (pubkey_size != NULL) {
				*pubkey_size = key_len;
			}
			return BK_OK;
		} else {
			offset += (sizeof(image_tlv_t) + tlv.it_len);
		}
	}

	// If 0xA1 not found, try 0xA0 TLV (fallback for old format)
	offset = 0;
	while(offset < TLV_TOTAL_SIZE) {
		bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));

		if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
			offset += sizeof(image_tlv_t);
		} else if (tlv.it_type == IMAGE_TLV_CUSTOM) {
			// Found 0xA0 TLV, check if it contains new_pubkey + signature or just root pubkey
			offset += sizeof(image_tlv_t);
			if (tlv.it_len > sizeof(tlv_value_buf)) {
				return BK_FAIL;
			}
			bk_flash_read_cbus(fa_off + tlv_begin + offset, tlv_value_buf, tlv.it_len);
			
			// Check if this is new_pubkey + signature format: [pubkey_len(2)][pubkey_data][sig_r(32)][sig_s(32)]
			// If tlv.it_len > 100, it's likely new_pubkey + signature format
			// If tlv.it_len <= 100, it's likely just root pubkey
			if (tlv.it_len > 100) {
				// This is new_pubkey + signature format, but we need root pubkey
				// This shouldn't happen if primary partition is correctly formatted
				return BK_FAIL;
			} else {
				// This is just root pubkey (old format)
				if (tlv.it_len > MAX_PUBKEY_LEN) {
					return BK_FAIL;
				}
				key_len = tlv.it_len;
				memcpy(pubkey, tlv_value_buf, key_len);
			}
			break;
		} else {
			offset += (sizeof(image_tlv_t) + tlv.it_len);
		}
	}

	if (key_len == 0 || key_len > MAX_PUBKEY_LEN) {
		return BK_FAIL;
	}

	if (pubkey_size != NULL) {
		*pubkey_size = key_len;
	}
	return BK_OK;
}

// Helper function to read 0xA0 pubkey from primary partition
static bk_err_t read_pubkey_0xa0_from_primary(const bk_logic_partition_t *partition, uint8_t* pubkey, uint16_t* pubkey_size)
{
	uint32_t key_len = 0;
	image_header_t primary_hdr;

	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition->partition_start_addr));
	bk_flash_read_cbus(fa_off, &primary_hdr, sizeof(image_header_t));
	uint32_t tlv_begin = primary_hdr.ih_hdr_size + primary_hdr.ih_img_size;
	uint32_t offset = 0;
	image_tlv_t tlv;
	uint8_t tlv_value_buf[256];  // Buffer to read TLV value

	// Find 0xA0 TLV in primary partition
	while(offset < TLV_TOTAL_SIZE) {
		bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));

		if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
			offset += sizeof(image_tlv_t);
		} else if (tlv.it_type == IMAGE_TLV_CUSTOM) {
			// Found 0xA0 TLV
			offset += sizeof(image_tlv_t);
			if (tlv.it_len > sizeof(tlv_value_buf) || tlv.it_len > MAX_PUBKEY_LEN) {
				return BK_FAIL;
			}
			bk_flash_read_cbus(fa_off + tlv_begin + offset, pubkey, tlv.it_len);
			key_len = tlv.it_len;
			if (pubkey_size != NULL) {
				*pubkey_size = key_len;
			}
			return BK_OK;
		} else {
			offset += (sizeof(image_tlv_t) + tlv.it_len);
		}
	}

	return BK_FAIL;
}

// Verify trust chain using TLV data collected during download
// Two modes:
// 1. New format (update_img_sign_key_en=TRUE): 0xA0, 0xA1, 0xA2 all exist
//    - Compare 0xA1 with primary partition root pubkey
//    - Verify 0xA2 signature using root key
// 2. Old format (update_img_sign_key_en=FALSE): only 0xA0 exists
//    - Compare 0xA0 from OTA with 0xA0 from primary partition
static bk_err_t verify_trust_chain_from_tlv(ota_verify_t* ota_verify, const bk_logic_partition_t *app_partition)
{
	if (ota_verify == NULL || ota_verify->tlv_total == NULL) {
		return BK_ERR_PARAM;
	}
	
	uint32_t tlv_off = 0;
	image_tlv_info_t tlv_pro_info, tlv_info;
	image_tlv_t tlv;
	
	memcpy(&tlv_pro_info, ota_verify->tlv_total + tlv_off, sizeof(image_tlv_info_t));
	tlv_off += sizeof(image_tlv_info_t);
	memcpy(&tlv_info, ota_verify->tlv_total + tlv_pro_info.it_tlv_tot, sizeof(image_tlv_info_t));
	
	if(tlv_pro_info.it_magic != IMAGE_TLV_PROT_INFO_MAGIC || tlv_info.it_magic != IMAGE_TLV_INFO_MAGIC){
		return BK_ERR_OTA_VALIDATE_TLV_FAIL;
	}
	
	uint8_t *new_pubkey_tlv_value = NULL;  // 0xA0: new_img_sign_pubkey
	uint32_t new_pubkey_tlv_len = 0;
	uint8_t *root_pubkey_tlv_value = NULL;  // 0xA1: img_sign_pubkey (root key)
	uint32_t root_pubkey_tlv_len = 0;
	uint8_t *signature_tlv_value = NULL;  // 0xA2: signature of new_pubkey hash
	uint32_t signature_tlv_len = 0;
	
	// Parse TLV to find 0xA0, 0xA1, and 0xA2
	tlv_off = sizeof(image_tlv_info_t);
	while(tlv_off < tlv_pro_info.it_tlv_tot + tlv_info.it_tlv_tot) {
		if(tlv_off == tlv_pro_info.it_tlv_tot){
			tlv_off += sizeof(image_tlv_info_t);
		}
		memcpy(&tlv, ota_verify->tlv_total + tlv_off, sizeof(image_tlv_t));
		tlv_off += sizeof(image_tlv_t);
		
		if (tlv.it_type == IMAGE_TLV_CUSTOM) {
			// Found 0xA0 TLV: new_img_sign_pubkey
			new_pubkey_tlv_len = tlv.it_len;
			new_pubkey_tlv_value = (uint8_t*)malloc(tlv.it_len);
			if (new_pubkey_tlv_value == NULL) {
				return BK_ERR_NO_MEM;
			}
			memcpy(new_pubkey_tlv_value, ota_verify->tlv_total + tlv_off, tlv.it_len);
		} else if (tlv.it_type == IMAGE_TLV_CUSTOM_ORIGIN_PUBKEY) {
			// Found 0xA1 TLV: img_sign_pubkey (root key)
			root_pubkey_tlv_len = tlv.it_len;
			root_pubkey_tlv_value = (uint8_t*)malloc(tlv.it_len);
			if (root_pubkey_tlv_value == NULL) {
				if (new_pubkey_tlv_value) {
					free(new_pubkey_tlv_value);
				}
				return BK_ERR_NO_MEM;
			}
			memcpy(root_pubkey_tlv_value, ota_verify->tlv_total + tlv_off, tlv.it_len);
		} else if (tlv.it_type == IMAGE_TLV_CUSTOM_PUBKEY_SIG) {
			// Found 0xA2 TLV: signature of new_pubkey hash
			signature_tlv_len = tlv.it_len;
			signature_tlv_value = (uint8_t*)malloc(tlv.it_len);
			if (signature_tlv_value == NULL) {
				if (new_pubkey_tlv_value) {
					free(new_pubkey_tlv_value);
				}
				if (root_pubkey_tlv_value) {
					free(root_pubkey_tlv_value);
				}
				return BK_ERR_NO_MEM;
			}
			memcpy(signature_tlv_value, ota_verify->tlv_total + tlv_off, tlv.it_len);
		}
		
		tlv_off += tlv.it_len;
	}
	
	// Check if 0xA0 TLV exists (required for both formats)
	if (new_pubkey_tlv_value == NULL) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Missing required TLV: 0xA0=MISSING\r\n");
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}
	
	// Validate 0xA0 TLV length
	if (new_pubkey_tlv_len > MAX_PUBKEY_LEN) {
		free(new_pubkey_tlv_value);
		return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
	}
	
	// Determine verification mode based on TLV presence
	// If 0xA1 and 0xA2 exist, use new format (trust chain verification)
	// If only 0xA0 exists, use old format (simple pubkey comparison)
	if (root_pubkey_tlv_value != NULL && signature_tlv_value != NULL) {
		// New format: update_img_sign_key_en=TRUE
		// Need 0xA0, 0xA1, 0xA2 all present
		BK_LOGI(TAG, "[TRUST_CHAIN] Using new format (trust chain verification)\r\n");
		
		// Validate TLV lengths
		if (root_pubkey_tlv_len > MAX_PUBKEY_LEN) {
			free(new_pubkey_tlv_value);
			free(root_pubkey_tlv_value);
			free(signature_tlv_value);
			return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
		}
		
		// Step 1: Compare 0xA1 (root pubkey) with primary partition root pubkey
		uint8_t root_pubkey_from_primary[MAX_PUBKEY_LEN];
		uint16_t root_pubkey_from_primary_len = 0;
		bk_err_t ret = read_root_pubkey_from_primary(app_partition, root_pubkey_from_primary, &root_pubkey_from_primary_len);
		if (ret != BK_OK) {
			BK_LOGE(TAG, "[TRUST_CHAIN] Failed to read root pubkey from primary, ret=0x%x\r\n", ret);
			free(new_pubkey_tlv_value);
			free(root_pubkey_tlv_value);
			free(signature_tlv_value);
			return ret;
		}
		
		if (root_pubkey_tlv_len != root_pubkey_from_primary_len ||
		    memcmp(root_pubkey_tlv_value, root_pubkey_from_primary, root_pubkey_tlv_len) != 0) {
			BK_LOGE(TAG, "[TRUST_CHAIN] Root pubkey mismatch\r\n");
			free(new_pubkey_tlv_value);
			free(root_pubkey_tlv_value);
			free(signature_tlv_value);
			return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
		}
		
		// Step 2: Verify signature of new_pubkey (from 0xA0) using root pubkey (from 0xA1 or primary)
#if CONFIG_PSA_MBEDTLS
	// Calculate hash of new pubkey (from 0xA0)
	uint8_t new_pubkey_hash[32];
	mbedtls_sha256_context sha256_ctx;
	mbedtls_sha256_init(&sha256_ctx);
	mbedtls_sha256_starts(&sha256_ctx, 0);
	mbedtls_sha256_update(&sha256_ctx, new_pubkey_tlv_value, new_pubkey_tlv_len);
	mbedtls_sha256_finish(&sha256_ctx, new_pubkey_hash);
	mbedtls_sha256_free(&sha256_ctx);
	
	// NOTE: imgtool sign-hash uses ec.ECDSA(SHA256()) which hashes the input again
	// So we need to hash the hash again to match what imgtool signed
	// This is a workaround because we cannot modify imgtool
	uint8_t double_hashed[32];
	mbedtls_sha256_init(&sha256_ctx);
	mbedtls_sha256_starts(&sha256_ctx, 0);
	mbedtls_sha256_update(&sha256_ctx, new_pubkey_hash, 32);
	mbedtls_sha256_finish(&sha256_ctx, double_hashed);
	mbedtls_sha256_free(&sha256_ctx);
	
	// Use double-hashed value for verification (to match what imgtool signed)
	uint8_t *hash_for_verification = double_hashed;
	
	// Parse signature from 0xA2 (DER format)
	if (signature_tlv_len < 8 || signature_tlv_value[0] != 0x30) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Invalid DER signature format\r\n");
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Setup mbedTLS context for verification
	mbedtls_ecdsa_context ctx;
	mbedtls_ecdsa_init(&ctx);
	
	// Load EC group
	ret = mbedtls_ecp_group_load(&ctx.MBEDTLS_CONTEXT_MEMBER(grp), MBEDTLS_ECP_DP_SECP256R1);
	if (ret != 0) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Failed to load EC group, ret=%d\r\n", ret);
		mbedtls_ecdsa_free(&ctx);
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Import root public key (from primary, already verified to match 0xA1)
	uint8_t *ppubkey = root_pubkey_from_primary;
	uint8_t *end = root_pubkey_from_primary + root_pubkey_from_primary_len;
	ret = import_der_pubkey(&ppubkey, end);
	if (ret != 0) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Failed to import root pubkey, ret=%d\r\n", ret);
		mbedtls_ecdsa_free(&ctx);
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Read public key point
	uint32_t pubkey_point_len = end - ppubkey;
	if (pubkey_point_len != 65 || ppubkey[0] != 0x04) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Invalid pubkey point format\r\n");
		mbedtls_ecdsa_free(&ctx);
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	ret = mbedtls_ecp_point_read_binary(&ctx.MBEDTLS_CONTEXT_MEMBER(grp), &ctx.MBEDTLS_CONTEXT_MEMBER(Q), ppubkey, pubkey_point_len);
	if (ret != 0) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Failed to read pubkey point, ret=%d\r\n", ret);
		mbedtls_ecdsa_free(&ctx);
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Check public key validity
	ret = mbedtls_ecp_check_pubkey(&ctx.MBEDTLS_CONTEXT_MEMBER(grp), &ctx.MBEDTLS_CONTEXT_MEMBER(Q));
	if (ret != 0) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Invalid pubkey, ret=%d\r\n", ret);
		mbedtls_ecdsa_free(&ctx);
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Verify signature using root key
	ret = mbedtls_ecdsa_read_signature(&ctx, hash_for_verification, 32, signature_tlv_value, signature_tlv_len);
	mbedtls_ecdsa_free(&ctx);
	
	if (ret != 0) {
		BK_LOGE(TAG, "[TRUST_CHAIN] Signature verification failed, ret=%d\r\n", ret);
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
		BK_LOGI(TAG, "pubkey ok!");
#endif
		
		free(new_pubkey_tlv_value);
		free(root_pubkey_tlv_value);
		free(signature_tlv_value);
		
		return BK_OK;
	} else {
		// Old format: update_img_sign_key_en=FALSE
		// Only 0xA0 exists, compare with primary partition 0xA0
		BK_LOGI(TAG, "[TRUST_CHAIN] Using old format (simple pubkey comparison)\r\n");
		
		// Read 0xA0 pubkey from primary partition
		uint8_t primary_pubkey_0xa0[MAX_PUBKEY_LEN];
		uint16_t primary_pubkey_0xa0_len = 0;
		bk_err_t ret = read_pubkey_0xa0_from_primary(app_partition, primary_pubkey_0xa0, &primary_pubkey_0xa0_len);
		if (ret != BK_OK) {
			BK_LOGE(TAG, "[TRUST_CHAIN] Failed to read 0xA0 pubkey from primary, ret=0x%x\r\n", ret);
			free(new_pubkey_tlv_value);
			return ret;
		}
		
		// Compare 0xA0 pubkey from OTA with 0xA0 pubkey from primary
		if (new_pubkey_tlv_len != primary_pubkey_0xa0_len ||
		    memcmp(new_pubkey_tlv_value, primary_pubkey_0xa0, new_pubkey_tlv_len) != 0) {
			BK_LOGE(TAG, "[TRUST_CHAIN] 0xA0 pubkey mismatch between OTA and primary\r\n");
			free(new_pubkey_tlv_value);
			return BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
		}
		
		BK_LOGI(TAG, "pubkey ok!\r\n");
		free(new_pubkey_tlv_value);
		
		return BK_OK;
	}
}
#endif

/* this crt for url https://docs.bekencorp.com , support test*/
const char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"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\r\n"
"clib3BcxKZX9qK4A6FAATghuSQ==\r\n"
"-----END CERTIFICATE-----\r\n"
};

static void cli_ota_help(void)
{
	BK_LOGI(TAG,"Two example:\n");
	BK_LOGI(TAG,"https_ota + url\n");
	BK_LOGI(TAG,"OR\n");
	BK_LOGI(TAG,"ota init + url\n");
	BK_LOGI(TAG,"ota begin/download/pause/end/deinit/verify\n");
	BK_LOGI(TAG,"ota confirm current/ow/xip\r\n");
	BK_LOGI(TAG,"ota entire_erase 1/0\r\n");
}

void ota_event_cb(https_ota_event_t evt)
{
	switch (evt.event_id) {
	case HTTPS_OTA_START_EVENT:
		break;
	case HTTPS_OTA_ERROR_EVENT:
		BK_LOGE(TAG,"OTA ERROR CODE %d\r\n",*(int*)(evt.data));
		break;
	case HTTPS_OTA_SUCCESS_EVENT:
#if CONFIG_OTA_UPDATE_PUBKEY
		if (s_ota_config && s_ota_config->ota_parse && s_ota_config->ota_parse->ota_handle) {
			if (bk_ota_update_public_key(s_ota_config->ota_parse->ota_handle) != BK_OK) {
				BK_LOGE(TAG, "Back pubkey fail!\r\n");
				break;
			}
		}
#endif
#if CONFIG_OTA_CONFIRM_UPDATE
		if (s_ota_config && s_ota_config->ota_parse && s_ota_config->ota_parse->ota_handle) {
			if (bk_ota_confirm(s_ota_config->ota_parse->ota_handle) != BK_OK) {
				BK_LOGE(TAG,"set confirm fail!\r\n");
				break;
			}
		}
#endif
		BK_LOGI(TAG,"OTA SUCCESS\r\n");
		break;	
	case HTTPS_OTA_RESTART_EVENT:
		break;
	case HTTPS_OTA_PAUSE_EVENT:
		break;
	}
}

bk_err_t https_event_cb(bk_http_client_event_t *evt)
{
	switch (evt->event_id) {
	case HTTP_EVENT_ERROR:
		BK_LOGE(TAG, "HTTPS_EVENT_ERROR\r\n");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		BK_LOGI(TAG, "HTTPS_EVENT_ON_CONNECTED\r\n");
		break;
	case HTTP_EVENT_HEADER_SENT:
		BK_LOGD(TAG, "HTTPS_EVENT_HEADER_SENT\r\n");
		break;
	case HTTP_EVENT_ON_HEADER:
		BK_LOGD(TAG, "HTTPS_EVENT_ON_HEADER\r\n");
		break;
	case HTTP_EVENT_ON_DATA:
		BK_LOGD(TAG, "HTTP_EVENT_ON_DATA, length:%d\r\n", evt->data_len);
		break;
	case HTTP_EVENT_ON_FINISH:
		BK_LOGI(TAG, "HTTPS_EVENT_ON_FINISH\r\n");
		break;
	case HTTP_EVENT_DISCONNECTED:
		BK_LOGI(TAG, "HTTPS_EVENT_DISCONNECTED\r\n");
		break;

	}
	return BK_OK;
}

https_ota_t* generate_ota_config(char * https_url)
{
	https_ota_t* ota_config = (https_ota_t*)os_malloc(sizeof(https_ota_t));
	if (ota_config == NULL) {
		return NULL;
	}
	os_memset(ota_config, 0, sizeof(https_ota_t));
	ota_config->http_config = (bk_http_input_t*)os_malloc(sizeof(bk_http_input_t));
	if (ota_config->http_config == NULL) {
		os_free(ota_config);
		return NULL;
	}
	os_memset(ota_config->http_config, 0, sizeof(bk_http_input_t));
	

	ota_config->http_config ->url = https_url;
	ota_config->http_config ->cert_pem = ca_crt_rsa;
	ota_config->http_config ->event_handler = https_event_cb;
	ota_config->http_config ->buffer_size = 4096;
	ota_config->http_config ->user_agent = "TEST HTTP Client/1.1";
	ota_config->http_config ->timeout_ms = 15000;
	ota_config->http_config ->keep_alive_enable = true,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
	ota_config->http_config ->crt_bundle_attach = esp_crt_bundle_attach,
#endif
	ota_config->ota_download_resume = true;
	ota_config->ota_event_handler = ota_event_cb;
	return ota_config;
}

void release_ota_config(https_ota_t* ota_config)
{
	if (ota_config->http_config != NULL) {
		os_free(ota_config->http_config);
	}
	if (ota_config != NULL) {
		os_free(ota_config);
	}
}

void bk_https_start_download(beken_thread_arg_t arg) 
{
	https_ota_state_t state;
	s_ota_config = generate_ota_config(https_url);
	bk_https_ota_init(s_ota_config);
	bk_https_ota_set_erase_method(s_ota_config, true);

	state = bk_https_ota_perform(s_ota_config);

	if (state == HTTPS_OTA_FINISH) {
		bk_err_t verify_ret = BK_FAIL;
		bk_err_t trust_chain_ret = BK_OK;
		bk_err_t confirm_ret = BK_FAIL;
		if (s_ota_config && s_ota_config->ota_parse && s_ota_config->ota_parse->ota_handle) {
			verify_ret = bk_ota_verify(s_ota_config->ota_parse->ota_handle);
#if CONFIG_OTA_UPDATE_PUBKEY
			// Verify trust chain using TLV data collected during download
			if (verify_ret == BK_OK && s_ota_config->ota_parse->ota_verify != NULL) {
				const bk_logic_partition_t *app_partition = s_ota_config->ota_parse->ota_handle->app_partition;
				if (app_partition == NULL) {
					trust_chain_ret = BK_ERR_PARAM;
				} else {
					trust_chain_ret = verify_trust_chain_from_tlv(s_ota_config->ota_parse->ota_verify, app_partition);
				}
			}
			
			if (trust_chain_ret != BK_OK && s_ota_config->ota_parse->ota_handle != NULL) {
				const bk_logic_partition_t *ota_partition = NULL;
				#if CONFIG_OTA_OVERWRITE
				ota_partition = s_ota_config->ota_parse->ota_handle->ota_partition;//TODO optimize it
				#else
				if (flash_get_excute_enable()) {
					ota_partition = s_ota_config->ota_parse->ota_handle->app_partition;
				} else {
					ota_partition = s_ota_config->ota_parse->ota_handle->ota_partition;
				}
				#endif
				
				if (ota_partition != NULL) {
					uint32_t partition_start = ota_partition->partition_start_addr;
					uint32_t erase_size = 0x1000; // 4KB
					
					BK_LOGI(TAG, "Trust chain verification failed, erasing first 4KB of OTA partition at 0x%x\r\n", partition_start);
					
					bk_err_t erase_ret = bk_flash_erase_fast(partition_start, erase_size);
					if (erase_ret != BK_OK) {
						BK_LOGE(TAG, "Failed to erase OTA partition header, ret=0x%x\r\n", erase_ret);
					} else {
						BK_LOGI(TAG, "OTA partition header (4KB) erased successfully\r\n");
					}
				}
			}
#endif
			if (verify_ret == BK_OK && trust_chain_ret == BK_OK) {
				confirm_ret = bk_ota_confirm(s_ota_config->ota_parse->ota_handle);
			}
#if CONFIG_OTA_UPDATE_PUBKEY
			// Deinitialize ota_verify after trust chain verification
			if (s_ota_config->ota_parse->ota_verify != NULL) {
				ota_verify_deinit(s_ota_config->ota_parse->ota_verify);
				s_ota_config->ota_parse->ota_verify = NULL;
			}
#endif
		}
		if (verify_ret == BK_OK && trust_chain_ret == BK_OK && confirm_ret == BK_OK) {
			BK_LOGI(TAG, "HTTPS OTA SUCCESS (verify+trust_chain+confirm done)\r\n");
		} else {
			BK_LOGE(TAG, "HTTPS OTA verify/trust_chain/confirm fail, verify=0x%x trust_chain=0x%x confirm=0x%x\r\n", 
				verify_ret, trust_chain_ret, confirm_ret);
		}
	} else {
		BK_LOGE(TAG, "HTTPS OTA perform fail, state=%d\r\n", state);
	}

	bk_https_ota_deinit(s_ota_config);
	release_ota_config(s_ota_config);
	s_ota_config = NULL;
	ota_thread_handle = NULL;
	rtos_delete_thread(NULL);
}

void https_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 ret = 0;
	if (argc != 2 || argv[1] == NULL) {
		BK_LOGE(TAG, "Usage: https_ota <url>\r\n");
		return;
	}
	if (https_url != NULL) {
		free(https_url);
		https_url = NULL;
	}
	https_url = (char*)malloc(strlen(argv[1]) + 1);
	if (https_url == NULL) {
		BK_LOGE(TAG, "malloc https_url failed\r\n");
		return;
	}
	strcpy(https_url, argv[1]);
	BK_LOGI(TAG, "https_ota_start\r\n");
	if (ota_thread_handle == NULL) {
		ret = rtos_create_thread(&ota_thread_handle, 4,
								"https_ota",
								(beken_thread_function_t)bk_https_start_download,
								5120,
								0);
	}
	if (kNoErr != ret)
		BK_LOGE(TAG, "https_ota_start failed\r\n");
}

void ota_steps_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "init") == 0) {
		if (argc < 3 || argv[2] == NULL) {
			BK_LOGE(TAG, "Usage: ota init <url>\r\n");
			return;
		}
		if (https_url != NULL) {
			free(https_url);
			https_url = NULL;
		}
		https_url = (char*)malloc(strlen(argv[2]) + 1);
		if (https_url == NULL) {
			BK_LOGE(TAG, "malloc https_url failed\r\n");
			return;
		}
		strcpy(https_url, argv[2]);
		s_ota_config = generate_ota_config(https_url);
		bk_https_ota_init(s_ota_config);
	} else if (os_strcmp(argv[1], "begin") == 0) {
		bk_https_ota_begin(s_ota_config);
	} else if (os_strcmp(argv[1], "download") == 0) {
		bk_https_ota_progress_data(s_ota_config);
	} else if (os_strcmp(argv[1], "pause") == 0) {
		bk_https_ota_pause(s_ota_config);
		BK_LOGI(TAG, "OTA PAUSED\r\n");
	} else if (os_strcmp(argv[1], "end") == 0) {
		bk_https_ota_end(s_ota_config);
	} else if (os_strcmp(argv[1], "confirm") == 0) {
		if (argc < 3) {
			BK_LOGE(TAG, "Usage: ota confirm <current/ow/xip>\r\n");
			BK_LOGE(TAG, "  current - confirm current OTA upgrade\r\n");
			BK_LOGE(TAG, "  ow      - confirm OW mode upgrade\r\n");
			BK_LOGE(TAG, "  xip     - confirm XIP mode upgrade\r\n");
			return;
		}
		if (os_strcmp(argv[2], "ow") == 0) {
			bk_ota_t *ow_ota_handle = NULL;
			ow_ota_handle = bk_ota_get_ow_handle();
			if (ow_ota_handle != NULL) {
				bk_ota_confirm(ow_ota_handle);
				BK_LOGI(TAG, "OTA confirm (ow) success\r\n");
			} else {
				BK_LOGE(TAG, "Failed to get OW OTA handle\r\n");
			}
		} else if (os_strcmp(argv[2], "xip") == 0) {
			bk_ota_t *wifi_ota_handle = NULL;
			wifi_ota_handle = bk_ota_get_xip_handle();
			if (wifi_ota_handle != NULL) {
				bk_ota_confirm(wifi_ota_handle);
				BK_LOGI(TAG, "OTA confirm (xip) success\r\n");
			} else {
				BK_LOGE(TAG, "Failed to get XIP OTA handle\r\n");
			}
		} else if (os_strcmp(argv[2], "current") == 0) {
			if (s_ota_config != NULL && s_ota_config->ota_parse != NULL && s_ota_config->ota_parse->ota_handle != NULL) {
				BK_LOGI(TAG, "OTA confirm (current): using ota_handle from config\r\n");
				if (s_ota_config->ota_parse->ota_handle->control_partition != NULL) {
					BK_LOGI(TAG, "control_partition: %s, addr=0x%x\r\n", 
						s_ota_config->ota_parse->ota_handle->control_partition->partition_description ? 
						s_ota_config->ota_parse->ota_handle->control_partition->partition_description : "NULL",
						s_ota_config->ota_parse->ota_handle->control_partition->partition_start_addr);
				} else {
					BK_LOGE(TAG, "control_partition is NULL!\r\n");
				}
				bk_err_t ret = bk_ota_confirm(s_ota_config->ota_parse->ota_handle);
				if (ret == BK_OK) {
					BK_LOGI(TAG, "OTA confirm (current) success\r\n");
				} else {
					BK_LOGE(TAG, "OTA confirm (current) failed, ret=0x%x\r\n", ret);
					BK_LOGE(TAG, "Try: ota confirm xip (for DIRECT_XIP mode)\r\n");
				}
			} else {
				BK_LOGE(TAG, "config or ota_handle is NULL\r\n");
				BK_LOGE(TAG, "Try: ota confirm <ow/xip>\r\n");
			}
		} else {
			BK_LOGE(TAG, "Invalid confirm parameter: %s\r\n", argv[2]);
			BK_LOGE(TAG, "Usage: ota confirm <current/ow/xip>\r\n");
		}
	} else if (os_strcmp(argv[1], "verify") == 0) {
		if (s_ota_config != NULL) {
			if (bk_ota_verify(s_ota_config->ota_parse->ota_handle) != BK_OK) {
				BK_LOGE(TAG,"ota partition verify error!\r\n");
			} else {
				BK_LOGI(TAG,"verify pass\r\n");
			}
		} else {
			BK_LOGE(TAG,"config has been released,try other verify commands!\r\n");
		}
	} else if (os_strcmp(argv[1], "entire_erase") == 0) {
		bool erase = os_strtoul(argv[2], NULL, 10);
		bk_https_ota_set_erase_method(s_ota_config, erase);
		BK_LOGI(TAG,"set erase method:%d\r\n",s_ota_config->ota_parse->entire_flash_erase);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		bk_https_ota_deinit(s_ota_config);
		release_ota_config(s_ota_config);
		s_ota_config = NULL;
#if CONFIG_OTA_UPDATE_PUBKEY
	} else if (os_strcmp(argv[1], "update_pubkey") == 0) {
		bk_ota_t *ow_ota_handle = NULL;
		ow_ota_handle = bk_ota_get_ow_handle();
		bk_ota_update_public_key(ow_ota_handle);
#endif
	} else {
		cli_ota_help();
	}
}

#define OTA_CMD_CNT (sizeof(s_ota_commands) / sizeof(struct cli_command))
static const struct cli_command s_ota_commands[] = {
	{"https_ota","",https_ota_Command},
	{"ota","",ota_steps_Command},
};

int cli_https_ota_init(void)
{
	return cli_register_commands(s_ota_commands, OTA_CMD_CNT);
}
