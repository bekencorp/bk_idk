#!/usr/bin/env python3

import logging
from argparse import _MutuallyExclusiveGroup
from genericpath import exists
import click
from click_option_group import optgroup,RequiredMutuallyExclusiveOptionGroup

import struct
from scripts import create
from scripts import merge_tlv
from scripts import partition
from scripts.partition import *
from scripts import gen
from scripts.gen import *
from scripts import revert
from scripts import ascii
from scripts import dump
from scripts import copy
from scripts import modify
from scripts import check_sha384
from scripts import crc
from scripts import encrypt
from scripts import xts_aes
from scripts import generator

@click.group()
@click.version_option(version='1.0.0.0')
def cli():
    """This script showcases multi-scripts on operating flash."""
    pass

@cli.command("merge_tlv")
@click.option("-j", "--json_file", type=click.Path(exists=True, dir_okay=False),required=True, help="input a json file.")
@click.option("-o", "--outfile", type=click.Path(exists=False, dir_okay=False), required=True, help="The outfile name.")
@click.option("--debug/--no-debug", default=True, help="Enable debug options.")
def merge_tlv_bin(json_file, outfile, debug):
    """merge a new bin file from json file."""
    if (debug == True):
        logging.basicConfig()
        logging.getLogger().setLevel(logging.DEBUG)
    merge_tlv(json_file, outfile)
    logging.debug(f"merge success")

@cli.command("gen")
@click.option("-t", "--tools_dir", type=click.Path(exists=True, dir_okay=True), required=True, help="partition dependency tools dir.")
@click.option("-p", "--partition_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='partitions.csv', help="partition config csv file.")
@click.option("-s", "--security_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='security.csv', help="security config csv file.")
@click.option("-o", "--ota_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='ota.csv', help="security config csv file.")
@click.option("-m", "--mpc_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='mpc.csv', help="memory protection control config csv file.")
@click.option("-d", "--ppc_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='ppc.csv', help="peripheral protection control config csv file.")
@click.option("-g", "--gpio_dev_csv", type=click.Path(exists=True, dir_okay=False), required=False, default='gpio_dev.csv', help="gpio map control config csv file.")
@click.option("--genbin/--no-genbin", default=False, help="Generate binaries or not.")
@click.option("--gencode/--no-gencode", default=False, help="Generate code or not.")
@click.option("--debug/--no-debug", default=True, help="Enable debug option.")

def gen_process(tools_dir, partition_csv, security_csv, ota_csv, mpc_csv, ppc_csv, gpio_dev_csv, genbin=False, gencode=False, debug=True):
    """partitions processing."""

    if (debug == True):
        logging.basicConfig()
        logging.getLogger().setLevel(logging.DEBUG)

    p = Gen(tools_dir, partition_csv, security_csv, ota_csv, mpc_csv, ppc_csv, gpio_dev_csv, genbin, gencode, debug)

    if (gencode):
        logging.debug("start to generate code...")
        p.gen_code()

    if (genbin):
        logging.debug("start to generate binaries...")
        p.gen_bin()

@cli.command("crc")
@click.option("-i", "--infile", type=click.Path(exists=True, dir_okay=False), required=True, help="input a bin file.")
@click.option("-o", "--outfile", type=click.Path(dir_okay=False),default="outfile_crc.bin", help="The crc outfile name.", show_default=True)
def crc_bin(infile, outfile):
    """generate a crc file from the origin file."""
    crc(infile, outfile)

@cli.command("encrypt")
@click.option("-i", "--infile", type=click.Path(exists=True, dir_okay=False), required=True, help="input a bin file.")
@click.option("-k", "--keywords", nargs=4, type=str, help="input 4 keywords,such as '0 0 0 0'")
@click.option("-s","--start_address",type=str,default = "0",show_default = True,help="The start_address of encryption.")
@click.option("--un_skip/--skip",default=True,help="Makes a checkout un_skip or skip keyword,such as 'hex = bk7231n'.  un_skip by default.",)
@click.option("-o", "--outfile", type=click.Path(dir_okay=False),default="outfile_enc.bin", show_default=True,help="The encrypt outfile name.")
@click.option("-d", "--debug", is_flag=True,help="generate *.cpr & *.out files.")
def encrypt_bin(infile, keywords, start_address,un_skip,outfile,debug):
    """generate a encrypt file from the origin file and keywords"""
    enc = encrypt(infile, keywords, start_address,un_skip,outfile,debug)
    enc.func_enc()

@cli.group("aes")
def aes():
    """xts_aes,include generate keyfile and xts-aes to encrypt bin file ."""
@aes.command("genkey")
@click.option("-v", "--version", type=str, default="1", help="keyfile version.")
@click.option("-a","--aes_cbc",is_flag=True,default=True,show_default=True,help="create ota_key and ota_iv value",)
@click.option("-o", "--outfile", type=click.Path(dir_okay=False),default="genkey.txt", show_default=True,help="The generate key outfile name.")
def gen_bin(version,aes_cbc,outfile):
    """generate a key file ,include xts_aes / OTA_key / OTA_IV"""
    enc = generator(version,aes_cbc,outfile)
    enc.func_enc()

@aes.command("aes")
@click.option("-i", "--infile", type=click.Path(exists=True, dir_okay=False), required=True, help="input a bin file.")
@click.option("-a","--all_quick",is_flag=True,help="quickly generate xts_aes bin file.")
@click.option("-s","--start_address",type=str,default = "0",show_default = True,help="The start_address of encryption.")
@click.option("-o", "--outfile", type=click.Path(dir_okay=False),default="xts_aes.bin", show_default=True,help="The xts_aes outfile name.")
@optgroup.group('source data options',cls=RequiredMutuallyExclusiveOptionGroup,help='choice keywords or keyfile')
@optgroup.option("-k", "--keywords", type=str,help="input keywords,ciphertext or plaintext .")
@optgroup.option("-f", "--keyfile", type=click.Path(exists=True, dir_okay=False), help="input keyfile path.")
def aes_bin(infile,all_quick,start_address,keywords,keyfile,outfile):
    """encrypt bin file from the origin file and keywords or keyfile"""
    enc = xts_aes(infile,all_quick,start_address,keywords,keyfile,outfile)
    enc.func_enc()

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.INFO)
    cli()
