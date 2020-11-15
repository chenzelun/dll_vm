from typing import List, Optional

from shell.common.utils import Debugger


class Method:
    def __init__(self, line: str):
        self.clazz = ''
        self.method = ''
        self.sign: Optional[str] = None
        self.parse(line)

    def parse(self, line: str):
        method = line.split(r':')
        assert len(method) == 2 or len(method) == 3
        self.clazz = method[0]
        self.method = method[1]
        if len(method) == 3:
            self.sign = method[2]
        return self

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class KeyFunctionDefined:
    def __init__(self, file_path: str = None):
        self.clazz: List[str] = []
        self.method: List[Method] = []
        self.__parse(file_path)

    def __parse(self, file_name: str):
        with open(file_name, 'r') as reader:
            data_lines = reader.readlines()
        data_lines = sorted(map(str.strip, data_lines))

        last = ''
        for line in data_lines:
            if line == '' or line.startswith(r'//'):
                continue
            elif line.startswith(last):
                continue

            if line.find(r':') != -1:
                self.method.append(Method(line))
            else:
                self.clazz.append(line)
            last = line

        return self

    @Debugger.print_all_fields
    def __repr__(self):
        pass
