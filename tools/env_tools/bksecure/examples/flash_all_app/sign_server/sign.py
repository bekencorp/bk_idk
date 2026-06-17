#!/usr/bin/env python3

import logging
import subprocess
import os
import shutil
import sys

# Add tool_path to sys.path to import Partitions
def add_tool_path_to_syspath(tool_path):
    # Add the parent directory of scripts to sys.path so relative imports work
    # This allows "from .common import *" to work correctly
    if tool_path not in sys.path:
        sys.path.insert(0, tool_path)

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

def normalize_pubkey_to_hex(pubkey_or_path):
    if not isinstance(pubkey_or_path, str):
        return pubkey_or_path

    if not os.path.isfile(pubkey_or_path):
        return pubkey_or_path

    try:
        with open(pubkey_or_path, 'rt') as pem_file:
            pem_data = pem_file.read()
        from cryptography.hazmat.primitives import serialization
        public_key = serialization.load_pem_public_key(pem_data.encode())
        der_data = public_key.public_bytes(
            encoding=serialization.Encoding.DER,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )
        logging.debug(f'Converted pubkey file {pubkey_or_path} to hex: {der_data.hex()}')
        return der_data.hex()
    except Exception as e:
        logging.warning(f'Failed to convert pubkey file {pubkey_or_path} to hex, keep original value: {e}')
        return pubkey_or_path

def sign_server_process(tool_path, soc_type, pubkey_pem, privkey_pem, ota_type, bl2_version, app_version, security_counter, replace_key_en, new_privkey, new_pubkey, debug_opt=''):
    
    app_version_opt = ''
    if app_version is not None:
        app_version_opt = f' --app_version {app_version}'

    bl2_version_opt = ''
    if bl2_version is not None:
        bl2_version_opt = f' --bl2_version {bl2_version}'
    
    cmd = f'{tool_path}/main.py steps sign --soc_type {soc_type} --pubkey_pem {pubkey_pem} --privkey_pem {privkey_pem} --ota_type {ota_type}{bl2_version_opt}{app_version_opt} --security_counter {security_counter} --replace_key_en {replace_key_en} --new_privkey_pem {new_privkey} --new_pubkey_pem {new_pubkey} {debug_opt}'
    save_cmd('Sign server start', cmd)
    run_cmd(cmd)
