#!/usr/bin/env python3

import os
import subprocess
import sys
import logging
import getpass
import shutil
import platform

from argparse import _MutuallyExclusiveGroup
from genericpath import exists
import click
from click_option_group import optgroup,RequiredMutuallyExclusiveOptionGroup

def run_cmd(cmd):
	p = subprocess.Popen(cmd, shell=True)
	ret = p.wait()
	if (ret):
		logging.error(f'failed to run "{cmd}"')
		exit(1)

def run_cmd_with_ret(cmd):
	p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
	out, err = p.communicate()
	print(out)
	if (p.returncode):
		print(err)
		exit(1)

def copy_files(postfix, src_dir, dst_dir):
	logging.debug(f'copy *{postfix} from {src_dir}')
	logging.debug(f'to {dst_dir}')
	for f in os.listdir(src_dir):
		if f.endswith(postfix):
			shutil.copy(f'{src_dir}/{f}', f'{dst_dir}/{f}')

def install_configs(cfg_dir, install_dir):
	logging.debug(f'install configs from: {cfg_dir}')
	logging.debug(f'to: {install_dir}')
	if os.path.exists(f'{cfg_dir}') == False:
		return

	copy_files('.bin', cfg_dir, install_dir)
	copy_files('.json', cfg_dir, install_dir)
	copy_files('.pem', cfg_dir, install_dir)
	copy_files('.csv', cfg_dir, install_dir)

	if os.path.exists(f'{cfg_dir}/key') == True:
		copy_files('.pem', f'{cfg_dir}/key', install_dir)
	if os.path.exists(f'{cfg_dir}/csv') == True:
		copy_files('.csv', f'{cfg_dir}/csv', install_dir)
	if os.path.exists(f'{cfg_dir}/regs') == True:
		copy_files('.csv', f'{cfg_dir}/regs', install_dir)

def move_file(bin_name, src_dir, dst_dir):
	logging.debug(f'move {bin_name} from {src_dir}')
	logging.debug(f'to {dst_dir}')
	if os.path.exists(f'{src_dir}/{bin_name}') == True:
		if os.path.exists(dst_dir):
			shutil.copy2(f'{src_dir}/{bin_name}', f'{dst_dir}')
		else:
		    shutil.move(f'{src_dir}/{bin_name}', f'{dst_dir}')

def copy_file(src, dst):
	logging.debug(f'copy {src} to {dst}')
	logging.debug(f'to {dst}')
	if os.path.exists(f'{src}') == True:
		shutil.copy(f'{src}', f'{dst}')

