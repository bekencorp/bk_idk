#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Product ID Writer for Binary Files
Write product ID relative to boot magic_word

Usage:
    # Locate magic_word then write SPID at magic + spid_offset_in_boot
    python3 write_product_id.py app.bin T7QH4050
    
    # Read from product_id.txt file
    python3 write_product_id.py app.bin --product-id-file /path/to/product_id.txt
    
    # Write all zeros (clear product ID) - for non-target SOCs
    python3 write_product_id.py app.bin 0
    
    # Explicitly with product-ID CRC8: Write 16 bytes
    python3 write_product_id.py app.bin T7QH4050 --crc
    
    # Explicitly without product-ID CRC8: Write 15 bytes
    python3 write_product_id.py app.bin T7QH4050 --no-crc
    
    # With backup enabled
    python3 write_product_id.py app.bin T7QH4050 --backup
    
    # Custom absolute offset
    python3 write_product_id.py app.bin T7QH4050 --offset 0x200

Magic-based positioning:
    - Search for "BEKEN", "BK.SB", or "BK7236" in the binary
    - SPID write address = magic_offset + CONFIG_SPID_OFFSET_IN_BOOT (default 8)
    - Works for both secure/non-secure and crc/no_crc layouts (no fixed 0x100/0x110)

Special values:
    - Product ID "0" or empty string → writes all zeros (15 bytes of 0x00)

Default behavior:
    - Locate via magic_word; write 16 bytes (15-byte ID + 1-byte CRC8)
    - No backup (use --backup to enable)
"""

import sys
import os
import argparse

sys.dont_write_bytecode = True

from spid_resolve import get_spid_list_path, resolve_spid

# Global verbose flag
_verbose = False
DEFAULT_SPID_OFFSET_IN_BOOT = 8

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


def resolve_product_id_from_vendor(soc_name, spid_txt_path, spid_list_path=None):
    """Resolve product ID from vendor config and SOC mapping table."""
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    soc_name_lower = soc_name.strip().lower()
    effective_spid_list = (
        os.path.abspath(spid_list_path)
        if spid_list_path
        else get_spid_list_path(repo_root, soc_name_lower)
    )

    resolved = resolve_spid(
        soc_name=soc_name,
        spid_txt_path=os.path.abspath(spid_txt_path),
        spid_list_path=effective_spid_list,
    )
    selected = resolved.selected_entry

    if resolved.fallback_used:
        log_error(
            f"Warning: vendor '{resolved.requested_vendor_name}' is undefined for SOC {soc_name_lower}, "
            f"fallback to non-buck default '{selected.vendor_name}' ({selected.spid_value_raw})"
        )
    else:
        log(
            f"Vendor '{resolved.requested_vendor_name}' resolved as '{selected.vendor_name}' "
            f"({selected.spid_value_raw})"
        )
    return selected.spid_value_raw

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

MAGIC_STRINGS = (b'BEKEN', b'BK.SB', b'BK7236')


def find_magic_offset(bin_data):
    """Find boot magic_word offset used as SPID positioning base.

    Secure/non-secure and crc/no_crc images may place magic at different
    absolute offsets (e.g. after download headers/padding). SPID is always
    written at magic_offset + spid_offset_in_boot.

    Args:
        bin_data: Binary file data (bytes or bytearray)

    Returns:
        Absolute offset of magic_word, or None if not found
    """
    # Prefer common locations first for typical bootloader/app images
    for candidate in (0x100, 0x110):
        if len(bin_data) < candidate + 6:
            continue
        for magic in MAGIC_STRINGS:
            if bin_data[candidate:candidate + len(magic)] == magic:
                return candidate

    # Fallback: scan whole image (secure all-app.bin may shift magic)
    for magic in MAGIC_STRINGS:
        offset = bin_data.find(magic)
        if offset >= 0:
            return offset

    return None
def parse_int_value(value):
    """Parse integer string in decimal or hex format."""
    return int(value.strip(), 0)

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

def write_product_id_to_bin(bin_file, product_id, offset=None, backup=False, use_crc=None, spid_offset_in_boot=None):
    """Write product ID to binary file at specified offset
    
    Args:
        bin_file: Path to binary file
        product_id: Product ID string (ASCII format)
        offset: Absolute offset in binary file. If None, locate magic_word and use
                magic_offset + spid_offset_in_boot.
        backup: Create backup file before modification (default False)
        use_crc: Include product-ID CRC8 byte
                 None:  Always write 16 bytes (15-byte ID + 1-byte CRC8)
                 True:  Write 16 bytes (15-byte ID + 1-byte CRC8)
                 False: Write 15 bytes (15-byte ID only)
        spid_offset_in_boot: SPID offset relative to magic_word.
                             If None, uses default value 8.
    """
    if not os.path.exists(bin_file):
        raise FileNotFoundError(f"Binary file not found: {bin_file}")
    
    # Read original binary file
    with open(bin_file, 'rb') as f:
        bin_data = bytearray(f.read())
    
    file_size = len(bin_data)
    log(f"Binary file: {bin_file}")
    log(f"File size: {file_size} bytes (0x{file_size:X})")
    
    # Auto-select offset from magic_word if not specified
    if offset is None:
        magic_offset = find_magic_offset(bin_data)
        if magic_offset is None:
            log(f"No magic strings ({', '.join(m.decode() for m in MAGIC_STRINGS)}) detected.")
            log(f"Skipping product ID write operation.")
            return  # Exit without doing anything
        spid_offset = DEFAULT_SPID_OFFSET_IN_BOOT if spid_offset_in_boot is None else spid_offset_in_boot
        offset = magic_offset + spid_offset
        log(f"Magic found at 0x{magic_offset:X}, SPID offset in boot: 0x{spid_offset:X} "
            f"-> final 0x{offset:X}")
    
    # Product-ID CRC8: default on (do not couple to flash crc/no_crc layout)
    if use_crc is None:
        use_crc = True
    log(f"Product ID CRC8: {'Enabled' if use_crc else 'Disabled'}")
    log(f"Data size: {'16 bytes (15-byte ID + 1-byte CRC)' if use_crc else '15 bytes (ID only)'}")
    
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
        description='Write product ID relative to boot magic_word',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Locate magic_word, write SPID at magic + default offset 8
  %(prog)s app.bin T7QH4050
  
  # Read from product_id.txt file
  %(prog)s app.bin --product-id-file /path/to/product_id.txt
  
  # Write all zeros (clear product ID) - for non-target SOCs
  %(prog)s app.bin 0
  
  # Explicitly enable product-ID CRC8 (16 bytes)
  %(prog)s app.bin T7QH4050 --crc
  
  # Explicitly disable product-ID CRC8 (15 bytes)
  %(prog)s app.bin T7QH4050 --no-crc
  
  # Create backup before modification
  %(prog)s app.bin T7QH4050 --backup
  
  # Custom absolute offset
  %(prog)s app.bin T7QH4050 --offset 0x200
  
  # Custom absolute offset without product-ID CRC8, with backup
  %(prog)s app.bin ABC123 --offset 0x150 --no-crc --backup

Magic-based positioning:
  - Search for "BEKEN", "BK.SB", or "BK7236"
  - Write address = magic_offset + --spid-offset-in-boot (default 8)
  - Independent of flash crc/no_crc layout

Special values:
  - Product ID "0" or empty string → writes all zeros (15 bytes of 0x00)
  - Used automatically for non-target SOCs

Data format:
  Default / --crc: 16 bytes (15-byte ID + 1-byte CRC8)
  --no-crc:        15 bytes (ID only)
  
Backup:
  Default: No backup (use --backup to enable)
        """
    )
    
    parser.add_argument('bin_file', help='Path to binary file')
    parser.add_argument('product_id', nargs='?', default=None,
                        help='Product ID (ASCII string, max 15 characters). If not provided, must use --product-id-file')
    parser.add_argument('--product-id-file', type=str, default=None,
                        help='Path to product_id.txt file to read product ID from')
    parser.add_argument('--soc', type=str, default=None,
                        help='SOC name for vendor-based SPID resolving (e.g., bk7239n)')
    parser.add_argument('--spid-txt', type=str, default=None,
                        help='Path to vendor config spid.txt (e.g., properties/soc/spid.txt)')
    parser.add_argument('--spid-list', type=str, default=None,
                        help='Optional explicit path to per-SOC spid_list.csv')
    parser.add_argument('--offset', type=str, default=None,
                        help='Absolute offset in binary file (default: magic_offset + spid_offset_in_boot)')
    parser.add_argument('--spid-offset-in-boot', type=str, default=None,
                        help='SPID offset relative to magic_word '
                             '(default: 8)')
    
    # Product-ID CRC8 options (mutually exclusive; unrelated to flash crc layout)
    crc_group = parser.add_mutually_exclusive_group()
    crc_group.add_argument('--crc', action='store_true',
                          help='Explicitly enable product-ID CRC8 (write 16 bytes)')
    crc_group.add_argument('--no-crc', action='store_true',
                          help='Explicitly disable product-ID CRC8 (write 15 bytes)')
    
    parser.add_argument('--backup', action='store_true',
                        help='Create backup file before modification (default: no backup)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print detailed information (default: silent mode)')
    
    args = parser.parse_args()
    
    # Set global verbose flag
    global _verbose
    _verbose = args.verbose
    
    # Get product ID from vendor mapping, raw argument, or product_id.txt file
    product_id = args.product_id
    if args.soc and args.spid_txt:
        try:
            product_id = resolve_product_id_from_vendor(args.soc, args.spid_txt, args.spid_list)
            log(f"Product ID resolved from vendor mapping: '{product_id}'")
        except Exception as e:
            log_error(f"Error resolving vendor SPID: {e}")
            return 1
    elif product_id is None:
        if args.product_id_file is None:
            log_error(
                "Error: provide product_id, --product-id-file, or (--soc and --spid-txt)"
            )
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
            offset = parse_int_value(args.offset)
        except ValueError:
            log_error(f"Error: Invalid offset value '{args.offset}'")
            return 1

    spid_offset_in_boot = None
    if args.spid_offset_in_boot is not None:
        try:
            spid_offset_in_boot = parse_int_value(args.spid_offset_in_boot)
        except ValueError:
            log_error(f"Error: Invalid --spid-offset-in-boot value '{args.spid_offset_in_boot}'")
            return 1
    
    # Determine product-ID CRC8 mode (default: enabled)
    if args.crc:
        use_crc = True
    elif args.no_crc:
        use_crc = False
    else:
        use_crc = None
    
    try:
        write_product_id_to_bin(
            args.bin_file,
            product_id,
            offset=offset,
            backup=args.backup,
            use_crc=use_crc,
            spid_offset_in_boot=spid_offset_in_boot
        )
        return 0
    except Exception as e:
        log_error(f"Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())

