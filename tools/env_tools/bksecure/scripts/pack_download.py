#!/usr/bin/env python3

import logging
import os
import shutil
import struct

from .pack import Pack
from .pack_header import PackHeader
from .common import *
from .crc import pack_crc32
from .size import Size

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

    def __encrypt(self, bname, pheader, flash_aes_key, flash_crc_en, flash_aes_type=None):
        partition_name = pheader.name() if hasattr(pheader, 'name') else 'unknown'
        is_data = pheader.type().is_data()

        # Check if partition is data type (skip encryption for data partitions)
        if pheader.type().is_data():
            logging.debug(f'Encrypt: {partition_name} (data partition, skipping)')
            return bname

        # Generate output filename with _aes suffix
        outfile = bname[:-4] + '_aes.bin'
        outfile_abs = os.path.abspath(outfile)

        # If flash_aes_type is NONE, just copy the file without encryption
        if flash_aes_type == 'NONE':
            logging.debug(f'Encrypt: {partition_name} (AES disabled, copying)')
            if os.path.exists(bname) == False:
                logging.error(f'Encrypt: {bname} not exists, abort')
                exit(1)
            shutil.copy(bname, outfile)
            if not os.path.exists(outfile):
                logging.error(f'Encrypt: failed to copy {bname} to {outfile}')
                exit(1)
            return outfile

        # If flash_aes_key is None, return original filename (no encryption)
        if flash_aes_key == None:
            logging.warning(f'Encrypt: {partition_name} (no key provided, skipping encryption)')
            return bname

        # Get AES encryption tool
        aes_tool = get_flash_aes_tool()

        # Check if AES tool exists
        if not os.path.exists(aes_tool):
            logging.error(f'Encrypt: AES tool not found: {aes_tool}')
            exit(1)

        # Calculate encryption start address
        if pheader.type().is_merge():
            if flash_crc_en:
                start_address = hex(phy2virtual(pheader.size().phy_offset()))
            else:
                start_address = hex(pheader.size().phy_offset())
        else:
            if flash_crc_en:
                start_address = hex(pheader.size().vir_code_offset())
            else:
                start_address = hex(pheader.size().offset())

        # Check if input file exists
        if os.path.exists(bname) == False:
            logging.error(f'Encrypt: input file {bname} not exists, abort')
            exit(1)

        input_file_size = os.path.getsize(bname)

        # Determine command format based on tool type
        if aes_tool.endswith('.py'):
            cmd = f'python3 {aes_tool} encrypt -infile {bname} -keywords {flash_aes_key}  -aes {128} -outfile {outfile} -startaddress {start_address}'
        else:
            cmd = f'{aes_tool} encrypt -infile {bname} -keywords {flash_aes_key} -outfile {outfile} -startaddress {start_address}'

        logging.debug(f'Encrypt: {partition_name} - executing: {cmd}')
        run_cmd_not_check_ret(cmd)

        # Verify that output file was created after encryption
        if not os.path.exists(outfile):
            logging.error(f'Encrypt: {partition_name} - encryption failed, output file not created')
            logging.error(f'Encrypt: Command: {cmd}')
            exit(1)

        output_file_size = os.path.getsize(outfile)
        logging.info(f'Encrypt: {partition_name} ({input_file_size} -> {output_file_size} bytes)')

        return outfile

    def __add_crc(self, bname, pheader, flash_crc_en):
        if pheader.type().is_data() or (flash_crc_en == False):
            return bname

        crc_bname = f'{bname[:-4]}_crc.bin'
        calc_crc16(bname, crc_bname)
        logging.debug(f'add crc, in={bname}, out={crc_bname}')
        return crc_bname

    def __add_magic(self, bname, pheader, flash_crc_en):
        if not pheader.type().is_app():
            return

        size_obj = pheader.size()
        if flash_crc_en:
            if not hasattr(size_obj, "vir_offset") or size_obj.vir_offset() != 0:
                return
        else:
            if size_obj.offset() != 0:
                return

        #TODO: change magic wordk according to soc
        magic = "BEKEN".encode() + b'\x00' * 3
        logging.debug(f'Add magic code {magic} to {bname}')
        with open(bname, 'r+b') as f:
            f.seek(0x100)
            f.write(magic)

    def __add_version(self, bname, pheader, flash_crc_en):
        if not pheader.type().is_app():
            return

        size_obj = pheader.size()
        if flash_crc_en:
            if not hasattr(size_obj, "vir_offset") or size_obj.vir_offset() != 0:
                return
        else:
            if size_obj.offset() != 0:
                return

        version = pheader.version()
        numbers = [int(x) for x in version.split('.')]
        versions = bytes(numbers + [0] * (4-len(numbers)))
        logging.debug(f'Add version code {version} to {bname}')
        with open(bname, 'r+b') as f:
                f.seek(0x120)
                f.write(versions)

    def __add_pad(self, bname, pheader, flash_crc_en):
        size_obj = pheader.size()
        if pheader.type().is_app():
            if pheader.type().is_merge():
                pad_size = size_obj.hdr_pad_size() if hasattr(size_obj, "hdr_pad_size") else 0
            else:
                if flash_crc_en:
                    pad_size = (size_obj.phy_code_offset() - size_obj.offset()) if hasattr(size_obj, "phy_code_offset") else 0
                else:
                    pad_size = size_obj.offset()
        else:
            pad_size = 0

        logging.debug(f'add_pad: file={bname}, pad_size={hex(pad_size)}')

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

    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None, flash_aes_type=None):
        img_num = len(self._bins)
        if (img_num == 0):
            return

        logging.debug(f'PackDownload: {self._outfile}, bins={self._bins}, flash_aes_type={flash_aes_type}')

        img_offset = (DOWNLOAD_IMG_HDR_LEN * img_num) + DOWNLOAD_GLOBAL_HDR_LEN
        img_hdrs = []
        img_bufs = []

        for b in self._bins:
            # If pack download is too complex, can relay encrypt/magic/version/crc to Partition
            # Try to parse pack header, if file doesn't have header, try to use _bname (without header version)
            try:
                ph = PackHeader(b)
            except SystemExit:
                # File doesn't have pack header, check if _bname exists (created by PackHeader when parsing)
                bname_without_header = '_' + b
                if os.path.exists(bname_without_header):
                    logging.warning(f'{b} does not have pack header, but {bname_without_header} exists. This file may need to be processed first (e.g., BL2_SIGN or PACK_BL2_BIN)')
                    logging.error(f'{b} must have pack header to be used in PACK_BL1_DOWNLOAD_BIN. Please ensure it is processed by BL2_SIGN or PACK_BL2_BIN first.')
                    exit(1)
                else:
                    logging.error(f'{b} does not have pack header and {bname_without_header} does not exist. This file may need to be processed first (e.g., BL2_SIGN or PACK_BL2_BIN)')
                    exit(1)

            # In no_crc mode, app partitions use physical layout to avoid vir_offset drift
            if (not flash_crc_en) and ph.type().is_app():
                orig_size = ph.size()
                ph._size = Size(orig_size.offset(), orig_size.size(), orig_size.hdr_size())
                logging.debug(f'Override size to physical layout for {b}: offset=0x{orig_size.offset():x}, size=0x{orig_size.size():x}')

            outfile = self.__encrypt('_' + b, ph, flash_aes_key, flash_crc_en, flash_aes_type)

            size_obj = ph.size()
            # Safe logging: data partitions may not have vir_offset/phy_code_offset
            vir_offset = size_obj.vir_offset() if hasattr(size_obj, "vir_offset") else 0
            phy_code_offset = size_obj.phy_code_offset() if hasattr(size_obj, "phy_code_offset") else 0
            logging.debug(f'bin={b}, type={ph.type().type()}, offset=0x{size_obj.offset():x}, size=0x{size_obj.size():x}, flash_crc_en={flash_crc_en}')

            # Only app add magic & version
            if ph.type().is_app():
                self.__add_magic(outfile, ph, flash_crc_en)
                self.__add_version(outfile, ph, flash_crc_en)

            outfile = self.__add_crc(outfile, ph, flash_crc_en)
            img_buf = self.__add_pad(outfile, ph, flash_crc_en)

            logging.debug(f'bin={b}, after pad len=0x{len(img_buf):x}, img_offset=0x{img_offset:x}')

            # Special handling for OTA and XIP_B 4k padding
            if ph.type().is_ow_ota() or ph.type().is_xip_b():
                l = len(img_buf)
                img_buf = bytes([0xFF]*l)
                logging.debug(f'bin={b}, type special (ow_ota/xip_b), replace with 0xFF, len=0x{l:x}')

            img_bufs.append(img_buf)
            img_hdr = self.__gen_download_img_hdr(ph, img_offset, img_buf)
            img_hdrs.append(img_hdr)
            img_offset = img_offset + len(img_buf)

        global_hdr = self.__gen_download_global_hdr(img_num, img_hdrs, version=1)
        self.__create_download_bin(img_bufs, img_hdrs, global_hdr)
