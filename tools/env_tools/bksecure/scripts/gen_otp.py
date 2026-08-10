#!/usr/bin/env python3

import copy
import logging
from pathlib import Path

import yaml

from .common import *
from .rotpk_hash import *

_OTP_CONFIG_PATH = Path(__file__).with_name('gen_otp_config.yaml')
_OTP_CONFIG = None


def _load_otp_config():
    global _OTP_CONFIG
    if _OTP_CONFIG is None:
        with _OTP_CONFIG_PATH.open(encoding='utf-8') as f:
            _OTP_CONFIG = yaml.safe_load(f) or {}
    return _OTP_CONFIG


def get_supported_soc_types():
    return tuple(_load_otp_config().keys())


def normalize_soc_type(soc_type=None):
    if not soc_type:
        soc_type = get_soc_type()
    return soc_type.lower().replace('-', '')


def _get_soc_config(soc_type):
    soc_key = normalize_soc_type(soc_type)
    config = _load_otp_config()
    if soc_key not in config:
        supported = ', '.join(sorted(config.keys()))
        raise ValueError(f'Unsupported soc_type: {soc_type}, supported: {supported}')
    soc_config = config[soc_key]
    if 'security_ctrl' not in soc_config:
        raise ValueError(f'Missing security_ctrl in {_OTP_CONFIG_PATH} for soc_type: {soc_type}')
    if 'security_data' not in soc_config:
        raise ValueError(f'Missing security_data in {_OTP_CONFIG_PATH} for soc_type: {soc_type}')
    return soc_key, soc_config


def get_security_ctrl(soc_type=None):
    soc_key, soc_config = _get_soc_config(soc_type)
    security_ctrl = copy.deepcopy(soc_config['security_ctrl'])
    logging.debug(f'Security_Ctrl defaults for {soc_key}: {security_ctrl}')
    return security_ctrl


def update_security_ctrl(security_ctrl, soc_type, field_name, value):
    if field_name not in security_ctrl:
        logging.warning(
            f'Security_Ctrl field "{field_name}" not found for {normalize_soc_type(soc_type)}, skip update'
        )
        return
    security_ctrl[field_name] = value


def update_security_ctrl_last_value(security_ctrl, soc_type, field_name, last_value):
    """Update only the last segment of a comma-separated Security_Ctrl value."""
    if field_name not in security_ctrl:
        logging.warning(
            f'Security_Ctrl field "{field_name}" not found for {normalize_soc_type(soc_type)}, skip update'
        )
        return
    parts = security_ctrl[field_name].split(',')
    if not parts:
        logging.warning(
            f'Security_Ctrl field "{field_name}" has invalid value for {normalize_soc_type(soc_type)}, skip update'
        )
        return
    parts[-1] = str(last_value)
    security_ctrl[field_name] = ','.join(parts)


def get_otp_addr(soc_type, data_name):
    soc_key, soc_config = _get_soc_config(soc_type)
    security_data = soc_config['security_data']
    if data_name not in security_data:
        supported = ', '.join(sorted(security_data.keys()))
        raise ValueError(
            f'Unsupported Security_Data "{data_name}" for soc_type: {soc_type}, supported: {supported}'
        )
    addrs = security_data[data_name]
    logging.debug(f'{data_name} OTP addr for {soc_key}: {addrs}')
    return addrs


def get_flash_aes_key_otp_addr(soc_type=None):
    return get_otp_addr(soc_type, 'flash_aes_key')


def apply_otp_data_config(data, soc_type, data_name):
    otp_data = get_otp_addr(soc_type, data_name)
    data["start_addr"] = otp_data["start_addr"]
    data["last_valid_addr"] = otp_data["last_valid_addr"]
    data["byte_len"] = otp_data["byte_len"]

def reverse_order(hex_str):
    hex_str_len = len(hex_str)
    word_len = hex_str_len // 8
    reverse_str = ''
    for i in range(word_len):
        idx = i << 3
        reverse_str = reverse_str + hex_str[idx + 6]
        reverse_str = reverse_str + hex_str[idx + 7]
        reverse_str = reverse_str + hex_str[idx + 4]
        reverse_str = reverse_str + hex_str[idx + 5]
        reverse_str = reverse_str + hex_str[idx + 2]
        reverse_str = reverse_str + hex_str[idx + 3]
        reverse_str = reverse_str + hex_str[idx + 0]
        reverse_str = reverse_str + hex_str[idx + 1]


    logging.debug(f'hex_str={hex_str}')
    logging.debug(f'reverse_str={reverse_str}')
    return reverse_str


