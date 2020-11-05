from env import init_env
from shell.common.utils import Apk
from shell.shell import Shell

if __name__ == '__main__':
    init_env()
    shell = Shell()
    shell.shell()
    Apk.install(shell.dest_apk_path)
    Apk.start(shell.test_app_package_name)
    Apk.uninstall(shell.test_app_package_name)
