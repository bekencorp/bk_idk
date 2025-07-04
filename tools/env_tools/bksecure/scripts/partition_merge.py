#!/usr/bin/env python3

import os
import json
import logging
import struct

from .common import *
from .partition_app import PartitionApp
from .partition_str_type import PartitionStrType
from .size_crc import SizeCrc
from .size_cbus import SizeCbus
from .bin import Bin

class PartitionMerge(PartitionApp):

    def __init__(self, partitions=[], idx=0, merge_idx=0):
        if len(partitions) == 0:
            logging.error(f'PartitionMerge: empty partition list')
            exit(1)

        names = ''
        for p in partitions:
            names += f' {p.name()}'
        logging.debug(f'Create merge partition{merge_idx}: {names}')

        self._partitions = partitions
        self._ota_partition = None
        self._idx = idx
        self.__init_basic_info()

    def __init_basic_info(self):
        first = self._partitions[0]
        last = self._partitions[-1]

        t = first.type()
        if t.is_ow_active():
            self._name = 'overwrite'
            subtype = 'merge_overwrite'
        elif t.is_xip_a():
            self._name = 'xip_a'
            subtype = 'merge_xip_a'
        elif t.is_xip_b():
            self._name = 'xip_b'
            subtype = 'merge_xip_b'
        else:
            logging.error(f'PartitionMerge: invalid partition type={t.type()}')
            exit(1)

        if first.bin() != None:
            self._bin = Bin(f'{self.name()}.bin', self.name(), self.name(), first.bin().version(), first.bin().security_counter())
        else:
            self._bin = None
        self._type = PartitionStrType('app', subtype)
        self._flags = first.flags()

        self._int_offset = first.size().offset()
        self._int_size = last.size().offset() - first.size().offset() + last.size().size()
        self._mini_offset = self._int_offset

        if self.flags().download_write_cbus():
            self._size = SizeCbus(self._int_offset, self._int_size, BL2_HDR_SZ)
        else:
            self._size = SizeCrc(self._int_offset, self._int_size, BL2_HDR_SZ)

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True):
        self.__create_merge_bin(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en)
        p = self._ota_partition
        if p != None:
            offset = p.size().offset()
            size = p.size().size()
        else:
            offset = 0
            size = 0

        self._add_pack_header_to_bin(offset, size)

    def set_ota_partition(self, ota_partition):
        self._ota_partition = ota_partition

    def __create_merge_bin(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True):
        '''Generate the merged bin for BL2 signing'''

        if self.bin() == None:
            return

        bname = f'{self.name()}.bin'
        version = self.bin().version()
        logging.debug(f'Create merged bin: name={bname}, version={version}')

        parts = version.split('.')
        iv_major = struct.pack('B', int(parts[0]))
        iv_minor = struct.pack('B', int(parts[1]))
        iv_revision = struct.pack('<H', int(parts[2]))

        chip = struct.pack('<6s','BK7236'.encode('utf-8')) #TODO chip-independent
        with open(bname, 'wb+') as f:
            idx = -1
            for p in self._partitions:
                idx += 1
                if idx == 0:
                    pad_size = self.size().vir_code_offset() - self.size().vir_offset() - BL2_HDR_SZ
                    start_offset = self.size().vir_code_offset() - pad_size
                else:
                    pad_size = p.size().vir_code_offset() - start_offset - f.tell()

                logging.debug(f'PartitionMerge: merge bin {p.bin().name()}, pad_size:{hex(pad_size)}, start_offet={hex(start_offset)}')
                pad = bytes([0xFF]*(pad_size))
                f.write(pad)

                with open(p.bin().name(), 'rb') as pf:
                    f.write(pf.read())

            f.write(iv_major)
            f.write(iv_minor)
            f.write(iv_revision)
            f.write(chip)
