from .bk_packager_json import PartitionInfo
from bk_crc import bk_crc16
from . import logger


def pre_link(func):
    def wrapper(self, *args, **kwargs):
        for index, part_item in enumerate(self.part_info):
            crced_data = crc_handler.crc16_data(part_item.data)
            if len(crced_data) > part_item.size:
                raise RuntimeError(f"partition {part_item.name} binary length over partition size after crc")
            crced_part_item = PartitionInfo(part_item.addr, part_item.size, part_item.name, crced_data)
            self.part_info[index] = crced_part_item
        return func(self, *args, **kwargs)

    crc_handler = bk_crc16()
    return wrapper

def post_link(func):
    def wrapper(self, *args, **kwargs):
        ret = func(self, *args, **kwargs)
        with open(self.output_file_name, 'ab') as f:
            f.write(bytes([0xff] * 34))
        return ret
    
    return wrapper
