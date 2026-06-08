# BKSecure 安全打包工具

基于 Click 的 Beken 安全镜像生成、签名与打包 CLI。安装后命令名为 `bksecure`，也可在工具目录下执行 `python3 main.py`。

## 安装

1. 从 GitLab 拉取打包工具：  
   https://gitlab.bekencorp.com/armino/customer/signify/bk_tools  
   （分支示例：`debug_bk7236n`）
2. 在 `bksecure/setup.py` 目录执行：

```bash
pip install .
```

3. 查看版本与帮助：

```bash
bksecure --version
bksecure --help
```

## 常用流程

在包含 `partitions.csv`、`bin.csv`、`pack.json`、`security.csv`、`ota.csv` 等配置的工作目录中执行。

**一步打包（读默认配置并签名、打包）：**

```bash
bksecure pack all [--debug]
```

**分步：先签名再生成下载镜像：**

```bash
bksecure steps sign \
  --pubkey_pem root_ec256_pubkey.pem \
  --privkey_pem root_ec256_privkey.pem \
  --ota_type OVERWRITE \
  --security_counter 5 \
  [--bl2_version 1.0.0] [--app_version 0.0.1] \
  [--replace_key_en true] [--new_pubkey_pem ...] [--new_privkey_pem ...] \
  [--debug]

bksecure steps pack \
  --soc_type bk7236n \
  --flash_aes_type FIXED \
  --flash_aes_key <64_hex_chars> \
  --ota_type OVERWRITE \
  [--security_counter 5] \
  [--boot_ota] \
  [--debug]
```

**按 pack.json 打包（可指定密钥与 AES）：**

```bash
bksecure pack json [--pack_json pack.json] [--img_sign_privkey ...] [--flash_aes_key ...] [--flash_crc_en] [--data_aes_key ...] [--debug]
```

端到端示例见：`examples/flash_all_app/run.py`、`examples/flash_all_app/run.sh`。

---

## 命令总览

| 命令组 | 子命令 | 说明 |
|--------|--------|------|
| *(顶层)* | `image_info` | 打印镜像信息 |
| `gen` | `partition` | 由 partitions.csv 生成 partition 头文件 |
| | `ppc` | 由 ppc.csv / gpio_dev.csv 生成 `_ppc.h` |
| | `mpc` | 由 mpc.csv 生成 `_mpc.h` |
| | `security` | 由 security.csv 生成 `security.h` |
| | `ota` | 由 ota.csv 生成 `_ota.h` |
| | `otp` | 由 otp2.csv 生成 OTP 映射 |
| | `otp_efuse` | 生成 `otp_efuse_config.json` |
| | `all` | 根据安全配置一次性生成全部代码头文件 |
| `pack` | `compress` | 压缩 OTA 二进制（OVERWRITE） |
| | `insert_pk_hash` | 向二进制插入公钥哈希 |
| | `get_pk_hash` | 从 PEM 公钥计算哈希 |
| | `json` | 按 pack.json 打包 |
| | `all` | 按默认配置完整打包 |
| `steps` | `sign` | 对当前目录镜像执行签名流程 |
| | `pack` | 加密并生成下载 bin |
| | `pack_csv` | 按 CSV 配置打包 |
| | `get_app_bin_hash` / `sign_app_bin_hash` / `sign_from_app_sig` | 分步：应用镜像哈希签名 |
| | `get_ota_bin_hash` / `sign_ota_bin_hash` / `sign_from_ota_sig` | 分步：OTA 镜像哈希签名 |

各子命令均支持 `--debug` 开启详细日志（除另有说明外）。

---

## `steps sign` 参数

| 参数 | 说明 |
|------|------|
| `--pubkey_pem` | 镜像签名公钥 PEM（默认：`img_sign_pubkey`） |
| `--privkey_pem` | 镜像签名私钥 PEM（默认：`img_sign_privkey`） |
| `--ota_type` | OTA 策略：`XIP` \| `OVERWRITE` |
| `--bl2_version` | 覆盖 BL2 版本（可选） |
| `--app_version` | 覆盖 APP 版本（可选） |
| `--security_counter` | 安全计数器（整数，可选） |
| `--replace_key_en` | 是否换钥（字符串，默认 `False`；`true`/`false` 等，见工具内解析） |
| `--new_pubkey_pem` | 新公钥 PEM（换钥时） |
| `--new_privkey_pem` | 新私钥 PEM（换钥时） |
| `--debug` | 调试日志 |

## `steps pack` 参数

| 参数 | 说明 |
|------|------|
| `--soc_type` | SoC：`bk7236` \| `bk7236n`（默认 `bk7236n`） |
| `--flash_aes_type` | **必填** Flash AES：`NONE` \| `FIXED` \| `RANDOM` |
| `--flash_aes_key` | Flash AES 密钥（`FIXED` 时需提供） |
| `--ota_type` | OTA 策略：`XIP` \| `OVERWRITE` |
| `--security_counter` | 安全计数器（可选） |
| `--boot_ota` | 启用 Bootloader OTA（flag） |
| `--debug` | 调试日志 |

## `gen otp_efuse` 参数（节选）

| 参数 | 说明 |
|------|------|
| `--flash_aes_type` | **必填** `FIXED` \| `RANDOM` \| `NONE` |
| `--flash_crc_en` | Flash CRC：`True` / `False` |
| `--flash_aes_key` | AES 密钥 |
| `--pubkey_pem_file` | 安全启动公钥 PEM（默认 `root_ec256_pubkey.pem`） |
| `--secure_boot` | 启用安全启动（flag） |
| `--outfile` | 输出 JSON（默认 `otp_efuse_config.json`） |

## `gen partition` 参数（节选）

| 参数 | 说明 |
|------|------|
| `--partition_csv` | 分区表 CSV（默认 `partitions.csv`） |
| `--ota_type` | **必填** `OVERWRITE` \| `XIP` |
| `--out_hdr_file` / `--out_layout_file` | 输出头文件路径 |

更完整的选项说明请执行：

```bash
bksecure <group> <command> --help
# 例如：
bksecure steps pack --help
bksecure gen otp_efuse --help
```
