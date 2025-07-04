#!/usr/bin/env python3

import logging

from .pack import Pack
from .pack_header import PackHeader
from .pubkey import Pubkey
from .bl2_sign import bl2_sign

class PackBl2sign(Pack):

    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        l = len(self._bins)
        if l != 1:
            logging.error(f'Bin number should be 1, actual={l}')
            exit(1)

        pk = Pubkey(img_sign_pubkey)
        ph = PackHeader(self._bins[0])
        name = ph.name()
        infile = f'_{name}.bin'

        bl2_sign('sign', 'ec256', img_sign_privkey, pk.key_bytes(), None, infile, ph.size().sign_size(), ph.version(), ph.security_counter(), self._outfile, None)

        p = PackHeader()
        t = ph.type()
        p.create_header(self._outfile, ph.version(), ph.name(), ph.type().type(),
            ph.type().subtype(), ph.size().offset(), ph.size().size(), ph.flags().flags(), 0, ph.m1(), ph.m2(), ph.m3(), ph.m4())
