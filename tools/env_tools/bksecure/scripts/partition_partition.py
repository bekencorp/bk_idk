#!/usr/bin/env python3

import os
import json
import logging
import struct

from .common import *
from .partition import Partition
from .bin import Bin

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
                if len(name) < 20:
                    name += bytes([0xFF] * (20 - len(name)))
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

        with open('partition.bin','wb') as f,open('partition_raw.bin','rb') as f_src:
            f.write(f_src.read())
            f.flush()
            f.close()
