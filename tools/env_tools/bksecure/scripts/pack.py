#!/usr/bin/env python3

from abc import ABC

class Pack(ABC):

    def __init__(self, bins=None, outfile=None):
        self._bins = bins
        self._outfile = outfile

    def sign(self, img_sign_pubkey=None, img_sign_privkey=None, **kwargs):
        pass

    def pack(self, flash_aes_key=None, flash_crc_en=True, data_aes_key=None, flash_aes_type=None):
        pass
