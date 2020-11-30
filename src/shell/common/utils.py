import logging
import os
from functools import wraps
from subprocess import Popen, PIPE
from typing import Set, Union

import chardet

import env


class Debugger:
    @staticmethod
    def print_all_fields(func):
        @wraps(func)
        def wrapper(obj):
            return str(obj.__dict__)

        return wrapper


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


class Cmd:
    @staticmethod
    def run_and_wait(task: Popen, log: logging.Logger = None) -> (str, str):
        out_byte, err_byte = task.communicate()
        out_str, err_str = None, None
        if out_byte:
            out_str = str(out_byte, encoding=chardet.detect(out_byte)['encoding'])
        if err_byte:
            err_str = str(err_byte, encoding=chardet.detect(err_byte)['encoding'])
        if not log:
            log = logging.getLogger(Cmd.__name__)

        if out_str:
            log.debug(out_str)
        if err_str:
            log.error(err_str)
        return out_str, err_str


class Apk:
    @staticmethod
    def build(apk_root_path: str) -> bool:
        task = Popen([os.path.join(apk_root_path, r'gradlew'),
                      'clean', 'assembleRelease', '--info', '--stacktrace'],
                     stdout=PIPE, stderr=PIPE, cwd=apk_root_path)
        out, err = Cmd.run_and_wait(task)
        if not err and r'BUILD SUCCESSFUL' in out:
            return True
        else:
            return False

    @staticmethod
    def install(apk_path, adb_path: str = None):
        if not adb_path:
            adb_path = r'adb'
        elif not adb_path.endswith('adb'):
            os.path.join(adb_path, 'adb')
        task = Popen(
            [adb_path, 'install', '-r', '-t', apk_path],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(task)

    @staticmethod
    def start(pkg_name, adb_path: str = None):
        if not adb_path:
            adb_path = r'adb'
        elif not adb_path.endswith('adb'):
            os.path.join(adb_path, 'adb')
        task = Popen(
            [adb_path, 'shell', 'am', 'start', pkg_name + '/.MainActivity'],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(task)

    @staticmethod
    def uninstall(pkg_name, adb_path: str = None):
        if not adb_path:
            adb_path = r'adb'
        elif not adb_path.endswith('adb'):
            os.path.join(adb_path, 'adb')
        task = Popen(
            [adb_path, 'uninstall', pkg_name],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(task)

    @staticmethod
    def sign(store_pwd, alias, alias_pwd, in_path, out_path):
        task = Popen(
            ['jarsigner', '-verbose', '-sigalg', 'SHA1withRSA', '-digestalg', 'SHA1',
             '-keystore', env.RES_KEY_STORE_PATH, '-storepass', store_pwd,
             '-signedjar', out_path, in_path,
             alias, '-keypass', alias_pwd],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(task)


class Log:
    # config log
    @staticmethod
    def init_logging():
        log_file_path = os.path.join(env.LOG_ROOT, 'log.log')

        file_handle = logging.FileHandler(log_file_path, mode='w')
        file_handle.setLevel(logging.DEBUG)
        file_handle.setFormatter(
            logging.Formatter(
                fmt=r'%(asctime)s - %(levelname)s - %(pathname)s - pid: %(process)d, tid: %(thread)d\n'
                    r'%(funcName)s[%(lineno)d]: %(message)s'))

        console_handle = logging.StreamHandler()
        console_handle.setLevel(logging.DEBUG)
        console_handle.setFormatter(
            logging.Formatter(fmt=r'%(levelname)s - %(funcName)s[%(lineno)d]: %(message)s'))

        logger = logging.getLogger()
        logger.setLevel(logging.DEBUG)
        logger.addHandler(file_handle)
        logger.addHandler(console_handle)

    @staticmethod
    def log_function(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            obj = ReflectHelper.get_var_by_index(0, *args, **kwargs)
            if 'log' not in obj.__dict__:
                raise RuntimeWarning(r"can't find a field named 'log'.")
            obj.log.debug("enter function: " + func.__name__)

            ret = func(*args, **kwargs)

            if ret:
                obj.log.debug("ret value: ")
                obj.log.debug(ret)
            obj.log.debug("out function: " + func.__name__)
            return ret

        return wrapper

    @staticmethod
    def log_function_with_params(
            is_static: bool, *,
            include: Set[Union[int, str]] = None,
            exclude: Set[Union[int, str]] = None):
        def function_wrapper(func):
            @wraps(func)
            def wrapper(*args, **kwargs):
                obj = ReflectHelper.get_var_by_index(0, *args, **kwargs)
                if 'log' not in obj.__dict__:
                    raise RuntimeWarning(r"can't find a field named 'log'.")
                obj.log.debug("enter function: " + func.__name__)
                is_include_flag = True if include else False
                log_args = args if is_static else args[1:]
                for i, v in enumerate(log_args):
                    if include or exclude:
                        if is_include_flag and i not in include:
                            continue
                        elif not is_include_flag and i in exclude:
                            continue

                    obj.log.debug("param[{}]: {}".format(str(i), str(v)))
                for k, v in kwargs.items():
                    if include or exclude:
                        if is_include_flag and k not in include:
                            continue
                        elif not is_include_flag and k in exclude:
                            continue

                    obj.log.debug("param[{}]: {}".format(str(k), str(v)))

                ret = func(*args, **kwargs)

                if ret:
                    obj.log.debug("ret value: ")
                    obj.log.debug(ret)
                obj.log.debug("out function: " + func.__name__)
                return ret

            return wrapper

        return function_wrapper


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
    def aligned_4_with_zero(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            pr: Pointer = ReflectHelper.get_first_var_by_type(Pointer, *args, **kwargs)
            buf: bytearray = ReflectHelper.get_first_var_by_type(bytearray, *args, **kwargs)
            old_len = pr.cur
            pr.aligned(0x04)
            while old_len < pr.cur:
                buf.append(0)
                old_len += 1

            return func(*args, **kwargs)

        return wrapper

    @staticmethod
    def update_offset(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            pr: Pointer = ReflectHelper.get_first_var_by_type(Pointer, *args, **kwargs)
            obj = ReflectHelper.get_var_by_index(0, *args, **kwargs)
            obj.offset = pr.cur
            return func(*args, **kwargs)

        return wrapper

    @staticmethod
    def update_pointer(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            pr: Pointer = ReflectHelper.get_first_var_by_type(Pointer, *args, **kwargs)
            buf: bytearray = ReflectHelper.get_first_var_by_type(bytearray, *args, **kwargs)
            old_len = len(buf)
            ret = func(*args, **kwargs)
            new_len = len(buf)
            pr.add(new_len - old_len)
            return ret

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
