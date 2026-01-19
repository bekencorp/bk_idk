#!/usr/bin/env python3

import os
import json
import logging
import struct

from .common import *
from .partition_app import PartitionApp
from .bin import Bin

class PartitionAppPadding(PartitionApp):

    def __init__(self, idx=None, field_dic=None, mini_offset=None):
        super().__init__(idx, field_dic, mini_offset)
        self._bin = Bin(f'{self.name()}_4k.bin', 'padding', self.name())

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        self.__create_padding_bin()
        super().postbuild_process()

    def __create_padding_bin(self):
        with open(f'{self.name()}_4k.bin', 'wb') as f:
            f.write(bytes([0xFF]*0x1000))
