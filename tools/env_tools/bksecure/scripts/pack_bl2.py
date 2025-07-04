#!/usr/bin/env python3

import logging
from .common import *
from .pack import Pack

class PackBl2(Pack):
    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        for bin in self._bins:
            raw_bin = '_' + bin
            logging.debug(f"pack raw bl2 bin:{raw_bin}")
            calc_crc16(raw_bin, self._outfile)