#!/usr/bin/env python3

import logging
import struct

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
    def __gen_ota_global_hdr(self, img_num, img_hdr_list, global_security_counter):
        magic = "BK723658".encode()
        magic = struct.pack('8s', magic)
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
        if pheader.type().is_merge_overwrite() == False:
            return '_' + infile

        inner_signed_file = '_' + infile
        compressed_file = infile[:-4] + '_compress.bin'
        compress_bin(inner_signed_file, compressed_file)

        outer_signed_file = compressed_file[:4] + '_signed.bin'
        ota_sign_size = pheader.m2()
        ota_partition_offset = pheader.m1()
        logging.debug(f'OTA outer sign offset={ota_partition_offset}, size={ota_sign_size}')
        pk = Pubkey(img_sign_pubkey)
        bl2_sign('sign', 'ec256', img_sign_privkey, pk.key_bytes(), None, compressed_file, ota_sign_size, pheader.version(), pheader.security_counter(), outer_signed_file, None)
        return outer_signed_file

    def __get_img_buf(self, bname):
        with open(bname, 'rb+') as f:
            return f.read()

    def __get_img_flash_offset(self, ph):
        if ph.type().is_merge_overwrite():
            return ph.m1()
        else:
            return ph.size().offset()

    def __encrypt_and_add_crc(self, ph, bname, flash_aes_key, flash_crc_en):
        if (ph.flags().ota_write_cbus() == False) and (ph.flags().ota_read_cbus() == True):
            aes_bin_name = bname[:-4] + '_aes.bin'
            if flash_aes_key != None:
                aes_tool = get_flash_aes_tool()
                if flash_crc_en:
                    start_address = hex(phy2virtual(ota_partition.phy_partition_offset))
                else:
                    start_address = hex(ota_partition.phy_partition_offset)
                cmd = f'python3 {aes_tool} encrypt -infile {ota_sign_bin} -keywords {self.aes_key}  -aes {128} -outfile {aes_bin_name} -startaddress {start_address}'
                run_cmd_not_check_ret(cmd)
            else:
                aes_bin_name = bname

            if flash_crc_en == True:
                crc_bin_name = aes_bin_name[:-4] + '_crc.bin'
                crc(aes_bin_name, crc_bin_name)
            else:
                crc_bin_name = aes_bin_name

            return crc_bin_name
        else:
            return bname

    def __get_img_flags(self, ph):
        if ph.type().is_merge_overwrite():
            return OTA_IMG_FLAG_OW
        else:
            return OTA_IMG_FLAG_XIP

    def _pack_prepare(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        logging.debug(f'PackOta: bins={self._bins}, flash_aes_key={flash_aes_key} data_aes_key={data_aes_key}')
        img_num = len(self._bins)
        if img_num == 0:
            return

        self._img_hdrs = []
        self._img_bufs = []
        img_offset = (OTA_IMG_HDR_LEN * img_num) + OTA_GLOBAL_HDR_LEN

        for b in self._bins:
            ph = PackHeader(b)
            compress_sign_img_file = self.__compress_and_sign(ph, b, img_sign_pubkey, img_sign_privkey)
            crc_img_file = self.__encrypt_and_add_crc(ph, compress_sign_img_file, flash_aes_key, flash_crc_en)
            img_buf = self.__get_img_buf(crc_img_file)
            flash_offset = self.__get_img_flash_offset(ph)
            img_flags = self.__get_img_flags(ph)
            img_hdr = self.__gen_ota_img_hdr(ph.name(), img_offset=img_offset, 
                          flash_offset=flash_offset, img_content=img_buf, 
                          security_counter=ph.security_counter(), img_flags=img_flags)
            self._img_hdrs.append(img_hdr) 
            self._img_bufs.append(img_buf) 

        self._ota_global_hdr = self.__gen_ota_global_hdr(img_num=img_num, img_hdr_list=self._img_hdrs, global_security_counter=0)

    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        self._pack_prepare(img_sign_pubkey, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key)
        with open(self._outfile, 'w+b') as f:
            for buf in self._img_bufs:
                f.write(buf)
