#!/usr/bin/python3

import re
import os
import sys
import csv
from collections import OrderedDict


class MapHelper(object):

    def __init__(self, map_file_path) -> None:
        self.map_file_path = map_file_path
        if not os.path.isfile(map_file_path):
            print('{0} does not exist'.format(map_file_path))
            exit(1)
        self.flags = {
            'rodata': ['.rodata.', '.srodata.'],
            'text': ['.text.', '.stext.', '.itcm_sec_code', '.itcm'],
            'data': ['.data.', '.sdata.', '.dtcm_sec_data'],
            'bss': ['.bss.', '.sbss.', '.dtcm_sec_bss'],
        }
        self.size_pat = re.compile(' +0x([a-fA-F\d]+) +0x([a-fA-F\d]+) +(.*)\((.*)\)')
        self.oneline_size_pat = re.compile(' +.* +0x([a-fA-F\d]+) +0x([a-fA-F\d]+) +(.*)\((.*)\)')
        #                0x00000000        0x8 armino/bk_rtos/libbk_rtos.a(rtos_pub.c.obj)
        self.parsed_data = OrderedDict()

    def parse_flag(self, line):
        striped_line = line.strip()
        for (tmp_k, tmp_v) in self.flags.items():
            for tmp_flag in tmp_v:
                if striped_line.startswith(tmp_flag):
                    return tmp_k
        return None
    
    def add_parse_data(self, flag, size, file_path, obj_name):
        if file_path not in self.parsed_data.keys():
            self.parsed_data[file_path] = OrderedDict()
        if obj_name not in self.parsed_data[file_path].keys():
            self.parsed_data[file_path][obj_name] = {
                'rodata': 0,
                'text': 0,
                'data': 0,
                'bss': 0
            }
        self.parsed_data[file_path][obj_name][flag] += size

    def start_parse(self):
        with open(self.map_file_path, 'r') as f:
            tmp_flag = None
            for tmp_line in f.readlines():
                if tmp_flag is None:
                    tmp_flag = self.parse_flag(tmp_line)
                    if tmp_flag is not None:
                        size_m = re.match(self.oneline_size_pat, tmp_line)
                        if size_m:
                            if size_m.group(1) != '00000000':
                                #print('bbbbb', size_m.group(1), 'aaaa', tmp_line)
                                self.add_parse_data(tmp_flag, int(size_m.group(2), 16), size_m.group(3), size_m.group(4))
                                tmp_flag = None
                else:
                    # parse size
                    size_m = re.match(self.size_pat, tmp_line)
                    if size_m:
                        #print(size_m.group(1))
                        if size_m.group(1) != '00000000':
                            #print('aaaaa', tmp_line)
                            self.add_parse_data(tmp_flag, int(size_m.group(2), 16), size_m.group(3), size_m.group(4))
                    # clear flag content
                    tmp_flag = None

    def format_title_line(self):
        return '{0}	{1}	{2}	{3}	{4}	{5}	{6}	{7}\n'.format('text'.rjust(7), 'code'.rjust(7), 'rodata'.rjust(7), 'data'.rjust(7), 'bss'.rjust(7), 'dec'.rjust(7), 'hex'.rjust(7), 'filename')    

    def format_size_line(self, code_size, rodata_size, data_size, bss_size, filename, obj_name):
        text_size = code_size + rodata_size
        dec_size = text_size + data_size + bss_size
        hex_size = hex(dec_size)[2:]
        return ([text_size, code_size, rodata_size, data_size, bss_size, dec_size, '`'+str(hex_size), obj_name + ' (ex ' + filename + ')', filename], 
            '{0}	{1}	{2}	{3}	{4}	{5}	{6}	{7}\n'.format(
            str(text_size).rjust(7), 
            str(code_size).rjust(7), 
            str(rodata_size).rjust(7), 
            str(data_size).rjust(7), 
            str(bss_size).rjust(7), 
            str(dec_size).rjust(7), 
            hex_size.rjust(7),
            obj_name + ' (ex ' + filename + ')'
        ))
    
    def format_total_size_line(self, code_size, rodata_size, data_size, bss_size, filename):
        text_size = code_size + rodata_size
        dec_size = text_size + data_size + bss_size
        hex_size = hex(dec_size)[2:]
        return ( [text_size, code_size, rodata_size, data_size, bss_size, dec_size, '`'+str(hex_size), filename] ,
            '{0}	{1}	{2}	{3}	{4}	{5}	{6}	{7}\n'.format(
            str(text_size).rjust(7), 
            str(code_size).rjust(7), 
            str(rodata_size).rjust(7), 
            str(data_size).rjust(7), 
            str(bss_size).rjust(7), 
            str(dec_size).rjust(7), 
            hex_size.rjust(7),
            filename
        ))

    def save_to_file(self):
        with open(os.path.join(os.path.dirname(self.map_file_path), 'size_map.txt'), 'w') as f:
            with open(os.path.join(os.path.dirname(self.map_file_path), 'size_map_total.csv'), 'w', newline='') as f_csv:
                csv_writer = csv.writer(f_csv)
                csv_writer.writerow(['text', 'code', 'rodata', 'data', 'bss', 'dec', 'hex', 'filename'])
                with open(os.path.join(os.path.dirname(self.map_file_path), 'size_map_detail.csv'), 'w', newline='') as f_csv_detail:
                    csv_detail_writer = csv.writer(f_csv_detail)
                    csv_detail_writer.writerow(['text', 'code', 'rodata', 'data', 'bss', 'dec', 'hex', 'filename', 'filename'])
                    for tmp_file in self.parsed_data.keys():
                        tmp_total = {
                            'text': 0,
                            'rodata': 0,
                            'data': 0,
                            'bss': 0,
                        }
                        f.write(self.format_title_line())
                        for tmp_obj in self.parsed_data[tmp_file].keys():
                            #if tmp_obj == "sys_hal.c.obj":
                            #    print("==========="+self.parsed_data[tmp_file][tmp_obj]['text'])
                            tmp_total['text'] += self.parsed_data[tmp_file][tmp_obj]['text']
                            tmp_total['rodata'] += self.parsed_data[tmp_file][tmp_obj]['rodata']
                            tmp_total['data'] += self.parsed_data[tmp_file][tmp_obj]['data']
                            tmp_total['bss'] += self.parsed_data[tmp_file][tmp_obj]['bss']
                            (detail_csv_tmp, detail_tmp) = self.format_size_line(
                                self.parsed_data[tmp_file][tmp_obj]['text'],
                                self.parsed_data[tmp_file][tmp_obj]['rodata'],
                                self.parsed_data[tmp_file][tmp_obj]['data'],
                                self.parsed_data[tmp_file][tmp_obj]['bss'],
                                tmp_file,
                                tmp_obj
                            )
                            f.write(detail_tmp)
                            csv_detail_writer.writerow(detail_csv_tmp)
                        (csv_tmp, total_tmp) = self.format_total_size_line(tmp_total['text'], tmp_total['rodata'], tmp_total['data'], tmp_total['bss'], tmp_file)
                        f.write(total_tmp)
                        f.write('\n')
                        csv_writer.writerow(csv_tmp)
                        csv_tmp.append(csv_tmp[-1])
                        csv_detail_writer.writerow(csv_tmp)


if __name__ == '__main__':
    print(sys.argv)
    map_file_path = sys.argv[1]
    mh = MapHelper(map_file_path=map_file_path)
    mh.start_parse()
    mh.save_to_file()

