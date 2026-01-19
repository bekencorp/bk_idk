#!/usr/bin/env python3

import os
import json
import logging
import struct

from .common import *
from .partition import Partition
from .bin import Bin

class PartitionBootFlag(Partition):

    def __init__(self, idx=None, field_dic=None, mini_offset=None, partitions=None):
        super().__init__(idx, field_dic, mini_offset)
        self._bin = Bin('boot_flag.bin', 'boot_flag', self.name())

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        self.__create_boot_flag_bin(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en)
        super().postbuild_process()

    def __create_boot_flag_bin(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True):

        aes_type = "FIXED"
        boot_flag_val = bytes([0x63,0x54,0x72,0x4c,0x1])
        bin_name = 'boot_flag.bin'
        with open(bin_name, 'wb+') as f:
            logging.debug(f'create new {bin_name}')
            pad = bytes([0x0]*(0x20))
            f.write(pad)
            f.seek(32)
            pad = bytes([0xFF]*(0xFE0))
            f.write(pad)

        with open(bin_name, 'rb+') as f:
            f.seek(0)
            f.write(boot_flag_val)
            if aes_type == 'RANDOM':
                with open("jump.bin", 'rb') as jump_f:
                    jump_data = jump_f.read(8)
                    f.seek(8)
                    f.write(jump_data)