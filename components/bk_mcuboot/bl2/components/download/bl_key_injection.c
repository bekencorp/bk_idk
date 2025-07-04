/*
 * bl_key_injection.h
 *
 *  Created on: 2024-10-12
 *      Author: beken-sw-security
 */

#include <stdio.h>
#include <stdint.h>
#include "bl_key_injection.h"
#include "bl_uart.h"
#include "crc32.h"
#include "bl_extflash.h"
#include "bl_config.h"
#include "flash_partition.h"
#include "partitions_gen.h"

extern u8 *g_backup_buf;
extern unsigned int crc32_table[256];

#ifdef CONFIG_ENCRYPT_PRIVATE_KEY
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
// #include "mbedtls/entropy_poll.h"
#include "mbedtls/error.h"

#if CONFIG_TFM_OTP_NSC
#include "tfm_otp_nsc.h"
#endif

// extern int bk_otp_update_s(uint8_t map_id, uint32_t item, uint8_t* buf, uint32_t size);

#define KEY_SIZE 2048
#define BUFFER_SIZE 256 // 2048-bit RSA uses 256-byte buffers

unsigned char hex_char_to_byte(char hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    } else if (hex >= 'a' && hex <= 'f') {
        return hex - 'a' + 10;
    } else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 10;
    }
    return 0;
}

void hex_string_to_bytes(const char *hex_str, unsigned char *byte_array, size_t byte_array_len) {
    size_t hex_str_len = strlen(hex_str);
    for (size_t i = 0; i < byte_array_len && i * 2 < hex_str_len; i++) {
        byte_array[i] = (hex_char_to_byte(hex_str[i * 2]) << 4) | hex_char_to_byte(hex_str[i * 2 + 1]);
    }
}

static int fake_rng(void *p_rng, unsigned char *output, size_t len)
{
    size_t i;

    (void)p_rng;
    for (i = 0; i < len; i++) {
        output[i] = (char)i;
    }

    return 0;
}

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    
// Function to handle error printing
void handle_mbedtls_error(int ret) {
    char error_buf[100];
    mbedtls_strerror(ret, error_buf, sizeof(error_buf));
    printf("Error: %s (Error code: %x)\n", error_buf, ret);
}

// Function to load public key
int load_public_key(const unsigned char *public_key_pem, mbedtls_pk_context *pk) {
    mbedtls_pk_init(pk);
    size_t pem_size = strlen((const char *)public_key_pem) + 1;
    int ret = mbedtls_pk_parse_public_key(pk, public_key_pem, pem_size);
    if (ret != 0) {
        handle_mbedtls_error(ret);
        return ret;
    }

    if (!mbedtls_pk_can_do(pk, MBEDTLS_PK_RSA)) {
        printf("Error: Loaded key is not an RSA key.\n");
        return -1;
    }

    return 0;
}

// Function to load private key
int load_private_key(const unsigned char *private_key_pem, mbedtls_pk_context *pk) {
    mbedtls_pk_init(pk);
    size_t pem_size = strlen((const char *)private_key_pem) + 1;
    int ret = mbedtls_pk_parse_key(pk, private_key_pem, pem_size, NULL, 0,fake_rng,NULL);
    if (ret != 0) {
        handle_mbedtls_error(ret);
        return ret;
    }

    if (!mbedtls_pk_can_do(pk, MBEDTLS_PK_RSA)) {
        printf("Error: Loaded key is not an RSA key.\n");
        return -1;
    }

    return 0;
}

// Function to encrypt data with RSA public key
int rsa_encrypt(const unsigned char *public_key_pem, const unsigned char *plaintext, size_t plaintext_len, unsigned char *encrypted_data, size_t *encrypted_len) {
    mbedtls_pk_context pk;
    mbedtls_rsa_context *rsa;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    int ret;

    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (ret != 0) {
        printf("Failed to seed the RNG! Error code: -0x%04X\n", -ret);
        return 1;
    }

    if (load_public_key(public_key_pem, &pk) != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        return 1;
    }

    rsa = mbedtls_pk_rsa(pk);

    ret = mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    if (ret != 0) {
        handle_mbedtls_error(ret);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        return -1;
    }

    ret = mbedtls_rsa_rsaes_oaep_encrypt(rsa, fake_rng, NULL, NULL , 0 , plaintext_len,plaintext, encrypted_data);
    if (ret != 0) {
        handle_mbedtls_error(ret);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        return -1;
    }

    *encrypted_len = rsa->len;

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return 0;
}

