#!/usr/bin/env python3

import os
import logging
import sys
from genericpath import exists
import click
import json
import struct

from version import get_version, version
from scripts.pack import Pack
from scripts.pack_all import PackAll
from scripts.pack_json import PackJson
from scripts.gen_code import gen_code

def set_debug(debug):
    if debug:
        logging.basicConfig()
        logging.getLogger().setLevel(logging.DEBUG)
        stream_handler = logging.StreamHandler(sys.stdout)
        stream_handler.setLevel(logging.DEBUG)
    else:
        logging.basicConfig()
        logging.getLogger().setLevel(logging.INFO)
        stream_handler = logging.StreamHandler(sys.stdout)
        stream_handler.setLevel(logging.INFO)

@click.group()
@click.version_option(version=get_version())
def cli():
    """Beken security tools"""
    pass

@cli.group("gen")
def gen():
    """Generate code"""

@cli.group("sign")
def sign():
    """Signing commands"""

@cli.group("pack")
def pack():
    """Pack commands"""

@cli.command("image_info")
@click.option("--loadfile", type=click.Path(exists=True, dir_okay=False), required=False, default='all-app.bin', help="Binary file to be checked.")
@click.option("--debug", is_flag=True, help="Enable debug")
def mage_info_command(loadfile, debug):
    """print image information"""
    set_debug(debug)
    mage_info_processing(loadfile)

