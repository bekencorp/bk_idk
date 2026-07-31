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

def pack_server_process(tool_path, soc_type, flash_aes_type, flash_aes_key, ota_type, security_counter, pubkey_pem, privkey_pem, debug_opt=''):
    
    cmd = f'{tool_path}/main.py steps pack --soc_type {soc_type} --flash_aes_type {flash_aes_type} --flash_aes_key {flash_aes_key} --ota_type {ota_type} --ota_security_counter {security_counter} {debug_opt}'
    save_cmd('pack server step 1: generate all-app.bin', cmd)
    run_cmd(cmd)

    cmd = f'{tool_path}/main.py steps gen_ota_bin --soc_type {soc_type} --flash_aes_type {flash_aes_type} --flash_aes_key {flash_aes_key} --pubkey_pem {pubkey_pem} --privkey_pem {privkey_pem} {debug_opt}'
    save_cmd('pack server step 2: generate ota.bin', cmd)
    run_cmd(cmd)