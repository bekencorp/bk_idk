#include <string.h>
#include "sdkconfig.h"
#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "CheckSumUtils.h"
#include "security.h"
#include "ota_verify.h"
#include "ota_common.h"
#include "partitions.h"

#define TAG "ota"

typedef struct flash_data {
	uint8_t data[32];
	uint16_t crc16;
} flash_data_t;

bk_err_t ota_check_crc(const bk_logic_partition_t *partition)
{
	flash_data_t enc_data;
	uint32_t phy_start = CEIL_ALIGN_34(partition->partition_start_addr);
	size_t size = FLOOR_ALIGN_34(partition->partition_length);
	uint8_t ff_buf[34];

	for (size_t i = 0; i < size / 34; i++) {
		bk_flash_read_bytes(phy_start + i*34, (uint8_t *)&enc_data, 34);
		memset(ff_buf, 0xFF, 34);
		if (memcmp(ff_buf, (const void *)&enc_data, 34) == 0) {
			continue;
		}
		if (enc_data.crc16 != calculate_crc16(enc_data.data, 32)) {
			BK_LOGE(TAG, "partition %s offset %#x crc error!\r\n",partition->partition_description, i*34);
			BK_LOGE(TAG, "read crc = %#x, calculate crc =%#x.\r\n",enc_data.crc16, calculate_crc16(enc_data.data, 32));
			return false;
		}
	}
	return true;
}

bool ota_check_data_is_crc(uint8_t* buf)
{
	flash_data_t enc_data;
	memcpy(&enc_data, buf, 34);
	if (enc_data.crc16 == calculate_crc16(enc_data.data, 32)) {
		return true;
	} else {
		return false;
	}
}

static uint32_t count_bit_one_in_byte(uint8_t byte)
{
	uint32_t counter = 0;
	while(byte) {
		counter += byte & 1;
		byte >>= 1;
	}
	return counter;
}

static uint32_t count_bit_one_in_buffer(uint8_t* buffer, size_t length)
{
	uint32_t counter = 0;
	uint32_t temp;
	for(size_t i=0; i < length; ++i){
		temp = count_bit_one_in_byte(buffer[i]);
		counter += temp;
		if(temp == 0)
			break;
	}
	return counter;
}

static void set_bit_one_in_buffer(uint8_t* buffer, size_t value)
{
	size_t byte_index = 0;
	while(value > 0){
		if(value > 8){
			buffer[byte_index] = 0xFF;
			value -= 8;
		} else {
			buffer[byte_index] |= (0xFF >> (8 - value));
			value = 0;
		}
		byte_index++;
	}
}

#if CONFIG_PSA_MBEDTLS

static const uint8_t ec_pubkey_oid[] = MBEDTLS_OID_EC_ALG_UNRESTRICTED;
static const uint8_t ec_secp256r1_oid[] = MBEDTLS_OID_EC_GRP_SECP256R1;
struct bootutil_key bootutil_keys;

static int ota_import_key(uint8_t **cp, uint8_t *end)
{
	size_t len;
	mbedtls_asn1_buf alg;
	mbedtls_asn1_buf param;

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
}

int ota_mbedtls_ecdsa_verify(mbedtls_ecdsa_context *ctx,
											uint8_t *pk, size_t pk_len,
											uint8_t *hash, size_t hash_len,
											uint8_t *sig, size_t sig_len)
{
	int rc = -1;

	(void)sig;
	(void)hash;
	(void)hash_len;

	rc = mbedtls_ecp_group_load(&ctx->MBEDTLS_CONTEXT_MEMBER(grp), MBEDTLS_ECP_DP_SECP256R1);
	if (rc) {
		return -1;
	}

	rc = mbedtls_ecp_point_read_binary(&ctx->MBEDTLS_CONTEXT_MEMBER(grp), &ctx->MBEDTLS_CONTEXT_MEMBER(Q), pk, pk_len);
	if (rc) {
		return -1;
	}

	rc = mbedtls_ecp_check_pubkey(&ctx->MBEDTLS_CONTEXT_MEMBER(grp), &ctx->MBEDTLS_CONTEXT_MEMBER(Q));
	if (rc) {
		return -1;
	}

	rc = mbedtls_ecdsa_read_signature(ctx, hash, CRYPTO_ECDSA_P256_HASH_SIZE,
										sig, sig_len);
	if (rc) {
		return -1;
	}

	return 0;
}

