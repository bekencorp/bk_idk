from abc import ABC,abstractmethod
from .bk_packager_json import PartitionInfo

class bk_packager_linker(ABC):
    @abstractmethod
    def link(self, part_info:list, output_file_name):
        pass