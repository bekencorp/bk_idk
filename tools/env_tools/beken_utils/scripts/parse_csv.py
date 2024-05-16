#!/usr/bin/env python3
import os
import logging
import shutil
import re
import csv

class Csv:

    def __init__(self, csv_file, is_list=False, supported_key_list=[]):

        self.csv_file = csv_file
        self.is_list = is_list
        self.supported_key_list = supported_key_list
        self.dic_list = []
        self.dic = {}

        logging.debug(f'parse csv_file={csv_file}')
        if not os.path.exists(self.csv_file):
            logging.error(f'csv_file={csv_file} not exists')
            exit(1)

        if self.is_list:
            self.parse_csv_list()
        else:
            self.parse_csv_dic()


    def upper_list(self, str_list):
        upper_str_list = []
        for str in str_list:
            upper_str_list.append(str.upper())
        return upper_str_list

    def check_keys(self, keys):
        if len(self.supported_key_list) == 0:
            return

        upper_keys = self.upper_list(keys)
        upper_supported_key_list = self.upper_list(self.supported_key_list)

        idx = 0
        for key in upper_keys:
            if key not in upper_supported_key_list:
                logging.error(f'Field {keys[idx]} of {self.csv_file} not supported')
                exit(1)
            idx += 1

        idx = 0
        for key in upper_supported_key_list:
            if key not in upper_keys:
                logging.error(f'{self.csv_file} missing Field {self.supported_key_list[idx]}')
                exit(1)
            idx += 1
    
    def parse_csv_list(self):
        with open(self.csv_file, 'r') as f:
            csv_data = csv.reader(f)

            row_idx = 0
            for row in csv_data:
                if row_idx == 0:
                    row_idx = 1
                    self.check_keys(row)
                    continue
    
                dic = {}
                key_idx = 0
                for cell in row:
                    dic[self.supported_key_list[key_idx]] = cell
                    key_idx += 1
                self.dic_list.append(dic)
            #logging.debug(f'dic_list={self.dic_list}')

    def parse_csv_dic(self):
        with open(self.csv_file, 'r') as f:
            csv_data = csv.reader(f)

            actual_keys = []
            row_idx = 0
            for row in csv_data:
                if row_idx == 0:
                    row_idx = 1
                    if (row[0].upper() != 'FIELD') or (row[1].upper() != 'VALUE'):
                        logging.error(f'{self.csv_file} first row should be "Field", "Value"')
                        exit(1)
                    continue
                actual_keys.append(row[0])
                self.dic[row[0]] = row[1]
   
            self.check_keys(actual_keys)
            #logging.debug(f'parse {self.csv_file}: {self.dic}')
