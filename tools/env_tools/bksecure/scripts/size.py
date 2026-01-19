#!/usr/bin/env python3

import os
import json
import logging
import re
from .common import floor_align

class Size:

    def __init__(self, offset=0, size=0, hdr_size=0):
        self._offset = offset
        self._size = size
        self._hdr_size = hdr_size
        # In no_crc mode, calculate code offsets using 1:1 mapping
        self._cal_offset()
        self._cal_size()

    def _cal_offset(self):
        """Calculate offsets for no_crc mode (1:1 mapping)"""
        # In no_crc mode: physical = virtual (1:1 mapping)
        self._phy_offset = self._offset
        self._vir_offset = self._phy_offset  # 1:1 mapping
        # Code offset is offset + hdr_size (no alignment needed in no_crc mode)
        self._vir_code_offset = self._vir_offset + self._hdr_size
        self._phy_code_offset = self._vir_code_offset  # 1:1 mapping

    def _cal_size(self):
        """Calculate sizes for no_crc mode (1:1 mapping)"""
        # Sizes are also 1:1 mapped
        self._phy_size = self._size
        self._vir_size = self._phy_size  # 1:1 mapping
        self._phy_code_size = self._phy_size - (self._phy_code_offset - self._phy_offset)
        self._vir_code_size = self._phy_code_size  # 1:1 mapping
        # In no_crc mode, no padding needed
        self._hdr_pad_size = 0
        self._tail_pad_size = 0
        # Sign size: align virtual size to 4K
        self._vir_sign_size = floor_align(self._vir_size, 4096)

    def offset(self):
        return self._offset

    def size(self):
        return self._size

    def hdr_size(self):
        return self._hdr_size

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
