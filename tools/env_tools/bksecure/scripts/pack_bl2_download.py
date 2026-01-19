#!/usr/bin/env python3

import logging

from .pack_download import PackDownload

class PackBl2Download(PackDownload):

    def __init__(self, bins=None, outfile=None):
        super().__init__(bins, outfile)
        self._magic = 'BL2DLV10'
