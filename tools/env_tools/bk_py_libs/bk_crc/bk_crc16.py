from . import logger
import os
import shutil

def crc16(data : bytearray, offset , length):
    if data is None or offset < 0 or offset > len(data)- 1 and offset+length > len(data):
        return 0
    crc = 0xFFFFFFFF
    for i in range(0, length):
        crc ^= data[offset + i] << 8
        for j in range(0,8):
            if (crc & 0x8000) > 0:
                crc =(crc << 1) ^ 0x8005 #for beken poly:8005
            else:
                crc = crc << 1
    return crc & 0xFFFF

class bk_crc16:
    CRC_PACK_LEN = 32
    PADDING_BYTE = bytes([0xFF])

    def __init__(self):
        pass

    def compute_crced_size(self, raw_size):
        return (raw_size + self.CRC_PACK_LEN - 1) // self.CRC_PACK_LEN * (self.CRC_PACK_LEN + 2)

    def crc_file(self, raw_file, output_file):
        self.file_path = os.path.dirname(raw_file)
        operate_file = self._backup_file(raw_file)
        newfile = self._file_align_handle(operate_file)
        self._crc16_handle(operate_file, output_file)
        os.remove(operate_file)

    def crc16_data(self, raw_data) -> bytes:
        raw_data = self._data_align_handle(raw_data)
        output_data = bytearray()
        for i in range(0, len(raw_data), self.CRC_PACK_LEN):
            chunk = raw_data[i:i + self.CRC_PACK_LEN]
            output_data.extend(chunk)
            crc = crc16(chunk, 0, self.CRC_PACK_LEN).to_bytes(2, byteorder='big')
            output_data.extend(crc)
        
        return bytes(output_data)

    def _file_align_handle(self, operate_file):
        file_size = os.path.getsize(operate_file)
        if file_size % self.CRC_PACK_LEN == 0:
            return

        padding_len = self.CRC_PACK_LEN - (file_size % self.CRC_PACK_LEN)
        logger.debug(f"padding_len = {padding_len}")
        with open(operate_file, 'ab') as f:
            f.write(self.PADDING_BYTE * padding_len)

    def _backup_file(self, raw_file):
        raw_file_name = os.path.basename(raw_file)
        operate_file_name = 'bk_crc_' + raw_file_name
        operate_file_path = os.path.join(self.file_path, operate_file_name)
        if os.path.exists(operate_file_path):
            os.remove(operate_file_path)
        shutil.copy(raw_file, operate_file_path)
        return operate_file_path

    def _crc16_handle(self, operate_file, output_file):
        output_bytes = bytearray()
        chunk_size = 4096
        with open(operate_file, 'rb') as file:
            while True:
                data_chunk = file.read(chunk_size)
                if not data_chunk:
                    break
                output_bytes.extend(self.crc16_data(data_chunk))
        
        if os.path.exists(output_file):
            os.remove(output_file)
        
        with open(output_file, 'wb') as f:
            f.write(output_bytes)
    
    def _data_align_handle(self, raw_data):
        data_len = len(raw_data)
        if data_len % self.CRC_PACK_LEN == 0:
            return raw_data

        padding_len = self.CRC_PACK_LEN - (data_len % self.CRC_PACK_LEN)
        logger.debug(f"padding_len = {padding_len}")

        return raw_data + self.PADDING_BYTE * padding_len
