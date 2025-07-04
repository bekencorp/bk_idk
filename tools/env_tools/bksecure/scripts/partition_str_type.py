#!/usr/bin/env python3

import os
import logging

from .partition_type import *

class PartitionStrType(PartitionType):

    def __init__(self, type='', subtype=''):
        if type in type_mapping:
            _type = type_mapping[type]
        else:
            logging.error(f'Unsupported type={type}')
            exit(1)

        if subtype in subtype_mapping:
            _subtype = subtype_mapping[subtype]
        else:
            logging.error(f'Unsupported subtype={subtype}')
            exit(1)

        super().__init__(_type, _subtype)
