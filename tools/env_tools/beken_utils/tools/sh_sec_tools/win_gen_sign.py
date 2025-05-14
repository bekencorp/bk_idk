import os
import sys
import argparse
from cryptography.hazmat.primitives.asymmetric import rsa, ec
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
import warnings
 
warnings.filterwarnings("ignore")

def load_private_key(key_path):
    """from PEM file load private key"""
    try:
        with open(key_path, 'rb') as key_file:
            private_key = serialization.load_pem_private_key(
                key_file.read(),
                password=None,
                backend=default_backend()
            )
        return private_key
    except Exception as e:
        print(f"Error loading private key: {e}")
        sys.exit(1)

def sign_file(private_key, file_path):
    try:
        with open(file_path, 'rb') as f:
            file_content = f.read()
        
        if isinstance(private_key, rsa.RSAPrivateKey):
            raise ValueError("private key type error, need curve private key.")
        elif isinstance(private_key, ec.EllipticCurvePrivateKey):
            signature = private_key.sign(
                file_content,
                ec.ECDSA(hashes.SHA256())
            )
            r, s = decode_dss_signature(signature)
            return {
                'signature': signature,
                'r': r,
                's': s,
                'algorithm': f'ECDSA-{private_key.curve.name}-SHA256'
            }
        else:
            raise ValueError("Unsupported private key type")
        
    except Exception as e:
        print(f"Error signing file: {e}")
        sys.exit(1)

def save_signature(signature_data, output_path):
    try:      
        # save raw signature
        raw_output_path = 'raw_' + output_path
        with open(raw_output_path, 'wb') as f:
            f.write(signature_data['signature'])
        print(f"Raw Signature saved to {raw_output_path}")
        
        # save r and s value signature.
        with open(output_path, 'wb') as f:
            r_hex = hex(signature_data['r'])[2:].zfill(64)
            s_hex = hex(signature_data['s'])[2:].zfill(64)
            r_value = bytes.fromhex(r_hex)
            s_value = bytes.fromhex(s_hex)
            f.write(r_value)
            f.write(s_value)
        print(f"Signature(R/S) saved to {output_path}")
        print(f"r: {r_hex}")
        print(f"s: {s_hex}")

    except Exception as e:
        print(f"Error saving signature: {e}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description='Sign a file using a private key and extract signature components')
    parser.add_argument('--private-key', required=True, help='Path to private key file')
    parser.add_argument('--input-file', required=True, help='Path to file to sign')
    parser.add_argument('--output', required=True, help='Path to save signature (base filename)')
    
    args = parser.parse_args()
    
    # check dependency files
    if not os.path.exists(args.private_key):
        print(f"Private key file {args.private_key} does not exist")
        sys.exit(1)
    
    if not os.path.exists(args.input_file):
        print(f"Input file {args.input_file} does not exist")
        sys.exit(1)
    
    # load private key
    private_key = load_private_key(args.private_key)
    
    # get file signature
    signature_data = sign_file(private_key, args.input_file)
    
    # save signature
    save_signature(signature_data, args.output)

if __name__ == "__main__":
    main()
    # python win_gen_sign.py --private-key private_key.pem --input-file manifest.bin --output signature.bin