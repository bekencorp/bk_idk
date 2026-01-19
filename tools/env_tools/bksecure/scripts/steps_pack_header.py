#!/usr/bin/env python3

import logging
from .common import *
from .pack import Pack
from .pack_header import PackHeader

class StepsPackHeader(Pack):
    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        pack_manifest = self._bins[0]
        ph = PackHeader(pack_manifest)
        p = PackHeader()
        p.create_header(self._outfile, ph.version(), ph.name(), ph.type().type(),
            ph.type().subtype(), ph.size().offset(), ph.size().size(), ph.flags().flags(), 0, ph.m1(), ph.m2(), ph.m3(), ph.m4())