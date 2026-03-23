#!/usr/bin/env python3

import argparse
import logging
import subprocess
import os
import shutil
import sys

from build_server.build import *
from sign_server.sign import *
from pack_server.pack import *
from log_util import setup_logging, LogLevel, log_step_start, log_step_end, log_step_info, log_progress

def go_back_dir(levels):
    cwd = os.getcwd()
    for i in range(levels):
        cwd = os.path.dirname(cwd)
    return cwd

def copy_file(src, dst):
    """
    Copy a file from source to destination.
    
    Args:
        src: Source file path
        dst: Destination file path or directory
    
    Returns:
        True if copy succeeded, False otherwise
    """
    if not os.path.exists(src):
        logging.error(f'Source file does not exist: {src}')
        return False
    
    try:
        # If dst is a directory, use the source filename
        if os.path.isdir(dst):
            dst_file = os.path.join(dst, os.path.basename(src))
        else:
            dst_file = dst
        
        # Ensure destination directory exists
        dst_dir = os.path.dirname(dst_file)
        if dst_dir and not os.path.exists(dst_dir):
            os.makedirs(dst_dir, exist_ok=True)

        shutil.copy(src, dst_file)
        
        # Verify copy succeeded
        if os.path.exists(dst_file):
            logging.debug(f'Successfully copied: {src} -> {dst_file}')
            return True
        else:
            logging.error(f'Copy failed: destination file does not exist after copy: {dst_file}')
            return False
    except Exception as e:
        logging.error(f'Failed to copy {src} to {dst}: {e}')
        return False

def copy_file_or_exit(src, dst, error_msg=None):
    """
    Copy a file from source to destination, exit on failure.
    
    Args:
        src: Source file path
        dst: Destination file path or directory
        error_msg: Optional custom error message. If None, a default message is used.
    
    Exits:
        Exits with code 1 if copy fails
    """
    if not copy_file(src, dst):
        if error_msg:
            logging.error(error_msg)
        else:
            logging.error(f'Failed to copy {src} to {dst}')
        exit(1)

def copy_files(src, dst):
    files = os.listdir(src)
    for f in files:
        shutil.copy(f'{src}/{f}', dst)

def rm_file(f):
    if os.path.exists(f):
        os.remove(f)

def rm_dir(d):
    if os.path.exists(d):
        for f in os.listdir(d):
            f = os.path.join(d, f)
            if os.path.isfile(f):
                os.remove(f)
            elif os.path.isdir(f):
                os.rmtree(f)

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def set_log_level(log_level_str):
    """Set log level from string"""
    log_level_map = {
        'quiet': LogLevel.QUIET,
        'simple': LogLevel.SIMPLE,
        'normal': LogLevel.NORMAL,
        'verbose': LogLevel.VERBOSE,
        'debug': LogLevel.DEBUG,
    }
    level = log_level_map.get(log_level_str.lower(), LogLevel.SIMPLE)
    setup_logging(level)
    return level

