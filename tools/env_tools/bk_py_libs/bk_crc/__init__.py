
__version__ = '0.0.1'

import logging

def setup_logging():
    logger = logging.getLogger(__name__)
    console_handler = logging.StreamHandler()
    formatter = logging.Formatter('[%(asctime)s|%(name)s|%(levelname)s] %(message)s')
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)
    logger.setLevel(logging.INFO)
    return logger

logger = setup_logging()

def set_debug_log():
    logger.setLevel(logging.DEBUG)

def set_info_log():
    logger.setLevel(logging.INFO)

from .bk_crc16 import bk_crc16
from .bk_crc32 import bk_crc32
