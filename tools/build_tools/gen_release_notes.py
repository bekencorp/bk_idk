#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动生成 release notes 脚本
输入：git tag（格式如 v2.2.1.11）
输出：release_notes.md
"""

import os
import sys
import re
import argparse
import xml.etree.ElementTree as ET
import subprocess
from collections import defaultdict
from typing import List, Tuple, Set, Dict

def check_required_directories():
    """检查必需的目录是否存在"""
    required_dirs = ['.repo', 'include', 'components', 'middleware', 'properties', 'tools']
    missing_dirs = []
    
    for dir_name in required_dirs:
        if not os.path.exists(dir_name):
            missing_dirs.append(dir_name)
    
    if missing_dirs:
        print(f"Error: Required directories not found: {', '.join(missing_dirs)}")
        sys.exit(1)

def parse_manifest_file(manifest_path: str) -> List[Tuple[str, str]]:
    """
    解析 manifest 文件，提取 git repo path 和 group
    
    Args:
        manifest_path: manifest 文件路径
        
    Returns:
        List of (path, group) tuples
    """
    repos = []
    
    if not os.path.exists(manifest_path):
        print(f"Warning: Manifest file not found: {manifest_path}")
        return repos
    
    try:
        tree = ET.parse(manifest_path)
        root = tree.getroot()
        
        # 查找所有 project 节点
        for project in root.findall('.//project'):
            path_attr = project.get('path')
            groups_attr = project.get('groups', '')
            
            if path_attr:
                # groups 可能是多个，用逗号分隔，这里取第一个作为主要 group
                group = groups_attr.split(',')[0].strip() if groups_attr else ''
                repos.append((path_attr, group))
    except ET.ParseError as e:
        # 如果 XML 解析失败，尝试使用正则表达式解析
        print(f"Warning: Failed to parse XML, trying regex: {e}")
        with open(manifest_path, 'r', encoding='utf-8') as f:
            for line in f:
                # 匹配 <project path="..." groups="..." />
                match = re.search(r'<project\s+path="([^"]+)"[^>]*(?:groups="([^"]*)")?', line)
                if match:
                    path = match.group(1)
                    groups = match.group(2) if match.group(2) else ''
                    group = groups.split(',')[0].strip() if groups else ''
                    repos.append((path, group))
    except Exception as e:
        print(f"Error: Failed to parse manifest file {manifest_path}: {e}")
    
    return repos

def get_git_log_for_tag(repo_path: str, tag: str) -> List[str]:
    """
    获取指定 tag 之前的 git log（从 tag 到 HEAD 之间的所有 commit）
    
    Args:
        repo_path: git repo 路径
        tag: git tag
        
    Returns:
        List of git log lines (each line contains commit hash, subject, and body)
    """
    if not os.path.exists(repo_path):
        return []
    
    if not os.path.exists(os.path.join(repo_path, '.git')):
        return []
    
    try:
        # 首先检查 tag 是否存在
        result = subprocess.run(
            ['git', 'rev-parse', '--verify', tag + '^{tag}'],
            cwd=repo_path,
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode != 0:
            # tag 不存在于这个 repo，尝试作为 commit 查找
            result = subprocess.run(
                ['git', 'rev-parse', '--verify', tag],
                cwd=repo_path,
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode != 0:
                return []
        
        # 获取从 tag 到 HEAD 的所有 commit log
        # 使用 --format 输出 commit hash, subject 和 body（用特殊分隔符分隔）
        # 使用 %x00 作为分隔符，避免与 commit message 内容冲突
        result = subprocess.run(
            ['git', 'log', '--format=%H%x00%s%x00%b%x01', f'{tag}..HEAD'],
            cwd=repo_path,
            capture_output=True,
            text=True,
            timeout=30
        )
        
        if result.returncode == 0 and result.stdout.strip():
            # 使用 \x01 分割每个 commit
            commits = result.stdout.split('\x01')
            lines = []
            for commit in commits:
                commit = commit.strip()
                if not commit:
                    continue
                # 使用 \x00 分割 hash, subject, body
                parts = commit.split('\x00', 2)
                if len(parts) >= 2:
                    hash_part = parts[0]
                    subject = parts[1]
                    body = parts[2] if len(parts) > 2 else ''
                    # 使用 | 作为最终分隔符
                    lines.append(f"{hash_part}|{subject}|{body}")
            return lines
        return []
    except subprocess.TimeoutExpired:
        print(f"Warning: Timeout when getting git log for {repo_path}")
        return []
    except Exception as e:
        print(f"Warning: Failed to get git log for {repo_path}: {e}")
        return []
    
    return []

def extract_jira_info(log_lines: List[str], repo_path: str) -> List[Tuple[str, str]]:
    """
    从 git log 中提取 [Jira ...] 格式的信息
    
    Args:
        log_lines: git log 行列表（格式：hash|subject|body）
        repo_path: repo 路径，用于提取模块名
        
    Returns:
        List of (module_name, jira_info) tuples
    """
    jira_infos = []
    seen_jira_ids = set()  # 在同一批 log_lines 中去重，避免同一个 commit 内的重复
    
    # 从 repo_path 提取模块名（作为默认模块名）
    # 例如: properties/modules/wifi/ip_ax -> wifi
    # 或者: middleware/soc/bk7239n -> soc
    default_module_name = ''
    parts = repo_path.split('/')
    if len(parts) >= 2:
        if parts[0] == 'properties' and len(parts) >= 3:
            # properties/modules/wifi/ip_ax -> wifi
            default_module_name = parts[2] if parts[1] == 'modules' else parts[1]
        elif parts[0] in ['middleware', 'components', 'tools']:
            # middleware/soc/bk7239n -> soc
            default_module_name = parts[1] if len(parts) > 1 else parts[0]
        else:
            default_module_name = parts[-1] if parts else repo_path
    else:
        default_module_name = repo_path
    
    # 如果没有合适的模块名，使用 repo_path 的最后一部分
    if not default_module_name:
        default_module_name = os.path.basename(repo_path) if repo_path else 'unknown'
    
    for log_line in log_lines:
        if not log_line.strip():
            continue
        
        # 解析 log line（格式：hash|subject|body）
        parts_line = log_line.split('|', 2)
        subject = parts_line[1] if len(parts_line) > 1 else ''
        body = parts_line[2] if len(parts_line) > 2 else ''
        
        # 清理内容，移除可能的引号后缀（如 " into bk_idk_release_2.2.1）
        # 在 subject 和 body 中查找 [Jira ...] 格式的信息
        search_text = subject + '\n' + body
        
        # 查找所有 [Jira 开头的匹配
        # 格式可能是: [Jira BK7239SW-1850]: <fix> modify persist file from excel to csv
        # 或者: [Jira BK7239SW-1850]: <platform> <fix> modify persist file from excel to csv
        jira_pattern = r'\[Jira\s+([^\]]+)\]:\s*(.+?)(?=\n|\[Jira|$)'
        matches = re.finditer(jira_pattern, search_text, re.IGNORECASE | re.DOTALL)
        
        for match in matches:
            jira_id = match.group(1).strip()
            content = match.group(2).strip()
            
            # 清理内容，移除可能的引号和后续文本（如 " into bk_idk_release_2.2.1）
            # 移除末尾的引号和 " into ..." 这样的文本
            content = re.sub(r'["\']\s*into\s+[^"\']+["\']?\s*$', '', content, flags=re.IGNORECASE)
            content = content.strip().rstrip('"').rstrip("'").strip()
            
            # 规范化空格（多个空格变成一个）
            content = re.sub(r'\s+', ' ', content)
            
            # 如果内容中包含 <module> 标签，提取模块名
            # 例如: <platform> <fix> modify persist file
            module_match = re.match(r'<([^>]+)>\s*(.+)', content)
            if module_match:
                actual_module = module_match.group(1).strip()
                actual_content = module_match.group(2).strip()
                # 使用实际的模块名进行分类，但不在 jira_info 中显示模块名
                # 移除模块名，只保留类型标签和描述
                jira_info = f"[{jira_id}]:  {actual_content}"
                final_module = actual_module
            else:
                # 没有模块标签，使用默认模块名
                jira_info = f"[{jira_id}]: {content}"
                final_module = default_module_name
            
            # 使用 (module, jira_id) 作为去重键，确保同一个模块的同一个 Jira ID 只保留第一个
            dedup_key = (final_module, jira_id)
            if dedup_key not in seen_jira_ids:
                jira_infos.append((final_module, jira_info))
                seen_jira_ids.add(dedup_key)
    
    return jira_infos

def create_release_notes_template(output_file: str):
    """创建 release notes 模板"""
    template = """# Release Notes

