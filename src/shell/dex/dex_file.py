import hashlib
import logging
import os
import zlib
from abc import ABCMeta, abstractmethod
from enum import IntEnum, unique
from struct import unpack_from, pack
from typing import Optional, List, Dict, Union

import env
from shell.common.utils import Debugger, Pointer

NO_INDEX = 0xffffffff


@unique
class MapListItemType(IntEnum):
    TYPE_HEADER_ITEM = 0x0000
    TYPE_STRING_ID_ITEM = 0x0001
    TYPE_TYPE_ID_ITEM = 0x0002
    TYPE_PROTO_ID_ITEM = 0x0003
    TYPE_FIELD_ID_ITEM = 0x0004
    TYPE_METHOD_ID_ITEM = 0x0005
    TYPE_CLASS_DEF_ITEM = 0x0006
    TYPE_MAP_LIST = 0x1000
    TYPE_TYPE_LIST_ITEM = 0x1001
    TYPE_ANNOTATION_SET_REF_LIST = 0x1002
    TYPE_ANNOTATION_SET_ITEM = 0x1003
    TYPE_CLASS_DATA_ITEM = 0x2000
    TYPE_CODE_ITEM = 0x2001
    TYPE_STRING_DATA_ITEM = 0x2002
    TYPE_DEBUG_INFO_ITEM = 0x2003
    TYPE_ANNOTATION_ITEM = 0x2004
    TYPE_ENCODED_ARRAY_ITEM = 0x2005
    TYPE_ANNOTATION_DIRECTORY_ITEM = 0x2006


@unique
class EndianTag(IntEnum):
    ENDIAN_CONSTANT = 0x12345678
    REVERSE_ENDIAN_CONSTANT = 0x78563412


@unique
class EncodedValueType(IntEnum):
    VALUE_BYTE = 0x00
    VALUE_SHORT = 0x02
    VALUE_CHAR = 0x03
    VALUE_INT = 0x04
    VALUE_LONG = 0x06
    VALUE_FLOAT = 0x10
    VALUE_DOUBLE = 0x11
    VALUE_STRING = 0x17
    VALUE_TYPE = 0x18
    VALUE_FIELD = 0x19
    VALUE_METHOD = 0x1a
    VALUE_ENUM = 0x1b
    VALUE_ARRAY = 0x1c
    VALUE_ANNOTATION = 0x1d
    VALUE_NULL = 0x1e
    VALUE_BOOLEAN = 0x1f


class AccessFlag(IntEnum):
    ACC_NONE = 0x00000000  # none AccessFlag
    ACC_PUBLIC = 0x00000001  # class , field, method, ic
    ACC_PRIVATE = 0x00000002  # field, method, ic
    ACC_PROTECTED = 0x00000004  # field, method, ic
    ACC_STATIC = 0x00000008  # field, method, ic
    ACC_FINAL = 0x00000010  # class , field, method, ic
    ACC_SYNCHRONIZED = 0x00000020  # method(only allowed on natives)
    ACC_SUPER = 0x00000020  # class(not used in Dalvik)
    ACC_VOLATILE = 0x00000040  # field
    ACC_BRIDGE = 0x00000040  # method(1.5)
    ACC_TRANSIENT = 0x00000080  # field
    ACC_VARARGS = 0x00000080  # method(1.5)
    ACC_NATIVE = 0x00000100  # method
    ACC_INTERFACE = 0x00000200  # class , ic
    ACC_ABSTRACT = 0x00000400  # class , method, ic
    ACC_STRICT = 0x00000800  # method
    ACC_SYNTHETIC = 0x00001000  # field, method, ic
    ACC_ANNOTATION = 0x00002000  # class , ic (1.5)
    ACC_ENUM = 0x00004000  # class , field, ic (1.5)
    ACC_CONSTRUCTOR = 0x00010000  # method(Dalvik only)
    ACC_DECLARED_SYNCHRONIZED = 0x00020000  # method(Dalvik only)


@unique
class DebugInfoOpCodes(IntEnum):
    DBG_END_SEQUENCE = 0x00
    DBG_ADVANCE_PC = 0x01
    DBG_ADVANCE_LINE = 0x02
    DBG_START_LOCAL = 0x03
    DBG_START_LOCAL_EXTENDED = 0x04
    DBG_END_LOCAL = 0x05
    DBG_RESTART_LOCAL = 0x06
    DBG_SET_PROLOGUE_END = 0x07
    DBG_SET_EPILOGUE_BEGIN = 0x08
    DBG_SET_FILE = 0x09
    DBG_FIRST_SPECIAL = 0x0a
    DBG_LINE_BASE = -4
    DBG_LINE_RANGE = 15


@unique
class Visibility(IntEnum):
    VISIBILITY_BUILD = 0x0
    VISIBILITY_RUNTIME = 0x1
    VISIBILITY_SYSTEM = 0x2


class Leb128:
    @staticmethod
    @Pointer.tmp_reset_step(step=1)
    def read_unsigned_leb128(buf: bytes, pr: Pointer) -> int:
        result: int = buf[pr.cur_and_increase]
        if result > 0x7f:
            cur: int = buf[pr.cur_and_increase]
            result = (result & 0x7f) | ((cur & 0x7f) << 7)
            if cur > 0x7f:
                cur = buf[pr.cur_and_increase]
                result |= (cur & 0x7f) << 14
                if cur > 0x7f:
                    cur = buf[pr.cur_and_increase]
                    result |= (cur & 0x7f) << 21
                    if cur > 0x7f:
                        cur = buf[pr.cur_and_increase]
                        result |= cur << 28

        return result

    @staticmethod
    def write_unsigned_leb128(data: int, pr: Pointer = None) -> bytearray:
        result = bytearray()
        while True:
            out: int = data & 0x7f
            if out != data:
                result.append(out | 0x80)
                data >>= 7
            else:
                result.append(out)
                break
        if pr:
            pr.add(len(result))
        return result

    @staticmethod
    @Pointer.tmp_reset_step(step=1)
    def pass_leb128(buf: bytes, pr: Pointer, size: int = 1):
        while size > 0:
            if buf[pr.cur_and_increase] <= 0x7f:
                size -= 1

    @staticmethod
    @Pointer.tmp_reset_step(step=1)
    def read_signed_leb128(buf: bytes, pr: Pointer) -> int:
        result: int = buf[pr.cur_and_increase]
        if result <= 0x7f:
            if result > 0x3f:
                result |= ~0x7f
        else:
            cur = buf[pr.cur_and_increase]
            result = (result & 0x7f) | ((cur & 0x7f) << 7)
            if cur <= 0x7f:
                if cur > 0x3f:
                    result |= ~0x3fff
            else:
                cur = buf[pr.cur_and_increase]
                result |= (cur & 0x7f) << 14
                if cur <= 0x7f:
                    if cur > 0x3f:
                        result |= ~0x1fffff
                else:
                    cur = buf[pr.cur_and_increase]
                    result |= (cur & 0x7f) << 21
                    if cur <= 0x7f:
                        if cur > 0x3f:
                            result |= ~0x0fffffff
                    else:
                        cur = buf[pr.cur_and_increase]
                        result |= cur << 28
                        if cur > 0x07:
                            result |= ~0x7fffffff
        return result

    @staticmethod
    def write_signed_leb128(data: int, pr: Pointer = None) -> bytearray:
        result = bytearray()
        while True:
            out: int = data & 0x7f
            data >>= 7
            if data == 0x00 or (data == ~0x00 and out > 0x3f):
                result.append(out)
                break
            else:
                result.append(out | 0x80)
        if pr:
            pr.add(len(result))
        return result

    @staticmethod
    def read_unsigned_leb128p1(buf: bytes, pr: Pointer) -> int:
        return Leb128.read_unsigned_leb128(buf, pr) - 1

    @staticmethod
    def write_unsigned_leb128p1(data: int, pr: Pointer = None) -> bytearray:
        return Leb128.write_unsigned_leb128(data + 1, pr)


class Writeable(metaclass=ABCMeta):
    def __init__(self):
        self.offset = -1

    @abstractmethod
    def parse(self, buf: bytes, pr: Pointer):
        """ Convert binary to data structure. """
        pass

    @abstractmethod
    def to_bytes(self, buf: bytearray, pr: Pointer):
        """ Convert data structure to binary. """
        pass

    @abstractmethod
    def __repr__(self):
        """ debug object. """


