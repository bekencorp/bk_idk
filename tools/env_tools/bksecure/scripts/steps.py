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
    
    ph = PackHeader("overwrite.bin")
    name = ph.name()
    infile = f'_{name}.bin'
    
    new_pubkey_tlv = None
    root_pubkey_tlv = None
    signature_tlv = None
    
    if s.update_img_sign_key_en and ota_type == 'XIP':
        new_pubkey_tlv = s.new_img_sign_pubkey_bytes
        root_pubkey_tlv = s.img_sign_pubkey_bytes
        
        bl2_sign_hash(s.img_sign_privkey, s.new_img_sign_pubkey_bytes.hex(), 'new_pubkey_sig.json')
        with open('new_pubkey_sig.json', 'r') as f:
            sig_data = json.load(f)
            sig_der_hex = sig_data['signature']
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
def steps_pack(aes_key_type=None, aes_key=None, flash_crc_en=None, bl1_secureboot_en=False, ota_type='XIP', ota_security_counter=0, ota_encrypt='FALSE', boot_ota=False, bl2_version='1.0.0', app_version='1.0.0', pubkey_pem=None, privkey_pem=None):
    s = None
    if aes_key_type != None:
        flash_aes_type = aes_key_type
        flash_aes_key = aes_key
    elif os.path.exists('security.csv'):
        s = Security('security.csv')
        flash_aes_key = s.flash_aes_key 
        flash_aes_type = s.flash_aes_type
    else:
        flash_aes_type = 'NONE'
        flash_aes_key = ''

    # Determine flash_crc_en: use parameter if provided, otherwise from security.csv, otherwise default to True
    if flash_crc_en is None:
        if s is not None:
            flash_crc_en = s.flash_crc_en
        else:
            flash_crc_en = True

    logging.info(f'steps_pack, bl1_secureboot_en={bl1_secureboot_en}, ota_type={ota_type}, ota_security_counter={ota_security_counter}, ota_encrypt={ota_encrypt}, flash_crc_en={flash_crc_en}')

    p = Partitions('partitions.csv', 'bin.csv', 'pack.json')
    if bl2_version:
        # Remove 'v' prefix if present (e.g., 'v1.0.3' -> '1.0.3')
        bl2_version_clean = bl2_version.lstrip('vV')
        p.set_version_override('bl2', bl2_version_clean)
        logging.info(f'Set BL2 version override: {bl2_version_clean} (from parameter, overriding bin.csv)')
    if app_version:
        # Remove 'v' prefix if present (e.g., 'v0.0.3' -> '0.0.3')
        app_version_clean = app_version.lstrip('vV')
        if ota_type == 'XIP':
            p.set_version_override('xip_a', app_version_clean)
        elif ota_type == 'OVERWRITE':
            p.set_version_override('overwrite', app_version_clean)
        logging.info(f'Set APP version override: {app_version_clean} (from parameter, overriding bin.csv)')

    p.pack_bin('pack.json', pubkey_pem, privkey_pem, flash_aes_type, flash_aes_key, ota_security_counter, ota_encrypt, boot_ota, False)
    p.install_bin()

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
    # Import PackJson here to avoid circular import (pack_json -> pack_bl2_sign -> steps)
    from .pack_json import PackJson
    pack = PackJson()
    pack.pack('steps_pack.json', s.img_sign_pubkey, None, s.flash_aes_key, s.flash_crc_en, None, s.flash_aes_type)
    gen_otp_efuse_config_file(s.flash_aes_type, s.flash_crc_en, s.flash_aes_key, s.img_sign_pubkey, s.bl1_secureboot_en, False, 'otp_efuse_config.json')
    pack.install_bin()
    insert_pk_hash('bootloader.bin', s.img_sign_pubkey)
