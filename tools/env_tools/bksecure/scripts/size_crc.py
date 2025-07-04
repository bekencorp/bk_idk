#!/usr/bin/env python3

import logging

from .common import *
from .size import Size

'''
+------------------------+  <--- offset
|                        |
|                        |
+------------------------+  <--- phy_offset
|  HDR (plain data)      |
|  (bin_hdr_size)        |  
|                        |
+------------------------+  <--- bin_code_partition_offset
|  vector/CRC aligned pad|
|                        |
+------------------------+  <--- bin_1st_instruction_offset
|  Code (CRC/Encrypted)  |
|                        |
|                        |
+------------------------+
|  Unused                |
|                        |
+------------------------+  <--- bin_tail_offset
|  TAIL (plain data)     |
|  (bin_tail_size)       |
|                        |
+------------------------+  <--- partition end
'''


class SizeCrc(Size):

    def __init__(self, offset=0, size=0, hdr_size=0):
        super().__init__(offset, size, hdr_size)
        self._cal_offset()
        self._cal_size()

    def _cal_offset(self):
        self._phy_offset = ceil_align(self.offset(), CRC_UNIT_TOTAL_SZ)
        self._vir_offset = phy2virtual(self._phy_offset)
        self._vir_code_offset = ceil_align(self._vir_offset + self._hdr_size, CPU_VECTOR_ALIGN_SZ)
        self._phy_code_offset = virtual2phy(self._vir_code_offset)

    def _cal_size(self):
        self._hdr_pad_size = self._phy_offset - self.offset()
        tail_pad_size = (self.offset() + self.size()) - floor_align(self.offset() + self.size(), 34)
        self._phy_size = self.size() - self._hdr_pad_size - tail_pad_size
        self._vir_size = phy2virtual(self._phy_size)
        self._phy_code_size = self._phy_size - (self._phy_code_offset - self._phy_offset)
        self._vir_code_size = phy2virtual(self._phy_code_size)

        #TODO double check it, may need to align last sub-partition to 4K also
        self._vir_sign_size = floor_align(self._vir_size, 4096)

    def phy_offset(self):
        return self._phy_offset

    def vir_offset(self):
        return self._vir_offset

    def phy_code_offset(self):
        return self._phy_code_offset

    def vir_code_offset(self):
        return self._vir_code_offset

    def phy_size(self):
        return self._phy_size

    def vir_size(self):
        return self._vir_size

    def phy_code_size(self):
        return self._phy_code_size

    def vir_code_size(self):
        return self._vir_code_size

    def sign_size(self):
        return self._vir_sign_size

    def hdr_pad_size(self):
        return self._hdr_pad_size

    def tail_pad_size(self):
        return self._tail_pad_size

    def hdr_size(self):
        return self._hdr_size



