#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Product ID Writer for Binary Files
Write product ID to binary file with CRC auto-detection

Usage:
    # Auto-detect CRC mode (checks for magic strings, no backup by default)
    python3 write_product_id.py app.bin T7QH4050
    
    # Read from product_id.txt file
    python3 write_product_id.py app.bin --product-id-file /path/to/product_id.txt
    
    # Write all zeros (clear product ID) - for non-target SOCs
    python3 write_product_id.py app.bin 0
    
    # Explicitly with CRC: Write 16 bytes to 0x118
    python3 write_product_id.py app.bin T7QH4050 --crc
    
    # Explicitly without CRC: Write 15 bytes to 0x108
    python3 write_product_id.py app.bin T7QH4050 --no-crc
    
    # With backup enabled
    python3 write_product_id.py app.bin T7QH4050 --backup
    
    # Custom offset
    python3 write_product_id.py app.bin T7QH4050 --offset 0x200

Auto-detection:
    - Detects "BEKEN", "BK.SB", or "BK7236" at 0x100 → no_crc mode
    - Detects "BEKEN", "BK.SB", or "BK7236" at 0x110 → crc mode
    - If none found, skips write operation

Special values:
    - Product ID "0" or empty string → writes all zeros (15 bytes of 0x00)

Default behavior:
    - Auto-detect CRC mode
    - No backup (use --backup to enable)
