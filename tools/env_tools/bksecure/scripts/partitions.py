#!/usr/bin/env python3

import os
import json
import logging
import csv
import struct
import shutil
import base64

from .common import *
from .parse_csv import *
from .partition import Partition
from .partition_merge import PartitionMerge
from .partition_app import PartitionApp
from .partition_subapp import PartitionSubapp
from .partition_partition import PartitionPartition
from .partition_bl1_control import PartitionBl1Control
from .partition_boot_flag import PartitionBootFlag
from .partition_manifest import PartitionManifest
from .partition_padding import PartitionPadding
from .partition_app_padding import PartitionAppPadding
from .partition_nvs_key import PartitionNvsKey
from .partition_nvs import PartitionNvs
from .gen_license import *
from .bin import Bin
from .ota import OTA

OTA_CONFIRM = 0xA16D8FB0

class Partitions:

    def __init__(self, partition_csv, bin_csv, pack_json):
        logging.debug(f'Partitions: init {type(self)}')
        self._bins = self.__parse_bin_csv(bin_csv)
        self.__parse_partition_csv(partition_csv)
        self._pack_json = pack_json
        
        self._flash_crc_en = is_flash_crc_enabled()
        self._update_img_sign_key_en = self.__is_update_img_sign_key_enabled()
        self._version_overrides = {}
        self.__common_process()

    def __validate_duplication(self):
        names = []
        for p in self._partitions:
            names.append(p.name())

        if (len(names) != len(set(names))):
            logging.error(f'partitions.csv contains duplicated partitions!')
            exit(1)

    def __validate_isolation_boundary(self):
        pre_flags = None
        pre_partition = None

        for p in self._partitions:
            flags = p.flags()
            if (pre_flags != None) and (flags.cbus_secure() != pre_flags.cbus_secure()):
                offset = p.size().offset()

                if (offset % (68<<10)) != 0:
                    block = offset // (68<<10)
                    suggest_boundary = (block + 1) * (68 << 10)
                    diff = suggest_boundary - offset
                    logging.error(f'The partition {pre_partition.name()} and {p.name()} is NOT in 68K boundary!')
                    logging.error(f'{p.name()} start {offset/1024}k, suggest start {suggest_boundary/1024}k')
                    size_k = pre_partition.size().size() /1024
                    suggest_size_k = (size_k + diff)/1024
                    logging.error(f'Suggest to change {pre_partition.name()} size from {size_k}k to {suggest_size_k}k')
                    exit(1)

            pre_partition = p;
            pre_flags = flags
 
    def __validate_bin(self):
        for b in self._bins:
            p = self._find_partition_by_name(b.partition())
            if p == None:
                logging.error(f'Partition {b.partition()} not exists')
                exit(1)

    def __get_bin(self, partition):
        for b in self._bins:
            if b.partition() == partition:
                return b

        return None

    def __validate(self):
        self.__validate_duplication()
        self.__validate_isolation_boundary()
        self.__validate_bin()

    def __create_merged_partitions(self):

        pre_type = None
        merge_idx = 0
        partitions = []

        ow_active = None
        ow_ota = None
        for p in self._partitions:
            type = p.type()

            if type.is_ow_ota():
                ow_ota = p

            if (pre_type != None) and (pre_type.subtype() != type.subtype()):
                if len(partitions) != 0:
                    idx = len(self._partitions)
                    merged_partition = PartitionMerge(partitions, idx, merge_idx)
                    self._partitions.append(merged_partition)
                    merge_idx = merge_idx + 1
                    partitions = []

                    if pre_type.is_ow_active():
                        ow_active = merged_partition

            if type.is_subapp():
                partitions.append(p)    
                pre_type = type
            else:
                pre_type = None

        if ow_active != None:
            ow_active.set_ota_partition(ow_ota)
        
    def __parse_bin_csv(self, bin_csv):
        logging.debug(f'Partitions: __parse_bin_csv')
        o = OTA('ota.csv')
        app_version = o.get_version()
        bl2_version = o.get_bootloader_version()
        if bin_csv == None:
            return []

        bin_keys = ['Name', 'Type', 'Partition', 'Version', 'Security_counter']
        csv = Csv(bin_csv, True, bin_keys)

        bins = []
        logging.debug(f'bin.csv: {csv.dic_list}')
        for dic in csv.dic_list:
            if dic['Name'] == 'bl2.bin':
                dic['Version'] = bl2_version
            elif dic['Name'] == 'cpu0_app.bin':
                dic['Version'] = app_version
            b = Bin(dic['Name'], dic['Type'], dic['Partition'], dic['Version'], dic['Security_counter'])
            bins.append(b)

        return bins

    def __parse_partition_csv(self, partition_csv):
        logging.debug(f'Partitions: __parse_partition_csv {partition_csv} ')

        if partition_csv == None:
            return []

        mini_offset = 0
        partition_keys_v2 = ['Name', 'Type', 'SubType', 'Offset', 'Size', 'Flags']
        partition_keys_v1 = ['Name', 'Offset', 'Size', 'Execute', 'Read', 'Write' ]
        csv = Csv(partition_csv, True, partition_keys_v2, partition_keys_v1)

        subapp_idx_dic = {
            "ow_active" : 0,
            "xip_a" : 0,
            "xip_b" : 0
        }

        idx = -1
        self._partitions = []
        for pdic in csv.dic_list:
            idx += 1
            type = pdic['Type'].strip()
            subtype = pdic['SubType'].strip()
            b = self.__get_bin(pdic['Name'].strip())

            if (type == 'app') and (subtype == 'bl1_control'):
                p = PartitionBl1Control(idx, pdic, mini_offset, self._partitions)
            elif (type == 'data') and (subtype == 'manifest'):
                p = PartitionManifest(idx, pdic, mini_offset)
            elif (type == 'data') and (subtype == 'boot_flag'):
                p = PartitionBootFlag(idx, pdic, mini_offset, self._partitions)
            elif (type == 'data') and (subtype == 'partition'):
                p = PartitionPartition(idx, pdic, mini_offset, self._partitions)
            elif (type == 'data') and (subtype == 'nvs_key'):
                p = PartitionNvsKey(idx, pdic, mini_offset, self._partitions)
            elif (type == 'data') and (subtype == 'nvs'):
                p = PartitionNvs(idx, pdic, mini_offset, self._partitions)
            elif (type == 'data') and ((subtype == 'ow_ota_control') or (subtype == 'xip_ota_control')):
                p = PartitionPadding(idx, pdic, mini_offset)
            elif (type == 'app') and ((subtype == 'ow_active') or (subtype == 'xip_a') or (subtype == 'xip_b')):
                subapp_idx_dic[subtype] += 1
                if subapp_idx_dic[subtype] == 1:
                    # Only the first partition need to add BL2 signed header
                    p = PartitionSubapp(idx, pdic, mini_offset, BL2_HDR_SZ, b)
                else:
                    p = PartitionSubapp(idx, pdic, mini_offset, 0, b)
            elif (type == 'app') and (subtype == 'ow_ota'):
                p = PartitionAppPadding(idx, pdic, mini_offset)
            elif (type == 'app'):
                p = PartitionApp(idx, pdic, mini_offset, 0, b)
            elif (type == 'data'):
                p = Partition(idx, pdic, mini_offset)
            else:
                logging.error(f'Unsupported partition type={type}, subtype={subtype}')
                exit(1)

            mini_offset = p.size().offset() + p.size().size()
            self._partitions.append(p)

    def _find_partition_by_name(self, name):
        for p in self._partitions:
            if (name == p.name()):
                return p
        return None

    def __has_overwrite(self):
        for p in self._partitions:
            if p.type().is_ow_active():
                return True
        return False

    def __has_xip(self):
        for p in self._partitions:
            if p.type().is_xip_a():
                return True
        return False

    def __is_update_img_sign_key_enabled(self):
        """Check if update_img_sign_key_en is enabled by reading security.csv using Security class
        Returns True if update_img_sign_key_en is True, False otherwise
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
                update_img_sign_key_en = s.update_img_sign_key_en
                logging.debug(f'Read update_img_sign_key_en={update_img_sign_key_en} from {security_csv}')
                return update_img_sign_key_en
        except Exception as e:
            logging.debug(f'Failed to read update_img_sign_key_en from config: {e}')
        
        # Default to False if not found
        return False

    def __common_process(self):
        logging.debug(f'Partitions: common process')
        self.__validate()
        self.__create_merged_partitions()

    def __bin_partition_info_to_bin(self):
        print(f'__bin_partition_info_to_bin')

    def __gen_code(self, fname):
        f = open(fname, 'w+')
        
        logging.debug(f'Create partition hdr file: {fname}')
        f.write(get_license())
    
        line = f'#include "security.h"\r\n'
        f.write(line)
     
        line = f'#define %-45s %s' %("KB(size)", "((size) << 10)\r\n")
        f.write(line)
        line = f'#define %-45s %s' %("MB(size)", "((size) << 20)\r\n\r\n")
        f.write(line)

        macro_name = f'CPU_VECTOR_ALIGN_SZ'
        line = f'#define %-45s %d\r\n' %(macro_name, 512)
        f.write(line)

        macro_name = f'CONFIG_OTA_OVERWRITE'
        if self.__has_overwrite():
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
            f.write(line)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
            f.write(line)

        macro_name = f'OTA_CONFIRM'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, OTA_CONFIRM)
        f.write(line)

        macro_name = f'CONFIG_DIRECT_XIP'
        if self.__has_xip():
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)

        macro_name = f'CONFIG_FLASH_CRC_ENABLE'
        # If flash_crc_en is True, then CONFIG_FLASH_CRC should be 1 (crc mode)
        # If flash_crc_en is False, then CONFIG_FLASH_CRC should be 0 (no_crc mode)
        if self._flash_crc_en:
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)

        macro_name = f'CONFIG_UPDATE_IMG_SIGN_KEY'
        # If update_img_sign_key_en is True, then CONFIG_UPDATE_IMG_SIGN_KEY should be 1
        # If update_img_sign_key_en is False, then CONFIG_UPDATE_IMG_SIGN_KEY should be 0
        if self._update_img_sign_key_en:
            line = f'#define %-45s %d\r\n' %(macro_name, 1)
        else:
            line = f'#define %-45s %d\r\n' %(macro_name, 0)
        f.write(line)
        macro_name = f'XIP_MAGIC_TYPE'
        line = f'#define %-45s %d\r\n' %(macro_name, 1)
        f.write(line)
        macro_name = f'XIP_COPY_DONE_TYPE'
        line = f'#define %-45s %d\r\n' %(macro_name, 2)
        f.write(line)
        macro_name = f'XIP_IMAGE_OK_TYPE'
        line = f'#define %-45s %d\r\n' %(macro_name, 3)
        f.write(line)
        macro_name = f'XIP_IMAGE_TEST_FIRST'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFFFFFF00)
        f.write(line)
        macro_name = f'XIP_IMAGE_TEST_SECOND'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFFFF0000)
        f.write(line)
        macro_name = f'XIP_IMAGE_TEST_FINAL'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFF000000)
        f.write(line)
        macro_name = f'XIP_IMAGE_OK'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0x0)
        f.write(line)
        macro_name = f'XIP_IMAGE_UNKNOWN'
        line = f'#define %-45s 0x%x\r\n' %(macro_name, 0xFFFFFFFF)
        f.write(line)

        # Bootloader version: CONFIG_BOOTLOADER_VERSION and CONFIG_BOOTLOADER_VERSION_BYTES (optional in ota.csv)
        ota = OTA('ota.csv')
        bootloader_version = ota.get_bootloader_version()
        if bootloader_version != '0.0.0' and bootloader_version:
            line = f'#define %-45s "%s"\r\n' % ('CONFIG_BOOTLOADER_VERSION', bootloader_version)
            f.write(line)
            parts = bootloader_version.split('.')
            version_bytes = [int(p) for p in parts[:4]]
            while len(version_bytes) < 4:
                version_bytes.append(0)
            bytes_str = ', '.join(str(b) for b in version_bytes)
            line = f'#define %-45s {{{bytes_str}}}\r\n' % ('CONFIG_BOOTLOADER_VERSION_BYTES')
            f.write(line)
        
        macros = ''
        partition_ids = ''
        id = 0
        struct_array = f"#define BK_FLASH_PARTITIONS_MAP {{ \\\r\n"
        for p in self._partitions:
            macros += p.gen_code() + '\r\n'
            name = p.name()
            partition_ids += f"#define BK_PARTITION_{name.upper().replace(' ', '_')} {id}\r\n"
            id += 1
            struct_array +=  f"    {{BK_FLASH_EMBEDDED, \"{name}\""
            name = name.upper()
            name = name.replace(' ', '_')

            subtype_options = p.flags().flags() + (p.type().subtype() << 24)
            struct_array += f""", CONFIG_{name}_PHY_PARTITION_OFFSET, CONFIG_{name}_PHY_PARTITION_SIZE, {hex(subtype_options)}}}, \\\r\n"""
 
        struct_array += f"}}\r\n"
        f.write(macros)
        f.write(struct_array)
        f.write('\r\n')
        f.write(partition_ids)
        f.write(f"#define BK_PARTITIONS_TABLE_SIZE {id}\r\n")
        f.flush()
        f.close()

    def prebuild_process(self):
        for p in self._partitions:
            p.prebuild_process()
        self.__gen_code('partitions_gen.h')

    def postbuild_process(self, flash_aes_en=False, flash_aes_key=None, img_sign_key=None, flash_crc_en=True, flash_aes_type=None):
        # nvs partitions depend on nvs_key.bin generated by nvs_key partitions
        # Process nvs_key partitions first, then process other partitions (including nvs)
        nvs_key_partitions = []
        other_partitions = []
        for p in self._partitions:
            if isinstance(p, PartitionNvsKey):
                nvs_key_partitions.append(p)
            else:
                other_partitions.append(p)
        
        # Process nvs_key partitions first
        for p in nvs_key_partitions:
            p.postbuild_process(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en, flash_aes_type)
        
        # Then process other partitions (nvs_key.bin should be available now)
        for p in other_partitions:
            p.postbuild_process(flash_aes_en, flash_aes_key, img_sign_key, flash_crc_en, flash_aes_type)

    def gen_primary_all_bin_for_bl2_signing(self, partition_name, app_version):
        # In OTA, generate primary_all.bin from app.bin
        if partition_name != 'primary_all':
            logging.debug(f'Skip generate partition {partition_name}')
            return
        
        logging.debug(f'Generate {partition_name} from app.bin')
        app_bin = 'app.bin'
        all_bin_name = f'{partition_name}.bin'
        
        if not os.path.exists(app_bin):
            logging.error(f'{app_bin} not exists, cannot generate {all_bin_name}')
            return
        
        # Copy app.bin to primary_all.bin
        shutil.copy(app_bin, all_bin_name)
        logging.debug(f'Copied {app_bin} to {all_bin_name}')
        
        # Append version and chip info (same format as original logic)
        parts = app_version.split('.')
        iv_major = struct.pack('B', int(parts[0]))
        iv_minor = struct.pack('B', int(parts[1]))
        iv_revision = struct.pack('<H', int(parts[2]))
        chip = struct.pack('<6s','BK7236N'.encode('utf-8'))
        
        with open(all_bin_name, 'ab') as f:
            f.write(iv_major)
            f.write(iv_minor)
            f.write(iv_revision)
            f.write(chip)
        
        logging.debug(f'Generated {all_bin_name} from {app_bin}')

    def set_version_override(self, partition_name, version):
        """Set version override for a partition
        
        This allows overriding the version from bin.csv with a version passed via command line.
        The override takes priority over bin.csv when adding pack headers.
        
        Args:
            partition_name: Name of the partition (e.g., 'bl2', 'app_a', 'cpu0_app')
            version: Version string (e.g., '1.0.3', '0.0.3')
        """
        self._version_overrides[partition_name] = version
        logging.debug(f'Set version override for partition {partition_name}: {version}')

    def pack_bin(self, pack_json, pubkey_pem, privkey_pem, aes_type, aes_key, security_counter, ota_aes_en, boot_ota=False, ci_test=False, app_version=None, bl2_version=None):
        """Pack bins for Pack Server (after Build and Sign servers)
        
        This method implements the Pack Server functionality in the three-server architecture:
        1. Build Server: compiles source code, generates raw binaries (no private keys)
        2. Sign Server: signs binaries (requires private keys, security isolated)
        3. Pack Server: encrypts, packs, generates final download files (no private keys)
        
        Args:
            pack_json: Path to pack.json file
            pubkey_pem: Path to public key PEM
            privkey_pem: Path to private key PEM
            aes_type: AES encryption type ('FIXED', 'RANDOM', or 'NONE')
            aes_key: AES encryption key
            security_counter: Security counter for OTA
            ota_aes_en: Whether to enable AES encryption for OTA
            boot_ota: Whether to enable boot OTA
            ci_test: Whether this is a CI test build
        """
        logging.info(f'pack_bin, aes_type={aes_type}, aes_key={aes_key}, security_counter={security_counter}, ota_aes_en={ota_aes_en}, boot_ota={boot_ota}, ci_test={ci_test}')

        # Get bl1_secureboot_en and ota_type from security.csv if available
        self._bl1_secureboot_en = self.__get_bl1_secureboot_en()
        self._ota_type = self.__get_ota_type()
        self._boot_ota = boot_ota

        self.parse_and_validate_partitions_after_build()
        self.parse_and_validate_pack_json(pack_json)
        
        # Process partitions: handle encryption, CRC, etc. (similar to bksecure_mfr)
        # These operations work on raw bin files from Sign Server (no pack header needed)
        self.process_bl1_control(aes_type, aes_key, boot_ota)
        self.process_manifest()
        self.process_partition_partition(aes_type)
        self.process_boot_flag(aes_type)
        self.process_nvs(aes_type, aes_key)
        self.process_encrypted(aes_type, aes_key)
        self.process_app_aes_crc(aes_type, aes_key)
        
        # Before calling PackJson.pack(), ensure all bin files have pack headers
        # Pack Server receives signed binaries from Sign Server (may not have pack headers)
        # PackDownload expects files with pack headers, so we need to add them first
        logging.debug('Adding pack headers to bin files for PackDownload')
        self.__ensure_pack_headers_for_pack_json(pack_json, aes_type, aes_key)
        
        # Use PackJson to handle the actual packing (all-app.bin, ota.bin, bootloader.bin, etc.)
        # This is the Pack Server functionality: encrypt, pack, generate final download files
        from .pack_json import PackJson
        pack = PackJson()
        
        # PackJson handles: all-app.bin, ota.bin, bootloader.bin, bootloader_ota.bin, etc.
        flash_crc_en = self._flash_crc_en
        # If pubkey is a hex string (not a file path), convert to .pem file in current directory
        if isinstance(pubkey_pem, str) and not os.path.isfile(pubkey_pem):
            s = pubkey_pem.strip()
            if len(s) >= 64 and len(s) % 2 == 0 and all(c in '0123456789abcdefABCDEF' for c in s):
                der = bytes.fromhex(s)
                b64 = base64.b64encode(der).decode('ascii')
                pem_lines = '\n'.join(b64[i:i + 64] for i in range(0, len(b64), 64))
                pem_content = '-----BEGIN PUBLIC KEY-----\n' + pem_lines + '\n-----END PUBLIC KEY-----\n'
                pubkey_pem_path = os.path.join(os.getcwd(), 'pubkey.pem')
                with open(pubkey_pem_path, 'w') as f:
                    f.write(pem_content)
                pubkey_pem = 'pubkey.pem'
        pack.pack(pack_json, pubkey_pem, privkey_pem, aes_key, flash_crc_en, None, aes_type)
        
        # Call gen methods for compatibility (they are no-ops, actual work done by PackJson)
        self.gen_all_apps_bin(aes_type)  # all_apps depends on nvs/encrypted/app etc
        self.gen_ota_bin(ota_aes_en, aes_key, security_counter, ci_test)
        self.gen_bootloader_ota_bin(aes_type)
        if (self._bl1_secureboot_en == True):
            self.gen_provisioning_bin(aes_type)

    def __ensure_pack_headers_for_pack_json(self, pack_json, aes_type, aes_key):
        """Ensure all bin files referenced in pack.json have pack headers
        
        Pack Server receives signed binaries from Sign Server which may not have pack headers.
        PackDownload expects files with pack headers, so we need to add them before calling PackJson.pack().
        
        This method:
        1. Reads pack.json to find all bin files that need pack headers
        2. For each bin file, checks if it has a pack header
        3. If not, adds pack header using partition information
        """
        from .pack_header import PackHeader, PACK_HEADER_SIZE, PACK_HEADER_MAGIC
        import struct
        from .pack_header import PACK_HEADER_FORMAT
        
        if not os.path.exists(pack_json):
            return
        
        with open(pack_json, 'r') as f:
            pack_data = json.load(f)
        
        # Collect all bin files that need pack headers
        bins_needing_headers = set()
        for k in pack_data:
            bins = pack_data[k].get('bin', [])
            for bin_name in bins:
                bins_needing_headers.add(bin_name)
                
        logging.debug(f'Bins needing pack headers: {bins_needing_headers}')
        logging.debug(f'Bins need override version: {self._version_overrides}')
        # For each bin file, check and add pack header if needed
        for bin_name in bins_needing_headers:
            if bin_name == 'user_mfr.bin':
                logging.debug('Skip adding pack header to user_mfr.bin (handled by USER_MFR_AES flow)')
                continue

            if not os.path.exists(bin_name):
                logging.debug(f'{bin_name} not exists, skip adding pack header')
                continue

            # Find partition name for this bin file
            # Remove .bin extension and try to find partition
            partition_name = bin_name.replace('.bin', '')
            # Handle special cases like pack_primary_manifest.bin -> primary_manifest
            if partition_name.startswith('pack_'):
                partition_name = partition_name[5:]

            # If this partition has a version override, we should always
            # re-generate the header to update the version, even if a header exists.
            has_override = partition_name in self._version_overrides

            # Check if file already has pack header
            has_header = False
            try:
                with open(bin_name, 'rb') as f:
                    header_data = f.read(PACK_HEADER_SIZE)
                    if len(header_data) == PACK_HEADER_SIZE:
                        try:
                            magic_bytes, name_bytes, version_bytes, type_, subtype, offset, size, flags, hdr_size, m1, m2, m3, m4 = struct.unpack(PACK_HEADER_FORMAT, header_data)
                            magic = magic_bytes.rstrip(b'\x00').decode('utf-8')
                            if magic == PACK_HEADER_MAGIC:
                                has_header = True
                                if has_override:
                                    logging.debug(f'{bin_name} already has pack header, but version override exists for {partition_name}, will replace header with override version')
                                    #remove old header
                                    remaining_data = f.read()
                                    with open(bin_name, 'wb') as new_bin_f:
                                        new_bin_f.write(remaining_data)
                                else:
                                    logging.debug(f'{bin_name} already has pack header, skipping')
                        except Exception as e:
                            logging.debug(f'{bin_name} header parse failed ({e}), will add new header')
            except Exception as e:
                logging.debug(f'Failed to check header for {bin_name}: {e}')

            # if already has header and no version override, keep it
            if has_header and not has_override:
                continue

            # Find partition for this bin file
            p = self._find_partition_by_name(partition_name)
            if p is None:
                logging.warning(f'Partition {partition_name} not found for {bin_name}, cannot add pack header')
                continue
            
            # Get bin version from Bin object if available
            version = "1.0.0"  # Default version
            if partition_name in self._version_overrides:
                version = self._version_overrides[partition_name]
                logging.info(f'Using version override for {partition_name}: {version}')
            elif hasattr(p, 'bin') and p.bin():
                version = p.bin().version()
                logging.debug(f'Using version from bin.csv for {partition_name}: {version}')
            # Add pack header
            try:
                ph = PackHeader()
                ph.create_header(
                    bin_name,
                    version,
                    p.name(),
                    p.type().type(),
                    p.type().subtype(),
                    p.size().offset(),
                    p.size().size(),
                    p.flags().flags(),
                    hdr_size,
                    m1, m2, m3, m4
                )
                logging.debug(f'Added pack header to {bin_name}')
            except Exception as e:
                logging.warning(f'Failed to add pack header to {bin_name}: {e}')

    def __get_bl1_secureboot_en(self):
        """Get bl1_secureboot_en from security.csv if available"""
        try:
            from .security import Security
            if os.path.exists('security.csv'):
                s = Security('security.csv')
                return s.bl1_secureboot_en
        except Exception as e:
            logging.debug(f'Failed to read bl1_secureboot_en from security.csv: {e}')
        return False

    def __get_ota_type(self):
        """Get ota_type from security.csv if available, or infer from partitions"""
        try:
            from .security import Security
            if os.path.exists('security.csv'):
                s = Security('security.csv')
                if hasattr(s, 'ota_type'):
                    return s.ota_type
        except Exception as e:
            logging.debug(f'Failed to read ota_type from security.csv: {e}')
        
        # Infer from partitions
        if self.__has_overwrite():
            return 'OVERWRITE'
        elif self.__has_xip():
            return 'XIP'
        return 'NONE'

    def parse_and_validate_partitions_after_build(self):
        """Validate partitions after build is complete"""
        logging.debug('parse_and_validate_partitions_after_build')
        for p in self._partitions:
            if hasattr(p, 'parse_and_validate_after_build'):
                p.parse_and_validate_after_build()

    def parse_and_validate_pack_json(self, pack_json):
        """Parse and validate pack.json file"""
        if pack_json == None:
            logging.error('Invalid pack json')
            exit(1)

        if not os.path.exists(pack_json):
            logging.error(f'pack.json file {pack_json} not exists')
            exit(1)

        self._pack_json_data = None
        with open(pack_json, 'r') as f:
            self._pack_json_data = json.load(f)

        # Build list of all partitions that need to be packed
        self._all_pack_partitions = ['bl2', 'partition']
        for k in self._pack_json_data:
            bins = self._pack_json_data[k].get('bin', [])
            for bin_name in bins:
                # Remove .bin extension if present
                partition_name = bin_name.replace('.bin', '')
                if partition_name not in self._all_pack_partitions:
                    self._all_pack_partitions.append(partition_name)
        
        logging.debug(f'all_pack_partitions: {self._all_pack_partitions}')

    def is_partition_need_pack(self, p):
        """Check if partition needs to be packed"""
        partition_name = p.name()
        if partition_name in self._all_pack_partitions:
            return True
        return False

    def get_pack_idx_list(self, pack_name):
        """Get list of partition indices for a pack bin name"""
        idx_list = []
        if not hasattr(self, '_pack_json_data') or pack_name not in self._pack_json_data:
            logging.debug(f'Pack bin {pack_name} not in pack.json, skip it')
            return idx_list

        bins = self._pack_json_data[pack_name].get('bin', [])
        for bin_name in bins:
            # Remove .bin extension if present
            partition_name = bin_name.replace('.bin', '')
            p = self._find_partition_by_name(partition_name)
            if p:
                idx = self._partitions.index(p)
                idx_list.append(idx)
        
        logging.debug(f'Partition id list of {pack_name}: {idx_list}')
        return idx_list

    def process_bl1_control(self, aes_type, aes_key, boot_ota):
        """Process bl1_control partition"""
        if self._bl1_secureboot_en == False:
            return

        p = self._find_partition_by_name('bl1_control')
        if p == None:
            return

        # bl1_control.bin should already exist from build/sign process
        bin_name = 'bl1_control.bin'
        if not os.path.exists(bin_name):
            logging.warning(f'{bin_name} not exists, creating empty one')
            with open(bin_name, 'wb') as f:
                pad = bytes([0xFF]*(0xF00))
                f.write(pad)

        if aes_type == 'RANDOM':
            logging.debug(f'Copy MSP and PC from provision.bin to {bin_name}')
            if os.path.exists('provision.bin'):
                with open('provision.bin', 'rb') as f:
                    privison_msp_pc = f.read(64)
                    with open(bin_name, 'rb+') as bl1_control_f:
                        bl1_control_f.seek(0)
                        bl1_control_f.write(privison_msp_pc)
            aes_bin_name = bin_name
        else:
            if boot_ota == False:
                logging.debug(f'Copy MSP and PC from bl2.bin to {bin_name}')
                if os.path.exists('bl2.bin'):
                    with open('bl2.bin', 'rb') as f:
                        bl2_msp_pc = f.read(64)
                        with open(bin_name, 'rb+') as bl1_control_f:
                            bl1_control_f.seek(0)
                            bl1_control_f.write(bl2_msp_pc)
            else:
                if os.path.exists("bl2_B.bin") and os.path.exists("jump.bin"):
                    logging.debug(f'Copy jump_bin to {bin_name}')
                    with open("jump.bin", 'rb') as jump_f:
                        jump_data = jump_f.read()
                        with open(bin_name, 'rb+') as bl1_control_f:
                            bl1_control_f.seek(0)
                            bl1_control_f.write(jump_data)

            if aes_type == 'FIXED':
                aes_bin_name = f'{p.name()}_aes.bin'
                aes_tool = get_flash_aes_tool()
                start_address = hex(phy2virtual(p.size().offset()))
                cmd = f'python3 {aes_tool} encrypt -infile {bin_name} -keywords {aes_key} -outfile {aes_bin_name} -startaddress {start_address}'
                run_cmd_not_check_ret(cmd)
            else:
                aes_bin_name = bin_name

        # Add magic code and CRC if needed
        if self._flash_crc_en:
            # Magic code should be added by partition's postbuild_process
            crc_bin_name = f'{p.name()}_crc.bin'
            calc_crc16(aes_bin_name, crc_bin_name)
            final_bin_name = crc_bin_name
        else:
            final_bin_name = aes_bin_name

        # Read partition buffer for packing
        read_size = os.path.getsize(final_bin_name)
        if read_size > p.size().size():
            read_size = p.size().size()

        with open(final_bin_name, 'rb') as f:
            p.partition_buf = bytearray(f.read(read_size))

    def process_manifest(self):
        """Process manifest partitions"""
        p = self._find_partition_by_name('primary_manifest')
        if p != None and hasattr(p, 'bin') and p.bin() and os.path.exists(p.bin().name()):
            with open(p.bin().name(), 'rb') as f:
                p.partition_buf = bytearray(f.read())
        
        p2 = self._find_partition_by_name('secondary_manifest')
        if p2 != None and hasattr(p2, 'bin') and p2.bin() and os.path.exists(p2.bin().name()):
            with open(p2.bin().name(), 'rb') as f:
                p2.partition_buf = bytearray(f.read())

    def process_partition_partition(self, aes_type):
        """Process partition partition (partition.bin)"""
        p = self._find_partition_by_name('partition')
        if p == None:
            logging.debug(f'partition partition not exists, not create partition.bin')
            return

        # partition.bin should already be created by PartitionPartition.postbuild_process
        # Just read it for packing
        bin_name = 'partition.bin'
        if os.path.exists(bin_name):
            with open(bin_name, 'rb') as f:
                p.partition_buf = bytearray(f.read())
        else:
            logging.warning(f'{bin_name} not exists')

    def process_boot_flag(self, aes_type):
        """Process boot_flag partition"""
        if self._bl1_secureboot_en == False:
            return

        p = self._find_partition_by_name('boot_flag')
        if p == None:
            return

        boot_flag_val = bytes([0x63, 0x54, 0x72, 0x4c, 0x1])
        bin_name = 'boot_flag.bin'
        
        # boot_flag.bin should already exist from build process
        if not os.path.exists(bin_name):
            with open(bin_name, 'wb+') as f:
                logging.debug(f'create new {bin_name}')
                pad = bytes([0x0]*(0x20))
                f.write(pad)
                f.seek(32)
                pad = bytes([0xFF]*(0xFE0))
                f.write(pad)

        with open(bin_name, 'rb+') as f:
            f.seek(0)
            f.write(boot_flag_val)

        with open(bin_name, 'rb') as f:
            p.partition_buf = bytearray(f.read())

    def process_nvs(self, aes_type, aes_key):
        """Process NVS and NVS key partitions"""
        nvs_key_partition_cnt = 0
        self._nvs_key_partition = None
        logging.debug(f'process nvs partitions')
        
        # Process nvs_key partitions first
        for p in self._partitions:
            if isinstance(p, PartitionNvsKey):
                if nvs_key_partition_cnt == 1:
                    logging.error(f'More than 2 partition with nvs_key subtype')
                    exit(1)

                if not self.is_partition_need_pack(p):
                    logging.debug(f'process_nvs_key: skip {p.name()}, no need pack')
                    continue

                self._nvs_key_partition = p.name()
                nvs_key_partition_cnt = nvs_key_partition_cnt + 1
                # nvs_key should already be processed by postbuild_process
                # Just read partition_buf if available
                if hasattr(p, 'partition_buf'):
                    pass  # Already processed

        # Process nvs partitions
        for p in self._partitions:
            if isinstance(p, PartitionNvs):
                if not self.is_partition_need_pack(p):
                    logging.debug(f'process_nvs: skip {p.name()}, no need pack')
                    continue
                # nvs should already be processed by postbuild_process
                # Just read partition_buf if available
                if hasattr(p, 'partition_buf'):
                    pass  # Already processed

    def process_encrypted(self, aes_type, aes_key):
        """Process encrypted partitions"""
        logging.debug('process encrypted partition')
        for p in self._partitions:
            if not self.is_partition_need_pack(p):
                logging.debug(f'process_encrypted: skip {p.name()}, no need pack')
                continue
            
            # Encrypted partitions should already be processed by postbuild_process
            # Just ensure partition_buf is available
            if hasattr(p, 'partition_buf'):
                pass  # Already processed

    def process_app_aes_crc(self, aes_type, aes_key):
        """Process app partitions: add AES encryption and CRC"""
        logging.debug('process app partition, add aes and crc')
        for p in self._partitions:
            partition_name = p.name()
            if (partition_name == 'bl2') or (partition_name == 'bl2_B') or (partition_name == 'primary_all'):
                # These partitions should already be processed by postbuild_process
                # Just ensure partition_buf is available
                if hasattr(p, 'partition_buf'):
                    pass  # Already processed
                elif hasattr(p, 'bin') and p.bin() and os.path.exists(p.bin().name()):
                    # Read bin file if partition_buf not available
                    with open(p.bin().name(), 'rb') as f:
                        p.partition_buf = bytearray(f.read())

    def gen_all_apps_bin(self, aes_type):
        """Generate all-app.bin - handled by PackJson in pack() method"""
        logging.debug('gen_all_apps_bin: will be handled by PackJson')
        # The actual packing is done by PackJson.pack() method
        # This method is called for compatibility with bksecure_mfr interface
        pass

    def gen_ota_bin(self, ota_aes_en, aes_key, security_counter, ci_test):
        """Generate ota.bin - handled by PackJson in pack() method"""
        logging.debug(f'gen_ota_bin: will be handled by PackJson (ota_aes_en={ota_aes_en}, security_counter={security_counter}, ci_test={ci_test})')
        # The actual OTA packing is done by PackJson.pack() method
        # This method is called for compatibility with bksecure_mfr interface
        pass

    def gen_bootloader_ota_bin(self, aes_type):
        """Generate bootloader_ota.bin - handled by PackJson in pack() method"""
        if self._boot_ota == False:
            return
        logging.debug('gen_bootloader_ota_bin: will be handled by PackJson')
        # The actual packing is done by PackJson.pack() method
        # This method is called for compatibility with bksecure_mfr interface
        pass

    def gen_provisioning_bin(self, aes_type):
        """Generate provisioning bin - handled by PackJson in pack() method"""
        if self._bl1_secureboot_en == False:
            return
        logging.debug('gen_provisioning_bin: will be handled by PackJson')
        # The actual packing is done by PackJson.pack() method
        # This method is called for compatibility with bksecure_mfr interface
        pass

    def install_bin(self):
        """Install generated bins to install directory"""
        install_dir = 'install'
        os.makedirs(install_dir, exist_ok=True)
        
        # List of files to install
        files_to_install = [
            'all-app.bin',
            'ota.bin',
            'bootloader.bin',
            'bootloader_B.bin',
            'bootloader_ota.bin',
            'provision_pack.bin',
            'otp_efuse_config.json',
            'app.bin',
            'pack_cmd_list.txt',
            'ota_error.bin',
            'ota_vmin.bin',
            'nvs.bin',
            'nvs_key.bin'
        ]
        
        for file_name in files_to_install:
            if os.path.exists(file_name):
                shutil.copy(file_name, f'{install_dir}/{file_name}')
                logging.debug(f'Installed {file_name} to {install_dir}')
            else:
                logging.debug(f'{file_name} not exists, skip install')