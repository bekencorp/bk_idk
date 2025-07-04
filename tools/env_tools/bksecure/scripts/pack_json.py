#!/usr/bin/env python3

import logging
import os
import json

from .pack import Pack
from .pack_bl2 import PackBl2
from .pack_bl2_sign import PackBl2sign
from .pack_bl1_sign import PackBl1sign
from .pack_ota import PackOta
from .pack_raw_ota import PackRawOta
from .pack_download import PackDownload
from .pack_bl2_download import PackBl2Download
from .pack_nvs import PackNvs

class PackJson(Pack):

    def __init__(self):
        self._packs = []

    def pack(self, pack_json='pack.json', img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        print(f'pack json: json={pack_json}, pubkey={img_sign_pubkey} privkey={img_sign_privkey}, flash_aes_key={flash_aes_key}, data_aes_key={data_aes_key}')
        self.__create_pack(pack_json)
        self.__pack(img_sign_pubkey, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key)
        
    def __create_pack(self, pack_json):

        if not os.path.exists(pack_json):
            return

        json_data = None
        with open(pack_json, 'r') as f:
            json_data = json.load(f)

        for k in json_data:
            v = json_data[k]
            bins = v['bin']
            action = v['action']

            if action == 'BL1_SIGN':
                p = PackBl1sign(bins, k)
            elif action == 'BL2_SIGN':
                p = PackBl2sign(bins, k)
            elif action == 'PACK_BL2_BIN':
                p = PackBl2(bins, k)
            elif action == 'PACK_BL2_DOWNLOAD_BIN':
                p = PackBl2Download(bins, k)
            elif action == 'PACK_BL1_DOWNLOAD_BIN':
                p = PackDownload(bins, k)
            elif action == 'PACK_OTA_BIN':
                p = PackOta(bins, k)
            elif action == 'PACK_RAW_OTA_BIN':
                p = PackRawOta(bins, k)
            elif action == 'PACK_NVS':
                p = PackNvs(bins, k)
            else:
                logging.error(f'Unsupported pack action={action}')
                exit(1)

            self._packs.append(p)
 

    def __pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):

        for p in self._packs:
            p.pack(img_sign_pubkey, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key)