"""

import sys
import os
import argparse

# Global verbose flag
_verbose = False

def log(*args, **kwargs):
    """Print function wrapper - only prints if verbose mode is enabled"""
    if _verbose:
        print(*args, **kwargs)

def log_error(*args, **kwargs):
    """Print error messages - always prints regardless of verbose mode"""
    print(*args, **kwargs, file=sys.stderr)

def read_product_id_from_file(product_id_file):
    """Read product ID from specified file
    
    Args:
        product_id_file: Path to product_id.txt file
    
    Returns:
        Product ID string from product_id.txt
        Returns "0" if file is empty or contains "0" (indicating zero product ID)
    """
    if not os.path.exists(product_id_file):
        raise FileNotFoundError(f"product_id.txt not found at {product_id_file}")
    
    with open(product_id_file, 'r') as f:
        product_id = f.read().strip()
    
    # Empty file or "0" means write all zeros
    if not product_id or product_id == "0":
        return "0"
    
    return product_id

def calculate_crc8(data):
    """Calculate CRC8 for the given data
    
    Algorithm matches CheckSumUtils.c implementation:
    - Initial value: 0
    - Polynomial: 0x8C
    - Right shift (LSB first)
    
    Args:
        data: bytes array (15 bytes)
    
    Returns:
        CRC8 value (1 byte)
    """
    crc = 0  # Initial value: 0 (not 0xFF)
    polynomial = 0x8C  # Polynomial: 0x8C (not 0x07)
    
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x01:  # Check LSB (not MSB)
                crc = (crc >> 1) ^ polynomial  # Right shift (not left shift)
            else:
                crc >>= 1
    
    return crc

def detect_crc_mode(bin_data):
    """Auto-detect CRC mode by checking for magic strings at specific offsets
    
    Check for "BEKEN", "BK.SB", or "BK7236" at:
    - 0x100: Indicates no_crc mode
    - 0x110: Indicates crc mode
    
    Args:
        bin_data: Binary file data (bytes or bytearray)
    
    Returns:
        True if CRC mode detected, False if no_crc mode detected, None if cannot detect
    """
    magic_strings = [b'BEKEN', b'BK.SB', b'BK7236']
    
    # Check for no_crc mode (magic string at 0x100)
    if len(bin_data) >= 0x100 + 6:  # Need at least 6 bytes for "BK7236"
        for magic in magic_strings:
            if bin_data[0x100:0x100 + len(magic)] == magic:
                return False  # no_crc mode
    
    # Check for crc mode (magic string at 0x110)
    if len(bin_data) >= 0x110 + 6:  # Need at least 6 bytes for "BK7236"
        for magic in magic_strings:
            if bin_data[0x110:0x110 + len(magic)] == magic:
                return True  # crc mode
    
    return None  # Cannot detect

def ascii_to_bytes(ascii_str, length=15):
    """Convert ASCII string to bytes array with padding
    
    Args:
        ascii_str: ASCII string (e.g., "T7QH4050")
                   Special value "0" or empty string: returns all zeros
        length: Target length (default 15 bytes)
    
    Returns:
        bytes array of specified length
    """
    # Special case: "0" or empty string means all zeros
    if ascii_str == "0" or ascii_str == "":
        return bytes(length)  # All zeros
    
    if len(ascii_str) > length:
        raise ValueError(f"ASCII string length exceeds {length} characters: {len(ascii_str)} characters")
    
    # Convert ASCII string to bytes (big-endian order, matching OTP storage)
    id_bytes = bytearray(length)
    for i, char in enumerate(ascii_str):
        id_bytes[i] = ord(char)
    
    # Remaining bytes are already 0 (default value of bytearray)
    return bytes(id_bytes)

def write_product_id_to_bin(bin_file, product_id, offset=None, backup=False, use_crc=None):
    """Write product ID to binary file at specified offset
    
    Args:
        bin_file: Path to binary file
        product_id: Product ID string (ASCII format)
        offset: Offset address in binary file (if None, auto-select based on use_crc)
        backup: Create backup file before modification (default False)
        use_crc: Include CRC byte (None for auto-detect, True for CRC, False for no CRC)
                 None:  Auto-detect by checking magic strings at 0x100 (no_crc) or 0x110 (crc)
                 True:  Write 16 bytes (15-byte ID + 1-byte CRC) to 0x118
                 False: Write 15 bytes (15-byte ID only) to 0x108
    """
    if not os.path.exists(bin_file):
        raise FileNotFoundError(f"Binary file not found: {bin_file}")
    
    # Read original binary file
    with open(bin_file, 'rb') as f:
        bin_data = bytearray(f.read())
    
    file_size = len(bin_data)
    log(f"Binary file: {bin_file}")
    log(f"File size: {file_size} bytes (0x{file_size:X})")
    
    # Auto-detect CRC mode if not specified
    if use_crc is None:
        detected_mode = detect_crc_mode(bin_data)
        if detected_mode is None:
            log(f"No magic strings detected at 0x100 or 0x110.")
            log(f"Skipping product ID write operation.")
            return  # Exit without doing anything
        use_crc = detected_mode
        log(f"CRC mode: Auto-detected as {'Enabled' if use_crc else 'Disabled'} "
            f"(magic string found at 0x{'110' if use_crc else '100'})")
    else:
        log(f"CRC mode: {'Enabled' if use_crc else 'Disabled'} (manually specified)")
    
    log(f"Data size: {'16 bytes (15-byte ID + 1-byte CRC)' if use_crc else '15 bytes (ID only)'}")
    
    # Auto-select offset based on CRC option if not specified
    if offset is None:
        offset = 0x118 if use_crc else 0x108
    
    # Convert ASCII string to 15 bytes
    id_bytes = ascii_to_bytes(product_id, 15)
    
    if use_crc:
        # Calculate CRC for the 15-byte ID
        crc_byte = calculate_crc8(id_bytes)
        # Combine ID + CRC to form 16 bytes
        data_to_write = id_bytes + bytes([crc_byte])
        data_length = 16
    else:
        # No CRC, only 15 bytes
        data_to_write = id_bytes
        data_length = 15
    
    # Check if offset is valid
    if offset + data_length > file_size:
        raise ValueError(f"Offset 0x{offset:X} + {data_length} bytes exceeds file size 0x{file_size:X}")
    
    # Display information
    if product_id == "0" or product_id == "":
        log(f"\nProduct ID: All zeros (clearing product ID)")
    else:
        log(f"\nProduct ID (ASCII): '{product_id}'")
    log(f"Product ID (15 bytes): {' '.join(f'{b:02X}' for b in id_bytes)}")
    
    if use_crc:
        log(f"CRC byte: 0x{crc_byte:02X}")
        log(f"Complete ID (16 bytes): {' '.join(f'{b:02X}' for b in data_to_write)}")
        log(f"\nWrite offset: 0x{offset:X} - 0x{offset + 15:X} (16 bytes)")
    else:
        log(f"Complete ID (15 bytes, no CRC): {' '.join(f'{b:02X}' for b in data_to_write)}")
        log(f"\nWrite offset: 0x{offset:X} - 0x{offset + 14:X} (15 bytes)")
    
    # Create backup if requested
    if backup:
        backup_file = bin_file + '.backup'
        with open(backup_file, 'wb') as f:
            f.write(bin_data)
        log(f"Backup created: {backup_file}")
    
    # Write product ID to binary file at specified offset
    for i, byte in enumerate(data_to_write):
        bin_data[offset + i] = byte
    
    # Write modified binary file
    with open(bin_file, 'wb') as f:
        f.write(bin_data)
    
    log(f"\n✓ Product ID successfully written to {bin_file}")
    
    # Verify the write
    with open(bin_file, 'rb') as f:
        f.seek(offset)
        written_data = f.read(data_length)
    
    if written_data == data_to_write:
        log("✓ Verification passed!")
    else:
        log_error("✗ Verification failed!")
        log_error(f"Expected: {' '.join(f'{b:02X}' for b in data_to_write)}")
        log_error(f"Read:     {' '.join(f'{b:02X}' for b in written_data)}")
        raise ValueError("Verification failed: written data does not match expected data")

def main():
    """Main function"""
    parser = argparse.ArgumentParser(
        description='Write product ID to binary file with optional CRC (auto-detect by default)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Auto-detect CRC mode (checks for magic strings at 0x100 or 0x110)
  %(prog)s app.bin T7QH4050
  
  # Read from product_id.txt file
  %(prog)s app.bin --product-id-file /path/to/product_id.txt
  
  # Write all zeros (clear product ID) - for non-target SOCs
  %(prog)s app.bin 0
  
  # Explicitly enable CRC (16 bytes to 0x118)
  %(prog)s app.bin T7QH4050 --crc
  
  # Explicitly disable CRC (15 bytes to 0x108)
  %(prog)s app.bin T7QH4050 --no-crc
  
  # Create backup before modification
  %(prog)s app.bin T7QH4050 --backup
  
  # Custom offset with auto-detect
  %(prog)s app.bin T7QH4050 --offset 0x200
  
  # Custom offset without CRC, with backup
  %(prog)s app.bin ABC123 --offset 0x150 --no-crc --backup

Auto-detection:
  - Checks for "BEKEN", "BK.SB", or "BK7236" at 0x100 → no_crc mode
  - Checks for "BEKEN", "BK.SB", or "BK7236" at 0x110 → crc mode
  - If none found, skips write operation

Special values:
  - Product ID "0" or empty string → writes all zeros (15 bytes of 0x00)
  - Used automatically for non-target SOCs

Address mapping:
  With CRC (--crc):       0x118 - 0x127 (16 bytes: 15-byte ID + 1-byte CRC)
  Without CRC (--no-crc): 0x108 - 0x116 (15 bytes: ID only)
  
Backup:
  Default: No backup (use --backup to enable)
        """
    )
    
    parser.add_argument('bin_file', help='Path to binary file')
    parser.add_argument('product_id', nargs='?', default=None,
                        help='Product ID (ASCII string, max 15 characters). If not provided, must use --product-id-file')
    parser.add_argument('--product-id-file', type=str, default=None,
                        help='Path to product_id.txt file to read product ID from')
    parser.add_argument('--offset', type=str, default=None,
                        help='Offset address in binary file (default: auto-select based on CRC option)')
    
    # CRC mode options (mutually exclusive)
    crc_group = parser.add_mutually_exclusive_group()
    crc_group.add_argument('--crc', action='store_true',
                          help='Explicitly enable CRC (write 16 bytes to 0x118)')
    crc_group.add_argument('--no-crc', action='store_true',
                          help='Explicitly disable CRC (write 15 bytes to 0x108)')
    
    parser.add_argument('--backup', action='store_true',
                        help='Create backup file before modification (default: no backup)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print detailed information (default: silent mode)')
    
    args = parser.parse_args()
    
    # Set global verbose flag
    global _verbose
    _verbose = args.verbose
    
    # Get product ID from argument or product_id.txt file
    product_id = args.product_id
    if product_id is None:
        if args.product_id_file is None:
            log_error("Error: Either provide product_id as argument or use --product-id-file")
            return 1
        
        try:
            product_id = read_product_id_from_file(args.product_id_file)
            log(f"Product ID read from {args.product_id_file}: '{product_id}'")
        except Exception as e:
            log_error(f"Error reading from {args.product_id_file}: {e}")
            return 1
    
    # Parse offset (support both hex and decimal)
    offset = None
    if args.offset is not None:
        try:
            if args.offset.startswith('0x') or args.offset.startswith('0X'):
                offset = int(args.offset, 16)
            else:
                offset = int(args.offset)
        except ValueError:
            log_error(f"Error: Invalid offset value '{args.offset}'")
            return 1
    
    # Determine CRC mode
    if args.crc:
        use_crc = True
    elif args.no_crc:
        use_crc = False
    else:
        use_crc = None  # Auto-detect
    
    try:
        write_product_id_to_bin(
            args.bin_file,
            product_id,
            offset=offset,
            backup=args.backup,
            use_crc=use_crc
        )
        return 0
    except Exception as e:
        log_error(f"Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())

