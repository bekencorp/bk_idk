#!/usr/bin/env python3

#TODO - optimize it, should finally remove this packer

import os
import sys
import json
import shutil
import argparse
import copy
import errno
import re
import csv
import subprocess
import logging

cur_path    = os.path.realpath('.')
cfg_path    = '%s/config'%(cur_path)
input_path  = '%s/input_dir'%(cur_path)
output_path = '%s/output_dir'%(cur_path)
tool_path   = '%s/tools'%(cur_path)
ota_tool = '%s/tools/ota-rbl/ota_packager_python.py'%(cur_path)

gen_file_list = set()

#add new process
def run_cmd_with_ret(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
    out, err = p.communicate()
    print(out)
    if (p.returncode):
       print(err)
       exit(1)

#Size format conversion  string->int
def parse_int(v):
    for letter, multiplier in [("k", 1024), ("m", 1024 * 1024)]:
        if v.lower().endswith(letter):
            return parse_int(v[:-1]) * multiplier
        return int(v, 0)

#Size format conversion  int->string
def size_format(size, include_size):
    if include_size:
        for (val, suffix) in [(0x400, "K"), (0x100000, "M")]:
            if size % val == 0:
                return "%d%s" % (size // val, suffix)
    return "0x%x" % size

#Path check, if no, create
def ensure_directory(dir):
    if not os.path.exists(dir):
        try:
            os.makedirs(dir)
        except OSError as exc:
            if exc.errno != errno.EEXIST:
                raise

#physical address is translated into a logical address
def shrink_val(val, need_shrink):
    return int(val*32/34) if need_shrink else val

#Read the csv file
def read_csv_data(file_path):
    data = {}
    with open(file_path, 'r') as file:
        reader = csv.reader(file)
        next(reader)  # Skip header line
        for row in reader:
            data[row[0]] = row[1]
    return data

#get partition table cfg json
def get_config_json_path():
    name = 'configuration.json'
    json_path = None
    if os.path.exists('%s/%s'%(input_path, name)):
        json_path = '%s/%s'%(input_path, name)
    else:
        logging.error(f'{name} NOT exists, please provide the file in the input_path folder')
    return json_path

#get firmware path
def get_firmware_path(firmware):
    firmware_path = None
    if os.path.exists('%s/%s'%(input_path, firmware)):
        firmware_path = '%s/%s'%(input_path, firmware)
    else:
        logging.error(f'{firmware} NOT exists, please provide the file in the input_path folder')
    return firmware_path

def get_aes_key_for_flash_encrypt():
    name = 'security.csv'
    csv_path = None
    if os.path.exists('%s/%s'%(cfg_path, name)):
        csv_path = '%s/%s'%(cfg_path, name)
    else:
        logging.error(f'{name} NOT exists, please provide the file in the input_path folder')

    data = read_csv_data(csv_path)
    aes_key = data.get('flash_aes_key')
    #print("<<<<<<debug aes_key>>>>>>>:",aes_key)
    if aes_key == None or aes_key == '':
        logging.error(f'flash encrypt key NOT exists, please cinfigure in the {name} file')

    return aes_key

# add 0xff padding to binary file
def add_padding_to_binary(binary_file):
    with open(binary_file , 'ab')as f:
        f.write(bytes([0xff] * 34))  # 34 bytes align

def check_and_padding_binary(app_pack_secs, app_crc_name):
    last_firmware = app_pack_secs[-1]['firmware']
    last_firmware_size = os.path.getsize(last_firmware)
    last_partition_raw_size = parse_int(app_pack_secs[-1]['size'])
    last_firmware_aligned_size = (((last_firmware_size + 32 - 1) // 32) * 32) # 32 bytes align
    app_crc_name += '.bin'
    if last_firmware_aligned_size < last_partition_raw_size:
        add_padding_to_binary(app_crc_name) # if the last firmware not fill up the partition, add padding value.

#create image that only supports encryption 
#reads the size byte from the offset of src_file and overwrites dest_file to generate a new file
def create_only_support_enc(src_file, dest_file, offset, size):
    out_file = os.path.join(output_path, 'all_app_pack_enc_crc.bin')
    if not os.path.exists(out_file.strip()):
        shutil.copyfile(dest_file, out_file)
    with open(src_file, 'rb') as file1:
        src_data = bytearray(file1.read())
        data = src_data[offset:offset+size]
    with open(out_file, 'r+b') as f:
        f.seek(offset)
        f.write(data)

#Package according to json file
def pack_from_config_json(cfg_json, nm):
    pack_tool = '%s/cmake_Gen_image'%(tool_path)
    json_file = './cfg_temp.json'
    infiles = ''
    if 'pack' in nm:
        outfile = os.path.join(output_path, '%s'%nm)
    else:
        nm_suffix = 'pack'
        outfile = os.path.join(output_path, '%s_%s'%(nm, nm_suffix))
    start_addr = cfg_json['section'][0]['start_addr']
    for sec in cfg_json['section']:
        infiles += '%s '%(sec['firmware'])
        start_addr = start_addr if parse_int(start_addr) <= parse_int(sec['start_addr']) else sec['start_addr']
    cfg_json_temp = copy.deepcopy(cfg_json)
    for sec in cfg_json_temp['section']:
        sec['start_addr'] = size_format(parse_int(sec['start_addr']) - parse_int(start_addr), False)
    cfg_json_temp = json.dumps(cfg_json_temp, sort_keys=False, indent=4)
    with open(json_file, 'w') as f:
        f.write(cfg_json_temp)
    if os.path.exists(pack_tool.strip()) and os.path.isfile(pack_tool.strip()):
        os.system('%s genfile -injsonfile %s -infile %s -outfile %s.bin'%(pack_tool, json_file, infiles, outfile))
    else:
        raise 'pack_tool path error!'
    os.remove(json_file)
    gen_file_list.add('%s.bin'%(outfile))
    return outfile

#encrypt image
def enc_from_config_json(cfg_json, nm):
    enc_tool = '%s/beken_aes'%(tool_path)
    nm_suffix = 'enc'

    aes_key = get_aes_key_for_flash_encrypt()
    start_addr = cfg_json['section'][0]['start_addr']
    for sec in cfg_json['section']:
        start_addr = start_addr if parse_int(start_addr) <= parse_int(sec['start_addr']) else sec['start_addr']

    outfile = os.path.join(output_path, '%s_%s'%(nm, nm_suffix))
    if os.path.exists(enc_tool.strip()) and os.path.isfile(enc_tool.strip()):
        os.system("%s encrypt -infile %s.bin -startaddress %s -keywords %s -outfile %s.bin"%(enc_tool, nm, start_addr, aes_key, outfile))
    else:
        raise 'enc_tool path error!'
    gen_file_list.add('%s.bin'%(outfile))
    gen_file_list.add('%s_crc.bin'%(outfile))
    return outfile

#add crc
def crc_from_config_json(cfg_json, nm):
    crc_tool = '%s/cmake_encrypt_crc'%(tool_path)
    nm_suffix = 'crc'
    outfile = os.path.join(output_path, '%s_%s'%(nm, nm_suffix))
    if os.path.exists(crc_tool.strip()) and os.path.isfile(crc_tool.strip()):
        os.system("%s -crc %s.bin"%(crc_tool, nm))
    else:
        raise 'crc_tool path error!'
    gen_file_list.add('%s.bin'%(outfile))
    return outfile

def package_json_enc_rebuild():
    header_path   = '%s/tools/ota-rbl/'%(cur_path)
    app_pack_secs = list()
    armino_path   = None
    project_dir   = None
    rbl_file = 'app_pack.rbl'

    json_file = get_config_json_path()
    with open(json_file, 'r') as local_json:
        config_json = json.load(local_json)
    for sec in config_json['section']:
        sec['firmware'] = get_firmware_path(sec['firmware'])
        sec['start_addr'] = size_format(shrink_val(parse_int(sec["start_addr"]), True), False)
        sec['size'] = size_format(shrink_val(parse_int(sec["size"]), True), True)
        if(sec['partition'] != 'bootloader'):
            app_pack_secs.append(sec)
    #gen all_app_pack.bin
    all_app_pack_name = pack_from_config_json(config_json, 'all_app')
    all_app_crc_name  = crc_from_config_json(config_json, all_app_pack_name)
    check_and_padding_binary(app_pack_secs, all_app_crc_name)
    all_app_enc_name  = enc_from_config_json(config_json, all_app_pack_name)
    #all_app_crc_name = crc_from_config_json(config_json, all_app_enc_name)
    create_only_support_enc('%s.bin'%(all_app_crc_name), '%s_crc.bin'%(all_app_enc_name), 0x110, 34)

    #gen app_pack.bin
    if len(app_pack_secs) > 0:
        config_json['count'] = len(app_pack_secs)
        config_json['section'] = app_pack_secs
    app_pack_name = pack_from_config_json(config_json, 'app')
    os.system('python3 %s -i %s.bin -o %s -g %s -ap %s -pjd %s packager'%(ota_tool, app_pack_name, rbl_file, header_path, armino_path, project_dir))
    shutil.move(rbl_file, output_path)

#generate otp_efuse_config.json file for burn
def generate_otp_efuse_config_json():
    outfile =  os.path.join(output_path, 'otp_efuse_config.json')
    
    flash_aes_key = get_aes_key_for_flash_encrypt()
    otp_efuse_config = {
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
            "boot_critical_error": "0,7,0",
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

    otp_efuse_config["Security_Ctrl"][0].update({"flash_aes_enable":"3,5,1"})
    data["name"] = "flash_aes_key"
    data["mode"] = "write"
    data["permission"] = "WR" #TODO change to NA
    data["start_addr"] = "0x4B100460"
    data["last_valid_addr"] = "0x4B100480"
    data["byte_len"] = "0x20"
    data["data"] = flash_aes_key
    data["data_type"] = "hex"
    data["status"] = "true"
    otp_efuse_config["Security_Data"].append(data)

    json_str = json.dumps(otp_efuse_config, indent=4)
    with open(outfile, 'w',newline="\n") as file:
        file.write(json_str)
    file.close()

def clear_output_path():
    if os.path.isdir(output_path):
        shutil.rmtree(output_path, ignore_errors=True)
    os.makedirs(output_path)

def main():
    clear_output_path()
    generate_otp_efuse_config_json()
    package_json_enc_rebuild()

if __name__ == '__main__':
    try:
        main()
    except InputError as e:
        print(e, file=sys.stderr)
        sys.exit(2)