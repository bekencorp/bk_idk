#!/usr/bin/env python3
import os
import logging
import csv
import json
import hashlib

from .security import *
from .ota import *
from .ppc import *
from .mpc import *
from .partition import *
from .common import *
from cryptography.hazmat.primitives import serialization

s_license = "\
// Copyright 2022-2023 Beken\r\n\
//\r\n\
// Licensed under the Apache License, Version 2.0 (the \"License\");\r\n\
// you may not use this file except in compliance with the License.\r\n\
// You may obtain a copy of the License at\r\n\
//\r\n\
//     http://www.apache.org/licenses/LICENSE-2.0\r\n\
//\r\n\
// Unless required by applicable law or agreed to in writing, software\r\n\
// distributed under the License is distributed on an \"AS IS\" BASIS,\r\n\
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\r\n\
// See the License for the specific language governing permissions and\r\n\
// limitations under the License.\r\n\
\r\n\
//This is a generated file, don't modify it!\r\n\
\r\n\
#pragma once\r\n\
\r\n\
\r\n\
"

s_phy2code_start = "\
#define FLASH_CEIL_ALIGN(v, align) ((((v) + ((align) - 1)) / (align)) * (align))\r\n\
#define FLASH_PHY2VIRTUAL_CODE_START(phy_addr) FLASH_CEIL_ALIGN(FLASH_PHY2VIRTUAL(FLASH_CEIL_ALIGN((phy_addr), 34)), CPU_VECTOR_ALIGN_SZ)\r\n\
"

