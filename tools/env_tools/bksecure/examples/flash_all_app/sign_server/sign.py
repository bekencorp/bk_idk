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

def sign_server_process(tool_path, ota_type, app_version, app_security_counter, ota_version, ota_security_counter, privkey, pubkey, replace_key_en, new_privkey, new_pubkey, app_slot_size, ota_slot_size, bl1_secureboot_en, bl2_load_addr, debug_opt=''):
    debug_opt=''
    
    # Generate primary_all.bin from app.bin in OTA before signing
    if os.path.exists('app.bin'):
        logging.info('Generate primary_all.bin from app.bin in OTA before signing')
        add_tool_path_to_syspath(tool_path)
        try:
            # Import Partitions from scripts package (relative imports will work now)
            from scripts.partitions import Partitions
                
            # Check if partitions.csv and ota.csv exist, if not, try to copy from pack_server/immutable
            pack_server_immutable = '../pack_server/immutable'
            if not os.path.exists('partitions.csv') and os.path.exists(f'{pack_server_immutable}/partitions.csv'):
                shutil.copy(f'{pack_server_immutable}/partitions.csv', 'partitions.csv')
                logging.info('Copied partitions.csv from pack_server/immutable')
            if not os.path.exists('ota.csv') and os.path.exists(f'{pack_server_immutable}/ota.csv'):
                shutil.copy(f'{pack_server_immutable}/ota.csv', 'ota.csv')
                logging.info('Copied ota.csv from pack_server/immutable')
            
            # Create Partitions object and generate primary_all.bin
            if os.path.exists('partitions.csv'):
                # bin_csv and pack_json can be None for this operation
                p = Partitions('partitions.csv', None, None)
                # Set ota_type explicitly since we know it from the parameter
                p._ota_type = ota_type
                p.gen_primary_all_bin_for_bl2_signing('primary_all', app_version)
                logging.info('Generated primary_all.bin from app.bin')
            else:
                logging.warning('partitions.csv not found, skip generating primary_all.bin from app.bin')
        except ImportError as e:
            logging.warning(f'Failed to import Partitions: {e}')
            # Continue with signing even if import fails
        except Exception as e:
            logging.warning(f'Failed to generate primary_all.bin from app.bin: {e}')
            import traceback
            logging.debug(traceback.format_exc())
            # Continue with signing even if generation fails
    
    if bl1_secureboot_en:
        cmd = f'{tool_path}/main.py sign bl1_sign  --key_type ec256 --privkey_pem_file {privkey} --pubkey_pem_file {pubkey} --bin_file bl2.bin --load_addr {bl2_load_addr} --outfile primary_manifest.bin'
        run_cmd(cmd)
        save_cmd("Sign and create primary_manifest.bin", cmd)

    if replace_key_en:
        cmd = (f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign '
               f'-k {new_privkey} '
               f'--public-key-format hash '
               f'--max-align 8 --align 1 '
               f'--version {app_version} '
               f'--security-counter {app_security_counter} '
               f'--custom-tlv 0xa0 0x{new_pubkey} '
               f'--pad-header --header-size 0x1000 '
               f'--slot-size {app_slot_size} '
               f'--pad --boot-record SPE '
               f'--endian little --encrypt-keylen 128 '
               f'primary_all.bin primary_all_signed.bin {debug_opt}')
        run_cmd(cmd)
        save_cmd("Sign with new key and create primary_all_signed.bin from primary_all.bin", cmd)
    else:
        cmd = (f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign '
               f'-k {privkey} '
               f'--public-key-format hash '
               f'--max-align 8 --align 1 '
               f'--version {app_version} '
               f'--security-counter {app_security_counter} '
               f'--custom-tlv 0xa0 0x{pubkey} '
               f'--pad-header --header-size 0x1000 '
               f'--slot-size {app_slot_size} '
               f'--pad --boot-record SPE '
               f'--endian little --encrypt-keylen 128 '
               f'primary_all.bin primary_all_signed.bin {debug_opt}')
        run_cmd(cmd)
        save_cmd("Sign and create primary_all_signed.bin from primary_all.bin", cmd)

    if ota_type == "XIP":
        cmd = (f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign '
               f'-k {privkey} '
               f'--public-key-format hash '
               f'--max-align 8 --align 1 '
               f'--version {ota_version} '
               f'--security-counter {ota_security_counter} '
               f'--custom-tlv 0xa0 0x{pubkey} '
               f'--pad-header --header-size 0x1000 '
               f'--slot-size {app_slot_size} '
               f'--pad --boot-record SPE '
               f'--endian little --encrypt-keylen 128 '
               f'primary_all.bin ota_signed.bin {debug_opt}')
        run_cmd(cmd)
        save_cmd("Sign and create XIP ota_signed.bin from primary_all.bin", cmd)
    else:
        cmd = f'{tool_path}/main.py pack compress --infile primary_all_signed.bin --outfile compress.bin {debug_opt}'
        run_cmd(cmd)
        save_cmd("Overwrite mode, Compress primary_all_signed.bin to compress.bin", cmd)

        cmd = (f'{tool_path}/scripts/../tools/mcuboot_tools/imgtool.py sign '
               f'-k {privkey} '
               f'--public-key-format hash '
               f'--max-align 8 --align 1 '
               f'--version {ota_version} '
               f'--security-counter {ota_security_counter} '
               f'--custom-tlv 0xa0 0x{pubkey} '
               f'--pad-header --header-size 0x1000 '
               f'--slot-size {ota_slot_size} '
               f'--pad --boot-record SPE '
               f'--endian little --encrypt-keylen 128 '
               f'compress.bin ota_signed.bin {debug_opt}')
        run_cmd(cmd)
        save_cmd("Overwrite mode, Sign and create ota_signed.bin from compress.bin", cmd)
