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
from scripts.steps import *

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
@click.option("--flash_crc_en", type=click.Choice(['True', 'False', 'true', 'false']), required=True, default=None, help="Enable flash CRC (True/False). If not specified, read from security.csv or default to True")
@click.option("--flash_aes_key", type=str, required=False, default=None, help="flash AES key.")
@click.option("--pubkey_pem_file", type=click.Path(exists=False, dir_okay=False), required=False, default='root_ec256_pubkey.pem', help="PEM secure boot public key file.")
@click.option("--secure_boot", is_flag=True, help="Enable secure boot")
@click.option("--outfile", type=str, required=False, default='otp_efuse_config.json', help="Output file")
@click.option("--debug", is_flag=True, help="Enable debug")
def gen_otp_efuse_command(flash_aes_type, flash_crc_en, flash_aes_key, pubkey_pem_file, secure_boot, outfile, debug):
    """gen otp_efuse_config.json from security csv files."""
    set_debug(debug)
    gen_otp_efuse_config_file(flash_aes_type, flash_crc_en, flash_aes_key, pubkey_pem_file, secure_boot, False, outfile)

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

@cli.group("steps")
def steps():
    """Create downloadable bin step by step"""

@steps.command("get_app_bin_hash")
@click.option("--debug", is_flag=True, help="Enable debug")
def get_bin_hash_command(debug):
    """Get binaries hash for signing"""
    set_debug(debug)
    get_app_bin_hash()

@steps.command("sign_app_bin_hash")
@click.option("--bl2_bin_hash", type=str, required=False, default=None, help="hash of BL2 binaries")
@click.option("--bl2_b_bin_hash", type=str, required=False, default=None, help="hash of BL2_B binaries")
@click.option("--app_bin_hash", type=str, required=False, default=None, help="hash of APP binaries")
@click.option("--debug", is_flag=True, help="Enable debug")
def sign_hash_command(bl2_bin_hash, bl2_b_bin_hash, app_bin_hash, debug):
    """Signing app binaries hash"""
    set_debug(debug)
    sign_app_bin_hash(bl2_bin_hash, app_bin_hash)

@steps.command("sign_from_app_sig")
@click.option("--bl2_sig_r", type=str, required=False, default=None, help="BL2 sig r")
@click.option("--bl2_sig_s", type=str, required=False, default=None, help="BL2 sig s")
@click.option("--bl2_b_sig_r", type=str, required=False, default=None, help="BL2_B sig r")
@click.option("--bl2_b_sig_s", type=str, required=False, default=None, help="BL2_B sig s")
@click.option("--app_sig", type=str, required=False, default=None, help="APP sig")
@click.option("--debug", is_flag=True, help="Enable debug")
def sign_from_sig_command(bl2_sig_r, bl2_sig_s, bl2_b_sig_r, bl2_b_sig_s, app_sig, debug):
    """Create signed app bin based on signature"""
    set_debug(debug)
    # sign_from_app_sig(bl2_sig_r, bl2_sig_s, bl2_b_sig_r, bl2_b_sig_s, app_sig)
    sign_from_app_sig(bl2_sig_r, bl2_sig_s, app_sig)

@steps.command("get_ota_bin_hash")
@click.option("--debug", is_flag=True, help="Enable debug")
def get_ota_bin_hash_command(debug):
    """Get OTA binary hash"""
    set_debug(debug)
    get_ota_bin_hash()

@steps.command("sign_ota_bin_hash")
@click.option("--ota_bin_hash", type=str, required=False, default=None, help="hash of OTA bin")
@click.option("--debug", is_flag=True, help="Enable debug")
def sign_ota_bin_hash_command(ota_bin_hash, debug):
    """Create signature of OTA bin hash"""
    set_debug(debug)
    sign_ota_bin_hash(ota_bin_hash)

@steps.command("sign_from_ota_sig")
@click.option("--ota_bin_sig", type=str, required=False, default=None, help="OTA bin sig")
@click.option("--debug", is_flag=True, help="Enable debug")
def sign_from_ota_sig_command(ota_bin_sig, debug):
    """Create signed OTA bin from signature"""
    set_debug(debug)
    sign_from_ota_sig(ota_bin_sig)

@steps.command("pack")
@click.option("--debug", is_flag=True, help="Enable debug")
@click.option("--flash_aes_type", type=click.Choice(['NONE', 'FIXED', 'RANDOM']), default='FIXED', required=True, help="The flash AES type.")
@click.option("--flash_aes_key", type=str, required=False, default=None, help="configuration flash aes key")
@click.option("--flash_crc_en", type=click.Choice(['True', 'False', 'true', 'false']), required=False, default=None, help="Enable flash CRC (True/False). If not specified, read from security.csv or default to True")
@click.option("--bl1_secureboot_en", is_flag=True, default=False, help="Indicate whether enable BL1 secure boot")
@click.option("--ota_type", type=click.Choice(['XIP', 'OVERWRITE']), help="OTA strategy")
@click.option("--ota_security_counter", type=int, help="APP security counter")
@click.option("--boot_ota", is_flag=True, default=False, help="Indicate whether support bootloader OTA")
@click.option("--ota_encrypt", is_flag=True, default=False, help="Indicate whether ota.bin is encrypted")
@click.option("--pubkey_pem", type=str, default='root_ec256_pubkey.pem', help="BL2 public key PEM file")
@click.option("--privkey_pem", type=str, default='root_ec256_privkey.pem', help="BL2 private key PEM file")
@click.option("--bl2_version", type=str, required=True, default=None, help="configuration bl2 version")
@click.option("--app_version", type=str, required=True, default=None, help="configuration app version")
def pack_command(debug, flash_aes_type, flash_aes_key, flash_crc_en, bl1_secureboot_en, ota_type, ota_security_counter, ota_encrypt, boot_ota, bl2_version, app_version, pubkey_pem, privkey_pem):
    """Pack download bin"""
    set_debug(debug)
    # Convert flash_crc_en string to boolean if provided
    flash_crc_en_bool = None
    if flash_crc_en is not None:
        flash_crc_en_bool = flash_crc_en.lower() == 'true'
    steps_pack(flash_aes_type, flash_aes_key, flash_crc_en_bool, bl1_secureboot_en, ota_type, ota_security_counter, ota_encrypt, boot_ota, bl2_version, app_version, pubkey_pem, privkey_pem)

@steps.command("pack_csv")
@click.option("--debug", is_flag=True, help="Enable debug")
def pack_csv_command(debug):
    """Pack download bin according to CSV file"""
    set_debug(debug)
    steps_pack_csv()

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setLevel(logging.DEBUG)
    os.environ["BKSECURE_PATH"] = os.getcwd()
    cli()
