#!/usr/bin/env python3

import logging

from .common import *
from .size_crc import SizeCrc

class SizeCbus(SizeCrc):

    def _cal_offset(self):
        self._phy_offset = self.offset()
        self._vir_offset = phy2virtual(self._phy_offset)
        phy_min_code_offset = self._phy_offset + hdr_size
        vir_min_code_offset = phy2virtual(phy_min_code_offset)
        self._vir_code_offset = ceil_align(vir_min_code_offset, CPU_VECTOR_ALIGN_SZ)
        self._phy_code_offset = virtual2phy(self._vir_code_offset)
