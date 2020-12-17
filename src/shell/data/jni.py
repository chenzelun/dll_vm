import logging
from datetime import date
from typing import List, Union

import env
from shell.common.file_builder import FileBuilder
from shell.common.utils import Debugger, Log
from shell.dex import dex_file
from shell.dex.dex_file import DexFile


class VmKeyFuncJniFile:
    def __init__(self, dex: DexFile):
        self.__all_methods: List[dex_file.EncodedMethod] = []
        self.__dex = dex
        self.log = logging.getLogger(VmKeyFuncJniFile.__name__)

    def append(self, method: dex_file.EncodedMethod):
        self.__all_methods.append(method)

    def extends(self, methods: List[dex_file.EncodedMethod]):
        for m in methods:
            self.append(m)

    @Log.log_function
    def to_bytes_h(self) -> bytes:
        file = FileBuilder()
        file.add_line()
        today = str(date.today()).replace('-', r'/')
        file.add_line(r'//')
        file.add_line(r'// Created by 陈泽伦 on {today_date}'.format(today_date=today))
        file.add_line(r'//')
        file.add_line()
        file.add_line(r'#ifndef VM_KEYFUNCJNI_H')
        file.add_line(r'#define VM_KEYFUNCJNI_H')
        file.add_line()
        file.add_line(r'#include <jni.h>')
        file.add_line()

        for m in self.__all_methods:
            file.add_line()
            self.__add_func_header(m, file)
            file.append(';')
            file.add_line()

        file.add_line(r'#endif //VM_KEYFUNCJNI_H')
        file.add_line()
        return bytes(file.build(), encoding='utf-8')

    @Log.log_function
    def to_bytes_cpp(self) -> bytes:
        file = FileBuilder()
        file.add_line()
        today = str(date.today()).replace('-', r'/')
        file.add_line(r'//')
        file.add_line(r'// Created by 陈泽伦 on {today_date}'.format(today_date=today))
        file.add_line(r'//')
        file.add_line()
        file.add_line(r'#include "{header}"'.format(header=env.KEY_FUNC_JNI_H_NAME))
        file.add_line(r'#include "common/Util.h"')
        file.add_line(r'#include "vm/Vm.h"')
        file.add_line()

        for m in self.__all_methods:
            file.add_line()
            self.__add_func(m, file)
            file.add_line()

        return bytes(file.build(), encoding='utf-8')

    @Debugger.print_all_fields
    def __repr__(self):
        pass

    @Log.log_function
    def __add_func(self, method: dex_file.EncodedMethod, file: FileBuilder):
        self.__add_func_header(method, file)
        file.append(' {')
        file.new_block()

        # function body
        file.add_line()
        file.add_line(r'LOG_D("jni function start.");')
        if method.access_flags & dex_file.AccessFlag.ACC_STATIC:
            file.add_line(r'jobject instance = clazz_obj;')
        else:
            file.add_line(r'jclass clazz_obj = (*env).GetObjectClass(instance);')

        method_name = self.__dex.get_method_name(method.method_idx)
        method_sign = self.__dex.get_method_sign(method.method_idx)
        param_call = ''
        for no in range(len(self.__dex.get_method_param_short_names(method.method_idx))):
            param_call += r', param_{no}'.format(no=no)

        file.add_line(r'jmethodID method_id = (*env).GetMethodID(clazz_obj, '
                      r'"{name}", "{sign}");'.format(name=method_name, sign=method_sign))
        file.add_line(r'jvalue retValue;')
        file.add_line(r'Vm::callMethod(instance, method_id, &retValue'
                      r'{param});'.format(param=param_call))
        file.add_line(r'LOG_D("jni function finish.");')
        ret_jvalue_type = VmKeyFuncJniFile.wrap_to_jvalue_type(self.__dex.get_method_short(method.method_idx))
        if ret_jvalue_type:
            file.add_line(r'return retValue.{jvalue_type};'.format(jvalue_type=ret_jvalue_type))
        # function body end

        file.end_block()
        file.add_line('}')
        file.add_line()

    @Log.log_function
    def __add_func_header(self, method: dex_file.EncodedMethod, file: FileBuilder):
        method_id = method.method_idx
        return_type = VmKeyFuncJniFile.wrap_to_jni_type(self.__dex.get_method_return_type(method_id))
        class_name = VmKeyFuncJniFile.wrap_to_jni_name(self.__dex.get_class_name_by_method_id(method_id))
        method_name = VmKeyFuncJniFile.wrap_to_jni_name(self.__dex.get_method_name(method_id))
        param_sign = VmKeyFuncJniFile.wrap_to_jni_name(self.__dex.get_method_param_sign(method_id))
        jni_func_name_map = {'c_name': class_name, 'm_name': method_name, 'sign': param_sign}

        if method.access_flags & dex_file.AccessFlag.ACC_STATIC:
            oc = 'jclass clazz_obj'
        else:
            oc = 'jobject instance'
        pl = ''
        for no, p in enumerate(self.__dex.get_method_param_short_names(method_id)):
            pl += r', {jni_type} {p_name}'.format(jni_type=VmKeyFuncJniFile.wrap_to_jni_type(p),
                                                  p_name=r'param_' + str(no))
        jni_func_param_map = {'o_or_c': oc, 'param_list': pl}

        file.add_line(r'extern "C"')
        file.add_line(r'JNIEXPORT {rt_type} JNICALL'.format(rt_type=return_type))
        if param_sign:
            file.add_line(r'Java_{c_name}_{m_name}__{sign}('.format_map(jni_func_name_map))
        else:
            file.add_line(r'Java_{c_name}_{m_name}('.format_map(jni_func_name_map))
        file.add_line(r'        JNIEnv *env, {o_or_c}{param_list})'.format_map(jni_func_param_map), '')

    @staticmethod
    def wrap_to_jni_type(java_type: str) -> str:
        if java_type[0] == 'I':
            return 'jint'
        elif java_type[0] == 'B':
            return 'jbyte'
        elif java_type[0] == 'Z':
            return 'jboolean'
        elif java_type[0] == 'S':
            return 'jshort'
        elif java_type[0] == 'C':
            return 'jchar'
        elif java_type[0] == 'D':
            return 'jdouble'
        elif java_type[0] == 'F':
            return 'jfloat'
        elif java_type[0] == 'J':
            return 'jlong'
        elif java_type[0] == 'L' or java_type[0] == '[':
            return 'jobject'
        elif java_type[0] == 'V':
            return 'void'

    @staticmethod
    def wrap_to_jni_name(java_name: str) -> str:
        return java_name.replace('_', '_1') \
            .replace(';', '_2') \
            .replace('[', '_3') \
            .replace('/', '_').replace('.', '_') \
            .replace('$', '_00024')

    @staticmethod
    def wrap_to_jvalue_type(java_type: str) -> Union[str, None]:
        if java_type[0] == 'I':
            return 'i'
        elif java_type[0] == 'B':
            return 'b'
        elif java_type[0] == 'Z':
            return 'z'
        elif java_type[0] == 'S':
            return 's'
        elif java_type[0] == 'C':
            return 'c'
        elif java_type[0] == 'D':
            return 'd'
        elif java_type[0] == 'F':
            return 'f'
        elif java_type[0] == 'J':
            return 'j'
        elif java_type[0] == 'L' or java_type[0] == '[':
            return 'l'
        else:
            return None
