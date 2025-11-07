#!/usr/bin/env python3

import os
import os.path
import sys

def main():
	project_dir = sys.argv[1]
	soc = sys.argv[2]

	secure_config_path = os.path.join(project_dir, 'config', soc, 'config')
	if os.path.exists(secure_config_path):
		with open(secure_config_path, 'r') as config_file:
			config_string = config_file.read()
		config_lines = config_string.strip().split('\n')

		config_dict = {}

		for line in config_lines:
			if ('=' not in line) or ('#' in line):
				continue
			key, value = line.split('=')
			config_dict[key.strip()] = value.strip()

		if config_dict.get('CONFIG_SECURITY_FIRMWARE') == 'y':
			print('1')
		else:
			print('0')
	else:
		print('0')

if __name__ == "__main__":
	main()
