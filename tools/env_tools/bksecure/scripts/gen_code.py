#!/usr/bin/env python3

import logging

from .gen_ppc import *
from .gen_mpc import *
from .gen_security import *
from .gen_otp_map import *
from .partitions import Partitions

def gen_code():
    gen_ppc_config_file('ppc.csv', 'gpio_dev.csv', '_ppc.h')
    gen_mpc_config_file('mpc.csv', '_mpc.h')
    gen_security_config_file('security.csv', 'security.h')
    gen_otp_map_file()

    p = Partitions('partitions.csv', 'bin.csv', 'pack.json')
    p.prebuild_process()
