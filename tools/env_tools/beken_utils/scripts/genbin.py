#!/usr/bin/env python3
import os
import logging
import csv

from .security import *
from .ota import *
from .ppc import *
from .mpc import *
from .partition import *
from .genbl1 import *

class Genbin:
    def is_legacyboot(self):
        return (self.security.secureboot_en == False)

    def gen_manifest(self, g, manifest_partition):
        boot_partition_name = manifest_partition.partition_name.replace("manifest", "bl2")
        p = self.partition.find_partition_by_name("bl2")
        if (p == None):
            logging.debug(f'boot partition {boot_partition_name} not exists')
            exit(1)
        logging.debug(f'generate manifest, bin_name={p.bin_name}')
        manifest_name = f'{manifest_partition.partition_name}.json'
        security_counter = self.ota.get_bl2_security_counter()
        if security_counter == None:
            logging.error(f'Failed to find SecurityCounter of {p.bin_name} in ota.csv, please set it')
            exit(1)
        g.gen_manifest(security_counter, p.static_addr, p.load_addr, p.bin_name, manifest_name)

    def gen_bl1_config(self):
        if self.is_legacyboot():
            return

        g = Genbl1(True, self.security.bl1_root_key_type, self.security.bl1_root_privkey, self.security.bl1_root_pubkey)
        g.gen_key_desc()
        pre_partition_manifest_type = 0
        for p in self.partition.partitions:
            if (p.partition_name == 'primary_manifest'):
                self.gen_manifest(g, p)
                self.primary_manifest_exists = True
            elif (p.partition_name == 'secondary_manifest'):
                self.gen_manifest(g, p)
                self.secondary_manifest_exists = True

    def gen_manifest_bin(self, manifest_json_file_name):
        if self.is_legacyboot():
            return

        manifest_bin_name = manifest_json_file_name.replace("json", "bin")
        sh_sec_tools = f'{self.tools_dir}/sh_sec_tools/secure_boot_tool'
        pwd = os.getcwd()
        cmd = f'{sh_sec_tools} -k key_desc.json -m {manifest_json_file_name} -o {pwd}/'
        run_cmd(cmd)
        cmd = f'mv manifest.bin {manifest_bin_name}'
        run_cmd(cmd)

    def gen_bl1_control_bin(self):
        if self.is_legacyboot():
            return

        #if (os.path.exists(f'puk_digest.bin') == True):
        #    logging.debug(f'Generate BL1 bl1_control.bin')
        run_cmd(f'{self.tools_dir}/beken_utils/main.py merge_tlv -j bl1_control.json -o bl1_control.bin')
        #else:
        #    logging.debug(f'Skip generate BL1 bl1_control.bin')

       
    def gen_bin(self):
        self.gen_bl1_config()
        self.gen_bl1_control_bin()
        self.partition.process_bl1_boot()
        self.gen_manifest_bin('primary_manifest.json')
        if self.secondary_manfiest_exists:
            self.gen_manifest_bin('secondary_manifest.json')
        self.partition.parse_and_validate_partitions_after_build()
        self.partition.gen_bin()

    def __init__(self, tools_dir, partition, security, ota, ppc, mpc):
        self.tools_dir = tools_dir
        self.partition = partition
        self.security = security
        self.ota = ota 
        self.ppc = ppc
        self.mpc = mpc
        self.primary_manfiest_exists = False
        self.secondary_manfiest_exists = False
