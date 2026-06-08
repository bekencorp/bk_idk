#!/usr/bin/env python3

from .pack_raw_ota import PackRawOta

class PackOta(PackRawOta):
    def pack(self, flash_aes_key=None, flash_crc_en=True, data_aes_key=None, flash_aes_type=None):
        self._pack_encrypt_build_hdr(flash_aes_key, flash_crc_en, data_aes_key)
        with open(self._outfile, 'w+b') as f:
            f.write(self._ota_global_hdr)
            for hdr in self._img_hdrs:
                f.write(hdr)

            for buf in self._img_bufs:
                f.write(buf)