static int ota_verify_sig(uint8_t *hash, uint32_t hlen, uint8_t *sig, size_t slen,
                    uint8_t key_id)
{
    int rc;
    mbedtls_ecdsa_context ctx;

    uint8_t *pubkey;
    uint8_t *end;

    pubkey = (uint8_t *)bootutil_keys.key;
    end = pubkey + *bootutil_keys.len;
    mbedtls_ecdsa_init(&ctx);

    rc = ota_import_key(&pubkey, end);
    if (rc) {
        goto out;
    }

    rc = ota_mbedtls_ecdsa_verify(&ctx, pubkey, end-pubkey, hash, hlen, sig, slen);

out:
    mbedtls_ecdsa_free(&ctx);
	return 0;
}

static void mbedtls_sha256_hash(uint8_t *digest, uint32_t digest_sz, uint8_t *hash_result)
{
	mbedtls_sha256_context sha256_ctx;
	mbedtls_sha256_init(&sha256_ctx);
	mbedtls_sha256_starts(&sha256_ctx,0);
	mbedtls_sha256_update(&sha256_ctx, digest, digest_sz);
	mbedtls_sha256_finish(&sha256_ctx, hash_result);
	mbedtls_sha256_free(&sha256_ctx);
}

bk_err_t ota_verify_signature(uint8_t *hash, uint32_t hash_len,
                               uint8_t *pubkey, uint32_t pubkey_len,
                               uint8_t *signature, uint32_t signature_len)
{
	// Validate hash length
	if (hash_len != 32) {
		BK_LOGE(TAG, "Invalid hash length, must be 32 bytes\r\n");
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Validate signature format (DER format should start with 0x30)
	if (signature_len < 8 || signature[0] != 0x30) {
		BK_LOGE(TAG, "Invalid DER signature format\r\n");
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Setup mbedTLS context for verification
	mbedtls_ecdsa_context ctx;
	mbedtls_ecdsa_init(&ctx);
	
	// Import public key
	uint8_t *ppubkey = pubkey;
	uint8_t *end = pubkey + pubkey_len;
	int ret = ota_import_key(&ppubkey, end);
	if (ret != 0) {
		BK_LOGE(TAG, "Failed to import public key, ret=%d\r\n", ret);
		mbedtls_ecdsa_free(&ctx);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	// Verify signature
	ret = ota_mbedtls_ecdsa_verify(&ctx, ppubkey, end - ppubkey, hash, hash_len, signature, signature_len);
	mbedtls_ecdsa_free(&ctx);
	
	if (ret != 0) {
		BK_LOGE(TAG, "Signature verification failed, ret=%d\r\n", ret);
		return BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}
	
	return BK_OK;
}
#endif

// Read TLV from TLV data buffer
// This is a generic function to read specific TLV type from TLV data
bk_err_t ota_read_tlv_from_data(uint8_t *tlv_data, uint32_t tlv_data_size, uint16_t tlv_type, 
                                 uint8_t *tlv_value, uint32_t *tlv_value_len)
{
	uint32_t tlv_off = 0;
	image_tlv_info_t tlv_pro_info, tlv_info;
	image_tlv_t tlv;
	
	if (tlv_data == NULL || tlv_value == NULL || tlv_value_len == NULL) {
		return BK_ERR_PARAM;
	}
	
	memcpy(&tlv_pro_info, tlv_data + tlv_off, sizeof(image_tlv_info_t));
	tlv_off += sizeof(image_tlv_info_t);
	memcpy(&tlv_info, tlv_data + tlv_pro_info.it_tlv_tot, sizeof(image_tlv_info_t));
	
	if(tlv_pro_info.it_magic != IMAGE_TLV_PROT_INFO_MAGIC || tlv_info.it_magic != IMAGE_TLV_INFO_MAGIC){
		return BK_ERR_OTA_VALIDATE_TLV_FAIL;
	}
	
	// Parse TLV to find specific type
	tlv_off = sizeof(image_tlv_info_t);
	while(tlv_off < tlv_pro_info.it_tlv_tot + tlv_info.it_tlv_tot) {
		if(tlv_off == tlv_pro_info.it_tlv_tot){
			tlv_off += sizeof(image_tlv_info_t);
		}
		memcpy(&tlv, tlv_data + tlv_off, sizeof(image_tlv_t));
		tlv_off += sizeof(image_tlv_t);
		
		if (tlv.it_type == tlv_type) {
			// Found TLV
			if (tlv.it_len > *tlv_value_len) {
				return BK_ERR_PARAM;
			}
			memcpy(tlv_value, tlv_data + tlv_off, tlv.it_len);
			*tlv_value_len = tlv.it_len;
			return BK_OK;
		}
		
		tlv_off += tlv.it_len;
	}
	
	return BK_FAIL;
}

// Read TLV from flash partition
bk_err_t ota_read_tlv_from_partition(const bk_logic_partition_t *partition, uint16_t tlv_type,
                                      uint8_t *tlv_value, uint16_t *tlv_value_len)
{
	uint32_t key_len = 0;
	image_header_t primary_hdr;
	
	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition->partition_start_addr));
	bk_flash_read_cbus(fa_off, &primary_hdr, sizeof(image_header_t));
	uint32_t tlv_begin = primary_hdr.ih_hdr_size + primary_hdr.ih_img_size;
	uint32_t offset = 0;
	image_tlv_t tlv;
	
	while(offset < TLV_TOTAL_SIZE) {
		bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t));
		
		if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
			offset += sizeof(image_tlv_t);
		} else if (tlv.it_type == tlv_type) {
			// Found TLV
			offset += sizeof(image_tlv_t);
			key_len = tlv.it_len;
			if (key_len > MAX_PUBKEY_LEN || (tlv_value_len && key_len > *tlv_value_len)) {
				return BK_FAIL;
			}
			bk_flash_read_cbus(fa_off + tlv_begin + offset, tlv_value, key_len);
			if (tlv_value_len != NULL) {
				*tlv_value_len = key_len;
			}
			return BK_OK;
		} else {
			offset += (sizeof(image_tlv_t) + tlv.it_len);
		}
	}
	
	return BK_FAIL;
}

static void debug_print_buf(const char *str, const uint8_t *buf, uint32_t len)
{
	BK_RAW_LOGI(TAG, "%s\r\n", str);
	for (int i = 0; i < len; i++) {
		if (i && (i % 32) == 0) {
			BK_RAW_LOGI(TAG, "\r\n");
		}
		BK_RAW_LOGI(TAG, "%02x ", buf[i]);
	}
	BK_RAW_LOGI(TAG, "\r\n");
}


bk_err_t read_pubkey_from_primary(const bk_logic_partition_t *partition, uint8_t* pubkey, uint16_t* pubkey_size)
{
	uint32_t key_len = 0;
	image_header_t primary_hdr;

	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition->partition_start_addr));
	(bk_flash_read_cbus(fa_off, &primary_hdr, sizeof(image_header_t)));
	uint32_t tlv_begin = primary_hdr.ih_hdr_size + primary_hdr.ih_img_size;
	uint32_t offset = 0;
	image_tlv_t tlv;

	while(offset < TLV_TOTAL_SIZE) {
		(bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t)));

		if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
			offset += sizeof(image_tlv_t);
		} else if (tlv.it_type != IMAGE_TLV_CUSTOM) {
			offset += (sizeof(image_tlv_t) + tlv.it_len);
		} else {
			offset += sizeof(image_tlv_t);
			key_len = tlv.it_len;
			break;
		}
	}

	OTA_LOGI(TAG, "public key in primary offset=%#x, len=%#x\r\n",offset, key_len);
	if (offset > TLV_TOTAL_SIZE || key_len > MAX_PUBKEY_LEN) {
		return BK_FAIL;
	}

	(bk_flash_read_cbus(fa_off + tlv_begin + offset, pubkey, key_len));

	if (pubkey_size != NULL) {
		*pubkey_size = key_len;
	}

	return BK_OK;
}

