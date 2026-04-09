import logging
import os
import shutil
import sys
import argparse
from pathlib import Path


logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def copy_file(src: str, dst: str):
    logger.debug(f"copy {src} to {dst}")
    logger.debug(f"to {dst}")
    if os.path.exists(f"{src}"):
        shutil.copy(f"{src}", f"{dst}")


def copy_files(postfix: str, src_dir: str, dst_dir: str):
    logger.debug(f"copy *{postfix} from {src_dir}")
    logger.debug(f"to {dst_dir}")
    for f in os.listdir(src_dir):
        if f.endswith(postfix):
            if os.path.isfile(f"{src_dir}/{f}"):
                shutil.copy(f"{src_dir}/{f}", f"{dst_dir}/{f}")


def install_configs(cfg_dir: Path, install_dir: Path, secure: int):
    logger.debug(f"install configs from: {cfg_dir}")
    logger.debug(f"to: {install_dir}")
    if not cfg_dir.exists():
        logger.info(f"config dir not found, skip: {cfg_dir}")
        return

    if os.path.exists(f"{cfg_dir}/partitions.csv") and os.path.exists(
        f"{install_dir}/partitions.csv"
    ):
        logger.debug(f"exists {cfg_dir}/partitions.csv")
        logger.debug(f"remove {install_dir}/partitions.csv")
        os.remove(f"{install_dir}/partitions.csv")

    if os.path.exists(f"{cfg_dir}/csv/partitions.csv") and os.path.exists(
        f"{install_dir}/csv/partitions.csv"
    ):
        logger.debug(f"exists {cfg_dir}/csv/partitions.csv")
        logger.debug(f"remove {install_dir}/partitions.csv")
        os.remove(f"{install_dir}/csv/partitions.csv")

    for f in os.listdir(f"{cfg_dir}"):
        if (
            f.endswith(".csv")
            or f.endswith(".bin")
            or f.endswith(".json")
            or f.endswith(".pem")
        ):
            if f == "partitions.csv":
                if secure == 0:
                    continue

            if os.path.isfile(f"{cfg_dir}/{f}"):
                shutil.copy(f"{cfg_dir}/{f}", f"{install_dir}/{f}")

    if os.path.exists(f"{cfg_dir}/key"):
        copy_files(".pem", f"{cfg_dir}/key", str(install_dir))
    if os.path.exists(f"{cfg_dir}/csv"):
        copy_files(".csv", f"{cfg_dir}/csv", str(install_dir))
    if os.path.exists(f"{cfg_dir}/regs"):
        copy_files(".csv", f"{cfg_dir}/regs", str(install_dir))


def resolve_project_config_dir(soc: str, armino_dir: Path, project_dir: Path) -> Path:
    armino_dir = armino_dir.resolve()
    project_dir = project_dir.resolve()

    candidates = [
        project_dir / "config" / soc,
        project_dir / "src" / "config" / soc,
        armino_dir.parent / "src" / "config" / soc,
    ]

    env_project_path = os.getenv("ARMINO_PROJECT_PATH")
    if env_project_path:
        candidates.append(Path(env_project_path) / "config" / soc)

    for cfg_dir in candidates:
        cfg_dir_abs = cfg_dir.resolve()
        if cfg_dir_abs.exists() and cfg_dir_abs.is_dir():
            logger.info(f"resolved project config dir: {cfg_dir_abs}")
            return cfg_dir_abs

    # Keep original behavior as fallback so existing flows are not broken.
    fallback = (project_dir / "config" / soc).resolve()
    logger.warning(
        "failed to resolve project config dir from known locations, "
        f"fallback to: {fallback}"
    )
    return fallback


def prebuild(soc, armino_dir, project_dir, build_dir, secure):

    middleware_soc_cfg_dir = armino_dir / "middleware/boards" / soc
    project_config_dir = resolve_project_config_dir(soc, armino_dir, project_dir)

    logger.info(f"soc={soc}")
    logger.info(f"project_dir={project_dir}")
    logger.info(f"middleware_cfg_dir={middleware_soc_cfg_dir}")
    logger.info(f"project_cfg_dir={project_config_dir}")
    logger.info(f"build_dir={build_dir}")

    if secure:
        bk_utils_script = armino_dir / "tools/env_tools/bksecure/main.py"
    else:
        bk_utils_script = armino_dir / "tools/env_tools/beken_utils/main.py"

    logger.debug("Create temporary _build")
    os.makedirs(build_dir, exist_ok=True)
    os.chdir(build_dir)
    logger.debug(f"cd {build_dir}")
    copy_file(
        f"{middleware_soc_cfg_dir}/partitions/bl1_control.json",
        f"{build_dir}/bl1_control.json",
    )

    install_configs(middleware_soc_cfg_dir, build_dir, secure)
    install_configs(project_config_dir, build_dir, secure)
    logger.debug("partition pre-processing")
    ret = os.system(f"python3 {bk_utils_script} gen all --debug")
    if ret != 0:
        raise RuntimeError(f"run {bk_utils_script} fail")


if __name__ == "__main__":
    set_logging()
    parser = argparse.ArgumentParser(description='Security prebuild process')
    parser.add_argument('--soc', type=str, help='The soc name')
    parser.add_argument('--armino_dir', type=Path, help='The armino dir')
    parser.add_argument('--project_dir', type=Path, help='The project dir')
    parser.add_argument('--build_dir', type=Path, help='The build dir')
    parser.add_argument('--secure', type=int, help='Whether it is a secure build')
    args = parser.parse_args()

    sys.path.append(str(Path(__file__).parent.parent))
    os.chdir(os.getenv("ARMINO_PATH", "."))
    prebuild(args.soc, args.armino_dir, args.project_dir, args.build_dir, args.secure)

    if args.secure == 1:
        partitions_dir = os.path.join(args.build_dir, '..',  "partitions")
        os.makedirs(partitions_dir, exist_ok=True)
        shutil.copy(os.path.join(args.build_dir, "partitions_gen.h"), partitions_dir)
        partition_hdr_file = os.path.join(args.armino_dir, "tools", "build_tools", "partitions.h")
        shutil.copy(partition_hdr_file, partitions_dir)
