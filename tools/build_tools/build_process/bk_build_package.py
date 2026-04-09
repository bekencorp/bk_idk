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
from env_tools.bksecure.scripts.gen_otp import gen_otp_efuse_config_file

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
        
        output_origin_bin = bin_file.parent / f"origin_{bin_file.stem}{bin_file.suffix}"
        shutil.copy2(bin_file, output_origin_bin)
        logger.debug(f"Backed up origin file to {output_origin_bin}")
        
        if "aes_enable" not in pack_data or not pack_data["aes_enable"]:
            logger.debug("aes_enable is false in pack_json, skip encryption")
            continue

        logger.info(f"Encrypting {firmware} with start_addr={start_addr}")
        cmd = f'python3 {aes_tool} encrypt -infile {bin_file} -keywords {aes_key} -aes 128 -outfile {bin_file} -startaddress {start_addr}'
        run_cmd_not_check_ret(cmd)

        if firmware == "bootloader.bin" and bin_file.exists():
            flash_crc_enable = pack_data.get("crc_enable", False)
            magic = "BEKEN".encode()
            magic_offset = 0x100
            if flash_crc_enable:
                pass
            with bin_file.open('r+b') as f:
                f.seek(magic_offset)
                f.write(magic)
            logger.debug(f'Added magic code {magic} to {bin_file} at offset {magic_offset}')

def _parse_size_to_bytes(size_str: str) -> int:
    """Parse size string like '68K', '2312K', '2M' to bytes."""
    size_str = size_str.strip().upper()
    if size_str.endswith("K"):
        return int(size_str[:-1]) * 1024
    if size_str.endswith("M"):
        return int(size_str[:-1]) * 1024 * 1024
    return int(size_str)


def pack_origin_bin(pack_dir: Path, pack_json: Path, output_origin_bin: Path):
    """
    Pack bootloader_oringin.bin and app_oringin.bin into all_app_oringin.bin.
    Order: bootloader first, app second. Pad with 0xFF up to each partition size from pack_json.
    """
    if not pack_json.exists():
        raise FileNotFoundError(f"{pack_json} not found")
    with pack_json.open("r") as f:
        pack_data = json.load(f)
    sections = pack_data.get("section", [])
    if not sections:
        logger.warning("pack_json has no section, skip pack_origin_bin")
        return
    # sort by start_addr so order is bootloader -> app
    sections = sorted(sections, key=lambda s: int(s.get("start_addr", "0x0"), 16))
    with output_origin_bin.open("wb") as out_f:
        for section in sections:
            partition = section.get("partition", "")
            size_str = section.get("size", "0")
            part_size = _parse_size_to_bytes(size_str)
            origin_name = f"origin_{partition}.bin"
            origin_path = pack_dir / origin_name
            if not origin_path.exists():
                logger.warning(f"{origin_path} not found, skip in all_app_oringin.bin")
                out_f.write(bytes([0xFF] * part_size))
                continue
            data = origin_path.read_bytes()
            if len(data) > part_size:
                logger.warning(f"{origin_name} size {len(data)} > partition size {part_size}, truncate")
                data = data[:part_size]
            out_f.write(data)
            if len(data) < part_size:
                out_f.write(bytes([0xFF] * (part_size - len(data))))
    logger.info(f"Packed origin bins to {output_origin_bin}")


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
    # pack origin_bootloader.bin + origin_app.bin -> origin_all_app.bin (0xFF pad by partition size)
    output_origin_bin = output_bin.parent / "origin_all_app.bin"
    pack_origin_bin(pack_dir, pack_json, output_origin_bin)


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

def _read_flash_crc_enable_from_csv(csv_path: Path, default_value: bool = True) -> bool:
    if not csv_path.exists():
        logger.warning(f"auto_partitions.csv not found at {csv_path}, using default flash_crc_enable={default_value}")
        return default_value

    try:
        csv_contents = csv_path.read_text()
        lines = csv_contents.splitlines()
        for line in lines:
            line_content = line.strip()
            if line_content.startswith("#") or len(line_content) == 0:
                continue
            if "FLASH_CRC_ENABLE" in line_content:
                crc_value = line_content.split("=")[1].strip().upper()
                if crc_value in ("TRUE", "1"):
                    logger.info(f"Read FLASH_CRC_ENABLE=TRUE from {csv_path}")
                    return True
                if crc_value in ("FALSE", "0"):
                    logger.info(f"Read FLASH_CRC_ENABLE=FALSE from {csv_path}")
                    return False
                logger.warning(f"Invalid FLASH_CRC_ENABLE value: {crc_value}, using default: {default_value}")
                return default_value
    except Exception as e:
        logger.warning(f"Failed to read FLASH_CRC_ENABLE from {csv_path}: {e}, using default: {default_value}")
    logger.info(f"FLASH_CRC_ENABLE not found in {csv_path}, using default: {default_value}")
    return default_value

