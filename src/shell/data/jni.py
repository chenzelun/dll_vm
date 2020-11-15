from shell.common.utils import Debugger
from shell.dex import dex_file
from shell.dex.dex_file import DexFile


class VmKeyFuncJniFile:
    def __init__(self):
        pass

    def add_method(self, method:dex_file.EncodedMethod, dex:DexFile):
        pass

    def to_bytes_h(self) -> bytes:
        pass

    def to_bytes_cpp(self) -> bytes:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass
