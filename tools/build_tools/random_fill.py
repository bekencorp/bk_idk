#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import argparse
import random

def merge_firmware(app_firmware_path, output_size_mb):
    output_size = output_size_mb * 1024 * 1024

    with open(app_firmware_path, 'rb') as f:
        app_data = f.read()

    app_size = len(app_data)

    merged_data = bytearray(output_size)
    merged_data[0:app_size] = app_data

    remaining_size = output_size - app_size

    random_bytes = bytearray(random.getrandbits(8) for _ in range(remaining_size))
    merged_data[app_size:output_size] = random_bytes

    app_name = os.path.splitext(os.path.basename(app_firmware_path))[0]
    output_dir = os.path.dirname(os.path.abspath(app_firmware_path))
    output_path = os.path.join(output_dir, f"{app_name}_merged_{output_size_mb}M.bin")

    with open(output_path, 'wb') as f:
        f.write(merged_data)

    print(f"Merge completed: {output_path} ({output_size_mb}MB)")
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Generate merged flash image from app bin',
        usage='python3 random_fill.py <app_firmware.bin> <2|4|8>',
        epilog='Examples:\n'
               '  python3 random_fill.py build/.../all-app.bin 4\n'
               '  python3 random_fill.py build/.../all-app.bin 8',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument('app_firmware', help='App firmware path')
    parser.add_argument('output_size', type=int, choices=[2, 4, 8], help='Output file size: 2, 4 or 8')

    args = parser.parse_args()
    merge_firmware(args.app_firmware, args.output_size)


if __name__ == '__main__':
    main()
