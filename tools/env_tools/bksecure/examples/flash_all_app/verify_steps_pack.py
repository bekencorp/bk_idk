#!/usr/bin/env python3
"""
Verify bksecure steps_pack using the installed package.

Run from the directory that contains the pack_server/immutable config set
(same file names as examples/flash_all_app/pack_server/immutable). Example:

    cd /path/to/your/firmware/config
    python3 /path/to/bksecure/verify_steps_pack.py
"""

import logging
import os
import sys

from bksecure.scripts.steps import steps_pack

SOC_TYPE = "bk7236n"
FLASH_AES_TYPE = "FIXED"
FLASH_AES_KEY = (
    "73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18"
)
OTA_TYPE = "OVERWRITE"
OTA_SECURITY_COUNTER = 5

# pack_server/immutable (fixed config set for flash_all_app)
REQUIRED_FILES = (
    "nvs.csv",
    "pack.json",
    "partitions.csv",
)


def setup_logging():
    logging.basicConfig(
        level=logging.INFO,
        format="%(levelname)s: %(message)s",
        stream=sys.stdout,
    )


def check_workdir():
    cwd = os.getcwd()
    missing = [name for name in REQUIRED_FILES if not os.path.isfile(name)]
    if missing:
        logging.error("Working directory: %s", cwd)
        logging.error("Missing required file(s): %s", ", ".join(missing))
        logging.error(
            "Run this script from a directory that already has signed bins "
            "and pack config (same as `bksecure steps pack`)."
        )
        return False
    logging.info("Working directory: %s", cwd)
    for name in REQUIRED_FILES:
        logging.info("  found %s", name)
    return True


def main():
    setup_logging()
    if not check_workdir():
        return 1

    logging.info(
        "Calling steps_pack(soc_type=%s, flash_aes_type=%s, ota_type=%s, "
        "ota_security_counter=%s)",
        SOC_TYPE,
        FLASH_AES_TYPE,
        OTA_TYPE,
        OTA_SECURITY_COUNTER,
    )
    try:
        steps_pack(
            SOC_TYPE,
            aes_key_type=FLASH_AES_TYPE,
            aes_key=FLASH_AES_KEY,
            ota_type=OTA_TYPE,
            security_counter=OTA_SECURITY_COUNTER,
        )
    except Exception:
        logging.exception("steps_pack failed")
        return 1

    logging.info("steps_pack completed successfully")
    return 0


if __name__ == "__main__":
    sys.exit(main())
