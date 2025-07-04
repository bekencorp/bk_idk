from .bk_packager_format_linker import bk_packager_format_linker
from .bk_packager import bk_packager
from . import bk_packager_crc_decorator

class bk_packager_format_crc(bk_packager):
    def __init__(self, workdir, pack_json:str, output_file_name="all-app.bin"):
        linker = bk_packager_format_linker()
        super().__init__(workdir, pack_json, linker, output_file_name)

    @bk_packager_crc_decorator.pre_link
    def _pre_link(self):
        pass
    
    @bk_packager_crc_decorator.post_link
    def _post_link(self):
        pass
