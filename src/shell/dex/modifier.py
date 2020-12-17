import logging
from typing import List

from res.key_functions import define_reader
from res.key_functions.define_reader import KeyFunctionDefined
from shell.dex import dex_file
from shell.dex.dex_file import DexFile


class DexFileModifier:
    def __init__(self, dex: DexFile):
        self.__dex = dex
        self.__key_methods: List[dex_file.EncodedMethod] = []
        self.log = logging.getLogger(DexFileModifier.__name__)

    @property
    def dex(self):
        return self.__dex

    @property
    def key_methods(self):
        return self.__key_methods

    @staticmethod
    def parse_dex_file(dex_path: str):
        return DexFileModifier(DexFile.parse_file(dex_path))

    def native_key_func(self):
        for em in self.key_methods:
            # set bit on: AccessFlag.ACC_NATIVE
            em.access_flags |= dex_file.AccessFlag.ACC_NATIVE.value
            # del this code in dex file
            assert em.code is not None
            self.dex.map_list.map[dex_file.MapListItemType.TYPE_CODE_ITEM] \
                .data.del_key(em.code.offset)
            # delete code from encoded method
            em.code = None
            em.code_off = 0

    def filter_key_func_encoded_method(self, key_function_defined_path: str):
        self.key_methods.clear()
        keys = KeyFunctionDefined(key_function_defined_path)

        for class_def in self.dex.map_list.map[dex_file.MapListItemType.TYPE_CLASS_DEF_ITEM].data:
            if not class_def.class_data:
                continue

            name = DexFile.wrap_to_class_name(
                self.dex.get_type_name_by_idx(class_def.class_id))
            if name.find(r'.R$') != -1 or \
                    name.endswith(r'.BuildConfig') or \
                    name.endswith(r'.R'):
                continue

            cur_ret: List[dex_file.EncodedMethod] = []
            need_to_del = []
            for c in keys.clazz:
                if not name.startswith(c):
                    continue

                # self.key_methods.extend(class_def.class_data.virtual_methods)
                # self.key_methods.extend(class_def.class_data.direct_methods)
                for m in class_def.class_data.virtual_methods:
                    if m.code:
                        cur_ret.append(m)
                for m in class_def.class_data.direct_methods:
                    if m.code:
                        cur_ret.append(m)

                if name == c:
                    need_to_del.append(name)
                break

            if cur_ret:
                # add all methods by class name.
                self.key_methods.extend(self.__filter_java_init_func(cur_ret))
                for d in need_to_del:
                    keys.clazz.remove(d)
                continue

            for m in keys.method:
                if not name.startswith(m.clazz):
                    continue
                cur_ret = self.__key_func_filter_encoded_method(
                    class_def.class_data.virtual_methods, m)
                if cur_ret:
                    if m.sign:
                        # only one, needn't find direct_methods
                        # add a method by method sign
                        need_to_del.append(m)
                        self.key_methods.extend(self.__filter_java_init_func(cur_ret))
                        continue

                cur_ret.extend(self.__key_func_filter_encoded_method(
                    class_def.class_data.direct_methods, m))
                if cur_ret:
                    self.key_methods.extend(self.__filter_java_init_func(cur_ret))
                    need_to_del.append(m)

            for d in need_to_del:
                keys.method.remove(d)

            if not keys.method and not keys.clazz:
                break
        if keys.method:
            for m in keys.method:
                if m.sign:
                    self.log.error('err-method: {}#{}:{}'.format(
                        m.clazz, m.method, m.sign))
                else:
                    self.log.error('err-method: {}#{}'.format(
                        m.clazz, m.method))
            raise RuntimeWarning("can't find all key method.")

    def __key_func_filter_encoded_method(
            self, encoded_methods: List[dex_file.EncodedMethod],
            comp: define_reader.Method) -> List[dex_file.EncodedMethod]:
        ret: List[dex_file.EncodedMethod] = []
        for em in encoded_methods:
            # continue means failure.
            if not em.code:
                continue

            em_name = self.dex.get_method_name(em.method_idx)
            if em_name != comp.method:
                continue
            if comp.sign:
                em_sign = self.dex.get_method_sign(em.method_idx)
                if em_sign == comp.sign:
                    ret.append(em)
                    break
            else:
                ret.append(em)

        return ret

    def __filter_java_init_func(self, func: List[dex_file.EncodedMethod]) \
            -> List[dex_file.EncodedMethod]:
        ret: List[dex_file.EncodedMethod] = []
        for em in func:
            func_name = self.dex.get_method_name(em.method_idx)
            if func_name not in {r'<clinit>', r'<init>'}:
                ret.append(em)
        return ret

    def get_class_names_by_super_class_name(self, super_name: str) -> List[str]:
        ret = []
        super_name = DexFile.wrap_to_class_name_sign(super_name)
        class_def_pool = self.dex.map_list.map[
            dex_file.MapListItemType.TYPE_CLASS_DEF_ITEM].data
        cur_idx = 0
        dst_idx = 0
        for cur_idx, class_def in enumerate(class_def_pool):
            if class_def.superclass_idx != dex_file.NO_INDEX and \
                    super_name == self.dex.get_type_name_by_idx(class_def.superclass_idx):
                dst_idx = class_def.superclass_idx
                ret.append(self.dex.get_type_name_by_idx(class_def.class_id)[1:-1].replace('/', '.'))
                break

        for cur_idx in range(cur_idx + 1, len(class_def_pool)):
            if cur_idx == dst_idx:
                ret.append(self.dex.get_type_name_by_idx(
                    class_def_pool.get_item(cur_idx).class_id)[1:-1].replace('/', '.'))

        return ret
