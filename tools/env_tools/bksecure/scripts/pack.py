#!/usr/bin/env python3

import logging
from abc import ABC, abstractmethod

class Pack(ABC):

    def __init__(self, bins=None, outfile=None):
        self._bins = bins
        self._outfile = outfile

    @abstractmethod
    def pack(self, *args):
        pass
