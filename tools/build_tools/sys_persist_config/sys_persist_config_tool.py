#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
配置文件转换工具：将 CSV 配置文件转换为二进制文件或 C 代码
支持的命令: csv2bin, gen_code
"""

import argparse
import sys
import os
import struct
import zlib
from typing import Dict, List, Tuple, Optional
import pandas as pd

# wpa_supplicant CRC32 表（IEEE 802.11 FCS CRC32）
WPA_CRC32_TABLE = [
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
    0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
    0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
    0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
    0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
    0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
    0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
    0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
    0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
    0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
    0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
    0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
    0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
    0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
    0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
    0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
    0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
    0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
    0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
    0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
    0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
    0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
    0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
    0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
    0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
    0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
    0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
    0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
    0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
    0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
    0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
    0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
    0x2d02ef8d
]

def wpa_crc32(data: bytearray) -> int:
    """
    使用与 wpa_supplicant 完全相同的 CRC32 计算方法
    
    Args:
        data: 要计算 CRC 的数据（bytearray 或 bytes）
        
    Returns:
        CRC32 值（32位无符号整数）
    """
    crc = 0xFFFFFFFF
    for byte in data:
        crc = WPA_CRC32_TABLE[(crc ^ byte) & 0xFF] ^ (crc >> 8)
    return (~crc) & 0xFFFFFFFF


class ConfigConverter:
    """配置转换器类"""
    
    def __init__(self, csv_file: str):
        """
        初始化配置转换器
        
        Args:
            csv_file: CSV 文件路径
        """
        self.csv_file = csv_file
        self.df = None
        self.bin_data = bytearray()
        self.crc_offset = None  # CRC字段的偏移量
        self.crc_size = 0  # CRC字段的大小
        self.magic_offset = None  # Magic字段的偏移量
        self.magic_size = 0  # Magic字段的大小
        self.tlv_type_counter = 0  # TLV type计数器
        
    def load_csv(self) -> bool:
        """
        加载CSV文件
        
        Returns:
            bool: 成功返回True，失败返回False
        """
        try:
            self.df = pd.read_csv(self.csv_file)
            return True
        except Exception as e:
            print(f"Error: failed to read CSV file {self.csv_file}: {e}")
            return False
    
    def check_required_columns(self) -> bool:
        """
        检查必需的列是否存在
        
        Returns:
            bool: 所有必需列存在返回True，否则返回False
        """
        required_columns = ['space', 'offset', 'size', 'field', 'value', 'reg', 'mask']
        
        # 检查bits_value列（可能是value.1）
        if 'bits_value' in self.df.columns:
            self.bits_value_col = 'bits_value'
        elif 'value.1' in self.df.columns:
            self.bits_value_col = 'value.1'
        else:
            print("Error: required column 'bits_value' (or 'value.1') not found")
            return False
        
        for col in required_columns:
            if col not in self.df.columns:
                print(f"Error: required column '{col}' not found")
                return False
        
        return True
    
    def parse_hex(self, value) -> Optional[int]:
        """
        解析十六进制值
        
        Args:
            value: 可能是字符串、数字或NaN的值
            
        Returns:
            int: 解析后的整数值，失败返回None
        """
        if pd.isna(value):
            return None
        
        if isinstance(value, (int, float)):
            return int(value)
        
        if isinstance(value, str):
            value = value.strip()
            if value.startswith('0x') or value.startswith('0X'):
                try:
                    return int(value, 16)
                except ValueError:
                    return None
            try:
                return int(value, 16) if len(value) > 0 else None
            except ValueError:
                return None
        
        return None
    
    def version_to_hex(self, version_str: str) -> Optional[int]:
        """
        将点分十进制版本号转换为十六进制
        
        Args:
            version_str: 版本字符串，如 "16.1.1.1"
            
        Returns:
            int: 十六进制值，如 0x10010101
        """
        try:
            parts = version_str.strip().split('.')
            if len(parts) != 4:
                return None
            
            values = [int(p) for p in parts]
            if any(v < 0 or v > 255 for v in values):
                return None
            
            # 格式: aa.bb.cc.dd -> 0xaabbccdd
            result = (values[0] << 24) | (values[1] << 16) | (values[2] << 8) | values[3]
            return result
        except Exception:
            return None
    
    def convert_csv_to_bin(self) -> bool:
        """
        将CSV数据转换为二进制
        
        Returns:
            bool: 成功返回True，失败返回False
        """
        if self.df is None:
            print("Error: CSV file is not loaded")
            return False
        
        # 过滤掉空行（所有关键字段都为空）
        valid_rows = self.df[
            (self.df['offset'].notna()) | 
            (self.df['size'].notna()) | 
            (self.df['field'].notna())
        ].copy()
        
        if len(valid_rows) == 0:
            print("Error: no valid data rows in the CSV file")
            return False
        
        # 确保bin_data足够大，初始化为0
        max_offset = 0
        for idx, row in valid_rows.iterrows():
            offset_val = row.get('offset')
            size_val = row.get('size')
            if pd.notna(offset_val) and pd.notna(size_val):
                try:
                    offset = int(offset_val)
                    size = int(size_val)
                    max_offset = max(max_offset, offset + size)
                except (ValueError, TypeError):
                    pass
        
        self.bin_data = bytearray(max_offset)
        prev_offset = None
        prev_size = None
        
        # 第一遍遍历：处理除CRC外的所有字段
        for idx, row in valid_rows.iterrows():
            space = row.get('space')
            offset_val = row.get('offset')
            size_val = row.get('size')
            field = row.get('field')
            value = row.get('value')
            reg = row.get('reg')
            mask = row.get('mask')
            bits_value = row.get(self.bits_value_col)
            
            # 跳过完全空的行
            if pd.isna(offset_val) and pd.isna(size_val) and pd.isna(field):
                continue
            
            # 处理offset和size
            if pd.isna(offset_val):
                # 如果offset为空，使用上一行的offset + size
                if prev_offset is not None and prev_size is not None:
                    offset = prev_offset + prev_size
                else:
                    print(f"Error: line {idx+2} offset is empty and no previous row to infer from")
                    return False
            else:
                offset = int(offset_val)
            
            if pd.isna(size_val):
                print(f"Error: line {idx+2} size is empty")
                return False
            else:
                size = int(size_val)
            
            # 检查offset合法性（如果offset不为空）
            if pd.notna(offset_val):
                if prev_offset is not None and prev_size is not None:
                    expected_offset = prev_offset + prev_size
                    if offset != expected_offset:
                        print(f"Error: line {idx+2} offset={offset}, expected offset={expected_offset} (previous offset={prev_offset} + size={prev_size})")
                        return False
            
            # 检查space不为空时，offset必须16字节对齐
            if pd.notna(space) and str(space).strip() != '':
                if offset % 16 != 0:
                    print(f"Error: line {idx+2} space='{space}' is set but offset={offset} is not 16-byte aligned")
                    return False
            
            # 确保bin_data足够大
            required_size = offset + size
            if len(self.bin_data) < required_size:
                self.bin_data.extend(bytearray(required_size - len(self.bin_data)))
            
            # 处理不同的field类型
            if pd.isna(field):
                # field为空，可能是延续上一行的数据（如tlv_value的后续部分）
                field = 'continue'
            else:
                field = str(field).strip().lower()
            
            if field == 'crc' or field == 'crc32':
                # CRC字段，记录偏移量和大小，最后计算
                self.crc_offset = offset
                self.crc_size = size
                prev_offset = offset
                prev_size = size
                continue
            
            elif field == 'magic':
                # Magic字段，记录偏移量和大小，计算CRC时排除
                self.magic_offset = offset
                self.magic_size = size
                # 写入magic值
                if pd.notna(value):
                    hex_value = self.parse_hex(value)
                    if hex_value is not None:
                        if size == 4:
                            struct.pack_into('<I', self.bin_data, offset, hex_value)
                        else:
                            for i in range(size):
                                self.bin_data[offset + i] = (hex_value >> (i * 8)) & 0xFF
            
            elif field == 'version':
                # 版本字段，点分十进制转十六进制
                if pd.isna(value):
                    print(f"Error: line {idx+2} field=version but value is empty")
                    return False
                
                version_hex = self.version_to_hex(str(value))
                if version_hex is None:
                    print(f"Error: line {idx+2} version value '{value}' is invalid, expected dotted decimal like '16.1.1.1'")
                    return False
                
                struct.pack_into('<I', self.bin_data, offset, version_hex)
            
            elif field == 'name':
                # 名称字段，写入字符串
                if pd.isna(value):
                    value_str = ''
                else:
                    value_str = str(value)
                
                if len(value_str) >= size:
                    print(f"Error: line {idx+2} field=name string length {len(value_str)} >= size {size}")
                    return False
                
                # 写入字符串（不包括null终止符，如果size足够可以包含）
                encoded = value_str.encode('utf-8', errors='ignore')
                actual_len = min(len(encoded), size)
                self.bin_data[offset:offset+actual_len] = encoded[:actual_len]
            
            elif field == 'tlv_type' or field == 'type':
                # TLV type字段，按出现顺序写入
                # 检查对应的 tlv_value 行是否有 reg, mask, bits_value
                has_reg_mask_value = False
                # 查找后续的 tlv_value 行
                for j in range(idx + 1, len(valid_rows)):
                    r = valid_rows.iloc[j]
                    r_space = r.get('space')
                    if pd.isna(r_space) or str(r_space).strip() == '':
                        r_field = r.get('field')
                        if pd.notna(r_field):
                            r_field_str = str(r_field).strip().lower()
                            if r_field_str == 'value' or r_field_str == 'tlv_value':
                                # 检查 reg, mask, bits_value 列
                                r_reg = r.get('reg')
                                r_mask = r.get('mask')
                                r_bits_value = None
                                if hasattr(self, 'bits_value_col') and self.bits_value_col:
                                    r_bits_value = r.get(self.bits_value_col)
                                if r_bits_value is None and 'bits_value' in valid_rows.columns:
                                    r_bits_value = r.get('bits_value')
                                if r_bits_value is None and 'value.1' in valid_rows.columns:
                                    r_bits_value = r.get('value.1')
                                
                                has_reg = pd.notna(r_reg) and str(r_reg).strip() != '' and str(r_reg).strip() != '-'
                                has_mask = pd.notna(r_mask) and str(r_mask).strip() != '' and str(r_mask).strip() != '-'
                                has_bits_value = pd.notna(r_bits_value) and str(r_bits_value).strip() != '' and str(r_bits_value).strip() != '-'
                                
                                if has_reg and has_mask and has_bits_value:
                                    has_reg_mask_value = True
                                break
                            elif r_field_str == 'type' or r_field_str == 'tlv_type':
                                # 遇到下一个 tlv_type，停止查找
                                break
                    elif pd.notna(r_space) and str(r_space).strip() != 'tlv_base':
                        # 遇到新的 space，停止查找
                        break
                
                # 如果有 reg_mask_value，将 bit15 置1
                tlv_type_value = self.tlv_type_counter
                if has_reg_mask_value:
                    tlv_type_value = self.tlv_type_counter | 0x8000
                
                # 根据size写入相应字节数
                if size == 1:
                    struct.pack_into('<B', self.bin_data, offset, tlv_type_value & 0xFF)
                elif size == 2:
                    struct.pack_into('<H', self.bin_data, offset, tlv_type_value & 0xFFFF)
                elif size == 4:
                    struct.pack_into('<I', self.bin_data, offset, tlv_type_value & 0xFFFFFFFF)
                else:
                    # 其他size，按字节写入
                    for i in range(size):
                        self.bin_data[offset + i] = (tlv_type_value >> (i * 8)) & 0xFF
                self.tlv_type_counter += 1
            
            elif field == 'value' or field == 'tlv_value':
                # TLV value字段
                # 检查：要么value非空，要么reg/mask/bits_value三个都非空
                has_value = pd.notna(value) and str(value).strip() != '' and str(value).strip() != '-'
                has_reg = pd.notna(reg) and str(reg).strip() != '' and str(reg).strip() != '-'
                has_mask = pd.notna(mask) and str(mask).strip() != '' and str(mask).strip() != '-'
                has_bits_value = pd.notna(bits_value) and str(bits_value).strip() != '' and str(bits_value).strip() != '-'
                
                if not has_value and not (has_reg and has_mask and has_bits_value):
                    print(f"Error: line {idx+2} field=value/tlv_value must satisfy: value is not empty OR (reg/mask/bits_value are all provided)")
                    return False
                
                if has_value:
                    # 使用value值
                    hex_value = self.parse_hex(value)
                    if hex_value is None:
                        print(f"Error: line {idx+2} value='{value}' cannot be parsed as hex")
                        return False
                    
                    # 根据size写入相应字节数
                    if size == 1:
                        struct.pack_into('<B', self.bin_data, offset, hex_value & 0xFF)
                    elif size == 2:
                        struct.pack_into('<H', self.bin_data, offset, hex_value & 0xFFFF)
                    elif size == 4:
                        struct.pack_into('<I', self.bin_data, offset, hex_value & 0xFFFFFFFF)
                    elif size == 8:
                        struct.pack_into('<Q', self.bin_data, offset, hex_value & 0xFFFFFFFFFFFFFFFF)
                    else:
                        # 其他size，按字节写入
                        for i in range(size):
                            self.bin_data[offset + i] = (hex_value >> (i * 8)) & 0xFF
                
                else:
                    # 使用reg/mask/bits_value
                    if size != 12:
                        print(f"Error: line {idx+2} using reg/mask/bits_value requires size=12, current size={size}")
                        return False
                    
                    # 检查reg和mask是否为0x开头的十六进制
                    reg_str = str(reg).strip()
                    mask_str = str(mask).strip()
                    bits_value_str = str(bits_value).strip()
                    
                    if not reg_str.startswith('0x') and not reg_str.startswith('0X'):
                        print(f"Error: line {idx+2} reg='{reg_str}' must start with 0x")
                        return False
                    if not mask_str.startswith('0x') and not mask_str.startswith('0X'):
                        print(f"Error: line {idx+2} mask='{mask_str}' must start with 0x")
                        return False
                    
                    # bits_value可以是0x开头或普通数字
                    reg_val = self.parse_hex(reg_str)
                    mask_val = self.parse_hex(mask_str)
                    bits_val = self.parse_hex(bits_value_str)
                    
                    if reg_val is None or mask_val is None or bits_val is None:
                        print(f"Error: line {idx+2} failed to parse reg/mask/bits_value")
                        return False
                    
                    # 写入：reg到offset，mask到offset+4，bits_value到offset+8
                    struct.pack_into('<I', self.bin_data, offset, reg_val)
                    struct.pack_into('<I', self.bin_data, offset + 4, mask_val)
                    struct.pack_into('<I', self.bin_data, offset + 8, bits_val)
            
            elif field == 'continue':
                # 延续字段，通常用于多行的tlv_value
                # 检查是否有reg/mask/bits_value
                has_reg = pd.notna(reg) and str(reg).strip() != '' and str(reg).strip() != '-'
                has_mask = pd.notna(mask) and str(mask).strip() != '' and str(mask).strip() != '-'
                has_bits_value = pd.notna(bits_value) and str(bits_value).strip() != '' and str(bits_value).strip() != '-'
                
                if has_reg and has_mask and has_bits_value:
                    if size != 12:
                        print(f"Error: line {idx+2} using reg/mask/bits_value requires size=12, current size={size}")
                        return False
                    
                    reg_str = str(reg).strip()
                    mask_str = str(mask).strip()
                    bits_value_str = str(bits_value).strip()
                    
                    if not reg_str.startswith('0x') and not reg_str.startswith('0X'):
                        print(f"Error: line {idx+2} reg='{reg_str}' must start with 0x")
                        return False
                    if not mask_str.startswith('0x') and not mask_str.startswith('0X'):
                        print(f"Error: line {idx+2} mask='{mask_str}' must start with 0x")
                        return False
                    
                    # bits_value可以是0x开头或普通数字
                    reg_val = self.parse_hex(reg_str)
                    mask_val = self.parse_hex(mask_str)
                    bits_val = self.parse_hex(bits_value_str)
                    
                    if reg_val is None or mask_val is None or bits_val is None:
                        print(f"Error: line {idx+2} failed to parse reg/mask/bits_value")
                        return False
                    
                    struct.pack_into('<I', self.bin_data, offset, reg_val)
                    struct.pack_into('<I', self.bin_data, offset + 4, mask_val)
                    struct.pack_into('<I', self.bin_data, offset + 8, bits_val)
                elif pd.notna(value) and str(value).strip() != '' and str(value).strip() != '-':
                    # 使用value
                    hex_value = self.parse_hex(value)
                    if hex_value is None:
                        print(f"Error: line {idx+2} value='{value}' cannot be parsed as hex")
                        return False
                    
                    if size == 1:
                        struct.pack_into('<B', self.bin_data, offset, hex_value & 0xFF)
                    elif size == 2:
                        struct.pack_into('<H', self.bin_data, offset, hex_value & 0xFFFF)
                    elif size == 4:
                        struct.pack_into('<I', self.bin_data, offset, hex_value & 0xFFFFFFFF)
                    elif size == 8:
                        struct.pack_into('<Q', self.bin_data, offset, hex_value & 0xFFFFFFFFFFFFFFFF)
                    else:
                        for i in range(size):
                            self.bin_data[offset + i] = (hex_value >> (i * 8)) & 0xFF
                else:
                    # 默认为0或其他值
                    if pd.notna(value):
                        hex_value = self.parse_hex(value)
                        if hex_value is not None:
                            if size == 1:
                                struct.pack_into('<B', self.bin_data, offset, hex_value & 0xFF)
                            elif size == 2:
                                struct.pack_into('<H', self.bin_data, offset, hex_value & 0xFFFF)
                            elif size == 4:
                                struct.pack_into('<I', self.bin_data, offset, hex_value & 0xFFFFFFFF)
                            elif size == 8:
                                struct.pack_into('<Q', self.bin_data, offset, hex_value & 0xFFFFFFFFFFFFFFFF)
                            else:
                                for i in range(size):
                                    self.bin_data[offset + i] = (hex_value >> (i * 8)) & 0xFF
            
            else:
                # 其他字段，尝试解析value
                # 特殊处理reserved字段：使用value中的值填充整个字段
                if field == 'reserved':
                    fill_value = 0
                    if pd.notna(value):
                        hex_value = self.parse_hex(value)
                        if hex_value is not None:
                            fill_value = hex_value & 0xFF  # 使用单个字节值填充
                    # 用fill_value填充整个reserved字段
                    for i in range(size):
                        self.bin_data[offset + i] = fill_value
                elif pd.notna(value):
                    hex_value = self.parse_hex(value)
                    if hex_value is not None:
                        if size == 1:
                            struct.pack_into('<B', self.bin_data, offset, hex_value & 0xFF)
                        elif size == 2:
                            struct.pack_into('<H', self.bin_data, offset, hex_value & 0xFFFF)
                        elif size == 4:
                            struct.pack_into('<I', self.bin_data, offset, hex_value & 0xFFFFFFFF)
                        elif size == 8:
                            struct.pack_into('<Q', self.bin_data, offset, hex_value & 0xFFFFFFFFFFFFFFFF)
                        else:
                            for i in range(size):
                                self.bin_data[offset + i] = (hex_value >> (i * 8)) & 0xFF
            
            prev_offset = offset
            prev_size = size
        
        # 16字节对齐，补0xFF
        current_size = len(self.bin_data)
        aligned_size = ((current_size + 15) // 16) * 16
        if aligned_size > current_size:
            padding = aligned_size - current_size
            self.bin_data.extend(bytearray([0xFF] * padding))
        
        # 计算CRC32并写入
        if self.crc_offset is not None:
            # 计算除magic和crc之外所有内容的CRC32
            # 使用已保存的 magic_size 和 crc_size
            magic_size = self.magic_size if self.magic_offset is not None else 0
            crc_size = self.crc_size
            
            # 构建CRC数据：排除magic和crc字段的所有数据
            crc_data = bytearray()
            skipped_magic = 0
            skipped_crc = 0
            for i in range(len(self.bin_data)):
                # 跳过magic字段的范围
                if self.magic_offset is not None:
                    if self.magic_offset <= i < self.magic_offset + magic_size:
                        skipped_magic += 1
                        continue
                # 跳过crc字段的范围
                if self.crc_offset <= i < self.crc_offset + crc_size:
                    skipped_crc += 1
                    continue
                # 包含其余所有数据（包括对齐填充）
                crc_data.append(self.bin_data[i])
            
            # 计算CRC32
            # 使用与 wpa_supplicant 完全相同的 CRC32 计算方法
            # wpa_supplicant crc32() 函数的行为：
            # 1. 初始值：0xFFFFFFFF
            # 2. 计算：crc = crc32_table[(crc ^ frame[i]) & 0xff] ^ (crc >> 8)
            # 3. 返回：~crc
            crc32_value = wpa_crc32(crc_data)
            struct.pack_into('<I', self.bin_data, self.crc_offset, crc32_value)
            expected_crc_data_size = len(self.bin_data) - (magic_size if self.magic_offset is not None else 0) - crc_size
            print(f"CRC32: 0x{crc32_value:08X}, written to offset {self.crc_offset}")
            print(f"  bin_data size: {len(self.bin_data)} bytes")
            print(f"  magic: offset={self.magic_offset}, size={magic_size}")
            print(f"  crc: offset={self.crc_offset}, size={crc_size}")
            print(f"  CRC data size: {len(crc_data)} bytes (expected: {expected_crc_data_size} bytes)")
        
        return True
    
    def save_bin(self, output_file: str) -> bool:
        """
        保存二进制文件（16字节对齐和4k对齐）
        
        Args:
            output_file: 输出文件路径
            
        Returns:
            bool: 成功返回True，失败返回False
        """
        try:
            # 保存16字节对齐的bin文件
            with open(output_file, 'wb') as f:
                f.write(self.bin_data)
            print(f"Generated binary: {output_file} (size: {len(self.bin_data)} bytes, 16-byte aligned)")
            
            # 生成4k对齐的bin文件
            base_name = os.path.splitext(output_file)[0]
            output_file_4k = base_name + '_4k.bin'
            
            # 创建4k对齐的数据
            current_size = len(self.bin_data)
            aligned_size_4k = ((current_size + 4095) // 4096) * 4096
            bin_data_4k = bytearray(self.bin_data)
            
            if aligned_size_4k > current_size:
                padding = aligned_size_4k - current_size
                bin_data_4k.extend(bytearray([0xFF] * padding))
            
            with open(output_file_4k, 'wb') as f:
                f.write(bin_data_4k)
            print(f"Generated binary: {output_file_4k} (size: {len(bin_data_4k)} bytes, 4k aligned)")
            
            return True
        except Exception as e:
            print(f"Error: failed to save binary file {output_file}: {e}")
            return False


def csv2bin(csv_file: str, output_file: Optional[str] = None) -> bool:
    """
    将CSV文件转换为二进制文件
    
    Args:
        csv_file: CSV文件路径
        output_file: 输出二进制文件路径，如果为None则自动生成
        
    Returns:
        bool: 成功返回True，失败返回False
    """
    if not os.path.exists(csv_file):
        print(f"Error: CSV file does not exist: {csv_file}")
        return False
    
    if output_file is None:
        # 自动生成输出文件名
        base_name = os.path.splitext(csv_file)[0]
        output_file = base_name + '.bin'
    
    converter = ConfigConverter(csv_file)
    
    # 加载CSV
    if not converter.load_csv():
        return False
    
    # 检查必需的列
    if not converter.check_required_columns():
        return False
    
    # 转换
    if not converter.convert_csv_to_bin():
        return False
    
    # 保存
    if not converter.save_bin(output_file):
        return False
    
    return True


class CodeGenerator:
    """C代码生成器类"""
    
    def __init__(self, csv_file: str):
        """
        初始化代码生成器
        
        Args:
            csv_file: CSV 文件路径
        """
        self.csv_file = csv_file
        self.df = None
        self.bits_value_col = None
        
    def load_csv(self) -> bool:
        """
        加载CSV文件
        
        Returns:
            bool: 成功返回True，失败返回False
        """
        try:
            self.df = pd.read_csv(self.csv_file)
            return True
        except Exception as e:
            print(f"Error: failed to read CSV file {self.csv_file}: {e}")
            return False
    
    def check_required_columns(self) -> bool:
        """
        检查必需的列是否存在
        
        Returns:
            bool: 所有必需列存在返回True，否则返回False
        """
        required_columns = ['space', 'offset', 'size', 'field', 'value']
        
        # 检查bits_value列（可能是value.1）
        if 'bits_value' in self.df.columns:
            self.bits_value_col = 'bits_value'
        elif 'value.1' in self.df.columns:
            self.bits_value_col = 'value.1'
        
        for col in required_columns:
            if col not in self.df.columns:
                print(f"Error: required column '{col}' not found")
                return False
        
        return True
    
    def generate_code(self, output_dir: Optional[str] = None) -> bool:
        """
        生成C代码
        
        Args:
            output_dir: 输出目录，如果为None则使用CSV文件所在目录
            
        Returns:
            bool: 成功返回True，失败返回False
        """
        if output_dir is None:
            output_dir = os.path.dirname(os.path.abspath(self.csv_file))
            if not output_dir:
                output_dir = '.'
        
        os.makedirs(output_dir, exist_ok=True)
        
        h_file = os.path.join(output_dir, 'sys_persist_config.h')
        c_file = os.path.join(output_dir, 'sys_persist_config.c')
        cli_file = os.path.join(output_dir, 'sys_persist_config_cli.c')
        cli_h_file = os.path.join(output_dir, 'sys_persist_config_cli.h')
        
        # 生成头文件
        if not self.generate_header(h_file):
            return False
        
        # 生成源文件
        if not self.generate_source(c_file):
            return False

        # 生成 CLI 头/源文件
        if not self.generate_cli(cli_file, cli_h_file):
            return False
        
        print("Generated C code files:")
        print(f"  Header: {h_file}")
        print(f"  Source: {c_file}")
        print(f"  CLI source: {cli_file}")
        print(f"  CLI header: {cli_h_file}")
        return True
    
    def generate_header(self, h_file: str) -> bool:
        """
        生成头文件
        
        Args:
            h_file: 头文件路径
            
        Returns:
            bool: 成功返回True，失败返回False
        """
        try:
            with open(h_file, 'w', encoding='utf-8') as f:
                # 配置文件存在时，CONFIG_SYS_PERSIST_SIZE 固定写为 0x1000
                config_size = 0x1000
                
                # 计算16字节对齐后的实际长度
                valid_rows = self.df[
                    (self.df['offset'].notna()) | 
                    (self.df['size'].notna()) | 
                    (self.df['field'].notna())
                ].copy()
                
                max_offset = 0
                prev_offset = None
                prev_size = None
                for idx, row in valid_rows.iterrows():
                    offset_val = row.get('offset')
                    size_val = row.get('size')
                    
                    if pd.isna(offset_val):
                        if prev_offset is not None and prev_size is not None:
                            offset = prev_offset + prev_size
                        else:
                            continue
                    else:
                        offset = int(offset_val)
                    
                    if pd.isna(size_val):
                        continue
                    else:
                        size = int(size_val)
                    
                    max_offset = max(max_offset, offset + size)
                    prev_offset = offset
                    prev_size = size
                
                # 16字节对齐
                aligned_size = ((max_offset + 15) // 16) * 16 if max_offset > 0 else 0
                
                # 写入文件头
                f.write("""#ifndef SYS_PERSIST_CONFIG_H