class MapItem(Writeable):
    def __init__(self):
        super().__init__()
        self.type: Optional[MapListItemType] = None
        self.unused = 0
        self.size = 0
        self.data_offset = 0

        self.data: Optional[Pool] = None

    def parse(self, buf: bytes, pr: Pointer):
        map_type, \
        self.unused, \
        self.size, \
        self.data_offset = unpack_from('<2H2I', buf, pr.get_and_add(len(self)))
        self.type = MapListItemType(map_type)
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(pack('<2H2I',
                        self.type.value,
                        self.unused,
                        self.size,
                        self.data_offset))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x0c


INDEX_POOL_TYPE = (
    MapListItemType.TYPE_STRING_ID_ITEM,
    MapListItemType.TYPE_TYPE_ID_ITEM,
    MapListItemType.TYPE_PROTO_ID_ITEM,
    MapListItemType.TYPE_FIELD_ID_ITEM,
    MapListItemType.TYPE_METHOD_ID_ITEM,
    MapListItemType.TYPE_CLASS_DEF_ITEM,
)
OFFSET_POOL_TYPE = (
    MapListItemType.TYPE_TYPE_LIST_ITEM,
    MapListItemType.TYPE_ANNOTATION_SET_REF_LIST,
    MapListItemType.TYPE_ANNOTATION_SET_ITEM,
    MapListItemType.TYPE_CLASS_DATA_ITEM,
    MapListItemType.TYPE_CODE_ITEM,
    MapListItemType.TYPE_STRING_DATA_ITEM,
    MapListItemType.TYPE_DEBUG_INFO_ITEM,
    MapListItemType.TYPE_ANNOTATION_ITEM,
    MapListItemType.TYPE_ENCODED_ARRAY_ITEM,
    MapListItemType.TYPE_ANNOTATION_DIRECTORY_ITEM,
)


class Pool(Writeable):
    def __init__(self, map_item: MapItem, val_type: type):
        super().__init__()
        self.__order: List[int] = []
        self.__map: Dict[int,
                         Union[Writeable,
                               StringIdItem,
                               TypeIdItem]] = {}
        self.__map_item = map_item
        self.__val_type = val_type

        self.__iter_index: int = 0

    def get_item(self, idx: int):
        return self.__map[self.__order[idx]]

    def parse(self, buf: bytes, pr: Pointer):
        for idx in range(self.__map_item.size):
            item: Writeable = self.__val_type().parse(buf, pr)
            key = idx if self.__map_item.type in INDEX_POOL_TYPE else item.offset
            self.__map[key] = item
            self.__order.append(key)
        return self

    def add(self, item: Writeable):
        key = len(self.__order) if self.__map_item.type in INDEX_POOL_TYPE else item.offset
        self.__map[key] = item
        self.__order.append(key)

    def __getitem__(self, key: int):
        return self.__map[key]

    def __contains__(self, key: int):
        return key in self.__map

    @Pointer.aligned_4_with_zero
    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        for key in self.__order:
            self.__map[key].to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return len(self.__map)

    def __iter__(self):
        self.__iter_index = 0
        return self

    def __next__(self):
        if self.__iter_index >= len(self.__order):
            raise StopIteration()
        ret = self[self.__order[self.__iter_index]]
        self.__iter_index += 1
        return ret


