@echo off

@REM XIP
python3 -B run.py ^
  --flash_crc_en False ^
  --flash_aes_type FIXED ^
  --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 ^
  --app_version 0.0.3 ^
  --bl2_version 1.0.3 ^
  --app_security_counter 5 ^
  --ota_version 0.0.4 ^
  --ota_security_counter 5 ^
  --ota_type XIP ^
  --privkey root_ec256_privkey.pem ^
  --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb ^
  --app_slot_size 1867776 ^
  --project security/xip ^
  --log-level simple

@REM OVERWRITE
@REM python3 -B run.py ^
@REM   --flash_crc_en false ^
@REM   --flash_aes_type FIXED ^
@REM   --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 ^
@REM   --app_version 0.0.3 ^
@REM   --bl2_version 1.0.3 ^
@REM   --app_security_counter 5 ^
@REM   --ota_version 0.0.4 ^
@REM   --ota_security_counter 5 ^
@REM   --ota_type OVERWRITE ^
@REM   --privkey root_ec256_privkey.pem ^
@REM   --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb ^
@REM   --app_slot_size 2465792 ^
@REM   --ota_slot_size 1437696 ^
@REM   --project security/overwrite ^
@REM   --log-level simple

@REM #------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
@REM ## if you want to replace pubkey, please compile with replace_pubkey_en and both old key and new key. In this ota, old key will be used to add and verify signature and new key will replace old key after the overwrite upgrade.
@REM # python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --bl2_version 1.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey root_ec256_privkey.pem --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb --replace_key_en --new_privkey ec256_private_key.pem --new_pubkey 3059301306072a8648ce3d020106082a8648ce3d030107034200047853a7803ff40c19e7f892f6e6aa72a2875aeb21b35c732d8aaf69fc9c0311f3547f4ea9720d2674b6d21f373fbbe1ef915b965c319f450a4b08c4cb2e624c39 --app_slot_size 2318336 --ota_slot_size 1437696 --project security/secureboot_no_tfm --clean --build
@REM ## All subsequent singing will use the new key.
@REM # python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --bl2_version 1.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey ec256_private_key.pem --pubkey 3059301306072a8648ce3d020106082a8648ce3d030107034200047853a7803ff40c19e7f892f6e6aa72a2875aeb21b35c732d8aaf69fc9c0311f3547f4ea9720d2674b6d21f373fbbe1ef915b965c319f450a4b08c4cb2e624c39 --app_slot_size 2318336 --ota_slot_size 1437696 --project security/secureboot_no_tfm --clean --build

@REM #======================================================================================================
@REM # run.py Parameter Reference
@REM #======================================================================================================
@REM #
@REM # REQUIRED PARAMETERS:
@REM # --------------------
@REM # --flash_aes_type      Flash AES encryption type
@REM #                       Choices: NONE | FIXED | RANDOM
@REM #
@REM # --bl2_version         Bootloader (BL2) version string
@REM #                       Example: 1.0.3
@REM #
@REM # --app_version         Application version string
@REM #                       Example: 0.0.3
@REM #
@REM # --app_security_counter  Application security counter (integer)
@REM #                         Example: 5
@REM #
@REM # --ota_version         OTA version string
@REM #                       Example: 0.0.4
@REM #
@REM # --ota_security_counter  OTA security counter
@REM #                         Example: 5
@REM #
@REM # --ota_type            OTA upgrade strategy
@REM #                       Choices: XIP | OVERWRITE
@REM #
@REM # --pubkey              BL2 public key (PEM file or hex string)
@REM #                       Example: root_ec256_pubkey.pem
@REM #                       Example: 3059301306072a8648ce3d020106082a8648ce3d0301070342...
@REM #
@REM # --app_slot_size       Virtual APP slot size in bytes (integer)
@REM #                       Example: 1867776
@REM #
@REM # OPTIONAL PARAMETERS:
@REM # --------------------
@REM # --flash_aes_key       Flash AES key (required when flash_aes_type is FIXED)
@REM #                       Example: 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18
@REM #
@REM # --flash_crc_en        Enable flash CRC
@REM #                       Choices: True | False | true | false
@REM #                       Default: Read from security.csv or True
@REM #
@REM # --privkey             BL2 private key PEM file (for signing)
@REM #                       Example: ec256_private_key.pem
@REM #
@REM # --replace_key_en      Enable key replacement (flag, no value needed)
@REM #
@REM # --new_privkey         New BL2 private key PEM file (required with --replace_key_en)
@REM #                       Example: ec256_private_key_new.pem
@REM #
@REM # --new_pubkey          New BL2 public key (required with --replace_key_en)
@REM #                       Example: 3059301306072a8648ce3d020106082a8648ce3d0301070342...
@REM #
@REM # --ota_slot_size       Virtual OTA slot size (required when ota_type is OVERWRITE)
@REM #                       Example: 1437696
@REM #
@REM # --project             Project name (required with --build)
@REM #                       Example: security/xip
@REM #
@REM # --bl1_secureboot_en   Enable BL1 secure boot (flag, no value needed)
@REM #
@REM # --bl2_load_addr       Bootloader load address (required with --bl1_secureboot_en)
@REM #                       Example: 0x02008000
@REM #
@REM # --boot_ota            Enable bootloader OTA support (flag, no value needed)
@REM #
@REM # --build               Rebuild the project before signing/packing (flag, no value needed)
@REM #
@REM # --clean               Clean all temporary files after completion (flag, no value needed)
@REM #
@REM # --debug               Enable debug mode (deprecated, use --log-level debug)
@REM #
@REM # --log-level           Set log verbosity level
@REM #                       Choices: quiet | simple | normal | verbose | debug
@REM #                       Default: simple
@REM #                       - quiet   : Errors only
@REM #                       - simple  : Key steps (default)
@REM #                       - normal  : Normal info
@REM #                       - verbose : Detailed info
@REM #                       - debug   : Everything
@REM #
@REM #======================================================================================================
