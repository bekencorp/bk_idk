#!/usr/bin/env python3
import os
import json
import logging
import shutil
import re
import copy
import hashlib
import math

from .crc import *
from .common import *
from .security import *
from .ota import *
from .parse_csv import *

SZ_16M = 0x1000000
FLASH_SECTOR_SZ = 0x1000
CRC_UNIT_DATA_SZ = 32
CRC_UNIT_TOTAL_SZ = 34
GLOBAL_HDR_LEN = 32
IMG_HDR_LEN = 32
COMPRESS_BLOCK_SZ = 0x8000
HDR_SZ = 0x1000
TAIL_SZ = 0x2F0

partition_keys = [
    'Name',
    'Offset',
    'Size',
    'Execute',
    'Read',
    'Write'
]

def signing(partition, in_bin_name, out_bin_name, key, security_counter, version='0.0.1', is_virture = 0):
    logging.debug(f'signing: partition_name={partition.partition_name}, partition_size={partition.partition_size}')
    logging.debug(f'signing: in_bin_name={in_bin_name}, out_bin_name={out_bin_name}, in_bin_size={os.path.getsize(in_bin_name)}')
    logging.debug(f'signing: key={key}, security_counter={security_counter}')

    bl2_signing_tool_dir = f'{partition.tools_dir}/mcuboot_tools/imgtool.py'
    if(is_virture):
        cmd = f'{bl2_signing_tool_dir} sign -k {key} --public-key-format full --max-align 8 --align 1 --version {version} --security-counter {security_counter} --pad-header --header-size 0x1000 --slot-size {partition.virtual_partition_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 {in_bin_name} {out_bin_name}'
    else:
        cmd = f'{bl2_signing_tool_dir} sign -k {key} --public-key-format full --max-align 8 --align 1 --version {version} --security-counter {security_counter} --pad-header --header-size 0x1000 --slot-size {partition.partition_size} --pad --boot-record SPE --endian little --encrypt-keylen 128 {in_bin_name} {out_bin_name}'
    run_cmd(cmd)
    return

class Hdr:
    def __init__(self, magic, crc, version, hdr_len, img_num, flags = 0):
        self.magic = bytes.fromhex(magic)
        self.crc = bytes.fromhex(int2hexstr4(crc))
        self.version = bytes.fromhex(int2hexstr4(version))
        self.hdr_len = bytes.fromhex(int2hexstr2(hdr_len))
        self.img_num = bytes.fromhex(int2hexstr2(img_num))
        self.flags = bytes.fromhex(int2hexstr4(flags))
        self.reserved = bytes(0)

        logging.debug(f'global hdr: magic={self.magic}, version={self.version}, hdr_len={self.hdr_len}, img_num={self.img_num}, flags={self.flags}')

class Imghdr:
    def __init__(self, img_len, img_offset, flash_offset, crc, version, flags = 0):
        self.img_len = bytes.fromhex(int2hexstr4(img_len))
        self.img_offset = bytes.fromhex(int2hexstr4(img_offset))
        self.flash_offset = bytes.fromhex(int2hexstr4(flash_offset))
        self.crc = bytes.fromhex(int2hexstr4(crc))
        self.version = bytes.fromhex(int2hexstr4(version))
        self.flags = bytes.fromhex(int2hexstr4(flags))
        self.reserved = bytes(0)
        logging.debug(f'img hdr: img_len=%x, img_offset=%x, flash_offset=%x, crc=%x, version=%x flags=%x'
            %(img_len, img_offset, flash_offset, crc, version, flags))
 
