from . import logger

class bk_crc32:
    CRC32_INIT = 0xffffffff
    BYTEORDER = 'big'
    def __init__(self):
        self._init_crc32_table()

    def crc_data(self, raw_data:bytes) -> bytes:
        return self._crc32(raw_data)

    @classmethod
    def set_byteorder(cls, order:str):
        if order != 'big' or order != 'little':
            raise RuntimeError("order must be big or little")
        cls.BYTEORDER = order

    def _init_crc32_table(self):
        self.crc32_table = []
        for i in range(0,256):
            c = i
            for j in range(0,8):
                if c&1:
                    c = 0xEDB88320 ^ (c >> 1)
                else:
                    c = c >> 1
            self.crc32_table.append(c)

    def _crc32(self, raw_data:bytes) -> int:
        data = self.CRC32_INIT
        for byte in raw_data:
            data = (data >> 8 ) ^ self.crc32_table[(data ^ byte) & 0xff]
        return data