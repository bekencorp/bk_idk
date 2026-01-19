#!/usr/bin/env python3

import os
import json
import logging
import struct

from .common import *
from .partition import Partition
from .bin import Bin

class PartitionManifest(Partition):
    # generate empty manifest bin for pack header
    # empty bin used in pack_bl1_sign.py, and cannot share name of this empty bin

    def __init__(self, idx=None, field_dic=None, mini_offset=None):
        super().__init__(idx, field_dic, mini_offset)
        self._bin = Bin(f'pack_{self.name()}.bin', 'padding', self.name())

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        self.__create_padding_bin()
        super().postbuild_process()

    def __create_padding_bin(self):
        with open(f'pack_{self.name()}.bin', 'wb') as f:
            f.write(bytes([0xFF]*self.size().size()))