#!/usr/bin/env python3

import logging

from .partition_flags import *

flags_mapping = {
    "encrypted" : FLAG_ENCRYPTED,
    "flash_encrypted" : FLAG_FLASH_ENCRYPTED,
    "download_write_cbus" : FLAG_DOWNLOAD_WRITE_CBUS,
    "ota_write_cbus" : FLAG_OTA_WRITE_CBUS,
    "ota_read_cbus"  : FLAG_OTA_READ_CBUS,
    "cbus_nonsecure" : FLAG_CBUS_NS,
    "dbus_nonsecure" : FLAG_DBUS_NS
}

class PartitionStrFlags(PartitionFlags):

    def __init__(self, str_flags=''):
        self._str_flags = str_flags
        int_flags = self.__parse_str_flags(str_flags, flags_mapping)
        super().__init__(int_flags)

    def __parse_str_flags(self, str_flags, flags_mapping):
        if (str_flags == '') or (str_flags == None):
            return 0

        flags_enum = 0
        flag_list = re.split(r'\s*\|\s*', str_flags)

        for flag in flag_list:
            if flag in flags_mapping:
                flags_enum |= flags_mapping[flag]

        return flags_enum    
