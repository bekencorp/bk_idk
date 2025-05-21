#!/usr/bin/env python3
import os
import re
import sys

def parse_kconfig_files(kconfig_dir):
    """解析所有Kconfig文件，返回配置项描述字典"""
    config_dict = {}
    kconfig_files = []

    # 递归查找所有Kconfig文件
    for root, _, files in os.walk(kconfig_dir):
        for file in files:
            if file == "Kconfig" or file.endswith(".kconfig"):
                kconfig_files.append(os.path.join(root, file))

    # 解析每个Kconfig文件
    for kconfig_file in kconfig_files:
        with open(kconfig_file, 'r', encoding='utf-8', errors='ignore') as f:
            lines = [line.rstrip() for line in f.readlines()]
            i = 0
            while i < len(lines):
                line = lines[i].strip()

                # 匹配config定义
                if re.match(r'^\s*config\s+(\w+)', line):
                    config_name = re.match(r'^\s*config\s+(\w+)', line).group(1)
                    description = None

                    # 查找help或引号内容
                    j = i + 1
                    while j < len(lines):
                        current_line = lines[j]

                        # 查找记录第一个非空引号内容
                        if description is None:
                            quoted_match = re.search(r'"([^"]+)"', current_line)
                            if quoted_match and quoted_match.group(1).strip():
                                description = quoted_match.group(1).strip()
                                #break

                        # 查找help（忽略前导空格/制表符）
                        if re.match(r'^\s*help\s*$', current_line):
                            if j + 1 < len(lines):
                                description = lines[j + 1].strip()
                                break

                        #查找结束
                        if re.match(r'^\s*(config|endmenu)\s+(\w+)', current_line):
                            break

                        j += 1

                    # 保存找到的描述
                    if description and config_name not in config_dict:
                        config_dict[config_name] = description

                i += 1

    return config_dict

def process_defconfig_file(defconfig_path, kconfig_dict):
    """处理defconfig文件，在宏定义前一行添加##注释"""
    modified_lines = []

    with open(defconfig_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
        i = 0
        while i < len(lines):
            line = lines[i].rstrip()

            # 跳过空行和注释行
            if not line or line.startswith('#'):
                modified_lines.append(line)
                i += 1
                continue

            if '=' in line:
                # 分割配置名和值
                config_part = line.split('=', 1)[0]
                config_name = config_part.replace('CONFIG_', '', 1).strip()

                # 获取描述
                description = kconfig_dict.get(config_name, '')

                if description:
                    # 在宏定义前添加注释行
                    comment_line = f"#@ {description}"

                    # 检查是否已有相同注释
                    if i > 0 and modified_lines and modified_lines[-1].startswith("#@"):
                        modified_lines[-1] = comment_line    # 替换已有注释
                    else:
                        modified_lines.append(comment_line)  # 添加新注释

                # 添加宏定义行
                modified_lines.append(line)
                i += 1
            else:
                modified_lines.append(line)
                i += 1

    # 写回文件
    if modified_lines:
        with open(defconfig_path, 'w', encoding='utf-8') as f:
            f.write('\n'.join(modified_lines).strip() + '\n')

def process_all_defconfigs(defconfig_dir, kconfig_dict):
    """递归处理目录下的所有defconfig文件"""
    defconfig_files = []
    
    # 递归查找所有defconfig文件
    for root, _, files in os.walk(defconfig_dir):
        for file in files:
            if (file.endswith("defconfig") or
                file.endswith(".config") or
                file == "config"):
                defconfig_files.append(os.path.join(root, file))

    # 处理每个defconfig文件
    for defconfig_file in defconfig_files:
        print(f"处理文件: {defconfig_file}")
        try:
            process_defconfig_file(defconfig_file, kconfig_dict)
        except Exception as e:
            print(f"处理文件 {defconfig_file} 时出错: {str(e)}")

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <defconfig_dir> <kconfig_dir>")
        sys.exit(1)

    defconfig_dir = sys.argv[1]
    kconfig_dir = sys.argv[2]

    if not os.path.isdir(defconfig_dir):
        print(f"Error: Defconfig file not found: {defconfig_dir}")
        sys.exit(1)

    if not os.path.isdir(kconfig_dir):
        print(f"Error: Kconfig directory not found: {kconfig_dir}")
        sys.exit(1)

    #print("parse Kconfig files...")
    kconfig_dict = parse_kconfig_files(kconfig_dir)
    
    #print("handle defconfig file...")
    process_all_defconfigs(defconfig_dir, kconfig_dict)

    print("macro add comment suc！")

if __name__ == "__main__":
    main()