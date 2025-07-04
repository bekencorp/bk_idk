from .bk_packager import bk_packager
from .bk_packager_linear_linker import bk_packager_linear_linker
from . import logger

class bk_packager_linear(bk_packager):
    def __init__(self, workdir, pack_json:str, output_file_name="all-app.bin"):
        self.linker = bk_packager_linear_linker()
        super().__init__(workdir, pack_json, self.linker, output_file_name)

    def _pre_link(self):
        pass
    
    def _post_link(self):
        pass

    def set_start_padding_mode(self, mode:bool):
        self.linker.start_padding_mode(mode)
