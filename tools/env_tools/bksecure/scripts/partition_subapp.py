#!/usr/bin/env python3

import os
import json
import logging

from .common import *
from .partition_app import PartitionApp

class PartitionSubapp(PartitionApp):

    def __init__(self, idx=None, field_dic=None, mini_offset=None, hdr_size=0, bin=None):
        super().__init__(idx, field_dic, mini_offset, hdr_size, bin)

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        '''No need to add pack header for each binary'''
        pass
