#!/usr/bin/env python3

import logging
from .security import *
from .gen_license import get_license
from .common import *

def gen_security_config_file(security_csv, outfile):
    security = Security(security_csv)
    f = open(outfile, 'w+')
    logging.debug(f'Create {outfile}')
    f.write(get_license())

    line = f'#include "_ppc.h"\r\n'
    f.write(line)
    line = f'#include "_mpc.h"\r\n'
    f.write(line)

    line = f'#undef %-45s\r\n' %("MCUBOOT_SIGN_RSA")
    f.write(line)
    if (security.bl1_secureboot_en):
        line = f'#define %-45s %d\r\n' %("CONFIG_BL1_SECUREBOOT", 1)
        f.write(line)

    if (security.img_sign_key_type == 'rsa2048'):
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA", "1\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA_LEN", "2048\r\n")
        f.write(line)
    elif (security.img_sign_key_type == 'rsa3072'):
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA", "1\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA_LEN", "3072\r\n")
        f.write(line)
    elif (security.img_sign_key_type == 'ec256'):
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_EC256", "1\r\n")
        f.write(line)
    elif (security.img_sign_key_type == 'NONE' or security.img_sign_key_type == None):
        logging.debug(f"mcuboot not have root key")
    else:
        logging.error(f'unsupported bl2 key type:{security.img_sign_key_type},{type(security.img_sign_key_type)}')
        exit(1)

    macro_name = f'CONFIG_CODE_ENCRYPTED'
    if security.is_flash_aes_fixed():
        line = f'#define %-45s %d\r\n' %(macro_name, 1)
    elif security.is_flash_aes_random():
        line = f'#define %-45s %d\r\n' %(macro_name, 2)
    else:
        line = f'#define %-45s %d\r\n' %(macro_name, 0)

    f.write(line)
    # Generate address conversion macros based on flash_crc_en
    # When CONFIG_FLASH_CRC_ENABLE == 1: use CRC mode conversion (32:34 mapping)
    # When CONFIG_FLASH_CRC_ENABLE == 0: use 1:1 mapping (no conversion, no alignment)
    if security.flash_crc_en:
        line = f'#define %-45s %s' %("FLASH_VIRTUAL2PHY(virtual_addr)", "((((virtual_addr) >> 5) * 34) + ((virtual_addr) & 31))\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("FLASH_PHY2VIRTUAL(phy_addr)", "((((phy_addr) / 34) << 5) + ((phy_addr) % 34))\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("CEIL_ALIGN_34(addr)", "(((addr) + 34 - 1) / 34 * 34)\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("FLOOR_ALIGN_34(addr)", "((addr) / 34 * 34)\r\n")
        f.write(line)
        # Convert virtual length to physical length (32-byte aligned virtual to 34-byte physical)
        line = f'#define %-45s %s' %("VIR_LEN_TO_PHY_LEN(vir_len)", "((((vir_len) + 31) >> 5) * 34)\r\n")
        f.write(line)
    else:
        # In no_crc mode: physical address = virtual address (1:1 mapping), no alignment
        line = f'#define %-45s %s' %("FLASH_VIRTUAL2PHY(virtual_addr)", "(virtual_addr)\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("FLASH_PHY2VIRTUAL(phy_addr)", "(phy_addr)\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("CEIL_ALIGN_34(addr)", "(addr)\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("FLOOR_ALIGN_34(addr)", "(addr)\r\n")
        f.write(line)
        # In no_crc mode: virtual length = physical length (1:1 mapping)
        line = f'#define %-45s %s' %("VIR_LEN_TO_PHY_LEN(vir_len)", "(vir_len)\r\n")
        f.write(line)
    line = f'#define %-45s %s' %("FLOOR_ALIGN_4K(addr)", "((addr) / 4096 * 4096)\r\n")
    f.write(line)

    macro_name = f'CONFIG_ANTI_ROLLBACK'
    if security.get_anti_rollback():
        line = f'#define %-45s %d\r\n' %(macro_name, 1)
        f.write(line)
    else:
        line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)

    empty_line(f)
    # Generate FLASH_CEIL_ALIGN and FLASH_PHY2VIRTUAL_CODE_START based on flash_crc_en
    line = f'#define %-45s %s' %("FLASH_CEIL_ALIGN(v, align)", "((((v) + ((align) - 1)) / (align)) * (align))\r\n")
    f.write(line)
    if security.flash_crc_en:
        # In CRC mode: use full conversion with alignment
        line = f'#define %-45s %s' %("FLASH_PHY2VIRTUAL_CODE_START(phy_addr)", "FLASH_CEIL_ALIGN(FLASH_PHY2VIRTUAL(FLASH_CEIL_ALIGN((phy_addr), 34)), CPU_VECTOR_ALIGN_SZ)\r\n")
    else:
        # In no_crc mode: no conversion, just return the address
        line = f'#define %-45s %s' %("FLASH_PHY2VIRTUAL_CODE_START(phy_addr)", "(phy_addr)\r\n")
    f.write(line)
    empty_line(f)
