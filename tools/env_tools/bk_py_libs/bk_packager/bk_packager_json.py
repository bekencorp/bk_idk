import json
import os
from collections import namedtuple
from . import logger

PartitionInfo = namedtuple('PartitionInfo', ['addr', 'size', 'name', 'data'])
    
def check_overlaps(spaces):
    intervals = [(start, start + length) for start, length in spaces]
    intervals.sort()
    for i in range(1, len(intervals)):
        if intervals[i][0] < intervals[i - 1][1]:
            return True

    return False

def parse_size(size_str:str):
    for letter, multiplier in [("k", 1024), ("m", 1024 * 1024)]:
        if size_str.lower().endswith(letter):
            return parse_size(size_str[:-1]) * multiplier
        return int(size_str, 0)

""" Demo json
{
    "count": 3,
    "crc_enable": 0,
    "section": [
        {
            "firmware": "bootloader.bin",
            "version": "2M.1220",
            "partition": "bootloader",
            "start_addr": "0x00000000",
            "size": "64K"
        },
        {
            "firmware": "cm-app.bin",
            "version": "2M.1220",
            "partition": "app",
            "start_addr": "0x00010000",
            "size": "1984K"
        },
        {
            "firmware": "ca-app.bin",
            "version": "2M.1220",
            "partition": "app2",
            "start_addr": "0x00200000",
            "size": "2176K"
        }
    ]
}
"""

class bk_packager_json:
    def __init__(self, workdir:str, pack_json:str):
        self.workdir = os.path.abspath(workdir)
        self.pack_json = os.path.abspath(pack_json)
        logger.debug(f"pack_json:{self.pack_json}")
        # check workdir
        if (not os.path.exists(self.workdir)) or (not os.path.isdir(self.workdir)):
            raise RuntimeError(f"work directory {self.workdir} not exist.")

        # check json exist
        if (not os.path.exists(self.pack_json)) or (not os.path.isfile(self.pack_json)):
            raise RuntimeError(f"config json {self.pack_json} not exist.")
        
        # check json valid
        with open(self.pack_json, "r", encoding='utf-8') as f:
            config_json = json.load(f)

        self.section = config_json['section']
        self.partitions = []
        self._parse_and_check_partitions()

    def get_partitions(self) -> list:
        return self.partitions

    def _parse_and_check_partitions(self):
        space_sections = []
        for part in self.section:
            bin_name = part['firmware']
            part_addr = int(part['start_addr'], 16)
            part_size = parse_size(part['size'])
            part_name = part['partition']
            bin_path = os.path.join(self.workdir, bin_name)
            if (not os.path.exists(bin_path)) or (not os.path.isfile(bin_path)):
                raise RuntimeError(f"{bin_path} not exist!")
            self._check_binary_size_valid(bin_path, part_size)
            with open(bin_path, 'rb') as f:
                bin_content = f.read()
            part_info = PartitionInfo(part_addr, part_size, part_name, bin_content)
            self.partitions.append(part_info)
            space_sections.append((part_addr, part_size))

        if check_overlaps(space_sections):
            raise RuntimeError(f"partition exist overlaps!")
        
        self.partitions.sort()
    
    def _check_binary_size_valid(self, bin_path, part_size):
        bin_size = os.path.getsize(bin_path)
        if bin_size > part_size:
            raise RuntimeError(f"{bin_path} size is over partitions size.")
        
def parse_packager_json(workdir:str, pack_json:str) -> list:
    pack_json_parser = bk_packager_json(workdir, pack_json)
    return pack_json_parser.get_partitions()