class Partition:
    def is_out_of_range(self, addr):
        if (addr >= SZ_16M):
            return True 
        else:
            return False

    def bin_is_code(self):
        if (self.bin_type == "code"):
            return True
        else:
            return False

    def is_flash_sector_aligned(self, addr):
        if ((addr % FLASH_SECTOR_SZ) == 0):
            return True
        else:
            return False

    def parse_raw_fields(self):
        self.partition_name = self.pdic['Name']
        self.partition_offset = self.pdic['Offset']
        self.partition_size = self.pdic['Size']
        self.permission = self.pdic['Execute']

        logging.debug(f'partition{self.idx} raw fields:')
        logging.debug(f'partition_name = {self.partition_name}')
        logging.debug(f'partition_offset = {self.partition_offset}')
        logging.debug(f'partition_size = {self.partition_size}')
        logging.debug(f'permission = {self.permission}')

    def is_bootloader_partition(self):
        if (self.partition_name.endswith("bootloader") or self.partition_name.endswith("bl2")):
            return True
        else:
            return False

    def find_partition_by_name(self, name):
        for p in self.partitions.partitions:
            if (name == p.partition_name):
                return p
        return None

    def parse_and_validate_bin_type(self):
        self.bin_type = "data"
        if pattern_match(self.permission, r"TRUE") == True:
            self.bin_type = 'code'

        if (self.bin_type == "code"):
            self.need_add_crc = True
        else:
            self.need_add_crc = False

        logging.debug(f'bin type={self.bin_type} need_add_crc={self.need_add_crc}')
 
    def parse_and_validate_partition_name(self):
        reserved_partition_names = ['primary_all', 'secondary_all', 'all']
        secureboot_partitions = ['bl1_control', 'primary_manifest', 'secondary_manifest']

        if self.partition_name in reserved_partition_names:
            logging.error(f'Partition {self.partition_name} is reserved')
            exit(1)

        if self.partition_name in secureboot_partitions:
            if self.secureboot_en == False:
                logging.error(f'partition {self.partition_name} is reserved for secureboot')
                exit(1)

        self.pre_partition = None
        if self.idx != 0:
            self.pre_partition = self.partitions.partition_names[self.idx - 1]
        logging.debug(f'pre partition = {self.pre_partition}')

        self.file_name_prefix = self.partition_name
        self.primary_partition_name = None
        self.primary_partition = None
        self.is_secondary = False
        self.is_primary = False

        if pattern_match(self.partition_name, r"secondary_"):
            self.is_secondary = True
            self.primary_partition_name = self.partition_name.replace("secondary_", "primary_")
            self.primary_partition = self.find_partition_by_name(self.primary_partition_name)
            if self.primary_partition == None:
                logging.debug(f'Code partition {self.partition_name} has no primary partition')
                exit(1)
        elif pattern_match(self.partition_name, r"primary_"):
            self.is_primary = True

        if (self.bin_is_code() == False) and (self.is_primary or self.is_secondary) and (self.partition_name != 'primary_manifest') and (self.partition_name != 'secondary_manifest'):
            logging.error(f'Invalid data partition {self.partition_name}, prefix primary_/secondary is only for code partition')
            exit(1)

    def parse_and_validate_bin_name(self):
        self.bin_name = None
        self.bin_size = 0

        if (self.partition_name == 'bl1_control'):
            self.bin_name = 'bl1_control.bin'
            return

        if (self.partition_name == 'primary_manifest'):
            self.bin_name = 'primary_manifest.bin'
            return

        if (self.partition_name == 'secondary_manifest'):
            self.bin_name = 'secondary_manifest.bin'
            return

        if (self.partition_name == 'bl2'):
            self.bin_name = 'bl2.bin'
            return

        if (self.partition_name == 'ota'):
            return

        if (self.bin_type == 'data'):
            return

        if (pattern_match(self.partition_name, r"primary_") == False) and (pattern_match(self.partition_name, r"secondary_") == False):
            logging.error(f'code partition {self.partition_name} should started with primary_ or secondary_')
            exit(1)

        self.bin_name = self.partition_name.replace("secondary_", "")
        self.bin_name = self.bin_name.replace("primary_", "")
        self.bin_name += '.bin'
        logging.debug(f'partition={self.partition_name}, bin_name={self.bin_name}')

    def parse_and_validate_verifier(self):
        self.bin_verifier = None
        self.bin_hdr_size = 0
        self.bin_tail_size = 0
 
        if (self.bin_type == "data"):
            return

        if (self.partition_name == 'bl2'):
            if self.secureboot_en:
                self.bin_verifier = 'bl1'
        elif (self.bl2_exists) and (self.idx > self.bl2_partition_idx):
            self.bin_verifier = 'bl2'
            if self.ota.is_overwrite():
                self.Dbus_en = True
                self.bin_hdr_size = crc_size(HDR_SZ)
            else:
                self.bin_hdr_size = HDR_SZ
            self.bin_tail_size = 0x1000
            if self.is_primary:
                self.partitions.append_primary_partitions_verified_by_bl2(self.partition_name)
            if self.is_secondary:
                self.partitions.append_secondary_partitions_verified_by_bl2(self.partition_name)

        logging.debug(f'bin verifier={self.bin_verifier}, bin_hdr_size={self.bin_hdr_size}, bin_tail_size={self.bin_tail_size}')


    def parse_and_validate_partition_offset(self):
        if self.partition_offset == '':
            self.partition_offset = self.deduced_min_partition_offset
        else:
            self.partition_offset = hex2int(self.partition_offset)

        if (self.idx == 0) and (self.partition_offset > 0):
            logging.error(f'first {self.partition_name} offset should be 0')
            exit(1)

        if (self.partition_offset < self.deduced_min_partition_offset):
            logging.error(f'partition{self.idx} partition {self.partition_name} offset=%x overlapped with {self.pre_partition}' %(self.partition_offset))
            exit(1)

        if (self.is_out_of_range(self.partition_offset) == True):
            logging.error(f'partition{self.idx} partition {self.partition_name} offset=%x is out of range' %(self.partition_offset))
            exit(1)

        if (self.bin_is_code()):
            if (self.is_flash_sector_aligned(self.partition_offset) == False):
                logging.error(f'partition{self.idx} partition {self.partition_name} offset=%x not FLASH sector aligned' %(self.phy_partition_offset))
                exit(1)

        logging.debug(f'partition{self.idx} {self.partition_name} partition_offset=%x' %(self.partition_offset))

    def parse_and_validate_partition_size(self):
        if self.partition_size == '':
            logging.error(f'Size of partition {self.partition_name} is not configured')
            exit(1)

        self.partition_size = size2int(self.partition_size)
        if (self.is_out_of_range(self.partition_size) == True):
            logging.error(f'partition{self.idx} partition {self.partition_name} size=%x out of range' %(self.partition_size))
            exit(1)

        if (self.is_flash_sector_aligned(self.partition_size) == False):
            logging.error(f'partition{self.idx} partition {self.partition_name} size=%x not FLASH sector aligned' %(self.partition_size))
            exit(1)

        if self.is_secondary:
            if (self.partition_size != self.primary_partition.partition_size):
                logging.error(f'Size of {self.partition_name} and {self.primary_partition.partition_name} not equal')
                exit(1)

        logging.debug(f'partition{self.idx} {self.partition_name} partition size=%x' %(self.partition_size))
 

    def find_partition_by_name(self, name):
        for p in self.partitions.partitions:
            if (name == p.partition_name):
                return p
        return None

    '''
    +------------------------+  <--- partition_offset/bin_hdr_offset
    |  HDR (plain data)      |
    |  (bin_hdr_size)        |  
    |                        |
    +------------------------+  <--- bin_code_partition_offset
    |  vector/CRC aligned pad|
    |                        |
    +------------------------+  <--- bin_1st_instruction_offset
    |  Code (CRC/Encrypted)  |
    |                        |
    |                        |
    +------------------------+
    |  Unused                |
    |                        |
    +------------------------+  <--- bin_tail_offset
    |  TAIL (plain data)     |
    |  (bin_tail_size)       |
    |                        |
    +------------------------+  <--- partition end
    '''

    def check_s_ns_offset(self):
        s_ns_boundary = self.partition_offset + self.partition_size
        if (s_ns_boundary % (68<<10)) != 0:
            block = s_ns_boundary // (68<<10)
            suggest_s_ns_boundary = (block + 1) * (68 << 10)
            diff = suggest_s_ns_boundary - s_ns_boundary
            logging.debug(f'NS start {s_ns_boundary/1024}, suggest start {suggest_s_ns_boundary/1024}')
            logging.error(f'The offset of partition next to TFM_S is %x, NOT in 68K boundary!' %(s_ns_boundary))
            size_k = self.partition_size/1024
            suggest_size_k = (self.partition_size + diff)/1024
            logging.error(f'Suggest to change TFM_S partition size from {size_k}k to {suggest_size_k}k')
            exit(1)

    def calculate_offset(self,hdr_size):
        # Are the results of two branches the same?
        if self.Dbus_en:
            self.phy_partition_offset = ceil_align(self.partition_offset, CRC_UNIT_TOTAL_SZ)
            self.vir_partition_offset = phy2virtual(self.phy_partition_offset)
            self.vir_code_offset = ceil_align(self.vir_partition_offset + hdr_size,self.cpu_vector_align_bytes)
            self.phy_code_offset = virtual2phy(self.vir_code_offset)
        else:
            self.phy_partition_offset = self.partition_offset
            self.vir_partition_offset = phy2virtual(self.phy_partition_offset)
            phy_min_code_offset = self.phy_partition_offset + hdr_size
            vir_min_code_offset = phy2virtual(phy_min_code_offset)
            self.vir_code_offset = ceil_align(vir_min_code_offset,self.cpu_vector_align_bytes)
            self.phy_code_offset = virtual2phy(self.vir_code_offset)

        logging.debug(f'name = %s, phy_partition_offset= 0x%x, vir_partition_offset= 0x%x,vir_code_offset= 0x%x, phy_code_offset= 0x%x' 
                    %(self.partition_name,self.phy_partition_offset,self.vir_partition_offset,self.vir_code_offset,self.phy_code_offset))


    def calculate_size(self):
        self.partition_hdr_pad_size =self.phy_partition_offset - self.partition_offset
        self.partition_tail_pad_size = (self.partition_offset+self.partition_size) - floor_align(self.partition_offset+self.partition_size,34)
        self.phy_partition_size = self.partition_size - self.partition_hdr_pad_size - self.partition_tail_pad_size
        self.virtual_partition_size = phy2virtual(self.phy_partition_size)
        self.phy_code_size = self.phy_partition_size - (self.phy_code_offset - self.phy_partition_offset)
        self.vir_code_size = phy2virtual(self.phy_code_size)

        logging.debug(f'phy_partition_size= 0x%x, virtual_partition_size= 0x%x' %(self.phy_partition_size,self.virtual_partition_size))


    def parse_and_validate_address(self):
        if (self.is_out_of_range(self.partition_offset + self.partition_size - 1) == True):
            logging.error(f'partition{self.idx} {self.partition_name} start=%x size=%x is out of range' %(self.partition_offset, self.partition_size))
            exit(1)

        if(self.bin_is_code()):
            if (self.is_secondary) and (self.bin_verifier == "bl2"):
                if (self.primary_partition == None):
                    logging.error(f'partition{self.idx} {self.partition_name} missing primary partition')
                    exit(1)
                # if swap
                self.phy_partition_offset = self.primary_partition.phy_partition_offset
                self.vir_partition_offset = self.primary_partition.vir_partition_offset
                self.vir_code_offset = self.primary_partition.vir_code_offset
                self.phy_code_offset = self.primary_partition.phy_code_offset
                # else xip
                # if len(self.partitions.secondary_partitions_verified_by_bl2) == 1:
                #     self.calculate_offset(0)
                # else:
                #     self.calculate_offset(HDR_SZ)
            elif (self.bin_verifier == "bl2"):
                if len(self.partitions.primary_partitions_verified_by_bl2) == 1:
                    self.calculate_offset(HDR_SZ)
                else:
                    self.calculate_offset(0)
            else:
                self.calculate_offset(0)

            # SWAP
            if (self.partition_name == 'primary_tfm_s'):
                self.check_s_ns_offset()
            # XIP
            # if (self.partition_name == 'primary_tfm_s' or self.partition_name == 'secondary_tfm_s'):

            self.calculate_size()

            with open("partition.bin", 'a+b') as f:
                # 32 bytes for name,16 bytes for data and 16 bytes reserved
                name = self.partition_name.encode('utf-8')
                if len(name) < 24:
                    name += bytes([0xFF] * (24 - len(name)))
                f.write(name)

                offset = struct.pack(">I",(self.partition_offset))
                f.write(offset)

                size = struct.pack(">I",(self.partition_size))
                f.write(size)

                offset = struct.pack(">I",(self.vir_code_offset))
                f.write(offset)

                size = struct.pack(">I",(self.vir_code_size))
                f.write(size)

        else:
            # data bin : 24 bytes for name,8 bytes for data and 8 bytes pad
            with open("partition.bin", 'a+b') as f:
                name = self.partition_name.encode('utf-8')
                if len(name) < 24:
                    name += bytes([0xFF] * (24 - len(name)))
                f.write(name)

                offset = struct.pack(">I",(self.partition_offset))
                f.write(offset)

                size = struct.pack(">I",(self.partition_size))
                f.write(size)

                pad = bytes([0xFF] * 8)
                f.write(pad)

    def post_parse_and_validate_partitions(self):
        for p in self.partitions:
            p.post_parse_and_validate_partitions()
 
    def parse_and_validate_static_and_load_addr(self):
        if (self.bin_is_code() == False):
            return

        virtual_1st_instruction_addr = phy2virtual(self.phy_code_offset)
        static_addr_int = self.flash_base_addr + virtual_1st_instruction_addr
        static_addr_hex = f'0x%08x' %(static_addr_int)
        self.static_addr = static_addr_hex
        # Set load_addr to a fixed value if it's different as static_addr
        self.load_addr = self.static_addr
        logging.debug(f'static_addr = {self.static_addr}, load address={self.load_addr}')

    def parse_and_validate_bin(self):
        if self.bin_name == None:
            self.bin_size = 0
            return

        if not os.path.exists(self.bin_name):
            logging.error(f'bin {self.bin_name} for partition{self.idx} {self.partition_name} not exists')
            exit(1)

        self.bin_size = os.path.getsize(self.bin_name)
        if self.bin_is_code():
            if self.Dbus_en:
                if(self.vir_code_offset + self.bin_size > self.vir_partition_offset + self.virtual_partition_size):
                    logging.error(f'partition{self.idx} {self.partition_name} bin size %x > partition size %x' %(self.bin_size, self.virtual_partition_size))
                    exit(1)
            else:
                if(self.phy_code_offset + self.bin_size > self.phy_partition_offset + self.phy_partition_size):
                    logging.error(f'partition{self.idx} {self.partition_name} bin size %x > partition size %x' %(self.bin_size, self.virtual_partition_size))
                    exit(1)
        else:
            self.bin_data_size = self.bin_size
            self.bin_total_size = self.bin_size

            if (self.bin_size > self.partition_size):
                logging.error(f'partition{self.idx} {self.partition_name} bin size %x > partition size %x' %(self.bin_size, self.partition_size))
                exit(1)

            if (self.bin_total_size > self.partition_size):
                logging.error(f'partition{self.idx} {self.partition_name} total bin size %x > partition size %x' %(self.bin_total_size, self.partition_size))
                exit(1)

    def parse_and_validate_after_build(self):
        self.parse_and_validate_bin()

    def __init__(self, partitions, idx, pdic, deduced_min_partition_offset):
        self.tools_dir = partitions.tools_dir
        self.partitions = partitions
        self.flash_aes_en = partitions.flash_aes_en
        self.flash_aes_key = partitions.flash_aes_key
        self.secureboot_en = partitions.secureboot_en
        self.idx = idx
        self.pdic = pdic
        self.ota = partitions.ota
        self.deduced_min_partition_offset = deduced_min_partition_offset
        self.flash_base_addr = partitions.flash_base_addr
        self.cpu_vector_align_bytes = partitions.cpu_vector_align_bytes
        self.need_add_crc = False
        self.crc = 0 # TODO
        self.bl2_exists = self.partitions.bl2_exists
        self.bl2_partition_idx = self.partitions.bl2_partition_idx
        self.Dbus_en = False
        self.ab_en = self.partitions.ab_en
        self.partition_buf = bytearray()

        self.parse_raw_fields()
        self.parse_and_validate_bin_type()
        self.parse_and_validate_partition_name()
        self.parse_and_validate_bin_name()
        self.parse_and_validate_verifier()
        self.parse_and_validate_partition_offset()
        self.parse_and_validate_partition_size()
        self.parse_and_validate_address()
        self.parse_and_validate_static_and_load_addr()

    def encrypt_bin(self):
        logging.info("TODO encrypt bin")

    def process_data_bin(self):
        self.bin_name = self.partition_name + ".bin"
        if os.path.exists(self.bin_name):
            self.bin_size = os.path.getsize(self.bin_name)
        if (self.bin_size == 0):
            return

        with open(self.bin_name, 'rb') as f:
            self.partition_buf = f.read()

    def init_buf(self, buf, size, fill):
        for i in range(size):
            buf[i] = fill

    def bl2_layout_contains_only_one_bin(self):
        if ((self.partitions.is_1st_bin_verified_by_bl2(self.partition_name) == True)
            and (self.partitions.is_last_bin_verified_by_bl2(self.partition_name) == True)):
            return True
        return False

    def is_1st_bin_verified_by_bl2(self):
        return self.partitions.is_1st_bin_verified_by_bl2(self.partition_name)

    def is_last_bin_verified_by_bl2(self):
        return self.partitions.is_last_bin_verified_by_bl2(self.partition_name)

    def add_magic_code(self, bin_file):
        print(f'Add magic code BK7236 to {bin_file}')
        bk7236 = b'\x42\x4b\x37\x32\x33\x36\x00'
        with open(bin_file, 'r+b') as f:
                f.seek(0x100)
                f.write(bk7236)

    def process_code_bin(self):
        if self.partition_name == 'primary_all' or self.partition_name == 'secondary_all':
            return

        if self.bin_name == None:
            return

        if self.Dbus_en:
            self.crc_bin_name = self.bin_name
        else:
            if self.flash_aes_en:
                self.aes_bin_name = f'{self.file_name_prefix}_code_aes.bin'
                aes_tool = f'{self.tools_dir}/beken_packager/beken_aes'
                start_address = hex(self.vir_code_offset)
                logging.debug(f'encrypt {self.partition_name}, startaddress={start_address}')
                cmd = f'{aes_tool} encrypt -infile {self.bin_name} -keywords {self.flash_aes_key} -outfile {self.aes_bin_name} -startaddress {start_address}'
                run_cmd_not_check_ret(cmd)
                if self.vir_code_offset == 0:
                    self.add_magic_code(self.aes_bin_name)
            else:
                self.aes_bin_name = self.bin_name
            # Add CRC
            self.crc_bin_name = f'{self.file_name_prefix}_code_crc.bin'
            if (self.need_add_crc == True):
                crc(self.aes_bin_name, self.crc_bin_name)
            else:
                self.crc_bin_name = self.aes_bin_name

        self.bin_size = os.path.getsize(self.crc_bin_name)
        logging.debug(f'{self.bin_name}:{self.bin_size}')
        self.partition_buf = bytearray()
        with open(self.crc_bin_name, 'rb') as f:
            self.partition_buf = f.read()


    def process_bin(self):
        logging.debug(f'process bin for {self.partition_name}')
        if (self.bin_is_code()):
            self.process_code_bin()
        else:
            self.process_data_bin()

