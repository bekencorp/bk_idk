概述
-----------------------

本示例通过运行 run.sh（Linux）或 run.bat（Windows）来创建 all-app.bin。具体来说，它演示了如何：

 - 通过对原始应用二进制文件（primary_app.bin）进行签名来创建已签名的应用二进制文件（primary_all_signed.bin）。
 - 通过对原始应用二进制文件（primary_app.bin）使用不同版本进行签名来创建已签名的 OTA 二进制文件（ota_sign.bin）。
 - 如果 nvs_key.bin 不存在，则生成 nvs_key.bin；否则使用现有的 nvs_key.bin。
 - 使用 beken flash AES 密钥加密 bl2.bin、primary_all_signed.bin 和 nvs_key.bin，然后为它们添加 CRC。
 - 从 nvs.csv 创建 nvs.bin，然后使用 nvs_key.bin 加密 nvs.bin。
 - 使用 nvs_key.bin 加密 user_mfr.bin。
 - 根据 pack.json 创建 all-app.bin。

前提条件
-------------------------
此目录中的每个子目录（build_server、sign_server、pack_server）可以是独立的真实服务器。在正确运行 run.py 之前，应满足以下前提条件：

 - beken_utils 已正确部署在签名服务器和打包服务器中。
 - 私钥 PEM 文件存在于签名服务器中。
 - 不可变配置文件 nvs.csv、partitions.csv、pack.csv 和公钥 PEM 文件存在于打包服务器中。
 - 工厂数据 user_mfr.bin 存在于打包服务器中。
 - 如果每个设备使用相同的 NVS 密钥，则 nvs_key.bin 也需要存在于打包服务器中。

如何运行示例
-------------------------

示例：

    python3 -B run.py --flash_crc_en false --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.3 --bl2_version 1.0.3 --app_security_counter 5 --ota_version 0.0.4 --ota_security_counter 5 --ota_type XIP --privkey root_ec256_privkey.pem --pubkey 3059301306072a8648ce3d020106082a8648ce3d03010703420004c70395cd213da2e86daf373412212eaea0d5d87668a7b55edcc3fdc195f9ed75f35bd37b7ac7a0d078edcc96255836d78e9ca45801b8b8a7047c12d03e0dffdb --app_slot_size 1867776 --project security/xip
    
如果 beken_utils 位于 idk/tools/env_tools/，您可以从 idk/projects/security/secureboot_no_tfm 构建引导程序（bl2.bin）和应用（primary_all.bin），然后生成 all-app.bin：

    python3 -B run.py --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --pubkey root_ec256_pubkey.pem --clean --build

您也可以直接运行 run.sh 或 run.bat。命令成功运行后，结果在 install 目录中：

 - all-app.bin - 将所有分区二进制文件合并为单个二进制文件。
 - ota.bin - 用于 OTA 的二进制文件。
 - pack_cmd_list.txt - 用于签名、加密和打包二进制文件的详细命令。

如何将工具集成到您自己的工具中
--------------------------------------------------------

按照以下步骤将 beken 打包工具集成到您的工具中：

 - 查看 run.py 以了解打包流程的概述。
 - 将 sign_server/sign.py 移植到您的真实签名服务器。
 - 将 pack_server/pack.py 移植到您的真实打包服务器。

在哪里找到下载工具：bk_loader
---------------------------------------------------------

beken 下载工具：bk_loader 位于：idk/tools/env_tools/beken_utils/tools/bk_loader_pi。

一些有用的命令：

 - 下载命令：./bk_loader download -p 0 -i ./all_app.bin --reboot 0
 - 读取 UID：./bk_loader readinfo -p 0 --read-uid --reboot 0
 - 读取 Flash：./bk_loader readinfo -p 0 -f 1000-20 --reboot 0
 - 重启：./bk_loader readinfo -p 0 --reboot 1

在哪里找到 NVS 工具
--------------------------------------------------------
第三方 NVS 工具位于：tools/env_tools/beken_utils/scripts/NVS

run.py 参数说明
--------------------------------------------------------

### 必需参数

| 参数                     | 说明                                             | 可选值 / 示例                               |
|-------------------------|--------------------------------------------------|---------------------------------------------|
| `--flash_aes_type`      | Flash AES 加密类型                                | `NONE` \| `FIXED` \| `RANDOM`               |
| `--bl2_version`         | Bootloader (BL2) 版本字符串                       | `1.0.3`                                     |
| `--app_version`         | 应用版本字符串                                    | `0.0.3`                                     |
| `--app_security_counter`| 应用安全计数器（整数）                            | `5`                                         |
| `--ota_version`         | OTA 版本字符串                                    | `0.0.4`                                     |
| `--ota_security_counter`| OTA 安全计数器                                    | `5`                                         |
| `--ota_type`            | OTA 升级策略                                      | `XIP` \| `OVERWRITE`                        |
| `--pubkey`              | BL2 公钥（PEM 文件或十六进制字符串）               | `root_ec256_pubkey.pem` 或十六进制字符串     |
| `--app_slot_size`       | 虚拟 APP slot 大小（字节）                        | `1867776`                                   |

### 可选参数

| 参数                     | 说明                                             | 可选值 / 示例                               |
|-------------------------|--------------------------------------------------|---------------------------------------------|
| `--flash_aes_key`       | Flash AES 密钥（flash_aes_type=FIXED 时必需）     | `73c7bf397f2ad6bf...`                      |
| `--flash_crc_en`        | 启用 Flash CRC                                   | `True` \| `False`（默认：从 csv 读取/True）  |
| `--privkey`             | BL2 私钥 PEM 文件                                 | `ec256_private_key.pem`                     |
| `--replace_key_en`      | 启用密钥替换（标志位）                            | -                                           |
| `--new_privkey`         | 新 BL2 私钥（需配合 --replace_key_en）            | `ec256_private_key_new.pem`                 |
| `--new_pubkey`          | 新 BL2 公钥（需配合 --replace_key_en）            | 十六进制字符串                               |
| `--ota_slot_size`       | OTA slot 大小（ota_type=OVERWRITE 时必需）        | `1437696`                                   |
| `--project`             | 项目名称（需配合 --build）                        | `security/xip`                              |
| `--bl1_secureboot_en`   | 启用 BL1 安全启动（标志位）                       | -                                           |
| `--bl2_load_addr`       | BL2 加载地址（需配合 --bl1_secureboot_en）        | `0x02008000`                                |
| `--boot_ota`            | 启用 bootloader OTA 支持（标志位）                | -                                           |
| `--build`               | 签名/打包前重新编译项目（标志位）                  | -                                           |
| `--clean`               | 完成后清理临时文件（标志位）                       | -                                           |
| `--log-level`           | 设置日志详细级别                                  | `quiet` \| `simple` \| `normal` \| `verbose` \| `debug` |

### 日志级别说明

| 级别       | 说明                |
|-----------|---------------------|
| `quiet`   | 仅错误信息           |
| `simple`  | 关键步骤（默认）      |
| `normal`  | 普通信息             |
| `verbose` | 详细信息             |
| `debug`   | 全部信息             |
