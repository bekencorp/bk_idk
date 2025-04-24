import os
from .bk_packager_json import PartitionInfo
from .bk_packager_linker import bk_packager_linker
from . import logger

class bk_packager_linear_linker(bk_packager_linker):
    def __init__(self):
        super().__init__()
        self.start_padding_mode = False

    def link(self, part_info:list, output_file_name):
        curr_pos = 0
        output_file = output_file_name
        if os.path.exists(output_file):
            os.remove(output_file)
        f = open(output_file, "wb")
        for part_item in part_info:
            part_addr = part_item.addr
            logger.debug(f"part name: {part_item.name}")
            logger.debug(f"part_addr: {part_addr}")
            if (curr_pos > part_addr):
                raise RuntimeError("")
            if curr_pos or self.start_padding_mode:
                padding_len = part_addr - curr_pos
            else:
                padding_len = 0
            logger.debug(f"padding_len: {padding_len}")
            if padding_len:
                f.write(bytes([0xff] * padding_len))
            curr_pos = part_addr
            part_bin_content = part_item.data
            f.write(part_bin_content)
            curr_pos += len(part_bin_content)
        f.close()

    def set_start_padding_mode(self, mode:bool):
        self.start_padding_mode = mode