#define SYS_PERSIST_CONFIG_H

#define CONFIG_SYS_PERSIST_SIZE 0x""")
                f.write(f"{config_size:X}\n")
                f.write(f"#define CONFIG_SYS_PERSIST_ALIGNED_SIZE 0x{aligned_size:X}\n\n")
                f.write("""/* C代码部分，仅在非链接器脚本环境中包含 */
#ifndef __ASSEMBLER__
#include <stdint.h>
#include "partitions.h"

#ifndef SYS_PERSIST_CONFIG_FLASH_OFFSET
#define SYS_PERSIST_CONFIG_FLASH_OFFSET (CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_START + 0x02000000)
#endif

/* 数据结构定义 */
typedef struct reg_mask_value {
    uint32_t reg;
    uint32_t mask;
    uint32_t value;
} reg_mask_value_t;

typedef struct reg_mask_value_tlv {
    uint16_t type;
    uint16_t length;
    reg_mask_value_t rmv[0];
} reg_mask_value_tlv_t;

typedef struct sys_persist_tlv {
    uint16_t type;
    uint16_t length;
    uint8_t data[0];
} sys_persist_tlv_t;

""")
                
                # 解析CSV生成代码
                valid_rows = self.df[
                    (self.df['offset'].notna()) | 
                    (self.df['size'].notna()) | 
                    (self.df['field'].notna())
                ].copy()
                
                if len(valid_rows) == 0:
                    print("Error: no valid data rows in the CSV file")
                    return False
                
                # 用于跟踪当前处理的space和结构体信息
                current_space = None
                current_struct_name = None
                struct_members = []
                struct_member_names = set()  # 用于去重
                tlv_type_counter = 0
                prev_offset = None
                prev_size = None
                
                # 第一遍遍历：生成宏定义和结构体声明
                for idx, row in valid_rows.iterrows():
                    space = row.get('space')
                    offset_val = row.get('offset')
                    size_val = row.get('size')
                    field = row.get('field')
                    value = row.get('value')
                    
                    # 跳过完全空的行
                    if pd.isna(offset_val) and pd.isna(size_val) and pd.isna(field):
                        continue
                    
                    # 计算offset
                    if pd.isna(offset_val):
                        if prev_offset is not None and prev_size is not None:
                            offset = prev_offset + prev_size
                        else:
                            continue
                    else:
                        offset = int(offset_val)
                    
                    if pd.isna(size_val):
                        continue
                    else:
                        size = int(size_val)
                    
                    # 处理space不为空的情况
                    if pd.notna(space) and str(space).strip() != '':
                        space_str = str(space).strip()
                        
                        # 如果之前有未关闭的结构体，先关闭它
                        if current_space and current_space != 'tlv_base' and current_struct_name:
                            f.write(f"}} {current_struct_name}_t;\n\n")
                        
                        current_space = space_str
                        
                        # 4.1.1 生成宏定义
                        macro_name = f"SYS_PERSIST_CONFIG_{space_str.upper().replace('-', '_')}"
                        f.write(f"#define {macro_name} ({offset} + SYS_PERSIST_CONFIG_FLASH_OFFSET)\n")
                        
                        # 4.1.2 如果space不为tlv_base，创建结构体
                        if space_str != 'tlv_base':
                            # 去掉_base后缀
                            struct_name_base = space_str.replace('_base', '')
                            current_struct_name = f"sys_persist_{struct_name_base}"
                            struct_members = []
                            struct_member_names = set()
                            
                            # 找到对应的name字段（提前查找）
                            name_value = None
                            name_size = None
                            
                            # 首先检查当前行是否就是name字段
                            if pd.notna(field) and str(field).strip().lower() == 'name':
                                name_value = value
                                name_size = size_val
                            
                            # 如果在当前行没找到，继续在当前space下的行查找name字段
                            if name_value is None:
                                for j in range(idx + 1, len(valid_rows)):
                                    r = valid_rows.iloc[j]
                                    r_space = r.get('space')
                                    if pd.isna(r_space) or str(r_space).strip() == '':
                                        # space为空，继续在当前space中查找
                                        r_field = r.get('field')
                                        if pd.notna(r_field) and str(r_field).strip().lower() == 'name':
                                            name_value = r.get('value')
                                            name_size = r.get('size')
                                            break
                                    elif pd.notna(r_space) and str(r_space).strip() != '' and str(r_space).strip() != space_str:
                                        # 遇到了新的space，停止查找
                                        break
                            
                            # 生成结构体开始
                            f.write(f"\n/* Structure for {space_str} */\n")
                            f.write(f"typedef struct {current_struct_name} {{\n")
                            
                            # 第一个成员：name字段（char数组），如果存在的话
                            if name_value is not None and name_size is not None:
                                name_str = str(name_value).strip()
                                name_size_int = int(name_size)
                                f.write(f"    char {name_str}[{name_size_int}];\n")
                                struct_members.append(f"    char {name_str}[{name_size_int}];")
                                struct_member_names.add('name')  # 使用'name'作为标记，避免重复添加
                            
                            # 检查当前行（space不为空的行）的field，如果是magic等字段，也要添加
                            if pd.notna(field):
                                field_str = str(field).strip().lower()
                                if field_str == 'magic':
                                    # Magic字段，根据size确定类型
                                    if size == 4:
                                        member_type = 'uint32_t magic'
                                    elif size == 2:
                                        member_type = 'uint16_t magic'
                                    elif size == 1:
                                        member_type = 'uint8_t magic'
                                    else:
                                        member_type = f'uint8_t magic[{size}]'
                                    if 'magic' not in struct_member_names:
                                        f.write(f"    {member_type};\n")
                                        struct_member_names.add('magic')
                                elif field_str not in ['name', 'type', 'tlv_type']:
                                    # 其他字段（除了name和tlv_type），也要处理
                                    member_type = None
                                    if field_str == 'crc' or field_str == 'crc32':
                                        member_type = 'uint32_t'
                                    elif field_str == 'version':
                                        member_type = 'uint32_t'
                                    elif field_str == 'reserved':
                                        member_type = f'uint8_t reserved[{size}]'
                                    elif pd.notna(value):
                                        if size == 4:
                                            member_type = 'uint32_t'
                                        elif size == 2:
                                            member_type = 'uint16_t'
                                        elif size == 1:
                                            member_type = 'uint8_t'
                                        else:
                                            member_type = f'uint8_t {field_str}[{size}]'
                                    else:
                                        if size == 4:
                                            member_type = 'uint32_t'
                                        elif size == 2:
                                            member_type = 'uint16_t'
                                        elif size == 1:
                                            member_type = 'uint8_t'
                                        else:
                                            member_type = f'uint8_t {field_str}[{size}]'
                                    
                                    if member_type and field_str not in struct_member_names:
                                        if field_str in member_type:
                                            f.write(f"    {member_type};\n")
                                        else:
                                            f.write(f"    {member_type} {field_str};\n")
                                        struct_member_names.add(field_str)
                        
                        # 注意：getter 声明在结构体关闭后生成；实现在 sys_persist_config.c
                    
                    # 处理space为空的情况
                    elif pd.isna(space) or str(space).strip() == '':
                        if pd.notna(field):
                            field_str = str(field).strip().lower()
                            
                            # 4.2 如果space不为tlv_base，添加结构体成员
                            if current_space and current_space != 'tlv_base' and current_struct_name:
                                # 处理name字段
                                if field_str == 'name':
                                    # name字段应该已经作为第一个成员添加，但如果没有，则添加
                                    if 'name' not in struct_member_names:
                                        if pd.notna(value) and pd.notna(size_val):
                                            name_str = str(value).strip()
                                            name_size_int = int(size_val)
                                            f.write(f"    char {name_str}[{name_size_int}];\n")
                                            struct_member_names.add('name')
                                # 处理其他字段
                                else:
                                    member_type = None
                                    
                                    # 4.2.2-4.2.5 根据field或value确定类型
                                    if field_str == 'crc' or field_str == 'crc32':
                                        member_type = 'uint32_t'
                                    elif field_str == 'version':
                                        member_type = 'uint32_t'
                                    elif field_str == 'magic':
                                        # Magic字段，根据size确定类型
                                        if size == 4:
                                            member_type = 'uint32_t'
                                        elif size == 2:
                                            member_type = 'uint16_t'
                                        elif size == 1:
                                            member_type = 'uint8_t'
                                        else:
                                            member_type = f'uint8_t magic[{size}]'
                                    elif field_str == 'reserved':
                                        member_type = f'uint8_t reserved[{size}]'
                                    elif pd.notna(value):
                                        # 判断value是否为数字
                                        value_str = str(value).strip()
                                        if size == 4:
                                            member_type = 'uint32_t'
                                        elif size == 2:
                                            member_type = 'uint16_t'
                                        elif size == 1:
                                            member_type = 'uint8_t'
                                        else:
                                            member_type = f'uint8_t {field_str}[{size}]'
                                    else:
                                        # 默认根据size确定类型
                                        if size == 4:
                                            member_type = 'uint32_t'
                                        elif size == 2:
                                            member_type = 'uint16_t'
                                        elif size == 1:
                                            member_type = 'uint8_t'
                                        else:
                                            member_type = f'uint8_t {field_str}[{size}]'
                                    
                                    if member_type:
                                        # 检查是否已经添加过（避免重复）
                                        if field_str not in struct_member_names:
                                            # 如果member_type已经包含字段名，直接使用；否则添加字段名
                                            if field_str in member_type:
                                                f.write(f"    {member_type};\n")
                                            else:
                                                f.write(f"    {member_type} {field_str};\n")
                                            struct_member_names.add(field_str)
                            # 4.3 处理tlv_type
                            elif current_space == 'tlv_base' and (field_str == 'type' or field_str == 'tlv_type'):
                                if pd.notna(value):
                                    value_str = str(value).strip().upper().replace('-', '_')
                                    macro_name_tlv = f"SYS_PERSIST_CONFIG_TLV_{value_str}"
                                    
                                    # 检查对应的 tlv_value 行是否有 reg, mask, bits_value
                                    has_reg_mask_value = False
                                    # 查找后续的 tlv_value 行
                                    for j in range(idx + 1, len(valid_rows)):
                                        r = valid_rows.iloc[j]
                                        r_space = r.get('space')
                                        if pd.isna(r_space) or str(r_space).strip() == '':
                                            r_field = r.get('field')
                                            if pd.notna(r_field):
                                                r_field_str = str(r_field).strip().lower()
                                                if r_field_str == 'value' or r_field_str == 'tlv_value':
                                                    # 检查 reg, mask, bits_value 列
                                                    r_reg = r.get('reg')
                                                    r_mask = r.get('mask')
                                                    r_bits_value = None
                                                    if hasattr(self, 'bits_value_col') and self.bits_value_col:
                                                        r_bits_value = r.get(self.bits_value_col)
                                                    if r_bits_value is None and 'bits_value' in valid_rows.columns:
                                                        r_bits_value = r.get('bits_value')
                                                    if r_bits_value is None and 'value.1' in valid_rows.columns:
                                                        r_bits_value = r.get('value.1')
                                                    
                                                    has_reg = pd.notna(r_reg) and str(r_reg).strip() != '' and str(r_reg).strip() != '-'
                                                    has_mask = pd.notna(r_mask) and str(r_mask).strip() != '' and str(r_mask).strip() != '-'
                                                    has_bits_value = pd.notna(r_bits_value) and str(r_bits_value).strip() != '' and str(r_bits_value).strip() != '-'
                                                    
                                                    if has_reg and has_mask and has_bits_value:
                                                        has_reg_mask_value = True
                                                    break
                                                elif r_field_str == 'type' or r_field_str == 'tlv_type':
                                                    # 遇到下一个 tlv_type，停止查找
                                                    break
                                        elif pd.notna(r_space) and str(r_space).strip() != 'tlv_base':
                                            # 遇到新的 space，停止查找
                                            break
                                    
                                    # 如果有 reg_mask_value，将 bit15 置1
                                    tlv_type_value = tlv_type_counter
                                    if has_reg_mask_value:
                                        tlv_type_value = tlv_type_counter | 0x8000
                                    
                                    f.write(f"#define {macro_name_tlv} 0x{tlv_type_value:04X}\n")
                                    tlv_type_counter += 1
                    
                    # 更新prev_offset和prev_size
                    prev_offset = offset
                    prev_size = size
                
                # 关闭最后一个结构体（如果不是tlv_base）
                if current_space and current_space != 'tlv_base' and current_struct_name:
                    f.write(f"}} {current_struct_name}_t;\n\n")
                
                # 第二遍遍历：生成各 space 基址 getter 声明（实现在 sys_persist_config.c）
                current_space_func = None
                for idx, row in valid_rows.iterrows():
                    space = row.get('space')
                    if pd.notna(space) and str(space).strip() != '':
                        space_str = str(space).strip()
                        if space_str != 'tlv_base' and space_str != current_space_func:
                            current_space_func = space_str
                            struct_name_base = space_str.replace('_base', '')
                            struct_name = f"sys_persist_{struct_name_base}"
                            func_name = f"get_{struct_name}"
                            f.write(f"{struct_name}_t *{func_name}(void);\n")
                f.write("\n")

                # 写入文件尾
                f.write("/* API声明 */\n")
                f.write("sys_persist_tlv_t *sys_persist_get_tlv(uint16_t type);\n")
                f.write("reg_mask_value_tlv_t *sys_persist_get_reg_mask_value_tlv(uint16_t type);\n")
                f.write("int sys_persist_check_crc(void);\n")
                
                # 生成dump函数声明
                structs_generated = []
                has_tlv_base = False
                prev_space = None
                for idx, row in valid_rows.iterrows():
                    space = row.get('space')
                    if pd.notna(space) and str(space).strip() != '':
                        space_str = str(space).strip()
                        if space_str == 'tlv_base':
                            has_tlv_base = True
                        elif space_str != prev_space:
                            struct_name_base = space_str.replace('_base', '')
                            struct_name = f"sys_persist_{struct_name_base}"
                            func_name = f"sys_persist_dump_{struct_name_base}"
                            if struct_name not in structs_generated:
                                f.write(f"void {func_name}(void);\n")
                                structs_generated.append(struct_name)
                        prev_space = space_str
                
                # 添加TLV dump函数声明
                if has_tlv_base:
                    f.write("void sys_persist_dump_tlv(void);\n")
                
                f.write("\n#endif /* !__ASSEMBLER__ */\n")
                f.write("\n#endif /* SYS_PERSIST_CONFIG_H */\n")
            
            return True
        except Exception as e:
            print(f"Error: failed to generate header file {h_file}: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def generate_source(self, c_file: str) -> bool:
        """
        生成源文件
        
        Args:
            c_file: 源文件路径
            
        Returns:
            bool: 成功返回True，失败返回False
        """
        try:
            with open(c_file, 'w', encoding='utf-8') as f:
                # 写入文件头
                f.write("""#include "sys_persist_config.h"
#include "components/log.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#ifndef SYS_PERSIST_CONFIG_LOG
#define SYS_PERSIST_CONFIG_LOG(...) BK_LOGI("sys_persist", __VA_ARGS__)
#endif

/* External declaration for crc32 function */
extern uint32_t crc32(const uint8_t *frame, size_t frame_len);

""")
                # Base-address getters (implementation; declarations in sys_persist_config.h)
                valid_rows_getters = self.df[
                    (self.df['offset'].notna()) |
                    (self.df['size'].notna()) |
                    (self.df['field'].notna())
                ].copy()
                f.write("/* Base-address getters (generated from CSV spaces) */\n")
                current_space_func = None
                for idx, row in valid_rows_getters.iterrows():
                    space = row.get('space')
                    if pd.notna(space) and str(space).strip() != '':
                        space_str = str(space).strip()
                        if space_str != 'tlv_base' and space_str != current_space_func:
                            current_space_func = space_str
                            struct_name_base = space_str.replace('_base', '')
                            struct_name = f"sys_persist_{struct_name_base}"
                            macro_name = f"SYS_PERSIST_CONFIG_{space_str.upper().replace('-', '_')}"
                            func_name = f"get_{struct_name}"
                            f.write(f"{struct_name}_t *{func_name}(void)\n")
                            f.write("{\n")
                            f.write(f"    return ({struct_name}_t *){macro_name};\n")
                            f.write("}\n\n")

                # 5.1 解析TLV的API
                f.write("""/* 解析TLV的API */
sys_persist_tlv_t *sys_persist_get_tlv(uint16_t type)
{
    sys_persist_tlv_t *tlv = (sys_persist_tlv_t *)SYS_PERSIST_CONFIG_TLV_BASE;
    uint16_t num = *(uint16_t *)((uint8_t *)tlv + 8); /* 假设num在offset 8 */
    
    uint8_t *p = (uint8_t *)tlv + 10; /* 跳过name和num */
    for (uint16_t i = 0; i < num; i++) {
        sys_persist_tlv_t *entry = (sys_persist_tlv_t *)p;
        if (entry->type == type) {
            return entry;
        }
        p += sizeof(uint16_t) * 2 + entry->length; /* type + length + data */
    }
    return NULL;
}

reg_mask_value_tlv_t *sys_persist_get_reg_mask_value_tlv(uint16_t type)
{
    sys_persist_tlv_t *tlv = (sys_persist_tlv_t *)SYS_PERSIST_CONFIG_TLV_BASE;
    uint16_t num = *(uint16_t *)((uint8_t *)tlv + 8); /* 假设num在offset 8 */
    
    uint8_t *p = (uint8_t *)tlv + 10; /* 跳过name和num */
    for (uint16_t i = 0; i < num; i++) {
        reg_mask_value_tlv_t *rmv_tlv = (reg_mask_value_tlv_t *)p;
        if (rmv_tlv->type == type) {
            return rmv_tlv;
        }
        p += sizeof(uint16_t) * 2 + rmv_tlv->length; /* type + length + data */
    }
    return NULL;
}

""")
                
                # 5.2 校验CRC的API
                # 查找 magic 和 crc 字段的位置和大小
                valid_rows = self.df[
                    (self.df['offset'].notna()) | 
                    (self.df['size'].notna()) | 
                    (self.df['field'].notna())
                ].copy()
                
                magic_offset = None
                magic_size = 0
                crc_offset = None
                crc_size = 0
                max_offset = 0
                prev_offset = None
                prev_size = None
                
                for idx, row in valid_rows.iterrows():
                    offset_val = row.get('offset')
                    size_val = row.get('size')
                    field = row.get('field')
                    
                    if pd.isna(offset_val):
                        if prev_offset is not None and prev_size is not None:
                            offset = prev_offset + prev_size
                        else:
                            continue
                    else:
                        offset = int(offset_val)
                    
                    if pd.isna(size_val):
                        continue
                    else:
                        size = int(size_val)
                    
                    max_offset = max(max_offset, offset + size)
                    
                    if pd.notna(field):
                        field_str = str(field).strip().lower()
                        if field_str == 'magic':
                            magic_offset = offset
                            magic_size = size
                        elif field_str == 'crc' or field_str == 'crc32':
                            crc_offset = offset
                            crc_size = size
                    
                    prev_offset = offset
                    prev_size = size
                
                # 计算16字节对齐后的长度
                aligned_size = ((max_offset + 15) // 16) * 16 if max_offset > 0 else 0
                
                # 生成 sys_persist_check_crc 函数
                if crc_offset is None:
                    # 如果没有 CRC 字段，生成一个简单的函数
                    f.write("""/* 校验CRC的API */
int sys_persist_check_crc(void)
{
    /* No CRC field found in configuration, return success */
    return 0;
}

""")
                else:
                    f.write("""/* 校验CRC的API */
int sys_persist_check_crc(void)
{
    uint8_t *config_base = (uint8_t *)SYS_PERSIST_CONFIG_FLASH_OFFSET;
    uint32_t stored_crc;
    uint32_t calculated_crc;
    uint8_t crc_data[CONFIG_SYS_PERSIST_ALIGNED_SIZE];
    uint32_t crc_data_len = 0;
    uint32_t i;
    
    /* 读取存储的CRC值 */
    stored_crc = *(uint32_t *)(config_base + """)
                    f.write(f"{crc_offset});\n")
                    f.write("""
    /* 构建CRC数据：排除magic和crc字段的所有数据 */
""")
                
                    if magic_offset is not None:
                        f.write(f"    /* Skip magic field: offset {magic_offset}, size {magic_size} */\n")
                    f.write(f"    /* Skip crc field: offset {crc_offset}, size {crc_size} */\n")
                    
                    f.write("    for (i = 0; i < CONFIG_SYS_PERSIST_ALIGNED_SIZE; i++) {\n")
                    
                    if magic_offset is not None:
                        f.write(f"        /* Skip magic field range */\n")
                        f.write(f"        if (i >= {magic_offset} && i < {magic_offset} + {magic_size}) {{\n")
                        f.write(f"            continue;\n")
                        f.write(f"        }}\n")
                    
                    f.write(f"        /* Skip crc field range */\n")
                    f.write(f"        if (i >= {crc_offset} && i < {crc_offset} + {crc_size}) {{\n")
                    f.write(f"            continue;\n")
                    f.write(f"        }}\n")
                    
                    f.write("        crc_data[crc_data_len++] = config_base[i];\n")
                    f.write("    }\n")
                    f.write("""
    /* 计算CRC32 */
    /* Note: crc32 function from wpa_supplicant uses u32/u8 types, which are typically uint32_t/uint8_t */
    calculated_crc = (uint32_t)crc32((const uint8_t *)crc_data, (size_t)crc_data_len);
    
    /* 比较计算出的CRC和存储的CRC */
    if (calculated_crc == stored_crc) {
        return 0; /* CRC校验成功 */
    } else {
        return -1; /* CRC校验失败 */
    }
}

""")
                
                # 5.3 生成dump API
                f.write("#ifdef CONFIG_PERSIST_CONFIG_CLI\n\n")
                valid_rows = self.df[
                    (self.df['offset'].notna()) | 
                    (self.df['size'].notna()) | 
                    (self.df['field'].notna())
                ].copy()
                
                structs_generated = []
                prev_space = None
                
                for idx, row in valid_rows.iterrows():
                    space = row.get('space')
                    if pd.notna(space) and str(space).strip() != '':
                        space_str = str(space).strip()
                        if space_str != 'tlv_base' and space_str != prev_space:
                            struct_name_base = space_str.replace('_base', '')
                            struct_name = f"sys_persist_{struct_name_base}"
                            func_name = f"sys_persist_dump_{struct_name_base}"
                            
                            if struct_name not in structs_generated:
                                macro_name = f"SYS_PERSIST_CONFIG_{space_str.upper().replace('-', '_')}"
                                f.write(f"/* Dump {struct_name} */\n")
                                f.write(f"void {func_name}(void)\n")
                                f.write("{\n")
                                f.write(f"    {struct_name}_t *cfg = ({struct_name}_t *){macro_name};\n")
                                f.write(f"    if (!cfg) return;\n")
                                f.write(f"    SYS_PERSIST_CONFIG_LOG(\"=== {struct_name} ===\\n\");\n")
                                
                                # 查找该结构体的所有成员
                                # 首先找到name字段的实际成员名
                                name_member = None
                                for j in range(idx, len(valid_rows)):
                                    r = valid_rows.iloc[j]
                                    r_space = r.get('space')
                                    if pd.isna(r_space) or str(r_space).strip() == '':
                                        r_field = r.get('field')
                                        if pd.notna(r_field) and str(r_field).strip().lower() == 'name':
                                            name_member = str(r.get('value')).strip()
                                            break
                                    elif pd.notna(r_space) and str(r_space).strip() == space_str:
                                        # 当前行就是space不为空的行，也要检查field
                                        r_field = r.get('field')
                                        if pd.notna(r_field) and str(r_field).strip().lower() == 'name':
                                            name_member = str(r.get('value')).strip()
                                            break
                                    elif pd.notna(r_space) and str(r_space).strip() != space_str:
                                        break
                                
                                member_printed = set()
                                # 首先处理第一行（space不为空的行）的字段
                                r = valid_rows.iloc[idx]
                                r_field = r.get('field')
                                if pd.notna(r_field):
                                    r_field_str = str(r_field).strip().lower()
                                    if r_field_str not in member_printed:
                                        member_printed.add(r_field_str)
                                        if r_field_str == 'name' and name_member:
                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  name: %s\\n\", cfg->{name_member});\n")
                                        elif r_field_str in ['crc', 'crc32', 'version', 'date', 'magic']:
                                            r_size = r.get('size')
                                            if pd.notna(r_size):
                                                r_size_int = int(r_size)
                                                if r_size_int == 4:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                                elif r_size_int == 2:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%04X\\n\", cfg->{r_field_str});\n")
                                                elif r_size_int == 1:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%02X\\n\", cfg->{r_field_str});\n")
                                                else:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: [array, size=%d]\\n\", {r_size_int});\n")
                                            else:
                                                f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                        elif r_field_str == 'reserved':
                                            r_size = r.get('size')
                                            if pd.notna(r_size):
                                                f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: [array, size=%d]\\n\", {int(r_size)});\n")
                                            else:
                                                f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: [array]\\n\");\n")
                                        else:
                                            # 其他字段，根据size确定打印格式
                                            r_size = r.get('size')
                                            if pd.notna(r_size):
                                                r_size_int = int(r_size)
                                                if r_size_int == 4:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                                elif r_size_int == 2:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%04X\\n\", cfg->{r_field_str});\n")
                                                elif r_size_int == 1:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%02X\\n\", cfg->{r_field_str});\n")
                                                else:
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: [array, size=%d]\\n\", {r_size_int});\n")
                                            else:
                                                f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                
                                # 然后处理后续行（space为空的行）
                                for j in range(idx + 1, len(valid_rows)):
                                    r = valid_rows.iloc[j]
                                    r_space = r.get('space')
                                    if pd.isna(r_space) or str(r_space).strip() == '':
                                        r_field = r.get('field')
                                        if pd.notna(r_field):
                                            r_field_str = str(r_field).strip().lower()
                                            if r_field_str not in member_printed:
                                                member_printed.add(r_field_str)
                                                if r_field_str == 'name' and name_member:
                                                    # name字段是字符串，使用实际的成员名
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  name: %s\\n\", cfg->{name_member});\n")
                                                elif r_field_str in ['crc', 'crc32', 'version', 'date', 'magic']:
                                                    # uint32_t类型（magic可能是其他类型，根据size判断）
                                                    r_size = r.get('size')
                                                    if pd.notna(r_size):
                                                        r_size_int = int(r_size)
                                                        if r_size_int == 4:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                                        elif r_size_int == 2:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%04X\\n\", cfg->{r_field_str});\n")
                                                        elif r_size_int == 1:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%02X\\n\", cfg->{r_field_str});\n")
                                                        else:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: [array, size=%d]\\n\", {r_size_int});\n")
                                                    else:
                                                        f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                                elif r_field_str == 'reserved':
                                                    # 数组类型，简化打印
                                                    r_size = r.get('size')
                                                    if pd.notna(r_size):
                                                        f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: [array, size=%d]\\n\", {int(r_size)});\n")
                                                    else:
                                                        f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: [array]\\n\");\n")
                                                elif r_field_str.startswith('r') and len(r_field_str) <= 4:
                                                    # r0, r1等寄存器字段
                                                    r_size = r.get('size')
                                                    if pd.notna(r_size):
                                                        r_size_int = int(r_size)
                                                        if r_size_int == 4:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                                        elif r_size_int == 2:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%04X\\n\", cfg->{r_field_str});\n")
                                                        elif r_size_int == 1:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%02X\\n\", cfg->{r_field_str});\n")
                                                    else:
                                                        f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                                elif r_field_str in ['vdddig', 'vddana']:
                                                    # uint8_t类型
                                                    f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%02X\\n\", cfg->{r_field_str});\n")
                                                else:
                                                    # 其他字段
                                                    r_size = r.get('size')
                                                    if pd.notna(r_size):
                                                        r_size_int = int(r_size)
                                                        if r_size_int == 4:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%08X\\n\", cfg->{r_field_str});\n")
                                                        elif r_size_int == 2:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%04X\\n\", cfg->{r_field_str});\n")
                                                        elif r_size_int == 1:
                                                            f.write(f"    SYS_PERSIST_CONFIG_LOG(\"  {r_field_str}: 0x%02X\\n\", cfg->{r_field_str});\n")
                                    elif pd.notna(r_space) and str(r_space).strip() != space_str:
                                        break
                                
                                f.write("}\n\n")
                                structs_generated.append(struct_name)
                        
                        prev_space = space_str
                
                # 5.4 生成TLV dump函数
                # 检查是否存在tlv_base
                has_tlv_base = False
                tlv_type_defs = []
                name_offset = None
                name_size = None
                tlv_base_offset = None  # tlv_base 的绝对偏移量
                prev_offset = None
                prev_size = None
                
                current_tlv_base = False
                for idx, row in valid_rows.iterrows():
                    space = row.get('space')
                    offset_val = row.get('offset')
                    size_val = row.get('size')
                    field = row.get('field')
                    
                    # 检查是否进入或离开 tlv_base
                    if pd.notna(space) and str(space).strip() == 'tlv_base':
                        has_tlv_base = True
                        current_tlv_base = True
                        # 计算 tlv_base 的起始偏移量
                        if pd.notna(offset_val):
                            tlv_base_offset = int(offset_val)
                        else:
                            # 如果 offset 为空，需要从上一行推断
                            if prev_offset is not None and prev_size is not None:
                                tlv_base_offset = prev_offset + prev_size
                            else:
                                tlv_base_offset = 0
                        prev_offset = None
                        prev_size = None
                    elif pd.notna(space) and str(space).strip() != '':
                        current_tlv_base = False
                        continue
                    
                    # 只在 tlv_base 范围内处理
                    if current_tlv_base:
                        # 计算offset
                        if pd.isna(offset_val):
                            if prev_offset is not None and prev_size is not None:
                                offset = prev_offset + prev_size
                            else:
                                offset = tlv_base_offset if tlv_base_offset is not None else 0
                        else:
                            offset = int(offset_val)
                        
                        if pd.notna(size_val):
                            size = int(size_val)
                        else:
                            size = 0
                        
                        if pd.notna(field):
                            field_str = str(field).strip().lower()
                            if field_str == 'name':
                                # 计算相对于 tlv_base 的偏移量
                                if tlv_base_offset is not None:
                                    name_offset = offset - tlv_base_offset
                                else:
                                    name_offset = offset
                                name_size = size
                            elif field_str == 'type' or field_str == 'tlv_type':
                                value = row.get('value')
                                if pd.notna(value):
                                    tlv_type_defs.append(str(value).strip().upper().replace('-', '_'))
                        
                        prev_offset = offset
                        prev_size = size
                
                if has_tlv_base:
                    # 生成dump函数
                    f.write("/* Dump TLV */\n")
                    f.write("void sys_persist_dump_tlv(void)\n")
                    f.write("{\n")
                    f.write("    sys_persist_tlv_t *tlv = (sys_persist_tlv_t *)SYS_PERSIST_CONFIG_TLV_BASE;\n")
                    f.write("    if (!tlv) return;\n")
                    f.write("    \n")
                    f.write("    SYS_PERSIST_CONFIG_LOG(\"=== sys_persist_tlv ===\\n\");\n")
                    f.write("    \n")
                    
                    # 读取name字段（相对于 tlv_base 的偏移量，通常是 0）
                    if name_offset is not None and name_size is not None:
                        f.write(f"    /* Read name from offset {name_offset} (relative to tlv_base) */\n")
                        f.write(f"    char name[{name_size}] = {{0}};\n")
                        f.write(f"    memcpy(name, (uint8_t *)tlv + {name_offset}, {name_size});\n")
                        f.write("    SYS_PERSIST_CONFIG_LOG(\"  name: %s\\n\", name);\n")
                        # 计算TLV条目的起始位置（name之后，相对于 tlv_base）
                        tlv_start_offset = name_offset + name_size
                        f.write(f"    \n")
                        f.write(f"    /* Traverse all TLV entries starting from offset {tlv_start_offset} (relative to tlv_base) */\n")
                        f.write(f"    uint8_t *p = (uint8_t *)tlv + {tlv_start_offset};\n")
                    else:
                        f.write("    /* Warning: name field not found in CSV, using default offset */\n")
                        f.write("    uint8_t *p = (uint8_t *)tlv + 8; /* Skip name (8 bytes) */\n")
                    
                    f.write("    \n")
                    f.write("    /* Traverse all TLV entries until end of config */\n")
                    f.write("    uint16_t i = 0;\n")
                    # end 应该指向配置的结束位置，而不是相对于 tlv 的偏移
                    if tlv_base_offset is not None:
                        f.write(f"    /* tlv_base is at offset {tlv_base_offset}, calculate remaining space */\n")
                        f.write(f"    uint8_t *config_base = (uint8_t *)SYS_PERSIST_CONFIG_FLASH_OFFSET;\n")
                        f.write(f"    uint8_t *end = config_base + CONFIG_SYS_PERSIST_SIZE;\n")
                    else:
                        f.write("    uint8_t *config_base = (uint8_t *)SYS_PERSIST_CONFIG_FLASH_OFFSET;\n")
                        f.write("    uint8_t *end = config_base + CONFIG_SYS_PERSIST_SIZE;\n")
                    f.write("    while (p < end) {\n")
                    f.write("        /* Check if there's enough space for a TLV entry (at least type + length) */\n")
                    f.write("        if (p + sizeof(uint16_t) * 2 > end) break;\n")
                    f.write("        \n")
                    f.write("        sys_persist_tlv_t *entry = (sys_persist_tlv_t *)p;\n")
                    f.write("        \n")
                    f.write("        /* Check if entry is valid (type and length not both zero) */\n")
                    f.write("        if (entry->type == 0 && entry->length == 0) break;\n")
                    f.write("        \n")
                    f.write("        /* Check if entry length is valid */\n")
                    f.write("        if (p + sizeof(uint16_t) * 2 + entry->length > end) break;\n")
                    f.write("        \n")
                    f.write("        uint16_t type_with_flag = entry->type;\n")
                    f.write("        uint16_t type_base = type_with_flag & 0x7FFF; /* Clear bit15 */\n")
                    f.write("        SYS_PERSIST_CONFIG_LOG(\"  TLV[%u]: type=0x%04X (base=0x%04X), length=%u\\n\", i, type_with_flag, type_base, entry->length);\n")
                    f.write("        \n")
                    f.write("        /* Check bit15 to determine if it's a reg_mask_value_tlv */\n")
                    f.write("        if (type_with_flag & 0x8000) {\n")
                    f.write("            /* reg_mask_value_tlv type */\n")
                    f.write("            reg_mask_value_tlv_t *rmv_tlv = (reg_mask_value_tlv_t *)entry;\n")
                    f.write("            if (entry->length >= sizeof(reg_mask_value_t)) {\n")
                    f.write("                uint16_t count = entry->length / sizeof(reg_mask_value_t);\n")
                    f.write("                SYS_PERSIST_CONFIG_LOG(\"    reg_mask_value count: %u\\n\", count);\n")
                    f.write("                for (uint16_t j = 0; j < count; j++) {\n")
                    f.write("                    SYS_PERSIST_CONFIG_LOG(\"      [%u] reg=0x%08X, mask=0x%08X, value=0x%08X\\n\",\n")
                    f.write("                        j, rmv_tlv->rmv[j].reg, rmv_tlv->rmv[j].mask, rmv_tlv->rmv[j].value);\n")
                    f.write("                }\n")
                    f.write("            } else {\n")
                    f.write("                SYS_PERSIST_CONFIG_LOG(\"    Warning: length %u < sizeof(reg_mask_value_t)\\n\", entry->length);\n")
                    f.write("            }\n")
                    f.write("        } else {\n")
                    f.write("            /* Normal TLV type, print raw data */\n")
                    f.write("            SYS_PERSIST_CONFIG_LOG(\"    data: \");\n")
                    f.write("            for (uint16_t j = 0; j < entry->length && j < 16; j++) {\n")
                    f.write("                SYS_PERSIST_CONFIG_LOG(\"%02X \", entry->data[j]);\n")
                    f.write("            }\n")
                    f.write("            if (entry->length > 16) {\n")
                    f.write("                SYS_PERSIST_CONFIG_LOG(\"...\");\n")
                    f.write("            }\n")
                    f.write("            SYS_PERSIST_CONFIG_LOG(\"\\n\");\n")
                    f.write("        }\n")
                    f.write("        \n")
                    f.write("        p += sizeof(uint16_t) * 2 + entry->length; /* type + length + data */\n")
                    f.write("        i++;\n")
                    f.write("    }\n")
                    f.write("}\n\n")
                    f.write("#endif /* CONFIG_PERSIST_CONFIG_CLI */\n\n")
                
            return True
        except Exception as e:
            print(f"Error: failed to generate source file {c_file}: {e}")
            import traceback
            traceback.print_exc()
            return False

    def generate_cli(self, cli_file: str, cli_h_file: str) -> bool:
        """
        生成 CLI 源文件，用于注册 sys_persist 相关命令
        
        Args:
            cli_file: CLI 源文件路径
            cli_h_file: CLI 头文件路径
        """
        try:
            # 先生成头文件，提供初始化函数声明
            with open(cli_h_file, 'w', encoding='utf-8') as hf:
                hf.write("""#ifndef SYS_PERSIST_CONFIG_CLI_H
#define SYS_PERSIST_CONFIG_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

int sys_persist_cli_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SYS_PERSIST_CONFIG_CLI_H */
""")

            with open(cli_file, 'w', encoding='utf-8') as f:
                f.write("""#include "sys_persist_config.h"
#include "sys_persist_config_cli.h"
#include "cli.h"
#include <string.h>

""")

                # 收集需要 dump 的结构体名称（去重、排除 tlv_base）
                valid_rows = self.df[
                    (self.df['offset'].notna()) | 
                    (self.df['size'].notna()) | 
                    (self.df['field'].notna())
                ].copy()

                struct_bases = []
                has_tlv_base = False
                prev_space = None
                for idx, row in valid_rows.iterrows():
                    space = row.get('space')
                    if pd.notna(space) and str(space).strip() != '':
                        space_str = str(space).strip()
                        if space_str == 'tlv_base':
                            has_tlv_base = True
                        else:
                            base = space_str.replace('_base', '')
                            if base not in struct_bases:
                                struct_bases.append(base)
                        prev_space = space_str

                # dump_all
                f.write("static void sys_persist_dump_all(void)\n{\n")
                if struct_bases:
                    for base in struct_bases:
                        f.write(f"    sys_persist_dump_{base}();\n")
                if has_tlv_base:
                    f.write("    sys_persist_dump_tlv();\n")
                f.write("}\n\n")

                # dump 命令
                f.write("static void sys_persist_cli_dump(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)\n")
                f.write("{\n")
                f.write("    (void)pcWriteBuffer; (void)xWriteBufferLen;\n")
                f.write("    if (argc < 2) {\n")
                f.write("        BK_DUMP_OUT(\"Usage: sys_persist_dump <all")
                for base in struct_bases:
                    f.write(f"|{base}")
                if has_tlv_base:
                    f.write("|tlv")
                f.write(">\\r\\n\");\n")
                f.write("        return;\n")
                f.write("    }\n")
                f.write("    const char *target = argv[1];\n")
                f.write("    if (strcmp(target, \"all\") == 0) {\n")
                f.write("        sys_persist_dump_all();\n")
                f.write("        return;\n")
                f.write("    }\n")
                for base in struct_bases:
                    f.write(f"    else if (strcmp(target, \"{base}\") == 0) {{ sys_persist_dump_{base}(); return; }}\n")
                if has_tlv_base:
                    f.write("    else if (strcmp(target, \"tlv\") == 0) { sys_persist_dump_tlv(); return; }\n")
                f.write("    BK_DUMP_OUT(\"Unknown section: %s\\r\\n\", target);\n")
                f.write("}\n\n")

                # crc 命令
                f.write("static void sys_persist_cli_crc(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)\n")
                f.write("{\n")
                f.write("    (void)pcWriteBuffer; (void)xWriteBufferLen; (void)argc; (void)argv;\n")
                f.write("    int ret = sys_persist_check_crc();\n")
                f.write("    BK_DUMP_OUT(\"sys_persist_check_crc ret: %d\\r\\n\", ret);\n")
                f.write("}\n\n")

                # 命令表与注册
                f.write("#define SYS_PERSIST_CLI_CMD_CNT (sizeof(s_sys_persist_cli_commands) / sizeof(struct cli_command))\n")
                f.write("static const struct cli_command s_sys_persist_cli_commands[] = {\n")
                f.write("    {\"sys_persist_dump\", \"sys_persist_dump <all|section>\", sys_persist_cli_dump},\n")
                f.write("    {\"sys_persist_crc\", \"sys_persist_crc\", sys_persist_cli_crc},\n")
                f.write("};\n\n")

                f.write("int sys_persist_cli_init(void)\n")
                f.write("{\n")
                f.write("    return cli_register_commands(s_sys_persist_cli_commands, SYS_PERSIST_CLI_CMD_CNT);\n")
                f.write("}\n")

            return True
        except Exception as e:
            print(f"Error: failed to generate CLI source file {cli_file}: {e}")
            import traceback
            traceback.print_exc()
            return False