class Header(Writeable):
    def __init__(self):
        super().__init__()
        self.magic: bytes = b''
        self.checksum: int = 0
        self.signature: bytes = b''
        self.file_size: int = 0
        self.header_size: int = 0
        self.endian_tag: int = 0
        self.link_size: int = 0
        self.link_off: int = 0
        self.map_off: int = 0
        self.string_ids_size: int = 0
        self.string_ids_off: int = 0
        self.type_ids_size: int = 0
        self.type_ids_off: int = 0
        self.proto_ids_size: int = 0
        self.proto_ids_off: int = 0
        self.field_ids_size: int = 0
        self.field_ids_off: int = 0
        self.method_ids_size: int = 0
        self.method_ids_off: int = 0
        self.class_defs_size: int = 0
        self.class_defs_off: int = 0
        self.data_size: int = 0
        self.data_off: int = 0

    def parse(self, buf: bytes, pr: Pointer):
        assert pr.cur == 0
        self.magic: bytes = buf[pr.cur:pr.cur + 0x08]
        self.checksum: int = unpack_from('<I', buf, pr.cur + 0x08)[0]
        self.signature: bytes = buf[pr.cur + 0x0c: pr.cur + 0x20]
        self.file_size, \
        self.header_size, \
        self.endian_tag, \
        self.link_size, \
        self.link_off, \
        self.map_off, \
        self.string_ids_size, \
        self.string_ids_off, \
        self.type_ids_size, \
        self.type_ids_off, \
        self.proto_ids_size, \
        self.proto_ids_off, \
        self.field_ids_size, \
        self.field_ids_off, \
        self.method_ids_size, \
        self.method_ids_off, \
        self.class_defs_size, \
        self.class_defs_off, \
        self.data_size, \
        self.data_off = unpack_from('<20I', buf, pr.cur + 0x20)
        pr.get_and_add(len(self))
        return self

    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(self.magic)
        buf.extend(pack('<I', self.checksum))
        buf.extend(self.signature)
        buf.extend(pack('<20I',
                        self.file_size,
                        self.header_size,
                        self.endian_tag,
                        self.link_size,
                        self.link_off,
                        self.map_off,
                        self.string_ids_size,
                        self.string_ids_off,
                        self.type_ids_size,
                        self.type_ids_off,
                        self.proto_ids_size,
                        self.proto_ids_off,
                        self.field_ids_size,
                        self.field_ids_off,
                        self.method_ids_size,
                        self.method_ids_off,
                        self.class_defs_size,
                        self.class_defs_off,
                        self.data_size,
                        self.data_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x70


class StringDataItem(Writeable):
    def __init__(self):
        super().__init__()
        self.size = -1
        self.data = b''

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.size = Leb128.read_unsigned_leb128(buf, pr)
        start = pr.cur
        while buf[pr.get_and_add(1)]:
            pass
        self.data = buf[start:pr.cur - 1]
        return self

    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(Leb128.write_unsigned_leb128(self.size))
        buf.extend(self.data)
        buf.append(0x00)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class StringIdItem(Writeable):
    def __init__(self):
        super().__init__()
        self.data_offset = -1
        self.data: Optional[StringDataItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.data_offset = unpack_from('<I', buf, pr.get_and_add(len(self)))[0]
        assert self.data_offset
        self.data = MapList.get_item_and_add(
            MapListItemType.TYPE_STRING_DATA_ITEM,
            self.data_offset, StringDataItem, buf
        )
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.data.offset != -1
        self.data_offset = self.data.offset
        buf.extend(pack('<I', self.data_offset))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x04


class TypeIdItem(Writeable):
    def __init__(self):
        super().__init__()
        self.descriptor_id: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.descriptor_id = unpack_from('<I', buf, pr.get_and_add(len(self)))[0]
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(pack('<I', self.descriptor_id))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x04


class TypeListItem(Writeable):
    def __init__(self):
        super().__init__()
        self.size: int = -1
        self.list: List[int] = []

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.size = unpack_from('<I', buf, pr.get_and_add(0x04))[0]
        self.list.extend(unpack_from('<' + str(self.size) + 'H', buf, pr.get_and_add(self.size * 0x02)))
        return self

    @Pointer.aligned_4_with_zero
    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(pack('<I', self.size))
        buf.extend(pack('<' + str(self.size) + 'H', *self.list))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ProtoIdItem(Writeable):
    def __init__(self):
        super().__init__()
        self.shorty_id: int = -1
        self.return_type_idx: int = -1
        self.parameters_off: int = -1
        self.parameters: Optional[TypeIdItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.shorty_id, self.return_type_idx, self.parameters_off = unpack_from('<3I', buf, pr.get_and_add(len(self)))
        if self.parameters_off:
            self.parameters = MapList.get_item_and_add(
                MapListItemType.TYPE_TYPE_LIST_ITEM,
                self.parameters_off, TypeListItem, buf
            )

        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.parameters_off = self.parameters.offset if self.parameters else 0
        buf.extend(pack('<3I', self.shorty_id, self.return_type_idx, self.parameters_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x0c


class FieldIdItem(Writeable):
    def __init__(self):
        super().__init__()
        self.class_id: int = -1
        self.type_id: int = -1
        self.name_id: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.class_id, self.type_id, self.name_id = unpack_from('<2HI', buf, pr.get_and_add(len(self)))
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(pack('<2HI', self.class_id, self.type_id, self.name_id))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x08


class MethodIdItem(Writeable):
    def __init__(self):
        super().__init__()
        self.class_id: int = -1
        self.proto_id: int = -1
        self.name_id: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.class_id, self.proto_id, self.name_id = unpack_from('<2HI', buf, pr.get_and_add(len(self)))
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(pack('<2HI', self.class_id, self.proto_id, self.name_id))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x08


class EncodedArray(Writeable):
    def __init__(self):
        super().__init__()
        self.size: int = -1
        self.values: List[EncodedValue] = []

    def parse(self, buf: bytes, pr: Pointer):
        self.size = Leb128.read_unsigned_leb128(buf, pr)
        for _ in range(self.size):
            self.values.append(EncodedValue().parse(buf, pr))
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        size = Leb128.read_unsigned_leb128(buf, pr)
        for _ in range(size):
            EncodedValue.ignore(buf, pr)

    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.size == len(self.values)
        buf.extend(Leb128.write_unsigned_leb128(self.size, pr))
        for v in self.values:
            v.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class EncodedValue(Writeable):
    def __init__(self):
        super().__init__()
        self.value_type: EncodedValueType = EncodedValueType.VALUE_NULL
        self.value_arg: int = -1
        self.data: Union[EncodedArray, EncodedAnnotation, int, None] = None

    def parse(self, buf: bytes, pr: Pointer):
        value = buf[pr.get_and_add(0x01)]
        self.value_type = EncodedValueType(value & 0x1f)
        self.value_arg = (value >> 5) & 0x07
        if self.value_type == EncodedValueType.VALUE_BYTE:
            self.data = buf[pr.get_and_add(0x01)]
        elif self.value_type in (EncodedValueType.VALUE_SHORT,
                                 EncodedValueType.VALUE_CHAR,
                                 EncodedValueType.VALUE_INT,
                                 EncodedValueType.VALUE_LONG,
                                 EncodedValueType.VALUE_FLOAT,
                                 EncodedValueType.VALUE_DOUBLE,
                                 EncodedValueType.VALUE_STRING,
                                 EncodedValueType.VALUE_TYPE,
                                 EncodedValueType.VALUE_FIELD,
                                 EncodedValueType.VALUE_METHOD,
                                 EncodedValueType.VALUE_ENUM):
            self.data = EncodedValue.read_encode_value_data(buf, pr, self.value_arg + 1)
        elif self.value_type == EncodedValueType.VALUE_ARRAY:
            self.data = EncodedArray().parse(buf, pr)
        elif self.value_type == EncodedValueType.VALUE_ANNOTATION:
            self.data = EncodedAnnotation().parse(buf, pr)
        elif self.value_type in (EncodedValueType.VALUE_NULL,
                                 EncodedValueType.VALUE_BOOLEAN):
            pass
        else:
            raise RuntimeWarning('Unknown type for encoded value ' + hex(self.value_type.val))
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        value = buf[pr.get_and_add(0x01)]
        value_type = EncodedValueType(value & 0x1f)
        value_arg = (value >> 5) & 0x07
        if value_type == EncodedValueType.VALUE_BYTE:
            pr.add(0x01)
        elif value_type in (EncodedValueType.VALUE_SHORT,
                            EncodedValueType.VALUE_CHAR,
                            EncodedValueType.VALUE_INT,
                            EncodedValueType.VALUE_LONG,
                            EncodedValueType.VALUE_FLOAT,
                            EncodedValueType.VALUE_DOUBLE,
                            EncodedValueType.VALUE_STRING,
                            EncodedValueType.VALUE_TYPE,
                            EncodedValueType.VALUE_FIELD,
                            EncodedValueType.VALUE_METHOD,
                            EncodedValueType.VALUE_ENUM):
            pr.add(value_arg + 0x01)
        elif value_type == EncodedValueType.VALUE_ARRAY:
            EncodedArray.ignore(buf, pr)
        elif value_type == EncodedValueType.VALUE_ANNOTATION:
            EncodedAnnotation.ignore(buf, pr)
        elif value_type in (EncodedValueType.VALUE_NULL,
                            EncodedValueType.VALUE_BOOLEAN):
            pass
        else:
            raise RuntimeWarning('Unknown type for encoded value ' + hex(value_type.val))

    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.append((self.value_type.value & 0x1f) | ((self.value_arg & 0x07) << 5))
        pr.add(0x01)
        if self.value_type == EncodedValueType.VALUE_BYTE:
            assert self.data and type(self.data) == int and self.data <= 0xff
            buf.append(self.data)
            pr.add(0x01)
        elif self.value_type in (EncodedValueType.VALUE_SHORT,
                                 EncodedValueType.VALUE_CHAR,
                                 EncodedValueType.VALUE_INT,
                                 EncodedValueType.VALUE_LONG,
                                 EncodedValueType.VALUE_FLOAT,
                                 EncodedValueType.VALUE_DOUBLE,
                                 EncodedValueType.VALUE_STRING,
                                 EncodedValueType.VALUE_TYPE,
                                 EncodedValueType.VALUE_FIELD,
                                 EncodedValueType.VALUE_METHOD,
                                 EncodedValueType.VALUE_ENUM):
            assert type(self.data) == int
            ret = EncodedValue.write_encode_value_data(self.data, self.value_arg + 1)
            buf.extend(ret)
            pr.add(len(ret))
        elif self.value_type == EncodedValueType.VALUE_ARRAY:
            assert self.data and type(self.data) == EncodedArray
            self.data.to_bytes(buf, pr)
        elif self.value_type == EncodedValueType.VALUE_ANNOTATION:
            assert self.data and type(self.data) == EncodedAnnotation
            self.data.to_bytes(buf, pr)
        elif self.value_type in (EncodedValueType.VALUE_NULL,
                                 EncodedValueType.VALUE_BOOLEAN):
            assert self.data is None
            pass
        else:
            raise RuntimeWarning('Unknown type for encoded value ' + hex(self.value_type.value))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    @staticmethod
    @Pointer.tmp_reset_step(step=1)
    def read_encode_value_data(buf: bytes, pr: Pointer, size: int) -> int:
        ret = 0
        shift = 0
        for _ in range(size):
            ret |= buf[pr.cur_and_increase] << shift
            shift += 8
        return ret & 0xffffffff_ffffffff

    @staticmethod
    def write_encode_value_data(data: int, size: int) -> bytearray:
        result = bytearray()
        for _ in range(size):
            result.append(data & 0xff)
            data >>= 8
        return result


class AnnotationElement(Writeable):
    def __init__(self):
        super().__init__()
        self.name_idx: int = -1
        self.value: Optional[EncodedValue] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.name_idx = Leb128.read_unsigned_leb128(buf, pr)
        self.value = EncodedValue().parse(buf, pr)
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        Leb128.pass_leb128(buf, pr)
        EncodedValue.ignore(buf, pr)

    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.value
        buf.extend(Leb128.write_unsigned_leb128(self.name_idx, pr))
        self.value.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class EncodedAnnotation(Writeable):
    def __init__(self):
        super().__init__()
        self.type_idx: int = -1
        self.size: int = -1
        self.elements: List[AnnotationElement] = []

    def parse(self, buf: bytes, pr: Pointer):
        self.type_idx = Leb128.read_unsigned_leb128(buf, pr)
        self.size = Leb128.read_unsigned_leb128(buf, pr)
        for _ in range(self.size):
            self.elements.append(AnnotationElement().parse(buf, pr))
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        Leb128.pass_leb128(buf, pr)
        size = Leb128.read_unsigned_leb128(buf, pr)
        for _ in range(size):
            AnnotationElement.ignore(buf, pr)

    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.size == len(self.elements)
        buf.extend(Leb128.write_unsigned_leb128(self.type_idx, pr))
        buf.extend(Leb128.write_unsigned_leb128(self.size, pr))
        for e in self.elements:
            e.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class AnnotationItem(Writeable):
    def __init__(self):
        super().__init__()
        # self.visibility: Visibility = Visibility.VISIBILITY_BUILD
        # self.annotation: Optional[EncodedAnnotation] = None
        self.buf: Optional[bytearray] = None

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        # visibility = unpack_from('<B', buf, pr.get_and_add(0x01))[0]
        # self.visibility = Visibility(visibility)
        # self.annotation = EncodedAnnotation().parse(buf, pr)
        # return self
        self.buf = Pointer.ignore_data(AnnotationItem, buf, pr)
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        pr.add(0x01)
        EncodedAnnotation.ignore(buf, pr)

    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        # assert self.annotation
        # buf.extend(pack('<I', self.visibility.value))
        # self.annotation.to_bytes(buf, pr)
        buf.extend(self.buf)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class AnnotationOff(Writeable):
    def __init__(self):
        super().__init__()
        self.annotation_off: int = -1

        self.annotation: Optional[AnnotationItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.annotation_off = unpack_from('<I', buf, pr.get_and_add(len(self)))[0]
        if self.annotation_off:
            self.annotation = MapList.get_item_and_add(
                MapListItemType.TYPE_ANNOTATION_ITEM,
                self.annotation_off, AnnotationItem, buf
            )
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.annotation_off = self.annotation.offset if self.annotation else 0
        buf.extend(pack('<I', self.annotation_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x04


class AnnotationSetItem(Writeable):
    def __init__(self):
        super().__init__()
        self.size: int = -1
        self.entries: List[AnnotationOff] = []

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.size = unpack_from('<I', buf, pr.get_and_add(0x04))[0]
        for _ in range(self.size):
            self.entries.append(AnnotationOff().parse(buf, pr))
        return self

    @Pointer.aligned_4_with_zero
    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.size == len(self.entries)
        buf.extend(pack('<I', self.size))
        pr.add(0x04)
        for e in self.entries:
            e.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class FieldAnnotation(Writeable):
    def __init__(self):
        super().__init__()
        self.field_idx: int = -1
        self.annotations_off: int = -1

        self.annotations: Optional[AnnotationSetItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.field_idx, self.annotations_off = unpack_from('<2I', buf, pr.get_and_add(len(self)))
        if self.annotations_off:
            self.annotations = MapList.get_item_and_add(
                MapListItemType.TYPE_ANNOTATION_SET_ITEM,
                self.annotations_off, AnnotationSetItem, buf
            )
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.annotations_off = self.annotations.offset if self.annotations else 0
        buf.extend(pack('<2I', self.field_idx, self.annotations_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x08


class MethodAnnotation(Writeable):
    def __init__(self):
        super().__init__()
        self.method_idx: int = -1
        self.annotations_off: int = -1

        self.annotations: Optional[AnnotationSetItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.method_idx, self.annotations_off = unpack_from('<2I', buf, pr.get_and_add(len(self)))
        if self.annotations_off:
            self.annotations = MapList.get_item_and_add(
                MapListItemType.TYPE_ANNOTATION_SET_ITEM,
                self.annotations_off, AnnotationSetItem, buf
            )
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.annotations_off = self.annotations.offset if self.annotations else 0
        buf.extend(pack('<2I', self.method_idx, self.annotations_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x08


class AnnotationSetRef(Writeable):
    def __init__(self):
        super().__init__()
        self.annotations_off: int = -1

        self.annotations: Optional[AnnotationSetItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.annotations_off = unpack_from('<I', buf, pr.get_and_add(len(self)))[0]
        if self.annotations_off:
            self.annotations = MapList.get_item_and_add(
                MapListItemType.TYPE_ANNOTATION_SET_ITEM,
                self.annotations_off, AnnotationSetItem, buf
            )
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.annotations_off = self.annotations.offset if self.annotations else 0
        buf.extend(pack('<I', self.annotations_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x04


class AnnotationSetRefListItem(Writeable):
    def __init__(self):
        super().__init__()
        self.size: int = -1
        self.list: List[AnnotationSetRef] = []

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.size = unpack_from('<I', buf, pr.get_and_add(0x04))[0]
        for _ in range(self.size):
            self.list.append(AnnotationSetRef().parse(buf, pr))
        return self

    @Pointer.aligned_4_with_zero
    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.size == len(self.list)
        buf.extend(pack('<I', self.size))
        pr.add(0x04)
        for e in self.list:
            e.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ParameterAnnotation(Writeable):
    def __init__(self):
        super().__init__()
        self.method_idx: int = -1
        self.annotations_off: int = -1

        self.annotations: Optional[AnnotationSetRefListItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.method_idx, self.annotations_off = unpack_from('<2I', buf, pr.get_and_add(len(self)))
        if self.annotations_off:
            self.annotations = MapList.get_item_and_add(
                MapListItemType.TYPE_ANNOTATION_SET_REF_LIST,
                self.annotations_off, AnnotationSetRefListItem, buf
            )
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.annotations_off = self.annotations.offset if self.annotations else 0
        buf.extend(pack('<2I', self.method_idx, self.annotations_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x08


class AnnotationsDirectoryItem(Writeable):
    def __init__(self):
        super().__init__()
        self.class_annotations_off: int = -1
        self.fields_size: int = -1
        self.methods_size: int = -1
        self.parameters_size: int = -1
        self.field_annotations: List[FieldAnnotation] = []
        self.method_annotations: List[MethodAnnotation] = []
        self.parameter_annotations: List[ParameterAnnotation] = []

        self.class_annotations: Optional[AnnotationSetItem] = None

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.class_annotations_off, \
        self.fields_size, \
        self.methods_size, \
        self.parameters_size = unpack_from('<4I', buf, pr.get_and_add(0x10))
        for _ in range(self.fields_size):
            self.field_annotations.append(FieldAnnotation().parse(buf, pr))
        for _ in range(self.methods_size):
            self.method_annotations.append(MethodAnnotation().parse(buf, pr))
        for _ in range(self.parameters_size):
            self.parameter_annotations.append(ParameterAnnotation().parse(buf, pr))
        if self.class_annotations_off:
            self.class_annotations = MapList.get_item_and_add(
                MapListItemType.TYPE_ANNOTATION_SET_ITEM,
                self.class_annotations_off, AnnotationSetItem, buf
            )
        return self

    @Pointer.aligned_4_with_zero
    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.fields_size == len(self.field_annotations)
        assert self.methods_size == len(self.method_annotations)
        assert self.parameters_size == len(self.parameter_annotations)
        self.class_annotations_off = self.class_annotations.offset if self.class_annotations else 0
        buf.extend(pack('<4I', self.class_annotations_off, self.fields_size, self.methods_size, self.parameters_size))
        pr.add(0x10)
        for f in self.field_annotations:
            f.to_bytes(buf, pr)
        for m in self.method_annotations:
            m.to_bytes(buf, pr)
        for p in self.parameter_annotations:
            p.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class TryBlock(Writeable):
    def __init__(self):
        super().__init__()
        self.start_addr: int = -1
        self.insns_count: int = -1
        self.handler_off: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.start_addr, \
        self.insns_count, \
        self.handler_off = unpack_from('<I2H', buf, pr.get_and_add(len(self)))
        return self

    @staticmethod
    def ignore(pr: Pointer):
        pr.add(0x08)

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(pack('<I2H', self.start_addr, self.insns_count, self.handler_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x08


class EncodedTypeAddrPair(Writeable):
    def __init__(self):
        super().__init__()
        self.type_idx: int = -1
        self.addr: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.type_idx = Leb128.read_unsigned_leb128(buf, pr)
        self.addr = Leb128.read_unsigned_leb128(buf, pr)
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        Leb128.pass_leb128(buf, pr, 2)

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        buf.extend(Leb128.write_unsigned_leb128(self.type_idx))
        buf.extend(Leb128.write_unsigned_leb128(self.addr))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class EncodedCatchHandler(Writeable):
    def __init__(self):
        super().__init__()
        self.size: int = -1
        self.handlers: List[EncodedTypeAddrPair] = []
        self.catch_all_addr: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.size = Leb128.read_signed_leb128(buf, pr)
        for _ in range(abs(self.size)):
            self.handlers.append(EncodedTypeAddrPair().parse(buf, pr))
        if self.size <= 0:
            self.catch_all_addr = Leb128.read_unsigned_leb128(buf, pr)
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        size = Leb128.read_signed_leb128(buf, pr)
        for _ in range(abs(size)):
            EncodedTypeAddrPair.ignore(buf, pr)
        if size <= 0:
            Leb128.pass_leb128(buf, pr)

    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert abs(self.size) == len(self.handlers)
        assert (self.size <= 0 and self.catch_all_addr != -1) or \
               (self.size > 0 and self.catch_all_addr == -1)
        buf.extend(Leb128.write_signed_leb128(self.size, pr))
        for h in self.handlers:
            h.to_bytes(buf, pr)
        if self.size <= 0:
            buf.extend(Leb128.write_unsigned_leb128(self.catch_all_addr, pr))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class EncodedCatchHandlerList(Writeable):
    def __init__(self):
        super().__init__()
        self.size: int = -1
        self.list: List[EncodedCatchHandler] = []

    def parse(self, buf: bytes, pr: Pointer):
        self.size = Leb128.read_unsigned_leb128(buf, pr)
        for _ in range(self.size):
            self.list.append(EncodedCatchHandler().parse(buf, pr))
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        size = Leb128.read_unsigned_leb128(buf, pr)
        for _ in range(size):
            EncodedCatchHandler.ignore(buf, pr)

    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.size == len(self.list)
        buf.extend(Leb128.write_unsigned_leb128(self.size, pr))
        for e in self.list:
            e.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class DebugOpcode(Writeable):
    def __init__(self):
        super().__init__()
        self.opcode: DebugInfoOpCodes = DebugInfoOpCodes.DBG_END_SEQUENCE
        self.addr_diff: int = -1
        self.line_diff: int = -1
        self.register_num: int = -1
        self.name_idx: int = -1
        self.type_idx: int = -1
        self.sig_idx: int = -1
        self.special_value = -1

    def parse(self, buf: bytes, pr: Pointer):
        opcode: int = buf[pr.get_and_add(0x01)]
        self.opcode = DebugInfoOpCodes(
            opcode if opcode < DebugInfoOpCodes.DBG_FIRST_SPECIAL.value else DebugInfoOpCodes.DBG_FIRST_SPECIAL)

        if self.opcode == DebugInfoOpCodes.DBG_FIRST_SPECIAL:
            self.special_value = opcode
        elif self.opcode in (DebugInfoOpCodes.DBG_END_SEQUENCE,
                             DebugInfoOpCodes.DBG_SET_PROLOGUE_END,
                             DebugInfoOpCodes.DBG_SET_EPILOGUE_BEGIN):
            pass
        elif self.opcode == DebugInfoOpCodes.DBG_ADVANCE_PC:
            self.addr_diff = Leb128.read_unsigned_leb128(buf, pr)
        elif self.opcode == DebugInfoOpCodes.DBG_ADVANCE_LINE:
            self.line_diff = Leb128.read_signed_leb128(buf, pr)
        elif self.opcode == DebugInfoOpCodes.DBG_START_LOCAL:
            self.register_num = Leb128.read_unsigned_leb128(buf, pr)
            self.name_idx = Leb128.read_unsigned_leb128p1(buf, pr)
            self.type_idx = Leb128.read_unsigned_leb128p1(buf, pr)
        elif self.opcode == DebugInfoOpCodes.DBG_START_LOCAL_EXTENDED:
            self.register_num = Leb128.read_unsigned_leb128(buf, pr)
            self.name_idx = Leb128.read_unsigned_leb128p1(buf, pr)
            self.type_idx = Leb128.read_unsigned_leb128p1(buf, pr)
            self.sig_idx = Leb128.read_unsigned_leb128p1(buf, pr)
        elif self.opcode in (DebugInfoOpCodes.DBG_END_LOCAL, DebugInfoOpCodes.DBG_RESTART_LOCAL):
            self.register_num = Leb128.read_unsigned_leb128(buf, pr)
        elif self.opcode == DebugInfoOpCodes.DBG_SET_FILE:
            self.name_idx = Leb128.read_unsigned_leb128p1(buf, pr)
        else:
            raise RuntimeWarning('error debug info type.')

        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        opcode: int = buf[pr.get_and_add(0x01)]
        if opcode in (DebugInfoOpCodes.DBG_END_SEQUENCE.value,
                      DebugInfoOpCodes.DBG_SET_PROLOGUE_END.value,
                      DebugInfoOpCodes.DBG_SET_EPILOGUE_BEGIN.value) \
                or opcode >= DebugInfoOpCodes.DBG_FIRST_SPECIAL.value:
            pass
        elif opcode in (DebugInfoOpCodes.DBG_ADVANCE_PC.value,
                        DebugInfoOpCodes.DBG_ADVANCE_LINE.value,
                        DebugInfoOpCodes.DBG_END_LOCAL.value,
                        DebugInfoOpCodes.DBG_RESTART_LOCAL.value,
                        DebugInfoOpCodes.DBG_SET_FILE.value):
            Leb128.pass_leb128(buf, pr)
        elif opcode == DebugInfoOpCodes.DBG_START_LOCAL.value:
            Leb128.pass_leb128(buf, pr, 3)
        elif opcode == DebugInfoOpCodes.DBG_START_LOCAL_EXTENDED.value:
            Leb128.pass_leb128(buf, pr, 4)
        else:
            raise RuntimeWarning('error debug info type.')

    def to_bytes(self, buf: bytearray, pr: Pointer):
        if self.opcode == DebugInfoOpCodes.DBG_FIRST_SPECIAL:
            buf.append(self.special_value)
            pr.add(0x01)
            return

        buf.append(self.opcode.value)
        pr.add(0x01)
        if self.opcode in (DebugInfoOpCodes.DBG_END_SEQUENCE,
                           DebugInfoOpCodes.DBG_SET_PROLOGUE_END,
                           DebugInfoOpCodes.DBG_SET_EPILOGUE_BEGIN):
            pass
        elif self.opcode == DebugInfoOpCodes.DBG_ADVANCE_PC:
            buf.extend(Leb128.write_unsigned_leb128(self.addr_diff, pr))
        elif self.opcode == DebugInfoOpCodes.DBG_ADVANCE_LINE:
            buf.extend(Leb128.write_signed_leb128(self.line_diff, pr))
        elif self.opcode == DebugInfoOpCodes.DBG_START_LOCAL:
            buf.extend(Leb128.write_unsigned_leb128(self.register_num, pr))
            buf.extend(Leb128.write_unsigned_leb128p1(self.name_idx, pr))
            buf.extend(Leb128.write_unsigned_leb128p1(self.type_idx, pr))
        elif self.opcode == DebugInfoOpCodes.DBG_START_LOCAL_EXTENDED:
            buf.extend(Leb128.write_unsigned_leb128(self.register_num, pr))
            buf.extend(Leb128.write_unsigned_leb128p1(self.name_idx, pr))
            buf.extend(Leb128.write_unsigned_leb128p1(self.type_idx, pr))
            buf.extend(Leb128.write_unsigned_leb128p1(self.sig_idx, pr))
        elif self.opcode in (DebugInfoOpCodes.DBG_END_LOCAL, DebugInfoOpCodes.DBG_RESTART_LOCAL):
            buf.extend(Leb128.write_unsigned_leb128(self.register_num, pr))
        elif self.opcode == DebugInfoOpCodes.DBG_SET_FILE:
            buf.extend(Leb128.write_unsigned_leb128(self.name_idx, pr))
        else:
            raise RuntimeWarning('error debug info type.')

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class DebugInfoItem(Writeable):
    def __init__(self):
        super().__init__()
        # self.line_start: int = -1
        # self.parameters_size: int = -1
        # self.parameter_names: List[int] = []
        # self.opcodes: List[DebugOpcode] = []
        # self.opcodes: List[bytearray] = []
        self.buf: Optional[bytearray] = None

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        # self.line_start = Leb128.read_unsigned_leb128(buf, pr)
        # self.parameters_size = Leb128.read_unsigned_leb128(buf, pr)
        # for _ in range(self.parameters_size):
        #     self.parameter_names.append(Leb128.read_unsigned_leb128p1(buf, pr))
        # while True:
        #     self.opcodes.append(DebugOpcode().parse(buf, pr))
        #     if self.opcodes[-1].opcode == DebugInfoOpCodes.DBG_END_SEQUENCE:
        #         break
        # return self
        self.buf = Pointer.ignore_data(DebugInfoItem, buf, pr)
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        Leb128.pass_leb128(buf, pr)
        parameters_size = Leb128.read_unsigned_leb128(buf, pr)
        Leb128.pass_leb128(buf, pr, parameters_size)
        while buf[pr.cur] != DebugInfoOpCodes.DBG_END_SEQUENCE.value:
            DebugOpcode.ignore(buf, pr)
        DebugOpcode.ignore(buf, pr)  # DebugInfoOpCodes.DBG_END_SEQUENCE

    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        # assert self.parameters_size == len(self.parameter_names)
        # buf.extend(Leb128.write_unsigned_leb128(self.line_start))
        # buf.extend(Leb128.write_unsigned_leb128(self.parameters_size))
        # for n in self.parameter_names:
        #     buf.extend(Leb128.write_unsigned_leb128p1(n))
        # for o in self.opcodes:
        #     o.to_bytes(buf, pr)
        buf.extend(self.buf)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class CodeItem(Writeable):
    def __init__(self):
        super().__init__()
        self.registers_size: int = -1
        self.ins_size: int = -1
        self.outs_size: int = -1
        self.tries_size: int = -1
        self.debug_info_off: int = -1
        self.insns_size: int = -1
        self.insns: bytes = b''
        # self.tries: List[TryBlock] = []
        # self.handlers: Optional[EncodedCatchHandlerList] = None
        self.try_buf: Optional[bytearray] = None

        self.debug_info: Optional[DebugInfoItem] = None

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.registers_size, \
        self.ins_size, \
        self.outs_size, \
        self.tries_size, \
        self.debug_info_off, \
        self.insns_size = unpack_from('<4H2I', buf, pr.get_and_add(0x10))
        self.insns = buf[pr.get_and_add(0x02 * self.insns_size):pr.cur]
        if self.tries_size > 0:
            buf_start = pr.cur
            pr.aligned(0x04, 0x02 * self.insns_size)
            for _ in range(self.tries_size):
                TryBlock.ignore(pr)  # used parameter: buf
            EncodedCatchHandlerList.ignore(buf, pr)
            buf_end = pr.cur
            self.try_buf = buf[buf_start:buf_end]

        if self.debug_info_off:
            self.debug_info = MapList.get_item_and_add(
                MapListItemType.TYPE_DEBUG_INFO_ITEM,
                self.debug_info_off, DebugInfoItem, buf
            )
        return self

    @Pointer.aligned_4_with_zero
    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.insns_size * 0x02 == len(self.insns)
        self.debug_info_off = self.debug_info.offset if self.debug_info else 0
        buf.extend(pack('<4H2I',
                        self.registers_size,
                        self.ins_size,
                        self.outs_size,
                        self.tries_size,
                        self.debug_info_off,
                        self.insns_size))
        buf.extend(self.insns)
        if self.tries_size > 0:
            # start = pr.cur
            # pr.aligned(0x04, self.insns_size * 0x02)
            # while start < pr.cur:
            #     buf.append(0x00)
            #     start += 1
            #
            # for t in self.tries:
            #     t.to_bytes(buf, pr)
            # self.handlers.to_bytes(buf, pr)
            buf.extend(self.try_buf)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class EncodedMethod(Writeable):
    def __init__(self):
        super().__init__()
        self.method_idx_diff: int = -1
        self.access_flags: int = 0
        self.code_off: int = -1

        self.method_idx: int = -1
        self.code: Optional[CodeItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.method_idx_diff = Leb128.read_unsigned_leb128(buf, pr)
        self.access_flags = Leb128.read_unsigned_leb128(buf, pr)
        self.code_off = Leb128.read_unsigned_leb128(buf, pr)
        if self.code_off:
            self.code = MapList.get_item_and_add(
                MapListItemType.TYPE_CODE_ITEM,
                self.code_off, CodeItem, buf
            )
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.method_idx_diff > 0
        self.code_off = self.code.offset if self.code else 0
        buf.extend(Leb128.write_unsigned_leb128(self.method_idx_diff))
        buf.extend(Leb128.write_unsigned_leb128(self.access_flags))
        buf.extend(Leb128.write_unsigned_leb128(self.code_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class EncodedField(Writeable):
    def __init__(self):
        super().__init__()
        self.field_idx_diff: int = -1
        self.access_flags: int = 0

        self.field_idx: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.field_idx_diff = Leb128.read_unsigned_leb128(buf, pr)
        self.access_flags = Leb128.read_unsigned_leb128(buf, pr)
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.field_idx_diff > 0
        buf.extend(Leb128.write_unsigned_leb128(self.field_idx_diff))
        buf.extend(Leb128.write_unsigned_leb128(self.access_flags))

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ClassDataItem(Writeable):
    def __init__(self):
        super().__init__()
        self.static_fields_size: int = -1
        self.instance_fields_size: int = -1
        self.direct_methods_size: int = -1
        self.virtual_methods_size: int = -1
        self.static_fields: List[EncodedField] = []
        self.instance_fields: List[EncodedField] = []
        self.direct_methods: List[EncodedMethod] = []
        self.virtual_methods: List[EncodedMethod] = []

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.static_fields_size = Leb128.read_unsigned_leb128(buf, pr)
        self.instance_fields_size = Leb128.read_unsigned_leb128(buf, pr)
        self.direct_methods_size = Leb128.read_unsigned_leb128(buf, pr)
        self.virtual_methods_size = Leb128.read_unsigned_leb128(buf, pr)

        for _ in range(self.static_fields_size):
            self.static_fields.append(EncodedField().parse(buf, pr))
        for _ in range(self.instance_fields_size):
            self.instance_fields.append(EncodedField().parse(buf, pr))
        for _ in range(self.direct_methods_size):
            self.direct_methods.append(EncodedMethod().parse(buf, pr))
        for _ in range(self.virtual_methods_size):
            self.virtual_methods.append(EncodedMethod().parse(buf, pr))

        self.update_idx()
        return self

    def update_idx(self):
        index = 0
        for f in self.static_fields:
            index += f.field_idx_diff
            f.field_idx = index

        index = 0
        for f in self.instance_fields:
            index += f.field_idx_diff
            f.field_idx = index

        index = 0
        for m in self.direct_methods:
            index += m.method_idx_diff
            m.method_idx = index

        index = 0
        for m in self.virtual_methods:
            index += m.method_idx_diff
            m.method_idx = index

    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        assert self.static_fields_size == len(self.static_fields)
        assert self.instance_fields_size == len(self.instance_fields)
        assert self.direct_methods_size == len(self.direct_methods)
        assert self.virtual_methods_size == len(self.virtual_methods)

        buf.extend(Leb128.write_unsigned_leb128(self.static_fields_size, pr))
        buf.extend(Leb128.write_unsigned_leb128(self.instance_fields_size, pr))
        buf.extend(Leb128.write_unsigned_leb128(self.direct_methods_size, pr))
        buf.extend(Leb128.write_unsigned_leb128(self.virtual_methods_size, pr))

        self.update_diff()
        for s in self.static_fields:
            s.to_bytes(buf, pr)
        for i in self.instance_fields:
            i.to_bytes(buf, pr)
        for d in self.direct_methods:
            d.to_bytes(buf, pr)
        for v in self.virtual_methods:
            v.to_bytes(buf, pr)

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def update_diff(self):
        index = 0
        for f in self.static_fields:
            f.field_idx_diff = f.field_idx - index
            index = f.field_idx

        index = 0
        for f in self.instance_fields:
            f.field_idx_diff = f.field_idx - index
            index = f.field_idx

        index = 0
        for m in self.direct_methods:
            m.method_idx_diff = m.method_idx - index
            index = m.method_idx

        index = 0
        for m in self.virtual_methods:
            m.method_idx_diff = m.method_idx - index
            index = m.method_idx


class EncodedArrayItem(Writeable):
    def __init__(self):
        super().__init__()
        # self.encoded_array: Optional[EncodedArray] = None
        self.buf: Optional[bytearray] = None

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        # self.encoded_array = EncodedArray().parse(buf, pr)
        # return self
        self.buf = Pointer.ignore_data(EncodedArrayItem, buf, pr)
        return self

    @staticmethod
    def ignore(buf: bytes, pr: Pointer):
        EncodedArray.ignore(buf, pr)

    @Pointer.update_offset
    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        # assert self.encoded_array
        # self.encoded_array.to_bytes(buf, pr)
        buf.extend(self.buf)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ClassDefItem(Writeable):
    def __init__(self):
        super().__init__()
        self.log = logging.getLogger(ClassDefItem.__name__)

        self.class_id: int = -1
        self.access_flag: int = 0
        self.superclass_idx: int = -1
        self.interfaces_off: int = -1
        self.source_file_idx: int = -1
        self.annotations_off: int = -1
        self.class_data_off: int = -1
        self.static_values_off: int = -1

        self.interfaces: Optional[TypeListItem] = None
        self.annotations: Optional[AnnotationsDirectoryItem] = None
        self.class_data: Optional[ClassDataItem] = None
        self.static_values: Optional[EncodedArrayItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        self.class_id, \
        self.access_flag, \
        self.superclass_idx, \
        self.interfaces_off, \
        self.source_file_idx, \
        self.annotations_off, \
        self.class_data_off, \
        self.static_values_off = unpack_from('<8I', buf, pr.get_and_add(len(self)))
        if self.interfaces_off:
            self.interfaces = MapList.get_item_and_add(
                MapListItemType.TYPE_TYPE_LIST_ITEM,
                self.interfaces_off, TypeListItem, buf
            )
        if self.annotations_off:
            self.annotations = MapList.get_item_and_add(
                MapListItemType.TYPE_ANNOTATION_DIRECTORY_ITEM,
                self.annotations_off, AnnotationsDirectoryItem, buf
            )
        if self.class_data_off:
            self.class_data = MapList.get_item_and_add(
                MapListItemType.TYPE_CLASS_DATA_ITEM,
                self.class_data_off, ClassDataItem, buf
            )
        if self.static_values_off:
            self.static_values = MapList.get_item_and_add(
                MapListItemType.TYPE_ENCODED_ARRAY_ITEM,
                self.static_values_off, EncodedArrayItem, buf
            )

        self.log.debug(r'parse class def: ' + str(
            MapList.instance().map[MapListItemType.TYPE_STRING_ID_ITEM].data.get_item(
                MapList.instance().map[MapListItemType.TYPE_TYPE_ID_ITEM].data.get_item(
                    self.class_id).descriptor_id
            ).data.data, encoding='ASCII'))
        return self

    @Pointer.update_pointer
    def to_bytes(self, buf: bytearray, pr: Pointer):
        self.interfaces_off = self.interfaces.offset if self.interfaces else 0
        self.annotations_off = self.annotations.offset if self.annotations else 0
        self.class_data_off = self.class_data.offset if self.class_data else 0
        self.static_values_off = self.static_values.offset if self.static_values else 0

        buf.extend(pack('<8I', self.class_id,
                        self.access_flag,
                        self.superclass_idx,
                        self.interfaces_off,
                        self.source_file_idx,
                        self.annotations_off,
                        self.class_data_off,
                        self.static_values_off))

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self) -> int:
        return 0x20


class MapList(Writeable):
    __instance = None

    @classmethod
    def instance(cls):
        # don't change code
        instance: MapList = cls.__instance
        return instance

    def __init__(self):
        super().__init__()
        self.size = -1
        self.map: Dict[MapListItemType, MapItem] = {}
        MapList.__instance = self

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.size = unpack_from('<I', buf, pr.get_and_add(0x04))[0]
        for _ in range(self.size):
            item = MapItem().parse(buf, pr)
            self.map[item.type] = item

        self.__init_data_all_pools()
        return self

    @Pointer.aligned_4_with_zero
    @Pointer.update_offset
    def to_bytes(self, buf: bytearray, pr: Pointer):
        # update map list item's offset before writing
        self.map[MapListItemType.TYPE_MAP_LIST].data_offset = self.offset
        buf.extend(pack('<I', self.size))
        pr.add(0x04)
        for item_type in sorted(self.map.keys()):
            self.map[item_type].to_bytes(buf, pr)

    @staticmethod
    def get_item_and_add(pool_type: MapListItemType, key: int, value_type: type, buf: bytes):
        data = MapList.instance().map[pool_type].data
        assert data is not None
        if key in data:
            return data[key]

        pr = Pointer(key)
        value = value_type().parse(buf, pr)
        data.add(value)
        return value

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __init_data_pool(self, map_item_type: MapListItemType, item_type: type):
        if map_item_type not in self.map:
            return
        map_item = self.map[map_item_type]
        map_item.data = Pool(map_item, item_type)

    def __init_data_all_pools(self):
        # no header and map list
        self.__init_data_pool(MapListItemType.TYPE_STRING_ID_ITEM, StringIdItem)
        self.__init_data_pool(MapListItemType.TYPE_TYPE_ID_ITEM, TypeIdItem)
        self.__init_data_pool(MapListItemType.TYPE_STRING_DATA_ITEM, StringDataItem)
        self.__init_data_pool(MapListItemType.TYPE_TYPE_LIST_ITEM, TypeListItem)
        self.__init_data_pool(MapListItemType.TYPE_PROTO_ID_ITEM, ProtoIdItem)
        self.__init_data_pool(MapListItemType.TYPE_FIELD_ID_ITEM, FieldIdItem)
        self.__init_data_pool(MapListItemType.TYPE_METHOD_ID_ITEM, MethodIdItem)
        self.__init_data_pool(MapListItemType.TYPE_CLASS_DEF_ITEM, ClassDefItem)

        self.__init_data_pool(MapListItemType.TYPE_CLASS_DATA_ITEM, ClassDataItem)
        self.__init_data_pool(MapListItemType.TYPE_ANNOTATION_SET_ITEM, AnnotationSetItem)
        self.__init_data_pool(MapListItemType.TYPE_ANNOTATION_ITEM, AnnotationItem)
        self.__init_data_pool(MapListItemType.TYPE_ANNOTATION_SET_REF_LIST, AnnotationSetRefListItem)
        self.__init_data_pool(MapListItemType.TYPE_CODE_ITEM, CodeItem)
        self.__init_data_pool(MapListItemType.TYPE_DEBUG_INFO_ITEM, DebugInfoItem)
        self.__init_data_pool(MapListItemType.TYPE_ANNOTATION_DIRECTORY_ITEM, AnnotationsDirectoryItem)
        self.__init_data_pool(MapListItemType.TYPE_ENCODED_ARRAY_ITEM, EncodedArrayItem)


class DexFile:
    def __init__(self):
        super().__init__()
        self.header: Optional[Header] = None
        self.map_list: Optional[MapList] = None
        self.log = logging.getLogger(DexFile.__name__)

    def parse(self, buf: bytes):
        self.header = Header().parse(buf, Pointer(0))
        self.log.debug(r'parse header')
        self.map_list = MapList().parse(buf, Pointer(self.header.map_off))
        self.log.debug(r'parse map list')

        self.__parse_pool(MapListItemType.TYPE_STRING_ID_ITEM, buf)
        self.log.debug(r'parse string id')
        self.__parse_pool(MapListItemType.TYPE_TYPE_ID_ITEM, buf)
        self.log.debug(r'parse type id')
        self.__parse_pool(MapListItemType.TYPE_PROTO_ID_ITEM, buf)
        self.log.debug(r'parse proto id')
        self.__parse_pool(MapListItemType.TYPE_FIELD_ID_ITEM, buf)
        self.log.debug(r'parse field id')
        self.__parse_pool(MapListItemType.TYPE_METHOD_ID_ITEM, buf)
        self.log.debug(r'parse method id')
        self.__parse_pool(MapListItemType.TYPE_CLASS_DEF_ITEM, buf)
        self.log.debug(r'parse class def')
        return self

    def __parse_pool(self, item_type: MapListItemType, buf: bytes):
        assert item_type in INDEX_POOL_TYPE
        map_item = self.map_list.map[item_type]
        map_item.data.parse(buf, Pointer(map_item.data_offset))

    @staticmethod
    def parse_file(path: str):
        with open(path, 'rb') as reader:
            buf = reader.read()
        return DexFile().parse(buf)

    def pool_to_bytes_if_exist(self, item_type: MapListItemType, buf: bytearray, pr: Pointer):
        if item_type in self.map_list.map:
            self.map_list.map[item_type].data.to_bytes(buf, pr)

    def to_bytes(self) -> bytearray:
        index_buf = bytearray()
        index_pr = Pointer(0)

        data_off = self.__compute_data_off()
        data_buf = bytearray()
        data_pr = Pointer(data_off)

        # update header's data_off
        self.header.data_off = data_off
        self.header.to_bytes(index_buf, index_pr)
        self.log.debug(r'write header')

        self.pool_to_bytes_if_exist(MapListItemType.TYPE_STRING_DATA_ITEM, data_buf, data_pr)
        self.log.debug(r'write string data')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_TYPE_LIST_ITEM, data_buf, data_pr)
        self.log.debug(r'write type list')

        self.pool_to_bytes_if_exist(MapListItemType.TYPE_ANNOTATION_ITEM, data_buf, data_pr)
        self.log.debug(r'write annotation')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_ANNOTATION_SET_ITEM, data_buf, data_pr)
        self.log.debug(r'write annotation set')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_ANNOTATION_SET_REF_LIST, data_buf, data_pr)
        self.log.debug(r'write annotation set ref list')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_ANNOTATION_DIRECTORY_ITEM, data_buf, data_pr)
        self.log.debug(r'write annotation directory')

        self.pool_to_bytes_if_exist(MapListItemType.TYPE_ENCODED_ARRAY_ITEM, data_buf, data_pr)
        self.log.debug(r'write encoded array')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_DEBUG_INFO_ITEM, data_buf, data_pr)
        self.log.debug(r'write debug info')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_CODE_ITEM, data_buf, data_pr)
        self.log.debug(r'write code')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_CLASS_DATA_ITEM, data_buf, data_pr)
        self.log.debug(r'write class data')

        self.pool_to_bytes_if_exist(MapListItemType.TYPE_STRING_ID_ITEM, index_buf, index_pr)
        self.log.debug(r'write string id')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_TYPE_ID_ITEM, index_buf, index_pr)
        self.log.debug(r'write type id')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_PROTO_ID_ITEM, index_buf, index_pr)
        self.log.debug(r'write proto id')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_FIELD_ID_ITEM, index_buf, index_pr)
        self.log.debug(r'write field id')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_METHOD_ID_ITEM, index_buf, index_pr)
        self.log.debug(r'write method id')
        self.pool_to_bytes_if_exist(MapListItemType.TYPE_CLASS_DEF_ITEM, index_buf, index_pr)
        self.log.debug(r'write class def')

        self.update_map_list()
        self.map_list.to_bytes(data_buf, data_pr)
        self.log.debug(r'write map list')
        dex_buf = index_buf + data_buf
        assert len(index_buf) == self.header.data_off

        self.update_header(dex_buf)
        self.log.debug(r'update header')
        self.update_signature(dex_buf)
        self.log.debug(r'update signature')
        self.update_checksum(dex_buf)
        self.log.debug(r'update checksum')
        assert len(dex_buf) == self.header.file_size
        return dex_buf

    def update_signature(self, dex_buf: bytearray):
        self.header.signature = hashlib.sha1(dex_buf[0x20:]).digest()
        dex_buf[0x0c:0x20] = self.header.signature

    def update_checksum(self, dex_buf: bytearray):
        self.header.checksum = zlib.adler32(dex_buf[0x0c:])
        dex_buf[0x08:0x0c] = pack('<I', self.header.checksum)

    def update_header(self, dex_buf: bytearray):
        self.header.file_size = len(dex_buf)
        self.header.header_size = len(self.header)
        self.header.map_off = self.map_list.offset

        # string id
        map_item = self.map_list.map[MapListItemType.TYPE_STRING_ID_ITEM]
        self.header.string_ids_off = map_item.data_offset
        self.header.string_ids_size = map_item.size

        # type id
        map_item = self.map_list.map[MapListItemType.TYPE_TYPE_ID_ITEM]
        self.header.type_ids_off = map_item.data_offset
        self.header.type_ids_size = map_item.size

        # proto id
        map_item = self.map_list.map[MapListItemType.TYPE_PROTO_ID_ITEM]
        self.header.proto_ids_off = map_item.data_offset
        self.header.proto_ids_size = map_item.size

        # field id
        map_item = self.map_list.map[MapListItemType.TYPE_FIELD_ID_ITEM]
        self.header.field_ids_off = map_item.data_offset
        self.header.field_ids_size = map_item.size

        # method id
        map_item = self.map_list.map[MapListItemType.TYPE_METHOD_ID_ITEM]
        self.header.method_ids_off = map_item.data_offset
        self.header.method_ids_size = map_item.size

        # class def
        map_item = self.map_list.map[MapListItemType.TYPE_CLASS_DEF_ITEM]
        self.header.class_defs_off = map_item.data_offset
        self.header.class_defs_size = map_item.size

        # header's data_off has update
        self.header.data_size = self.header.file_size - self.header.data_off

        dex_buf[0x20:0x70] = pack('<20I',
                                  self.header.file_size,
                                  self.header.header_size,
                                  self.header.endian_tag,
                                  self.header.link_size,
                                  self.header.link_off,
                                  self.header.map_off,
                                  self.header.string_ids_size,
                                  self.header.string_ids_off,
                                  self.header.type_ids_size,
                                  self.header.type_ids_off,
                                  self.header.proto_ids_size,
                                  self.header.proto_ids_off,
                                  self.header.field_ids_size,
                                  self.header.field_ids_off,
                                  self.header.method_ids_size,
                                  self.header.method_ids_off,
                                  self.header.class_defs_size,
                                  self.header.class_defs_off,
                                  self.header.data_size,
                                  self.header.data_off)

    def update_map_list(self):
        self.map_list.size = len(self.map_list.map)
        # header
        item = self.map_list.map[MapListItemType.TYPE_HEADER_ITEM]
        item.offset = 0
        item.size = 1

        # map list
        item = self.map_list.map[MapListItemType.TYPE_MAP_LIST]
        item.offset = self.map_list.offset
        item.size = self.map_list.size

        # other
        for item_type, map_item in self.map_list.map.items():
            if item_type in (MapListItemType.TYPE_HEADER_ITEM,
                             MapListItemType.TYPE_MAP_LIST):
                continue
            assert map_item.data
            map_item.size = len(map_item.data)
            map_item.data_offset = map_item.data.offset

    def __compute_data_off(self):
        return 0x70 * 1 + \
               0x04 * len(self.map_list.map[MapListItemType.TYPE_STRING_ID_ITEM].data) + \
               0x04 * len(self.map_list.map[MapListItemType.TYPE_TYPE_ID_ITEM].data) + \
               0x0c * len(self.map_list.map[MapListItemType.TYPE_PROTO_ID_ITEM].data) + \
               0x08 * len(self.map_list.map[MapListItemType.TYPE_FIELD_ID_ITEM].data) + \
               0x08 * len(self.map_list.map[MapListItemType.TYPE_METHOD_ID_ITEM].data) + \
               0x20 * len(self.map_list.map[MapListItemType.TYPE_CLASS_DEF_ITEM].data)

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def get_string_by_idx(self, idx: int) -> str:
        string_id_pool = self.map_list.map[MapListItemType.TYPE_STRING_ID_ITEM].data
        return str(string_id_pool[idx].data.data, encoding='ASCII')

    def get_type_name_by_idx(self, idx: int) -> str:
        type_id_pool = self.map_list.map[MapListItemType.TYPE_TYPE_ID_ITEM].data
        return self.get_string_by_idx(type_id_pool[idx].descriptor_id)


if __name__ == '__main__':
    # TEST
    TEST_DEX_PATH = r'/Users/chenzelun/PycharmProjects/dll_vm/test/TestApp/app/release/app-release/classes.dex'

    NEW_DEX_PATH = os.path.join(env.TEST_ROOT, 'new_dex')
    NEW_DEX_PATH_1 = os.path.join(NEW_DEX_PATH, r'dex1.dex')
    NEW_DEX_PATH_2 = os.path.join(NEW_DEX_PATH, r'dex2.dex')

    main_dex = DexFile.parse_file(TEST_DEX_PATH)
    main_buf = main_dex.to_bytes()

    with open(NEW_DEX_PATH_1, 'wb') as writer:
        writer.write(main_buf)

    main_dex1 = DexFile().parse(main_buf)
    main_buf1 = main_dex1.to_bytes()
    with open(NEW_DEX_PATH_2, 'wb') as writer:
        writer.write(main_buf1)

    assert main_buf == main_buf1
