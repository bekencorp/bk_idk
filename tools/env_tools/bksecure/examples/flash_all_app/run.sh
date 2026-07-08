# security/overwrite, OTA OVERWRITE, BL1 secure boot disabled, flash AES enabled.
# Note: In OVERWRITE mode, sign_server will automatically generate primary_all.bin from app.bin before signing.
# The app.bin file will be copied to sign_server directory, and primary_all.bin will be generated from it.

# example 1: OVERWRITE
# python3 -B run.py \
#   --soc_type bk7236n \
#   --flash_aes_type FIXED \
#   --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 \
#   --ota_type OVERWRITE \
#   --bl2_version 1.2.0 \
#   --app_version 1.6.0 \
#   --security_counter 5 \
#   --project security/overwrite \
#   --log-level debug \
#   --build \
#   --clean

# security/xip, OTA XIP, BL1 secure boot disabled, flash AES enabled.
# Note: In XIP mode, sign_server will automatically generate primary_all.bin from app.bin before signing.
# The app.bin file will be copied to sign_server directory, and primary_all.bin will be generated from it.

# example 2: XIP
python3 -B run.py \
  --soc_type bk7236n \
  --flash_aes_type FIXED \
  --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 \
  --ota_type XIP \
  --bl2_version 1.2.0 \
  --app_version 1.6.0 \
  --security_counter 5 \
  --project security/xip \
  --log-level debug \
  --build \
  --clean

#======================================================================================================
# run.py Parameter Reference (see argparse in run.py main())
#======================================================================================================
#
# REQUIRED:
# ---------
# --flash_aes_type      Flash AES type. Choices: NONE | FIXED | RANDOM
#
# --security_counter    App security counter (integer). Example: 5
#
# --ota_type            OTA strategy. Choices: XIP | OVERWRITE
#
# CONDITIONALLY REQUIRED:
# -----------------------
# --flash_aes_key       Flash AES key (hex string). Required when --flash_aes_type is FIXED
#
# --project             Project path under IDK tree. Required when --build is set
#                       Example: security/overwrite
#
# OPTIONAL:
# ---------
# --soc_type            SoC model. Choices: bk7236 | bk7236n. Default: bk7236n
#
# --bl2_version         BL2 version string passed to bksecure steps sign
#
# --app_version         App version string passed to bksecure steps sign
#
# --privkey             BL2 signing private key PEM. Default: root_ec256_privkey.pem
#
# --pubkey              BL2 signing public key PEM. Default: root_ec256_pubkey.pem
#
# --replace_key_en      Enable image signing key replacement (string, not a flag)
#                       Default: False. Use true/True/1/yes to enable; false/False/0/no/none to disable
#
# --new_privkey         New BL2 private key PEM when key replacement is enabled
#
# --new_pubkey          New BL2 public key PEM when key replacement is enabled
#
# FLAGS (no value):
# -----------------
# --boot_ota            Enable bootloader OTA support
#
# --build               Rebuild project (requires --project)
#
# --clean               Clean temporary files after run
#
# --debug               Enable debug logging (deprecated; prefer --log-level debug)
#
# --log-level           Log verbosity. Choices: quiet | simple | normal | verbose | debug
#                       Default: simple
#                       quiet   - errors only
#                       simple  - key steps
#                       normal  - info
#                       verbose - detailed info
#                       debug   - everything
#
#======================================================================================================
