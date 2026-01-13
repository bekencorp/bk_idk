#!/usr/bin/env python3

import logging
import json
import struct
import os
import shutil
from .gen_ppc import *
from .gen_mpc import *
from .gen_security import *
from .gen_ota import *
from .gen_otp import *
from .bl1_sign import *
from .bl2_sign import *
from .partition import *
from .compress import *
from .gen_ppc import *
from .pk_hash import *
from .partitions import *
from .pack_json import *
from .common import copy_files as copy_all_files, get_config_paths

def copy_files_by_postfix(postfix, src_dir, dst_dir):
    """Copy files with specific postfix from src_dir to dst_dir"""
    if not os.path.exists(src_dir) or not os.path.isdir(src_dir):
        return
    logging.debug(f'copy *{postfix} from {src_dir} to {dst_dir}')
    for f in os.listdir(src_dir):
        if f.endswith(postfix):
            src_path = os.path.join(src_dir, f)
            dst_path = os.path.join(dst_dir, f)
            if os.path.isfile(src_path):
                shutil.copy(src_path, dst_path)

def install_configs(cfg_dir, install_dir):
    """Install config files (csv, pem, json, bin) from cfg_dir to install_dir"""
    if not os.path.exists(cfg_dir):
        return
    
    logging.debug(f'install configs from: {cfg_dir}')
    logging.debug(f'to: {install_dir}')
    
    # Ensure install directory exists
    os.makedirs(install_dir, exist_ok=True)
    
    # Copy common config files
    copy_files_by_postfix('.bin', cfg_dir, install_dir)
    copy_files_by_postfix('.json', cfg_dir, install_dir)
    copy_files_by_postfix('.pem', cfg_dir, install_dir)
    copy_files_by_postfix('.csv', cfg_dir, install_dir)
    
    # Copy files from subdirectories if they exist
    if os.path.exists(os.path.join(cfg_dir, 'key')):
        copy_files_by_postfix('.pem', os.path.join(cfg_dir, 'key'), install_dir)
    if os.path.exists(os.path.join(cfg_dir, 'csv')):
        copy_files_by_postfix('.csv', os.path.join(cfg_dir, 'csv'), install_dir)
    if os.path.exists(os.path.join(cfg_dir, 'regs')):
        copy_files_by_postfix('.csv', os.path.join(cfg_dir, 'regs'), install_dir)

def check_exist(file_name):
    if not os.path.exists(file_name):
        logging.error(f'{file_name} NOT exists')
        exit(1)

def check_config():
    check_exist('bl2.bin')
    check_exist('tfm_s.bin')
    check_exist('cpu0_app.bin')
    check_exist('security.csv')
    check_exist('ota.csv')
    check_exist('partitions.csv')

def get_hash(file_name):
    with open(file_name, 'r') as f:
        d = json.load(f)
        return d['hash']

def get_app_sig(file_name):
    with open(file_name, 'r') as f:
        d = json.load(f)
        return d['signature']

#Step1 - get hash of app binary and hash of manifest
def get_app_bin_hash():
    s = Security('security.csv')
    o = OTA('ota.csv')
    ota_type = o.get_strategy().upper()
    
    ph = PackHeader("bl2.bin")
    name = ph.name()
    infile = f'_{name}.bin'

    static_addr_int = 0x02000000 + ph.size().vir_code_offset()
    static_addr = f'0x%08x' %(static_addr_int)
    load_addr = static_addr

    bl1_sign('hash', s.img_sign_key_type, s.img_sign_privkey, s.img_sign_pubkey, None, infile, static_addr, load_addr, 'primary_manifest.bin')

    ph = PackHeader("overwrite.bin")
    name = ph.name()
    infile = f'_{name}.bin'
    if s.update_img_sign_key_en:
        bl2_sign('hash', s.img_sign_key_type, s.new_img_sign_privkey, s.new_img_sign_pubkey_bytes, None, infile, ph.size().sign_size(), ph.version(), ph.security_counter(), 'app_signed.bin', 'app_hash.json')
    else:
        bl2_sign('hash', s.img_sign_key_type, s.img_sign_privkey, s.img_sign_pubkey_bytes, None, infile, ph.size().sign_size(), ph.version(), ph.security_counter(), 'app_signed.bin', 'app_hash.json')

