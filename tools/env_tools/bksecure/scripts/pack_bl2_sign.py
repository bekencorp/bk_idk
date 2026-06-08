#!/usr/bin/env python3

import logging
import struct
import hashlib
import json
import os
import subprocess
import tempfile

from .pack import Pack
from .pack_header import PackHeader
from .pubkey import Pubkey
from .bl2_sign import bl2_sign, bl2_sign_hash
from .gen_security import Security
from .gen_ota import OTA
from cryptography.hazmat.primitives import serialization

def verify_key_pair_match(privkey_path, pubkey_path):
    """Verify that private key and public key are a matching pair using openssl"""
    try:
        # Extract public key from private key
        with tempfile.NamedTemporaryFile(mode='w', suffix='.pem', delete=False) as tmp_pubkey:
            tmp_pubkey_path = tmp_pubkey.name
        
        try:
            # Extract public key from private key
            cmd = f'openssl ec -in "{privkey_path}" -pubout -out "{tmp_pubkey_path}" 2>&1'
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
            if result.returncode != 0:
                return False
            
            # Compare the two public keys
            cmd = f'diff "{tmp_pubkey_path}" "{pubkey_path}" > /dev/null 2>&1'
            result = subprocess.run(cmd, shell=True)
            return result.returncode == 0
        finally:
            # Clean up temp file
            if os.path.exists(tmp_pubkey_path):
                os.unlink(tmp_pubkey_path)
    except Exception as e:
        return False


def _prefer(explicit, fallback):
    """Prefer caller-provided value; when None, use value from security.csv / CSV helpers."""
    return explicit if explicit is not None else fallback


def _new_pubkey_bytes(new_pubkey_pem, security):
    if new_pubkey_pem is not None:
        return Pubkey(new_pubkey_pem).key_bytes()
    if security is not None and security.new_img_sign_pubkey_bytes is not None:
        return security.new_img_sign_pubkey_bytes
    raise ValueError("new_img_sign_pubkey: pass new_pubkey_pem or configure new_img_sign_pubkey in security.csv")


def _coerce_cli_bool_none(value):
    """Map CLI/subprocess strings to bool or None. Avoids bool('False') == True."""
    if value is None:
        return None
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        t = value.strip().lower()
        if t in ('none', 'null', ''):
            return None
        if t in ('true', '1', 'yes'):
            return True
        if t in ('false', '0', 'no'):
            return False
        logging.warning('Unrecognized bool-like string %r; treating as None', value)
        return None
    if isinstance(value, int) and not isinstance(value, bool):
        if value == 1:
            return True
        if value == 0:
            return False
    logging.warning('Unrecognized value for bool-like arg type=%s repr=%r; treating as None', type(value).__name__, value)
    return None

def _get_public_bytes_from_pubkeyfile(pubkey_file):
        if pubkey_file == None:
            return

        if not os.path.exists(pubkey_file):
            logging.warning(f'Public key file not found, skip loading key bytes: {pubkey_file}')
            return None

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

class PackBl2sign(Pack):

    def sign(self, img_sign_pubkey=None, img_sign_privkey=None, ota_type=None, security_counter=None, replace_key_en=None, new_pubkey_pem=None, new_privkey_pem=None, **kwargs):
        logging.debug(f'PackBl2sign: img_sign_pubkey={img_sign_pubkey}, img_sign_privkey={img_sign_privkey}, ota_type={ota_type}, security_counter={security_counter}, replace_key_en={replace_key_en}, new_privkey_pem={new_privkey_pem}, new_pubkey_pem={new_pubkey_pem}')
        l = len(self._bins)
        if l != 1:
            logging.error(f'Bin number should be 1, actual={l}')
            exit(1)
        
        signing_privkey = img_sign_privkey
        signing_pubkey = img_sign_pubkey
        signature_tlv = None
        root_pubkey_tlv = None

        replace_key_en = _coerce_cli_bool_none(replace_key_en)
        if replace_key_en:
            if not os.path.exists(new_pubkey_pem) or not os.path.exists(new_privkey_pem):
                raise ValueError(f"new_pubkey_pem or new_privkey_pem not found")

            signing_privkey = new_privkey_pem
            signing_pubkey = new_pubkey_pem
            if ota_type == 'XIP':
                new_pubkey_bytes = _get_public_bytes_from_pubkeyfile(new_pubkey_pem)
                root_pubkey_tlv = _get_public_bytes_from_pubkeyfile(img_sign_pubkey)
                bl2_sign_hash(img_sign_privkey, new_pubkey_bytes.hex(), 'new_pubkey_sig.json')
                with open('new_pubkey_sig.json', 'r') as f:
                    sig_data = json.load(f)
                    if isinstance(sig_data, str):
                        sig_data = json.loads(sig_data)
                    if 'signature' not in sig_data:
                        raise ValueError(f"Invalid signature JSON format: {sig_data}")
                    if isinstance(sig_data['signature'], str):
                        sig_der_hex = sig_data['signature']
                        signature_tlv = bytes.fromhex(sig_der_hex)
                    else:
                        raise ValueError(f"Invalid signature format: expected string, got {type(sig_data['signature'])}")
        
        signing_pubkey_bytes = _get_public_bytes_from_pubkeyfile(signing_pubkey)


        ph = PackHeader(self._bins[0])
        name = ph.name()
        infile = f'_{name}.bin'

        bl2_sign('sign', 'ec256', signing_privkey, signing_pubkey_bytes, None, infile, ph.size().sign_size(), ph.version(), security_counter, self._outfile, None, root_pubkey_tlv=root_pubkey_tlv, signature_tlv=signature_tlv)

        p = PackHeader()
        p.create_header(self._outfile, ph.version(), ph.name(), ph.type().type(),
            ph.type().subtype(), ph.size().offset(), ph.size().size(), ph.flags().flags(), 0, ph.m1(), ph.m2(), ph.m3(), ph.m4())
