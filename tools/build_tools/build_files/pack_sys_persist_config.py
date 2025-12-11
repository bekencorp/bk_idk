#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将 sys_persist_config_4k.bin 插入到 app.bin 头部
"""

import argparse
import logging
import sys
from pathlib import Path

# 设置日志
logging.basicConfig(
    level=logging.INFO,
    format='[%(levelname)s] %(message)s'
)
logger = logging.getLogger(__name__)


def pack_sys_persist_config(
    project_build_dir: str,
    armino_soc: str,
    sys_persist_config_dir: str
) -> bool:
    """
    将 sys_persist_config_4k.bin 插入到 app.bin 头部
    
    Args:
        project_build_dir: 项目构建目录 (PROJECT_BUILD_DIR)
        armino_soc: SOC 名称 (ARMINO_SOC_NAME)
        sys_persist_config_dir: sys_persist_config 目录 (SYS_PERSIST_CONFIG_DIR)
    
    Returns:
        bool: 成功返回 True，失败返回 False
    """
    # 转换为 Path 对象
    project_build_path = Path(project_build_dir)
    sys_persist_config_path = Path(sys_persist_config_dir)
    
    # 检查 sys_persist_config_4k.bin 是否存在
    sys_persist_config_bin = sys_persist_config_path / "sys_persist_config_4k.bin"
    
    if not sys_persist_config_bin.exists():
        logger.info(f"sys_persist_config_4k.bin 不存在于 {sys_persist_config_bin}，跳过插入操作")
        return True
    
    # 构建 app.bin 路径
    app_bin_path = project_build_path / armino_soc / "app.bin"
    
    if not app_bin_path.exists():
        logger.warning(f"app.bin 不存在于 {app_bin_path}，无法插入 sys_persist_config_4k.bin")
        return False
    
    try:
        # 读取 sys_persist_config_4k.bin 内容
        logger.info(f"读取 {sys_persist_config_bin}")
        with sys_persist_config_bin.open("rb") as f:
            sys_persist_data = f.read()
        
        # 读取 app.bin 内容
        logger.info(f"读取 {app_bin_path}")
        with app_bin_path.open("rb") as f:
            app_bin_data = f.read()
        
        # 将 sys_persist_config_4k.bin 插入到 app.bin 头部
        logger.info(f"将 sys_persist_config_4k.bin 插入到 {app_bin_path} 头部")
        with app_bin_path.open("wb") as f:
            # 先写入 sys_persist_config_4k.bin 的内容
            f.write(sys_persist_data)
            # 再写入原 app.bin 的内容
            f.write(app_bin_data)
        
        # 获取文件大小信息
        app_bin_size = app_bin_path.stat().st_size
        sys_persist_size = len(sys_persist_data)
        original_app_size = len(app_bin_data)
        
        logger.info(f"成功插入 {sys_persist_size} 字节到 app.bin 头部")
        logger.info(f"原 app.bin 大小: {original_app_size} 字节")
        logger.info(f"app.bin 总大小: {app_bin_size} 字节")
        
        return True
        
    except Exception as e:
        logger.error(f"插入 sys_persist_config_4k.bin 到 app.bin 头部时出错: {e}")
        return False


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='将 sys_persist_config_4k.bin 插入到 app.bin 头部'
    )
    
    parser.add_argument(
        '--project_build_dir',
        type=str,
        required=True,
        help='项目构建目录 (PROJECT_BUILD_DIR)'
    )
    
    parser.add_argument(
        '--armino_soc',
        type=str,
        required=True,
        help='SOC 名称 (ARMINO_SOC_NAME)'
    )
    
    parser.add_argument(
        '--sys_persist_config_dir',
        type=str,
        required=True,
        help='sys_persist_config 目录 (SYS_PERSIST_CONFIG_DIR)'
    )
    
    args = parser.parse_args()
    
    success = pack_sys_persist_config(
        args.project_build_dir,
        args.armino_soc,
        args.sys_persist_config_dir
    )
    
    if not success:
        sys.exit(1)


if __name__ == "__main__":
    main()

