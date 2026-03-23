#!/usr/bin/env python3
import os
import logging
import csv

from .parse_csv import *

ota_keys = [
    'strategy',
    'encrypt',
    'anti_rollback',
    'app_security_counter',
    'app_version',
    'bootloader_ota',
    'bootloader_version'
]

class OTA(list):

    def __getitem__(self, key):
        return self.csv.dic[key]

    def __is__exist__item__(self, key):
        return (key in self.csv.dic)

    def get_app_security_counter(self):
        return int(self.csv.dic['app_security_counter'])

    def get_bl2_security_counter(self):
        return 4

    def get_strategy(self):
        return self.csv.dic['strategy']

    def get_encrypt(self):
        if self.__is__exist__item__('encrypt'):
            return (self.csv.dic['encrypt'].upper() == 'TRUE')
        else:
            return False

    def get_version(self):
        if self.__is__exist__item__('app_version'):
            return self.csv.dic['app_version']
        else:
            return "0.0.0"

    def get_anti_rollback(self):
        if self.__is__exist__item__('anti_rollback'):
            return (self.csv.dic['anti_rollback'].upper() == 'TRUE')
        else:
            return False

    def get_boot_ota(self):
        if self.__is__exist__item__('bootloader_ota'):
            return (self.csv.dic['bootloader_ota'] == 'TRUE')
        else:
            return False

    def get_bootloader_version(self):
        if self.__is__exist__item__('bootloader_version'):
            return self.csv.dic['bootloader_version']
        else:
            return "0.0.0"

    def is_overwrite(self):
        if(self.csv.dic['strategy'].upper() == 'OVERWRITE'):
            return True
        else:
            return False

    def __init__(self, csv_file):
        self.csv = Csv(csv_file, False, ota_keys)