bk_err_t ota_update_back_pubkey_from_primary(const bk_logic_partition_t *app_partition, const bk_logic_partition_t *key_partition)
{
	if ((!app_partition) || (!key_partition)) {
		OTA_LOGE(TAG, "invalid key partition\r\n");
		return BK_FAIL;
	}

	uint32_t phy_off = key_partition->partition_start_addr;
	uint32_t vir_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_off + 3)); 
	uint16_t vir_length;
	uint8_t* back_key = NULL;
	uint8_t primary_key[MAX_PUBKEY_LEN] = {0};
	uint16_t primary_key_size = 0;
	CRC8_Context crc8;
	CRC8_Context check_crc;
	uint8_t *enc_buf = NULL;
	uint8_t check_erase_buf[64];
	uint8_t ff_buf[64];
	bk_err_t ret = BK_OK;

	ret = read_pubkey_from_primary(app_partition, primary_key, &primary_key_size);
	if (ret != BK_OK) {
		OTA_LOGE(TAG, "read primary key fail, ret=%d\r\n",ret);
		return ret;
	}

	memset(ff_buf, 0xFF, 64);
	bk_flash_read_bytes(phy_off, check_erase_buf, 64);
	if (memcmp(check_erase_buf, ff_buf, 64) != 0) {
		bk_flash_read_bytes(phy_off, (uint8_t *)&vir_length, 2);
		bk_flash_read_bytes(phy_off + 2, (uint8_t *)&check_crc, 1);
		uint16_t phy_length = VIR_LEN_TO_PHY_LEN(vir_length);

		enc_buf = (uint8_t*)malloc(phy_length);
		if (!enc_buf) {
			ret = BK_ERR_NO_MEM;
			goto end;
		}

		bk_flash_read_bytes(phy_off + 3, enc_buf, phy_length);
		CRC8_Init(&crc8);
		CRC8_Update(&crc8, enc_buf, phy_length);
		free(enc_buf);
		enc_buf = NULL;

		if (crc8.crc == check_crc.crc) {
			back_key = (uint8_t *)malloc(vir_length);
			if (!back_key) {
				ret = BK_ERR_NO_MEM;
				goto end;
			}
			bk_flash_read_bytes(vir_off, back_key, vir_length);
			if (memcmp(back_key, primary_key, primary_key_size) == 0) {
				ret = BK_OK;
				goto end;
			}
		}
	}

	bk_flash_erase_sector(phy_off);
	uint16_t phy_key_size = VIR_LEN_TO_PHY_LEN(primary_key_size);
	uint16_t vir_key_size = FLASH_PHY2VIRTUAL(phy_key_size); // vir_key_size - keysize = data 0

	bk_flash_write_bytes(vir_off, primary_key, vir_key_size);
	enc_buf = (uint8_t*)malloc(phy_key_size);
	if (enc_buf == NULL) {
		ret = BK_ERR_NO_MEM;
		goto end;
	}

	bk_flash_read_bytes(phy_off + 3, (uint8_t *)enc_buf, phy_key_size);
	CRC8_Init(&crc8);
	CRC8_Update(&crc8, enc_buf, phy_key_size);
	bk_flash_write_bytes(phy_off + 2, (uint8_t *)&crc8, 1);
	bk_flash_write_bytes(phy_off, (uint8_t *)&primary_key_size, 2);

	// re-read to ensure update success
	back_key = (uint8_t *)malloc(primary_key_size);
	if (back_key == NULL) {
		ret = BK_ERR_NO_MEM;
		goto end;
	}
	bk_flash_read_bytes(vir_off, back_key, primary_key_size);
	bk_flash_read_bytes(phy_off + 2, (uint8_t *)&check_crc, 1);

	if ((crc8.crc == check_crc.crc) && (memcmp(back_key, primary_key, primary_key_size) == 0)) {
		OTA_LOGI(TAG, "update back pubkey ok\r\n");
		ret = BK_OK;
	} else {
		ret = BK_FAIL;
		OTA_LOGE(TAG, "back pubkey failed\r\n");
		OTA_LOGE(TAG, "crc = %#x,check crc = %#x",crc8.crc, check_crc.crc);
		debug_print_buf("back_key", back_key, primary_key_size);
		debug_print_buf("primary key", primary_key, primary_key_size);
	}