class Gencode:
    def empty_line(self, f):
        line = f'\r\n'
        f.write(line)

    def gen_partitions_hdr_file(self):
        self.partition_hdr_file_name = f'partitions_partition.h'
        f = open(self.partition_hdr_file_name, 'w+')

        logging.debug(f'Create partition constants file: {self.partition_hdr_file_name}')
        f.write(s_license)

        line = f'#define %-45s %s' %("KB(size)", "((size) << 10)\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("MB(size)", "((size) << 20)\r\n\r\n")
        f.write(line)

        line = f'#undef %-45s\r\n' %("MCUBOOT_SIGN_RSA")
        f.write(line)
        if (self.security.secureboot_en):
            line = f'#define %-45s %d\r\n' %("CONFIG_SECUREBOOT", 1)
            f.write(line)

        if (self.security.bl2_root_key_type == 'rsa2048'):
            line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA", "1\r\n")
            f.write(line)
            line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA_LEN", "2048\r\n")
            f.write(line)
        elif (self.security.bl2_root_key_type == 'rsa3072'):
            line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA", "1\r\n")
            f.write(line)
            line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA_LEN", "3072\r\n")
            f.write(line)
        elif (self.security.bl2_root_key_type == 'ec256'):
            line = f'#define %-45s %s' %("MCUBOOT_SIGN_EC256", "1\r\n")
            f.write(line)
        else:
            logging.error(f'unsupported bl2 key type\r\n')
            exit(1)

        macro_name = f'CONFIG_CODE_ENCRYPTED'
        if self.security.flash_aes_en:
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)

        macro_name = f'CONFIG_OTA_OVERWRITE'
        if (self.ota.get_strategy().upper() == 'OVERWRITE'):
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)

        macro_name = f'CONFIG_TFM_S_JUMP_TO_TFM_NS'
        if self.partition.tfm_ns_exists == True:
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)

        macro_name = f'CONFIG_TFM_S_JUMP_TO_CPU0_APP'
        if self.partition.primary_cpu0_app_exists == True:
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)

        macro_name = f'CONFIG_CODE_HAS_CRC'
        if self.partition.need_crc:
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
            f.write(line)
            line = f'#define %-45s %s' %("FLASH_VIRTUAL2PHY(virtual_addr)", "((((virtual_addr) >> 5) * 34) + ((virtual_addr) & 31))\r\n")
            f.write(line)
            line = f'#define %-45s %s' %("FLASH_PHY2VIRTUAL(phy_addr)", "((((phy_addr) / 34) << 5) + ((phy_addr) % 34))\r\n")
            f.write(line)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
            f.write(line)
            line = f'#define %-45s %s' %("FLASH_VIRTUAL2PHY(virtual_addr)", "(virtual_addr)\r\n")
            f.write(line)
            line = f'#define %-45s %s' %("FLASH_PHY2VIRTUAL(phy_addr)", "(phy_addr)\r\n")
            f.write(line)

        macro_name = f'CPU_VECTOR_ALIGN_SZ'
        line = f'#define %-45s %d\r\n' %(macro_name, self.partition.cpu_vector_align_bytes)
        f.write(line)

        self.empty_line(f)
        f.write(s_phy2code_start)
        self.empty_line(f)

        for partition in self.partition.partitions:

            partition_name = partition.partition_name
            partition_name = partition_name.upper()
            partition_name = partition_name.replace(' ', '_')

            logging.debug(f'generate constants for partition {partition_name}')

            macro_name = f'CONFIG_{partition_name}_PHY_PARTITION_OFFSET'
            line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.partition_offset)
            f.write(line)
            macro_name = f'CONFIG_{partition_name}_PHY_PARTITION_SIZE'
            line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.partition_size)
            f.write(line)

            if partition.bin_is_code() and partition.bin_name != None:
                macro_name = f'CONFIG_{partition_name}_PHY_CODE_START'
                line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.phy_code_offset)
                f.write(line)


                if (partition.bin_hdr_size > 0):
                    macro_name = f'CONFIG_{partition_name}_PHY_HDR_SIZE'
                    line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.bin_hdr_size)
                    f.write(line)

                if (partition.bin_tail_size > 0):
                    macro_name = f'CONFIG_{partition_name}_PHY_TAIL_SIZE'
                    line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.bin_tail_size)
                    f.write(line)

                if (partition.virtual_partition_size > 0):
                    macro_name = f'CONFIG_{partition_name}_VIRTUAL_PARTITION_SIZE'
                    line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.virtual_partition_size)
                    f.write(line)

                    macro_name = f'CONFIG_{partition_name}_VIRTUAL_CODE_START'
                    line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.vir_code_offset)
                    f.write(line)

                    macro_name = f'CONFIG_{partition_name}_VIRTUAL_CODE_SIZE'
                    line = f'#define %-45s 0x%x\r\n' %(macro_name, partition.vir_code_size)
                    f.write(line)

            self.empty_line(f)

        f.flush()
        f.close()

    def gen_partitions_layout_file(self):
        self.partition_layout_file_name = f'partitions_layout.txt'
        f = open(self.partition_layout_file_name, 'w+')

        logging.debug(f'Create partition layout file: {self.partition_layout_file_name}')
        line = f'%-24s   %-10s   %-8s\r\n' %("name", "offset", "size")
        f.write(line)
        line = f'------------------------   ----------   ----------------\r\n'
        f.write(line)

        for partition in self.partition.partitions:

            partition_name = partition.partition_name
            line = f'%-24s   0x%-8x   0x%-6x(%uK)\r\n' %(partition_name, partition.partition_offset, partition.partition_size, partition.partition_size>>10)
            f.write(line)

        f.flush()
        f.close()

    def gen_mpc(self,device,block_size,f,sector_num):
        generator = (dict for dict in self.mpc.csv.dic_list if dict["Device"] == device)
        count  = len(list(generator))
        sector_num.append(count)
        line = f'const static int %s[%d][3] = {{' % (device, count)
        f.write(line)
        end = 0
        i = 0
        for dict in self.mpc.csv.dic_list:
            if dict["Device"] == device:
                start = int(dict["Offset"],16)
                if(start % block_size != 0):
                    logging.error("address not aligned!\r\n")
                    exit("check mpc.csv\r\n")
                if (start < end):
                    logging.error("address overlapped!\r\n")
                    exit("check mpc.csv\r\n")
                end = int(dict["Offset"],16)+ size2int(dict["Size"])
                size = size2int(dict["Size"])
                if(dict["Secure"] == "TRUE"):
                    sec = 0 #secure value 0
                else:
                    sec = 1 #non secure value 1
                line = f'{{0x%x,%d,%d}},' %(start , size / block_size, sec)
                f.write(line)
                i+=1
        line = f'}};\r\n'
        f.write(line)

    def gen_ppc_init_file(self):
        self.ppc_init_file_name = f'tfm_hal_ppc.h'
        f = open(self.ppc_init_file_name, 'w+')

        logging.debug(f'Create partition constants file: {self.ppc_init_file_name}')
        f.write(s_license)

        line = f'static inline void tfm_hal_ppc_init(void)\r\n'
        f.write(line)
        line = f'{{\r\n'
        f.write(line)

        self.ppc.check_gpio_security()
        for dict in self.ppc.csv.dic_list:
            if dict["Secure"] == "TRUE":
                line = f'    bk_prro_set_secure(PRRO_DEV_%s, PRRO_SECURE);\r\n' %(dict["Device"])
                f.write(line)

            if dict["Privilege"] == "FALSE":
                line = f'    bk_prro_set_privilege(PRRO_DEV_%s, PRRO_NON_PRIVILEGED);\r\n' %(dict["Device"])
                f.write(line)

        line = f'}}'
        f.write(line)

    def gen_security_hdr_file(self):
        self.security_hdr_file_name = f'security.h'
        f = open(self.security_hdr_file_name, 'w+')

        logging.debug(f'Create partition constants file: {self.security_hdr_file_name}')
        f.write(s_license)

        self.ppc.check_gpio_security()
        for dict in self.ppc.csv.dic_list:
            if dict["Secure"] == "TRUE":
                line = f'#define GEN_SECURITY_DEV_%s_IS_SECURE    %s' %(dict["Device"], "1\r\n")
            else:
                line = f'#define GEN_SECURITY_DEV_%s_IS_SECURE    %s' %(dict["Device"], "0\r\n")
            f.write(line)
            if dict["Privilege"] == "TRUE":
                line = f'#define GEN_SECURITY_DEV_%s_IS_PRIVILEGE    %s' %(dict["Device"], "1\r\n")
            else:
                line = f'#define GEN_SECURITY_DEV_%s_IS_PRIVILEGE    %s' %(dict["Device"], "0\r\n")
            f.write(line)


        line = f'#define DEV_IS_SECURE(dev)    GEN_SECURITY_DEV_##dev##_IS_SECURE\r\n'
        f.write(line)
        line = f'#define DEV_ID_IS_SECURE(dev,id)    GEN_SECURITY_DEV_##dev##id##_IS_SECURE\r\n'
        f.write(line)
        line = f'#define DEV_IS_PRIVILEGE(dev)    GEN_SECURITY_DEV_##dev##_IS_PRIVILEGE\r\n'
        f.write(line)
        line = f'#define DEV_ID_IS_PRIVILEGE(dev, id)    GEN_SECURITY_DEV_##dev##id##_IS_PRIVILEGE\r\n'
        f.write(line)

        line = f'//device [start_addr] [block_num] [secure_type] (secure = 0,non secure = 1)\r\n'
        f.write(line)
        sector_num = []
        self.gen_mpc("psram",256*1024,f,sector_num)
        self.gen_mpc("qspi0",256*1024,f,sector_num)
        self.gen_mpc("qspi1",256*1024,f,sector_num)
        self.gen_mpc("otp2",256,f,sector_num)
        self.gen_mpc("flash",64*1024,f,sector_num)
        self.gen_mpc("mem0",4*1024,f,sector_num)
        self.gen_mpc("mem1",4*1024,f,sector_num)
        self.gen_mpc("mem2",4*1024,f,sector_num)
        self.gen_mpc("mem3",4*1024,f,sector_num)
        self.gen_mpc("mem4",4*1024,f,sector_num)
        self.gen_mpc("mem5",4*1024,f,sector_num)

        self.empty_line(f)
        line = f'typedef struct {{\r\n'
        f.write(line)
        line = f'    int dev_num;\r\n'
        f.write(line)
        line = f'    const int* pointer;\r\n'
        f.write(line)
        line = f'    int sector_num;\r\n'
        f.write(line)
        line = f'}} dev_security_t;\r\n'
        f.write(line)
        self.empty_line(f)

        list = ["psram","qspi0","qspi1","otp2","flash","mem0","mem1","mem2","mem3","mem4","mem5"]
        line = f'const static dev_security_t mpc_security_table[11] = {{\r\n'
        f.write(line)
        for i in range(11):
            line = f'    {{%d, *%s, %d}},\r\n'%(i,list[i],sector_num[i])
            f.write(line)
        line = f'}};\r\n'
        f.write(line)
        f.close()

    def reverse_half_key(self, half_key):
        return (half_key[24:32] + half_key[16:24] + half_key[8:16] + half_key[0:8])

    def configured_aes_key_to_otp_aes_key(self, aes_key):
        length = len(aes_key)
        midpoint = length // 2
        first_half = aes_key[:midpoint]
        second_half = aes_key[midpoint:]
        return (second_half + first_half)

    def reverse_order(self, hex_str):
        hex_str_len = len(hex_str)
        word_len = hex_str_len // 8
        reverse_str = ''
        for i in range(word_len):
            idx = i << 3
            reverse_str = reverse_str + hex_str[idx + 6]
            reverse_str = reverse_str + hex_str[idx + 7]
            reverse_str = reverse_str + hex_str[idx + 4]
            reverse_str = reverse_str + hex_str[idx + 5]
            reverse_str = reverse_str + hex_str[idx + 2]
            reverse_str = reverse_str + hex_str[idx + 3]
            reverse_str = reverse_str + hex_str[idx + 0]
            reverse_str = reverse_str + hex_str[idx + 1]


        logging.debug(f'hex_str={hex_str}')
        logging.debug(f'reverse_str={reverse_str}')
        return reverse_str

    def gen_rotpk_hash(self):
        if (os.path.exists(f'puk_digest.bin') == True):
            with open('puk_digest.bin', 'rb') as f:
                self.bl1_rotpk_hash = f.read()
                self.bl1_rotpk_hash = self.bl1_rotpk_hash.hex()
                self.bl1_rotpk_hash = self.reverse_order(self.bl1_rotpk_hash)
                logging.debug(f'bl1 rotpk hash={self.bl1_rotpk_hash}')

        with open(self.security.bl2_root_pubkey, 'rt') as pem_file:
            pem_data = pem_file.read()

        try:
            public_key = serialization.load_pem_public_key(pem_data.encode())
            der_data = public_key.public_bytes(
                encoding=serialization.Encoding.DER,
                format=serialization.PublicFormat.SubjectPublicKeyInfo)

            sha256_hash = hashlib.sha256()
            sha256_hash.update(der_data)
            self.bl2_rotpk_hash = sha256_hash.hexdigest()
            self.bl2_rotpk_hash = self.reverse_order(self.bl2_rotpk_hash)
            logging.debug(f'bl2 rotpk hash={self.bl2_rotpk_hash}')
        except Exception as e:
            logging.error(f'failed to load PEM data {e}')
            exit(1)

        return der_data


    def gen_otp_efuse_config_json(self):
        self.otp_efuse_config = {
            "User_Operate_Enable":  "false",
            "Security_Ctrl_Enable": "true",
            "Security_Data_Enable": "true",

            "User_Operate":[],

            "Security_Ctrl":[{
                "secure_boot_enable":           "0,0,0",
                "secure_boot_debug_disable":    "0,1,0",
                "fast_boot_disable":            "0,2,0",
                "boot_mode":                    "0,3,0",
                "secure_boot_clock_select":     "0,4,0",
                "random_delay_enable":          "0,5,0",
                "direct_jump_enable":           "0,6,0",
                "security_boot_critical_error": "0,7,0",
                "attack_nmi_enable":            "2,4,0",
                "spi_to_ahb_disable":           "2,5,0",
                "auto_reset_enable[0]":         "2,6,0",
                "auto_reset_enable[1]":         "2,7,0",
                "flash_aes_enable":             "3,5,0",
                "spi_download_disable":         "3,6,0",
                "swd_disable":                  "3,7,0"
            }],

            "Security_Data":[]
        }

        data = {}
        if self.security.flash_aes_en:
            self.otp_efuse_config["Security_Ctrl"][0].update({"flash_aes_enable":"3,5,1"})
            data["name"] = "flash_aes_key"
            data["mode"] = "write"
            data["permission"] = "WR" #TODO change to NA
            data["start_addr"] = "0x4B100460"
            data["last_valid_addr"] = "0x4B100480"
            data["byte_len"] = "0x20"
            data["data"] = self.security.flash_aes_key
            data["data_type"] = "hex"
            data["status"] = "true"
            self.otp_efuse_config["Security_Data"].append(data)

        if self.security.secureboot_en:
            self.gen_rotpk_hash()
            self.otp_efuse_config["Security_Ctrl"][0].update({"secure_boot_enable":"0,0,1"})
            self.otp_efuse_config["Security_Ctrl"][0].update({"boot_mode":"0,3,1"})
            #self.otp_efuse_config["Security_Ctrl"][0].update({"secure_boot_debug_disable":"0,1,1"})
            self.otp_efuse_config["Security_Ctrl"][0].update({"direct_jump_enable":"0,6,1"})

            data = data.copy()
            data["name"] = "bl1_rotpk_hash"
            data["mode"] = "write"
            data["permission"] = "WR" #TODO change to RO
            data["start_addr"] = "0x4B100528"
            data["last_valid_addr"] = "0x4B100548"
            data["byte_len"] = "0x20"
            data["data"] = self.bl1_rotpk_hash
            data["data_type"] = "hex"
            data["status"] = "true"
            self.otp_efuse_config["Security_Data"].append(data)

            data = data.copy()
            data["name"] = "bl2_rotpk_hash"
            data["mode"] = "write"
            data["permission"] = "WR" #TODO change to RO
            data["start_addr"] = "0x4B100548"
            data["last_valid_addr"] = "0x4B100568"
            data["byte_len"] = "0x20"
            data["data"] = self.bl2_rotpk_hash
            data["data_type"] = "hex"
            data["status"] = "true"
            self.otp_efuse_config["Security_Data"].append(data)

        json_str = json.dumps(self.otp_efuse_config, indent=4)
        with open('otp_efuse_config.json', 'w',newline="\n") as file:
            file.write(json_str)
        file.close()


    def gen_code(self):
        self.gen_partitions_hdr_file()
        self.gen_partitions_layout_file()
        self.gen_security_hdr_file()
        self.gen_ppc_init_file()

    def __init__(self, partition, security, ota, ppc, mpc):
        self.partition = partition
        self.security = security
        self.ota = ota
        self.ppc = ppc
        self.mpc = mpc
        self.bl1_rotpk_hash = None
        self.bl2_rotpk_hash = None
