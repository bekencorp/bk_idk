# Get sign slot size of secureboot_no_tfm
#../../main.py sign sign_args --partition_csv ./pack_server/immutable/partitions.csv --ota_type OVERWRITE

# secureboot_no_tfm, OTA XIP, BL1 secure boot disabled, flash fixed AES key.
# Note: In XIP mode, sign_server will automatically generate primary_all.bin from app.bin before signing.
# python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --privkey root_ec256_privkey.pem --pubkey root_ec256_pubkey.pem --app_slot_size 1605632 --project security/secureboot_no_tfm --clean --build

# security/xip, OTA XIP, BL1 secure boot disabled, flash AES disabled.
# Note: In XIP mode, sign_server will automatically generate primary_all.bin from app.bin before signing.
# The app.bin file will be copied to sign_server directory, and primary_all.bin will be generated from it.

# example 1: XIP
python3 -B run.py \
  --flash_crc_en false \
  --flash_aes_type FIXED \
  --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 \
  --app_version 0.0.3 \
  --bl2_version 1.0.3 \
  --app_security_counter 5 \
  --ota_version 0.0.4 \
  --ota_security_counter 5 \
  --ota_type XIP \
  --privkey root_ec256_privkey.pem \
  --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb \
  --app_slot_size 1867776 \
  --project security/xip \
  --log-level simple  \
  --build

# example 2: OVERWRITE
# python3 -B run.py \
#   --flash_crc_en false \
#   --flash_aes_type FIXED \
#   --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 \
#   --app_version 0.0.3 \
#   --bl2_version 1.0.3 \
#   --app_security_counter 5 \
#   --ota_version 0.0.4 \
#   --ota_security_counter 5 \
#   --ota_type OVERWRITE \
#   --privkey root_ec256_privkey.pem \
#   --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb \
#   --app_slot_size 2465792 \
#   --ota_slot_size 1437696 \
#   --project security/overwrite \
#   --log-level simple \
#   --build
#------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
## if you want to replace pubkey, please compile with replace_pubkey_en and both old key and new key. In this ota, old key will be used to add and verify signature and new key will replace old key after the overwrite upgrade.
# python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --bl2_version 1.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey root_ec256_privkey.pem --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb --replace_key_en --new_privkey ec256_private_key.pem --new_pubkey 3059301306072a8648ce3d020106082a8648ce3d030107034200047853a7803ff40c19e7f892f6e6aa72a2875aeb21b35c732d8aaf69fc9c0311f3547f4ea9720d2674b6d21f373fbbe1ef915b965c319f450a4b08c4cb2e624c39 --app_slot_size 2318336 --ota_slot_size 1437696 --project security/secureboot_no_tfm --clean --build
## All subsequent singing will use the new key.
# python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --bl2_version 1.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey ec256_private_key.pem --pubkey 3059301306072a8648ce3d020106082a8648ce3d030107034200047853a7803ff40c19e7f892f6e6aa72a2875aeb21b35c732d8aaf69fc9c0311f3547f4ea9720d2674b6d21f373fbbe1ef915b965c319f450a4b08c4cb2e624c39 --app_slot_size 2318336 --ota_slot_size 1437696 --project security/secureboot_no_tfm --clean --build

#======================================================================================================
# run.py Parameter Reference
#======================================================================================================
#
# REQUIRED PARAMETERS:
# --------------------
# --flash_aes_type      Flash AES encryption type
#                       Choices: NONE | FIXED | RANDOM
#
# --bl2_version         Bootloader (BL2) version string
#                       Example: 1.0.3
#
# --app_version         Application version string
#                       Example: 0.0.3
#
# --app_security_counter  Application security counter (integer)
#                         Example: 5
#
# --ota_version         OTA version string
#                       Example: 0.0.4
#
# --ota_security_counter  OTA security counter
#                         Example: 5
#
# --ota_type            OTA upgrade strategy
#                       Choices: XIP | OVERWRITE
#
# --pubkey              BL2 public key (PEM file or hex string)
#                       Example: root_ec256_pubkey.pem
#                       Example: 3059301306072a8648ce3d020106082a8648ce3d0301070342...
#
# --app_slot_size       Virtual APP slot size in bytes (integer)
#                       Example: 1867776
#
# OPTIONAL PARAMETERS:
# --------------------
# --flash_aes_key       Flash AES key (required when flash_aes_type is FIXED)
#                       Example: 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18
#
# --flash_crc_en        Enable flash CRC
#                       Choices: True | False | true | false
#                       Default: Read from security.csv or True
#
# --privkey             BL2 private key PEM file (for signing)
#                       Example: ec256_private_key.pem
#
# --replace_key_en      Enable key replacement (flag, no value needed)
#
# --new_privkey         New BL2 private key PEM file (required with --replace_key_en)
#                       Example: ec256_private_key_new.pem
#
# --new_pubkey          New BL2 public key (required with --replace_key_en)
#                       Example: 3059301306072a8648ce3d020106082a8648ce3d0301070342...
#
# --ota_slot_size       Virtual OTA slot size (required when ota_type is OVERWRITE)
#                       Example: 1437696
#
# --project             Project name (required with --build)
#                       Example: security/xip
#
# --bl1_secureboot_en   Enable BL1 secure boot (flag, no value needed)
#
# --bl2_load_addr       Bootloader load address (required with --bl1_secureboot_en)
#                       Example: 0x02008000
#
# --boot_ota            Enable bootloader OTA support (flag, no value needed)
#
# --build               Rebuild the project before signing/packing (flag, no value needed)
#
# --clean               Clean all temporary files after completion (flag, no value needed)
#
# --debug               Enable debug mode (deprecated, use --log-level debug)
#
# --log-level           Set log verbosity level
#                       Choices: quiet | simple | normal | verbose | debug
#                       Default: simple
#                       - quiet   : Errors only
#                       - simple  : Key steps (default)
#                       - normal  : Normal info
#                       - verbose : Detailed info
#                       - debug   : Everything
#
#======================================================================================================