def gen_code(csv_file: str, output_dir: Optional[str] = None) -> bool:
    """
    生成C代码
    
    Args:
        csv_file: CSV文件路径
        output_dir: 输出目录，如果为None则使用CSV文件所在目录
        
    Returns:
        bool: 成功返回True，失败返回False
    """
    if not os.path.exists(csv_file):
        print(f"Error: CSV file does not exist: {csv_file}")
        return False
    
    generator = CodeGenerator(csv_file)
    
    # 加载CSV
    if not generator.load_csv():
        return False
    
    # 检查必需的列
    if not generator.check_required_columns():
        return False
    
    # 生成代码
    if not generator.generate_code(output_dir):
        return False
    
    return True


class BinToCsvConverter:
    """二进制到CSV转换器类"""
    
    def __init__(self, csv_file: str, bin_file: str):
        """
        初始化转换器
        
        Args:
            csv_file: CSV文件路径
            bin_file: 二进制文件路径
        """
        self.csv_file = csv_file
        self.bin_file = bin_file
        self.df = None
        self.bin_data = None
        self.bits_value_col = None
    
    def load_csv(self) -> bool:
        """加载CSV文件"""
        try:
            self.df = pd.read_csv(self.csv_file)
            return True
        except Exception as e:
            print(f"Error: failed to read CSV file {self.csv_file}: {e}")
            return False
    
    def load_bin(self) -> bool:
        """加载二进制文件"""
        try:
            with open(self.bin_file, 'rb') as f:
                self.bin_data = f.read()
            return True
        except Exception as e:
            print(f"Error: failed to read binary file {self.bin_file}: {e}")
            return False
    
    def check_required_columns(self) -> bool:
        """检查必需的列"""
        required_columns = ['space', 'offset', 'size', 'field', 'value']
        
        # 检查bits_value列
        if 'bits_value' in self.df.columns:
            self.bits_value_col = 'bits_value'
        elif 'value.1' in self.df.columns:
            self.bits_value_col = 'value.1'
        
        for col in required_columns:
            if col not in self.df.columns:
                print(f"Error: required column '{col}' not found")
                return False
        
        return True
    
    def convert_bin_to_csv(self, output_file: Optional[str] = None) -> bool:
        """
        将二进制文件转换为CSV
        
        Args:
            output_file: 输出CSV文件路径，如果为None则覆盖原文件
            
        Returns:
            bool: 成功返回True，失败返回False
        """
        if output_file is None:
            output_file = self.csv_file
        
        valid_rows = self.df[
            (self.df['offset'].notna()) | 
            (self.df['size'].notna()) | 
            (self.df['field'].notna())
        ].copy()
        
        if len(valid_rows) == 0:
            print("Error: no valid data rows in the CSV file")
            return False
        
        prev_offset = None
        prev_size = None
        
        # 遍历每一行，解析bin数据并更新CSV
        for idx, row in valid_rows.iterrows():
            space = row.get('space')
            offset_val = row.get('offset')
            size_val = row.get('size')
            field = row.get('field')
            
            # 跳过完全空的行
            if pd.isna(offset_val) and pd.isna(size_val) and pd.isna(field):
                continue
            
            # 计算offset
            if pd.isna(offset_val):
                if prev_offset is not None and prev_size is not None:
                    offset = prev_offset + prev_size
                else:
                    continue
            else:
                offset = int(offset_val)
            
            if pd.isna(size_val):
                continue
            else:
                size = int(size_val)
            
            # 检查offset和size是否超出bin文件范围
            if offset + size > len(self.bin_data):
                print(f"Warning: line {idx+2} offset={offset}, size={size} exceeds bin file size")
                continue
            
            if pd.notna(field):
                field_str = str(field).strip().lower()
                
                # 根据field类型解析数据
                if field_str == 'crc' or field_str == 'crc32':
                    # CRC字段，读取uint32_t
                    if size == 4 and offset + 4 <= len(self.bin_data):
                        crc_value = struct.unpack('<I', self.bin_data[offset:offset+4])[0]
                        self.df.at[idx, 'value'] = f"0x{crc_value:08X}"
                
                elif field_str == 'version':
                    # 版本字段，转换为点分十进制
                    if size == 4 and offset + 4 <= len(self.bin_data):
                        version_value = struct.unpack('<I', self.bin_data[offset:offset+4])[0]
                        v1 = (version_value >> 24) & 0xFF
                        v2 = (version_value >> 16) & 0xFF
                        v3 = (version_value >> 8) & 0xFF
                        v4 = version_value & 0xFF
                        self.df.at[idx, 'value'] = f"{v1}.{v2}.{v3}.{v4}"
                
                elif field_str == 'name':
                    # 名称字段，读取字符串
                    name_bytes = self.bin_data[offset:offset+size]
                    # 找到字符串结束位置（0x00）
                    null_pos = name_bytes.find(0)
                    if null_pos >= 0:
                        name_bytes = name_bytes[:null_pos]
                    try:
                        name_str = name_bytes.decode('utf-8', errors='ignore').rstrip('\x00')
                        self.df.at[idx, 'value'] = name_str
                    except:
                        self.df.at[idx, 'value'] = ''
                
                elif field_str == 'reserved':
                    # Reserved字段，读取为十六进制值（取第一个字节的值）
                    if size > 0 and offset < len(self.bin_data):
                        fill_value = self.bin_data[offset]
                        self.df.at[idx, 'value'] = f"0x{fill_value:02X}"
                
                elif field_str == 'value' or field_str == 'tlv_value':
                    # TLV value字段，可能有两种情况：
                    # 1. 直接是value（size通常为4）
                    # 2. 是reg/mask/bits_value（size为12）
                    if size == 12 and offset + 12 <= len(self.bin_data):
                        # 可能是reg/mask/bits_value格式
                        reg_value = struct.unpack('<I', self.bin_data[offset:offset+4])[0]
                        mask_value = struct.unpack('<I', self.bin_data[offset+4:offset+8])[0]
                        bits_value = struct.unpack('<I', self.bin_data[offset+8:offset+12])[0]
                        
                        # 检查是否有reg/mask/bits_value列
                        if 'reg' in self.df.columns and 'mask' in self.df.columns:
                            self.df.at[idx, 'reg'] = f"0x{reg_value:08X}"
                            self.df.at[idx, 'mask'] = f"0x{mask_value:08X}"
                            if self.bits_value_col:
                                self.df.at[idx, self.bits_value_col] = f"0x{bits_value:08X}"
                    elif size == 4 and offset + 4 <= len(self.bin_data):
                        # 普通value
                        value = struct.unpack('<I', self.bin_data[offset:offset+4])[0]
                        self.df.at[idx, 'value'] = f"0x{value:08X}"
                    elif size == 2 and offset + 2 <= len(self.bin_data):
                        value = struct.unpack('<H', self.bin_data[offset:offset+2])[0]
                        self.df.at[idx, 'value'] = f"0x{value:04X}"
                    elif size == 1 and offset < len(self.bin_data):
                        value = self.bin_data[offset]
                        self.df.at[idx, 'value'] = f"0x{value:02X}"
                
                else:
                    # 其他字段，根据size读取
                    if size == 4 and offset + 4 <= len(self.bin_data):
                        value = struct.unpack('<I', self.bin_data[offset:offset+4])[0]
                        self.df.at[idx, 'value'] = f"0x{value:08X}"
                    elif size == 2 and offset + 2 <= len(self.bin_data):
                        value = struct.unpack('<H', self.bin_data[offset:offset+2])[0]
                        self.df.at[idx, 'value'] = f"0x{value:04X}"
                    elif size == 1 and offset < len(self.bin_data):
                        value = self.bin_data[offset]
                        self.df.at[idx, 'value'] = f"0x{value:02X}"
                    else:
                        # 其他size，读取为字节序列
                        hex_str = ' '.join([f"{b:02X}" for b in self.bin_data[offset:offset+min(size, len(self.bin_data)-offset)]])
                        self.df.at[idx, 'value'] = hex_str
            
            prev_offset = offset
            prev_size = size
        
        # 保存到CSV文件
        try:
            # 直接使用pandas保存CSV文件
            self.df.to_csv(output_file, index=False, encoding='utf-8')
            print(f"Updated CSV file: {output_file}")
            return True
        except Exception as e:
            print(f"Error: failed to save CSV file {output_file}: {e}")
            import traceback
            traceback.print_exc()
            return False


