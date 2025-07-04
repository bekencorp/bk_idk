#!/usr/bin/env python3

import os
import json
import logging
import csv
import struct

from .common import *
from .parse_csv import *
from .partition import Partition
from .partition_merge import PartitionMerge
from .partition_app import PartitionApp
from .partition_subapp import PartitionSubapp
from .partition_partition import PartitionPartition
from .partition_padding import PartitionPadding
from .partition_app_padding import PartitionAppPadding
from .gen_license import *
from .bin import Bin

OTA_CONFIRM = 0xA16D8FB0

class Partitions:

    def __init__(self, partition_csv, bin_csv, pack_json):
        logging.debug(f'Partitions: init {type(self)}')
        self._bins = self.__parse_bin_csv(bin_csv)
        self.__parse_partition_csv(partition_csv)
        self._pack_json = pack_json

        self.__common_process()

    def __validate_duplication(self):
        names = []
        for p in self._partitions:
            names.append(p.name())

        if (len(names) != len(set(names))):
            logging.error(f'partitions.csv contains duplicated partitions!')
            exit(1)

    def __validate_isolation_boundary(self):
        pre_flags = None
        pre_partition = None

        for p in self._partitions:
            flags = p.flags()
            if (pre_flags != None) and (flags.cbus_secure() != pre_flags.cbus_secure()):
                offset = p.size().offset()

                if (offset % (68<<10)) != 0:
                    block = offset // (68<<10)
                    suggest_boundary = (block + 1) * (68 << 10)
                    diff = suggest_boundary - offset
                    logging.error(f'The partition {pre_partition.name()} and {p.name()} is NOT in 68K boundary!')
                    logging.error(f'{p.name()} start {offset/1024}k, suggest start {suggest_boundary/1024}k')
                    size_k = pre_partition.size().size() /1024
                    suggest_size_k = (size_k + diff)/1024
                    logging.error(f'Suggest to change {pre_partition.name()} size from {size_k}k to {suggest_size_k}k')
                    exit(1)

            pre_partition = p;
            pre_flags = flags
 
    def __validate_bin(self):
        for b in self._bins:
            p = self._find_partition_by_name(b.partition())
            if p == None:
                logging.error(f'Partition {b.partition()} not exists')
                exit(1)

    def __get_bin(self, partition):
        for b in self._bins:
            if b.partition() == partition:
                return b

        return None

    def __validate(self):
        self.__validate_duplication()
        self.__validate_isolation_boundary()
        self.__validate_bin()

    def __create_merged_partitions(self):

        pre_type = None
        merge_idx = 0
        partitions = []

        ow_active = None
        ow_ota = None
        for p in self._partitions:
            type = p.type()

            if type.is_ow_ota():
                ow_ota = p

            if (pre_type != None) and (pre_type.subtype() != type.subtype()):
                if len(partitions) != 0:
                    idx = len(self._partitions)
                    merged_partition = PartitionMerge(partitions, idx, merge_idx)
                    self._partitions.append(merged_partition)
                    merge_idx = merge_idx + 1
                    partitions = []

                    if pre_type.is_ow_active():
                        ow_active = merged_partition

            if type.is_subapp():
                partitions.append(p)    
                pre_type = type
            else:
                pre_type = None

        if ow_active != None:
            ow_active.set_ota_partition(ow_ota)
        
    def __parse_bin_csv(self, bin_csv):
        logging.debug(f'Partitions: __parse_bin_csv')

        if bin_csv == None:
            return []

        bin_keys = ['Name', 'Type', 'Partition', 'Version', 'Security_counter']
        csv = Csv(bin_csv, True, bin_keys)

        bins = []
        print(f'{csv.dic_list}')
        for dic in csv.dic_list:
            b = Bin(dic['Name'], dic['Type'], dic['Partition'], dic['Version'], dic['Security_counter'])
            bins.append(b)

        return bins

    def __parse_partition_csv(self, partition_csv):
        logging.debug(f'Partitions: __parse_partition_csv')

        if partition_csv == None:
            return []

        mini_offset = 0
        partition_keys_v2 = ['Name', 'Type', 'SubType', 'Offset', 'Size', 'Flags']
        partition_keys_v1 = ['Name', 'Offset', 'Size', 'Execute', 'Read', 'Write' ]
        csv = Csv(partition_csv, True, partition_keys_v2, partition_keys_v1)

        subapp_idx_dic = {
            "ow_active" : 0,
            "xip_a" : 0,
            "xip_b" : 0
        }

        idx = -1
        self._partitions = []
        for pdic in csv.dic_list:
            idx += 1
            type = pdic['Type'].strip()
            subtype = pdic['SubType'].strip()
            b = self.__get_bin(pdic['Name'].strip())

            if (type == 'data') and (subtype == 'partition'):
                p = PartitionPartition(idx, pdic, mini_offset, self._partitions)
            elif (type == 'data') and ((subtype == 'ow_ota_control') or (subtype == 'xip_ota_control')):
                p = PartitionPadding(idx, pdic, mini_offset)
            elif (type == 'app') and ((subtype == 'ow_ota') or (subtype == 'xip_b')):
                p = PartitionAppPadding(idx, pdic, mini_offset)
            elif (type == 'app') and ((subtype == 'ow_active') or (subtype == 'xip_a') or (subtype == 'xip_b')):
                subapp_idx_dic[subtype] += 1
                if subapp_idx_dic[subtype] == 1:
                    # Only the first partition need to add BL2 signed header
                    p = PartitionSubapp(idx, pdic, mini_offset, BL2_HDR_SZ, b)
                else:
                    p = PartitionSubapp(idx, pdic, mini_offset, 0, b)
            elif (type == 'app'):
                p = PartitionApp(idx, pdic, mini_offset, 0, b)
            elif (type == 'data'):
                p = Partition(idx, pdic, mini_offset)
            else:
                logging.error(f'Unsupported partition type={type}, subtype={subtype}')
                exit(1)

            mini_offset = p.size().offset() + p.size().size()
            self._partitions.append(p)

    def _find_partition_by_name(self, name):
        for p in self._partitions:
            if (name == p.name()):
                return p
        return None

    def __has_overwrite(self):
        for p in self._partitions:
            if p.type().is_ow_active():
                return True
        return False

    def __has_xip(self):
        for p in self._partitions:
            if p.type().is_xip_a():
                return True
        return False

    def __common_process(self):
        logging.debug(f'Partitions: common process')
        self.__validate()
        self.__create_merged_partitions()

    def __bin_partition_info_to_bin(self):
        print(f'__bin_partition_info_to_bin')

    def __gen_code(self, fname):
        f = open(fname, 'w+')
    
        logging.debug(f'Create partition hdr file: {fname}')
        f.write(get_license())
    
        line = f'#include "security.h"\r\n'
        f.write(line)
     
        line = f'#define %-45s %s' %("KB(size)", "((size) << 10)\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("MB(size)", "((size) << 20)\r\n\r\n")
        f.write(line)

        macro_name = f'CPU_VECTOR_ALIGN_SZ'
        line = f'#define %-45s %d\r\n' %(macro_name, 512)
        f.write(line)

        macro_name = f'CONFIG_OTA_OVERWRITE'
        if self.__has_overwrite():
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
            f.write(line)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
            f.write(line)

        macro_name = f'OVERWRITE_CONFIRM'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, OTA_CONFIRM)
        f.write(line)

        macro_name = f'CONFIG_DIRECT_XIP'
        if self.__has_xip():
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
            f.write(line)

        f.write(line)
        macro_name = f'XIP_SET'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, OTA_CONFIRM)
        f.write(line)
        macro_name = f'XIP_MAGIC_TYPE'
        line = f'#define %-45s %d\r\n' %(macro_name, 1)
        f.write(line)
        macro_name = f'XIP_COPY_DONE_TYPE'
        line = f'#define %-45s %d\r\n' %(macro_name, 2)
        f.write(line)
        macro_name = f'XIP_IMAGE_OK_TYPE'
        line = f'#define %-45s %d\r\n' %(macro_name, 3)
        f.write(line)
        macro_name = f'XIP_IMAGE_TEST_FIRST'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFFFFFF00)
        f.write(line)
        macro_name = f'XIP_IMAGE_TEST_SECOND'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFFFF0000)
        f.write(line)
        macro_name = f'XIP_IMAGE_TEST_FINAL'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFF000000)
        f.write(line)
        macro_name = f'XIP_IMAGE_OK'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0x0)
        f.write(line)
        macro_name = f'XIP_IMAGE_UNKNOWN'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFFFFFFFF)
        f.write(line)

        macros = ''
        struct_array = f"#define PARTITION_MAP {{ \\\r\n"
        for p in self._partitions:
            macros += p.gen_code() + '\r\n'
            name = p.name()
            struct_array +=  f"    {{BK_FLASH_EMBEDDED, \"{name}\""
            name = name.upper()
            name = name.replace(' ', '_')

            subtype_options = p.flags().flags() + (p.type().subtype() << 24)
            struct_array += f""", CONFIG_{name}_PHY_PARTITION_OFFSET, CONFIG_{name}_PHY_PARTITION_SIZE, {hex(subtype_options)}}}, \\\r\n"""
 
        struct_array += f"}}\r\n"
        f.write(macros)
        f.write(struct_array)
        f.flush()
        f.close()

    def prebuild_process(self):

        for p in self._partitions:
            p.prebuild_process()
        self.__gen_code('partitions_partition.h')

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True):
        for p in self._partitions:
            p.postbuild_process(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en)
