#!/usr/bin/env python3

import os
import json
import logging
import struct

from .common import *
from .crc import make_crc32_table, pack_crc32
from .partition import Partition
from .bin import Bin

PARTITION_NAME_LEN = 32

class PartitionPartition(Partition):

    def __init__(self, idx=None, field_dic=None, mini_offset=None, partitions=None):
        super().__init__(idx, field_dic, mini_offset)
        self._partitions = partitions
        self._bin = Bin('partition.bin', 'PARTITION', self.name())

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        self.__create_partition_bin(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en)
        super().postbuild_process()

    def __create_partition_bin(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True):

        with open("partition_raw.bin", 'wb') as f:

            if os.path.exists('ppc_config.bin'):
                logging.debug(f'copy ppc_config.bin to partition_raw.bin')
                with open('ppc_config.bin', 'rb') as f_src:
                    f.write(f_src.read())
            else:
                f.write(bytes([0xFF]*1024))

            for p in self._partitions:
                name = p.name().encode('utf-8')
                if len(name) > PARTITION_NAME_LEN:
                    logging.warning(
                        'partition name truncated to %d bytes: %r',
                        PARTITION_NAME_LEN, p.name())
                    name = name[:PARTITION_NAME_LEN]
                elif len(name) < PARTITION_NAME_LEN:
                    name += bytes([0xFF] * (PARTITION_NAME_LEN - len(name)))
                f.write(name)

                _type = struct.pack(">B",(p.type().type()))
                f.write(_type)

                subtype = struct.pack(">B",(p.type().subtype()))
                f.write(subtype)

                offset = struct.pack(">I",(p.size().offset()))
                f.write(offset)
    
                size = struct.pack(">I",(p.size().size()))
                f.write(size)

                flags = struct.pack(">H",(p.flags().flags()))
                f.write(flags)

        with open('partition_raw.bin', 'rb') as f_src:
            payload = f_src.read()
        make_crc32_table()
        crc = (pack_crc32(0xFFFFFFFF, payload) ^ 0xFFFFFFFF) & 0xFFFFFFFF
        with open('partition.bin', 'wb') as f:
            f.write(payload)
            f.write(struct.pack('<I', crc))
            f.flush()