def pack(armino_soc, armino_path, build_path, project_dir):
	armino_path = os.path.abspath(armino_path)
	build_path = os.path.abspath(build_path)
	project_dir = os.path.abspath(project_dir)

	tools_dir = f'{armino_path}/tools/env_tools'
	base_cfg_dir = f'{armino_path}/middleware/boards/{armino_soc}'
	prefered_cfg_dir = f'{project_dir}/config'
	debug = True

	if (debug):
		logging.basicConfig()
		logging.getLogger().setLevel(logging.DEBUG)

	logging.debug(f'tools_dir={tools_dir}')
	logging.debug(f'base_cfg_dir={base_cfg_dir}')
	logging.debug(f'prefered_cfg_dir={prefered_cfg_dir}')
	logging.debug(f'soc={armino_soc}')
	logging.debug(f'pack={pack}')

	SH_SEC_TOOL = f'{tools_dir}/sh_sec_tools/secure_boot_tool'

	BKSECURE = f'{tools_dir}/bksecure/main.py'
	MCUBOOT_IMG_TOOL = f'{tools_dir}/mcuboot_tools'

	BASE_CFG_DIR = base_cfg_dir
	_BUILD_DIR = f'{build_path}/package'
	CMAKE_INSTALL_DIR = f'{build_path}/package/install'
	CMAKE_BIN_DIR = os.path.join(build_path, armino_soc)

	logging.debug(f'Create temporary _build')
	os.makedirs(f'{_BUILD_DIR}', exist_ok=True)

	saved_cur_dir = os.getcwd()
	os.chdir(_BUILD_DIR)
	logging.debug(f'cd {_BUILD_DIR}')

	install_configs(BASE_CFG_DIR, _BUILD_DIR)
	install_configs(f'{prefered_cfg_dir}/{armino_soc}', _BUILD_DIR)

	debug_option = ''
	if (debug):
		debug_option = f'--debug'
	logging.debug(f'partition pre-processing')

	logging.debug(f'Copy Armino bin to {_BUILD_DIR}')

	if (os.path.exists(f'{CMAKE_BIN_DIR}/app.bin') == True):
		copy_file(f'{CMAKE_BIN_DIR}/app.bin', f'{_BUILD_DIR}/cpu0_app.bin')
	TFM_BIN_DIR = f'{CMAKE_BIN_DIR}/tfm_install/outputs'
	if (os.path.exists(TFM_BIN_DIR) == True):
		logging.debug(f'Copy TFM bin to {_BUILD_DIR}')
		copy_files('.bin', TFM_BIN_DIR, _BUILD_DIR)

	BL2_BIN_DIR = f'{CMAKE_BIN_DIR}/bl2_install/outputs'
	if (os.path.exists(BL2_BIN_DIR) == True):
		logging.debug(f'Copy BL2 bin to {_BUILD_DIR}')
		copy_files('.bin', BL2_BIN_DIR, _BUILD_DIR)
	
	run_cmd(f'{BKSECURE} pack all --debug')

	'''
	logging.debug(f'copy binaries')
	os.makedirs(f'{CMAKE_INSTALL_DIR}', exist_ok=True)
	copy_file(f'{_BUILD_DIR}/ota.bin', f'{CMAKE_INSTALL_DIR}/ota.bin')
	copy_file(f'{_BUILD_DIR}/all-app.bin', f'{CMAKE_INSTALL_DIR}/all-app.bin')
	copy_file(f'{_BUILD_DIR}/bootloader.bin', f'{CMAKE_INSTALL_DIR}/bootloader.bin')
	copy_file(f'{_BUILD_DIR}/provision_pack.bin', f'{CMAKE_INSTALL_DIR}/provision_pack.bin')
	copy_file(f'{CMAKE_BIN_DIR}/app.elf', f'{CMAKE_INSTALL_DIR}/app.elf')
	copy_file(f'{CMAKE_BIN_DIR}/tfm_install/outputs/tfm_s.elf', f'{CMAKE_INSTALL_DIR}/tfm_s.elf')
	copy_file(f'{CMAKE_BIN_DIR}/tfm_install/outputs/bl2.elf', f'{CMAKE_INSTALL_DIR}/bl2.elf')
	copy_file(f'{_BUILD_DIR}/otp_efuse_config.json', f'{CMAKE_INSTALL_DIR}/otp_efuse_config.json')
	copy_file(f'{CMAKE_BIN_DIR}/armino/partitions/_build/partitions_layout.txt', f'{CMAKE_INSTALL_DIR}/partitions_layout.txt')
	copy_file(f'{CMAKE_BIN_DIR}/sdkconfig', f'{CMAKE_INSTALL_DIR}/sdkconfig')

	pack_dir = f'{CMAKE_INSTALL_DIR}/pack'
	os.makedirs(f'{pack_dir}', exist_ok=True)
	copy_file(f'{_BUILD_DIR}/cpu0_app.bin', f'{pack_dir}/cpu0_app.bin')
	copy_file(f'{_BUILD_DIR}/bl2.bin', f'{pack_dir}/bl2.bin')
	copy_file(f'{_BUILD_DIR}/tfm_s.bin', f'{pack_dir}/tfm_s.bin')
	copy_file(f'{_BUILD_DIR}/partitions.csv', f'{pack_dir}/partitions.csv')
	copy_file(f'{_BUILD_DIR}/ota.csv', f'{pack_dir}/ota.csv')
	copy_file(f'{_BUILD_DIR}/security.csv', f'{pack_dir}/security.csv')
	copy_file(f'{_BUILD_DIR}/pack.json', f'{pack_dir}/pack.json')
	copy_file(f'{_BUILD_DIR}/primary_all.bin', f'{pack_dir}/primary_all.bin')
	copy_file(f'{_BUILD_DIR}/pack_cmd_list.txt', f'{pack_dir}/pack_cmd_list.txt')
	copy_file(f'{CMAKE_INSTALL_DIR}/../armino/partitions/_build/ppc_config.bin', f'{pack_dir}/ppc_config.bin')
	copy_files('.pem', _BUILD_DIR, pack_dir)

	move_file('all-app.bin', _BUILD_DIR, CMAKE_BIN_DIR)
	'''
	os.chdir(saved_cur_dir)
	logging.debug(f'cd {saved_cur_dir}')

if __name__ == '__main__':
	logging.basicConfig()
	logging.getLogger().setLevel(logging.INFO)
	armino_soc = sys.argv[1]
	armino_path = sys.argv[2]
	build_path = sys.argv[3]
	project_dir = sys.argv[4]
	print(f'=================================================================armino_soc={armino_soc}')
	print(f'armino_path={armino_path}')
	print(f'build_path={build_path}')
	print(f'project_dir={project_dir}')
	pack(armino_soc, armino_path, build_path, project_dir)
