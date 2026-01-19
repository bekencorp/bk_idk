#!/usr/bin/env python3

import logging

class Bin():
    def __init__(self, name, _type, partition, version='1.0.0', security_counter=0):
        self._name = name
        self._type = _type
        self._partition = partition
        self._version = version
        self._security_counter = security_counter
        self.__dump_bin()

    def name(self):
        return self._name

    def type(self):
        return self._type

    def partition(self):
        return self._partition

    def version(self):
        return self._version

    def security_counter(self):
        return self._security_counter

    def is_padding(self):
        if (self._type == 'padding'):
            return True
        else:
            return False

    def __dump_bin(self):
        logging.debug(f'bin: {self._name, self._type, self._partition, self._version, self._security_counter}')
