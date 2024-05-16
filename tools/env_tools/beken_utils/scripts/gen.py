#!/usr/bin/env python3

import os
import json
import logging
import shutil
import re

from .security import *
from .ota import *
from .ppc import *
from .mpc import *
from .partition import *
from .gencode import *
from .genbin import *

class Gen:
    def __init__(self, tools_dir, partition_csv='partition.csv', security_csv='security.csv', ota_csv='ota.csv', mpc_csv='mpc.csv', ppc_csv='ppc.csv', gpio_dev_csv='gpio_dev.csv', genbin=False, gencode=False, debug=False):
        self.tools_dir = tools_dir
        self.genbin = genbin
        self.gencode = gencode
        self.debug = debug

        self.security = Security(security_csv)
        self.ota = OTA(ota_csv)
        self.ppc = PPC(ppc_csv, gpio_dev_csv)
        self.mpc = MPC(mpc_csv)
        self.partition = Partitions(tools_dir, partition_csv, self.security, self.ota)

    def gen_code(self):
        g = Gencode(self.partition, self.security, self.ota, self.ppc, self.mpc)
        g.gen_code()

    def gen_bin(self):
        g = Genbin(self.tools_dir, self.partition, self.security, self.ota, self.ppc, self.mpc)
        g.gen_bin()

        gcode = Gencode(self.partition, self.security, self.ota, self.ppc, self.mpc)
        gcode.gen_otp_efuse_config_json()
