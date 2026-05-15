BKSecure Packaging Tool

Installation
1. Clone the packaging tool from GitLab:
   https://gitlab.bekencorp.com/armino/customer/signify/bk_tools
   (use branch: debug_bk7236n)
2. Run installation in the bksecure directory:
   pip install .

Usage
After installation, run:
bksecure steps pack --soc_type bk7236n --flash_aes_type {flash_aes_type} --flash_aes_key {flash_aes_key} --ota_type {ota_type} --ota_security_counter {ota_security_counter}

Required parameters
- --soc_type: SoC model. Choices: bk7236, bk7236n
- --flash_aes_type: Flash AES type. Choices: NONE, FIXED, RANDOM
- --flash_aes_key: Flash AES key string
- --ota_type: OTA strategy. Choices: XIP, OVERWRITE
- --ota_security_counter: APP security counter (integer)

Optional parameters
- --debug: Enable debug logging (flag)
- --bl1_secureboot_en: Enable BL1 secure boot (flag, default: disabled)
- --boot_ota: Enable bootloader OTA support (flag, default: disabled)
- --ota_encrypt: Enable OTA binary encryption (flag, default: disabled)
- --pubkey_pem: BL2 public key PEM file path (default: root_ec256_pubkey.pem)
- --privkey_pem: BL2 private key PEM file path (default: root_ec256_privkey.pem)
- --bl2_version: BL2 version override (optional)
- --app_version: APP version override (optional)
