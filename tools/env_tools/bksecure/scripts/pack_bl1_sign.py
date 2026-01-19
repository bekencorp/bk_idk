#!/usr/bin/env python3

import logging
import os
import shutil

from .pack import Pack
from .pack_header import PackHeader
from .bl1_sign import bl1_sign

class PackBl1sign(Pack):
    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        logging.debug(f'PackManifest: bins={self._bins}, out={self._outfile},flash_aes_key={flash_aes_key} data_aes_key={data_aes_key}')
        logging.debug(f'img_sign_privkey={img_sign_privkey}')
        l = len(self._bins)
        if l != 1:
            logging.error(f'Bin number should be 1, actual={l}')
            exit(1)
        
        ph = PackHeader("bl2.bin")
        name = ph.name()
        infile = f'_bl2.bin'

        static_addr_int = 0x02000000 + ph.size().vir_code_offset()
        static_addr = f'0x%08x' %(static_addr_int)
        load_addr = static_addr

        if (img_sign_privkey != None):
            bl1_sign('sign', 'ec256', img_sign_privkey, img_sign_pubkey, None, infile, static_addr, load_addr, self._outfile)
        else:
            # Pack Server scenario: if img_sign_privkey is None, check if signed manifest already exists
            # Sign Server has already generated signed manifest (e.g., primary_manifest.bin)
            # Pack Server should use this signed manifest directly instead of signing again
            if os.path.exists('primary_manifest.bin'):
                logging.info(f'Pack Server: primary_manifest.bin already exists from Sign Server, copying to {self._outfile}')
                shutil.copy('primary_manifest.bin', self._outfile)
            else:
                logging.warning(f'Pack Server: img_sign_privkey is None and primary_manifest.bin not found, creating NoManifest')
                with open(self._outfile, "wb") as f:
                    text = "NoManifest".encode()
                    f.write(text)

        pack_manifest = f'pack_{self._outfile}'
        ph = PackHeader(pack_manifest)
        p = PackHeader()
        p.create_header(self._outfile, ph.version(), ph.name(), ph.type().type(),
            ph.type().subtype(), ph.size().offset(), ph.size().size(), ph.flags().flags(), 0, ph.m1(), ph.m2(), ph.m3(), ph.m4())