// Function to decrypt data with RSA private key
int rsa_decrypt(const unsigned char *private_key_pem, const unsigned char *encrypted_data, size_t encrypted_len, unsigned char *output, size_t *output_len) {
    mbedtls_pk_context pk;
    mbedtls_rsa_context *rsa;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    int ret;

    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (ret != 0) {
        printf("Failed to seed the RNG! Error code: -0x%04X\n", -ret);
        return 1;
    }

    if (load_private_key(private_key_pem, &pk) != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        return 1;
    }

    rsa = mbedtls_pk_rsa(pk);

    ret = mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    if (ret != 0) {
        handle_mbedtls_error(ret);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        return -1;
    }

    ret = mbedtls_rsa_rsaes_oaep_decrypt(rsa, fake_rng, NULL, NULL, 0, output_len, encrypted_data, output, *output_len);
    if (ret != 0) {
        handle_mbedtls_error(ret);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        return -1;
    }

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return 0;
}

void test_rsa_encryption_decryption() {
    const char *public_key_pem = 
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA6lbiYCn96KVaTY+8O5hj\n"
    "6+yXMKPkwMR4ilJMMi2Z5oTN8K++KaX3OayOCGzIsWeVqsm5bIpks1+Dc9lM4X8k\n"
    "+jr1g75Mb/RohHMWH2Giqx1QsckGvuZYlgHG8okKVw0Q2JY51LYwerAvjS4wTf+e\n"
    "MKqGby7l5mKbtj95xyvvP9PEKcrKXmr8VLcLgZyLVIEDf3Zjc9LaQi7RRnosYDJP\n"
    "cq8Rt6Y02wHN9hhyxgERgEJeziQcXZVXxA8oo23cKLf/WTZhv5NbjLqWEeI5pA3n\n"
    "VTKha783XccFbJZYnGOesi/3C6xF/16keR4+Myvt19TVOlo8boFJqQY9PX2stZuz\n"
    "OQIDAQAB\n"
    "-----END PUBLIC KEY-----";



    const char *private_key_pem = 
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEA6lbiYCn96KVaTY+8O5hj6+yXMKPkwMR4ilJMMi2Z5oTN8K++\n"
    "KaX3OayOCGzIsWeVqsm5bIpks1+Dc9lM4X8k+jr1g75Mb/RohHMWH2Giqx1QsckG\n"
    "vuZYlgHG8okKVw0Q2JY51LYwerAvjS4wTf+eMKqGby7l5mKbtj95xyvvP9PEKcrK\n"
    "Xmr8VLcLgZyLVIEDf3Zjc9LaQi7RRnosYDJPcq8Rt6Y02wHN9hhyxgERgEJeziQc\n"
    "XZVXxA8oo23cKLf/WTZhv5NbjLqWEeI5pA3nVTKha783XccFbJZYnGOesi/3C6xF\n"
    "/16keR4+Myvt19TVOlo8boFJqQY9PX2stZuzOQIDAQABAoIBAACx7QY0va1r1yCZ\n"
    "g2kIVsyHdTvWUpp9Qq/DVnlEIBuaS3uSI93fAYSU5c9x1icLi/q3kjKXlvyetQa6\n"
    "D2OwUzlgRSPkw7iFyK0S488IRLmd8sQyONBGYd8hBnhn7mVLc0A3Mtj2CN8GSH/S\n"
    "Uh+bGVqi2Nb51EH6iyRvXAK6l0Ew08T8UdQFA2kkrUaMQqF5g35HBIblZr5fVSI9\n"
    "GNX2iOjIZU7Yh3HNMoa6BaTpCRoerRkkmAJRKepmiSTVmMnw/9FQPQBw1nRk+SMr\n"
    "izGsbxnEUxMpBfmY9Ja9HykMegu9TeKgKqNuy55O1E80gbGfQirpnz8eEUgEVA9j\n"
    "1GsoAMECgYEA/zVxZ7P1ANqjUDlBwfq/RQTql5SpCn5YlJLrqVnmhguihhJm9DnP\n"
    "Sn/WSChwzoY23rQubmO66VOERC/xOqpUOWSRB7uyBD7Jc3DocEkaQMBOQj6Q4XlF\n"
    "ZdkRvNBzEMKd42dm9sRZulNIgPbfjF7NaAX0IjaiD3WfRlhfnHtSX8ECgYEA6xDg\n"
    "oJ0rfyOyDFBf0cQ/PAmYZrzZngPxOneoGJgqCoHPrWfj1FyGNTKySP8d/fTnfQwK\n"
    "4Z9oh7VB5xd4hHfrQIXGr406iy++t24yGrnGWHJrFUcQMdgWCgA51a/FoEEYalRb\n"
    "WfcdNmZwVkvZ7mQvDNXZTHG8yc/SM69tqtV4sXkCgYBjeqeL7t/Px2nleYwiWuV3\n"
    "t+Th1TbXOky5DVQDHWH4WkytyjqLdIVS2uXiFrFyL2rwAHgg24seLEngcnerkH1o\n"
    "KcVGqGlJslfAQgC41CYBm3Rl6hF7clQU0TpSyUYxAEufOs0t33fCY3hdnN2iz8lt\n"
    "hPSW6YjeUdEWEJikXIXNgQKBgQC0PuvPnNT2l5t2X8cppS7LzZvD47mGtgUqTQhj\n"
    "W4KZLfLeR9RmSVBLyIT2/lC0JC/BLhPF8CMKNI3FMrUh9oY4opu4jzW99q1qICBh\n"
    "Z+RUe9/qn+QmJEi6AHRYi4ChomijV+34bUlEJoarVVxVtRJy6CPIv0oBAc9v/Gu+\n"
    "XBRBGQKBgQDTrMq5ZaMw7d6XDBsvvmiOgcePrqxVj/kFUAU1A3vTe7cO0coOJP0T\n"
    "64hkIO1JkHA9J8Rv2+QIHFfbOWstcCjXaTqrU8oMkHN84+VjdiJbwJBqh0hIIzXM\n"
    "Nb9yhpVu7fB0bl5PLFYgkiFSYMjaDxHi2C8OvCVzUL5oLKhnSxmqmw==\n"
    "-----END RSA PRIVATE KEY-----";

    const unsigned char plaintext[] = "39bfc773bfd62a7fa703744e5cdc65b9f05d64e09cd6c23940fb4f8118fb8331";
    size_t plaintext_len = sizeof(plaintext);
    unsigned char encrypted_data[MBEDTLS_MPI_MAX_SIZE];
    unsigned char decrypted_data[MBEDTLS_MPI_MAX_SIZE];
    size_t encrypted_len = sizeof(encrypted_data);
    size_t decrypted_len = sizeof(decrypted_data);

    printf("Testing RSA encryption and decryption...\n");

    // Encrypt
    if (rsa_encrypt((const unsigned char *)public_key_pem, plaintext, plaintext_len, encrypted_data, &encrypted_len) != 0) {
        printf("Encryption failed\n");
        return;
    }

    printf("Encrypted data: ");
    for (size_t i = 0; i < encrypted_len; i++) {
        printf("%02x", encrypted_data[i]);
    }
    printf("\n");

    const char *encrypted_data_hex = "5863b9a03d07bd8c3974ccc8fc68abe21a70e274a721911ec0d815bca99966a317026d85a71dd9a7e71d3555af6f5b6e9f7d454b7b13b2c8ddcb5cc1c192f8742af1376f89b28e88ab86721e39f1f8ba56e76048dd003018585df8297d719d8306c26fc2d81fe3f813f171c951bcda8e33f2c55093af404a92db4bdf49fbf3da6abbfbcd6ef039dd4f57fcc98ff950b6eeeeacf5365f899ef32f0ff2abe49eda5145ce42c0a90d9062a56932357b00d754d2d8fe41fbe4c720a46db3e98cfbf149664e3a4687518f105d00da7582d4abb5d9796cd0b021c32296bbd4f47cd0cc9c0ab8d7d9d844df9000739fabf11aca441f63d483ecab41817a238a8f00b288";
    // Convert hex input to bytes
    unsigned char encrypted_data_1[MBEDTLS_MPI_MAX_SIZE] = {0};
    hex_string_to_bytes(encrypted_data_hex, encrypted_data_1, encrypted_len);

    printf("Encrypted data: ");
    for (size_t i = 0; i < encrypted_len; i++) {
        printf("%02x", encrypted_data_1[i]);
    }
    printf("\n");
    printf("encrypted_len = {%d}\n",encrypted_len);
    // Decrypt
    if (rsa_decrypt((const unsigned char *)PRIVATE_KEY_PEM, encrypted_data_1, encrypted_len, decrypted_data, &decrypted_len) != 0) {
        printf("Decryption failed\n");
        return;
    }

    printf("Decrypted data: %.*s\n", (int)decrypted_len, decrypted_data);
}

