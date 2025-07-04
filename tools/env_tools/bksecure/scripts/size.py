#!/usr/bin/env python3

import os
import json
import logging
import re

class Size:

    def __init__(self, offset=0, size=0, hdr_size=0):
        self._offset = offset
        self._size = size
        self._hdr_size = hdr_size

    def offset(self):
        return self._offset

    def size(self):
        return self._size

    def hdr_size(self):
        return self._hdr_size