def bin2csv(csv_file: str, bin_file: str, output_file: Optional[str] = None) -> bool:
    """
    将二进制文件转换为CSV
    
    Args:
        csv_file: CSV文件路径（用作模板）
        bin_file: 二进制文件路径
        output_file: 输出CSV文件路径，如果为None则覆盖原文件
        
    Returns:
        bool: 成功返回True，失败返回False
    """
    if not os.path.exists(csv_file):
        print(f"Error: CSV file does not exist: {csv_file}")
        return False
    
    if not os.path.exists(bin_file):
        print(f"Error: binary file does not exist: {bin_file}")
        return False
    
    converter = BinToCsvConverter(csv_file, bin_file)
    
    # 加载CSV
    if not converter.load_csv():
        return False
    
    # 加载二进制文件
    if not converter.load_bin():
        return False
    
    # 检查必需的列
    if not converter.check_required_columns():
        return False
    
    # 转换
    if not converter.convert_bin_to_csv(output_file):
        return False
    
    return True


def create_empty_config_files(output_dir: str) -> bool:
    """
    创建空的配置文件
    
    Args:
        output_dir: 输出目录
        
    Returns:
        bool: 成功返回True，失败返回False
    """
    try:
        os.makedirs(output_dir, exist_ok=True)
        
        h_file = os.path.join(output_dir, 'sys_persist_config.h')
        c_file = os.path.join(output_dir, 'sys_persist_config.c')
        
        # 生成空的头文件
        with open(h_file, 'w', encoding='utf-8') as f:
            f.write("""#ifndef SYS_PERSIST_CONFIG_H
#define SYS_PERSIST_CONFIG_H

#define CONFIG_SYS_PERSIST_SIZE 0x0

#endif /* SYS_PERSIST_CONFIG_H */
""")
        
        # 生成空的源文件
        with open(c_file, 'w', encoding='utf-8') as f:
            f.write("""#include "sys_persist_config.h"

/* 空的配置文件 */
""")
        
        print("Created empty config files:")
        print(f"  Header: {h_file}")
        print(f"  Source: {c_file}")
        return True
    except Exception as e:
        print(f"Error: failed to create empty config files: {e}")
        return False


