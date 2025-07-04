#!/usr/bin/env python3

import logging
import struct

from .common import *

crc32_table = [0] * 256

def make_crc32_table():
    if crc32_table[255] != 0:
        return
    
    for i in range(256):
        c = i
        for bit in range(8):
            if c & 1:
                c = (c >> 1) ^ 0xEDB88320
            else:
                c = c >> 1
        crc32_table[i] = c
    
def pack_crc32(crc, buf):
    make_crc32_table()
    for byte in buf:
        crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF]
    return crc
