#!/usr/bin/env python3

import logging
import os
import json
import shutil

from .pack import Pack
from .pack_bl2 import PackBl2
from .pack_bl2_sign import PackBl2sign
from .pack_bl1_sign import PackBl1sign
from .pack_ota import PackOta
from .pack_raw_ota import PackRawOta
from .pack_download import PackDownload
from .pack_bl2_download import PackBl2Download
from .pack_nvs import PackNvs
from .steps_pack_header import StepsPackHeader
from .pack_user_mfr import PackUserMfr
from .partitions import Partitions

class PackJson(Pack):

    def __init__(self):
        self._packs = []

    def pack(self, pack_json='pack.json', img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None, flash_aes_type=None):
        logging.info(f'Pack: json={pack_json}, flash_aes_type={flash_aes_type}, flash_crc_en={flash_crc_en}')
        # Ensure nvs_key.bin and nvs.bin are generated before processing ==========
        # Reference: xnf_connect_1226/tools/env_tools/beken_utils/scripts/partition.py process_partition_nvs_key() and process_partition_nvs()
        # Check if USER_MFR_AES action or nvs.bin is needed, if so, ensure nvs_key.bin and nvs.bin are generated first
        # Get flash_aes_type from security.csv if it exists
        self.__ensure_nvs_bins_generated(pack_json, flash_aes_type, flash_aes_key, flash_crc_en)
        self.__create_pack(pack_json)
        self.__pack(img_sign_pubkey, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key, flash_aes_type)
        
    def __ensure_nvs_bins_generated(self, pack_json, flash_aes_type=None, flash_aes_key=None, flash_crc_en=True):
        """Ensure nvs_key.bin, nvs_key_pack.bin and nvs.bin are generated before processing"""
        if not os.path.exists(pack_json):
            return
        
        # Check if USER_MFR_AES action, nvs.bin, or nvs_key_pack.bin is needed in pack.json
        json_data = None
        with open(pack_json, 'r') as f:
            json_data = json.load(f)
        
        has_user_mfr_aes = False
        needs_nvs_bin = False
        needs_nvs_key_pack = False
        for k in json_data:
            v = json_data[k]
            action = v.get('action', '')
            bins = v.get('bin', [])
            
            if action == 'USER_MFR_AES':
                has_user_mfr_aes = True
            
            # Check if nvs.bin or nvs_key_pack.bin is referenced in any bin list
            if isinstance(bins, list):
                if 'nvs.bin' in bins or 'nvs_aes.bin' in bins:
                    needs_nvs_bin = True
                if 'nvs_key_pack.bin' in bins:
                    needs_nvs_key_pack = True
        
        # If USER_MFR_AES action, nvs.bin, or nvs_key_pack.bin is needed, ensure nvs partitions are processed
        if has_user_mfr_aes or needs_nvs_bin or needs_nvs_key_pack:
            if os.path.exists('partitions.csv'):
                try:
                    # Create Partitions object (this will parse partitions.csv and create PartitionNvsKey/PartitionNvs if they exist)
                    p = Partitions('partitions.csv', None, None)
                    # Call postbuild_process to generate nvs_key.bin, nvs_key_pack.bin and nvs.bin
                    p.postbuild_process(flash_aes_en=False, flash_aes_key=flash_aes_key, img_sign_key=None, flash_crc_en=flash_crc_en, flash_aes_type=flash_aes_type)
                    logging.debug(f'PackJson: processed nvs partitions, nvs_key.bin and nvs.bin should be generated now')
                except Exception as e:
                    logging.warning(f'PackJson: failed to process nvs partitions: {e}')
                    # Continue anyway, PackUserMfr/PackNvs will check and report error if bins don't exist
            else:
                logging.warning(f'PackJson: partitions.csv not found, cannot generate nvs bins automatically')

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
            elif action == "STEPS_PACK_HEADER":
                p = StepsPackHeader(bins, k)
            elif action == 'USER_MFR_AES':
                p = PackUserMfr(bins, k)
            else:
                logging.error(f'Unsupported pack action={action}')
                exit(1)

            self._packs.append(p) 

    def __pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None, flash_aes_type=None):
        for p in self._packs:
            pack_type = type(p).__name__
            outfile = getattr(p, '_outfile', 'N/A')
            bins = getattr(p, '_bins', [])
            logging.info(f'Pack: {outfile} ({pack_type})')
            logging.debug(f'Pack: bins={bins}')
            
            # Pass flash_aes_type to pack methods that support it
            if hasattr(p, 'pack'):
                import inspect
                sig = inspect.signature(p.pack)
                supports_flash_aes_type = 'flash_aes_type' in sig.parameters
                
                if supports_flash_aes_type:
                    p.pack(img_sign_pubkey, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key, flash_aes_type)
                else:
                    p.pack(img_sign_pubkey, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key)

    def copy_file(self, file_name, dst):
        if os.path.exists(file_name):
            shutil.copy(file_name, f'{dst}/{file_name}')

    def install_bin(self):
        logging.debug(f"install bins")
        install_dir = 'install'
        os.makedirs(install_dir, exist_ok=True)
        self.copy_file('all-app.bin', install_dir)
        self.copy_file('ota.bin', install_dir)
        self.copy_file('bootloader.bin', install_dir)
        self.copy_file('otp_efuse_config.json', install_dir)
