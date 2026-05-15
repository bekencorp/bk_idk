BKSecure 打包工具

安装
1. 从 GitLab 拉取打包工具：
   https://gitlab.bekencorp.com/armino/customer/signify/bk_tools
   （使用分支：debug_bk7236n）
2. 在 bksecure 目录执行安装：
   pip install .

使用方法
安装完成后，执行：
bksecure steps pack --soc_type bk7236n --flash_aes_type {flash_aes_type} --flash_aes_key {flash_aes_key} --ota_type {ota_type} --ota_security_counter {ota_security_counter}

必填参数
- --soc_type：SoC 型号。可选值：bk7236、bk7236n
- --flash_aes_type：Flash AES 类型。可选值：NONE、FIXED、RANDOM
- --flash_aes_key：Flash AES 密钥字符串
- --ota_type：OTA 策略。可选值：XIP、OVERWRITE
- --ota_security_counter：APP 安全计数器（整数）

可选参数
- --debug：启用调试日志（flag）
- --bl1_secureboot_en：启用 BL1 安全启动（flag，默认关闭）
- --boot_ota：启用 Bootloader OTA 支持（flag，默认关闭）
- --ota_encrypt：启用 OTA bin 加密（flag，默认关闭）
- --pubkey_pem：BL2 公钥 PEM 文件路径（默认：root_ec256_pubkey.pem）
- --privkey_pem：BL2 私钥 PEM 文件路径（默认：root_ec256_privkey.pem）
- --bl2_version：BL2 版本覆盖（可选）
- --app_version：APP 版本覆盖（可选）
