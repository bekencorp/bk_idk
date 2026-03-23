#!/usr/bin/env python3

import os
import logging
import struct

from .partition_flags import PartitionFlags
from .partition_type import PartitionType
from .size_crc import SizeCrc
from .size import Size
from .common import is_flash_crc_enabled

#pack_header(12), Name(20)
#version(8), Type(1), Subtype(1), Reserved(2) Offset(4), Size(4), Flags(4), Hdr_size(4) Reserved(4)
#Meta1(4), Meta2(4), Meta3(4), Meta4(4), Reserved(16)
PACK_HEADER_FORMAT = "12s20s8sBB2xIIII4xIIII16x"
PACK_HEADER_SIZE = struct.calcsize(PACK_HEADER_FORMAT)
PACK_HEADER_MAGIC = 'pack_header'

class PackHeader:

    def __init__(self, bname=None):
        if bname != None:
            self.__parse_header(bname)

    def create_header(self, bname, version, name, type_, subtype, offset, size, flags, hdr_size, m1=0, m2=0, m3=0, m4=0):
        logging.debug(f'create pack header: bname={bname}, partition={name}')
        logging.debug(f'pack header: version={version},offset={hex(offset)},size={hex(size)}')
        logging.debug(f'now path : {os.getcwd()}')

        if not os.path.exists(bname):
            logging.error(f'create_header: Bin {name} not exists')
            exit(1)

        magic_bytes = PACK_HEADER_MAGIC.encode('utf-8')[:12].ljust(12, b'\x00')
        name_bytes = name.encode('utf-8')[:20].ljust(20, b'\x00')
        version_bytes = version.encode('utf-8')[:8].ljust(8, b'\x00')
        header = struct.pack(PACK_HEADER_FORMAT, magic_bytes, name_bytes,
            version_bytes, type_, subtype, offset, size, flags, hdr_size,
            m1, m2, m3, m4)

        backup = f'_{bname}'
        if os.path.exists(backup):
            os.remove(backup)
        os.rename(bname, backup)

        data = None
        with open(backup, "rb") as f:
            data = f.read()

        with open(bname, "wb") as f:
            f.write(header)
            f.write(data)

    def __parse_header(self, bname):
        logging.debug(f"parse Bin {bname} header")
        if os.path.exists(bname) == False:
            logging.error(f'parse header: Bin {bname} not exists')
            exit(1)

        with open(bname, "rb") as f:
            header_data = f.read(PACK_HEADER_SIZE)
            self._bin_data = f.read()
            bname_without_header = '_' + bname
            if os.path.exists(bname_without_header) == False:
                with open(bname_without_header, "wb") as f:
                    f.write(self._bin_data)

        magic_bytes, name_bytes, version_bytes, type_, subtype, offset, size, flags, hdr_size, m1, m2, m3, m4 = struct.unpack(PACK_HEADER_FORMAT, header_data)

        # Check magic before decoding to avoid UnicodeDecodeError for files without pack header
        if not magic_bytes.startswith(PACK_HEADER_MAGIC.encode('utf-8')):
            logging.error(f'Invalid bin pack header, bname={bname}')
            logging.error(f'  Expected magic: {PACK_HEADER_MAGIC}')
            logging.error(f'  Found magic bytes (hex): {magic_bytes[:12].hex()}')
            logging.error(f'  File may not have pack header or is corrupted')
            exit(1)
        
        # Safe to decode now that we know magic is valid
        magic = magic_bytes.rstrip(b'\x00').decode('utf-8')
        name = name_bytes.rstrip(b'\x00').decode('utf-8')
        version = version_bytes.rstrip(b'\x00').decode('utf-8')

        self._name = name
        self._type = PartitionType(type_, subtype)
        self._flags = PartitionFlags(flags)
        self._m1 = m1
        self._m2 = m2
        self._m3 = m3
        self._m4 = m4
        self._version = version
        if self.type().is_data():
            self._size = Size(offset, size, hdr_size)
        else:
            if is_flash_crc_enabled():
                self._size = SizeCrc(offset, size, hdr_size)
            else:
                self._size = Size(offset, size, hdr_size)

    def name(self):
        return self._name

    def size(self):
        return self._size

    def type(self):
        return self._type

    def subtype(self):
        return self._subtype

    def flags(self):
        return self._flags

    def version(self):
        return self._version

    def security_counter(self):
        return 0

    def flags(self):
        return self._flags

    def data(self):
        return self._bin_data

    def m1(self):
        return self._m1

    def m2(self):
        return self._m2

    def m3(self):
        return self._m3

    def m4(self):
        return self._m4