end:
	if (enc_buf != NULL) {
		free(enc_buf);
	}
	if (back_key != NULL) {
		free(back_key);
	}
	return ret;
}

static bk_err_t ota_verify_handle_tlv(const bk_logic_partition_t *partition, ota_verify_t* ota_verify)
{
	uint32_t tlv_off = 0;
	int ret = 0;
	bool image_hash_valid = false;
	bool valid_signature = false;
	bool valid_public_key = false;
	image_tlv_info_t tlv_pro_info,tlv_info;
	image_tlv_t tlv;
	uint8_t pubkey[MAX_PUBKEY_LEN] = {0};
	uint8_t *end = NULL;
	uint8_t* value = NULL;

	(void*)end;
	(void*)valid_public_key;
#if CONFIG_PSA_MBEDTLS
	if (ota_verify->ctx_inited) {
		mbedtls_sha256_finish(&ota_verify->ctx, ota_verify->hash_result);
		mbedtls_sha256_free(&ota_verify->ctx);
		ota_verify->ctx_inited = false;
	}
#endif
	memcpy(&tlv_pro_info, ota_verify->tlv_total + tlv_off, sizeof(image_tlv_info_t));
	tlv_off += sizeof(image_tlv_info_t);
	memcpy(&tlv_info, ota_verify->tlv_total + tlv_pro_info.it_tlv_tot, sizeof(image_tlv_info_t));

	if(tlv_pro_info.it_magic != IMAGE_TLV_PROT_INFO_MAGIC || tlv_info.it_magic != IMAGE_TLV_INFO_MAGIC){
		BK_LOGE(TAG, "tlv magic error!\r\n");
		return BK_ERR_OTA_VALIDATE_TLV_FAIL;
	}

	if(tlv_pro_info.it_tlv_tot +  tlv_info.it_tlv_tot > TLV_TOTAL_SIZE){
		BK_LOGE(TAG, "tlv size error!\r\n");
		return BK_ERR_OTA_VALIDATE_TLV_FAIL;
	}

	while(tlv_off < tlv_pro_info.it_tlv_tot + tlv_info.it_tlv_tot) {
		if(tlv_off == tlv_pro_info.it_tlv_tot){
			tlv_off += sizeof(image_tlv_info_t);
		}
		memcpy(&tlv, ota_verify->tlv_total + tlv_off, sizeof(image_tlv_t));
		tlv_off += sizeof(image_tlv_t);
		value = (uint8_t*)malloc(tlv.it_len);
		if (value == NULL) {
			BK_LOGE(TAG, "OOM tlv_len=%d\r\n", tlv.it_len);
			ret =  BK_ERR_NO_MEM;
			goto out;
		}
		memcpy(value, ota_verify->tlv_total + tlv_off, tlv.it_len);
		tlv_off += tlv.it_len;

		if(tlv.it_type == IMAGE_TLV_SEC_CNT){
#if CONFIG_ANTI_ROLLBACK
			uint32_t img_sec_counter;
			if(tlv.it_len != sizeof(img_sec_counter)){
				BK_LOGE(TAG, "security counter error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL;
				goto out;
			}
			memcpy(&img_sec_counter,value,sizeof(img_sec_counter));
			uint8_t security_counter[64];
			bk_otp_apb_read(OTP_BL2_SECURITY_COUNTER, security_counter, 64);
			uint32_t store_sec_counter = count_bit_one_in_buffer(security_counter,64);
			if(img_sec_counter < store_sec_counter){
				BK_LOGE(TAG, "security counter error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL;
				goto out;
			} else if(img_sec_counter > store_sec_counter){
				if(img_sec_counter > 64*8){
					BK_LOGE(TAG, "security counter overflow!\r\n");
					ret = BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL;
					goto out;
				}
				set_bit_one_in_buffer(security_counter, img_sec_counter);
				bk_otp_apb_update(OTP_BL2_SECURITY_COUNTER, security_counter, 64);
			}
#endif
		} else if(tlv.it_type == IMAGE_TLV_SHA256) {
			if(tlv.it_len != sizeof(ota_verify->hash_result)){
				ret = BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL;
				goto out;
			}
			if(memcmp(value,ota_verify->hash_result,32) != 0){
				BK_LOGE(TAG, "hash error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL;
				goto out;
			}
			image_hash_valid = true;
			BK_LOGI(TAG, "image hash ok\r\n");
		} else if(tlv.it_type == IMAGE_TLV_CUSTOM) {
			memcpy(pubkey, value, tlv.it_len);
			end = pubkey + tlv.it_len;
#if CONFIG_OTA_UPDATE_PUBKEY
			ret = ota_verify_pubkey(partition, ota_verify, pubkey, tlv.it_len);
			if (ret != BK_OK) {
				goto out;
			}
#else
#ifdef OTP_BL2_BOOT_PUBLIC_KEY_HASH
			uint8_t key_hash[32];
			mbedtls_sha256_hash(value,tlv.it_len,key_hash);
			uint8_t pubkey_hash[32];
			bk_otp_apb_read(OTP_BL2_BOOT_PUBLIC_KEY_HASH, pubkey_hash, 32);
			if (memcmp(key_hash, pubkey_hash, 32) != 0) {
				BK_LOGE(TAG, "incorrect public key hash!\r\n");
				ret = BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
				goto out;
			}
#endif
#endif
			valid_public_key = true;
			BK_LOGI(TAG, "public key ok\r\n");
#if CONFIG_PSA_MBEDTLS
		} else if(tlv.it_type == IMAGE_TLV_ECDSA256) {
			if (valid_public_key == false) {
				BK_LOGE(TAG, "OTA has no public key!\r\n");
				ret = BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
				goto out;
			}

			mbedtls_ecdsa_context ctx;
			mbedtls_ecdsa_init(&ctx);
			uint8_t *ppubkey = pubkey;
			ret = ota_import_key(&ppubkey, end);
			if (ret != 0) {
				BK_LOGE(TAG, "import key signature error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
				goto out;
			}

			ret = ota_mbedtls_ecdsa_verify(&ctx, ppubkey, end - ppubkey, ota_verify->hash_result, ota_verify->hash_size,value, tlv.it_len);
			if (ret != 0) {
				BK_LOGE(TAG, "verify signature error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
				goto out;
			}
			mbedtls_ecdsa_free(&ctx);
			valid_signature = true;
			BK_LOGI(TAG, "sig ok\r\n");
#endif
		}
		free(value);
		value = NULL;
	}

	if (image_hash_valid == false) {
		BK_LOGE(TAG, "invalid image hash!\r\n");
		ret = BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL;
	} else if(valid_signature == false) {
		BK_LOGE(TAG, "invalid sig!\r\n");
		ret = BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}

out:
	if (value != NULL) {
		free(value);
		value = NULL;
	}
	end = NULL;
	return ret;
}


ota_verify_t* ota_verify_init(void)
{
	ota_verify_t* ota_verify = (ota_verify_t*)malloc(sizeof(ota_verify_t));
	if (ota_verify == NULL) {
		return NULL;
	}
#if CONFIG_PSA_MBEDTLS
	mbedtls_sha256_init(&ota_verify->ctx);
	mbedtls_sha256_starts(&ota_verify->ctx,0);
	ota_verify->ctx_inited = true;
#endif
	ota_verify->is_verify_supported = true;
	ota_verify->hash_size = 0xFFFFFFFF;
	ota_verify->validate_offset = 0;
	memset(ota_verify->tlv_total,0xFF,TLV_TOTAL_SIZE);
	return ota_verify;
}

bk_err_t ota_verify_receive_data(ota_verify_t* ota_verify, const void* data, int len)
{
	int head_left_len = sizeof(image_header_t) - ota_verify->validate_offset;
	if(head_left_len > 0){
		uint8_t * tmp = (uint8_t *)&ota_verify->hdr;
		if (len < head_left_len) {
			memcpy(tmp + ota_verify->validate_offset, data, len);
		} else {
			memcpy(tmp + ota_verify->validate_offset, data, head_left_len);
		}
	} else {
		ota_verify->hash_size = ota_verify->hdr.ih_hdr_size + ota_verify->hdr.ih_img_size + ota_verify->hdr.ih_protect_tlv_size;
		uint32_t tlv_info_begin = ota_verify->hdr.ih_hdr_size + ota_verify->hdr.ih_img_size;
		if (ota_check_data_is_crc((uint8_t*)&ota_verify->hdr)) {
			ota_verify->is_verify_supported = false;
			return BK_ERR_NOT_SUPPORT;
		}
		uint32_t tlv_info_end = tlv_info_begin + TLV_TOTAL_SIZE;
		if(ota_verify->validate_offset + len >= tlv_info_begin && ota_verify->validate_offset < tlv_info_end) {
			if(ota_verify->validate_offset < tlv_info_begin){
				if(ota_verify->validate_offset + len > tlv_info_end){
					memcpy(ota_verify->tlv_total, data + (tlv_info_begin - ota_verify->validate_offset), TLV_TOTAL_SIZE);
				} else {
					memcpy(ota_verify->tlv_total, data + (tlv_info_begin - ota_verify->validate_offset), ota_verify->validate_offset + len - tlv_info_begin);
				}
			} else {
				memcpy(ota_verify->tlv_total + ota_verify->validate_offset - tlv_info_begin, data, MIN(tlv_info_end-ota_verify->validate_offset,len));
			}
		}
	}
#if CONFIG_PSA_MBEDTLS
	if (ota_verify->validate_offset + len <= ota_verify->hash_size){
		mbedtls_sha256_update(&ota_verify->ctx, data, len);
	} else if(ota_verify->validate_offset < ota_verify->hash_size){
		mbedtls_sha256_update(&ota_verify->ctx, data, ota_verify->hash_size - ota_verify->validate_offset);
	}
#endif
	ota_verify->validate_offset += len;
	return BK_OK;
}

bk_err_t ota_verify_image(const bk_logic_partition_t *partition, ota_verify_t* ota_verify)
{
	if (ota_verify->is_verify_supported == true) {
		return ota_verify_handle_tlv(partition, ota_verify);
	} else {
		return BK_ERR_NOT_SUPPORT;
	}
	
}

void ota_verify_deinit(ota_verify_t* ota_verify)
{
	if (ota_verify != NULL) {
#if CONFIG_PSA_MBEDTLS
		if (ota_verify->ctx_inited) {
			mbedtls_sha256_free(&ota_verify->ctx);
			ota_verify->ctx_inited = false;
		}
#endif
		free(ota_verify);
	}
}