import os
from subprocess import Popen, PIPE

import env
from shell.common.utils import Cmd


class ApkTool:
    APKTOOL_PATH = os.path.join(env.LIB_ROOT, r'apktool', r'apktool.jar')

    @classmethod
    def build(cls, in_path: str, out_path: str):
        task = Popen(
            ['java', '-jar', '-Duser.language=en', '-Dfile.encoding=UTF8', cls.APKTOOL_PATH,
             'b', '-f', '-o', out_path, in_path], stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(task)

    @classmethod
    def decode(cls, in_path: str, out_path: str):
        task = Popen(
            ['java', '-jar', '-Duser.language=en', '-Dfile.encoding=UTF8', cls.APKTOOL_PATH,
             'd', '-f', '-o', out_path, in_path], stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(task)
