import hashlib
import logging
import zlib
from abc import ABCMeta, abstractmethod
from enum import unique, IntEnum
from struct import pack
from typing import Optional, List, Union

from shell.common.utils import Debugger, Pointer, Log


class Writeable(metaclass=ABCMeta):
    def __init__(self):
        self.offset = 0

    @abstractmethod
    def to_bytes(self, buf: bytearray, pr: Pointer):
        pass

    @abstractmethod
    def __repr__(self):
        pass


# len: 4+20+4
class VmHeader:
    def __init__(self):
        self.index_size = 0
        self.signature = b''
        self.checksum = 0

    @Debugger.print_all_fields
    def __repr__(self):
        pass


@unique
class VmDataType(IntEnum):
    # uint32
    TYPE_KEY_VALUE = 1
    TYPE_FILE = 2


# len: 4+4
class VmIndex(Writeable):
    def __init__(self, data_type: VmDataType, data=None):
        super().__init__()
        self.type: VmDataType = data_type
        self.data_off = 0

        self.data: Optional[VmKeyValueData, VmFileData] = data

    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.data
        self.data_off = self.data.offset
        buf.extend(pack('<2I', self.type.value, self.data_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VmString(Writeable):
    def __init__(self, data: str):
        super().__init__()
        self.data_size = 0
        self.data = data

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        data_bytes = bytes(self.data, encoding='ASCII')
        self.data_size = len(data_bytes) + 1
        buf.extend(pack('<I', self.data_size))
        buf.extend(data_bytes)
        buf.append(0)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VmKeyValueData(Writeable):
    def __init__(self, key: str, val: str):
        super().__init__()
        self.key = VmString(key)
        self.val = VmString(val)

    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.key.to_bytes(buf, pr)
        self.val.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VmFileData(Writeable):
    def __init__(self, file_name: str, data: Union[bytes, bytearray]):
        super().__init__()
        self.name = VmString(file_name)
        self.data_size = len(data)
        self.data = data

    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.name.to_bytes(buf, pr)
        buf.extend(pack('<I', self.data_size))
        assert self.data
        buf.extend(self.data)
        pr.add(0x04 + len(self.data))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VmDataFile:
    """
        file format:
            data
            index
            header
    """

    def __init__(self):
        self.log = logging.getLogger(VmDataFile.__name__)

        self.header = VmHeader()
        self.index: List[VmIndex] = []
        self.data: List[Optional[VmFileData, VmKeyValueData]] = []

    @Log.log_function
    def add_file(self, file_name: str, file_data: Union[bytes, bytearray]):
        data = VmFileData(file_name, file_data)
        index = VmIndex(VmDataType.TYPE_FILE, data)
        self.data.append(data)
        self.index.append(index)

    @Log.log_function
    def add_key_value(self, key: str, val: str):
        data = VmKeyValueData(key, val)
        index = VmIndex(VmDataType.TYPE_KEY_VALUE, data)
        self.data.append(data)
        self.index.append(index)

    def to_bytes(self) -> bytes:
        buf = bytearray()
        pr = Pointer(0)
        for d in self.data:
            d.to_bytes(buf, pr)
        for i in self.index:
            i.to_bytes(buf, pr)

        self.header.index_size = len(self.index)
        buf.extend(pack('<I', self.header.index_size))
        self.header.signature = hashlib.sha1(buf).digest()
        buf.extend(self.header.signature)
        self.header.checksum = zlib.adler32(buf)
        buf.extend(pack('<I', self.header.checksum))

        # zip the vm data file
        # file_name = r'vm_data'
        # tmp_vm_data_dir = os.path.join(env.TMP_ROOT, file_name)
        # shutil.rmtree(tmp_vm_data_dir)
        # os.makedirs(tmp_vm_data_dir)
        # with open(os.path.join(tmp_vm_data_dir, file_name + r'.bin'), 'wb') as w:
        #     w.write(buf)
        # shutil.make_archive(tmp_vm_data_dir, r'zip', tmp_vm_data_dir, logger=self.log)
        # with open(tmp_vm_data_dir+'.zip', 'rb') as r:
        #     buf = r.read()

        return buf
