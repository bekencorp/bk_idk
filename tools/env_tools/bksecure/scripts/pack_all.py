#!/usr/bin/env python3

import logging
from .pack import Pack
from .pack_json import PackJson
from .security import Security
from .partitions import Partitions
from .gen_otp import *

class PackAll(Pack):

    def pack(self):
        s = Security('security.csv')

        if s.is_flash_aes_fixed() == False:
            flash_aes_key = None
        else:
            flash_aes_key = s.flash_aes_key

        p = Partitions('partitions.csv', 'bin.csv', 'pack.json')
        p.postbuild_process(flash_aes_en=False, flash_aes_key=flash_aes_key, img_sign_key=None, flash_crc_en=s.flash_crc_en, flash_aes_type=s.flash_aes_type)
        pack = PackJson()
        pack.pack('pack.json', s.img_sign_pubkey, s.img_sign_privkey, flash_aes_key, s.flash_crc_en, None, s.flash_aes_type)
        gen_otp_efuse_config_file(s.flash_aes_type, s.flash_crc_en, s.flash_aes_key, s.img_sign_pubkey, s.bl1_secureboot_en, False, 'otp_efuse_config.json')