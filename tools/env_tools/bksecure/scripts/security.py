#!/usr/bin/env python3
import os
import logging
import csv

from .common import *
from .parse_csv import *
from cryptography.hazmat.primitives import serialization

security_keys = [
    'bl1_secureboot_en',
    'flash_crc_en',
    'flash_aes_type',
    'flash_aes_key',
    'img_sign_key_type',
    'img_sign_pubkey',
    'img_sign_privkey',
    'new_img_sign_pubkey',
    'new_img_sign_privkey',
    'update_img_sign_key_en',
    'anti_rollback',
]

class Security(dict):

    def __getitem__(self, key):
        return self.csv.dic[key]

    def __init__(self, csv_file):
        self.csv = Csv(csv_file, False, security_keys)
        self.parse_csv()

    def is_flash_aes_fixed(self):
        return (self.flash_aes_type.upper() == 'FIXED')

    def is_flash_aes_random(self):
        return (self.flash_aes_type.upper() == 'RANDOM')

    def get_public_bytes_from_pubkeyfile(self, pubkey_file):
        if pubkey_file == None:
            return
        
        # Resolve file path
        if os.path.isabs(pubkey_file):
            # Absolute path: check directly
            if not os.path.exists(pubkey_file):
                logging.error(f'Absolute path pubkey file not found: {pubkey_file}')
                return

        with open(pubkey_file, 'rt') as pem_file:
            first_line = pem_file.readline().strip()
            if first_line != "-----BEGIN PUBLIC KEY-----":
                return
            pem_file.seek(0)
            pem_file_data = pem_file.read()

        try:
            public_key = serialization.load_pem_public_key(pem_file_data.encode())

            der_data = public_key.public_bytes(
                encoding=serialization.Encoding.DER,
                format=serialization.PublicFormat.SubjectPublicKeyInfo)
            return der_data
        except Exception as e:
            logging.error(f'failed to load PEM public key {e}')
            exit(1)

    def get_anti_rollback(self):
        return self.anti_rollback

    def parse_csv(self):
        self.bl1_secureboot_en = parse_bool(self.csv.dic['bl1_secureboot_en'])
        self.flash_crc_en = parse_bool(self.csv.dic['flash_crc_en'])
        self.flash_aes_type = self.csv.dic['flash_aes_type'].upper()

        self.flash_aes_key = self.csv.dic['flash_aes_key']
        if self.flash_aes_type == 'FIXED':
            key_len = len(self.flash_aes_key)
            if (key_len != 0) and (key_len != 64):
                logging.error(f'Invalid AES key: key length should be 0 or 64')
                exit(1)
        else:
            self.flash_aes_key = None

        if "img_sign_key_type" in self.csv.dic:
            self.img_sign_key_type = self.csv.dic['img_sign_key_type']
            self.img_sign_pubkey = self.csv.dic['img_sign_pubkey']
            self.img_sign_privkey = self.csv.dic['img_sign_privkey']
            self.img_sign_pubkey_bytes = self.get_public_bytes_from_pubkeyfile(self.img_sign_pubkey)
            logging.debug(f"key from security.csv privkey {self.img_sign_privkey} pubkey {self.img_sign_pubkey} pubkeybytes {self.img_sign_pubkey_bytes}")
        else:
            self.img_sign_key_type = None
            self.img_sign_pubkey = None
            self.img_sign_privkey = None
            self.img_sign_pubkey_bytes = None

        if "update_img_sign_key_en" in self.csv.dic:
            self.update_img_sign_key_en = parse_bool(self.csv.dic['update_img_sign_key_en'])
            if self.update_img_sign_key_en == True:
                self.new_img_sign_pubkey = self.csv.dic['new_img_sign_pubkey']
                self.new_img_sign_pubkey_bytes = self.get_public_bytes_from_pubkeyfile(self.new_img_sign_pubkey)
                self.new_img_sign_privkey = self.csv.dic['new_img_sign_privkey']
        else:
            self.update_img_sign_key_en = False

        if self.bl1_secureboot_en == True:
            if (self.img_sign_key_type != '') and (self.img_sign_key_type != 'ec256') and (self.img_sign_key_type != 'rsa2048') and (self.img_sign_key_type != 'rsa3072'):
                logging.error(f'Unknown root key type {self.img_sign_key_type}, only support ec256, rsa2048')
                exit(1)

        if "anti_rollback" in self.csv.dic:
            self.anti_rollback = parse_bool(self.csv.dic['anti_rollback'])
        else:
            self.anti_rollback = False
