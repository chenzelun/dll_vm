import logging
from typing import List

from shell.dex.dex_file import DexFile, MapListItemType, NO_INDEX


class DexFileModifier:
    def __init__(self, dex: DexFile):
        self.dex = dex
        self.log = logging.getLogger(DexFileModifier.__name__)

    @staticmethod
    def parse_dex_file(dex_path: str):
        return DexFileModifier(DexFile.parse_file(dex_path))

    def get_class_names_by_super_class_name(self, super_name: str) -> List[str]:
        ret = []
        super_name = r'L' + super_name.replace('.', '/') + r';'
        class_def_pool = self.dex.map_list.map[MapListItemType.TYPE_CLASS_DEF_ITEM].data
        cur_idx = 0
        dst_idx = 0
        for cur_idx, class_def in enumerate(class_def_pool):
            if class_def.superclass_idx != NO_INDEX and \
                    super_name == self.dex.get_type_name_by_idx(class_def.superclass_idx):
                dst_idx = class_def.superclass_idx
                ret.append(self.dex.get_type_name_by_idx(class_def.class_id)[1:-1].replace('/', '.'))
                break

        for cur_idx in range(cur_idx + 1, len(class_def_pool)):
            if cur_idx == dst_idx:
                ret.append(self.dex.get_type_name_by_idx(
                    class_def_pool.get_item(cur_idx).class_id)[1:-1].replace('/', '.'))

        return ret
