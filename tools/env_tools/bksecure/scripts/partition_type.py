#!/usr/bin/env python3

import os
import logging

TYPE_DATA = 0
TYPE_APP  = 1

type_mapping = {
    ""         : TYPE_DATA,
    "data"     : TYPE_DATA,
    "app"      : TYPE_APP
}

SUBTYPE_DEFAULT          = 0
SUBTYPE_OW_ACTIVE        = 1
SUBTYPE_OW_OTA           = 2
SUBTYPE_XIP_A            = 3
SUBTYPE_XIP_B            = 4
SUBTYPE_MERGE_OVERWRITE  = 5
SUBTYPE_MERGE_XIP_A      = 6
SUBTYPE_MERGE_XIP_B      = 7
SUBTYPE_BL1_CTRL         = 8 
SUBTYPE_BL1_MANIFEST     = 9
SUBTYPE_NVS              = 10
SUBTYPE_PARTITION        = 11
SUBTYPE_OW_OTA_CONTROL   = 12
SUBTYPE_XIP_OTA_CONTROL  = 13
SUBTYPE_BL1_CONTROL      = 14
SUBTYPE_BOOT_FLAG        = 15
SUBTYPE_MANIFEST         = 16
SUBTYPE_FACTORY          = 17
SUBTYPE_NVS_KEY          = 18

subtype_mapping = {
    ""                : SUBTYPE_DEFAULT,
    "ow_active"       : SUBTYPE_OW_ACTIVE,
    "ow_ota"          : SUBTYPE_OW_OTA,
    "xip_a"           : SUBTYPE_XIP_A,
    "xip_b"           : SUBTYPE_XIP_B,
    "merge_overwrite" : SUBTYPE_MERGE_OVERWRITE,
    "merge_xip_a"     : SUBTYPE_MERGE_XIP_A,
    "merge_xip_b"     : SUBTYPE_MERGE_XIP_B,
    "bl1_ctrl"        : SUBTYPE_BL1_CTRL,
    "bl1_manifest"    : SUBTYPE_BL1_MANIFEST,
    "nvs"             : SUBTYPE_NVS,
    "partition"       : SUBTYPE_PARTITION,
    "ow_ota_control"  : SUBTYPE_OW_OTA_CONTROL,
    "xip_ota_control" : SUBTYPE_XIP_OTA_CONTROL,
    "bl1_control"     : SUBTYPE_BL1_CONTROL,
    "boot_flag"       : SUBTYPE_BOOT_FLAG,
    "manifest"        : SUBTYPE_MANIFEST,
    "factory"         : SUBTYPE_FACTORY,
    "nvs_key"         : SUBTYPE_NVS_KEY,
}

class PartitionType:

    def __init__(self, type=0, subtype=0):
        self._type = type
        self._subtype = subtype

    def type(self):
        return self._type

    def subtype(self):
        return self._subtype

    def is_data(self):
        if self._type == TYPE_DATA:
            return True
        return False

    def is_app(self):
        if self._type == TYPE_APP:
            return True
        return False

    def is_xip_a(self):
        if self._subtype == SUBTYPE_XIP_A:
            return True
        return False

    def is_xip_b(self):
        if self._subtype == SUBTYPE_XIP_B:
            return True
        return False

    def is_ow_active(self):
        if self._subtype == SUBTYPE_OW_ACTIVE:
            return True
        return False

    def is_ow_ota(self):
        if self._subtype == SUBTYPE_OW_OTA:
            return True
        return False

    def is_subapp(self):
        if self.is_xip_a() or self.is_xip_b() or self.is_ow_active():
            return True
        else:
            return False

    def is_partition(self):
        if self._subtype == SUBTYPE_PARTITION:
            return True
        return False

    def is_merge(self):
        if (self._subtype == SUBTYPE_MERGE_OVERWRITE) or (self._subtype == SUBTYPE_MERGE_XIP_A) or (self._subtype == SUBTYPE_MERGE_XIP_B):
            return True
        return False

    def is_merge_overwrite(self):
        if (self._subtype == SUBTYPE_MERGE_OVERWRITE):
            return True
        return False

    def is_merge_xip(self):
        if (self._subtype == SUBTYPE_MERGE_XIP_A) or (self._subtype == SUBTYPE_MERGE_XIP_B):
            return True
        return False

    def is_merge_xip_a(self):
        if (self._subtype == SUBTYPE_MERGE_XIP_A):
            return True
        return False