def find_csv_file(soc_name: Optional[str] = None, project_dir: Optional[str] = None, 
                    armino_dir: Optional[str] = None) -> Optional[str]:
    """
    自动查找CSV文件
    
    查找顺序：
    1. project_dir/soc_name/config/sys_persist_config.csv
    2. armino_dir/middleware/soc/soc_name/sys_persist_config.csv
    
    Args:
        soc_name: SOC名称
        project_dir: 项目目录
        armino_dir: Armino目录
        
    Returns:
        str: CSV文件路径，如果找不到则返回None
    """
    csv_filename = 'sys_persist_config.csv'
    
    if not soc_name:
        print("Error: auto-search for CSV requires --soc option")
        return None
    
    # 1. 先查看 project_dir/soc_name/config/sys_persist_config.csv
    if project_dir:
        csv_path = os.path.join(project_dir, soc_name, 'config', csv_filename)
        if os.path.exists(csv_path):
            return csv_path
    
    # 2. 如果不存在，查看 armino_dir/middleware/soc/soc_name/sys_persist_config.csv
    if armino_dir:
        csv_path = os.path.join(armino_dir, 'middleware', 'soc', soc_name, csv_filename)
        if os.path.exists(csv_path):
            return csv_path
    
    return None


def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='配置文件转换工具')
    
    # 添加全局选项
    parser.add_argument('--soc', dest='soc_name', help='SOC名称')
    parser.add_argument('--project_dir', dest='project_dir', help='编译project的目录')
    parser.add_argument('--out_dir', dest='out_dir', help='工具输出结果的目录')
    parser.add_argument('--armino_dir', dest='armino_dir', help='armino目录')
    
    subparsers = parser.add_subparsers(dest='command', help='可用命令')
    
    # csv2bin命令
    csv2bin_parser = subparsers.add_parser('csv2bin', help='将CSV文件转换为二进制文件')
    csv2bin_parser.add_argument('-csv', help='输入的CSV文件路径（可选，如果未指定则自动查找）')
    csv2bin_parser.add_argument('-o', '--output', help='输出的二进制文件路径（可选，默认与CSV同名.bin）')
    
    # gen_code命令
    gen_code_parser = subparsers.add_parser('gen_code', help='将CSV文件转换为C代码')
    gen_code_parser.add_argument('-csv', help='输入的CSV文件路径（可选，如果未指定则自动查找）')
    gen_code_parser.add_argument('-o', '--output', help='输出目录（可选，默认与CSV文件同目录）')
    
    # bin2csv命令
    bin2csv_parser = subparsers.add_parser('bin2csv', help='将二进制文件转换为CSV')
    bin2csv_parser.add_argument('-csv', help='CSV模板文件路径（可选，如果未指定则自动查找）')
    bin2csv_parser.add_argument('-bin', required=True, help='输入的二进制文件路径')
    bin2csv_parser.add_argument('-o', '--output', help='输出的CSV文件路径（可选，默认覆盖原文件）')
    
    args = parser.parse_args()
    
    # 处理输出目录
    output_dir = args.out_dir if args.out_dir else None
    
    # 处理CSV文件路径
    csv_file = None
    if hasattr(args, 'csv') and args.csv:
        # 如果指定了csv参数，直接使用
        csv_file = args.csv
    else:
        # 如果未指定，自动查找
        csv_file = find_csv_file(
            soc_name=args.soc_name,
            project_dir=args.project_dir,
            armino_dir=args.armino_dir
        )
        if not csv_file:
            # 如果找不到CSV文件，对于gen_code命令，创建空文件并退出
            if args.command == 'gen_code':
                if not output_dir:
                    # 如果没有指定输出目录，尝试使用project_dir或armino_dir
                    if args.project_dir and args.soc_name:
                        output_dir = os.path.join(args.project_dir, args.soc_name, 'config')
                    elif args.armino_dir and args.soc_name:
                        output_dir = os.path.join(args.armino_dir, 'middleware', 'soc', args.soc_name)
                    else:
                        output_dir = '.'
                if create_empty_config_files(output_dir):
                    sys.exit(0)
                else:
                    sys.exit(1)
            else:
                #print("Error: cannot find CSV file. Please specify -csv or set --soc, --project_dir, --armino_dir")
                sys.exit(0)
        print(f"Found CSV file automatically: {csv_file}")
    
    if args.command == 'csv2bin':
        # 如果指定了out_dir，将输出文件放在out_dir中
        output_file = args.output
        if output_file is None and output_dir:
            csv_basename = os.path.splitext(os.path.basename(csv_file))[0]
            output_file = os.path.join(output_dir, csv_basename + '.bin')
        success = csv2bin(csv_file, output_file)
        sys.exit(0 if success else 1)
    elif args.command == 'gen_code':
        # 如果指定了out_dir，将输出文件放在out_dir中
        success = gen_code(csv_file, output_dir or args.output)
        sys.exit(0 if success else 1)
    elif args.command == 'bin2csv':
        success = bin2csv(csv_file, args.bin, args.output)
        sys.exit(0 if success else 1)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == '__main__':
    main()

