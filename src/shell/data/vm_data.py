import hashlib
import logging
import zlib
from abc import ABCMeta, abstractmethod
from enum import unique, IntEnum
from struct import pack
from typing import Optional, List, Union

from shell.common.utils import Debugger, Pointer, Log


class VDF_Writeable(metaclass=ABCMeta):
    def __init__(self):
        self.offset = 0

    @abstractmethod
    def to_bytes(self, buf: bytearray, pr: Pointer):
        pass

    @abstractmethod
    def __repr__(self):
        pass


# len: 4+20+4
class VDF_Header:
    def __init__(self):
        self.index_size = 0
        self.signature = b''
        self.checksum = 0

    @Debugger.print_all_fields
    def __repr__(self):
        pass


@unique
class VDF_DataType(IntEnum):
    # uint32
    TYPE_KEY_VALUE = 1
    TYPE_FILE = 2


# len: 4+4
class VDF_Index(VDF_Writeable):
    def __init__(self, data_type: VDF_DataType, data=None):
        super().__init__()
        self.type: VDF_DataType = data_type
        self.data_off = 0

        self.data: Optional[VDF_KeyValueData, VDF_FileData] = data

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.data
        self.data_off = self.data.offset
        buf.extend(pack('<2I', self.type.value, self.data_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VDF_String(VDF_Writeable):
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


class VDF_KeyValueData(VDF_Writeable):
    def __init__(self, key: str, val: str):
        super().__init__()
        self.key = VDF_String(key)
        self.val = VDF_String(val)

    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.key.to_bytes(buf, pr)
        self.val.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class VDF_FileData(VDF_Writeable):
    def __init__(self, file_name: str, data: Union[bytes, bytearray]):
        super().__init__()
        self.name = VDF_String(file_name)
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
        data format:
            data
            index
            header
    """

    def __init__(self):
        self.log = logging.getLogger(VmDataFile.__name__)

        self.header = VDF_Header()
        self.index: List[VDF_Index] = []
        self.data: List[Optional[VDF_FileData, VDF_KeyValueData]] = []

    @Log.log_function
    def add_file(self, file_name: str, file_data: Union[bytes, bytearray]):
        data = VDF_FileData(file_name, file_data)
        index = VDF_Index(VDF_DataType.TYPE_FILE, data)
        self.data.append(data)
        self.index.append(index)

    @Log.log_function
    def add_key_value(self, key: str, val: str):
        data = VDF_KeyValueData(key, val)
        index = VDF_Index(VDF_DataType.TYPE_KEY_VALUE, data)
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

        # zip the vm data data
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
