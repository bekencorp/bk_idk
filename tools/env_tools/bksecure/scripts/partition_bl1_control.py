#!/usr/bin/env python3

import os
import json
import logging
import struct

from .common import *
from .partition import Partition
from .bin import Bin

class PartitionBl1Control(Partition):

    def __init__(self, idx=None, field_dic=None, mini_offset=None, partitions=None):
        super().__init__(idx, field_dic, mini_offset)
        self._bin = Bin('bl1_control.bin', 'bl1_control', self.name())

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        self.__create_bl1_control_bin(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en)
        super().postbuild_process()

    def __create_bl1_control_bin(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True):

        aes_type = 'FIXED' # TODO: change to parameter
        boot_ota = False # TODO: change to parameter

        bin_name = 'bl1_control.bin'

        with open(bin_name, 'wb') as f:
            pad = bytes([0xFF]*(0xF00))
            f.write(pad)

        if aes_type == 'RANDOM':
            logging.debug(f'Copy jump_bin to {bin_name}')
            with open(bin_name, 'rb+') as bl1_control_f:
                with open("jump.bin", 'rb') as jump_f:
                    p.msp_pc = jump_f.read(8)
                    jump_f.seek(0)
                    jump_data = jump_f.read()
                    bl1_control_f.write(jump_data)
                logging.debug(f'Copy MSP and PC from provision.bin to {bin_name}')
                privison_msp_pc = bytearray()
                with open('provision.bin', 'rb') as f:
                    privison_msp_pc = f.read(8)
                    bl1_control_f.seek(0)
                    bl1_control_f.write(privison_msp_pc)
        else:
            if boot_ota == False:
                logging.debug(f'Copy MSP and PC from bl2.bin to {bin_name}')
                bl2_msp_pc = bytearray()
                with open('bl2.bin', 'rb') as f:
                    bl2_msp_pc = f.read(64)
                    with open(bin_name, 'rb+') as bl1_control_f:
                        bl1_control_f.seek(0)
                        bl1_control_f.write(bl2_msp_pc)
            else:
                if os.path.exists("bl2_B.bin"):
                    logging.debug(f'Copy jump_bin to {bin_name}')
                    with open("jump.bin", 'rb') as jump_f:
                        jump_data = jump_f.read()
                        with open(bin_name, 'rb+') as bl1_control_f:
                            bl1_control_f.write(jump_data)
