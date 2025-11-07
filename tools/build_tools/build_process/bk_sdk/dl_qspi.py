from __future__ import annotations

import csv
import os
import shutil
from pathlib import Path

from .bk_curr_project import curr_project

armino_path = curr_project.sdk_path
project_dir = curr_project.project_path
chip = curr_project.soc_name
build_path = curr_project.project_build_dir / curr_project.soc_name
qspi_dl_tool = f"{armino_path}/tools/env_tools/qspi_dl/gen_all_final_image.py"


# Read the csv file
def read_csv_data(file_path: str) -> dict[str, str]:
    data: dict[str, str] = {}
    with open(file_path, "r") as file:
        reader = csv.reader(file)
        next(reader)  # Skip header line
        for row in reader:
            data[row[0]] = row[1]
    return data


# get qspi_dl csv key value {TRUE :need do qspi download,FALSE :not need do qspi download }
def get_qspi_dl_value(input_file: str):
    data = read_csv_data(input_file)
    if data.get("qspi_dl", "false").lower() == "true":
        output_value = True
    else:
        output_value = False

    return output_value


# get qspi download flash's csv status {TRUE :need do qspi download flash,FALSE :not need do qspi download flash }
def get_qspi_dl_flash_csv_status():
    if os.path.exists("%s/config/%s/qspi_dl.csv" % (project_dir, chip)):
        qspi_dl_file = "%s/config/%s/qspi_dl.csv" % (project_dir, chip)
        qspi_dl_status = get_qspi_dl_value(qspi_dl_file)
    elif os.path.exists("%s/middleware/boards/%s/qspi_dl.csv" % (armino_path, chip)):
        qspi_dl_file = "%s/middleware/boards/%s/qspi_dl.csv" % (armino_path, chip)
        qspi_dl_status = get_qspi_dl_value(qspi_dl_file)
    else:
        qspi_dl_status = False
    # print(' >>>>>>>>>>>>pos_independent_status: %s '%pos_independent_status)
    return qspi_dl_status


def get_qspi_dl_flash_sub_image():
    if os.path.exists("%s/config/%s/sub.bin" % (project_dir, chip)):
        qspi_dl_sub_file = "%s/config/%s/sub.bin" % (project_dir, chip)
    elif os.path.exists("%s/middleware/boards/%s/sub.bin" % (armino_path, chip)):
        qspi_dl_sub_file = "%s/middleware/boards/%s/sub.bin" % (armino_path, chip)
    else:
        raise Exception("sub.bin is not found")
    return qspi_dl_sub_file


def do_qspi_dl_flash_package(
    src1_file: str, pos1_addr: int, pos2_addr: int, dest_file: str
):
    if get_qspi_dl_flash_csv_status():
        print("do qspi package")
        qspi_dl_sub_file = get_qspi_dl_flash_sub_image()
        shutil.copy(qspi_dl_sub_file, build_path)
        ret = os.system(
            f"python3 {qspi_dl_tool} {src1_file} {qspi_dl_sub_file} {pos1_addr} {pos2_addr}  {dest_file}"
        )
        if ret:
            raise RuntimeError("qspi dl flash package failed")
        curr_project.build_summary += (
            f"qspi_dl_flash_package:{Path(dest_file).absolute()}\n"
        )
    else:
        print("not do qspi package")
