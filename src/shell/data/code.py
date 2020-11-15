import hashlib
import zlib
from abc import abstractmethod, ABCMeta
from struct import pack
from typing import List

from shell.common.utils import Pointer, Debugger
from shell.dex import dex_file


class VKFC_Writeable(metaclass=ABCMeta):
    def __init__(self):
        self.offset = 0

    @abstractmethod
    def to_bytes(self, buf: bytearray, pr: Pointer):
        pass

    @abstractmethod
    def __repr__(self):
        pass


# len: 4+20+4
class VKFC_Header:
    def __init__(self):
        self.index_size = 0
        self.signature = b''
        self.checksum = 0

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VKFC_Code(VKFC_Writeable):
    def __init__(self, code: bytes):
        super().__init__()
        self.code = code

    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(self.code)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VKFC_Index(VKFC_Writeable):
    def __init__(self, method_id: int, code: VKFC_Code):
        super().__init__()
        self.method_id = method_id
        self.code_offset = -1
        self.code = code

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.code
        self.code_offset = self.code.offset
        buf.extend(pack('2I', self.method_id, self.code_offset))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VmKeyFuncCodeFile:
    """
        data format:
            data
            index
            header
    """

    def __init__(self):
        self.header = VKFC_Header()
        self.index: List[VKFC_Index] = []
        self.code: List[VKFC_Code] = []

    def add_method(self, method: dex_file.EncodedMethod):
        code = VKFC_Code(method.code.wrap_to_key_func())
        self.code.append(code)
        index = VKFC_Index(method.method_idx, code)
        self.index.append(index)

    def to_bytes(self) -> bytes:
        buf = bytearray()
        pr = Pointer(0)
        for d in self.code:
            d.to_bytes(buf, pr)
        for i in self.index:
            i.to_bytes(buf, pr)

        self.header.index_size = len(self.index)
        buf.extend(pack('<I', self.header.index_size))
        self.header.signature = hashlib.sha1(buf).digest()
        buf.extend(self.header.signature)
        self.header.checksum = zlib.adler32(buf)
        buf.extend(pack('<I', self.header.checksum))
        return buf

    @Debugger.print_all_fields
    def __repr__(self):
        pass
