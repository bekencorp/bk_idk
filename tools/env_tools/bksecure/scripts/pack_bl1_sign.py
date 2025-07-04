#!/usr/bin/env python3

import logging

from .pack import Pack

class PackBl1sign(Pack):

    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        logging.debug(f'PackNvs: bins={bins}, flash_aes_key={flash_aes_key} data_aes_key={data_aes_key}')
