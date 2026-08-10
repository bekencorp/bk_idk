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
from .partitions_legacy import Partitions_Legacy
from .common import copy_files as copy_all_files, get_config_paths
from .gen_otp import gen_otp_efuse_config_file
from .insert_mfr import insert_mfr

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

def is_pack_json_legacy(pack_json='pack.json'):
    """Return True if pack.json uses legacy all-app.bin format (partition name list)."""
    if not os.path.exists(pack_json):
        return False
    try:
        with open(pack_json, 'r') as f:
            data = json.load(f)
    except (json.JSONDecodeError, OSError) as e:
        logging.debug(f'is_pack_json_legacy: failed to read {pack_json}: {e}')
        return False

    all_app = data.get('all-app.bin')
    if all_app is None:
        return False

    # Legacy: "all-app.bin": ["bl2", "partition", ...]
    if isinstance(all_app, list):
        return True

    # Non-legacy: "all-app.bin": {"bin": [...], "action": "PACK_BL1_DOWNLOAD_BIN"}
    if isinstance(all_app, dict) and 'bin' in all_app and 'action' in all_app:
        return False

    return False

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
    o = OTA('ota.csv')
    compress_bin('app_signed.bin', 'compress.bin', o.get_flash_crc_en())
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

#Step7 - generate signed bin
def steps_sign(soc_type, pubkey_pem, privkey_pem, ota_type=None, bl2_version=None, app_version=None, security_counter=None, replace_key_en=None, new_privkey_pem=None, new_pubkey_pem=None):
    if pubkey_pem is None or privkey_pem is None:
        logging.error('pubkey_pem or privkey_pem is not specified')
        exit(1)
    os.environ['ARMINO_SOC'] = soc_type.upper()
    # If pubkey is a hex string (not a file path), convert to .pem file in current directory
    if isinstance(pubkey_pem, str) and not os.path.isfile(pubkey_pem):
        s = pubkey_pem.strip()
        if len(s) >= 64 and len(s) % 2 == 0 and all(c in '0123456789abcdefABCDEF' for c in s):
            der = bytes.fromhex(s)
            b64 = base64.b64encode(der).decode('ascii')
            pem_lines = '\n'.join(b64[i:i + 64] for i in range(0, len(b64), 64))
            pem_content = '-----BEGIN PUBLIC KEY-----\n' + pem_lines + '\n-----END PUBLIC KEY-----\n'
            pubkey_pem_path = os.path.join(os.getcwd(), 'generated_pubkey.pem')
            with open(pubkey_pem_path, 'w') as f:
                f.write(pem_content)
            pubkey_pem = 'generated_pubkey.pem'

    p = Partitions('partitions.csv', None,'pack.json')
    # Set version override for BL2 if provided
    if bl2_version:
        # Remove 'v' prefix if present (e.g., 'v1.0.3' -> '1.0.3')
        bl2_version_clean = bl2_version.lstrip('vV')
        p.set_version_override('bl2', bl2_version_clean)
        logging.info(f'Set BL2 version override: {bl2_version_clean} (from parameter, overriding bin.csv)')
    # Set version override for APP if provided
    if app_version:
        # Remove 'v' prefix if present (e.g., 'v0.0.3' -> '0.0.3')
        app_version_clean = app_version.lstrip('vV')
        if ota_type == 'XIP':
            p.set_version_override('xip_a', app_version_clean)
        elif ota_type == 'OVERWRITE':
            p.set_version_override('overwrite', app_version_clean)
        logging.info(f'Set APP version override: {app_version_clean} (from parameter, overriding bin.csv)')

    p.sign_bin(
        pubkey_pem, privkey_pem,
        ota_type=ota_type, security_counter=security_counter,
        replace_key_en=replace_key_en, new_privkey_pem=new_privkey_pem, new_pubkey_pem=new_pubkey_pem,
    )

def read_bl2_version():
    bl2_version = None
    if os.path.exists('bl2.bin'):
        with open('bl2.bin', 'rb') as f:
            f.seek(0x120)
            ver_bytes = f.read(4)
        if len(ver_bytes) == 4 and any(ver_bytes):
            numbers = list(ver_bytes)
            while len(numbers) > 1 and numbers[-1] == 0:
                numbers.pop()
            bl2_version = '.'.join(str(n) for n in numbers)
            logging.debug(f'read bl2 version from bl2.bin @0x120: {bl2_version}')
    return bl2_version

#Step8 - pack download bin
def steps_pack(soc_type, aes_key_type=None, aes_key=None, ota_type='OVERWRITE', security_counter=None):
    logging.debug(f'steps_pack, soc_type={soc_type}, aes_key_type={aes_key_type}, aes_key={aes_key}, ota_type={ota_type}, security_counter={security_counter}')
    if soc_type == 'bk7236' or soc_type == 'bk7236n':
        os.environ['ARMINO_SOC'] = soc_type.upper()
        if soc_type == 'bk7236':
            flash_crc_en = True
        else:
            flash_crc_en = False

        if aes_key_type != None:
            flash_aes_type = aes_key_type
            flash_aes_key = aes_key
        else:
            flash_aes_type = 'NONE'
            flash_aes_key = ''
        
        is_legacy = is_pack_json_legacy('pack.json')
        if is_legacy:
            bl2_version = read_bl2_version()
            if os.path.exists('app_signed.bin'):
                shutil.copy('app_signed.bin', 'primary_all_signed.bin')
            p = Partitions_Legacy('partitions.csv', ota_type, False, False, bl2_version)
            p.pack_bin('pack.json', flash_aes_type, flash_aes_key, security_counter, 'FALSE', False, False)
        else:
            p = Partitions('partitions.csv', None, 'pack.json')
            p.postbuild_process(flash_aes_en=False, flash_aes_key=flash_aes_key, img_sign_key=None, flash_crc_en=flash_crc_en, flash_aes_type=flash_aes_type)
            pack = PackJson()
            pack.exclude_ota_packers()
            pack.pack(flash_aes_key, flash_crc_en, None, flash_aes_type)
        p.install_bin()
        
    elif soc_type == 'bk3437':
        insert_mfr('primary_all_signed.bin', 'partitions.csv', 'mfr.bin')

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
    gen_otp_efuse_config_file(s.flash_aes_type, s.flash_crc_en, s.flash_aes_key, s.img_sign_pubkey, s.bl1_secureboot_en, False, 'otp_efuse_config.json', soc_type=get_soc_type())
    pack.install_bin()
    insert_pk_hash('bootloader.bin', s.img_sign_pubkey)

def steps_gen_ota_bin(soc_type, flash_aes_type, flash_aes_key, pubkey_pem, privkey_pem):
    logging.debug(f'steps_gen_ota_bin, soc_type={soc_type}, flash_aes_type={flash_aes_type}, flash_aes_key={flash_aes_key}, pubkey_pem={pubkey_pem}, privkey_pem={privkey_pem}')
    flash_crc_en = False
    if soc_type == 'bk7236':
        flash_crc_en = True
    pack = PackJson('pack.json')
    pack.only_ota_packers()
    logging.info(f"generating ota bin ...")
    pack.pack_ota_bin_only(flash_aes_key, flash_crc_en, None, flash_aes_type, pubkey_pem, privkey_pem)
    pack.install_bin()
    logging.info(f"generate ota bin done")