def _read_flash_aes_enable_from_csv(csv_path: Path, default_value: bool = False) -> bool:
    if not csv_path.exists():
        logger.warning(f"auto_partitions.csv not found at {csv_path}, using default flash_aes_enable={default_value}")
        return default_value

    try:
        csv_contents = csv_path.read_text()
        lines = csv_contents.splitlines()
        for line in lines:
            line_content = line.strip()
            if line_content.startswith("#") or len(line_content) == 0:
                continue
            if "FLASH_AES_ENABLE" in line_content:
                aes_value = line_content.split("=")[1].strip().upper()
                if aes_value in ("TRUE", "1"):
                    logger.info(f"Read FLASH_AES_ENABLE=TRUE from {csv_path}")
                    return True
                if aes_value in ("FALSE", "0"):
                    logger.info(f"Read FLASH_AES_ENABLE=FALSE from {csv_path}")
                    return False
                logger.warning(f"Invalid FLASH_AES_ENABLE value: {aes_value}, using default: {default_value}")
                return default_value
    except Exception as e:
        logger.warning(f"Failed to read FLASH_AES_ENABLE from {csv_path}: {e}, using default: {default_value}")

    logger.info(f"FLASH_AES_ENABLE not found in {csv_path}, using default: {default_value}")
    return default_value


def _read_flash_aes_key_from_csv(csv_path: Path, default_str: str = "") -> str:
    if not csv_path.exists():
        logger.warning(f"auto_partitions.csv not found at {csv_path}, using default flash_aes_key={default_str}")
        return default_str

    try:
        csv_contents = csv_path.read_text()
        lines = csv_contents.splitlines()
        for line in lines:
            line_content = line.strip()
            if line_content.startswith("#") or len(line_content) == 0:
                continue
            if "FLASH_AES_KEY" in line_content:
                aes_value = line_content.split("=")[1].strip()
                logger.info(f"Read FLASH_AES_KEY={aes_value} from {csv_path}")
                return aes_value
    except Exception as e:
        logger.warning(f"Failed to read FLASH_AES_KEY from {csv_path}: {e}, using default: {default_str}")

    logger.info(f"FLASH_AES_KEY not found in {csv_path}, using default: {default_str}")
    return default_str


def gen_otp_efuse_from_auto_partition(output_dir: Path):
    """
    Generate otp_efuse_config.json based on auto_partitions.csv settings.
    Only FLASH_AES related configuration is generated.
    """
    # Re-read AES settings from auto_partitions.csv to ensure this process
    # sees the same configuration as auto_partition step.
    csv_path = curr_project.auto_partitions_table
    flash_crc_enable = _read_flash_crc_enable_from_csv(csv_path)
    flash_aes_enable = _read_flash_aes_enable_from_csv(csv_path)
    flash_aes_key = _read_flash_aes_key_from_csv(csv_path)

    # Decide AES type according to enable flag and key
    if flash_aes_enable and flash_aes_key:
        aes_type = "FIXED"
    else:
        # AES disabled or key empty, keep FLASH AES disabled in otp_efuse_config.json
        aes_type = "NONE"

    prev_cwd = Path.cwd()
    try:
        output_dir.mkdir(parents=True, exist_ok=True)
        os.chdir(output_dir)
        # pubkey_pem_file=None, secureboot_en=False, boot_ota=False
        gen_otp_efuse_config_file(aes_type, flash_crc_enable, flash_aes_key, None, False, False, "otp_efuse_config.json")
        logger.info(f"Generated otp_efuse_config.json at {output_dir}")
    finally:
        os.chdir(prev_cwd)


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
    # Generate otp_efuse_config.json based on auto_partitions.csv (FLASH_AES only)
    gen_otp_efuse_from_auto_partition(build_pack_dir)
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
