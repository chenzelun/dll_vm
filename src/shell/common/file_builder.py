class FileBuilder:
    def __init__(self):
        self.__block = 0
        self.__buf = ''

    def build(self) -> str:
        return self.__buf

    def add_line(self, data: str = '', __end_line='\n'):
        self.__buf += ' ' * self.__block * 4 + data + __end_line

    def new_block(self):
        self.__block += 1

    def end_block(self):
        self.__block -= 1
        assert self.__block >= 0

    def append(self, data: str = '', __end_line='\n'):
        self.__buf += data + __end_line
