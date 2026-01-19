#!/usr/bin/env python3
import re
import logging
import subprocess
import os
import shutil

s_base_addr = 0
SZ_16M = 0x1000000
CRC_SIZE = 0X02
CRC_PAKEIT = 0X20
FLASH_SECTOR_SZ = 0x1000
CRC_UNIT_DATA_SZ = 32
CRC_UNIT_TOTAL_SZ = 34
GLOBAL_HDR_LEN = 32
IMG_HDR_LEN = 32
BL2_HDR_SZ = 0x1000
BL2_TAIL_SZ = 0x1000
CPU_VECTOR_ALIGN_SZ = 512

def ceil_align(value, alignment):
    return (((value + alignment - 1) // alignment) * alignment)

def floor_align(value, alignment):
    return ((value // alignment) * alignment)

def p2v(addr):
    return (addr - s_base_addr)

def hex2int(str):
    return int(str, base=16) % (2**32)

def decimal2int(str):
    return int(str, base=10) % (2**32)

def crc_size(size):
    if is_flash_crc_enabled():
        # Align size to 34-byte boundary (32-byte data + 2-byte CRC)
        return (((size + (CRC_UNIT_DATA_SZ - 1)) // CRC_UNIT_DATA_SZ) * CRC_UNIT_TOTAL_SZ)
    else:
        # In no_crc mode: no alignment needed (1:1 mapping)
        return size 

def virtual2phy(addr):
    if is_flash_crc_enabled():
        return ((addr % CRC_UNIT_DATA_SZ) + ((addr // CRC_UNIT_DATA_SZ) * CRC_UNIT_TOTAL_SZ))
    else:
        return addr

def phy2virtual(addr):
    if is_flash_crc_enabled():
        return ((addr % CRC_UNIT_TOTAL_SZ) + ((addr // CRC_UNIT_TOTAL_SZ) * CRC_UNIT_DATA_SZ))
    else:
        return addr

def crc_addr(addr):
    if is_flash_crc_enabled():
        return crc_size(addr)
    else:
        return addr

def size2int(str):
    size_str = re.findall(r"\d+", str)
    size = decimal2int(size_str[0])

    unit = re.findall(r"[k|K|m|M|g|G|b|B]+", str)
    if (unit[0] == 'b') or (unit[0] == 'B'):
        return size
    if (unit[0] == 'k') or (unit[0] == 'K'):
        return size * (1 << 10)
    elif (unit[0] == 'm') or (unit[0] == 'M'):
        return size * (1 << 20)
    elif (unit[0] == 'g') or (unit[0] == 'G'):
        return size * (1 << 30)
    else:
        logging.error(f'invalid size unit {unit[0]}, must be "b/B/k/K/m/M/g/G"')
        exit(1)


def is_out_of_range(addr, size):
    if ((addr + size) >= SZ_16M):
        return True
    return False

def crc16(data : bytearray, offset , length):
    if data is None or offset < 0 or offset > len(data)- 1 and offset+length > len(data):
        return 0
    crc = 0xFFFFFFFF
    for i in range(0, length):
        crc ^= data[offset + i] << 8
        for j in range(0,8):
            if (crc & 0x8000) > 0:
                crc =(crc << 1) ^ 0x8005 #for beken poly:8005
            else:
                crc = crc << 1
    return crc & 0xFFFF

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def run_cmd_not_check_ret(cmd):
    p = subprocess.Popen(cmd, shell=True)
    p.wait()

def pattern_match(string, pattern):
    match = re.search(pattern, string)
    if match:
        return True 
    else:
        return False

def int2hexstr2(v):
    return (f'%04x' %(v))

def int2hexstr4(v):
    return (f'%08x' %(v))

def parse_bool(v):
    if v == 'TRUE':
        return True
    return False

def is_flash_crc_enabled():
    """Check if flash_crc_en is enabled by reading security.csv using Security class
    Returns True if flash_crc_en is True (crc mode), False if flash_crc_en is False (no_crc mode)
    """
    try:
        from .security import Security
        # Try to find security.csv in current directory or common locations
        security_csv = None
        common_paths = [
            'security.csv',
        ]
        for path in common_paths:
            if os.path.exists(path):
                security_csv = path
                break
        
        if security_csv and os.path.exists(security_csv):
            s = Security(security_csv)
            flash_crc_en = s.flash_crc_en
            logging.debug(f'Read flash_crc_en={flash_crc_en} from {security_csv}')
            return flash_crc_en
    except Exception as e:
        logging.debug(f'Failed to read flash_crc_en from config: {e}')
    
    # Default to True (CRC mode) if not found
    logging.debug('Using default flash_crc_en=TRUE (CRC mode)')
    return True

def clear_dir(directory):
    for filename in os.listdir(directory):
        file_path = os.path.join(directory, filename)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                clear_files_in_directory(file_path)
                pass
        except Exception as e:
            print(f'Failed to delete {file_path}. Reason: {e}')

def copy_files(src_directory, dest_directory):
    os.makedirs(dest_directory, exist_ok=True)
    for filename in os.listdir(src_directory):
        src_file = os.path.join(src_directory, filename)
        if os.path.isfile(src_file):
            shutil.copy(src_file, dest_directory)

def empty_line(f):
    line = f'\r\n'
    f.write(line)

def get_script_dir():
    script_dir = os.path.abspath(__file__)
    script_dir = os.path.dirname(script_dir)
    return script_dir

def get_tools_dir():
    script_dir = get_script_dir()
    if is_win():
        return f'{script_dir}\\..\\tools'
    else:
        return f'{script_dir}/../tools'
 
def is_win():
    return os.name == 'nt'

def is_pi():
    if os.path.exists('/proc/cpuinfo'):
        with open('/proc/cpuinfo', 'r') as f:
            cpuinfo = f.read()
            if 'Raspberry Pi' in cpuinfo:
                return True
        return False
    elif os.path.exists('/etc/os-release'):
        with open('/etc/os-release', 'r') as f:
            os_info = f.read().lower()
            if 'raspbian' in os_info or 'raspberry' in os_info:
                return True
            else:
                return False
    else:
        return False

def is_centos():
    return True

def get_soc_type():
    """Get SOC type from environment variable ARMINO_SOC"""
    soc_type = os.environ.get('ARMINO_SOC', '').upper()
    if soc_type:
        logging.debug(f'Detected SOC type from ARMINO_SOC: {soc_type}')
        return soc_type
    else:
        logging.debug('ARMINO_SOC not set, using default: BK7236N')
        return 'BK7236N'

def get_flash_aes_tool():
    tools_dir = get_tools_dir()
    soc_type = get_soc_type()
    if soc_type  == 'BK7236':
        if os.name == 'nt':
            return f'{tools_dir}\\packager_tools\\win\\beken_aes.exe'

        if is_pi():
            return f'{tools_dir}/packager_tools/pi/beken_aes'

        return f'{tools_dir}/packager_tools/centos7/beken_aes'
    else:
        return f'{tools_dir}/packager_tools/aes_xts.py'

def get_crc_tool():
    tools_dir = get_tools_dir()
    if os.name == 'nt':
        return f'{tools_dir}\\packager_tools\\win\\cmake_encrypt_crc.exe'

    if is_pi():
        return f'{tools_dir}/packager_tools/pi/cmake_encrypt_crc'

    return f'{tools_dir}/packager_tools/centos7/cmake_encrypt_crc'

def calc_crc16(raw,dst):
    # Check if input file exists
    if not os.path.exists(raw):
        raise FileNotFoundError(f'Input file not found: {raw}')
    
    # Check if tool exists
    crc_tool = get_crc_tool()
    if not os.path.exists(crc_tool):
        raise FileNotFoundError(f'CRC tool not found: {crc_tool}')
    
    # Execute CRC tool and check return value
    # Quote file paths to handle spaces or special characters
    raw_quoted = f'"{raw}"' if ' ' in raw or '\t' in raw else raw
    cmd = f'{crc_tool} -enc {raw_quoted} 0 0 0 0 -crc'
    logging.debug(f'calc_crc16: executing command: {cmd}')
    logging.debug(f'calc_crc16: input file: {raw}, exists: {os.path.exists(raw)}, size: {os.path.getsize(raw) if os.path.exists(raw) else "N/A"}')
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    ret = p.returncode
    if ret != 0:
        error_msg = stderr.decode('utf-8', errors='ignore') if stderr else ''
        raise RuntimeError(f'CRC tool failed with return code {ret}: {cmd}\nError: {error_msg}')
    
    # Check if output file was generated
    aes_crc_bin_name = raw.replace(".bin","_crc.bin")
    if not os.path.exists(aes_crc_bin_name):
        raise FileNotFoundError(f'CRC tool did not generate output file: {aes_crc_bin_name}')
    
    # Rename output file to destination
    os.rename(aes_crc_bin_name, dst)
    logging.debug(f'calc_crc16: successfully generated {dst}')
    
def get_nvs_tool():
    tools_dir = get_tools_dir()
    if os.name == 'nt':
        return f'{tools_dir}\\packager_tools\\win\\mbedtls_aes_xts.exe'

    if is_pi():
        return f'{tools_dir}/packager_tools/pi/mbedtls_aes_xts'

    return f'{tools_dir}/packager_tools/centos7/mbedtls_aes_xts'

def get_lzma_tool():
    tools_dir = get_tools_dir()
    if os.name == 'nt':
        return f'{tools_dir}\\packager_tools\\win\\lzma.exe'

    if is_pi():
        return f'{tools_dir}/packager_tools/pi/lzma'
        return "TODO"

    return f'{tools_dir}/packager_tools/centos7/lzma'

def get_key_files_from_security_csv(csv_file='security.csv'):
    if not os.path.exists(csv_file):
        return ({}, False)
    
    key_files_dict = {}
    key_fields = ['img_sign_pubkey', 'img_sign_privkey', 'new_img_sign_pubkey', 'new_img_sign_privkey']
    update_img_sign_key_en = False
    
    try:
        import csv
        with open(csv_file, 'r') as f:
            csv_data = csv.reader(f)
            row_idx = 0
            csv_dict = {}
            for row in csv_data:
                if row_idx == 0:
                    # Skip header row
                    row_idx = 1
                    continue
                if len(row) >= 2:
                    field = row[0].strip()
                    value = row[1].strip()
                    csv_dict[field] = value
        
        # Extract key file names from CSV
        for field in key_fields:
            if field in csv_dict and csv_dict[field]:
                key_files_dict[field] = csv_dict[field]
        
        # Extract update_img_sign_key_en value
        if 'update_img_sign_key_en' in csv_dict:
            update_img_sign_key_en = parse_bool(csv_dict['update_img_sign_key_en'])
    except Exception as e:
        logging.debug(f'Failed to parse {csv_file}: {e}')
        return ({}, False)
    
    return (key_files_dict, update_img_sign_key_en)

def get_config_paths(armino_root=None, current_dir=None):
    if armino_root is None:
        # Try to infer armino_root from current script location or environment
        script_dir = os.path.dirname(os.path.abspath(__file__))
        # Assuming we're in tools/env_tools/bksecure/scripts
        armino_root = os.path.abspath(os.path.join(script_dir, '..', '..', '..', '..'))
    
    if current_dir is None:
        current_dir = os.getcwd()
    
    config_paths = []
    
    # Priority 1: Use environment variables if available
    project_dir = os.getenv('PROJECT_DIR', '')
    armino_soc = os.getenv('ARMINO_SOC', '')
    
    if project_dir and armino_soc:
        # Project-specific config paths
        config_paths.append(os.path.join(armino_root, project_dir, 'config', armino_soc))
        config_paths.append(os.path.join(armino_root, project_dir, 'config'))
        config_paths.append(os.path.join(armino_root, project_dir, 'config', 'common'))
    
    # Priority 2: Try to infer from current directory path
    # Common pattern: .../build/{project}/{soc}/... or .../build/{project}/...
    path_parts = os.path.normpath(current_dir).split(os.sep)
    try:
        build_idx = path_parts.index('build')
        if build_idx + 1 < len(path_parts):
            inferred_project = path_parts[build_idx + 1]
            if build_idx + 2 < len(path_parts):
                inferred_soc = path_parts[build_idx + 2]
                # Try inferred project paths (in projects directory)
                inferred_project_path = os.path.join(armino_root, 'projects', inferred_project, 'config', inferred_soc)
                if os.path.exists(inferred_project_path):
                    config_paths.append(inferred_project_path)
                inferred_project_path = os.path.join(armino_root, 'projects', inferred_project, 'config')
                if os.path.exists(inferred_project_path):
                    config_paths.append(inferred_project_path)
    except ValueError:
        # 'build' not in path, skip inference
        pass
    
    # Priority 3: Search common locations (only if PROJECT_DIR is set)
    # This allows for projects outside the standard structure
    if project_dir:
        # Search in projects directory for matching project name
        projects_dir = os.path.join(armino_root, 'projects')
        if os.path.exists(projects_dir):
            for item in os.listdir(projects_dir):
                # Skip hidden files/directories like .git
                if item.startswith('.'):
                    continue
                project_path = os.path.join(projects_dir, item)
                if os.path.isdir(project_path):
                    config_dir = os.path.join(project_path, 'config')
                    if os.path.exists(config_dir) and os.path.isdir(config_dir):
                        if armino_soc:
                            soc_config = os.path.join(config_dir, armino_soc)
                            if os.path.exists(soc_config) and os.path.isdir(soc_config):
                                config_paths.append(soc_config)
                        config_paths.append(config_dir)
                        common_config = os.path.join(config_dir, 'common')
                        if os.path.exists(common_config) and os.path.isdir(common_config):
                            config_paths.append(common_config)
    
    # Remove duplicates while preserving order
    seen = set()
    unique_paths = []
    for path in config_paths:
        if path not in seen:
            seen.add(path)
            unique_paths.append(path)
    
    return unique_paths