## Major Changes

### Major New Features

### Breaking Changes

## Known Issues

## Internal Test Impact

## ChangeLog

"""
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(template)

def update_release_notes(output_file: str, changelog_data: Dict[str, Set[str]]):
    """
    更新 release_notes.md 的 ChangeLog 部分
    
    Args:
        output_file: 输出文件路径
        changelog_data: {module_name: set of jira_infos}
    """
    # 读取现有文件
    if not os.path.exists(output_file):
        create_release_notes_template(output_file)
    
    with open(output_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 找到 ChangeLog 部分
    changelog_pattern = r'(## ChangeLog\n)'
    match = re.search(changelog_pattern, content)
    
    if not match:
        # 如果找不到 ChangeLog 部分，追加到文件末尾
        changelog_section = "\n## ChangeLog\n\n"
        for module in sorted(changelog_data.keys()):
            changelog_section += f"### {module}\n\n"
            for jira_info in sorted(changelog_data[module]):
                changelog_section += f"- {jira_info}\n"
            changelog_section += "\n"
        
        content += changelog_section
    else:
        # 解析现有的 ChangeLog 部分
        changelog_start = match.end()
        changelog_section_text = content[changelog_start:]
        
        # 解析现有的模块和条目
        existing_modules = {}
        
        # 使用正则表达式分割模块
        module_pattern = r'###\s+([^\n]+)\n\n(.*?)(?=\n###\s+|$)'
        for module_match in re.finditer(module_pattern, changelog_section_text, re.DOTALL):
            module_name = module_match.group(1).strip()
            module_content = module_match.group(2)
            
            # 提取现有条目（完整的一行）
            items = set()
            item_pattern = r'-\s+(\[.*?\].*?)(?=\n-|\n\n|$)'
            for item_match in re.finditer(item_pattern, module_content, re.DOTALL):
                items.add(item_match.group(1).strip())
            
            existing_modules[module_name] = items
        
        # 合并新数据到现有数据（去重）
        all_modules = set(existing_modules.keys()) | set(changelog_data.keys())
        merged_modules = {}
        
        for module in all_modules:
            merged_items = set()
            
            # 添加现有条目
            if module in existing_modules:
                merged_items.update(existing_modules[module])
            
            # 添加新条目（基于 Jira ID 去重）
            if module in changelog_data:
                existing_jira_ids = set()
                for item in merged_items:
                    jira_match = re.search(r'\[([^\]]+)\]', item)
                    if jira_match:
                        existing_jira_ids.add(jira_match.group(1))
                
                for jira_info in changelog_data[module]:
                    jira_match = re.search(r'\[([^\]]+)\]', jira_info)
                    if jira_match and jira_match.group(1) not in existing_jira_ids:
                        merged_items.add(jira_info)
            
            if merged_items:
                merged_modules[module] = merged_items
        
        # 重新生成 ChangeLog 部分
        new_changelog_section = ""
        for module in sorted(merged_modules.keys()):
            new_changelog_section += f"### {module}\n\n"
            for jira_info in sorted(merged_modules[module]):
                new_changelog_section += f"- {jira_info}\n"
            new_changelog_section += "\n"
        
        # 替换 ChangeLog 部分
        content = content[:changelog_start] + new_changelog_section
    
    # 写入文件
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)

def generate_release_notes(tag: str, output_file: str = 'release_notes.md'):
    """
    生成 release notes
    
    Args:
        tag: git tag（格式如 v2.2.1.11）
        output_file: 输出文件路径
    """
    print(f"Generating release notes for tag: {tag}")
    print(f"Output file: {output_file}")
    
    # 1. 检查必需的目录
    print("\n1. Checking required directories...")
    check_required_directories()
    print("   ✓ All required directories exist")
    
    # 2. 解析 manifest 文件
    print("\n2. Parsing manifest files...")
    manifest_files = [
        '.repo/manifests/armino_ip.xml',
        '.repo/manifests/armino_sdk.xml'
    ]
    
    all_repos = []
    for manifest_file in manifest_files:
        repos = parse_manifest_file(manifest_file)
        all_repos.extend(repos)
        print(f"   Found {len(repos)} repos in {manifest_file}")
    
    # 去重
    all_repos = list(set(all_repos))
    print(f"   Total unique repos: {len(all_repos)}")
    
    # 3. 创建 release notes 模板
    print("\n3. Creating release notes template...")
    create_release_notes_template(output_file)
    print(f"   ✓ Created {output_file}")
    
    # 4. 遍历每个 repo，提取 changelog
    print("\n4. Extracting changelog from git repos...")
    changelog_data = defaultdict(set)  # {module_name: set of jira_infos}
    jira_id_seen = defaultdict(set)  # {module_name: set of jira_ids} 用于去重
    
    current_dir = os.getcwd()
    processed_count = 0
    
    for repo_path, group in all_repos:
        full_path = os.path.join(current_dir, repo_path)
        
        if not os.path.exists(full_path):
            continue
        
        print(f"   Processing: {repo_path} (group: {group})")
        
        # 获取 git log
        log_lines = get_git_log_for_tag(full_path, tag)
        if not log_lines:
            continue
        
        # 提取 Jira 信息
        jira_infos = extract_jira_info(log_lines, repo_path)
        
        for module_name, jira_info in jira_infos:
            # 提取 Jira ID 用于去重
            jira_match = re.search(r'\[([^\]]+)\]', jira_info)
            if jira_match:
                jira_id = jira_match.group(1)
                # 如果该 Jira ID 在该模块中还没有出现过，则添加
                if jira_id not in jira_id_seen[module_name]:
                    changelog_data[module_name].add(jira_info)
                    jira_id_seen[module_name].add(jira_id)
                    processed_count += 1
        
        if jira_infos:
            print(f"      Found {len(jira_infos)} Jira entries")
    
    print(f"\n   Total processed entries (after deduplication): {processed_count}")
    print(f"   Total modules: {len(changelog_data)}")
    
    # 5. 更新 release notes
    print("\n5. Updating release notes...")
    update_release_notes(output_file, changelog_data)
    print(f"   ✓ Updated {output_file}")
    
    print(f"\n✓ Release notes generation completed!")
    print(f"  Output file: {output_file}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='Generate release notes from git tag')
    parser.add_argument('tag', help='Git tag (e.g., v2.2.1.11)')
    parser.add_argument('-o', '--output', default='release_notes.md',
                       help='Output file path (default: release_notes.md)')
    
    args = parser.parse_args()
    
    # 验证 tag 格式
    if not re.match(r'^v?\d+\.\d+\.\d+', args.tag):
        print(f"Warning: Tag format may be incorrect: {args.tag}")
        print("Expected format: v2.2.1.11 or 2.2.1.11")
    
    try:
        generate_release_notes(args.tag, args.output)
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()