#endif

uint8_t Transmit_key_injection(uint8_t *rx_param, u16 len)
{
	uint8_t status = 0;
	u32 addr;
	u32 crc32;
	u32 v_start_addr;
	uint32_t crc_me = 0xffffffff;

	unsigned char decrypted_data[MBEDTLS_MPI_MAX_SIZE];
	size_t encrypted_len = 256;
	size_t decrypted_len = sizeof(decrypted_data);

	bl_memcpy((u8*)&crc32,&(rx_param[0]),4);
	bl_memcpy((u8*)&addr,&(rx_param[4]),4);

	make_crc32_table();
	u8* tmp_data = (u8 *)&rx_param[4];
	u16 offset = len - 5;
	while (offset--) {	
		crc_me = (crc_me >> 8)^(crc32_table[(crc_me^(*(tmp_data)++))&0xff]);
	}
	if (crc32 != crc_me)
	{
		status = PARAM_ERROR;
	}
	else
	{
		if (rsa_decrypt((const unsigned char *)PRIVATE_KEY_PEM, &(rx_param[8]), encrypted_len, decrypted_data, &decrypted_len) != 0) {
			status = PARAM_ERROR;
		}
		else
		{
			uint32_t map_id = 1;
			uint32_t item = 1;
			uint32_t size = decrypted_len;
			uint8_t* data = (uint8_t*)os_malloc(size*sizeof(uint8_t));
			memset(data, 0, size);
			//hexstr2bin(decrypted_data, data, size);

			// int ret = bk_otp_update_s(map_id,item,data,size);
			// printf("write ret = %d\r\n",ret);
			bl_memset(g_backup_buf, 0x00, 4096);
			bl_flash_read_cbus(0, g_backup_buf, 4096);

			status = ext_flash_erase_sector_or_block_size(SECTOR_ERASE_CMD, 0);
			for (int i = 0; i < 8; i++) {
				g_backup_buf[i] = g_backup_buf[i + 8];
			}
			for (int i = 8; i <= 15; i++) {
				g_backup_buf[i] = 0;
			}
			bl_flash_write_cbus(0, g_backup_buf,4096);

			// printf("after \r\n");
			// bl_memset(g_backup_buf, 0x00, 4096);
			// bl_flash_read_cbus(0, g_backup_buf, 4096);
			// for (int i = 0; i < 8; i++) {
			// 	printf("Byte %d: 0x%02X\n", i, g_backup_buf[i]);
			// }

			os_free(data);
			data = NULL;
			status = STATUS_OK;
		}
	}
	
	return status;
}