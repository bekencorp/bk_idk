#!/usr/bin/env python3

import logging
import os
import shutil
import struct

from .common import *
from .security import Security
from .ota import OTA

OTA_SIGNED_BIN = 'ota_signed.bin'
from .pack import Pack
from .pack_header import PackHeader
from .compress import compress_bin
from .bl2_sign import bl2_sign
from .pubkey import Pubkey
from .crc import pack_crc32
from .flash_crc import crc

OTA_GLOBAL_HDR_LEN = 32
OTA_IMG_HDR_LEN = 32

#TODO auto gen flags macro
OTA_IMG_FLAG_OW =  (1<<16)
OTA_IMG_FLAG_XIP = (1<<17)

class PackRawOta(Pack):

    def __init__(self, bins=None, outfile=None):
        super().__init__(bins, outfile)
        if os.path.exists('ota.csv'):
            o = OTA('ota.csv')
            self.ota_encrypt_en = o.get_encrypt()
            self.ota_flash_crc_en = o.get_flash_crc_en()
        else:
            self.ota_encrypt_en = False
            self.ota_flash_crc_en = False
        if os.path.exists('security.csv'):
            s = Security('security.csv')
            self.flash_crc_en = s.flash_crc_en
            self.flash_aes_key = s.flash_aes_key if s.is_flash_aes_fixed() else None
        else:
            self.flash_crc_en = True
            self.flash_aes_key = None
        self._ota_magic = 'OTA ENC ' if self.ota_encrypt_en else 'BK723658'

    def __gen_ota_global_hdr(self, img_num, img_hdr_list, global_security_counter):
        magic = struct.pack('8s', self._ota_magic.encode())
        global_security_counter = struct.pack('<I', global_security_counter)
        hdr_len = struct.pack('<H', OTA_GLOBAL_HDR_LEN)
        img_num = struct.pack('<H', img_num)
        flags = struct.pack('<I', 0)
        reserved1 = struct.pack('<I', 0)
        reserved2 = struct.pack('<I', 0)
        global_crc_content = global_security_counter + hdr_len + img_num + flags + reserved1 + reserved2
        for img_hdr in img_hdr_list:
            global_crc_content += img_hdr

        global_crc = pack_crc32(0xffffffff, global_crc_content)
        global_crc = struct.pack('<I', global_crc)
        ota_global_hdr = magic + global_crc + global_security_counter + hdr_len + img_num + flags + reserved1 + reserved2
        logging.debug(f'add OTA global hdr: magic={magic}, img_num={img_num}, security_counter={global_security_counter}, flags={flags}, crc={global_crc}')
        return ota_global_hdr

    def __gen_ota_img_hdr(self, partition_name, img_offset, flash_offset, img_content, security_counter, img_flags):
        logging.debug(f'add ota img hdr of {partition_name}: img_offset=%x, flash_offset=%x, security_counter=%x, img_flags=%x'
            %(img_offset, flash_offset, security_counter, img_flags))
        img_offset = struct.pack('<I', img_offset)
        flash_offset = struct.pack('<I', flash_offset)
        security_counter = struct.pack('<I', security_counter)

        img_len = len(img_content)
        img_len = struct.pack('<I', img_len)

        checksum = pack_crc32(0xffffffff,img_content)
        checksum = struct.pack('<I', checksum)
        flags = 0
        flags = struct.pack('<I', img_flags)
        reserved1 = 0
        reserved1 = struct.pack('<I', reserved1)
        reserved2 = 0
        reserved2 = struct.pack('<I', reserved2)
        hdr = img_len + img_offset + flash_offset + checksum + security_counter + flags + reserved1 + reserved2
        return hdr 

    def __compress_and_sign(self, pheader, infile, img_sign_pubkey=None, img_sign_privkey=None):
        inner_signed_file = '_' + infile
        if not os.path.isfile(inner_signed_file):
            logging.error(f'Inner signed image not found: {inner_signed_file}')
            exit(1)
        if pheader.type().is_merge_overwrite() == False:
            if self.ota_encrypt_en:
                if not os.path.exists(img_sign_pubkey) or not os.path.exists(img_sign_privkey):
                    logging.error('img_sign_pubkey and img_sign_privkey are required for encrypted OTA')
                    exit(1)
                sign_input = self.__encrypt_ota_image_with_aes_xts(
                    pheader, inner_signed_file, self.flash_aes_key, self.flash_crc_en,
                    body_offset=0, out_ext='_enc.bin')
                pk = Pubkey(img_sign_pubkey)
                ota_outer_sign_size = ceil_align(os.path.getsize(sign_input) + BL2_HDR_SZ + BL2_TAIL_SZ, FLASH_SECTOR_SZ)
                bl2_sign(
                    'sign', 'ec256', img_sign_privkey, pk.key_bytes(), None,
                    sign_input, ota_outer_sign_size, pheader.version(), pheader.security_counter(),
                    OTA_SIGNED_BIN, None, pad_to_slot=False,
                )
                logging.debug(f'PackRawOta: encrypted {inner_signed_file} then signed with dynamic outer slot {ota_outer_sign_size:#x} -> {OTA_SIGNED_BIN}')
            else:
                shutil.copy2(inner_signed_file, OTA_SIGNED_BIN)
                logging.debug(f'PackRawOta: copied {inner_signed_file} -> {OTA_SIGNED_BIN}')
        else:
            if not os.path.exists(img_sign_pubkey) or not os.path.exists(img_sign_privkey):
                logging.error(f'img_sign_pubkey and img_sign_privkey are required for merge overwrite')
                exit(1)
            compressed_file = inner_signed_file[:-4] + '_compress.bin'
            compress_bin(inner_signed_file, compressed_file, self.ota_flash_crc_en)

            sign_input = compressed_file
            if self.ota_encrypt_en:
                sign_input = self.__encrypt_ota_image_with_aes_xts(
                    pheader, compressed_file, self.flash_aes_key, self.flash_crc_en,
                    out_ext='_enc.bin')

            ota_sign_size = pheader.m2()
            ota_partition_offset = pheader.m1()
            logging.debug(f'OTA outer sign offset={ota_partition_offset}, size={ota_sign_size}')
            pk = Pubkey(img_sign_pubkey)
            bl2_sign(
                'sign', 'ec256', img_sign_privkey, pk.key_bytes(), None,
                sign_input, ota_sign_size, pheader.version(), pheader.security_counter(),
                OTA_SIGNED_BIN, None,
            )
        return OTA_SIGNED_BIN

    def __get_img_buf(self, bname):
        with open(bname, 'rb+') as f:
            return f.read()

    def __get_img_flash_offset(self, ph):
        if ph.type().is_merge_overwrite():
            return ph.m1()
        else:
            return ph.size().offset()

    def __get_encrypt_start_address(self, ph, flash_crc_en):
        if ph.type().is_merge_overwrite():
            offset = ph.m1()
        elif ph.type().is_merge():
            offset = ph.size().phy_offset()
        else:
            offset = ph.size().offset()
        if flash_crc_en:
            return hex(phy2virtual(offset))
        return hex(offset)

    def __get_aes_mode(self, flash_aes_key):
        key_len = len(flash_aes_key) // 2
        if key_len == 32:
            return '128'
        if key_len == 64:
            return '256'
        logging.error(f'invalid flash aes key length: {key_len} bytes')
        exit(1)

    def __encrypt_ota_image_with_aes_xts(self, ph, bname, flash_aes_key, flash_crc_en,
                                         body_offset=BL2_HDR_SZ, out_ext='_aes.bin'):
        if flash_aes_key is None:
            logging.error('ota_image_encrypt_en is TRUE but flash_aes_key is missing')
            exit(1)

        aes_bin_name = bname[:-4] + out_ext
        aes_tool = get_flash_aes_tool()
        start_address = hex(int(self.__get_encrypt_start_address(ph, flash_crc_en), 0) + body_offset)
        aes_mode = self.__get_aes_mode(flash_aes_key)
        cmd = (
            f'python3 {aes_tool} encrypt -infile {bname} -keywords {flash_aes_key} '
            f'-aes {aes_mode} -outfile {aes_bin_name} -startaddress {start_address}'
        )
        logging.info(f'OTA image encrypt: {cmd}')
        run_cmd(cmd)
        return aes_bin_name

    def __get_img_flags(self, ph):
        if ph.type().is_merge_overwrite():
            return OTA_IMG_FLAG_OW
        else:
            return OTA_IMG_FLAG_XIP

    def sign(self, img_sign_pubkey=None, img_sign_privkey=None, **kwargs):
        logging.debug(f'PackRawOta sign: bins={self._bins}')
        img_num = len(self._bins)
        if img_num == 0:
            return
        ph = PackHeader(self._bins[0])
        self.__compress_and_sign(ph, self._bins[0], img_sign_pubkey, img_sign_privkey)

    def _pack_encrypt_build_hdr(self, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        logging.debug(f'PackRawOta pack encrypt/hdr: bins={self._bins}, flash_aes_key={flash_aes_key}')
        img_num = len(self._bins)
        if img_num == 0:
            return

        self._img_hdrs = []
        self._img_bufs = []
        img_offset = (OTA_IMG_HDR_LEN * img_num) + OTA_GLOBAL_HDR_LEN
        
        ph = PackHeader(self._bins[0])
        ota_sign_file = OTA_SIGNED_BIN
        if not os.path.exists(ota_sign_file):
            logging.error(
                f'OTA payload not found: {ota_sign_file} '
                f'(run sign on pack host or copy from sign host)'
            )
            exit(1)

        img_buf = self.__get_img_buf(ota_sign_file)
        flash_offset = self.__get_img_flash_offset(ph)
        img_flags = self.__get_img_flags(ph)
        img_hdr = self.__gen_ota_img_hdr(ph.name(), img_offset=img_offset, 
                        flash_offset=flash_offset, img_content=img_buf, 
                        security_counter=ph.security_counter(), img_flags=img_flags)
        self._img_hdrs.append(img_hdr) 
        self._img_bufs.append(img_buf) 

        self._ota_global_hdr = self.__gen_ota_global_hdr(img_num=img_num, img_hdr_list=self._img_hdrs, global_security_counter=0)

    def pack(self, flash_aes_key=None, flash_crc_en=True, data_aes_key=None, flash_aes_type=None):
        self._pack_encrypt_build_hdr(flash_aes_key, flash_crc_en, data_aes_key)
        with open(self._outfile, 'w+b') as f:
            for buf in self._img_bufs:
                f.write(buf)
                
