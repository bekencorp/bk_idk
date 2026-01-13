#!/usr/bin/env python3

import os
import json
import logging
from .common import *
from .genbl1 import *
from .save_cmd import *

def bl2_sign_hash(privkey_pem_file, hash, outfile):
    script_dir = get_script_dir()
    bl2_signing_tool = f'{script_dir}/../tools/mcuboot_tools/imgtool.py'
    cmd = f'{bl2_signing_tool} sign-hash -k {privkey_pem_file} -d {hash} -o {outfile}'
    run_cmd(cmd)

def bl2_sign(action_type, key_type, privkey_pem_file, pubkey_pem_file, signature, bin_file, partition_size, version, security_counter, sign_outfile, hash_outfile, root_pubkey_tlv=None, signature_tlv=None):

    logging.debug(f'bl2 sign, action_type={action_type}, key_type={key_type}')
    logging.debug(f'privkey_pem_file={privkey_pem_file}, pubkey_pem_file={pubkey_pem_file}, signature={signature} security_counter={security_counter} version={version}')
    logging.debug(f'bin={bin_file}, sign_outfile={sign_outfile} hash_outfile={hash_outfile} partition_size={partition_size}')
    logging.debug(f'root_pubkey_tlv={root_pubkey_tlv}, signature_tlv={signature_tlv}')

    if action_type == 'sign' and privkey_pem_file != None:
        key_opt = f'-k {privkey_pem_file}'
    else:
        key_opt = f''

    if action_type == 'hash':
        hash_outfile_opt = f'--hash_outfile {hash_outfile}'
    else:
        hash_outfile_opt = ''

    if action_type == 'sign_from_sig':
        signature_opt = f'--signature {signature}'
    else:
        signature_opt = ''

    script_dir = get_script_dir()
    bl2_signing_tool_dir = f'{script_dir}/../tools/mcuboot_tools/imgtool.py'
    
    # Build custom TLV options
    custom_tlv_opts = ''
    
    # 0xA0 TLV: new_img_sign_pubkey (new public key) or normal pubkey
    if pubkey_pem_file != None and type(pubkey_pem_file) != str:
        # pubkey_pem_file is bytes (new_pubkey or root pubkey), add as IMAGE_TLV_CUSTOM (0xa0)
        logging.debug("pubkey is not pem file, add custom tlv")
        custom_value = pubkey_pem_file.hex()
        custom_tlv_opts += f' --custom-tlv 0xa0 0x{custom_value}'
    
    # 0xA1 TLV: img_sign_pubkey (root public key) - only for XIP mode with update
    if root_pubkey_tlv != None:
        logging.debug("add root pubkey as 0xa1 tlv")
        root_pubkey_value = root_pubkey_tlv.hex()
        custom_tlv_opts += f' --custom-tlv 0xa1 0x{root_pubkey_value}'
    
    # 0xA2 TLV: signature of new_pubkey hash (DER format)
    if signature_tlv != None:
        logging.debug("add signature as 0xa2 tlv")
        signature_value = signature_tlv.hex()
        custom_tlv_opts += f' --custom-tlv 0xa2 0x{signature_value}'
    
    if type(pubkey_pem_file) == str and pubkey_pem_file.endswith("pem"):
        logging.debug("pubkey is pem file")
        cmd = f'{bl2_signing_tool_dir} sign {key_opt} --public-key-format hash --max-align 8 --align 1 --version {version} --security-counter {security_counter}{custom_tlv_opts} --pad-header --header-size 0x1000 --slot-size {partition_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 {bin_file} {sign_outfile} --action_type {action_type} {signature_opt} --pubkeyfile {pubkey_pem_file} {hash_outfile_opt} &> sign.txt'
    elif pubkey_pem_file != None:
        cmd = f'{bl2_signing_tool_dir} sign {key_opt} --public-key-format hash --max-align 8 --align 1 --version {version} --security-counter {security_counter}{custom_tlv_opts} --pad-header --header-size 0x1000 --slot-size {partition_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 {bin_file} {sign_outfile} --action_type {action_type} {signature_opt} {hash_outfile_opt}'
    else:
        cmd = f'{bl2_signing_tool_dir} sign {key_opt} --public-key-format hash --max-align 8 --align 1 --version {version} --security-counter {security_counter}{custom_tlv_opts}  --pad-header --header-size 0x1000 --slot-size {partition_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 {bin_file} {sign_outfile} --action_type {action_type} {signature_opt} {hash_outfile_opt}'

    run_cmd(cmd)
    return