#Step2 - generate signature from app/manifest hash, do it in server has private key
def sign_app_bin_hash(bl2_bin_hash=None, app_bin_hash=None):
    s = Security('security.csv')
    if bl2_bin_hash == None:
        bl2_bin_hash = get_hash('manifest_hash.json')

    if app_bin_hash == None:
        app_bin_hash = get_hash('app_hash.json')

    bl1_sign_hash(s.img_sign_privkey, bl2_bin_hash, 'manifest_sig.json')
    if s.update_img_sign_key_en:
        bl2_sign_hash(s.new_img_sign_privkey, app_bin_hash, 'app_sig.json')
    else:
        bl2_sign_hash(s.img_sign_privkey, app_bin_hash, 'app_sig.json')

#Step3 - generate signed bin from signature
def sign_from_app_sig(bl2_sig_s, bl2_sig_r, app_sig):
    s = Security('security.csv')
    ph = PackHeader("bl2.bin")
    name = ph.name()
    infile = f'_{name}.bin'

    static_addr_int = 0x02000000 + ph.size().vir_code_offset()
    static_addr = f'0x%08x' %(static_addr_int)
    load_addr = static_addr

    if s.bl1_secureboot_en:
        with open('bl1_signature.txt', 'w') as f:
            f.write(bl2_sig_s)
            f.write("\r\n")
            f.write(bl2_sig_r)
        bl1_sign('sign_from_sig', s.img_sign_key_type, s.img_sign_privkey, s.img_sign_pubkey, None, infile, static_addr, load_addr,  'primary_manifest.bin')

    o = OTA('ota.csv')
    ota_type = o.get_strategy().upper()
    
    logging.debug(f"sign_from_app_sig privkey = {s.img_sign_privkey}")
    
    ph = PackHeader("overwrite.bin")
    name = ph.name()
    infile = f'_{name}.bin'
    
    # For XIP mode with update_img_sign_key_en, generate TLV for new format:
    #   0xA0: new_img_sign_pubkey (new public key)
    #   0xA1: img_sign_pubkey (root public key)
    #   0xA2: signature of new_pubkey hash (signed by root key, DER format)
    new_pubkey_tlv = None  # 0xA0: new_img_sign_pubkey
    root_pubkey_tlv = None  # 0xA1: img_sign_pubkey (root key)
    signature_tlv = None  # 0xA2: signature of new_pubkey hash (DER format)
    
    if s.update_img_sign_key_en and ota_type == 'XIP':
        # 0xA0: new_img_sign_pubkey (new public key)
        new_pubkey_tlv = s.new_img_sign_pubkey_bytes
        
        # 0xA1: img_sign_pubkey (root public key)
        root_pubkey_tlv = s.img_sign_pubkey_bytes
        
        # 0xA2: Generate signature of new_pubkey hash using root key
        import hashlib
        new_pubkey_hash = hashlib.sha256(s.new_img_sign_pubkey_bytes).digest()
        # Sign the hash with root key (imgtool sign-hash returns DER format signature)
        bl2_sign_hash(s.img_sign_privkey, new_pubkey_hash.hex(), 'new_pubkey_sig.json')
        # Read signature (DER format, hex string)
        with open('new_pubkey_sig.json', 'r') as f:
            sig_data = json.load(f)
            sig_der_hex = sig_data['signature']
        # Convert hex string to bytes (DER format)
        signature_tlv = bytes.fromhex(sig_der_hex)
    
    if s.update_img_sign_key_en:
        bl2_sign('sign_from_sig', s.img_sign_key_type, s.new_img_sign_privkey, new_pubkey_tlv, app_sig, infile, ph.size().sign_size(), ph.version(), ph.security_counter(), 'app_signed.bin', 'app_hash.json', root_pubkey_tlv=root_pubkey_tlv, signature_tlv=signature_tlv)
    else:
        bl2_sign('sign_from_sig', s.img_sign_key_type, s.img_sign_privkey, s.img_sign_pubkey_bytes, app_sig, infile, ph.size().sign_size(), ph.version(), ph.security_counter(), 'app_signed.bin', 'app_hash.json')

