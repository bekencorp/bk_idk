# 日志系统使用说明

## 概述

新的日志系统提供了统一的日志格式和多个详细级别，使日志输出更加清晰、有条理，便于追踪问题和调试。

## 日志级别

通过 `--log-level` 参数可以设置以下日志级别：

- **`quiet`**: 仅显示错误信息
- **`simple`**: 显示关键步骤和错误（默认推荐）
- **`normal`**: 显示正常信息和错误
- **`verbose`**: 显示所有信息和调试信息
- **`debug`**: 显示所有内容，包括详细的调试信息

## 使用方法

### 在 run.sh 中设置

```bash
python3 -B run.py \
  --flash_aes_type FIXED \
  --flash_aes_key ... \
  --log-level simple \  # 添加这一行
  --project security/xip \
  --build
```

### 在命令行中设置

```bash
python3 -B run.py --log-level simple --flash_aes_type FIXED ...
```

## 日志格式说明

### 简洁模式 (simple) - 推荐

```
>>> Starting: Build Process - Project: security/xip, Log Level: simple
>>> Starting: Build Server - Building project: security/xip
<<< Completed: Build Server
>>> Starting: Sign Server - OTA Type: XIP, App Version: 0.0.3
<<< Completed: Sign Server
>>> Starting: Pack Server - AES Type: FIXED, CRC: False
--- Copying signed binaries to pack directory
Pack: all-app.bin (PackDownload)
Encrypt: bl2 (59412 -> 59424 bytes)
Encrypt: xip_a (1867776 -> 1867776 bytes)
<<< Completed: Pack Server
>>> Starting: Install - Copying final files to install directory
<<< Completed: Install
<<< Completed: Build Process - Output: /path/to/install/all-app.bin
```

### 详细模式 (verbose/debug)

在详细模式下，会显示更多技术细节，包括：
- 文件路径
- 加密命令
- 分区偏移和大小
- 内部处理步骤

## 日志标记说明

- `>>>`: 步骤开始标记
- `<<<`: 步骤完成标记
- `---`: 步骤信息标记
- `!!!`: 错误标记（自动添加）

## 关键步骤追踪

日志系统会在以下关键步骤显示清晰的开始/结束标记：

1. **Build Process**: 整个构建流程
2. **Build Server**: 编译服务器
3. **Sign Server**: 签名服务器
4. **Pack Server**: 打包服务器
5. **Install**: 文件安装

## 错误处理

当出现错误时，日志会：
- 使用 `!!!` 标记错误
- 显示错误发生的步骤
- 提供详细的错误信息
- 在简洁模式下仍然显示完整的错误信息

## 示例对比

### 简洁模式输出（simple）

```
>>> Starting: Build Process - Project: security/xip, Log Level: simple
>>> Starting: Pack Server - AES Type: FIXED, CRC: False
Pack: all-app.bin (PackDownload)
Encrypt: bl2 (59412 -> 59424 bytes)
<<< Completed: Pack Server
<<< Completed: Build Process - Output: /path/to/install/all-app.bin
```

### 详细模式输出（verbose）

```
>>> Starting: Build Process - Project: security/xip, Log Level: verbose
>>> Starting: Pack Server - AES Type: FIXED, CRC: False
--- Copying signed binaries to pack directory
Pack: all-app.bin (PackDownload)
Pack: bins=['bl2.bin', 'partition.bin', 'app_signed.bin', ...]
Encrypt: bl2 - executing: python3 .../aes_xts.py encrypt ...
Encrypt: bl2 (59412 -> 59424 bytes)
bin=bl2.bin, type=1, offset=0x0, size=0x13000, flash_crc_en=False
<<< Completed: Pack Server
<<< Completed: Build Process - Output: /path/to/install/all-app.bin
```

## 向后兼容

- 旧的 `--debug` 参数仍然支持，会自动转换为 `--log-level debug`
- 如果不指定 `--log-level`，默认使用 `simple` 模式

## 建议

- **日常使用**: 使用 `--log-level simple`（默认）
- **调试问题**: 使用 `--log-level verbose` 或 `--log-level debug`
- **CI/CD**: 使用 `--log-level normal` 或 `--log-level simple`
