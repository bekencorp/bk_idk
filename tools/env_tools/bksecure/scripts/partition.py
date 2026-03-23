#!/usr/bin/env python3

import os
import json
import logging

from .common import *
from .partition_str_flags import PartitionStrFlags
from .partition_str_type import PartitionStrType
from .pack_header import PackHeader
from .size import Size

class Partition:
    def __init__(self, idx=None, field_dic=None, mini_offset=None):
        logging.debug(f'Partition: init {type(self)}')
        self._idx = idx
        self._mini_offset = mini_offset
        self.__parse_field_dic(field_dic)
        self._flags = PartitionStrFlags(self._str_flags)
        self._type = PartitionStrType(self._str_type, self._str_subtype)
        self._bin = None
        self.__validate()
        self.__dump_partition()

    def idx(self):
        return self._idx

    def name(self):
        return self._name

    def size(self):
        return self._size

    def type(self):
        return self._type

    def flags(self):
        return self._flags

    def gen_code(self):
        lines = ''
        name = self.name()
        name = name.upper()
        name = name.replace(' ', '_')

        macro_name = f'CONFIG_{name}_PHY_PARTITION_OFFSET'
        lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().offset())
        macro_name = f'CONFIG_{name}_PHY_PARTITION_SIZE'
        lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().size())
        return lines

    def prebuild_process(self):
        pass

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        self._add_pack_header_to_bin()

    def _add_pack_header_to_bin(self, m1=0, m2=0, m3=0, m4=0):
        if self.bin() == None:
            return

        p = PackHeader()
        size = self.size().size()
        if self.name() == 'bl2':
            try:
                bin_obj = self.bin()
                bin_path = bin_obj.name() if bin_obj is not None else None
                if bin_path and os.path.exists(bin_path):
                    size = os.path.getsize(bin_path)
                else:
                    logging.warning('bl2 bin file not found, fallback to partition size')
            except Exception as e:
                logging.warning(f'failed to get bl2 bin size, fallback to partition size: {e}')
        p.create_header(self.bin().name(), self.bin().version(), self.name(), self.type().type(),
            self.type().subtype(), self.size().offset(), size, self.flags().flags(), 0, m1, m2, m3, m4)

    def bin(self):
        return self._bin

    def __validate(self):
        self.__validate_offset()
        self.__validate_size()

    def __validate_size(self):
        if (self.__is_sector_aligned(self.size().size()) == False):
            logging.error(f'{self.name()} size=%x not FLASH sector aligned' %(self.size().size()))
            exit(1)

    def __validate_offset(self):
        if (self.idx() == 0) and (self.size().offset() > 0):
            logging.error(f'first partition offset should be 0')
            exit(1)

        if self.size().offset() < self._mini_offset:
            logging.error(f'{self.name()} offset=%x overlapped' %(self.size().offset()))
            exit(1)

        if (self.__is_sector_aligned(self.size().offset()) == False):
                logging.error(f'{self.name()} offset=%x not FLASH sector aligned' %(self.size().offset()))
                exit(1)

    def __dump_partition(self):
        logging.debug(f'idx = {self._idx, self._name, self._str_type, self._str_subtype, self.size().offset(), self._mini_offset, self.size().size()}')

    def __parse_field_dic(self, field_dic):
        self._name = field_dic['Name']

        if field_dic['Offset'] == '':
            offset = self._mini_offset
        else:
            offset = hex2int(field_dic['Offset'])

        size = size2int(field_dic['Size'])
        self._size = Size(offset, size, 0)
        self._int_size = size
        self._int_offset = offset

        if 'Type' in field_dic:
            self._str_type = field_dic['Type']
            self._str_subtype = field_dic['SubType']
            self._str_flags = field_dic['Flags']
        else:
            logging.debug(f'Partition csv v1.0')
            self._str_subtype = ''
            self._str_flags = None
            if field_dic['Execute'] == 'TRUE':
                self._str_type = 'app'
            else:
                self._str_type = 'data'

    def __is_sector_aligned(self, addr):
        if ((addr % 0x1000) == 0):
            return True
        else:
            return False
