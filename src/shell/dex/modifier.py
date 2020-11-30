import logging
import os
from typing import List

import env
from res.key_functions import define_reader
from res.key_functions.define_reader import KeyFunctionDefined
from shell.data.code import VmKeyFuncCodeFile
from shell.data.jni import VmKeyFuncJniFile
from shell.data.vm_data import VmDataFile
from shell.dex import dex_file
from shell.dex.dex_file import DexFile


class DexFileModifier:
    def __init__(self, dex: DexFile, vm_data: VmDataFile):
        self.dex = dex
        self.log = logging.getLogger(DexFileModifier.__name__)
        self.vm_data = vm_data

    @staticmethod
    def parse_dex_file(dex_path: str, vm_data: VmDataFile = None):
        return DexFileModifier(DexFile.parse_file(dex_path), vm_data)

    def native_key_func(self, key_function_defined_path: str, out_path: str):
        key_func = self.__key_func_encoded_method(key_function_defined_path)
        code_file = VmKeyFuncCodeFile()
        jni_file = VmKeyFuncJniFile(self.dex)
        for em in key_func:
            # save code
            code_file.add_method(em)
            # set bit on: AccessFlag.ACC_NATIVE
            em.access_flags |= dex_file.AccessFlag.ACC_NATIVE.value
            # del this code in dex file
            assert em.code
            self.dex.map_list.map[dex_file.MapListItemType.TYPE_CODE_ITEM] \
                .data.del_key(em.code.offset)
            # delete code from encoded method
            em.code = None
            em.code_off = 0
            # create jni method
            jni_file.add_method(em)

        self.vm_data.add_file(env.VM_DATA_KEY_FUNC_CODE_FILE_NAME, code_file.to_bytes())
        with open(os.path.join(out_path, env.KEY_FUNC_JNI_H_NAME), 'wb') as writer:
            writer.write(jni_file.to_bytes_h())
        with open(os.path.join(out_path, env.KEY_FUNC_JNI_CPP_NAME), 'wb') as writer:
            writer.write(jni_file.to_bytes_cpp())

    def __key_func_encoded_method(self, key_function_defined_path: str) \
            -> List[dex_file.EncodedMethod]:
        ret: List[dex_file.EncodedMethod] = []
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

                # ret.extend(class_def.class_data.virtual_methods)
                # ret.extend(class_def.class_data.direct_methods)
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
                ret.extend(self.filter_java_init_func(cur_ret))
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
                        ret.extend(self.filter_java_init_func(cur_ret))
                        continue

                cur_ret.extend(self.__key_func_filter_encoded_method(
                    class_def.class_data.direct_methods, m))
                if cur_ret:
                    ret.extend(self.filter_java_init_func(cur_ret))
                    need_to_del.append(m)

            for d in need_to_del:
                keys.method.remove(d)

            if not keys.method and not keys.clazz:
                break

        assert not keys.method
        return ret

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

    def filter_java_init_func(self, func: List[dex_file.EncodedMethod]) \
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
