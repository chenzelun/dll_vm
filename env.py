import logging
import os

ROOT_PATH = os.path.abspath(os.path.curdir)
LIB_ROOT = ROOT_PATH + os.path.sep + r'lib'
LOG_ROOT = ROOT_PATH + os.path.sep + r'log'
TMP_ROOT = ROOT_PATH + os.path.sep + r'tmp'


# config log
def init_logging():
    log_file_path = LOG_ROOT + os.path.sep + 'log.log'

    file_handle = logging.FileHandler(log_file_path, mode='w')
    file_handle.setLevel(logging.DEBUG)
    file_handle.setFormatter(
        logging.Formatter(
            fmt=r'%(asctime)s - %(levelname)s - %(pathname)s[%(lineno)d]'
                r' - pid: %(process)d, tid: %(thread)d - %(funcName)s: %(message)s'))

    console_handle = logging.StreamHandler()
    console_handle.setLevel(logging.DEBUG)
    console_handle.setFormatter(
        logging.Formatter(fmt=r'%(asctime)s - %(levelname)s - %(funcName)s[%(lineno)d]: %(message)s'))

    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    logger.addHandler(file_handle)
    logger.addHandler(console_handle)


# init evn
def init_env():
    init_logging()