class Partitions:
    def is_1st_bin_verified_by_bl2(self, partition_name):
        if (self.primary_all_partitions_cnt > 0) and (self.primary_partitions_verified_by_bl2[0] == partition_name):
            return True

        if (self.secondary_all_partitions_cnt > 0) and (self.secondary_partitions_verified_by_bl2[0] == partition_name):
            return True

        return False

    def is_last_bin_verified_by_bl2(self, partition_name):
        if (self.primary_all_partitions_cnt > 0) and (self.primary_partitions_verified_by_bl2[-1] == partition_name):
            return True

        if (self.secondary_all_partitions_cnt > 0) and (self.secondary_partitions_verified_by_bl2[-1] == partition_name):
            return True

        return False

    def find_partition_by_name(self, name):
        for p in self.partitions:
            if (name == p.partition_name):
                return p
        return None

    def create_normal_partitions(self):
        deduced_next_partition_offset = 0
        self.first_partition_offset = 0

        idx = -1
        for pdic in self.csv.dic_list:
            idx += 1
            p = Partition(self, idx, pdic, deduced_next_partition_offset)
            deduced_next_partition_offset = p.partition_offset + p.partition_size
            self.partitions.append(p)
            if (idx == 0):
                self.first_partition_offset = p.partition_offset

    def partition_name_to_idx(self, partition_name):
        idx = 0
        for p in self.partitions:
            if (partition_name == p.partition_name):
                return idx
            idx += 1
        return -1

    def create_all_partition(self, partition_name, child_partition_names):
        child_num = len(child_partition_names)
        if (child_num <= 0):
            return

        partition_name_1st = child_partition_names[0]
        partition_1st = self.find_partition_by_name(partition_name_1st)

        partition_name_last = child_partition_names[-1]
        partition_last = self.find_partition_by_name(partition_name_last)

        all_partition = copy.copy(partition_1st)

        child_partition_num = len(child_partition_names)
        self.prefer_primary_secondary_all = True

        all_partition.bin_size = 0
        all_partition.partition_name = partition_name
        all_partition.partition_size = (partition_last.partition_offset - partition_1st.partition_offset) + partition_last.partition_size

        if self.ota.is_overwrite():
            all_partition.Dbus_en = True

        if (partition_name == 'primary_all'):
            partition_hdr_pad_size = partition_1st.phy_partition_offset - partition_1st.partition_offset
            partition_tail_pad_size = (all_partition.partition_offset+all_partition.partition_size) - floor_align(all_partition.partition_offset+all_partition.partition_size,34)
            all_partition.partition_hdr_pad_size = partition_hdr_pad_size
            all_partition.partition_tail_pad_size = partition_tail_pad_size
            all_partition.phy_partition_size = all_partition.partition_size-partition_hdr_pad_size-partition_tail_pad_size
            all_partition.virtual_partition_size = phy2virtual(all_partition.phy_partition_size)
            all_partition.phy_code_size = all_partition.phy_partition_size - HDR_SZ - TAIL_SZ
            all_partition.vir_code_size = phy2virtual(all_partition.phy_code_size)

            if self.ota.is_overwrite():
                all_partition.virtual_partition_size = floor_align(all_partition.virtual_partition_size,4096)
                all_partition.vir_code_size = all_partition.virtual_partition_size - HDR_SZ - TAIL_SZ
                all_partition.phy_code_size = virtual2phy(all_partition.vir_code_size)
        else:
            partition_primary = self.find_partition_by_name('primary_all')
            all_partition.phy_partition_size = partition_primary.phy_partition_size
            all_partition.virtual_partition_size = partition_primary.virtual_partition_size

        with open("partition.bin", 'a+b') as f:
            # 24 bytes for name,16 bytes for data
            name = all_partition.partition_name.encode('utf-8')
            if len(name) < 24:
                name += bytes([0xFF] * (24 - len(name)))
            f.write(name)

            offset = struct.pack(">I",(all_partition.partition_offset))
            f.write(offset)

            size = struct.pack(">I",(all_partition.partition_size))
            f.write(size)

            offset = struct.pack(">I",(all_partition.vir_code_offset))
            f.write(offset)

            size = struct.pack(">I",(all_partition.vir_code_size))
            f.write(size)

        idx = self.partition_name_to_idx(partition_name_last)
        self.partitions.insert(idx + 1, all_partition)
        logging.debug(f'create virtual partition: {partition_name} id={idx}')
        logging.debug(f'create virtual partition: offset={all_partition.partition_offset}, size={all_partition.partition_size}')
        logging.debug(f'create virtual partition: virtual_partition_size={all_partition.virtual_partition_size}, virtual_code_size={all_partition.vir_code_size}')

    def create_all_partitions(self):
        self.create_all_partition('primary_all', self.primary_partitions_verified_by_bl2)
        self.create_all_partition('secondary_all', self.secondary_partitions_verified_by_bl2)

    def check_partition_illegal(self,strategy):
        if(strategy.upper() == 'OVERWRITE'):
            for p in self.partitions:
                if(p.partition_name.find("secondary") != -1):
                    logging.error("overwrite cannot have secondary partitions!")
                    exit(1)
            p = self.find_partition_by_name("ota")
            primary_all = self.find_partition_by_name("primary_all")
            if(primary_all and primary_all.partition_size % (4096) != 0):
                logging.error("total size of all primary partition should be 4k aligned!")
        elif(strategy.upper() == 'XIP' or strategy.upper() == 'SWAP'):
            # TODO
            logging.debug("TODO")
        else:
            logging.debug("No OTA")


    def create_ota_pack_list(self):
        self.check_partition_illegal(self.ota.get_strategy())
        if self.bl2_exists:
            if(self.ota.get_strategy() == "SWAP" or self.ota.get_strategy() == "XIP"):
                idx = self.partition_name_to_idx('secondary_all')
                self.ota_pack_idx_list.append(idx)
            else:
                idx = -1
                for p in self.partitions:
                    idx += 1
                    if(p.bin_is_code() and p.is_primary and not(p.is_bootloader_partition()) and p.partition_name != "primary_all"):
                        self.ota_pack_idx_list.append(idx)
                        logging.debug(f'OTA pack: add {p.partition_name} to ota pack list')
        elif self.bootloader_exists:
            idx = -1
            for p in self.partitions:
                idx += 1
                if p.partition_name.startswith('primary_') and (p.partition_name != 'primary_all') and (p.partition_name != 'primary_bootloader'):
                    self.ota_pack_idx_list.append(idx)
                    logging.debug(f'OTA pack: add {p.partition_name} to ota pack list')
        else:
            logging.debug(f'No bootloader/bl2, do not need create ota.bin')
        logging.debug(f'OTA pack partition list: {self.ota_pack_idx_list}')

    def create_app_pack_list(self):
        app_exclude = []
        if self.bl2_exists:
            app_exclude = self.primary_partitions_verified_by_bl2
        logging.debug(f'APP exclude partitions: {app_exclude}')
        idx = -1
        for p in self.partitions:
            idx += 1
            if ((p.partition_name not in app_exclude) and p.is_primary) or (p.partition_name == 'bl2') or (p.partition_name == 'primary_all') or (p.partition_name == 'bl1_control') or (p.partition_name == 'ota') or (p.partition_name == 'primary_manifest') or (p.partition_name == 'partition'):
                self.all_app_pack_idx_list.append(idx)
                logging.debug(f'APP pack {p.partition_name}')
        logging.debug(f'APP pack partition list: {self.all_app_pack_idx_list}')

    def create_pack_list(self):
        self.create_ota_pack_list()
        self.create_app_pack_list()

    def create_partitions(self):
        self.create_normal_partitions()
        self.create_all_partitions()

    def append_ota_partitions(self, partition_name):
        self.ota_partitions.append(partition_name)
        logging.debug(f'ota partitions: {self.ota_partitions}')

    def append_primary_partitions_verified_by_bl2(self, partition_name):
        self.primary_partitions_verified_by_bl2.append(partition_name)
        self.primary_all_partitions_cnt = len(self.primary_partitions_verified_by_bl2)
        logging.debug(f'primary partitions verified by bl2:')
        logging.debug(f'{self.primary_partitions_verified_by_bl2}')

    def append_secondary_partitions_verified_by_bl2(self, partition_name):
        self.secondary_partitions_verified_by_bl2.append(partition_name)
        self.secondary_all_partitions_cnt = len(self.secondary_partitions_verified_by_bl2)
        logging.debug(f'secondary partitions verified by bl2:')
        logging.debug(f'{self.secondary_partitions_verified_by_bl2}')

    def parse_and_validate_tfm_ns(self):
        self.tfm_ns_exists = False
        self.primary_cpu0_app_exists = False

        if 'primary_tfm_ns' in self.partition_names:
            self.tfm_ns_exists = True

        if 'primary_cpu0_app' in self.partition_names:
            self.primary_cpu0_app_exists = True

        logging.debug(f'tfm_ns exists={self.tfm_ns_exists}, primary_cpu0_app={self.primary_cpu0_app_exists}')

    def parse_and_validate_tfm_s(self):
        if self.tfm_exists:
            if ('sys_ps' not in self.partition_names) or ('sys_its' not in self.partition_names) or ('sys_otp_nv' not in self.partition_names):
                logging.error(f'TFM need partition "sys_ps", "sys_its"')
                exit(1)

    def parse_and_validate_cpu0_app(self):
        if self.primary_cpu0_app_exists:
            if ('sys_net' not in self.partition_names) or ('sys_rf' not in self.partition_names):
                logging.error(f'partition primary_cpu0_app need partition "sys_net", "sys_rf"')
                exit(1)

    def parse_and_validate_secureboot_partitions(self):
        logging.debug(f'secure is {self.secureboot_en}')
        if self.secureboot_en:
            if len(self.partition_names) < 2:
               logging.error(f'secureboot should contains "bl1_control", "primary_manifest"')
               exit(1)

            if (self.partition_names[0] != 'bl1_control') or (self.partition_names[1] != 'primary_manifest'):
                logging.error(f'first 2 partitions of secureboot should be "bl1_control", "primary_manifest" in order')
                exit(1)

    def check_duplicated_partitions(self):
        if (len(self.partition_names) != len(set(self.partition_names))):
            logging.error(f'{self.partition_csv} contains duplicated partitions!')
            exit(1)

    def parse_and_validate_partitions_before_create(self):
        logging.debug(f'before create validation')
        partitions_cnt = len(self.partition_names)
        if (partitions_cnt == 0):
            logging.error(f'{self.partition_csv} has no partition')
            exit(1)
        self.check_duplicated_partitions()
        self.parse_and_validate_secureboot_partitions()
        self.parse_and_validate_tfm_ns()
        self.parse_and_validate_tfm_s()
        self.parse_and_validate_cpu0_app()

    def parse_and_validate_partitions_after_create(self):
        logging.debug(f'after create validation')
        logging.debug(f'primary/secondary continously')
        logging.debug(f'primary/secondary in-order')
        logging.debug(f'at least one X partition for bl2')
        logging.debug(f'other partitions checking')

    def parse_and_validate_bl2_security(self):
        if self.bl2_exists:
            self.bl2_privkey = self.security.bl2_root_privkey
            self.bl2_pubkey = self.security.bl2_root_pubkey
            p = self.primary_partitions_verified_by_bl2[0]
            bin_name = p.replace('primary_', '') + '.bin'
            self.app_security_counter = self.ota.get_app_security_counter()
            logging.debug(f'bl2_privkey={self.bl2_privkey}, bl2_pubkey={self.bl2_pubkey}, app_security_counter={self.app_security_counter}')

    def parse_and_validate_partitions_after_build(self):
        logging.debug(f'after build validation')
        self.parse_and_validate_bl2_security()
        for p in self.partitions:
            p.parse_and_validate_after_build()

    def init_globals(self):
        self.flash_aes_en = self.security.flash_aes_en
        self.flash_aes_key = self.security.flash_aes_key
        self.secureboot_en = self.security.secureboot_en
        for p in self.csv.dic_list:
            self.partition_names.append(p['Name'])

        self.tfm_exists = False
        if 'primary_tfm_s' in self.partition_names:
            self.tfm_exists = True

        self.bl2_exists = False
        self.bl2_partition_idx = -1
        if 'bl2' in self.partition_names:
            self.bl2_exists = True
            self.bl2_partition_idx = self.partition_names.index('bl2')

        self.bootloader_exists = False
        if 'primary_bootloader' in self.partition_names:
            self.bootloader_exists = True

        logging.debug(f'secureboot_en={self.secureboot_en}, tfm_exists={self.tfm_exists}, bl2_exits={self.bl2_exists}, bl2_index={self.bl2_partition_idx}')
 
    def init_globals_default(self):
        self.partitions = []
        self.partition_names = []

        self.flash_base_addr = 0x02000000
        self.flash_size = 0x400000
        self.cpu_vector_align_bytes = 512
        self.need_crc = True
        self.primary_partitions_verified_by_bl2 = []
        self.secondary_partitions_verified_by_bl2 = []
        self.ota_pack_idx_list = []
        self.all_app_pack_idx_list = []
        self.ota_partitions = []
        self.secureboot = False
        self.ab_en = False
        self.gen_partitions_bin_done = False

    def __init__(self, tools_dir, partition_csv, security, ota):
        self.tools_dir = tools_dir
        self.partition_csv = partition_csv
        self.security = security
        self.ota = ota
        self.secondary_all_partitions_cnt = 0

        self.csv = Csv(partition_csv, True, partition_keys)
        self.init_globals_default()
        self.init_globals()
        self.parse_and_validate_partitions_before_create()
        self.create_partitions()
        self.parse_and_validate_partitions_after_create()
        self.create_pack_list()

    def gen_primary_secondary_all_bin(self, partition_name, child_partition_names):
        length = len(child_partition_names)
        if (length <= 0):
            logging.debug(f'Skip generate partition {partition_name}')
            return

        if (self.prefer_primary_secondary_all == False):
            return

        logging.debug(f'Generate {partition_name}, child partitions: {child_partition_names}')
        all_bin_name = f'{partition_name}_code.bin'
        with open(all_bin_name, 'wb+') as f:
            idx = -1
            for pname in child_partition_names:
                p = self.find_partition_by_name(pname)
                idx += 1
                if self.ota.is_overwrite():
                    if idx == 0:
                        pad_size = p.vir_code_offset - p.vir_partition_offset - HDR_SZ
                        start_offset = p.vir_code_offset - pad_size
                    else:
                        pad_size = p.vir_code_offset - start_offset -f.tell()
                else:
                    if idx == 0:
                        pad_size = p.phy_code_offset - p.phy_partition_offset - HDR_SZ
                        start_offset = p.phy_code_offset - pad_size
                    else:
                        pad_size = p.phy_code_offset - start_offset -f.tell()

                logging.debug(f'pad_size:{hex(pad_size)}')
                pad = bytes([0xFF]*(pad_size))
                f.write(pad)
                f.write(p.partition_buf)

        all_bin_signed_name = f'{partition_name}_code_signed.bin'
        all_bin_partition = self.find_partition_by_name(partition_name)
        if(self.ota.is_overwrite()):
            signing(all_bin_partition, all_bin_name, all_bin_signed_name, self.bl2_privkey, self.app_security_counter,'0.0.1',1)
        else:
            signing(all_bin_partition, all_bin_name, all_bin_signed_name, self.bl2_privkey, self.app_security_counter)

        all_bin_pack = all_bin_signed_name
        if(self.ota.is_overwrite()):
            if self.flash_aes_en:
                self.aes_bin_name = f'{partition_name}_code_aes.bin'
                aes_tool = f'{self.tools_dir}/beken_packager/beken_aes'
                start_address = hex(phy2virtual(all_bin_partition.phy_partition_offset))
                logging.debug(f'encrypt {partition_name}, startaddress={start_address}')
                cmd = f'{aes_tool} encrypt -infile {all_bin_signed_name} -keywords {self.flash_aes_key} -outfile {self.aes_bin_name} -startaddress {start_address}'
                run_cmd_not_check_ret(cmd)
            else:
                self.aes_bin_name = all_bin_signed_name
            # Add CRC
            self.crc_bin_name = f'{partition_name}_code_crc.bin'
            crc(self.aes_bin_name, self.crc_bin_name)
            all_bin_pack = self.crc_bin_name

            all_bin_partition.partition_buf = bytearray()
            with open(all_bin_pack, 'rb') as f:
                pad = bytes([0xFF]*(all_bin_partition.partition_hdr_pad_size))
                all_bin_partition.partition_buf += pad
                all_bin_partition.partition_buf += f.read()
        else:
            all_bin_partition.partition_buf = bytearray()
            with open(all_bin_pack, 'rb') as f:
                all_bin_partition.partition_buf += f.read()

    def copy_file(self, f1, f2, offset):
        logging.debug(f'Copy {f1} to {offset} of {f2}')
        buf = bytearray()
        with open(f1, 'rb') as f1_f:
            buf = f1_f.read()
            with open(f2, 'rb+') as f2_f:
                f2_f.seek(offset)
                f2_f.write(buf)

    def process_software_jump(self):
        if self.secureboot_en == False:
            return

        logging.debug(f'Copy MSP and PC from tfm_s.bin to bl1_control.bin')
        tfm_msp_pc = bytearray()
        with open('tfm_s.bin', 'rb') as f:
            tfm_msp_pc = f.read(64)
            with open('bl1_control.bin', 'rb+') as bl1_control_f:
                bl1_control_f.seek(0)
                bl1_control_f.write(tfm_msp_pc)

    def process_hardware_jump(self):
        if self.secureboot_en == False:
            return

        self.copy_file('jump.bin', 'bl1_control.bin', 0)
        self.copy_file('jump.bin', 'bl2.bin', 0x140)
        # TODO insert the jump.bin length to bl2.bin

    def process_bl1_boot(self):
        self.hardware_jump = False

        if self.hardware_jump == True:
            self.process_hardware_jump()
        else:
            self.process_software_jump()

    def gen_partitions_bin(self):
        if (self.gen_partitions_bin_done == True):
            return

        self.gen_partitions_bin_done = True

        for p in self.partitions:
            p.process_bin()

        self.gen_primary_secondary_all_bin('primary_all', self.primary_partitions_verified_by_bl2)
        self.gen_primary_secondary_all_bin('secondary_all', self.secondary_partitions_verified_by_bl2)

    def bin_need_to_bypass_in_bl2_merged_bin(self, name):
        p = self.find_partition_by_name(name)
        if (p.bin_verifier != "bl2"):
            return False

        m1 = pattern_match(name, r"primary_")
        m2 = pattern_match(name, r"secondary_")
        if (m1 == False) and (m2 == False):
            return False

        if (self.prefer_primary_secondary_all == True):
            if (name == 'primary_all') or (name == 'secondary_all'):
                return False
            else:
                return True
        else:
            if (name == 'primary_all') or (name == 'secondary_all'):
                return True
            else:
                return False
 
    def gen_merged_partitions_bin(self, bin_name, partition_idx_list):
        f = open(bin_name, 'wb+')
        for idx in partition_idx_list:
            p = self.partitions[idx]
            if (p.bin_size > 0):
                logging.debug(f'merge {bin_name}: {p.partition_name} seek={hex(p.partition_offset)} len={hex(len(p.partition_buf))}')
                #TO Be Modifyed
                if(p.bin_is_code() and not p.partition_name == "primary_all"):
                    pad_size = p.phy_code_offset - f.tell()
                else:
                    pad_size = p.partition_offset - f.tell()
                pad = bytes([0xFF]*(pad_size))
                f.write(pad)
                f.write(p.partition_buf)
        f.flush()
        f.close()

    def gen_provisioning_bin(self):
        provisioning_idx_list = [0, 1, 2, 3]
        self.gen_merged_partitions_bin('bootloader.bin', provisioning_idx_list)

    def gen_all_apps_noboot_bin(self):
        bin_name = 'all-app.bin'
        f = open(bin_name, 'wb+')
        p = self.find_partition_by_name('primary_all')
        f.write(p.partition_buf)
        f.flush()
        f.close()

    def gen_all_apps_bin(self):
        if (self.secureboot_en == True):
            self.gen_all_apps_noboot_bin()
        else:
            if (len(self.all_app_pack_idx_list) > 0):
                self.gen_merged_partitions_bin('all-app.bin', self.all_app_pack_idx_list)

    def gen_write_file(self, f, offset, value, len):
        logging.debug(f'write file: offset=%x, len={len}' %(offset))
        f.seek(offset)
        f.write(value)
        return (offset + len)

    def init_crc32_table(self):
        self.crc32_table = []

        for i in range(0,256):
            c = i
            for j in range(0,8):
                if c&1:
                    c = 0xEDB88320 ^ (c >> 1)
                else:
                    c = c >> 1
            self.crc32_table.append(c)

    def crc32(self, crc, buf):
        for c in buf:
            crc = (crc>>8) ^ self.crc32_table[(crc^c)&0xff]
        return crc

    def gen_ota_global_hdr(self, img_num, img_hdr_list, global_security_counter):
        magic = "BK723658".encode()
        magic = struct.pack('8s', magic)
        global_security_counter = struct.pack('<I', global_security_counter)
        hdr_len = struct.pack('<H', GLOBAL_HDR_LEN)
        img_num = struct.pack('<H', img_num)
        flags = struct.pack('<I', 0)
        reserved1 = struct.pack('<I', 0)
        reserved2 = struct.pack('<I', 0)
        global_crc_content = global_security_counter + hdr_len + img_num + flags + reserved1 + reserved2
        for img_hdr in img_hdr_list:
            global_crc_content += img_hdr

        self.init_crc32_table()
        global_crc = self.crc32(0xffffffff, global_crc_content)
        global_crc = struct.pack('<I', global_crc)
        ota_global_hdr = magic + global_crc + global_security_counter + hdr_len + img_num + flags + reserved1 + reserved2
        logging.debug(f'add OTA global hdr: magic={magic}, img_num={img_num}, security_counter={global_security_counter}, flags={flags}, crc={global_crc}')
        return ota_global_hdr

    def gen_ota_img_hdr(self, partition_name, img_offset, flash_offset, img_content, security_counter, img_flags):
        logging.debug(f'add ota img hdr of {partition_name}: img_offset=%x, flash_offset=%x, security_counter=%x, img_flags=%x'
            %(img_offset, flash_offset, security_counter, img_flags))
        img_offset = struct.pack('<I', img_offset)
        flash_offset = struct.pack('<I', flash_offset)
        security_counter = struct.pack('<I', security_counter)

        img_len = len(img_content)
        img_len = struct.pack('<I', img_len)

        self.init_crc32_table()
        checksum = self.crc32(0xffffffff,img_content)
        checksum = struct.pack('<I', checksum)
        flags = 0
        flags = struct.pack('<I', img_flags)
        reserved1 = 0
        reserved1 = struct.pack('<I', reserved1)
        reserved2 = 0
        reserved2 = struct.pack('<I', reserved2)
        hdr = img_len + img_offset + flash_offset + checksum + security_counter + flags + reserved1 + reserved2
        return hdr 

    def gen_ota_raw_bin(self):
        if self.bootloader_exists == False:
            return

        with open('app_pack.bin', 'wb+') as f:
            offset = 0
            partition_virtual_size = 0
            for idx in self.ota_pack_idx_list:
                f.seek(offset)
                p = self.partitions[idx]
                with open(p.bin_name, 'rb') as pf:
                    f.write(pf.read())
                partition_virtual_size = int((p.partition_size / 34)*32)
                offset += partition_virtual_size

    def gen_ota_bin(self):
        if(self.ota.get_strategy() == "SWAP" or self.ota.get_strategy() == "XIP"):
            self.gen_ota_bin_for_xip_swap()
        elif(self.ota.get_strategy() == "OVERWRITE"):
            self.gen_ota_bin_for_overwrite()

    def gen_ota_bin_for_xip_swap(self):
        self.gen_ota_raw_bin()
        img_num = len(self.ota_pack_idx_list)
        if (img_num == 0):
            return

        self.gen_partitions_bin()
        pack_file_name = 'ota.bin'

        offset = (IMG_HDR_LEN * img_num) + GLOBAL_HDR_LEN
        logging.debug(f'pack ota: img num={img_num}, total header len=%x' %(offset))

        img_hdr_list = []
        for idx in self.ota_pack_idx_list:
            partition = self.partitions[idx]
            img_hdr = self.gen_ota_img_hdr(partition.partition_name, img_offset=offset, flash_offset=partition.partition_offset,
                img_content=partition.partition_buf, security_counter=self.ota.get_app_security_counter(), img_flags=0)
            offset = offset + partition.partition_size
            img_hdr_list.append(img_hdr)

        ota_global_hdr = self.gen_ota_global_hdr(img_num=img_num,img_hdr_list=img_hdr_list, global_security_counter=0)

        offset = 0
        with open(pack_file_name, 'wb+') as f:
            f.seek(offset)
            f.write(ota_global_hdr)
            offset += GLOBAL_HDR_LEN

            for img_hdr in img_hdr_list:
                f.seek(offset)
                f.write(img_hdr)
                offset += IMG_HDR_LEN

            for idx in self.ota_pack_idx_list:
                partition = self.partitions[idx]
                if (partition.bin_size > 0):
                    f.seek(offset)
                    f.write(partition.partition_buf)
                logging.debug(f'start to append partitions{idx}: offset=%x, partition_size=%x, bin_size=%x, bin_tail_size=%x'
                    %(offset, partition.partition_size, partition.bin_size, partition.bin_tail_size))
                offset += partition.partition_size

    def gen_overwrite_compress_bin(self,in_file_name):
        compress_size_list = []
        self.compress_bin = 'compress.bin'
        compress_temp_in = 'temp_before_compress'
        compress_temp_out = 'temp_after_compress'
        file_size = os.path.getsize(in_file_name)
        with open(in_file_name,'rb') as src,open(self.compress_bin,'w+b') as dst:
            offset = 2 * math.floor(file_size/COMPRESS_BLOCK_SZ) + 4 # 2 uint16_t for after and before block size
            logging.debug(f'block num = {offset //2 - 1}')
            dst.seek(offset)
            sum = 0
            file_in = open(compress_temp_in,"wb+")
            file_out = open(compress_temp_out,"wb+")
            src.seek(0)
            idx = 0
            while True:
                uncompress_block_size = bytes()
                chunk = bytes() # clear chunk
                chunk = src.read(COMPRESS_BLOCK_SZ)
                if(len(chunk) != COMPRESS_BLOCK_SZ):
                    uncompress_block_size = struct.pack("H",len(chunk))
                if not chunk:
                    break
                file_in.seek(0)
                file_in.write(chunk)
                compress_tool = f'{self.tools_dir}/beken_packager/lzma'
                cmd =f'{compress_tool} e {compress_temp_in} {compress_temp_out}'
                run_cmd(cmd)
                chunk = bytes() # clear chunk
                file_out.seek(0)
                chunk = file_out.read()
                compress_chunk_size = len(chunk)
                logging.debug(f'block after size:{compress_chunk_size}')
                compress_chunk_size = struct.pack("H",compress_chunk_size)
                compress_size = compress_chunk_size + uncompress_block_size
                compress_size_list.append(compress_size)
                dst.write(chunk)
                sum += len(chunk)
            file_in.close()
            file_out.close()
            offset = 0
            dst.seek(offset)
            for num in compress_size_list:
                dst.write(num)

    def gen_ota_bin_for_overwrite(self):
        # Compress
        self.gen_overwrite_compress_bin("primary_all_code_signed.bin")

        # Sign
        ota_sign_bin = 'ota_partition_signed.bin'
        ota_partition = self.find_partition_by_name("ota")
        signing(ota_partition, self.compress_bin, ota_sign_bin, self.bl2_privkey, self.app_security_counter)
        with open(ota_sign_bin,'rb') as f:
            ota_partition.partition_buf = f.read()

        # Add OTA Hdr
        ota_bin = "ota.bin"
        img_num = 1
        img_hdr_list = []
        with open(ota_sign_bin,'rb+') as src,open(ota_bin,'w+b') as dst:
            offset = (IMG_HDR_LEN * img_num) + GLOBAL_HDR_LEN
            dst.seek(offset)
            img_content = src.read()
            dst.write(img_content)

            img_hdr = self.gen_ota_img_hdr( ota_partition.partition_name, img_offset = 0, 
                                                    flash_offset=ota_partition.partition_offset,
                                                    img_content=img_content, 
                                                    security_counter=self.ota.get_app_security_counter(),img_flags=0)
            img_hdr_list.append(img_hdr)
            ota_global_hdr = self.gen_ota_global_hdr(img_num=img_num, img_hdr_list=img_hdr_list, global_security_counter=0)
            offset = 0
            dst.seek(offset)
            dst.write(ota_global_hdr)

            offset = GLOBAL_HDR_LEN
            for img_hdr in img_hdr_list:
                dst.seek(offset)
                dst.write(img_hdr)
                offset += IMG_HDR_LEN

    def gen_bin(self):
        self.gen_partitions_bin()
        self.gen_ota_bin()
        self.gen_all_apps_bin()
        if (self.secureboot_en == True):
            self.gen_provisioning_bin()
