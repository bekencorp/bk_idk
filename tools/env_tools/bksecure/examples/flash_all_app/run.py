#!/usr/bin/env python3

import argparse
import logging
import subprocess
import os
import shutil
import sys
import glob

# Parent of the bksecure package (tools/env_tools) must be on sys.path for
# `import bksecure` in pack_server/pack.py when run from this example dir.
_env_tools = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
if _env_tools not in sys.path:
    sys.path.insert(0, _env_tools)

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
    for matched_path in glob.glob(f):
        if os.path.isfile(matched_path):
            os.remove(matched_path)

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
    parse.add_argument('--soc_type', choices=['bk7236', 'bk7236n'], default='bk7236n', help='Specify SoC model')
    parse.add_argument('--flash_aes_type', choices=['NONE', 'FIXED', 'RANDOM'], required=True, help='Specify flash AES type')
    parse.add_argument('--flash_aes_key', type=str, help='Specify flash AES key')
    parse.add_argument('--bl2_version', type=str, required=False, help='Specify bl2.bin version')
    parse.add_argument('--app_version', type=str, required=False, help='Specify app.bin version')
    parse.add_argument('--security_counter', type=int, required=True, help='Specify app.bin security counter')
    parse.add_argument('--ota_type', choices=['XIP', 'OVERWRITE'], required=True, help='Specify OTA strategy')
    parse.add_argument('--privkey', type=str, required=False, default='root_ec256_privkey.pem', help='Specify BL2 private key PEM file')
    parse.add_argument('--pubkey', type=str, required=False, default='root_ec256_pubkey.pem', help='Specify BL2 public key PEM file')
    parse.add_argument('--replace_key_en', type=str, required=False, default=False, help='whether to replace key')
    parse.add_argument('--new_privkey', type=str, required=False, help='new Specify BL2 private key PEM file')
    parse.add_argument('--new_pubkey', type=str, required=False, help='new Specify BL2 public key PEM file')
    parse.add_argument('--project', type=str, help='Specify the project name')
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
        build_server_process(args.project, args.soc_type)
        log_step_end('Build Server', 'OK')
        project_base = os.path.basename(args.project)
    # Build Server End

    # Copy files to sign_server
    if args.ota_type == 'XIP':
        sign_server_required_files = [
            (f'{idk_path}/build/{args.soc_type}/xip/package/partitions.csv', f'{sign_path}/', 'partitions.csv'),
            (f'{idk_path}/build/{args.soc_type}/xip/package/pack.json', f'{sign_path}/', 'pack.json'),
            (f'{idk_path}/build/{args.soc_type}/xip/package/bl2.bin', f'{sign_path}/', 'bl2.bin'),
            (f'{idk_path}/build/{args.soc_type}/xip/package/xip_a.bin', f'{sign_path}/', 'xip_a.bin'),
            (f'{idk_path}/build/{args.soc_type}/xip/package/{args.privkey}', f'{sign_path}/', args.privkey),
            (f'{idk_path}/build/{args.soc_type}/xip/package/{args.pubkey}', f'{sign_path}/', args.pubkey),
        ]
    elif args.ota_type == 'OVERWRITE':
        sign_server_required_files = [
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/partitions.csv', f'{sign_path}/', 'partitions.csv'),
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/pack.json', f'{sign_path}/', 'pack.json'),
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/bl2.bin', f'{sign_path}/', 'bl2.bin'),
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/overwrite.bin', f'{sign_path}/', 'overwrite.bin'),
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/{args.privkey}', f'{sign_path}/', args.privkey),
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/{args.pubkey}', f'{sign_path}/', args.pubkey),
        ]

    # Copy files to sign_server
    for src, dst, name in sign_server_required_files:
            copy_file_or_exit(src, dst, f'Failed to copy {name} to sign_server')
    
    # Sign Server Start
    log_step_start('Sign Server', f'OTA Type: {args.ota_type}, App Version: {args.app_version}')
    os.chdir(sign_path)
    sign_server_process(tool_path, args.soc_type, args.pubkey, args.privkey, args.ota_type, args.bl2_version, args.app_version, args.security_counter, args.replace_key_en, args.new_privkey, args.new_pubkey, debug_opt)
    log_step_end('Sign Server', 'OK')
    # Sign Server End

    # Copy files to pack_server
    log_progress('Copying immutable files to pack server')
    if args.ota_type == 'XIP':

        pack_server_optional_files = [
            (f'{sign_path}/pack_cmd_list.txt', f'{pack_path}/immutable', 'pack_cmd_list.txt'),
            ]

        pack_server_required_files = [
            (f'{sign_path}/pack.json', f'{pack_path}/immutable', 'pack.json'),
            (f'{sign_path}/partitions.csv', f'{pack_path}/immutable', 'partitions.csv'),
            (f'{sign_path}/bl2.bin', f'{pack_path}/immutable', 'bl2.bin'),
            (f'{sign_path}/app_signed.bin', f'{pack_path}/immutable', 'app_signed.bin'),
            (f'{idk_path}/build/{args.soc_type}/xip/package/nvs.csv', f'{pack_path}/immutable', 'nvs.csv'),
            (f'{idk_path}/build/{args.soc_type}/xip/package/user_mfr.bin', f'{pack_path}/immutable', 'user_mfr.bin'),
        ]

    elif args.ota_type == 'OVERWRITE':

        pack_server_optional_files = [
            (f'{sign_path}/pack_cmd_list.txt', f'{pack_path}/immutable', 'pack_cmd_list.txt'),
            ]

        pack_server_required_files = [
            (f'{sign_path}/pack.json', f'{pack_path}/immutable', 'pack.json'),
            (f'{sign_path}/partitions.csv', f'{pack_path}/immutable', 'partitions.csv'),
            (f'{sign_path}/bl2.bin', f'{pack_path}/immutable', 'bl2.bin'),
            (f'{sign_path}/app_signed.bin', f'{pack_path}/immutable', 'app_signed.bin'),
            (f'{sign_path}/{args.privkey}', f'{pack_path}/immutable', args.privkey),
            (f'{sign_path}/{args.pubkey}', f'{pack_path}/immutable', args.pubkey),
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/nvs.csv', f'{pack_path}/immutable', 'nvs.csv'),
            (f'{idk_path}/build/{args.soc_type}/overwrite/package/user_mfr.bin', f'{pack_path}/immutable', 'user_mfr.bin'),
        ]
    else:
        logging.error(f'Invalid OTA type: {args.ota_type}, only support XIP and OVERWRITE type')
        exit(1)

    if os.path.exists(f'{pack_path}/immutable'):
        shutil.rmtree(f'{pack_path}/immutable')
    os.makedirs(f'{pack_path}/immutable', exist_ok=True)
    for src, dst, name in pack_server_required_files:
        copy_file_or_exit(src, dst, f'Failed to copy {name} to pack_server/immutable')
    for src, dst, name in pack_server_optional_files:
        copy_file(src, dst)
    
    if os.path.exists(f'{pack_path}/_tmp'):
        shutil.rmtree(f'{pack_path}/_tmp')
    os.makedirs(f'{pack_path}/_tmp', exist_ok=True)
    copy_files(f'{pack_path}/immutable', f'{pack_path}/_tmp')
    os.chdir(f'{pack_path}/_tmp')

    pack_server_process(tool_path, args.soc_type, args.flash_aes_type, args.flash_aes_key, args.ota_type, args.security_counter, args.pubkey, args.privkey, debug_opt)
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
        rm_file(f'{pack_path}/immutable/*')
        rm_file(f'{sign_path}/*.pem')
        rm_file(f'{sign_path}/*.bin')
        rm_file(f'{sign_path}/*.csv')
        rm_file(f'{sign_path}/pack.json')
        rm_file(f'{sign_path}/pack_cmd_list.txt')
        rm_file(f'{sign_path}/temp_after_compress')
        rm_file(f'{sign_path}/temp_before_compress')
    
    log_step_end('Build Process', 'OK', f'Output: {cwd}/install/all-app.bin')
    
    if log_level >= LogLevel.NORMAL:
        log_step_info(f"Flashing command: bk_loader.exe download --portinfo 18 --baudrate 1500000 --infile all-app.bin --aes-key {args.flash_aes_key}")

if __name__ == '__main__':
    # Default to simple logging if not specified
    setup_logging(LogLevel.SIMPLE)
    main()
