Overview
-----------------------

This example create all-app.bin by running run.sh (Linux) or run.bat (Windows). To be specific, it demos how to:

 - create signed app bin (primary_all_signed.bin) by signing raw app bin (primary_app.bin).
 - create signed OTA bin (ota_sign.bin) by signing raw app bin (primary_app.bin) with different version
 - generate nvs_key.bin if nvs_key.bin not exists; use existing nvs_key.bin otherwise
 - encrypt bl2.bin, primary_all_signed.bin and nvs_key.bin with beken flash AES key, and then add CRC to them
 - create nvs.bin from nvs.csv, then encrypt nvs.bin with nvs_key.bin
 - encrypt user_mfr.bin with nvs_key.bin
 - create all-app.bin per pack.json

Assumption
-------------------------
Each sub-directory (build_server, sign_server, pack_server) in this directory can be a sperate real server. Following assumption should be meet before
we can correctly run run.py:

 - beken_utils is correctly deployed in sign server and pack server
 - the private PEM key file exists in sign server
 - the immutable configuration file nvs.csv, partitions.csv, pack.csv and public PEM key file exists in pack server
 - the factory data user_mfr.bin exists in pack server
 - if each device use same NVS key, then nvs_key.bin also need to be existed in pack server

How to run example
-------------------------

Example:

    python3 -B run.py --flash_crc_en false --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.3 --bl2_version 1.0.3 --app_security_counter 5 --ota_version 0.0.4 --ota_security_counter 5 --ota_type XIP --privkey root_ec256_privkey.pem --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb --app_slot_size 1867776 --project security/xip

If the beken_utils is located in idk/tools/env_tools/, you can build bootloader (bl2.bin) and app (primary_all.bin) from idk/projects/security/secureboot_no_tfm, and then generate all-app.bin:

    python3 -B run.py --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --pubkey root_ec256_pubkey.pem --clean --build

You can also run run.sh or run.bat directly. After the command run successfully, the result is in install directory:

 - all-app.bin - combines all partition binary into a single binary blob.
 - ota.bin - the binary used for OTA
 - pack_cmd_list.txt - the detailed commands used to sign, encrypt and pack binaries.

How to integrate the tool to your owner tool
--------------------------------------------------------

Take following steps to integrate the beken pack tool into your tool:

 - Check run.py to gain overview understanding about the pack flow
 - Port sign_server/sign.py to your real sign server
 - Port pack_server/pack.py to your real pack server

Where to find the download tool: bk_loader
---------------------------------------------------------

The beken download tool: bk_loader is placed under: idk/tools/env_tools/beken_utils/tools/bk_loader_pi.

Some useful commands:

 - download command: ./bk_loader download -p 0 -i ./all_app.bin --reboot 0
 - read uid: ./bk_loader readinfo -p 0 --read-uid --reboot 0
 - read flash: ./bk_loader readinfo -p 0 -f 1000-20 --reboot 0
 - reboot: ./bk_loader readinfo -p 0 --reboot 1

Where to find the NVS tool
--------------------------------------------------------
The third-party NVS tool is place in: tools/env_tools/beken_utils/scripts/NVS

run.py Parameter Reference
--------------------------------------------------------

### Required Parameters

| Parameter               | Description                                      | Choices / Example                          |
|-------------------------|--------------------------------------------------|---------------------------------------------|
| `--flash_aes_type`      | Flash AES encryption type                        | `NONE` \| `FIXED` \| `RANDOM`               |
| `--bl2_version`         | Bootloader (BL2) version string                  | `1.0.3`                                     |
| `--app_version`         | Application version string                       | `0.0.3`                                     |
| `--app_security_counter`| Application security counter (integer)           | `5`                                         |
| `--ota_version`         | OTA version string                               | `0.0.4`                                     |
| `--ota_security_counter`| OTA security counter                             | `5`                                         |
| `--ota_type`            | OTA upgrade strategy                             | `XIP` \| `OVERWRITE`                        |
| `--pubkey`              | BL2 public key (PEM file or hex string)          | `root_ec256_pubkey.pem` or hex string       |
| `--app_slot_size`       | Virtual APP slot size in bytes                   | `1867776`                                   |

### Optional Parameters

| Parameter               | Description                                      | Choices / Example                          |
|-------------------------|--------------------------------------------------|---------------------------------------------|
| `--flash_aes_key`       | Flash AES key (required when flash_aes_type=FIXED)| `73c7bf397f2ad6bf...`                      |
| `--flash_crc_en`        | Enable flash CRC                                 | `True` \| `False` (default: from csv/True) |
| `--privkey`             | BL2 private key PEM file                         | `ec256_private_key.pem`                     |
| `--replace_key_en`      | Enable key replacement (flag)                    | -                                           |
| `--new_privkey`         | New BL2 private key (with --replace_key_en)      | `ec256_private_key_new.pem`                 |
| `--new_pubkey`          | New BL2 public key (with --replace_key_en)       | hex string                                  |
| `--ota_slot_size`       | OTA slot size (required when ota_type=OVERWRITE) | `1437696`                                   |
| `--project`             | Project name (required with --build)             | `security/xip`                              |
| `--bl1_secureboot_en`   | Enable BL1 secure boot (flag)                    | -                                           |
| `--bl2_load_addr`       | BL2 load address (with --bl1_secureboot_en)      | `0x02008000`                                |
| `--boot_ota`            | Enable bootloader OTA support (flag)             | -                                           |
| `--build`               | Rebuild project before signing/packing (flag)    | -                                           |
| `--clean`               | Clean temporary files after completion (flag)    | -                                           |
| `--log-level`           | Set log verbosity level                          | `quiet` \| `simple` \| `normal` \| `verbose` \| `debug` |

### Log Level Description

| Level     | Description              |
|-----------|--------------------------|
| `quiet`   | Errors only              |
| `simple`  | Key steps (default)      |
| `normal`  | Normal info              |
| `verbose` | Detailed info            |
| `debug`   | Everything               |