def gen_otp_efuse_config_file(aes_type, flash_crc_en, flash_aes_key, pubkey_pem_file, secureboot_en, boot_ota, outfile, soc_type=None):
    f = open(outfile, 'w+')
    logging.debug(f'Create {outfile}')

    security_ctrl = get_security_ctrl(soc_type)

    otp_efuse_config = {
        "User_Operate_Enable":  "false",
        "Security_Ctrl_Enable": "true",
        "Security_Data_Enable": "true",

        "User_Operate":[],

        "Security_Ctrl":[security_ctrl],

        "Security_Data":[]
    }

    if flash_crc_en == False or flash_crc_en == 'false':
        update_security_ctrl_last_value(security_ctrl, soc_type, "flash_no_crc_enable", 1)
    elif flash_crc_en == True or flash_crc_en == 'true':
        update_security_ctrl_last_value(security_ctrl, soc_type, "flash_no_crc_enable", 0)

    data = {}

    if aes_type == 'FIXED':
        update_security_ctrl_last_value(security_ctrl, soc_type, "flash_aes_enable", 1)
        data["name"] = "flash_aes_key"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to NA
        apply_otp_data_config(data, soc_type, "flash_aes_key")
        data["data"] = flash_aes_key
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)
    elif aes_type == 'RANDOM':
        update_security_ctrl_last_value(security_ctrl, soc_type, "flash_aes_enable", 1)
    else:
        pass

    if secureboot_en:
        h = Rotpk_hash(pubkey_pem_file)
        hash_dict = h.gen_rotpk_hash()
        bl1_pk_hash = hash_dict['bl1_rotpk_hash']
        bl2_pk_hash = hash_dict['bl2_rotpk_hash']

        if normalize_soc_type(soc_type) == 'bk7236':
            update_security_ctrl_last_value(security_ctrl, soc_type, "secureboot_enable", 1)
            update_security_ctrl_last_value(security_ctrl, soc_type, "boot_mode", 1)
        elif normalize_soc_type(soc_type) == 'bk7236n':
            update_security_ctrl_last_value(security_ctrl, soc_type, "secureboot_enable", 1)
        elif normalize_soc_type(soc_type) == 'bk7239n':
            update_security_ctrl_last_value(security_ctrl, soc_type, "download_disable", 1)
        
        if boot_ota == False:
            update_security_ctrl_last_value(security_ctrl, soc_type, "direct_jump_enable", 1)

        data = data.copy()
        data["name"] = "bl1_rotpk_hash"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to RO
        apply_otp_data_config(data, soc_type, "bl1_rotpk_hash")
        data["data"] = bl1_pk_hash
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)

        data = data.copy()
        data["name"] = "bl2_rotpk_hash"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to RO
        apply_otp_data_config(data, soc_type, "bl2_rotpk_hash")
        data["data"] = bl2_pk_hash
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)
    elif  pubkey_pem_file != None and os.path.exists(pubkey_pem_file):
        h = Rotpk_hash(pubkey_pem_file, False)
        hash_dict = h.gen_rotpk_hash()
        bl2_pk_hash = hash_dict['bl2_rotpk_hash']
        data = data.copy()
        data["name"] = "bl2_rotpk_hash"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to RO
        apply_otp_data_config(data, soc_type, "bl2_rotpk_hash")
        data["data"] = bl2_pk_hash
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)

    data = data.copy()
    data["name"] = "LCS"
    data["mode"] = "write"
    data["permission"] = "WR" 
    apply_otp_data_config(data, soc_type, "lcs")
    data["data"] = "00000003"
    data["data_type"] = "hex"
    data["status"] = "false"
    otp_efuse_config["Security_Data"].append(data)

    json_str = json.dumps(otp_efuse_config, indent=4)
    with open('otp_efuse_config.json', 'w',newline="\n") as file:
        file.write(json_str)
        if (secureboot_en):
            if (flash_aes_key):
                aes = f"\n# flash aes key in little endian:{reverse_order(flash_aes_key)}\n"
                file.write(aes)

            bl1 = f"# bl1_rotpk_hash in little endian:{reverse_order(bl1_pk_hash)}\n"
            file.write(bl1)
            bl2 = f"# bl2_rotpk_hash in little endian:{reverse_order(bl2_pk_hash)}\n"
            file.write(bl2)
    file.close()
