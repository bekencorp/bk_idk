from __future__ import annotations

import json
import logging
from pathlib import Path
from typing import Any, Optional

from bk_auto_partition import bk_partitions_table, partition_limit
from bk_flash_partiton import bk_flash_partition
from bk_sdk.bk_curr_project import curr_project
from bk_sdk.bk_flash_partitions_generator import bk_flash_denpendecny_generator

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)

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
                if crc_value == "TRUE" or crc_value == "1":
                    logger.info(f"Read FLASH_CRC_ENABLE=TRUE from {csv_path}")
                    return True
                elif crc_value == "FALSE" or crc_value == "0":
                    logger.info(f"Read FLASH_CRC_ENABLE=FALSE from {csv_path}")
                    return False
                else:
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
                if aes_value == "TRUE" or aes_value == "1":
                    logger.info(f"Read FLASH_AES_ENABLE=TRUE from {csv_path}")
                    return True
                elif aes_value == "FALSE" or aes_value == "0":
                    logger.info(f"Read FLASH_AES_ENABLE=FALSE from {csv_path}")
                    return False
                else:
                    logger.warning(f"Invalid FLASH_AES_ENABLE value: {aes_value}, using default: {default_value}")
                    return default_value
    except Exception as e:
        logger.warning(f"Failed to read FLASH_CRC_ENABLE from {csv_path}: {e}, using default: {default_value}")
    
    logger.info(f"FLASH_CRC_ENABLE not found in {csv_path}, using default: {default_value}")
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


def _read_partition_size_kb_from_auto_partitions_csv(csv_path: Path, partition_name: str) -> Optional[int]:
    """read the size of a partition from auto_partitions.csv in KB"""
    if not csv_path.exists():
        logger.error(f"auto_partitions.csv not found at {csv_path}, cannot read {partition_name} Size")
        return None

    try:
        csv_contents = csv_path.read_text()
        for raw_line in csv_contents.splitlines():
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            # Skip key=value lines like FLASH_CAPACITY=4M
            if "=" in line and "," not in line:
                continue

            cols = [c.strip() for c in line.split(",")]
            if len(cols) < 3:
                continue
            if cols[0] != partition_name:
                continue

            size_str = cols[2].strip()
            if not size_str:
                logger.error(f"{partition_name} Size is empty in {csv_path}")
                return None

            # Size unit is KB, common writing: 68k / 68K / 68
            s = size_str.strip()
            if s[-1] in ("k", "K"):
                s = s[:-1].strip()

            if not s.isdigit():
                logger.error(
                    f"Invalid {partition_name} Size '{size_str}' in {csv_path}. "
                    f"Expect integer KB like '68k' or '68'."
                )
                return None

            size_kb = int(s, 10)
            logger.info(f"Read {partition_name} Size={size_str} ({size_kb} KB) from {csv_path}")
            return size_kb
    except Exception as e:
        logger.error(f"Failed to read {partition_name} Size from {csv_path}: {e}")
        return None

    logger.error(f"{partition_name} not found in {csv_path}")
    return None


def _check_primary_bootloader_size_alignment(auto_partitions_csv: Path, flash_crc_enable: bool) -> None:
    primary_bootloader_size_kb = _read_partition_size_kb_from_auto_partitions_csv(auto_partitions_csv, "primary_bootloader")
    if primary_bootloader_size_kb is None:
        raise SystemExit(1)
    align_kb = 68 if flash_crc_enable else 64
    if primary_bootloader_size_kb % align_kb != 0:
        logger.error(
            f"primary_bootloader Size ({primary_bootloader_size_kb} KB) is NOT aligned to {align_kb}K "
            f"(FLASH_CRC_ENABLE={'TRUE' if flash_crc_enable else 'FALSE'}). "
            f"Please adjust primary_bootloader Size in {auto_partitions_csv}."
        )
        raise SystemExit(1)

