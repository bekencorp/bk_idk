#!/usr/bin/env python3

import os
import logging
import shutil

from .common import *
from .partition import Partition
from .bin import Bin

class PartitionNvsKey(Partition):
    def __init__(self, idx=None, field_dic=None, mini_offset=None, partitions=None):
        super().__init__(idx, field_dic, mini_offset)
        # nvs_key partition doesn't need a Bin object, it generates nvs_key.bin directly
        self._bin = None

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        self.__generate_nvs_key_bin()
        self.__process_nvs_key_with_alignment(flash_aes_key, flash_crc_en, flash_aes_type)
        self._add_pack_header_to_nvs_key_bin()

    def __generate_nvs_key_bin(self):
        """Generate nvs_key.bin using nvs_partition_gen.py"""
        key_bin_name = f'{self.name()}.bin'
        
        if os.path.exists(key_bin_name):
            logging.debug(f'{self.name()}: using existing {key_bin_name}')
            return
        
        # Get NVS partition generator tool path
        tools_dir = get_tools_dir()
        nvs_tool = f'{tools_dir}/NVS/nvs_partition_gen.py'
        
        if not os.path.exists(nvs_tool):
            logging.error(f'{self.name()}: NVS tool not found at {nvs_tool}')
            exit(1)
        
        # Generate nvs_key.bin using nvs_partition_gen.py
        # Command: python3 nvs_partition_gen.py generate-key --keyfile nvs_key.bin
        cmd = f'python3 {nvs_tool} generate-key --keyfile {key_bin_name}'
        logging.debug(f'{self.name()}: generating {key_bin_name}: {cmd}')
        run_cmd_not_check_ret(cmd)
        
        # Copy key file from keys directory if it exists (nvs_partition_gen.py creates keys in ./keys/ directory)
        if os.path.exists(f'./keys/{key_bin_name}'):
            shutil.copy(f'./keys/{key_bin_name}', f'./')
            logging.debug(f'{self.name()}: copied {key_bin_name} from ./keys/ directory')
        
        # Verify nvs_key.bin was created
        if not os.path.exists(key_bin_name):
            logging.error(f'{self.name()}: failed to generate {key_bin_name}')
            exit(1)
        
        logging.info(f'{self.name()}: generated {key_bin_name}')
        # ========== MODIFICATION END ==========
    
    def __process_nvs_key_with_alignment(self, flash_aes_key, flash_crc_en, flash_aes_type=None):
        """Process nvs_key.bin with address alignment, encryption and CRC, consistent with beken_utils
        
        Reference: xnf_connect_1226/tools/env_tools/beken_utils/scripts/partition.py process_partition_nvs_key() lines 690-737
        
        Logic (matching beken_utils):
        - If flash_aes_type == 'RANDOM': no encryption, no CRC, no alignment (skip all processing)
        - If flash_aes_type == 'FIXED': Flash AES encryption (if has_flash_encrypted), CRC, alignment
        - If flash_aes_type == 'NONE' or other: no encryption, but CRC and alignment (if flash_crc_en and has_flash_encrypted)
        
        Important: nvs_key.bin must remain as raw key data (no header) because:
        1. nvs partition needs raw nvs_key.bin to encrypt nvs.csv
        2. user_mfr needs raw nvs_key.bin to encrypt user_mfr.bin
        
        The processed file (with encryption/CRC/padding) will be used for creating pack header.
        """
        key_bin_name = f'{self.name()}.bin'
        if not os.path.exists(key_bin_name):
            logging.warning(f'{self.name()}: {key_bin_name} not exists, skip processing')
            return
        
        # Normalize flash_aes_type (default to 'NONE' if not provided)
        if flash_aes_type is None:
            flash_aes_type = 'NONE'
        else:
            flash_aes_type = flash_aes_type.upper()
        
        # Check if partition has flash_encrypted flag
        has_flash_encrypted = False
        if hasattr(self, '_str_flags') and self._str_flags:
            has_flash_encrypted = 'flash_encrypted' in self._str_flags
        
        # Reference: beken_utils partition.py lines 705-707
        # If flash_aes_type == 'RANDOM': no encryption, no CRC, no alignment
        if flash_aes_type == 'RANDOM':
            self._final_bin_name = key_bin_name
            self._partition_hdr_pad_size = 0
            self._final_partition_offset = self.size().offset()
            logging.info(f'{self.name()}: RANDOM mode, using original {key_bin_name} without alignment/encryption/CRC')
            return
        
        # ========== MODIFICATION START: FIXED/NONE mode - determine encryption and CRC needs ==========
        # Reference: beken_utils partition.py lines 708-723
        # Key observation: beken_utils ALWAYS performs CRC and alignment for FIXED/NONE modes
        # It does NOT check flash_crc_en or has_flash_encrypted for CRC decision
        # Only RANDOM mode skips CRC and alignment
        # 
        # For FIXED or NONE (or other): ALWAYS need CRC and alignment (matching beken_utils)
        # Only FIXED mode needs encryption (if has_flash_encrypted and flash_aes_key is provided)
        need_encryption = (flash_aes_type == 'FIXED') and has_flash_encrypted and (flash_aes_key is not None)
        # Note: beken_utils always performs CRC for non-RANDOM modes, regardless of flash_crc_en or has_flash_encrypted
        # So we should always perform CRC and alignment for FIXED/NONE modes
        
        logging.info(f'{self.name()}: flash_aes_type={flash_aes_type}, has_flash_encrypted={has_flash_encrypted}, need_encryption={need_encryption}, flash_crc_en={flash_crc_en}')
        # ========== MODIFICATION END ==========
        
        # ========== MODIFICATION START: Process with alignment, encryption and CRC ==========
        # Reference: beken_utils partition.py lines 709-723
        # Step 1: Align partition offset to 34-byte boundary (CRC_UNIT_TOTAL_SZ)
        original_offset = self.size().offset()
        if flash_crc_en:
            phy_partition_offset = ceil_align(original_offset, CRC_UNIT_TOTAL_SZ)
        else:
            phy_partition_offset = original_offset
        
        # Step 2: Encrypt nvs_key.bin with Flash AES key (only for FIXED mode)
        # Reference: beken_utils partition.py lines 709-719
        if need_encryption:
            # Convert physical address to virtual address for encryption tool
            start_address = hex(phy2virtual(phy_partition_offset))
            
            aes_bin_name = f'{self.name()}_aes.bin'
            aes_tool = get_flash_aes_tool()
            logging.debug(f'{self.name()}: encrypt {key_bin_name}, startaddress={start_address}, out={aes_bin_name}')
            cmd = f'python3 {aes_tool} encrypt -infile {key_bin_name} -keywords {flash_aes_key} -aes {128} -outfile {aes_bin_name} -startaddress {start_address}'
            run_cmd_not_check_ret(cmd)
        else:
            # For NONE mode or when encryption is not needed, use original file
            aes_bin_name = key_bin_name
            logging.debug(f'{self.name()}: no encryption (flash_aes_type={flash_aes_type}), using {key_bin_name} directly')
        
        # Step 3: Optionally add CRC16 (respect flash_crc_en); otherwise copy source file
        # Reference: beken_utils partition.py lines 721-722
        crc_bin_name = f'{self.name()}_crc.bin'
        if flash_crc_en:
            # Verify that input file exists before calling calc_crc16
            if not os.path.exists(aes_bin_name):
                raise FileNotFoundError(f'{self.name()}: Input file for CRC calculation not found: {aes_bin_name}')
            calc_crc16(aes_bin_name, crc_bin_name)
            logging.info(f'{self.name()}: added CRC to {crc_bin_name}')
        else:
            # Skip CRC: copy the encrypted/original file to crc_bin_name (same content, different filename)
            shutil.copy(aes_bin_name, crc_bin_name)
            logging.info(f'{self.name()}: flash_crc_en is False, skip CRC (copy {aes_bin_name} to {crc_bin_name})')
        
        # Step 4: Calculate padding size needed to align data to 34-byte boundary
        # Reference: beken_utils partition.py line 723
        partition_hdr_pad_size = phy_partition_offset - original_offset
        
        # Step 5: Add padding (0xFF) before data to align to 34-byte boundary
        # Reference: beken_utils partition.py lines 725-735
        bin_size = os.path.getsize(crc_bin_name)
        with open(crc_bin_name, 'rb') as f_in:
            data = f_in.read()
        
        # Create padded data: padding + actual data
        pad = bytes([0xFF] * partition_hdr_pad_size)
        padded_data = pad + data
        logging.info(f'{self.name()}: padded data: {len(padded_data)} bytes, data: {len(data)} bytes, pad_size: {partition_hdr_pad_size}')
        
        # Check if padded data exceeds partition size
        if len(padded_data) > self.size().size():
            read_size = self.size().size() - partition_hdr_pad_size
            padded_data = pad + data[:read_size]
            logging.warning(f'{self.name()}: padded data exceeds partition size, truncating to {len(padded_data)} bytes')
        
        # Step 6: Write padded data to processed file (for pack header)
        # Note: Keep original nvs_key.bin unchanged (raw key data for nvs and user_mfr)
        processed_bin_name = f'{self.name()}_processed.bin'
        with open(processed_bin_name, 'wb') as f_out:
            f_out.write(padded_data)
        
        # Update internal state
        self._final_bin_name = processed_bin_name
        self._partition_hdr_pad_size = partition_hdr_pad_size
        self._final_partition_offset = phy_partition_offset  # Aligned address
        
        logging.info(f'{self.name()}: processed with alignment - original_offset=0x{original_offset:x}, aligned_offset=0x{phy_partition_offset:x}, pad_size={partition_hdr_pad_size}, encrypted={need_encryption}')
        # ========== MODIFICATION END ==========
    
    def _add_pack_header_to_nvs_key_bin(self):
        """Create a pack header version of nvs_key.bin for PackDownload
        
        Important: nvs_key.bin must remain as raw key data (no header) because:
        1. nvs partition needs raw nvs_key.bin to encrypt nvs.csv
        2. user_mfr needs raw nvs_key.bin to encrypt user_mfr.bin
        
        Solution: Create a separate file with pack header (nvs_key_pack.bin) for PackDownload
        The processed file (with encryption/CRC/padding) will be used if available.
        """
        from .pack_header import PackHeader, PACK_HEADER_SIZE, PACK_HEADER_MAGIC
        
        key_bin_name = f'{self.name()}.bin'
        pack_bin_name = f'{self.name()}_pack.bin'  # File with pack header for PackDownload
        
        # If processing was performed, use the processed file (with encryption/CRC/padding)
        # Otherwise, use the original nvs_key.bin
        if hasattr(self, '_final_bin_name') and os.path.exists(self._final_bin_name):
            source_bin_name = self._final_bin_name  # Processed file (nvs_key_processed.bin)
            logging.debug(f'{self.name()}: using processed file {source_bin_name} for pack header')
        else:
            source_bin_name = key_bin_name  # Original file
            logging.debug(f'{self.name()}: using original file {source_bin_name} for pack header')
        
        if not os.path.exists(source_bin_name):
            logging.warning(f'{self.name()}: {source_bin_name} not exists, cannot create pack version')
            return
        
        # Reference: partition.py _add_pack_header_to_bin() - uses self.size().offset()
        # Reference: partition_nvs.py _add_pack_header_to_nvs_bin() - uses self.size().offset()
        # Pack header offset represents the partition's physical start address in Flash, not the data start address
        # The nvs_key_processed.bin already contains padding at the beginning, so pack_download.py will write
        # the entire file (including padding) starting from the partition offset
        partition_offset = self.size().offset()
        logging.debug(f'{self.name()}: using partition offset 0x{partition_offset:x} in pack header (original, not aligned)')
        
        # ========== MODIFICATION START: Extract raw data from source file (remove existing header if any) ==========
        # Read source file and check if it already has a pack header
        # If it has a header, extract only the raw data; otherwise use all data
        raw_data = None
        with open(source_bin_name, 'rb') as f:
            # Check if file starts with pack header magic
            header_data = f.read(PACK_HEADER_SIZE)
            if len(header_data) == PACK_HEADER_SIZE:
                # Try to parse header
                try:
                    import struct
                    from .pack_header import PACK_HEADER_FORMAT
                    magic_bytes, name_bytes, version_bytes, type_, subtype, offset, size, flags, hdr_size, m1, m2, m3, m4 = struct.unpack(PACK_HEADER_FORMAT, header_data)
                    magic = magic_bytes.rstrip(b'\x00').decode('utf-8')
                    if magic == PACK_HEADER_MAGIC:
                        # File already has a pack header, read only the data part
                        raw_data = f.read()
                        logging.debug(f'{self.name()}: {source_bin_name} already has pack header, extracted raw data ({len(raw_data)} bytes)')
                    else:
                        # No valid header, read all data
                        f.seek(0)
                        raw_data = f.read()
                        logging.debug(f'{self.name()}: {source_bin_name} has no pack header, using all data ({len(raw_data)} bytes)')
                except Exception as e:
                    # Parse failed, treat as raw data
                    f.seek(0)
                    raw_data = f.read()
                    logging.debug(f'{self.name()}: {source_bin_name} parse failed ({e}), using all data ({len(raw_data)} bytes)')
            else:
                # File is too small or empty, use as is
                f.seek(0)
                raw_data = f.read()
                logging.debug(f'{self.name()}: {source_bin_name} is small/empty, using all data ({len(raw_data)} bytes)')
        # ========== MODIFICATION END ==========
        
        # ========== MODIFICATION START: Create pack header version in separate file ==========
        # Create a new file with pack header for PackDownload
        # Keep original nvs_key.bin unchanged (raw key data for nvs and user_mfr)
        ph = PackHeader()
        version = "1.0.0"  # Default version
        partition_name = self.name()
        partition_type = self.type().type()
        partition_subtype = self.type().subtype()
        partition_size = self.size().size()
        partition_flags = self.flags().flags()
        hdr_size = 0  # No additional header size
        
        # Write raw data to temporary file first
        temp_bin_name = f'_{pack_bin_name}'
        with open(temp_bin_name, 'wb') as f:
            f.write(raw_data)
        
        # Create pack header on the temporary file
        # Reference: partition.py _add_pack_header_to_bin() - pass m1, m2, m3, m4 explicitly
        ph.create_header(
            temp_bin_name,
            version,
            partition_name,
            partition_type,
            partition_subtype,
            partition_offset,  # Use aligned offset if available
            partition_size,
            partition_flags,
            hdr_size,
        )
        
        # Rename to final pack file name
        if os.path.exists(pack_bin_name):
            os.remove(pack_bin_name)
        os.rename(temp_bin_name, pack_bin_name)
        
        logging.info(f'{self.name()}: created {pack_bin_name} with pack header (offset=0x{partition_offset:x}), original {key_bin_name} kept as raw key data')
        # ========== MODIFICATION END ==========

