#!/usr/bin/env python3
import logging
import math
import os
import struct
import tempfile

from .common import *

COMPRESS_BLOCK_SZ = 0x10000

def _logical_chunk_to_crc(chunk):
    from .flash_crc import crc
    fd_in, path_in = tempfile.mkstemp()
    fd_out, path_out = tempfile.mkstemp()
    os.close(fd_out)
    try:
        with os.fdopen(fd_in, 'wb') as f:
            f.write(chunk)
        crc(path_in, path_out)
        with open(path_out, 'rb') as f:
            return f.read()
    finally:
        os.unlink(path_in)
        os.unlink(path_out)

def compress_bin(infile, outfile, flash_crc_en=False):
    compress_size_list = []
    compress_temp_in = 'temp_before_compress'
    compress_temp_out = 'temp_after_compress'
    file_size = os.path.getsize(infile)
    with open(infile,'rb') as src,open(outfile,'w+b') as dst:
        offset = 2 * math.floor(file_size/COMPRESS_BLOCK_SZ) + 4 # 2 uint16_t for after and before block size
        logging.debug(f'block num = {offset //2 - 1}')
        dst.seek(offset)
        sum = 0
        src.seek(0)
        idx = 0
        while True:
            file_in = open(compress_temp_in,"wb+")
            uncompress_block_size = bytes()
            chunk = bytes() # clear chunk
            chunk = src.read(COMPRESS_BLOCK_SZ)
            if(len(chunk) != COMPRESS_BLOCK_SZ):
                uncompress_block_size = struct.pack("H",len(chunk))
            if not chunk:
                file_in.close()
                break
            if flash_crc_en:
                chunk = _logical_chunk_to_crc(chunk)
            file_in.seek(0)
            file_in.write(chunk)
            file_in.close()
            
            script_dir = get_script_dir()
            compress_tool = get_lzma_tool()
            cmd =f'{compress_tool} e {compress_temp_in} {compress_temp_out}'
            run_cmd(cmd)
            file_out = open(compress_temp_out,"rb")
            chunk = bytes() # clear chunk
            file_out.seek(0)
            chunk = file_out.read()
            compress_chunk_size = len(chunk)
            logging.debug(f'block after size:{compress_chunk_size}')
            compress_chunk_size = struct.pack("H",compress_chunk_size)
            compress_size = compress_chunk_size + uncompress_block_size
            compress_size_list.append(compress_size)
            dst.write(chunk)
            sum += len(chunk)
            file_out.close()
        offset = 0
        dst.seek(offset)
        for num in compress_size_list:
            dst.write(num)
