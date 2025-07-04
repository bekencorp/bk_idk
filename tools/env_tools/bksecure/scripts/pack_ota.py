#!/usr/bin/env python3

import logging
import struct

from .pack import Pack
from .pack_header import PackHeader
from .pack_raw_ota import PackRawOta
from .compress import compress_bin
from .bl2_sign import bl2_sign
from .pubkey import Pubkey
from .crc import pack_crc32
from .flash_crc import crc

OTA_GLOBAL_HDR_LEN = 32
OTA_IMG_HDR_LEN = 32

class PackOta(PackRawOta):
    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        self._pack_prepare(img_sign_pubkey, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key)
        with open(self._outfile, 'w+b') as f:
            f.write(self._ota_global_hdr)
            for hdr in self._img_hdrs:
                f.write(hdr)

            for buf in self._img_bufs:
                f.write(buf)
