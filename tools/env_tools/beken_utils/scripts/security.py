#!/usr/bin/env python3
import os
import logging
import csv

from .common import *
from .parse_csv import *

security_keys = [
    'secureboot_en',
    'flash_aes_en',
    'flash_aes_key',
    'root_key_type',
    'root_pubkey',
    'root_privkey',
    'debug_key_type',
    'debug_pubkey',
    'debug_privkey',
    'attestation_key_type',
    'attestation_pubkey',
    'attestation_privkey'
]

class Security(dict):

    def __getitem__(self, key):
        return self.csv.dic[key];

    def __init__(self, csv_file):
        self.csv = Csv(csv_file, False, security_keys)
        self.parse_csv()

    def parse_csv(self):
        self.secureboot_en = parse_bool(self.csv.dic['secureboot_en'])
        self.flash_aes_en = parse_bool(self.csv.dic['flash_aes_en'])

        self.flash_aes_key = self.csv.dic['flash_aes_key']
        if self.flash_aes_en  == True:
            if len(self.flash_aes_key) != 64:
                logging.error(f'Invalid AES key: key length should be 64')
                exit(1)

        self.bl1_root_key_type = self.csv.dic['root_key_type']
        self.bl1_root_pubkey = self.csv.dic['root_pubkey']
        self.bl1_root_privkey = self.csv.dic['root_privkey']

        self.bl2_root_key_type = self.csv.dic['root_key_type']
        self.bl2_root_pubkey = self.csv.dic['root_pubkey']
        self.bl2_root_privkey = self.csv.dic['root_privkey']

        if self.secureboot_en == True:
            #TODO need to support more root_key_type
            if (self.bl1_root_key_type != '') and (self.bl1_root_key_type != 'ec256') and (self.bl1_root_key_type != 'rsa2048'):
                logging.error(f'Unknown root key type {self.root_key_type}, only support ec256, rsa2048')
                exit(1)

        self.debug_key_type = self.csv.dic['debug_key_type']
        self.debug_pubkey = self.csv.dic['debug_pubkey']
        self.debug_privkey = self.csv.dic['debug_privkey']

        self.attestation_key_type = self.csv.dic['attestation_key_type']
        self.attestation_pubkey = self.csv.dic['attestation_pubkey']
        self.attestation_privkey = self.csv.dic['attestation_privkey']
