#!/usr/bin/env python3

import logging
import os

from cryptography.hazmat.primitives import serialization

class Pubkey():

    def __init__(self, pubkey=None):
        self._pubkey = pubkey

    def key_bytes(self):

        if self._pubkey == None:
            return None

        if not os.path.exists(self._pubkey):
            return None

        with open(self._pubkey, 'rt') as pem_file:
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


