from abc import ABCMeta, abstractmethod
from enum import IntEnum, unique
from functools import wraps
from struct import unpack_from
from typing import Optional, List, Dict

from src.shell.utils import Debugger


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
    TYPE_ANNOTATIONS_DIRECTORY_ITEM = 0x2006


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


class ReflectHelper:
    @staticmethod
    def get_first_var_by_type(t: type, *args, **kwargs):
        for a in args:
            if type(a) == t:
                dest = a
                break
        else:
            for k in kwargs:
                if type(kwargs[k]) == t:
                    dest = k
                    break
            else:
                raise RuntimeWarning("Can't find the var which type is " + t.__name__)
        return dest

    @staticmethod
    def get_var_by_index(idx: int, *args, **kwargs):
        if idx < len(args):
            return args[idx]
        idx -= len(args)
        if idx < len(kwargs):
            return kwargs[list(kwargs.keys())[idx]]
        else:
            raise RuntimeWarning("Can't find the var which index is " + str(idx))


class Pointer:
    def __init__(self, start: int, step: int = 1):
        assert start >= 0
        assert step >= 0
        self.cur = start
        self.step = step

    def __repr__(self):
        return str(self.__dict__)

    @staticmethod
    def tmp_reset_step(step: int):
        def func_wrapper(func):
            @wraps(func)
            def wrapper(*args, **kwargs):
                pr: Pointer = ReflectHelper.get_first_var_by_type(Pointer, *args, **kwargs)
                old_step = pr.step
                pr.step = step
                ret = func(*args, **kwargs)
                pr.step = old_step
                return ret

            return wrapper

        return func_wrapper

    @staticmethod
    def update_offset(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            pr: Pointer = ReflectHelper.get_first_var_by_type(Pointer, *args, **kwargs)
            obj: Writeable = ReflectHelper.get_var_by_index(0, *args, **kwargs)
            obj.offset = pr.cur
            return func(*args, **kwargs)

        return wrapper

    @property
    def cur_and_increase(self) -> int:
        old = self.cur
        self.cur += self.step
        return old

    def get_and_add(self, offset: int) -> int:
        assert offset >= 0
        old_val = self.cur
        self.cur += offset
        return old_val

    def add(self, offset: int) -> int:
        assert offset >= 0
        self.cur += offset
        return self.cur

    @staticmethod
    def ignore_data(value_type, buf: bytes, pr) -> bytes:
        start = pr.cur
        value_type.ignore(buf, pr)
        end = pr.cur
        return buf[start:end]

    def aligned(self, aligned_len: int, now_len: int = -1):
        """
        change pr:Pointer by aligned_len
        :param aligned_len: length of aligned, only the power of 2 and the min value is 2
        :param now_len: dest length (default: pr.cur)
        :return: None
        """
        assert aligned_len >= 2 and (aligned_len & (aligned_len - 1) == 0)
        if now_len == -1:
            now_len = self.cur
        if now_len & (aligned_len - 1) != 0:
            offset = aligned_len - (now_len & (aligned_len - 1))
        else:
            offset = 0
        self.add(offset)


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
    def write_unsigned_leb128(data: int) -> bytearray:
        result: bytearray = bytearray()
        while True:
            out: int = data & 0x7f
            if out != data:
                result.append(out | 0x80)
                data >>= 7
            else:
                result.append(out)
                break
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
    def write_signed_leb128(data: int) -> bytearray:
        result: bytearray = bytearray()
        while True:
            out: int = data & 0x7f
            data >>= 7
            if data == 0x00 or (data == ~0x00 and out > 0x3f):
                result.append(out)
                break
            else:
                result.append(out | 0x80)
        return result

    @staticmethod
    def read_unsigned_leb128p1(buf: bytes, pr: Pointer) -> int:
        return Leb128.read_unsigned_leb128(buf, pr) - 1

    @staticmethod
    def write_unsigned_leb128p1(data: int) -> bytearray:
        return Leb128.write_unsigned_leb128(data + 1)


class Writeable(metaclass=ABCMeta):
    def __init__(self):
        self.offset = -1

    @abstractmethod
    def parse(self, buf: bytes, pr: Pointer):
        """ Convert binary to data structure. """
        pass

    @abstractmethod
    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
        return 0x0c


class Pool(Writeable):
    index_pool_type = (
        MapListItemType.TYPE_STRING_ID_ITEM,
        MapListItemType.TYPE_TYPE_ID_ITEM,
        MapListItemType.TYPE_PROTO_ID_ITEM,
        MapListItemType.TYPE_FIELD_ID_ITEM,
        MapListItemType.TYPE_METHOD_ID_ITEM,
        MapListItemType.TYPE_CLASS_DEF_ITEM,
    )
    offset_pool_type = (
        MapListItemType.TYPE_TYPE_LIST_ITEM,
        MapListItemType.TYPE_ANNOTATION_SET_REF_LIST,
        MapListItemType.TYPE_ANNOTATION_SET_ITEM,
        MapListItemType.TYPE_CLASS_DATA_ITEM,
        MapListItemType.TYPE_CODE_ITEM,
        MapListItemType.TYPE_STRING_DATA_ITEM,
        MapListItemType.TYPE_DEBUG_INFO_ITEM,
        MapListItemType.TYPE_ANNOTATION_ITEM,
        MapListItemType.TYPE_ENCODED_ARRAY_ITEM,
        MapListItemType.TYPE_ANNOTATIONS_DIRECTORY_ITEM,
    )

    def __init__(self, map_item: MapItem, val_type: type):
        super().__init__()
        self.__order: List[int] = []
        self.__map: Dict[int, Writeable] = {}
        self.__map_item = map_item
        self.__val_type = val_type

    def parse(self, buf: bytes, pr: Pointer):
        for idx in range(self.__map_item.size):
            item: Writeable = self.__val_type().parse(buf, pr)
            key = idx if self.__map_item.type in self.index_pool_type else item.offset
            self.__map[key] = item
            self.__order.append(key)
        return self

    def add(self, item: Writeable):
        key = len(self.__order) if self.__map_item.type in self.index_pool_type else item.offset
        self.__map[key] = item
        self.__order.append(key)

    def __getitem__(self, key: int):
        return self.__map[key]

    def __contains__(self, key: int):
        return key in self.__map

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
        return 0x70


class StringDataItem(Writeable):
    def __init__(self):
        super().__init__()
        self.size = -1
        self.data = b''

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.size = Leb128.read_unsigned_leb128(buf, pr)
        assert self.size
        start = pr.cur
        while buf[pr.get_and_add(1)]:
            pass
        self.data = buf[start:pr.cur]
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
        self.data = MapList.get_and_add(
            MapListItemType.TYPE_STRING_DATA_ITEM,
            self.data_offset, StringDataItem, buf
        )
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
        return 0x04


class TypeIdItem(Writeable):
    def __init__(self):
        super().__init__()
        self.descriptor_id: int = -1

    def parse(self, buf: bytes, pr: Pointer):
        self.descriptor_id = unpack_from('<I', buf, pr.get_and_add(len(self)))[0]
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
            self.parameters = MapList.get_and_add(
                MapListItemType.TYPE_TYPE_LIST_ITEM,
                self.parameters_off, TypeListItem, buf
            )

        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class EncodedValue(Writeable):
    def __init__(self):
        super().__init__()
        self.value_type: EncodedValueType = EncodedValueType.VALUE_NULL
        self.value_arg: int = -1
        self.data: Optional[EncodedArray, EncodedAnnotation, int] = None

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
            raise RuntimeWarning('Unknown type for encoded value ' + hex(self.value_type.value))
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
            raise RuntimeWarning('Unknown type for encoded value ' + hex(value_type.value))

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
    def write_encode_value_data(buf: bytearray, data: int, size: int):
        for _ in range(size):
            buf.append(data & 0xff)
            data >>= 8


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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class AnnotationOff(Writeable):
    def __init__(self):
        super().__init__()
        self.annotation_off: int = -1

        self.annotation: Optional[AnnotationItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class MethodAnnotation(Writeable):
    def __init__(self):
        super().__init__()
        self.method_idx: int = -1
        self.annotations_off: int = -1

        self.annotations: Optional[AnnotationSetItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class AnnotationSetRef(Writeable):
    def __init__(self):
        super().__init__()
        self.annotations_off: int = -1

        self.annotations: Optional[AnnotationSetItem] = None

    def parse(self, buf: bytes, pr: Pointer):
        pass

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class AnnotationSetRefListItem(Writeable):
    def __init__(self):
        super().__init__()
        self.size: int = -1
        self.list: List[AnnotationSetRef] = []

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        pass

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
        pass

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


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
        pass

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class TryBlock(Writeable):
    def __init__(self):
        super().__init__()
        self.start_addr: int = -1
        self.insns_count: int = -1
        self.handler_off: int = -1

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.start_addr, \
        self.insns_count, \
        self.handler_off = unpack_from('<I2H', buf, pr.get_and_add(len(self)))
        return self

    @staticmethod
    def ignore(pr: Pointer):
        pr.add(0x08)

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
        if self.insns_size > 0:
            buf_start = pr.cur
            pr.aligned(0x04, 0x02 * self.insns_size)
            for _ in range(self.tries_size):
                TryBlock.ignore(pr)  # used parameter: buf
            EncodedCatchHandlerList.ignore(buf, pr)
            buf_end = pr.cur
            self.try_buf = buf[buf_start:buf_end]

        if self.debug_info_off:
            self.debug_info = MapList.get_and_add(
                MapListItemType.TYPE_DEBUG_INFO_ITEM,
                self.debug_info_off, DebugInfoItem, buf
            )
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
            self.code = MapList.get_and_add(
                MapListItemType.TYPE_CODE_ITEM,
                self.code_off, CodeItem, buf
            )
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

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
        for f in self.direct_methods:
            index += f.method_idx_diff
            f.field_idx = index

        index = 0
        for f in self.virtual_methods:
            index += f.method_idx_diff
            f.field_idx = index

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


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

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ClassDefItem(Writeable):
    def __init__(self):
        super().__init__()
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

    @Pointer.update_offset
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
            self.interfaces = MapList.get_and_add(
                MapListItemType.TYPE_TYPE_LIST_ITEM,
                self.interfaces_off, TypeListItem, buf
            )
        if self.annotations_off:
            self.annotations = MapList.get_and_add(
                MapListItemType.TYPE_ANNOTATIONS_DIRECTORY_ITEM,
                self.annotations_off, AnnotationsDirectoryItem, buf
            )
        if self.class_data_off:
            self.class_data = MapList.get_and_add(
                MapListItemType.TYPE_CLASS_DATA_ITEM,
                self.class_data_off, ClassDataItem, buf
            )
        if self.static_values_off:
            self.static_values = MapList.get_and_add(
                MapListItemType.TYPE_ENCODED_ARRAY_ITEM,
                self.static_values_off, EncodedArrayItem, buf
            )

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    def __len__(self):
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

    @Pointer.update_offset
    def parse(self, buf: bytes, pr: Pointer):
        self.size = unpack_from('<I', buf, pr.get_and_add(0x04))[0]
        for _ in range(self.size):
            item = MapItem().parse(buf, pr)
            self.map[item.type] = item

        self.__init_data_all_pools()
        return self

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @staticmethod
    def get_and_add(pool_type: MapListItemType, key: int, value_type: type, buf: bytes):
        data = MapList.instance().map[pool_type].data
        assert data
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
        self.__init_data_pool(MapListItemType.TYPE_ANNOTATIONS_DIRECTORY_ITEM, AnnotationsDirectoryItem)
        self.__init_data_pool(MapListItemType.TYPE_ENCODED_ARRAY_ITEM, EncodedArrayItem)


class DexFile:
    def __init__(self):
        super().__init__()
        self.header: Optional[Header] = None
        self.map_list: Optional[MapList] = None

    def parse(self, buf: bytes):
        self.header = Header().parse(buf, Pointer(0))
        self.map_list = MapList().parse(buf, Pointer(self.header.map_off))

        self.__parse_pool(MapListItemType.TYPE_STRING_ID_ITEM, buf)
        self.__parse_pool(MapListItemType.TYPE_TYPE_ID_ITEM, buf)
        self.__parse_pool(MapListItemType.TYPE_PROTO_ID_ITEM, buf)
        self.__parse_pool(MapListItemType.TYPE_FIELD_ID_ITEM, buf)
        self.__parse_pool(MapListItemType.TYPE_METHOD_ID_ITEM, buf)
        self.__parse_pool(MapListItemType.TYPE_CLASS_DEF_ITEM, buf)
        return self

    def __parse_pool(self, item_type: MapListItemType, buf: bytes):
        assert item_type in Pool.index_pool_type
        map_item = self.map_list.map[item_type]
        map_item.data.parse(buf, Pointer(map_item.data_offset))

    @staticmethod
    def parse_file(path: str):
        with open(path, 'rb') as reader:
            buf = reader.read()
        return DexFile().parse(buf)

    def to_bytes(self, buf: bytearray, pr: Pointer) -> bytearray:
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass
