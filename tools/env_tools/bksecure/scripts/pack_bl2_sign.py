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
from .steps import install_configs
from .common import get_config_paths, get_key_files_from_security_csv

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


class PackBl2sign(Pack):

    def pack(self, img_sign_pubkey=None, img_sign_privkey=None, flash_aes_key=None, flash_crc_en=True, data_aes_key=None):
        l = len(self._bins)
        if l != 1:
            logging.error(f'Bin number should be 1, actual={l}')
            exit(1)

        # Ensure config files are copied to current directory before proceeding
        # This is needed because PackBl2sign might be called from different paths
        cwd = os.getcwd()
        
        # Dynamically determine which key files are needed from security.csv
        # If security.csv exists, read key file names from it; otherwise, we'll copy configs
        security_csv_exists = os.path.exists('security.csv')
        key_files_dict = {}
        update_img_sign_key_en = False
        
        if security_csv_exists:
            # Extract key file names and update_img_sign_key_en from security.csv
            key_files_dict, update_img_sign_key_en = get_key_files_from_security_csv('security.csv')
        
        # Get list of key files to check
        key_files_to_check = [f for f in key_files_dict.values() if f]
        
        # Check if required key files exist
        missing_key_files = [f for f in key_files_to_check if not os.path.exists(f)]
        
        # If security.csv doesn't exist or key files are missing, try to copy config files
        if not security_csv_exists or missing_key_files:
            # Dynamically discover config paths
            config_paths = get_config_paths(current_dir=cwd)
            
            for cfg_dir in config_paths:
                if os.path.exists(cfg_dir):
                    install_configs(cfg_dir, cwd)
                    # Don't break, try all paths to ensure all files are copied
        
        # After copying, verify all required key files exist
        # If update_img_sign_key_en is TRUE, all 4 key files must exist
        if update_img_sign_key_en:
            required_fields = ['img_sign_pubkey', 'img_sign_privkey', 'new_img_sign_pubkey', 'new_img_sign_privkey']
            missing_required_files = []
            
            for field in required_fields:
                if field not in key_files_dict or not key_files_dict[field]:
                    missing_required_files.append(f"{field} (not specified in security.csv)")
                elif not os.path.exists(key_files_dict[field]):
                    missing_required_files.append(f"{key_files_dict[field]} (specified as {field})")
            
            if missing_required_files:
                error_msg = f"ERROR: update_img_sign_key_en=TRUE requires all 4 key files, but the following are missing:\n"
                for missing in missing_required_files:
                    error_msg += f"  - {missing}\n"
                error_msg += f"Current directory: {cwd}\n"
                error_msg += f"Please ensure all key files are present in the config directory."
                logging.error(error_msg)
                print(error_msg)
                exit(1)

        ph = PackHeader(self._bins[0])
        name = ph.name()
        infile = f'_{name}.bin'

        # For XIP mode with update_img_sign_key_en, setup trust chain verification
        # New TLV format:
        #   0xA0: new_img_sign_pubkey (new public key)
        #   0xA1: img_sign_pubkey (root public key)
        #   0xA2: signature of new_pubkey hash (signed by root key, DER format)
        root_pubkey_tlv = None  # For 0xA1: root public key
        signature_tlv = None  # For 0xA2: signature of new_pubkey hash (DER format)
        signing_privkey = img_sign_privkey  # Default to root key
        pubkey_for_0xa0 = None  # new_img_sign_pubkey bytes for 0xA0 (or root pubkey for normal mode)
        
        try:
            s = Security('security.csv')
            o = OTA('ota.csv')
            ota_type = o.get_strategy().upper()
            
            if s.update_img_sign_key_en and ota_type == 'XIP':
                # Use new_img_sign_privkey to sign app
                # Key files should already be in current directory (copied by install_configs)
                if not os.path.exists(s.new_img_sign_privkey):
                    raise ValueError(f"new_img_sign_privkey file not found: {s.new_img_sign_privkey}")
                signing_privkey = s.new_img_sign_privkey
                
                # Also resolve path for img_sign_privkey (root key) used for signing new_pubkey
                if not os.path.exists(s.img_sign_privkey):
                    raise ValueError(f"img_sign_privkey file not found: {s.img_sign_privkey}")
                resolved_root_privkey = s.img_sign_privkey
                
                # Verify that private key and public key are a matching pair
                if os.path.exists(s.img_sign_privkey) and os.path.exists(s.img_sign_pubkey):
                    keys_match = verify_key_pair_match(s.img_sign_privkey, s.img_sign_pubkey)
                    if not keys_match:
                        logging.warning(f"Root private key and public key DO NOT MATCH - signature verification will fail!")
                
                # Check if new_img_sign_pubkey_bytes is valid
                if s.new_img_sign_pubkey_bytes is None:
                    raise ValueError(f"new_img_sign_pubkey_bytes is None")
                
                # Check if img_sign_pubkey_bytes (root public key) is valid
                if s.img_sign_pubkey_bytes is None:
                    raise ValueError(f"img_sign_pubkey_bytes is None")
                
                # 0xA0: new_img_sign_pubkey (new public key)
                pubkey_for_0xa0 = s.new_img_sign_pubkey_bytes
                
                # 0xA1: img_sign_pubkey (root public key)
                root_pubkey_tlv = s.img_sign_pubkey_bytes
                
                # 0xA2: Generate signature of new_pubkey hash using root key
                new_pubkey_hash = hashlib.sha256(s.new_img_sign_pubkey_bytes).digest()
                bl2_sign_hash(resolved_root_privkey, new_pubkey_hash.hex(), 'new_pubkey_sig.json')
                
                # Read signature (DER format, hex string)
                with open('new_pubkey_sig.json', 'r') as f:
                    sig_data = json.load(f)
                    if isinstance(sig_data, str):
                        sig_data = json.loads(sig_data)
                    if 'signature' not in sig_data:
                        raise ValueError(f"Invalid signature JSON format: {sig_data}")
                    
                    # imgtool sign-hash returns DER format signature as hex string
                    if isinstance(sig_data['signature'], str):
                        sig_der_hex = sig_data['signature']
                        signature_tlv = bytes.fromhex(sig_der_hex)
                    else:
                        raise ValueError(f"Invalid signature format: expected string, got {type(sig_data['signature'])}")
            else:
                # Normal case: use root key for signing
                # Key files should already be in current directory (copied by install_configs)
                if img_sign_privkey and img_sign_pubkey:
                    # Verify key pair match in normal mode
                    if os.path.exists(img_sign_privkey) and os.path.exists(img_sign_pubkey):
                        keys_match_normal = verify_key_pair_match(img_sign_privkey, img_sign_pubkey)
                        if not keys_match_normal:
                            logging.warning(f"Root private key and public key DO NOT MATCH - signature verification will fail!")
                
                pk = Pubkey(img_sign_pubkey)
                pubkey_for_0xa0 = pk.key_bytes()
        except Exception as e:
            logging.debug(f"Failed to load security/ota config: {e}, continuing without trust chain")
            # Fallback to normal mode
            if pubkey_for_0xa0 is None:
                pk = Pubkey(img_sign_pubkey)
                pubkey_for_0xa0 = pk.key_bytes()
            # Reset TLV values to None in case of exception
            root_pubkey_tlv = None
            signature_tlv = None

        # Call bl2_sign with:
        # - signing_privkey: new_img_sign_privkey (for app signing) or img_sign_privkey (normal)
        # - pubkey_for_0xa0: new_img_sign_pubkey_bytes (for 0xA0) or root pubkey (normal)
        # - root_pubkey_tlv: root pubkey (for 0xA1, if update enabled)
        # - signature_tlv: signature of new_pubkey hash (for 0xA2, if update enabled)
        bl2_sign('sign', 'ec256', signing_privkey, pubkey_for_0xa0, None, infile, ph.size().sign_size(), ph.version(), ph.security_counter(), self._outfile, None, root_pubkey_tlv=root_pubkey_tlv, signature_tlv=signature_tlv)

        p = PackHeader()
        t = ph.type()
        p.create_header(self._outfile, ph.version(), ph.name(), ph.type().type(),
            ph.type().subtype(), ph.size().offset(), ph.size().size(), ph.flags().flags(), 0, ph.m1(), ph.m2(), ph.m3(), ph.m4())