class bk_part:
    def __init__(
        self,
        partitions_dir: Path,
        auto_part_table: Path,
        partitions_json: Path,
        crc_enable: bool,
        aes_enable: bool,
        aes_key: str,
    ):
        if not partitions_dir.exists():
            raise RuntimeError(f"partitions directory not found: {partitions_dir}")
        if not auto_part_table.exists():
            raise FileNotFoundError(f"partitions_table not found: {auto_part_table}")

        self.partitions_dir = partitions_dir
        self.auto_part_table = auto_part_table
        self.partitions_json = partitions_json
        self.crc_enable = crc_enable
        self.aes_enable = aes_enable
        self.aes_key = aes_key
    @staticmethod
    def _partitions_setting(part_table: bk_partitions_table):
        setting_path = curr_project.flash_partitions_setting
        if not setting_path.exists():
            logger.warning("no flash limit setting check.")
            return

        setting_json: dict[str, Any] = json.loads(setting_path.read_text())
        if setting_json.get("patitions_limit"):
            setting = [
                partition_limit(**item) for item in setting_json["patitions_limit"]
            ]
            part_table.set_default_setting(setting)
        if setting_json.get("internel_partitions"):
            part_table.sort_partitions(setting_json["internel_partitions"])

    @staticmethod
    def _get_flash_size():
        setting_path = curr_project.flash_partitions_setting
        if not setting_path.exists():
            return "8M"
        setting_json: dict[str, str] = json.loads(setting_path.read_text())
        return setting_json.get("FLASH_CAPACITY", "8M")

    def auto_partition(self, partitions_txt: Path):
        partitions_csv = self.partitions_dir / "partitions.csv"
        flash_size = self._get_flash_size()
        part_table = bk_partitions_table(
            self.auto_part_table, flash_size, self.crc_enable, self.aes_enable, self.aes_key
        )
        self._partitions_setting(part_table)
        part_table.gen_partition_csv(partitions_csv)
        part_table.gen_partition_json(self.partitions_json)
        part_table.gen_pretty_format_table(partitions_txt)

        generator = bk_flash_denpendecny_generator()
        self.flash_part = bk_flash_partition(self.partitions_json, generator)

    def gen_partition_header(self):
        header_path = self.partitions_dir / "partitions_gen.h"
        self.flash_part.gen_partitions_layout_hdr(header_path)

    def gen_pack_config_json(self):
        pack_json = self.partitions_dir / "bk_package.json"
        self.flash_part.gen_pack_json(pack_json)


def auto_patitions(partitions_dir: Path, auto_part_table: Path, flash_crc_enable: bool, flash_aes_enable: bool, flash_aes_key: str):
    partitions_json = partitions_dir / "partitions.json"
    partitions_txt = partitions_dir / "partitions.txt"
    partitioner = bk_part(
        partitions_dir, auto_part_table, partitions_json, flash_crc_enable, flash_aes_enable, flash_aes_key
    )
    partitioner.auto_partition(partitions_txt)
    partitioner.gen_partition_header()
    partitioner.gen_pack_config_json()


def main():
    logger.info("Enter Armino Auto Partition")
    build_partitions_dir = curr_project.project_build_parititons_dir
    auto_part_table = curr_project.auto_partitions_table
    # Read flash_crc_enable and flash_aes_enable from auto_partitions.csv
    flash_crc_enable = _read_flash_crc_enable_from_csv(auto_part_table)
    _check_primary_bootloader_size_alignment(auto_part_table, flash_crc_enable)
    flash_aes_enable = _read_flash_aes_enable_from_csv(auto_part_table)
    flash_aes_key = _read_flash_aes_key_from_csv(auto_part_table)
    # Update curr_project.flash_crc_enable and curr_project.flash_aes_enable values
    curr_project.set_flash_crc_enable(flash_crc_enable)
    curr_project.set_flash_aes_enable(flash_aes_enable)
    curr_project.set_flash_aes_key(flash_aes_key)

    curr_project.pre_auto_partition()
    auto_patitions(build_partitions_dir, auto_part_table, flash_crc_enable, flash_aes_enable, flash_aes_key)
    curr_project.post_auto_partition()


if __name__ == "__main__":
    set_logging()
    main()
