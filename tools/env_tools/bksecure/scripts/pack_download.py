#!/usr/bin/env python3

import logging
import struct

from .pack import Pack
from .pack_header import PackHeader
from .common import *
from .crc import pack_crc32

DOWNLOAD_GLOBAL_HDR_LEN = 32
DOWNLOAD_IMG_HDR_LEN = 32

class PackDownload(Pack):

    def __init__(self, bins=None, outfile=None):
        super().__init__(bins, outfile)
        self._magic = 'BL1DLV10'

    def __gen_download_global_hdr(self, img_num, img_hdrs, version):

        magic = struct.pack('8s', self._magic.encode())
        version = struct.pack('>I', version)
        hdr_len = struct.pack('>H', DOWNLOAD_GLOBAL_HDR_LEN)
        img_num = struct.pack('>H', img_num)
        flags = struct.pack('>I', 0)
        reserved1 = struct.pack('>I', 0)
        reserved2 = struct.pack('>I', 0)
        global_crc_content = version + hdr_len + img_num + flags + reserved1 + reserved2
        for img_hdr in img_hdrs:
            global_crc_content += img_hdr

        global_crc = pack_crc32(0xffffffff, global_crc_content)
        global_crc = struct.pack('>I', global_crc)
        global_hdr = magic + global_crc + version + hdr_len + img_num + flags + reserved1 + reserved2
        logging.debug(f'add download global hdr: magic={magic}, img_num={img_num}, version={version}, flags={flags}, crc={global_crc}')
        return global_hdr

    def __gen_download_img_hdr(self, ph, img_offset, img_buf):

        offset = ph.size().offset()
        size = ph.size().size()
        version = 0

        if ph.type().is_data():
            cbus = 0
        elif ph.flags().download_write_cbus():
            cbus = 1
        else:
            cbus = 0

        logging.debug(f'Gen download header: partition_offset={hex(offset)}, partition_size={hex(size)}, img_offset={hex(img_offset)}, version={version}, cbus={cbus}')

        partition_offset = struct.pack('>I', offset)
        partition_size = struct.pack('>I', size)
        flash_start_addr = struct.pack('>I', offset)
        img_offset = struct.pack('>I', img_offset)
        img_len = struct.pack('>I', len(img_buf))

        checksum = pack_crc32(0xffffffff, img_buf)
        checksum = struct.pack('>I', checksum)

        version = struct.pack('>I', version)
        cbus = struct.pack('>H', cbus)
        reserved = 0
        reserved = struct.pack('>H', reserved)

        hdr = partition_offset + partition_size + flash_start_addr + img_offset + img_len  + checksum + version + cbus + reserved
        return hdr 

    def __encrypt(self, bname, pheader, flash_aes_key):
        if pheader.type().is_data():
            return bname

        if flash_aes_key == None:
            return bname

        aes_tool = get_flash_aes_tool()
        if pheader.type().is_merge():
            # The encryption also include the BL2 sign hdr
            start_address = hex(phy2virtual(pheader.size().phy_offset()))
        else:
            start_address = hex(pheader.size().vir_code_offset())

        if os.path.exists(bname) == False:
            logging.error(f'{bname} not exists, abort')
            exit(1)

        logging.debug(f'encrypt {bname}, startaddress={start_address}')
        outfile = bname[:-4] + '_aes.bin'
        cmd = f'{aes_tool} encrypt -infile {bname} -keywords {flash_aes_key} -outfile {outfile} -startaddress {start_address}'
        run_cmd_not_check_ret(cmd)
        return outfile

    def __add_crc(self, bname, pheader, flash_crc_en):
        if pheader.type().is_data() or (flash_crc_en == False):
            return bname

        crc_bname = f'{bname[:-4]}_crc.bin'
        calc_crc16(bname, crc_bname)
        logging.debug(f'add crc, in={bname}, out={crc_bname}')
        return crc_bname

    def __add_magic(self, bname, pheader):
        if pheader.type().is_app() and (pheader.size().vir_offset() != 0):
            return

        #TODO: change magic wordk according to soc
        logging.debug(f'Add magic code beken to {bname}')
        beken = b'\x42\x45\x4B\x45\x4E\x00\x00'
        with open(bname, 'r+b') as f:
            f.seek(0x100)
            f.write(beken)

    def __add_version(self, bname, pheader):
        if pheader.type().is_app() and (pheader.size().vir_offset() != 0):
            return

        version = pheader.version()
        numbers = [int(x) for x in version.split('.')]
        versions = bytes(numbers + [0] * (4-len(numbers)))
        logging.debug(f'Add version code {version} to {bname}')
        with open(bname, 'r+b') as f:
                f.seek(0x120)
                f.write(versions)

    def __add_pad(self, bname, pheader):
        if pheader.type().is_app():
            pad_size = pheader.size().hdr_pad_size()
        else:
            pad_size = 0

        logging.debug(f'add_pad: file={bname}, pad_size={pad_size}')

        img_buf = bytearray()
        with open(bname, 'rb') as f:
            img_buf = bytes([0xFF]*pad_size)
            img_buf += f.read()
        return img_buf

    def __create_download_bin(self, img_bufs, img_hdrs, global_hdr):

        offset = 0
        with open(self._outfile, 'wb+') as f:
            f.seek(offset)
            f.write(global_hdr)
            offset += DOWNLOAD_GLOBAL_HDR_LEN

            for img_hdr in img_hdrs:
                f.seek(offset)
                f.write(img_hdr)
                offset += DOWNLOAD_IMG_HDR_LEN

            for buf in img_bufs:
                f.seek(offset)
                f.write(buf)
                offset += len(buf)

    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):

        logging.debug(f'PackDownload({self._outfile}): bins={self._bins}, flash_aes_key={flash_aes_key} data_aes_key={data_aes_key}')

        img_num = len(self._bins)
        if (img_num == 0):
            return

        img_offset = (DOWNLOAD_IMG_HDR_LEN * img_num) + DOWNLOAD_GLOBAL_HDR_LEN
        img_hdrs = []
        img_bufs = []

        for b in self._bins:
            # If pack download is too complex, can relay encrypt/magic/version/crc to Partition
            ph = PackHeader(b)
            outfile = self.__encrypt('_' + b, ph, flash_aes_key)
            self.__add_magic(outfile, ph)
            self.__add_version(outfile, ph)
            outfile = self.__add_crc(outfile, ph, flash_crc_en)
            img_buf = self.__add_pad(outfile, ph)

            #TODO special handling for OTA and XIP_B 4k padding
            if ph.type().is_ow_ota() or ph.type().is_xip_b():
                l = len(img_buf)
                img_buf = bytes([0xFF]*l)

            img_bufs.append(img_buf)
            img_hdr = self.__gen_download_img_hdr(ph, img_offset, img_buf)
            img_hdrs.append(img_hdr)
            img_offset = img_offset + len(img_buf)

        global_hdr = self.__gen_download_global_hdr(img_num, img_hdrs, version=1)
        self.__create_download_bin(img_bufs, img_hdrs, global_hdr)
