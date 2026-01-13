#!/usr/bin/env python3

import os
import json
import logging

from .common import *
from .partition import Partition
from .size_crc import SizeCrc
from .size import Size

class PartitionApp(Partition):

    def __init__(self, idx=None, field_dic=None, mini_offset=None, hdr_size=0, bin=None):
        super().__init__(idx, field_dic, mini_offset)
        if is_flash_crc_enabled():
            self._size = SizeCrc(self._int_offset, self._int_size, hdr_size)
        else:
            self._size = Size(self._int_offset, self._int_size, hdr_size)
        self._bin = bin

    def gen_code(self):
        lines = super().gen_code()
        name = self.name()
        name = name.upper()
        name = name.replace(' ', '_')
        macro_name = f'CONFIG_{name}_PHY_CODE_START'
        lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().phy_code_offset())

        macro_name = f'CONFIG_{name}_VIRTUAL_PARTITION_SIZE'
        lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().vir_size())

        macro_name = f'CONFIG_{name}_VIRTUAL_CODE_START'
        lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().vir_code_offset())

        macro_name = f'CONFIG_{name}_VIRTUAL_CODE_SIZE'
        lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().vir_code_size())

        if (self._bin != None) and (self._bin.type().upper() != name) and (self._bin.is_padding() == False):
            lines += '\r\n'
            lines += f'//Linker address and size to build {self._bin.name()}\r\n'

            macro_name = f'CONFIG_PRIMARY_{self._bin.type()}_PHY_PARTITION_OFFSET'
            lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().offset())
            macro_name = f'CONFIG_PRIMARY_{self._bin.type()}_PHY_PARTITION_SIZE'
            lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().size())
 
            macro_name = f'CONFIG_PRIMARY_{self._bin.type()}_VIRTUAL_CODE_START'
            lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().vir_code_offset())
    
            macro_name = f'CONFIG_PRIMARY_{self._bin.type()}_VIRTUAL_CODE_SIZE'
            lines += f'#define %-45s 0x%x\r\n' %(macro_name, self.size().vir_code_size())

        if self.type().is_merge_xip_a() and (self.flags().ota_write_cbus() == False) and self.flags().ota_read_cbus():
            macro_name = f'CONFIG_OTA_ENCRYPTED'
            lines += f'\r\n#define %-45s 0x%x\r\n' %(macro_name, 1)


        return lines
