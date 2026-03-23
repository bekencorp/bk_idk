#!/usr/bin/env python3

import os
import json
import logging
import re

FLAG_DBUS_READ           = (1<<0)
FLAG_DBUS_WRITE          = (1<<1)
FLAG_EXECUTE             = (1<<2)
FLAG_CBUS_READ           = (1<<3)
FLAG_CBUS_WRITE          = (1<<4)
FLAG_ENCRYPTED           = (1<<0)
FLAG_FLASH_ENCRYPTED     = (1<<1)

FLAG_OTA_WRITE_CBUS      = (1<<5)
FLAG_DOWNLOAD_WRITE_CBUS = (1<<6)
FLAG_OTA_READ_CBUS       = (1<<7)
FLAG_CBUS_NS             = (1<<8)
FLAG_DBUS_NS             = (1<<9)

class PartitionFlags:

    def __init__(self, int_flags=0):
        self._int_flags = int_flags
        self.__dump_flags()

    def flags(self):
        return self._int_flags

    def ota_write_cbus(self):
        if (self._int_flags & FLAG_OTA_WRITE_CBUS):
            return True
        return False

    def download_write_cbus(self):
        if (self._int_flags & FLAG_DOWNLOAD_WRITE_CBUS):
            return True
        return False

    def ota_read_cbus(self):
        if (self._int_flags & FLAG_OTA_READ_CBUS):
            return True
        return False

    def cbus_secure(self):
        if (self._int_flags & FLAG_CBUS_NS):
            return False
        return True

    def dbus_secure(self):
        if (self._int_flags & FLAG_DBUS_NS):
            return False
        return True

    def __dump_flags(self):
        logging.debug(f'ota_write_cbus={self.ota_write_cbus()}, download_write_cbus={self.download_write_cbus()}, ota_read_cbus={self.ota_read_cbus()}, cbus_secure={self.cbus_secure()}, dbus_secure={self.dbus_secure()}')
