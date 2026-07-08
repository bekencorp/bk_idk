#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Insert mfr.bin into wiz_data partition in all.bin

This module supports both CSV and XLSX partition files.
"""

import os
import csv
import logging
import shutil
from typing import Tuple

try:
    import openpyxl
    HAS_OPENPYXL = True
except ImportError:
    HAS_OPENPYXL = False

FILL_BYTE = 0xFF

def copy_file(file_name, dst):
    if os.path.exists(file_name):
        shutil.copy(file_name, f'{dst}/{file_name}')

def parse_size(size_str: str) -> int:
    """Convert size strings into bytes: 4k, 60K, 0x20000, 4096, etc."""
    if size_str is None:
        raise ValueError("Size field is empty")
    s = str(size_str).strip().lower()
    if not s:
        raise ValueError("Size field is empty string")
    if s.endswith("k"):
        return int(float(s[:-1]) * 1024)
    if s.endswith("m"):
        return int(float(s[:-1]) * 1024 * 1024)
    if s.startswith("0x"):
        return int(s, 16)
    return int(s)


def load_wiz_data_from_csv(csv_path: str) -> Tuple[int, int]:
    """
    Load wiz_data offset+size from CSV file.
    This version is tolerant of BOM, spaces, uppercase, and alternative column names.
    """
    logging.debug(f"[INFO] Loading CSV partition file: {csv_path}")
    
    # Possible alternative column names
    name_keys = {"name"}
    offset_keys = {"offset", "address", "addr", "start", "begin"}
    size_keys = {"size", "size(byte)", "length"}
    
    with open(csv_path, "r", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        
        # Normalize header names
        normalized_header = {h.strip().lower(): h for h in reader.fieldnames}
        
        # Find actual header keys in file
        def find_column(possible_keys):
            for key in normalized_header:
                if key in possible_keys:
                    return normalized_header[key]
            return None
        
        name_col = find_column(name_keys)
        offset_col = find_column(offset_keys)
        size_col = find_column(size_keys)
        
        if not name_col or not offset_col or not size_col:
            raise RuntimeError(
                f"CSV file is missing required columns.\n"
                f"Detected columns: {list(normalized_header.values())}\n"
                f"Need one name column, one offset column, one size column."
            )
        
        for row in reader:
            name = row.get(name_col, "").strip()
            if name.lower() != "wiz_data":
                continue
            
            offset_str = row.get(offset_col, "").strip()
            size_str = row.get(size_col, "").strip()
            
            if not offset_str or not size_str:
                raise RuntimeError("wiz_data row missing Offset/Size value")
            
            # parse offset
            if offset_str.lower().startswith("0x"):
                offset = int(offset_str, 16)
            else:
                offset = int(offset_str)
            
            size = parse_size(size_str)
            return offset, size
    
    raise RuntimeError("wiz_data partition not found in CSV file")


def load_wiz_data_from_excel(xlsx_path: str) -> Tuple[int, int]:
    """
    Load wiz_data offset+size from XLSX file.
    """
    if not HAS_OPENPYXL:
        raise RuntimeError("openpyxl is required for XLSX support. Install it with: pip install openpyxl")
    
    logging.debug(f"[INFO] Loading XLSX partition file: {xlsx_path}")
    
    wb = openpyxl.load_workbook(xlsx_path)
    ws = wb.active
    
    # Find header row
    header_row = None
    for idx, row in enumerate(ws.iter_rows(values_only=True), 1):
        row_lower = [str(cell).strip().lower() if cell else "" for cell in row]
        if any("name" in cell for cell in row_lower):
            header_row = idx
            break
    
    if header_row is None:
        raise RuntimeError("Could not find header row in XLSX file")
    
    # Get header
    headers = [str(cell).strip().lower() if cell else "" for cell in ws[header_row]]
    
    # Find column indices
    name_keys = {"name"}
    offset_keys = {"offset", "address", "addr", "start", "begin"}
    size_keys = {"size", "size(byte)", "length"}
    
    def find_col_idx(possible_keys):
        for idx, header in enumerate(headers):
            if header in possible_keys:
                return idx
        return None
    
    name_col = find_col_idx(name_keys)
    offset_col = find_col_idx(offset_keys)
    size_col = find_col_idx(size_keys)
    
    if name_col is None or offset_col is None or size_col is None:
        raise RuntimeError(
            f"XLSX file is missing required columns.\n"
            f"Detected columns: {headers}\n"
            f"Need one name column, one offset column, one size column."
        )
    
    # Search for wiz_data
    for row in ws.iter_rows(min_row=header_row + 1, values_only=True):
        name = str(row[name_col]).strip() if row[name_col] else ""
        if name.lower() != "wiz_data":
            continue
        
        offset_str = str(row[offset_col]).strip() if row[offset_col] else ""
        size_str = str(row[size_col]).strip() if row[size_col] else ""
        
        if not offset_str or not size_str:
            raise RuntimeError("wiz_data row missing Offset/Size value")
        
        # parse offset
        if offset_str.lower().startswith("0x"):
            offset = int(offset_str, 16)
        else:
            offset = int(offset_str)
        
        size = parse_size(size_str)
        return offset, size
    
    raise RuntimeError("wiz_data partition not found in XLSX file")


def insert_mfr(all_bin_path: str, part_path: str, mfr_bin_path: str):
    """
    Insert mfr.bin into wiz_data partition in all.bin.
    
    Args:
        all_bin_path: Path to all.bin file
        part_path: Path to partition file (CSV or XLSX)
        mfr_bin_path: Path to mfr.bin file
    
    Returns:
        Path to output file (all-app.bin)
    """
    # Determine file type
    if part_path.lower().endswith(".csv"):
        wiz_offset, wiz_size = load_wiz_data_from_csv(part_path)
    elif part_path.lower().endswith(".xlsx"):
        wiz_offset, wiz_size = load_wiz_data_from_excel(part_path)
    else:
        raise RuntimeError("Unsupported partition file. Use CSV or XLSX.")
    
    logging.debug(f"[INFO] wiz_data offset = 0x{wiz_offset:X}, size = {wiz_size} bytes")
    
    # Load all.bin
    with open(all_bin_path, "rb") as f:
        all_data = bytearray(f.read())
    
    # Load mfr.bin
    with open(mfr_bin_path, "rb") as f:
        mfr_data = f.read()
    
    if len(mfr_data) > wiz_size:
        raise RuntimeError(
            f"mfr.bin ({len(mfr_data)}) exceeds wiz_data size ({wiz_size})"
        )
    
    # Ensure all.bin is large enough
    required_size = wiz_offset + wiz_size
    if len(all_data) < required_size:
        pad = required_size - len(all_data)
        logging.debug(f"[INFO] Padding all.bin by {pad} bytes")
        all_data.extend([FILL_BYTE] * pad)
    
    # Clear wiz_data region
    logging.debug("[INFO] Clearing wiz_data region...")
    for i in range(wiz_size):
        all_data[wiz_offset + i] = FILL_BYTE
    
    # Write mfr.bin
    logging.debug("[INFO] Writing mfr.bin...")
    all_data[wiz_offset:wiz_offset + len(mfr_data)] = mfr_data
    
    out_path = os.path.join(os.path.dirname(all_bin_path), "all-app.bin")
    with open(out_path, "wb") as f:
        f.write(all_data)
    
    logging.info(f"[OK] Output file created: {out_path}")

    install_dir = 'install'
    os.makedirs(install_dir, exist_ok=True)
    copy_file('all-app.bin', install_dir)

