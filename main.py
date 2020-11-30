import env
from shell.common.utils import Apk
from shell.shell import Shell

if __name__ == '__main__':
    while True:
        env.init_env()
        shell = Shell()
        shell.shell()
        Apk.install(shell.dest_apk_path, env.ADB_TOOL_PATH)
        Apk.start(shell.test_app_package_name, env.ADB_TOOL_PATH)
        Apk.uninstall(shell.test_app_package_name, env.ADB_TOOL_PATH)
