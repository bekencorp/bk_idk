from __future__ import annotations

import json
import logging
import os
import shutil
import sys
from pathlib import Path

# add tools directory to Python search path
_script_dir = Path(__file__).parent
_tools_dir = _script_dir.parent.parent  # from build_process -> build_tools -> tools
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from bk_build_summary import bk_build_summary
from bk_sdk.bk_curr_project import curr_project
from env_tools.bksecure.scripts.common import get_flash_aes_tool, run_cmd_not_check_ret

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def backup_bootloader_path():
    # copy bootloader elf-map-asm to build directory
    bootloader_build_dir = curr_project.bootloader_build_path.parent
    if not bootloader_build_dir.exists():
        logger.debug("bootloader not build, not backup.")
        return

    bootloader_backup_path = curr_project.bootloader_backup_dir
    bootloader_backup_path.parent.mkdir(parents=True, exist_ok=True)
    if bootloader_backup_path.exists():
        shutil.rmtree(bootloader_backup_path)

    shutil.copytree(bootloader_build_dir, bootloader_backup_path)
    logger.info(f"backup bootloader to {bootloader_backup_path}")


def add_aes_to_all_bin(pack_dir: Path, pack_json: Path):
    if not pack_json.exists():
        raise FileNotFoundError(f"{pack_json} not found")
    with pack_json.open("r") as f:
        pack_data = json.load(f)
    
    if "aes_enable" not in pack_data or not pack_data["aes_enable"]:
        return
    
    aes_key = pack_data.get("aes_key")
    if not aes_key:
        logger.warning("aes_key not found or empty in pack_json when aes_enable is true, skip AES encryption")
        return
    
    aes_tool = get_flash_aes_tool()
    
    if "section" not in pack_data:
        logger.warning("section not found in pack_json, skip AES encryption")
        return
    
    for section in pack_data["section"]:
        firmware = section.get("firmware")
        start_addr = section.get("start_addr")
        
        if not firmware or not firmware.endswith(".bin"):
            continue
        
        if not start_addr:
            logger.warning(f"start_addr not found for {firmware}, skip encryption")
            continue
        
        bin_file = pack_dir / firmware
        if not bin_file.exists():
            logger.warning(f"{bin_file} not found, skip encryption")
            continue
        
        output_original_bin = bin_file.parent / f"{bin_file.stem}_no_aes{bin_file.suffix}"
        shutil.copy2(bin_file, output_original_bin)
        logger.debug(f"Backed up original file to {output_original_bin}")
        
        logger.info(f"Encrypting {firmware} with start_addr={start_addr}")
        cmd = f'python3 {aes_tool} encrypt -infile {bin_file} -keywords {aes_key} -aes 128 -outfile {bin_file} -startaddress {start_addr}'
        run_cmd_not_check_ret(cmd)

        if firmware == "bootloader.bin" and bin_file.exists():
            flash_crc_enable = pack_data.get("crc_enable", False)
            magic = "BEKEN".encode()
            magic_offset = 0x100
            if flash_crc_enable:
                magic_offset = 0x110
            with bin_file.open('r+b') as f:
                f.seek(magic_offset)
                f.write(magic)
            logger.debug(f'Added magic code {magic} to {bin_file} at offset {magic_offset}')

def pack_all_bin(pack_dir: Path, pack_json: Path, output_bin: Path):
    def binary_align_32_byte(bin_path: Path):
        if not bin_path.exists():
            raise RuntimeError(f"{bin_path} no exist.")
        bin_size = output_bin.stat().st_size
        padding_size = (32 - bin_size % 32) % 32
        with bin_path.open("ab") as f:
            f.write(bytes([0xFF] * padding_size))

    if not pack_json.exists():
        raise FileNotFoundError(f"{pack_json} not found")
    pack_dir.mkdir(parents=True, exist_ok=True)
    packager = curr_project.get_packager(pack_dir, pack_json, output_bin)
    packager.pack()
    # cmake_Gen_img 32byte align, so do same here.
    binary_align_32_byte(output_bin)


def gen_build_summary(output_info: str):
    build_pack_dir = curr_project.project_build_package_dir
    build_dir = curr_project.project_build_dir
    sumary_file = build_pack_dir / "build_summary.txt"
    summary = bk_build_summary()
    partitions_info = build_dir / "partitions" / "partitions.txt"
    summary.set_partitions_info(partitions_info)

    for app in curr_project.apps_info:
        app_build_dir = build_dir / app.app_name
        summary.set_app_folder(app.app_name_in_sdk, app_build_dir)

    summary.set_output_file_info(output_info)
    summary.gen_summary(sumary_file)


def firmware_package():
    build_pack_dir = curr_project.project_build_package_dir
    all_app_bin = build_pack_dir / "all-app.bin"
    build_partitions_dir = curr_project.project_build_parititons_dir
    pack_dir_temp = build_pack_dir / "tmp"
    pack_json = build_partitions_dir / "bk_package.json"
    if not pack_dir_temp.exists():
        pack_dir_temp.mkdir()
    curr_project.copy_binaries_to_pack_dir(pack_dir_temp)
    os.chdir(build_pack_dir)
    add_aes_to_all_bin(pack_dir_temp, pack_json)
    pack_all_bin(pack_dir_temp, pack_json, all_app_bin)
    return all_app_bin


def main():
    logger.info("Enter Armino Package")
    
    # Read flash_crc_enable from partitions.json (generated by auto_partition)
    partitions_json = curr_project.project_build_parititons_dir / "partitions.json"
    if partitions_json.exists():
        try:
            with partitions_json.open("r") as f:
                partitions_data = json.load(f)
                if "crc_enable" in partitions_data:
                    flash_crc_enable = bool(partitions_data["crc_enable"])
                    curr_project.set_flash_crc_enable(flash_crc_enable)
                    logger.info(f"Read flash_crc_enable={flash_crc_enable} from {partitions_json}")
        except Exception as e:
            logger.warning(f"Failed to read flash_crc_enable from {partitions_json}: {e}")
    
    backup_bootloader_path()
    curr_project.pre_package()
    all_app_bin = firmware_package()
    curr_project.build_summary = f"firmware: {all_app_bin}\n"
    curr_project.post_package()
    try:
        gen_build_summary(curr_project.build_summary)
    except Exception:
        logger.warning("skip generating build summary info")


if __name__ == "__main__":
    set_logging()
    main()
