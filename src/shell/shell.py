import logging
import os
import shutil
import sys
import zipfile

import env
from lib.AXMLUtil.axml import AndroidXML
from lib.apktool.apktool import ApkTool
from shell.common.utils import Apk, Log
from shell.data.vm_data import VmDataFile
from shell.dex.modifier import DexFileModifier


class Shell:
    def __init__(self):
        self.log = logging.getLogger(Shell.__name__)

        self.test_path = os.path.join(env.TMP_ROOT, r'test_app')
        self.test_app_package_name = ''

        self.vm_path = os.path.join(env.TMP_ROOT, r'vm')

        self.dest_path_root = os.path.join(env.TMP_ROOT, r'dest_root')
        self.dest_path = os.path.join(self.dest_path_root, r'dest')
        self.dest_assets_path = os.path.join(self.dest_path, r'assets')
        self.dest_lib_path = os.path.join(self.dest_path, r'lib')
        self.dest_apk_name = r'dest.apk'
        self.dest_apk_path = os.path.join(env.OUT_ROOT, self.dest_apk_name)

        self.vm_data_file = VmDataFile()
        self.vm_data_file_path = os.path.join(self.dest_assets_path, r'vm_data.bin')

    @Log.log_function
    def shell(self):
        self.init()
        self.virtual_dex_file()

        # rebuild vm app with key function.
        self.rebuild_and_unzip(env.VM_APP_ROOT, self.vm_path)
        self.log.info(r'update   vm apk: ' + self.vm_path)

        self.copy_vm_dex()
        self.change_application_name()
        self.append_libs()
        self.copy_others()
        self.build_vm_data()
        self.build_apk_and_signed()

    @Log.log_function
    def copy_vm_dex(self):
        src_path = os.path.join(self.vm_path, r'classes.dex')
        dst_path = self.dest_path
        shutil.copy(src_path, dst_path)
        self.log.info(r'copy from: {} to: {}'.format(src_path, dst_path))

    @Log.log_function
    def change_application_name(self):
        # get application name from test dex
        test_manifest_path = os.path.join(self.test_path, r'AndroidManifest.xml')
        xml_str, xml_err = AndroidXML.axml2xml(test_manifest_path)
        if xml_err:
            self.log.error("can't parse test app android manifest xml.")
            sys.exit()
        key_word_1 = r'package='
        key_word_2 = r'<application'
        key_word_3 = r'android:name='
        key_word_4 = r'>'
        cur_idx = xml_str.find(key_word_1) + len(key_word_1) + 1
        end_idx = xml_str.find(r'"', cur_idx)
        self.test_app_package_name = xml_str[cur_idx:end_idx]
        cur_idx = xml_str.find(key_word_2, end_idx)
        end_idx = xml_str.find(key_word_4, cur_idx)
        cur_idx = xml_str.find(key_word_3, cur_idx, end_idx)
        default_application_name = r'android.app.Application'
        if cur_idx > 0:
            cur_idx += len(key_word_3) + 1
            end_idx = xml_str.find(r'"', cur_idx)
            test_application_name = xml_str[cur_idx:end_idx]
            if test_application_name[0] == r'.':
                test_application_name = self.test_app_package_name + test_application_name
        else:
            test_application_name = default_application_name

        self.vm_data_file.add_key_value(r'application_name', test_application_name)
        self.log.info("test application name: " + test_application_name)

        # get application name from vm dex
        vm_dex_path = os.path.join(self.vm_path, r'classes.dex')
        modifier = DexFileModifier.parse_dex_file(vm_dex_path)
        application_name = modifier.get_class_names_by_super_class_name(default_application_name)
        assert len(application_name) == 1
        application_name = application_name[0]
        self.log.info("vm app application_name: " + application_name)

        dest_manifest_path = os.path.join(self.dest_path, r'AndroidManifest.xml')
        tmp_manifest_path = test_manifest_path + '.xml'
        # change application name
        AndroidXML.modify_attr(r'application', r'package', r'name', application_name,
                               test_manifest_path, tmp_manifest_path)
        # delete attr: android:appComponentFactory="androidx.core.app.CoreComponentFactory"
        AndroidXML.remove_attr(r'application', r'package', r'appComponentFactory',
                               tmp_manifest_path, dest_manifest_path)

        # log
        self.log.debug("test app android manifest xml: " + test_manifest_path)
        AndroidXML.print_xml(test_manifest_path)
        self.log.debug("dest app android manifest xml: " + dest_manifest_path)
        AndroidXML.print_xml(dest_manifest_path)

    @Log.log_function
    def init(self):
        # remove old files
        shutil.rmtree(env.TMP_ROOT)
        self.log.info(r'remove old tmp folder')
        os.makedirs(self.test_path)
        os.makedirs(self.vm_path)
        os.makedirs(self.dest_path)
        os.makedirs(self.dest_assets_path)
        self.log.debug(r'mkdir: ' + self.test_path)
        self.log.debug(r'mkdir: ' + self.vm_path)
        self.log.debug(r'mkdir: ' + self.dest_path)
        self.log.debug(r'mkdir: ' + self.dest_assets_path)

        # rebuild and unzip
        self.rebuild_and_unzip(env.TEST_APP_ROOT, self.test_path)
        self.log.info(r'update test apk: ' + self.test_path)

    @Log.log_function
    def rebuild_and_unzip(self, app_root: str, dest_path: str):
        if dest_path != self.test_path:
            if not Apk.build(app_root):
                self.log.error(r'errors about building app: ' + app_root)
                sys.exit()

        apk_path = os.path.join(app_root, r'app',
                                r'build', r'outputs', r'apk',
                                r'release', r'app-release-unsigned.apk')
        zipfile.ZipFile(apk_path).extractall(dest_path)

    @Log.log_function
    def virtual_dex_file(self):
        dex_file_name = r'classes.dex'
        test_dex_path = os.path.join(self.test_path, dex_file_name)
        modifier = DexFileModifier.parse_dex_file(test_dex_path, self.vm_data_file)
        modifier.native_key_func(env.RES_KEY_FUNCTIONS_DEFINED_PATH,
                                 env.KEY_FUNC_JNI_ROOT)
        dex_file_buf = modifier.dex.to_bytes()
        self.vm_data_file.add_file(dex_file_name, dex_file_buf)

    @Log.log_function
    def copy_others(self):
        # test app others
        for file_name in os.listdir(self.test_path):
            if os.path.isdir(os.path.join(self.test_path, file_name)):
                if file_name in (r'lib', r'META-INF'):
                    continue
                shutil.copytree(os.path.join(self.test_path, file_name),
                                os.path.join(self.dest_path, file_name))
            else:
                if file_name.startswith(r'AndroidManifest.xml') or \
                        (file_name.endswith('.dex') and
                         os.path.isfile(os.path.join(self.test_path, file_name))):
                    continue
                shutil.copy(os.path.join(self.test_path, file_name), self.dest_path)

        # copy vm app lib
        src_dir = os.path.join(self.vm_path, r'lib', r'arm64-v8a')
        dst_dir = os.path.join(self.dest_lib_path, r'arm64-v8a')
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)
        for file_name in os.listdir(src_dir):
            if not file_name.endswith(r'.so'):
                continue
            shutil.copy(os.path.join(src_dir, file_name), dst_dir)
            self.log.debug(r'copy lib file: ' + os.path.join(src_dir, file_name))

    @Log.log_function
    def build_apk_and_signed(self):
        # build apk
        apk_zip_path = shutil.make_archive(self.dest_path, r'zip', self.dest_path)
        self.log.debug(r'apk_zip_path: ' + apk_zip_path)
        dest_unsigned_apk_path = self.dest_path + r'_unsigned.apk'
        os.rename(apk_zip_path, dest_unsigned_apk_path)
        self.log.debug(r'dest_unsigned_apk_path: ' + dest_unsigned_apk_path)

        # apktool decode and build
        # if not, Android system can't set ShellApplication
        dest_unsigned_apk_decode_path = self.dest_path + r'_decode'
        dest_unsigned_apk_build_path = self.dest_path + r'_build_unsigned.apk'
        ApkTool.decode(dest_unsigned_apk_path, dest_unsigned_apk_decode_path)
        ApkTool.build(dest_unsigned_apk_decode_path, dest_unsigned_apk_build_path)

        # sign
        if os.path.exists(self.dest_apk_path):
            os.remove(self.dest_apk_path)
        Apk.sign(r'123456', r'dll', r'654321', dest_unsigned_apk_build_path, self.dest_apk_path)
        self.log.info(r'new signed apk: ' + self.dest_apk_path)

    @Log.log_function
    def append_libs(self):
        # arm64-v8a only
        # test_lib_root = os.path.join(self.test_path, r'lib', r'arm64-v8a')
        # for root_path, _, file_names in os.walk(test_lib_root):
        #     for file in file_names:
        #         file_path = os.path.join(root_path, file)
        #         file_name = file_path[len(self.test_path) + len(r'/lib/'):]
        #         self.log.debug(r'append lib: ' + file_name + ', from: ' + file_path)
        #         with open(file_path, 'rb') as r:
        #             file_data = r.read()
        #         self.vm_data_file.add_file(file_name, file_data)
        dst_dir = os.path.join(self.dest_path, r'lib', r'arm64-v8a')
        src_dir = os.path.join(self.test_path, r'lib', r'arm64-v8a')
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)
        for file_name in os.listdir(src_dir):
            if not file_name.endswith(r'.so'):
                continue
            shutil.copy(os.path.join(src_dir, file_name), dst_dir)
            self.log.debug(r'copy lib file: ' + os.path.join(src_dir, file_name))

    @Log.log_function
    def build_vm_data(self):
        with open(self.vm_data_file_path, 'wb') as w:
            w.write(self.vm_data_file.to_bytes())