def main():
    parse = argparse.ArgumentParser(description="Sign, encrypt and pack all-app.bin")
    parse.add_argument('--flash_aes_type', choices=['NONE', 'FIXED', 'RANDOM'], required=True, help='Specify flash AES type')
    parse.add_argument('--flash_aes_key', type=str, help='Specify flash AES key')
    parse.add_argument('--flash_crc_en', type=str, choices=['True', 'False', 'true', 'false'], required=False, default=None, help='Enable flash CRC (True/False). If not specified, read from security.csv or default to True')
    parse.add_argument('--bl2_version', type=str, required=True, help='Specify bl2.bin version')
    parse.add_argument('--app_version', type=str, required=True, help='Specify app.bin version')
    parse.add_argument('--app_security_counter', type=int, required=True, help='Specify app.bin security counter')
    parse.add_argument('--ota_version', type=str, required=True, help='Specify ota.bin version')
    parse.add_argument('--ota_security_counter', type=str, required=True, help='Specify ota.bin security counter')
    parse.add_argument('--ota_type', choices=['XIP', 'OVERWRITE'], required=True, help='Specify OTA strategy')
    parse.add_argument('--privkey', type=str, required=False, help='Specify BL2 private key PEM file')
    parse.add_argument('--pubkey', type=str, required=True, help='Specify BL2 public key PEM file')
    parse.add_argument('--replace_key_en', action='store_true', help='whether to replace key')
    parse.add_argument('--new_privkey', type=str, required=False, help='new Specify BL2 private key PEM file')
    parse.add_argument('--new_pubkey', type=str, required=False, help='new Specify BL2 public key PEM file')
    parse.add_argument('--app_slot_size', type=int, required=True, help='Specify virtual APP slot size')
    parse.add_argument('--ota_slot_size', type=int, help='Specify virtual OTA slot size (OVERWRITE only)')
    parse.add_argument('--project', type=str, help='Specify the project name')
    parse.add_argument('--bl1_secureboot_en', action='store_true', help='whether enable secureboot')
    parse.add_argument('--bl2_load_addr', type=str, help='Bootloader(BL2) load address')
    parse.add_argument('--boot_ota', action='store_true', help='Indicate whether support bootloader OTA')
    parse.add_argument('--build', action='store_true', help='Indicate whether rebuild the project')
    parse.add_argument('--clean', action='store_true', help='Clean all temp files')
    parse.add_argument('--debug', action='store_true', help='Enable debug (deprecated, use --log-level)')
    parse.add_argument('--log-level', type=str, choices=['quiet', 'simple', 'normal', 'verbose', 'debug'],
                       default='simple', help='Set log verbosity: quiet (errors only), simple (key steps, default), normal (info), verbose (all info), debug (everything)')

    args = parse.parse_args()
    cwd = os.getcwd()
    tool_path = go_back_dir(2)
    build_path = os.path.join(cwd, 'build_server')
    sign_path = os.path.join(cwd, 'sign_server')
    pack_path = os.path.join(cwd, 'pack_server')

    if args.flash_aes_type == 'FIXED':
        if args.flash_aes_key == None:
            logging.error(f'option "--flash_aes_key" is required when option "--flash_aes_type" is FIXED')
            exit(1)

    if args.bl1_secureboot_en:
        if args.bl2_load_addr == None:
            logging.error(f'option "--bl2_load_addr" is required when "--bl1_secureboot_en" is specified')
            exit(1)

    if args.ota_type == 'OVERWRITE':
        if args.ota_slot_size == None:
            logging.error(f'when ota_type is "OVERWRITE" then "--ota_slot_size" is required')
            exit(1)

    if args.build:
        if args.project == None:
            logging.error(f'option "--project" is required when "--build" is specified')
            exit(1)

    # Setup logging - handle both old --debug flag and new --log-level
    if args.debug:
        log_level = set_log_level('debug')
    else:
        log_level = set_log_level(args.log_level)
    
    # For backward compatibility, pass debug flag to subprocesses
    if log_level >= LogLevel.DEBUG:
        debug_opt = '--debug'
    else:
        debug_opt = ''
    
    log_step_start('Build Process', f'Project: {args.project if args.project else "N/A"}, Log Level: {args.log_level}')
    idk_path = go_back_dir(5)
    # Build Server Start
    if args.build:
        log_step_start('Build Server', f'Building project: {args.project}')
        os.chdir(idk_path)
        build_server_process(args.project)
        log_step_end('Build Server', 'OK')
        project_base = os.path.basename(args.project)
    # Build Server End

    # Copy files to sign_server
    if args.ota_type == 'XIP':
        copy_file_or_exit(f'{idk_path}/build/bk7236n/xip/package/bl2.bin', f'{sign_path}/',
                            'Failed to copy bl2.bin to sign_server')
        copy_file_or_exit(f'{idk_path}/build/bk7236n/xip/bk7236n/app.bin', f'{sign_path}/',
                        'Failed to copy app.bin to sign_server')
        copy_file_or_exit(f'{idk_path}/build/bk7236n/xip/package/{args.privkey}', f'{sign_path}/',
                        'Failed to copy {args.privkey} to sign_server')
    elif args.ota_type == 'OVERWRITE':
        copy_file_or_exit(f'{idk_path}/build/bk7236n/overwrite/package/bl2.bin', f'{sign_path}/',
                            'Failed to copy bl2.bin to sign_server')
        copy_file_or_exit(f'{idk_path}/build/bk7236n/overwrite/bk7236n/app.bin', f'{sign_path}/',
                        'Failed to copy app.bin to sign_server')
        copy_file_or_exit(f'{idk_path}/build/bk7236n/overwrite/package/{args.privkey}', f'{sign_path}/',
                        'Failed to copy {args.privkey} to sign_server')
    
    # Copy files to pack_server
    log_progress('Copying immutable files to pack server')
    if args.ota_type == 'XIP':
        optional_nvs_key_src = f'{idk_path}/build/bk7236n/xip/package/nvs_key.bin'
        required_files = [
            (f'{idk_path}/build/bk7236n/xip/package/bl2.bin', f'{pack_path}/immutable', 'bl2.bin'),
            (f'{idk_path}/build/bk7236n/xip/package/user_mfr.bin', f'{pack_path}/immutable', 'user_mfr.bin'),
            (f'{idk_path}/build/bk7236n/xip/package/bin.csv', f'{pack_path}/immutable', 'bin.csv'),
            (f'{idk_path}/build/bk7236n/xip/package/nvs.csv', f'{pack_path}/immutable', 'nvs.csv'),
            (f'{idk_path}/build/bk7236n/xip/package/pack.json', f'{pack_path}/immutable', 'pack.json'),
            (f'{idk_path}/build/bk7236n/xip/package/security.csv', f'{pack_path}/immutable', 'security.csv'),
            (f'{idk_path}/build/bk7236n/xip/package/ota.csv', f'{pack_path}/immutable', 'ota.csv'),
            (f'{idk_path}/build/bk7236n/xip/package/partitions.csv', f'{pack_path}/immutable', 'partitions.csv'),
            (f'{idk_path}/build/bk7236n/xip/package/partition.bin', f'{pack_path}/immutable', 'partition.bin'),
            (f'{idk_path}/build/bk7236n/xip/package/xip_a.bin', f'{pack_path}/immutable', 'xip_a.bin'),
            (f'{idk_path}/build/bk7236n/xip/package/provision.bin', f'{pack_path}/immutable', 'provision.bin'),
            (f'{idk_path}/build/bk7236n/xip/package/root_ec256_pubkey.pem', f'{pack_path}/immutable', 'root_ec256_pubkey.pem'),
            (f'{idk_path}/build/bk7236n/xip/package/{args.privkey}', f'{pack_path}/immutable', args.privkey),
        ]
    elif args.ota_type == 'OVERWRITE':
        optional_nvs_key_src = f'{idk_path}/build/bk7236n/overwrite/package/nvs_key.bin'
        required_files = [
            (f'{idk_path}/build/bk7236n/overwrite/package/bl2.bin', f'{pack_path}/immutable', 'bl2.bin'),
            (f'{idk_path}/build/bk7236n/overwrite/package/user_mfr.bin', f'{pack_path}/immutable', 'user_mfr.bin'),
            (f'{idk_path}/build/bk7236n/overwrite/package/bin.csv', f'{pack_path}/immutable', 'bin.csv'),
            (f'{idk_path}/build/bk7236n/overwrite/package/nvs.csv', f'{pack_path}/immutable', 'nvs.csv'),
            (f'{idk_path}/build/bk7236n/overwrite/package/pack.json', f'{pack_path}/immutable', 'pack.json'),
            (f'{idk_path}/build/bk7236n/overwrite/package/security.csv', f'{pack_path}/immutable', 'security.csv'),
            (f'{idk_path}/build/bk7236n/overwrite/package/ota.csv', f'{pack_path}/immutable', 'ota.csv'),
            (f'{idk_path}/build/bk7236n/overwrite/package/partitions.csv', f'{pack_path}/immutable', 'partitions.csv'),
            (f'{idk_path}/build/bk7236n/overwrite/package/partition.bin', f'{pack_path}/immutable', 'partition.bin'),
            (f'{idk_path}/build/bk7236n/overwrite/package/overwrite.bin', f'{pack_path}/immutable', 'overwrite.bin'),
            (f'{idk_path}/build/bk7236n/overwrite/package/provision.bin', f'{pack_path}/immutable', 'provision.bin'),
            (f'{idk_path}/build/bk7236n/overwrite/package/root_ec256_pubkey.pem', f'{pack_path}/immutable', 'root_ec256_pubkey.pem'),
            (f'{idk_path}/build/bk7236n/overwrite/package/{args.privkey}', f'{pack_path}/immutable', args.privkey),
        ]
    else:
        logging.error(f'Invalid OTA type: {args.ota_type}, only support XIP and OVERWRITE type')
        exit(1)

    os.makedirs(f'{pack_path}/immutable', exist_ok=True)
    for src, dst, name in required_files:
        copy_file_or_exit(src, dst, f'Failed to copy {name} to pack_server/immutable')

    # Copy optional nvs_key.bin if present in build output
    if os.path.exists(optional_nvs_key_src):
        copy_file_or_exit(optional_nvs_key_src, f'{pack_path}/immutable',
                          'Failed to copy optional nvs_key.bin to pack_server/immutable')
        log_progress('Copied optional nvs_key.bin to pack_server/immutable')
    else:
        log_progress(f'Skip optional nvs_key.bin (not found): {optional_nvs_key_src}')

    # Sign Server Start
    log_step_start('Sign Server', f'OTA Type: {args.ota_type}, App Version: {args.app_version}')
    os.chdir(sign_path)
    sign_server_process(tool_path, args.ota_type, args.app_version, args.app_security_counter, args.ota_version, args.ota_security_counter, args.privkey, args.pubkey, args.replace_key_en ,args.new_privkey, args.new_pubkey,args.app_slot_size, args.ota_slot_size, args.bl1_secureboot_en, args.bl2_load_addr, debug_opt)
    log_step_end('Sign Server', 'OK')
    # Sign Server End

    # Pack Server Start
    log_step_start('Pack Server', f'AES Type: {args.flash_aes_type}, CRC: {args.flash_crc_en}')
    
    if os.path.exists(f'{pack_path}/_tmp'):
        shutil.rmtree(f'{pack_path}/_tmp')
    os.makedirs(f'{pack_path}/_tmp', exist_ok=True)

    log_progress('Copying signed binaries to pack directory')
    copy_file_or_exit(f'{sign_path}/primary_all_signed.bin', f'{pack_path}/_tmp',
                    'Failed to copy primary_all_signed.bin to pack_server/_tmp')
    copy_file_or_exit(f'{sign_path}/ota_signed.bin', f'{pack_path}/_tmp',
                    'Failed to copy ota_signed.bin to pack_server/_tmp')
    shutil.move(f'{sign_path}/pack_cmd_list.txt', f'{pack_path}/_tmp')
    copy_files(f'{pack_path}/immutable', f'{pack_path}/_tmp')

    if args.bl1_secureboot_en:
        copy_file_or_exit(f'{sign_path}/primary_manifest.bin', f'{pack_path}/_tmp',
                          'Failed to copy primary_manifest.bin to pack_server/_tmp')
        if args.ota_type == 'XIP':
            copy_file_or_exit(f'{idk_path}/build/bk7236n/xip/package/primary_manifest.json', f'{pack_path}/immutable',
                              'Failed to copy primary_manifest.json to pack_server/immutable')
        elif args.ota_type == 'OVERWRITE':
            copy_file_or_exit(f'{idk_path}/build/bk7236n/overwrite/package/primary_manifest.json', f'{pack_path}/immutable',
                              'Failed to copy primary_manifest.json to pack_server/immutable')

    os.chdir(f'{pack_path}/_tmp')
    pack_server_process(tool_path, args.pubkey, args.privkey, args.bl1_secureboot_en, args.flash_aes_type, args.flash_aes_key, args.flash_crc_en, args.ota_type, args.ota_security_counter, args.boot_ota, args.bl2_version, args.app_version, debug_opt)
    log_step_end('Pack Server', 'OK')
    # Pack Server End
    
    # Install files
    log_step_start('Install', 'Copying final files to install directory')
    os.chdir(cwd)
    if os.path.exists(f'{cwd}/install'):
        shutil.rmtree(f'{cwd}/install')
    os.makedirs(f'{cwd}/install', exist_ok=True)
    copy_files(f'{pack_path}/_tmp/install', f'{cwd}/install')
    log_step_end('Install', 'OK')

    # Cleanup
    if args.clean:
        log_progress('Cleaning temporary files')
        if os.path.exists(f'{pack_path}/_tmp'):
            shutil.rmtree(f'{pack_path}/_tmp')
        if os.path.exists(f'{pack_path}/immutable'):
            shutil.rmtree(f'{pack_path}/immutable')
        rm_file(f'{sign_path}/*.pem')
        rm_file(f'{sign_path}/*.bin')
        rm_file(f'{sign_path}/*.csv')
        rm_file(f'{sign_path}/temp_after_compress')
        rm_file(f'{sign_path}/temp_before_compress')
    
    log_step_end('Build Process', 'OK', f'Output: {cwd}/install/all-app.bin')
    
    if log_level >= LogLevel.NORMAL:
        log_step_info(f"Flashing command: bk_loader.exe download --portinfo 18 --baudrate 1500000 --infile all-app.bin --aes-key {args.flash_aes_key}")

if __name__ == '__main__':
    # Default to simple logging if not specified
    setup_logging(LogLevel.SIMPLE)
    main()
