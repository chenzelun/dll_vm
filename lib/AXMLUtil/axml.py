import os
from subprocess import Popen, PIPE

import env
from shell.common.utils import Cmd


class AndroidXML:
    """
    1>插入属性

        java -jar AXMLEditor.jar -attr -i [标签名] [标签唯一标识] [属性名] [属性值] [输入xml] [输出xml]

        案例：java -jar AXMLEditor.jar -attr -i application package debuggable true
                        AndroidManifest.xml AndroidManifest_out.xml

        application的标签中插入android:debuggable="true"属性，让程序处于可调式状态

    2>删除属性

        java -jar AXMLEditor.jar -attr -r [标签名] [标签唯一标识] [属性名] [输入xml] [输出xml]

        案例：java -jar AXMLEditor.jar -attr -r application allowBackup AndroidManifest.xml AndroidManifest_out.xml

        application标签中删除allowBackup属性，这样此app就可以进行沙盒数据备份

    3>更改属性

        java -jar AXMLEditor.jar -attr -m [标签名] [标签唯一标识] [属性名] [属性值] [输入xml] [输出xml]

        案例：java -jar AXMLEditor.jar -attr -m application package debuggable true
                        AndroidManifest.xml AndroidManifest_out.xml

        application的标签中修改android:debuggable="true"属性，让程序处于可调式状态

    4>插入标签

        java -jar AXMLEditor.jar -tag -i [需要插入标签内容的xml文件] [输入xml] [输出xml]

        案例：java -jar AXMLEditor.jar -tag -i [insert.xml] AndroidManifest.xml AndroidManifest_out.xml

        因为插入标签时一个标签内容比较多，所以命令方式不方便，而是输入一个需要插入标签内容的xml文件即可。

    5>删除标签

        java -jar AXMLEditor.jar -tag -r [标签名] [标签唯一标识] [输入xml] [输出xml]

        案例：java -jar AXMLEditor.jar -tag -r activity cn.wjdiankong.demo.MainActivity
                        AndroidManifest.xml AndroidManifest_out.xml

        删除android:name="cn.wjdiankong.demo.MainActivity"的标签内容
    """

    editor_path = os.path.join(env.LIB_ROOT, r'AXMLUtil', r'AXMLEditor.jar')
    printer_path = os.path.join(env.LIB_ROOT, r'AXMLUtil', r'AXMLPrinter3.jar')

    @classmethod
    def insert_attr(cls, tag, name, key, value, in_path, out_path):
        child = Popen(
            ['java', '-jar', cls.editor_path, '-attr', '-i', tag, name, key, value, in_path, out_path],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(child)

    @classmethod
    def remove_attr(cls, tag, name, key, in_path, out_path):
        child = Popen(
            ['java', '-jar', cls.editor_path, '-attr', '-r', tag, name, key, in_path, out_path],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(child)

    @classmethod
    def modify_attr(cls, tag, name, key, value, in_path, out_path):
        child = Popen(
            ['java', '-jar', cls.editor_path, '-attr', '-m', tag, name, key, value, in_path, out_path],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(child)

    @classmethod
    def insert_tag(cls, tag_xml_path, in_path, out_path):
        child = Popen(
            ['java', '-jar', cls.editor_path, '-tag', '-i', tag_xml_path, in_path, out_path],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(child)

    @classmethod
    def remove_tag(cls, tag, name, in_path, out_path):
        child = Popen(
            ['java', '-jar', cls.editor_path, '-tag', '-r', tag, name, in_path, out_path],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(child)

    @classmethod
    def print_xml(cls, in_path):
        child = Popen(
            ['java', '-jar', cls.printer_path, in_path],
            stdout=PIPE, stderr=PIPE)
        Cmd.run_and_wait(child)

    @classmethod
    def axml2xml(cls, in_path) -> (str, str):
        child = Popen(
            ['java', '-jar', cls.printer_path, in_path],
            stdout=PIPE, stderr=PIPE)
        return Cmd.run_and_wait(child)
