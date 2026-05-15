#!/usr/bin/env python3

import logging
import subprocess
import os
import shutil
from bksecure.scripts.security import Security

def save_cmd(description, cmd):
    cmd_list_file = 'pack_cmd_list.txt'
    with open(cmd_list_file, 'a') as f:
        f.write('\r\n')
        f.write(description)
        f.write('\r\n')
        split_line = len(description)*'-'
        f.write(split_line)
        f.write('\r\n')
        f.write(cmd)
        f.write('\r\n')

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def pack_server_process(tool_path, soc_type, pubkey, privkey, bl1_secureboot_en, flash_aes_type, flash_aes_key, ota_type, ota_security_counter, boot_ota, bl2_version=None, app_version=None, debug_opt=''):
    
    s = Security('security.csv')
    flash_crc_en = s.is_flash_crc_enabled()
    cmd = f'{tool_path}/main.py gen otp_efuse --flash_aes_type {flash_aes_type} --flash_crc_en {flash_crc_en} --flash_aes_key {flash_aes_key} --outfile otp_efuse_config.json {debug_opt}'
    save_cmd('Generate OTP/eFuse Configuration', cmd)
    run_cmd(cmd)

    if bl1_secureboot_en:
        bl1_secureboot_en_opt = ' --bl1_secureboot_en'
    else: 
        bl1_secureboot_en_opt = ''

    if boot_ota:
        boot_ota_opt = ' --boot_ota'
    else:
        boot_ota_opt = ''

    app_version_opt = ''
    if app_version is not None:
        app_version_opt = f' --app_version {app_version}'

    bl2_version_opt = ''
    if bl2_version is not None:
        bl2_version_opt = f' --bl2_version {bl2_version}'

    cmd = f'{tool_path}/main.py steps pack --soc_type {soc_type}{bl1_secureboot_en_opt} --flash_aes_type {flash_aes_type} --flash_aes_key {flash_aes_key} --ota_type {ota_type} --ota_security_counter {ota_security_counter}{boot_ota_opt}{bl2_version_opt}{app_version_opt} --pubkey_pem {pubkey} --privkey_pem {privkey} {debug_opt}'
    save_cmd('steps pack', cmd)
    run_cmd(cmd)