#Step4 - get hash of ota binary
def get_ota_bin_hash():
    pheader = PackHeader('overwrite.bin')
    s = Security('security.csv')
    compress_bin('app_signed.bin', 'compress.bin')
    bl2_sign('hash', s.img_sign_key_type, s.img_sign_privkey, s.img_sign_pubkey_bytes, None, 'compress.bin', pheader.m2(), pheader.version(), pheader.security_counter(), 'ota_signed.bin', 'ota_hash.json')

#Step5 - generate signature from ota bin hash, do it in server has private key
def sign_ota_bin_hash(ota_hash):
    s = Security('security.csv')
    bl2_sign_hash(s.img_sign_privkey, ota_hash, 'ota_sig.json')

#Step6 - generate ota.bin from ota signature
def sign_from_ota_sig(ota_bin_sig):
    pheader = PackHeader('overwrite.bin')
    s = Security('security.csv')

    bl2_sign('sign_from_sig', s.img_sign_key_type, s.img_sign_privkey, s.img_sign_pubkey_bytes, ota_bin_sig, 'compress.bin', pheader.m2(), pheader.version(), pheader.security_counter(), 'ota_signed.bin', 'ota_hash.json')

#Step7 - pack download bin
# def steps_pack(aes_key_type=None, aes_key=None, bl1_secureboot_en=False, ota_type='XIP', ota_security_counter=0, ota_encrypt='FALSE', boot_ota=False, bl2_version="v1.0.0"):
    # if aes_key_type != None:
    #     flash_aes_type = aes_key_type
    #     flash_aes_key = aes_key
    # elif os.path.exists('security.csv'):
    #     s = Security('security.csv')
    #     flash_aes_key = s.flash_aes_key 
    #     flash_aes_type = s.flash_aes_type
    # else:
    #     flash_aes_type = 'NONE'
    #     flash_aes_key = ''

    # logging.debug(f'steps_pack, bl1_secureboot_en={bl1_secureboot_en}, ota_type={ota_type}, ota_security_counter={ota_security_counter}, ota_encrypt={ota_encrypt}')
    # p = Partitions('partitions.csv', ota_type, boot_ota, bl1_secureboot_en, bl2_version)
    # p.pack_bin('pack.json', flash_aes_type, flash_aes_key, ota_security_counter, ota_encrypt, boot_ota, False)
    # p.install_bin()

def steps_pack_csv():
    logging.debug(f'steps pack csv')
    
    cwd = os.getcwd()
    
    # Dynamically discover config paths
    config_paths = get_config_paths(current_dir=cwd)
    
    # Copy config files from found directories to current directory
    for cfg_dir in config_paths:
        if os.path.exists(cfg_dir):
            install_configs(cfg_dir, cwd)
    
    # Now all config files should be in current directory, proceed with packing
    s = Security('security.csv')
    # p = Partitions('partitions.csv', 'bin.csv', 'pack.json')
    # p.postbuild_process()
    pack = PackJson()
    pack.pack('steps_pack.json', s.img_sign_pubkey, None, s.flash_aes_key, s.flash_crc_en, None)
    gen_otp_efuse_config_file(s.flash_aes_type, s.flash_aes_key, s.img_sign_pubkey, s.bl1_secureboot_en, False, 'otp_efuse_config.json')
    pack.install_bin()
    insert_pk_hash('bootloader.bin', s.img_sign_pubkey)
