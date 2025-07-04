#!/usr/bin/env python3

import logging
from .pack import Pack
from .pack_json import PackJson
from .security import Security
from .partitions import Partitions

class PackAll(Pack):

    def pack(self):
        s = Security('security.csv')

        if s.is_flash_aes_fixed() == False:
            flash_aes_key = None
        else:
            flash_aes_key = s.flash_aes_key

        p = Partitions('partitions.csv', 'bin.csv', 'pack.json')
        p.postbuild_process()
        pack = PackJson()
        pack.pack('pack.json', s.img_sign_pubkey, s.img_sign_privkey, flash_aes_key, s.flash_crc_en, None)

