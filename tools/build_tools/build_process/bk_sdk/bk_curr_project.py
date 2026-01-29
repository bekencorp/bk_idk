import os
from pathlib import Path

from bk_sdk.bk_sdk_project import bk_project_info, bk_sdk_project


def get_curr_project():
    if not os.getenv("PROJECT_DIR"):
        raise RuntimeError("get PROJECT_DIR error")
    project_dir = Path(os.getenv("PROJECT_DIR", "")).absolute()
    soc_name = os.getenv("ARMINO_SOC_NAME", "")
    project_name = os.getenv("PROJECT_NAME", "")
    project_build_dir = Path(os.getenv("PROJECT_BUILD_DIR", "")).absolute()
    project_build_targets:str = os.getenv("BUILD_TARGETS", "")

    if not project_dir:
        raise RuntimeError("get project dir error")
    if not project_build_dir:
        raise RuntimeError("get build dir error")

    project_info = bk_project_info(
        project_name,       # project_name
        project_dir,        # project_path
        project_build_dir,  # build_dir
        soc_name,           # soc_name
        True,               # flash_crc_enable (default is True, will be updated later from auto_partitions.csv)
        False,              # flash_aes_enable (default is False, will be updated later from auto_partitions.csv)
        "",                 # flash_aes_key (default is empty)
        project_build_targets.split(),  # apps
    )
    return bk_sdk_project(project_info)


curr_project = get_curr_project()
