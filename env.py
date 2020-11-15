import os

from shell.common.utils import Log

ROOT_PATH = os.path.abspath(os.path.curdir)
LIB_ROOT = os.path.join(ROOT_PATH, r'lib')
LOG_ROOT = os.path.join(ROOT_PATH, r'log')
TMP_ROOT = os.path.join(ROOT_PATH, r'build')
OUT_ROOT = os.path.join(ROOT_PATH, r'out')
RES_ROOT = os.path.join(ROOT_PATH, r'res')

TEST_ROOT = os.path.join(ROOT_PATH, r'test')
TEST_APP_ROOT = os.path.join(TEST_ROOT, r'TestApp')
VM_APP_ROOT = os.path.join(ROOT_PATH, r'src', r'vm')

RES_KEY_STORE_PATH = os.path.join(
    RES_ROOT,
    r'key_store',
    r'android_apk_key_store_123456_dll_654321')

RES_KEY_FUNCTIONS_DEFINED_PATH = os.path.join(
    RES_ROOT,
    r'key_functions',
    r'define')

KEY_FUNC_JNI_H_NAME = r'key_func_jni.h'
KEY_FUNC_JNI_CPP_NAME = r'key_func_jni.cpp'
VM_DATA_KEY_FUNC_CODE_FILE_NAME = r'code'


# init evn
def init_env():
    Log.init_logging()