@gen.command("partition")
@click.option("--partition_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='partitions.csv', help="partition CSV file.")
@click.option("--ota_type", type=click.Choice(['OVERWRITE', 'XIP']), default='OVERWRITE', required=True, help="The OTA type.")
@click.option("--out_hdr_file", type=str, required=False, default='partition_gen.h', help="Output file")
@click.option("--out_layout_file", type=str, required=False, default='partition_layout.h', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_partition_command(partition_csv, ota_type, out_hdr_file, out_layout_file, debug):
    """gen partition header and layout file."""
    set_debug(debug)
    p = Partitions(partition_csv, ota_type)
    gen_partitions_hdr_file(p, out_hdr_file)
    gen_partitions_layout_file(p, out_layout_file)

@gen.command("ppc")
@click.option("--ppc_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='ppc.csv', help="PPC CSV file.")
@click.option("--gpio_dev_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='gpio_dev.csv', help="GPIO map control config csv file.")
@click.option("--outfile", type=str, required=False, default='_ppc.h', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_ppc_command(ppc_csv, gpio_dev_csv, outfile, debug):
    """gen ppc.h from ppc.csv and gpio_dev.csv."""
    set_debug(debug)
    gen_ppc_config_file(ppc_csv, gpio_dev_csv, outfile)

@gen.command("mpc")
@click.option("--mpc_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='mpc.csv', help="MPC CSV file.")
@click.option("--outfile", type=str, required=False, default='_mpc.h', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_mpc_command(mpc_csv, outfile, debug):
    """gen mpc.h from mpc.csv."""
    set_debug(debug)
    gen_mpc_config_file(mpc_csv, outfile)

@gen.command("security")
@click.option("--security_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='security.csv', help="Security CSV file.")
@click.option("--outfile", type=str, required=False, default='security.h', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_security_command(security_csv, outfile, debug):
    """gen security.h from security.csv."""
    set_debug(debug)
    gen_security_config_file(security_csv, outfile)

@gen.command("ota")
@click.option("--ota_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='ota.csv', help="OTA CSV file.")
@click.option("--outfile", type=str, required=False, default='_ota.h', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_ota_command(ota_csv, outfile, debug):
    """gen ota.h from ota.csv."""
    set_debug(debug)
    gen_ota_config_file(ota_csv, outfile)

@gen.command("otp")
@click.option("--otp_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='otp2.csv', help="OTP CSV file.")
@click.option("--outfile", type=str, required=False, default='_otp.h', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_ota_command(otp_csv, outfile, debug):
    """gen otp.h from otp.csv."""
    set_debug(debug)
    gen_otp_map_file()

@gen.command("otp_efuse")
@click.option("--flash_aes_type", type=click.Choice(['FIXED', 'RANDOM', 'NONE']), default='FIXED', required=True, help="Flash AES type.")
@click.option("--flash_aes_key", type=str, required=False, default=None, help="flash AES key.")
@click.option("--pubkey_pem_file", type=click.Path(exists=False, dir_okay=False), required=False, default='root_ec256_pubkey.pem', help="PEM secure boot public key file.")
@click.option("--secure_boot", is_flag=True, help="Enable secure boot")
@click.option("--outfile", type=str, required=False, default='otp_efuse_config.json', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_otp_efuse_command(flash_aes_type, flash_aes_key, pubkey_pem_file, secure_boot, outfile, debug):
    """gen otp_efuse_config.json from security csv files."""
    set_debug(debug)
    gen_otp_efuse_config_file(flash_aes_type, flash_aes_key, pubkey_pem_file, secure_boot, False, outfile)

@gen.command("all")
@click.option("--debug", is_flag=True, help="Enable debug")
def pack_command(debug):
    """generate all code from security csv files"""
    set_debug(debug)
    gen_code()

@pack.command("compress")
@click.option("--infile", type=click.Path(exists=True, dir_okay=False), required=False, default='primary_all_code_signed.bin', help="Binary to be compressed.")
@click.option("--outfile", type=str, required=False, default='otp_efuse_config.json', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def compress_command(infile, outfile, debug):
    """compress OTA binary (overwrite only)"""
    set_debug(debug)
    compress_bin(infile, outfile)

@pack.command("insert_pk_hash")
@click.option("--bin", type=click.Path(exists=True, dir_okay=False), required=True, default='bootloader.bin', help="Binaries that contains public key hash.")
@click.option("--pubkey_pem_file", type=click.Path(exists=True, dir_okay=False), required=False, default='root_ec256_pubkey.pem', help="PEM public key file.")
@click.option("--debug", is_flag=True, help="Enable debug")
def pk_hash_command(bin, pubkey_pem_file, debug):
    """insert public key hash from binary"""
    set_debug(debug)
    insert_pk_hash(bin, pubkey_pem_file)

@pack.command("get_pk_hash")
@click.option("--pubkey_pem_file", type=click.Path(exists=True, dir_okay=False), required=False, default='root_ec256_pubkey.pem', help="PEM public key file.")
@click.option("--debug", is_flag=True, help="Enable debug")
def pk_hash_command(pubkey_pem_file, debug):
    """insert public key hash from binary"""
    set_debug(debug)
    get_pk_hash(pubkey_pem_file)

@pack.command("json")
@click.option("--debug", is_flag=True, help="Enable debug")
@click.option("--pack_json", type=click.Path(exists=True, dir_okay=False), required=False, default='pack.json', help="Pack json file.")
@click.option("--img_sign_privkey", type=str, required=False, default=None, help="Image sign private key file, PEM format")
@click.option("--flash_aes_key", type=str, required=False, default=None, help="flash aes key")
@click.option("--flash_crc_en", is_flag=True, default=True, help="Enable flash CRC")
@click.option("--data_aes_key", type=str, required=False, default=None, help="data aes key")
def pack_command(debug, pack_json, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key):
    """Pack according to the pack JSON"""
    set_debug(debug)
    p = PackJson()
    p.pack(pack_json, img_sign_privkey, flash_aes_key, flash_crc_en, data_aes_key)

@pack.command("all")
@click.option("--debug", is_flag=True, help="Enable debug")
def pack_command(debug):
    """Pack everything according to default configs"""
    set_debug(debug)
    p = PackAll()
    p.pack()

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setLevel(logging.DEBUG)
    os.environ["BKSECURE_PATH"] = os.getcwd()
    cli()
