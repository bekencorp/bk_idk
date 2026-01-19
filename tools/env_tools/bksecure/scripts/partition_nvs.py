#!/usr/bin/env python3

import os
import logging
import shutil

from .common import *
from .partition import Partition
from .bin import Bin

# ========== MODIFICATION START: Add PartitionNvs class to handle nvs partition ==========
# Reference: xnf_connect_1226/tools/env_tools/beken_utils/scripts/partition.py process_partition_nvs() lines 739-768
# This class generates nvs.bin from nvs.csv and nvs_key.bin when processing nvs partition
class PartitionNvs(Partition):

    def __init__(self, idx=None, field_dic=None, mini_offset=None, partitions=None):
        super().__init__(idx, field_dic, mini_offset)
        # nvs partition doesn't need a Bin object, it generates nvs.bin directly
        self._bin = None
        self._partitions = partitions

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        # ========== MODIFICATION START: Generate nvs.bin from nvs.csv and nvs_key.bin ==========
        # Reference: xnf_connect_1226/tools/env_tools/beken_utils/scripts/partition.py lines 739-768
        self.__generate_nvs_bin()
        # ========== MODIFICATION END ==========
        # Add pack header to nvs.bin (required for PackDownload to parse it correctly)
        self._add_pack_header_to_nvs_bin()
        # Don't call super().postbuild_process() because nvs.bin doesn't have a Bin object
        # super().postbuild_process(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en)

    def __generate_nvs_bin(self):
        """Generate nvs.bin from nvs.csv and nvs_key.bin using nvs_partition_gen.py"""
        nvs_bin_name = f'{self.name()}.bin'
        nvs_aes_bin_name = f'{self.name()}_aes.bin'
        
        # Check if nvs.bin already exists
        if os.path.exists(nvs_bin_name):
            logging.debug(f'{self.name()}: using existing {nvs_bin_name}')
            return
        
        # Find nvs_key partition to get nvs_key.bin
        # Note: nvs_key partition should be processed before nvs partition
        nvs_key_bin_name = 'nvs_key.bin'
        if not os.path.exists(nvs_key_bin_name):
            logging.error(f'{self.name()}: {nvs_key_bin_name} not exists, required for generating {nvs_bin_name}')
            exit(1)
        
        # Check if nvs.csv exists
        nvs_csv_name = f'{self.name()}.csv'
        if not os.path.exists(nvs_csv_name):
            logging.error(f'{self.name()}: {nvs_csv_name} not exists, required for generating {nvs_bin_name}')
            exit(1)
        
        # Get NVS partition generator tool path
        tools_dir = get_tools_dir()
        nvs_tool = f'{tools_dir}/NVS/nvs_partition_gen.py'
        
        if not os.path.exists(nvs_tool):
            logging.error(f'{self.name()}: NVS tool not found at {nvs_tool}')
            exit(1)
        
        # Generate nvs_aes.bin from nvs.csv and nvs_key.bin
        # Command: python3 nvs_partition_gen.py encrypt nvs.csv nvs_aes.bin partition_size --inputkey nvs_key.bin
        # Note: partition_size should be in hex format (e.g., 0x20000 for 128k)
        partition_size_hex = hex(self.size().size())
        cmd = f'python3 {nvs_tool} encrypt {nvs_csv_name} {nvs_aes_bin_name} {partition_size_hex} --inputkey {nvs_key_bin_name}'
        logging.debug(f'{self.name()}: generating {nvs_aes_bin_name} from {nvs_csv_name} and {nvs_key_bin_name}: {cmd}')
        run_cmd_not_check_ret(cmd)
        
        # Verify nvs_aes.bin was created
        if not os.path.exists(nvs_aes_bin_name):
            logging.error(f'{self.name()}: failed to generate {nvs_aes_bin_name}')
            exit(1)
        
        # ========== MODIFICATION START: Ensure nvs_aes.bin is aligned to page size (4096 bytes) ==========
        # nvs_parser requires partition data to be aligned to page size (4096 bytes)
        # If the generated file is not page-aligned, pad it with 0xFF to the next page boundary
        NVS_PAGE_SIZE = 4096
        file_size = os.path.getsize(nvs_aes_bin_name)
        if file_size % NVS_PAGE_SIZE != 0:
            # Calculate padding needed to align to next page boundary
            padding_size = NVS_PAGE_SIZE - (file_size % NVS_PAGE_SIZE)
            logging.debug(f'{self.name()}: {nvs_aes_bin_name} size ({file_size}) is not page-aligned, padding {padding_size} bytes')
            
            # Read existing data
            with open(nvs_aes_bin_name, 'rb') as f:
                data = f.read()
            
            # Append padding (0xFF, which is the NVS default fill value)
            padding = bytes([0xFF] * padding_size)
            padded_data = data + padding
            
            # Write padded data back
            with open(nvs_aes_bin_name, 'wb') as f:
                f.write(padded_data)
            
            logging.info(f'{self.name()}: padded {nvs_aes_bin_name} from {file_size} to {len(padded_data)} bytes (aligned to page size)')
        # ========== MODIFICATION END ==========
        
        # Copy nvs_aes.bin to nvs.bin (for compatibility with pack.json)
        shutil.copy(nvs_aes_bin_name, nvs_bin_name)
        logging.info(f'{self.name()}: generated {nvs_bin_name} from {nvs_csv_name} and {nvs_key_bin_name}')
    
    def _add_pack_header_to_nvs_bin(self):
        """Add pack header to nvs.bin so it can be parsed by PackDownload"""
        from .pack_header import PackHeader, PACK_HEADER_SIZE, PACK_HEADER_MAGIC
        
        nvs_bin_name = f'{self.name()}.bin'
        if not os.path.exists(nvs_bin_name):
            logging.warning(f'{self.name()}: {nvs_bin_name} not exists, cannot add pack header')
            return
        
        # ========== MODIFICATION START: Check if file already has a pack header ==========
        # Check if nvs.bin already has a pack header to avoid adding multiple headers
        has_header = False
        with open(nvs_bin_name, 'rb') as f:
            header_data = f.read(PACK_HEADER_SIZE)
            if len(header_data) == PACK_HEADER_SIZE:
                try:
                    import struct
                    from .pack_header import PACK_HEADER_FORMAT
                    magic_bytes, name_bytes, version_bytes, type_, subtype, offset, size, flags, hdr_size, m1, m2, m3, m4 = struct.unpack(PACK_HEADER_FORMAT, header_data)
                    magic = magic_bytes.rstrip(b'\x00').decode('utf-8')
                    if magic == PACK_HEADER_MAGIC:
                        has_header = True
                        logging.debug(f'{self.name()}: {nvs_bin_name} already has pack header, skipping')
                except Exception as e:
                    # Parse failed, treat as no header
                    logging.debug(f'{self.name()}: {nvs_bin_name} header parse failed ({e}), will add new header')
        
        if has_header:
            return
        # ========== MODIFICATION END ==========
        
        # ========== MODIFICATION START: Use original offset without alignment (consistent with beken_utils) ==========
        # Reference: xnf_connect_1226/tools/env_tools/beken_utils/scripts/partition.py process_partition_nvs() line 768
        # beken_utils uses self.partition_offset directly without alignment for nvs partition
        # nvs partition does not need address alignment, encryption, or CRC (it's already encrypted with nvs_key)
        partition_offset = self.size().offset()
        # ========== MODIFICATION END ==========
        
        # Create pack header for nvs.bin
        # Use partition name, type, subtype, offset, size, and flags from partition definition
        ph = PackHeader()
        version = "1.0.0"  # Default version
        partition_name = self.name()
        partition_type = self.type().type()
        partition_subtype = self.type().subtype()
        partition_size = self.size().size()
        partition_flags = self.flags().flags()
        hdr_size = 0  # No additional header size
        
        ph.create_header(
            nvs_bin_name,
            version,
            partition_name,
            partition_type,
            partition_subtype,
            partition_offset,
            partition_size,
            partition_flags,
            hdr_size
        )
        logging.debug(f'{self.name()}: added pack header to {nvs_bin_name}')
# ========== MODIFICATION END ==========

