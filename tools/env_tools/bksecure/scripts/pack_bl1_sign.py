#!/usr/bin/env python3

import logging

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
        
        ph = PackHeader(self._bins[0])
        name = ph.name()
        infile = f'_{name}.bin'

        static_addr_int = 0x02000000 + ph.size().vir_code_offset()
        static_addr = f'0x%08x' %(static_addr_int)
        load_addr = static_addr

        if (img_sign_privkey != None):
            bl1_sign('sign', 'ec256', img_sign_privkey, img_sign_pubkey, None, infile, static_addr, load_addr, self._outfile)
        else:
            with open(self._outfile, "wb") as f:
                text = "NoManifest".encode()
                f.write(text)

        pack_manifest = f'pack_{self._outfile}'
        ph = PackHeader(pack_manifest)
        p = PackHeader()
        p.create_header(self._outfile, ph.version(), ph.name(), ph.type().type(),
            ph.type().subtype(), ph.size().offset(), ph.size().size(), ph.flags().flags(), 0, ph.m1(), ph.m2(), ph.m3(), ph.m4())