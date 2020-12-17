import logging
from abc import ABCMeta, abstractmethod
from enum import IntEnum, unique
from typing import List, Dict, Optional, Union

from shell.common.file_builder import FileBuilder
from shell.common.utils import Debugger, Log, Pointer
from shell.dex.dex_file import DexFile, EncodedMethod


@unique
class InstructionType(IntEnum):
    T = 0
    T12D12 = 1
    T1D = 2
    TD = 3
    T1 = 4
    TD1 = 5
    T11 = 6
    T1D1 = 7
    T12D = 8
    T12 = 9


@unique
class InstructionLinkType(IntEnum):
    LT_NO = 0
    LT_NEXT = 1
    LT_GOTO = 2
    LT_NEXT_GOTO = 3


class Instruction(metaclass=ABCMeta):
    def __init__(self, std_opcode, std_len):
        self.offset = -1
        self.std_opcode = std_opcode
        self.std_len = std_len

        self.op = 0

        self.src1 = 0
        self.src2 = 0
        self.dst = 0
        self.val1 = 0
        self.val2 = 0

        self.link_type = InstructionLinkType.LT_NEXT
        self.next: Optional[Instruction] = None
        self.goto: Union[int, Instruction] = 0

        self.insn_type = InstructionType.T
        self.bytecode = bytearray()
        self.assign = []
        self.handle = []

    @abstractmethod
    def parse(self, buf: bytes, pr: Pointer):
        pass

    @Pointer.update_offset
    def insns(self, buf: bytes, pr: Pointer):
        pass

    def link(self, off_handler_map):
        # off_handler_map: Dict[int, Instruction]
        # InstructionLinkType.LT_NO: do nothing
        if self.link_type==InstructionLinkType.LT_GOTO:
            # noinspection PyTypeChecker
            self.goto = off_handler_map[self.offset+self.goto]
        elif self.link_type==InstructionLinkType.LT_NEXT:
            self.next = off_handler_map[self.offset+self.std_len*2]
        elif self.link_type==InstructionLinkType.LT_NEXT_GOTO:
            self.next = off_handler_map[self.offset + self.std_len * 2]
            self.goto = off_handler_map[self.offset + self.goto]

    def to_cpp(self, name_prefix: str, name: str) -> List[str]:
        ret = ['']
        data = 'void {name_prefix}_{name}'.format(name_prefix=name_prefix, name=name)
        data += '::run(VmMethodContext *vmc) {'
        ret.append(data)
        ret.extend(self.assign)
        ret.extend(self.handle)
        assert len(self.insns) & 0x01 == 0
        data = 'vmc->pc_off({offset});'.format(offset=str(len(self.insns) // 2))
        ret.append(data)
        ret.append('}')
        ret.append('')
        return ret

    @staticmethod
    def to_hpp(name_prefix: str, name: str, super_name: str) -> List[str]:
        ret = ['']
        data = 'class {name_prefix}_{name}'.format(name_prefix=name_prefix, name=name)
        if super_name:
            data += ' : public {super_name}'.format(super_name=super_name)
        data += ' {'
        ret.append(data)
        ret.append('public:')
        ret.append('    void run(VmMethodContext *vmc) override;')
        ret.append('};')
        ret.append('')
        return ret

    @staticmethod
    def inst_a(buf: bytes, pr: Pointer):
        return buf[pr.cur * 2 + 1] & 0x0f

    @staticmethod
    def inst_b(buf: bytes, pr: Pointer):
        return (buf[pr.cur * 2 + 1] >> 4) & 0x0f

    @staticmethod
    def inst_aa(buf: bytes, pr: Pointer):
        return buf[pr.cur * 2 + 1]

    @staticmethod
    def fetch(buf: bytes, pr: Pointer, off: int):
        return (buf[(pr.cur + off) * 2 + 1] << 8) | buf[(pr.cur + off) * 2]

    @staticmethod
    def to_signed(num: int, bit_count: int):
        if num & (1 << (bit_count - 1)):
            num -= 1 << bit_count
        return num

    @abstractmethod
    def __repr__(self):
        """ debug object. """


class ST_CH_NOP(Instruction):
    def __init__(self):
        super().__init__(0x00, 1)
        self.handle = [
            r"""LOG_D_VM("|nop");""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.insn_type = InstructionType.T
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move(Instruction):
    def __init__(self):
        super().__init__(0x01, 1)
        self.handle = [
            r"""LOG_D_VM("|move%s v%u,v%u %s(v%u=%d)", "", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_From16(Instruction):
    def __init__(self):
        super().__init__(0x02, 2)
        self.handle = [
            r"""LOG_D_VM("|move%s/from16 v%u,v%u %s(v%u=%d)", "", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_16(Instruction):
    def __init__(self):
        super().__init__(0x03, 3)
        self.handle = [
            r"""LOG_D_VM("|move%s/16 v%u,v%u %s(v%u=%d)", "", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.fetch(buf, pr, 1)
        self.src1 = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Wide(Instruction):
    def __init__(self):
        super().__init__(0x04, 1)
        self.handle = [
            r"""LOG_D_VM("|move-wide v%u,v%u %s(v%u=%ld)", vmc->tmp->dst, vmc->tmp->src1, kSpacing + 5, vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Wide_From16(Instruction):
    def __init__(self):
        super().__init__(0x05, 2)
        self.handle = [
            r"""LOG_D_VM("|move-wide/from16 v%u,v%u  (v%u=%ld)", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Wide16(Instruction):
    def __init__(self):
        super().__init__(0x06, 3)
        self.handle = [
            r"""LOG_D_VM("|move-wide/16 v%u,v%u %s(v%u=%ld)", vmc->tmp->dst, vmc->tmp->src1, kSpacing + 8, vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->src1));""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.fetch(buf, pr, 1)
        self.src1 = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Object(Instruction):
    def __init__(self):
        super().__init__(0x07, 1)
        self.handle = [
            r"""LOG_D_VM("|move%s v%u,v%u %s(v%u=%p)", "-object", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Object_From16(Instruction):
    def __init__(self):
        super().__init__(0x08, 2)
        self.handle = [
            r"""LOG_D_VM("|move%s/from16 v%u,v%u %s(v%u=%p)", "-object", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Object16(Instruction):
    def __init__(self):
        super().__init__(0x09, 3)
        self.handle = [
            r"""LOG_D_VM("|move%s/16 v%u,v%u %s(v%u=%p)", "-object", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.fetch(buf, pr, 1)
        self.src1 = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Result(Instruction):
    def __init__(self):
        super().__init__(0x0a, 1)
        self.handle = [
            r"""LOG_D_VM("|move-result%s v%u %s(v%u=%d)", "", vmc->tmp->dst, kSpacing + 4, vmc->tmp->dst, vmc->retVal->i);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.TD
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Result_Wide(Instruction):
    def __init__(self):
        super().__init__(0x0b, 1)
        self.handle = [
            r"""LOG_D_VM("|move-result-wide v%u %s(v%u=%ld)", vmc->tmp->dst, kSpacing, vmc->tmp->dst, vmc->retVal->j);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->retVal->j);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.TD
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Result_Object(Instruction):
    def __init__(self):
        super().__init__(0x0c, 1)
        self.handle = [
            r"""LOG_D_VM("|move-result%s v%u %s(v%u=0x%p)", "-object", vmc->tmp->dst, kSpacing + 4, vmc->tmp->dst, vmc->retVal->l);""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->retVal->l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.TD
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Move_Exception(Instruction):
    def __init__(self):
        super().__init__(0x0d, 1)
        self.handle = [
            r"""LOG_D_VM("|move-exception v%u", vmc->tmp->dst);""",
            r"""assert(vmc->curException != nullptr);""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->curException);""",
            r"""vmc->curException = nullptr;""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.TD
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Return_Void(Instruction):
    def __init__(self):
        super().__init__(0x0e, 1)
        self.handle = [
            r"""LOG_D_VM("|return-void");""",
            r"""vmc->finish();""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.insn_type = InstructionType.T
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Return(Instruction):
    def __init__(self):
        super().__init__(0x0f, 1)
        self.handle = [
            r"""LOG_D_VM("|return%s v%u", "", vmc->tmp->src1);""",
            r"""vmc->retVal->j = 0L;    // set 0""",
            r"""vmc->retVal->i = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""vmc->finish();""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Return_Wide(Instruction):
    def __init__(self):
        super().__init__(0x10, 1)
        self.handle = [
            r"""LOG_D_VM("return-wide v%u", vmc->tmp->src1);""",
            r"""vmc->retVal->j = vmc->getRegisterLong(vmc->tmp->src1);""",
            r"""vmc->finish();""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Return_Object(Instruction):
    def __init__(self):
        super().__init__(0x11, 1)
        self.handle = [
            r"""LOG_D_VM("|return%s v%u", "-object", vmc->tmp->src1);""",
            r"""vmc->retVal->l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""vmc->finish();""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const4(Instruction):
    def __init__(self):
        super().__init__(0x12, 1)
        self.handle = [
            r"""LOG_D_VM("|const/4 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s4);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_1.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.val1 = self.to_signed(self.inst_b(buf, pr), 4)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const16(Instruction):
    def __init__(self):
        super().__init__(0x13, 2)
        self.handle = [
            r"""LOG_D_VM("|const/16 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->tmp->val_1.s2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const(Instruction):
    def __init__(self):
        super().__init__(0x14, 3)
        self.handle = [
            r"""LOG_D_VM("|const v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s4);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_1.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = (self.fetch(buf, pr, 2) << 16) | self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_High16(Instruction):
    def __init__(self):
        super().__init__(0x15, 2)
        self.handle = [
            r"""LOG_D_VM("|const/high16 v%u,#0x%04x0000", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, (u4) vmc->tmp->src1 << 16u);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_Wide16(Instruction):
    def __init__(self):
        super().__init__(0x16, 2)
        self.handle = [
            r"""LOG_D_VM("|const-wide/16 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_1.s2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_Wide32(Instruction):
    def __init__(self):
        super().__init__(0x17, 3)
        self.handle = [
            r"""LOG_D_VM("|const-wide/32 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s4);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_1.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = (self.fetch(buf, pr, 2) << 16) | self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_Wide(Instruction):
    def __init__(self):
        super().__init__(0x18, 5)
        self.handle = [
            r"""LOG_D_VM("|const-wide v%u,#%ld", vmc->tmp->dst, vmc->tmp->val_1.s8);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_1.s8);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.val1 |= self.fetch(buf, pr, 2) << 16
        self.val1 |= self.fetch(buf, pr, 3) << 32
        self.val1 |= self.fetch(buf, pr, 4) << 48
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_Wide_High16(Instruction):
    def __init__(self):
        super().__init__(0x19, 2)
        self.handle = [
            r"""LOG_D_VM("|const-wide/high16 v%u,#0x%04x000000000000", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.u8 = ((u8) vmc->tmp->src1) << 48u;""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.s8);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_String(Instruction):
    def __init__(self):
        super().__init__(0x1a, 2)
        self.handle = [
            r"""LOG_D_VM("|const-string v%u string@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_1.l = vmc->method->resolveString(vmc->tmp->val_1.u4);""",
            r"""assert(vmc->tmp->val_1.l != nullptr);""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_String_Jumbo(Instruction):
    def __init__(self):
        super().__init__(0x1b, 3)
        self.handle = [
            r"""LOG_D_VM("|const-string/jumbo v%u string@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_1.l = vmc->method->resolveString(vmc->tmp->val_1.u4);""",
            r"""assert(vmc->tmp->val_1.l != nullptr);""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.val1 |= self.fetch(buf, pr, 2) << 16
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Const_Class(Instruction):
    def __init__(self):
        super().__init__(0x1c, 1)
        self.handle = [
            r"""LOG_D_VM("|const-class v%u class@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_1.l = vmc->method->resolveClass(vmc->tmp->val_1.u4);""",
            r"""if (vmc->tmp->val_1.l == nullptr) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Monitor_Enter(Instruction):
    def __init__(self):
        super().__init__(0x1d, 1)
        self.handle = [
            r"""LOG_D_VM("|monitor-enter v%u %s(%p)", vmc->tmp->src1, kSpacing + 6, vmc->getRegisterAsObject(vmc->tmp->src1));""",
            r"""vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""(*VM_CONTEXT::env).MonitorEnter(vmc->tmp->val_1.l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Monitor_Exit(Instruction):
    def __init__(self):
        super().__init__(0x1e, 1)
        self.handle = [
            r"""LOG_D_VM("|monitor-exit v%u %s(%p)", vmc->tmp->src1, kSpacing + 5, vmc->getRegisterAsObject(vmc->tmp->src1));""",
            r"""vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""/**""",
            r""" * 注意：如果该指令需要抛出异常，则必须像 PC 已超出该指令那样抛出。""",
            r""" * 不妨将其想象成，该指令（在某种意义上）已成功执行，并在该指令执行后""",
            r""" * 但下一条指令找到机会执行前抛出异常。这种定义使得某个方法有可能将""",
            r""" * 监视锁清理 catch-all（例如 finally）分块用作该分块自身的""",
            r""" * 监视锁清理，以便处理可能由于 Thread.stop() 的既往实现而""",
            r""" * 抛出的任意异常，同时仍尽力维持适当的监视锁安全机制。""",
            r""" */""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""if (!(*VM_CONTEXT::env).MonitorExit(vmc->tmp->val_1.l)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Check_Cast(Instruction):
    def __init__(self):
        super().__init__(0x1f, 1)
        self.handle = [
            r"""LOG_D_VM("|check-cast v%u,class@%u", vmc->tmp->src1, vmc->tmp->val_2.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.l) {""",
            r"""    vmc->tmp->val_1.lc = vmc->method->resolveClass(vmc->tmp->val_1.u4);""",
            r"""    if (vmc->tmp->val_1.lc == nullptr) {""",
            r"""        JavaException::throwJavaException(vmc);""",
            r"""        return;""",
            r"""    }""",
            r"""    if (!(*VM_CONTEXT::env).IsInstanceOf(vmc->tmp->val_2.l, vmc->tmp->val_1.lc)) {""",
            r"""        JavaException::throwClassCastException(""",
            r"""            vmc, (*VM_CONTEXT::env).GetObjectClass(vmc->tmp->val_2.l), vmc->tmp->val_1.lc);""",
            r"""        return;""",
            r"""    }""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T11
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Instance_Of(Instruction):
    def __init__(self):
        super().__init__(0x20, 1)
        self.handle = [
            r"""LOG_D_VM("|instance-of v%u,v%u,class@%u", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.l == nullptr) {""",
            r"""    vmc->setRegister(vmc->tmp->dst, 0);""",
            r"""} else {""",
            r"""    vmc->tmp->val_1.lc = vmc->method->resolveClass(vmc->tmp->val_1.u4);""",
            r"""    if (vmc->tmp->val_1.lc == nullptr) {""",
            r"""        JavaException::throwJavaException(vmc);""",
            r"""        return;""",
            r"""    }""",
            r"""    vmc->tmp->val_2.z = (*VM_CONTEXT::env).IsInstanceOf(""",
            r"""    vmc->tmp->val_1.l, vmc->tmp->val_1.lc);""",
            r"""    vmc->setRegister(vmc->tmp->dst, vmc->tmp->val_2.z);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Array_Length(Instruction):
    def __init__(self):
        super().__init__(0x21, 1)
        self.handle = [
            r"""LOG_D_VM("|array-length v%u,v%u", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_1.u4 = (u4) (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.la);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->tmp->val_1.u4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_New_Instance(Instruction):
    def __init__(self):
        super().__init__(0x22, 1)
        self.handle = [
            r"""LOG_D_VM("|new-instance v%u,class@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_1.l = vmc->method->resolveClass(vmc->tmp->val_1.u4);""",
            r"""if (vmc->tmp->val_1.l == nullptr) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.l = (*VM_CONTEXT::env).AllocObject(vmc->tmp->val_1.lc);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_2.l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_New_Array(Instruction):
    def __init__(self):
        super().__init__(0x23, 1)
        self.handle = [
            r"""LOG_D_VM("|new-array v%u,v%u,class@%u  (%d elements)", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->getRegisterInt(vmc->tmp->src1));""",
            r"""vmc->tmp->val_2.s4 = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.s4 < 0) {""",
            r"""    JavaException::throwNegativeArraySizeException(vmc, vmc->tmp->val_2.s4);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_1.la = vmc->method->allocArray(vmc->tmp->val_2.s4, vmc->tmp->val_1.u4);""",
            r"""if (vmc->tmp->val_1.la == nullptr) {""",
            r"""    JavaException::throwRuntimeException(vmc, "error type of field... cc");""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Filled_New_Array(Instruction):
    def __init__(self):
        super().__init__(0x24, 3)
        self.handle = [
            r"""LOG_D_VM("|filled-new-array args=%u @%u {regs=%u %u}", vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->inst_A());""",
            r"""StandardInterpret::filledNewArray(vmc, false);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Filled_New_Array_Range(Instruction):
    def __init__(self):
        super().__init__(0x25, 3)
        self.handle = [
            r"""LOG_D_VM("|filled-new-array-range args=%u @%u {regs=v%u-v%u}", vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);""",
            r"""StandardInterpret::filledNewArray(vmc, true);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.src1 = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Fill_Array_Data(Instruction):
    def __init__(self):
        super().__init__(0x26, 1)
        self.handle = [
            r"""JNIEnv *env = VM_CONTEXT::env;""",
            r"""LOG_D_VM("|fill-array-data v%u +%d", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""""",
            r"""const u2 *data = vmc->arrayData(vmc->tmp->val_1.s4);""",
            r"""vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""/*""",
            r""" * Array data table format:""",
            r""" *  ushort ident = 0x0300   magic value""",
            r""" *  ushort width            width of each element in the table""",
            r""" *  uint   size             number of elements in the table""",
            r""" *  ubyte  data[size*width] table of data values (may contain a single-byte""",
            r""" *                          padding at the end)""",
            r""" *""",
            r""" * Total size is 4+(width * size + 1)/2 16-bit code units.""",
            r""" */""",
            r"""if (data[0] != kArrayDataSignature) {""",
            r"""    JavaException::throwInternalError(vmc, "bad array data magic");""",
            r"""    return;""",
            r"""}""",
            r"""u4 size = data[2] | ((u4) data[3] << 16u);""",
            r"""if (size > (*env).GetArrayLength(vmc->tmp->val_1.la)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""        vmc, (*env).GetArrayLength(vmc->tmp->val_1.la), size);""",
            r"""    return;""",
            r"""}""",
            r"""const std::string desc = VmMethod::getClassDescriptorByJClass(""",
            r"""(*env).GetObjectClass(vmc->tmp->val_1.l));""",
            r"""switch (desc[1]) {""",
            r"""    case 'I':""",
            r"""        (*env).SetIntArrayRegion(vmc->tmp->val_1.lia, 0, size, (jint *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    case 'C':""",
            r"""        (*env).SetCharArrayRegion(vmc->tmp->val_1.lca, 0, size, (jchar *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    case 'Z':""",
            r"""        (*env).SetBooleanArrayRegion(vmc->tmp->val_1.lza, 0, size, (jboolean *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    case 'B':""",
            r"""        (*env).SetByteArrayRegion(vmc->tmp->val_1.lba, 0, size, (jbyte *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    case 'F':""",
            r"""        (*env).SetFloatArrayRegion(vmc->tmp->val_1.lfa, 0, size, (jfloat *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    case 'D':""",
            r"""        (*env).SetDoubleArrayRegion(vmc->tmp->val_1.lda, 0, size, (jdouble *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    case 'S':""",
            r"""        (*env).SetShortArrayRegion(vmc->tmp->val_1.lsa, 0, size, (jshort *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    case 'J':""",
            r"""        (*env).SetLongArrayRegion(vmc->tmp->val_1.lja, 0, size, (jlong *) (data + 4));""",
            r"""        break;""",
            r"""""",
            r"""    default:""",
            r"""        LOG_E("Unknown primitive type '%c'", desc[1]);""",
            r"""        throw VMException("error type of field... cc");""",
            r"""        return;""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = (self.fetch(buf, pr, 2) << 16) | self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T11
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Throw(Instruction):
    def __init__(self):
        super().__init__(0x27, 1)
        self.handle = [
            r"""LOG_D_VM("throw v%u  (%p)", vmc->tmp->src1, vmc->getRegisterAsObject(vmc->tmp->src1));""",
            r"""vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {""",
            r"""    /* will throw a null pointer exception */""",
            r"""    LOG_E("Bad exception");""",
            r"""    return;""",
            r"""} else {""",
            r"""    /* use the requested exception */""",
            r"""    (*VM_CONTEXT::env).Throw(vmc->tmp->val_1.lt);""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Goto(Instruction):
    def __init__(self):
        super().__init__(0x28, 1)
        self.handle = [
            r"""if (vmc->tmp->val_1.s1 < 0) {""",
            r"""    LOG_D_VM("|goto -%d", -(vmc->tmp->val_1.s1));""",
            r"""} else {""",
            r"""    LOG_D_VM("|goto +%d", (vmc->tmp->val_1.s1));""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""}""",
            r"""vmc->goto_off(vmc->tmp->val_1.s1);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.val1 = self.to_signed(self.inst_aa(buf, pr), 8)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Goto16(Instruction):
    def __init__(self):
        super().__init__(0x29, 1)
        self.handle = [
            r"""if (vmc->tmp->val_1.s2 < 0) {""",
            r"""    LOG_D_VM("|goto -%d", -(vmc->tmp->val_1.s2));""",
            r"""} else {""",
            r"""    LOG_D_VM("|goto +%d", (vmc->tmp->val_1.s2));""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""}""",
            r"""vmc->goto_off(vmc->tmp->val_1.s2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.val1 = self.to_signed(self.fetch(buf, pr, 1), 16)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Goto32(Instruction):
    def __init__(self):
        super().__init__(0x2a, 1)
        self.handle = [
            r"""if (vmc->tmp->val_1.s4 < 0) {""",
            r"""    LOG_D_VM("|goto -%d", -(vmc->tmp->val_1.s4));""",
            r"""} else {""",
            r"""    LOG_D_VM("|goto +%d", (vmc->tmp->val_1.s4));""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""}""",
            r"""vmc->goto_off(vmc->tmp->val_1.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.val1 = (self.fetch(buf, pr, 2) << 16) | self.fetch(buf, pr, 1)
        self.val1 = self.to_signed(self.val1, 32)
        self.insn_type = InstructionType.T1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Packed_Switch(Instruction):
    def __init__(self):
        super().__init__(0x2b, 1)
        self.handle = [
            r"""LOG_D_VM("|packed-switch v%u +%d", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""const u2 *data = vmc->arrayData(vmc->tmp->val_1.s4);   // offset in 16-bit units""",
            r"""vmc->tmp->val_1.u4 = vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_1.s4 = StandardInterpret::handlePackedSwitch(vmc, data, vmc->tmp->val_1.u4);""",
            r"""LOG_D_VM("> branch taken (%d)", vmc->tmp->val_1.s4);""",
            r"""vmc->goto_off(vmc->tmp->val_1.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = (self.fetch(buf, pr, 2) << 16) | self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T11
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sparse_Switch(Instruction):
    def __init__(self):
        super().__init__(0x2c, 1)
        self.handle = [
            r"""LOG_D_VM("|packed-switch v%u +%d", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""const u2 *data = vmc->arrayData(vmc->tmp->val_1.s4);   // offset in 16-bit units""",
            r"""vmc->tmp->val_1.u4 = vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_1.s4 = StandardInterpret::handleSparseSwitch(vmc, data, vmc->tmp->val_1.u4);""",
            r"""LOG_D_VM("> branch taken (%d)", vmc->tmp->val_1.s4);""",
            r"""vmc->goto_off(vmc->tmp->val_1.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = (self.fetch(buf, pr, 2) << 16) | self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T11
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_CMPL_Float(Instruction):
    def __init__(self):
        super().__init__(0x2d, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|cmp%s v%u,v%u,v%u", "l-float", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.f = vmc->getRegisterFloat(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.f == vmc->tmp->val_2.f) {""",
            r"""    vmc->retVal->i = 0;""",
            r"""} else if (vmc->tmp->val_1.f > vmc->tmp->val_2.f) {""",
            r"""    vmc->retVal->i = 1;""",
            r"""} else {""",
            r"""    vmc->retVal->i = -1;""",
            r"""}""",
            r"""LOG_D_VM("+ result=%d", vmc->retVal->i);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_CMPG_Float(Instruction):
    def __init__(self):
        super().__init__(0x2e, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|cmp%s v%u,v%u,v%u", "g-float", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.f = vmc->getRegisterFloat(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.f == vmc->tmp->val_2.f) {""",
            r"""    vmc->retVal->i = 0;""",
            r"""} else if (vmc->tmp->val_1.f < vmc->tmp->val_2.f) {""",
            r"""    vmc->retVal->i = -1;""",
            r"""} else {""",
            r"""    vmc->retVal->i = 1;""",
            r"""}""",
            r"""LOG_D_VM("+ result=%d", vmc->retVal->i);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_CMPL_Double(Instruction):
    def __init__(self):
        super().__init__(0x2f, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|cmp%s v%u,v%u,v%u", "l-double", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.d = vmc->getRegisterDouble(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.d == vmc->tmp->val_2.d) {""",
            r"""    vmc->retVal->i = 0;""",
            r"""} else if (vmc->tmp->val_1.d > vmc->tmp->val_2.d) {""",
            r"""    vmc->retVal->i = 1;""",
            r"""} else {""",
            r"""    vmc->retVal->i = -1;""",
            r"""}""",
            r"""LOG_D_VM("+ result=%d", vmc->retVal->i);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_CMPG_Double(Instruction):
    def __init__(self):
        super().__init__(0x30, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|cmp%s v%u,v%u,v%u", "g-double", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.d = vmc->getRegisterDouble(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.d == vmc->tmp->val_2.d) {""",
            r"""    vmc->retVal->i = 0;""",
            r"""} else if (vmc->tmp->val_1.d < vmc->tmp->val_2.d) {""",
            r"""    vmc->retVal->i = -1;""",
            r"""} else {""",
            r"""    vmc->retVal->i = 1;""",
            r"""}""",
            r"""LOG_D_VM("+ result=%d", vmc->retVal->i);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_CMP_Long(Instruction):
    def __init__(self):
        super().__init__(0x31, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|cmp%s v%u,v%u,v%u", "-long", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.s8 = vmc->getRegisterWide(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.s8 = vmc->getRegisterWide(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.s8 > vmc->tmp->val_2.s8) {""",
            r"""    vmc->retVal->i = 1;""",
            r"""} else if (vmc->tmp->val_1.s8 < vmc->tmp->val_2.s8) {""",
            r"""    vmc->retVal->i = -1;""",
            r"""} else {""",
            r"""    vmc->retVal->i = 0;""",
            r"""}""",
            r"""LOG_D_VM("+ result=%d", vmc->retVal->i);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_EQ(Instruction):
    def __init__(self):
        super().__init__(0x32, 2)
        self.handle = [
            r"""vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.s4 == vmc->tmp->val_2.s4) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->dst;    /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u,+%d", "eq", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "eq", vmc->tmp->src1, vmc->tmp->src2);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_NE(Instruction):
    def __init__(self):
        super().__init__(0x33, 2)
        self.handle = [
            r"""vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.s4 != vmc->tmp->val_2.s4) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->dst; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u,+%d", "ne", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "ne", vmc->tmp->src1, vmc->tmp->src2);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.inst_b(buf, pr)
        self.dst = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_LT(Instruction):
    def __init__(self):
        super().__init__(0x34, 2)
        self.handle = [
            r"""vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.s4 < vmc->tmp->val_2.s4) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->dst; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u,+%d", "lt", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "lt", vmc->tmp->src1, vmc->tmp->src2);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.inst_b(buf, pr)
        self.dst = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_LE(Instruction):
    def __init__(self):
        super().__init__(0x37, 2)
        self.handle = [
            r"""vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.s4 <= vmc->tmp->val_2.s4) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->dst; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u,+%d", "le", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "le", vmc->tmp->src1, vmc->tmp->src2);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.inst_b(buf, pr)
        self.dst = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_GT(Instruction):
    def __init__(self):
        super().__init__(0x36, 2)
        self.handle = [
            r"""vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.s4 > vmc->tmp->val_2.s4) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->dst; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u,+%d", "gt", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "gt", vmc->tmp->src1, vmc->tmp->src2);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.inst_b(buf, pr)
        self.dst = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_GE(Instruction):
    def __init__(self):
        super().__init__(0x35, 2)
        self.handle = [
            r"""vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_1.s4 >= vmc->tmp->val_2.s4) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->dst; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u,+%d", "ge", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "ge", vmc->tmp->src1, vmc->tmp->src2);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.inst_b(buf, pr)
        self.dst = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_EQZ(Instruction):
    def __init__(self):
        super().__init__(0x38, 2)
        self.handle = [
            r"""if ((s4) vmc->getRegister(vmc->tmp->src1) == 0) {""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "eqz", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u", "eqz", vmc->tmp->src1);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.val1 = self.to_signed(self.fetch(buf, pr, 1), 16)
        self.insn_type = InstructionType.T11
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_NEZ(Instruction):
    def __init__(self):
        super().__init__(0x39, 2)
        self.handle = [
            r"""if ((s4) vmc->getRegister(vmc->tmp->src1) != 0) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->src2; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "nez", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u", "nez", vmc->tmp->src1);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_LTZ(Instruction):
    def __init__(self):
        super().__init__(0x3a, 2)
        self.handle = [
            r"""if ((s4) vmc->getRegister(vmc->tmp->src1) < 0) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->src2; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "ltz", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u", "ltz", vmc->tmp->src1);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_GEZ(Instruction):
    def __init__(self):
        super().__init__(0x3b, 2)
        self.handle = [
            r"""if ((s4) vmc->getRegister(vmc->tmp->src1) >= 0) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->src2; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "gez", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u", "gez", vmc->tmp->src1);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_GTZ(Instruction):
    def __init__(self):
        super().__init__(0x3c, 2)
        self.handle = [
            r"""if ((s4) vmc->getRegister(vmc->tmp->src1) > 0) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->src2; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "gtz", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u", "gtz", vmc->tmp->src1);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_IF_LEZ(Instruction):
    def __init__(self):
        super().__init__(0x3d, 2)
        self.handle = [
            r"""if ((s4) vmc->getRegister(vmc->tmp->src1) <= 0) {""",
            r"""    vmc->tmp->val_1.s4 = (s2) vmc->tmp->src2; /* sign-extended */""",
            r"""    LOG_D_VM("|if-%s v%u,v%u", "lez", vmc->tmp->src1, vmc->tmp->val_1.s4);""",
            r"""    LOG_D_VM("> branch taken");""",
            r"""    vmc->goto_off(vmc->tmp->val_1.s4);""",
            r"""} else {""",
            r"""    LOG_D_VM("|if-%s v%u", "lez", vmc->tmp->src1);""",
            r"""    vmc->pc_off(2);""",
            r"""}""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_a(buf, pr)
        self.src2 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T12
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aget(Instruction):
    def __init__(self):
        super().__init__(0x44, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lia = (jintArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lia)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lia);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""u8 buf[1];""",
            r"""(*VM_CONTEXT::env).GetIntArrayRegion(""",
            r"""vmc->tmp->val_1.lia, vmc->getRegister(vmc->tmp->src2), 1, (jint *) buf);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, *(jint *) buf);""",
            r"""LOG_D_VM("+ AGET[%u]=%d", vmc->getRegister(vmc->tmp->src2), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aget_Wide(Instruction):
    def __init__(self):
        super().__init__(0x45, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lja = (jlongArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lja)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lja);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""u8 buf[1];""",
            r"""(*VM_CONTEXT::env).GetLongArrayRegion(""",
            r"""vmc->tmp->val_1.lja, vmc->getRegister(vmc->tmp->src2), 1, (jlong *) buf);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, *(jlong *) buf);""",
            r"""LOG_D_VM("+ AGET[%u]=%ld", vmc->getRegister(vmc->tmp->src2), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aget_Object(Instruction):
    def __init__(self):
        super().__init__(0x46, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("|aget%s v%u,v%u,v%u", "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lla = (jobjectArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lla)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lla);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.l = (*VM_CONTEXT::env).GetObjectArrayElement(""",
            r"""vmc->tmp->val_1.lla, vmc->getRegister(vmc->tmp->src2));""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_2.l);""",
            r"""LOG_D_VM("+ AGET[%u]=%p", vmc->getRegister(vmc->tmp->src2), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aget_Boolean(Instruction):
    def __init__(self):
        super().__init__(0x47, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-boolean", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lza = (jbooleanArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lza)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lza);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""u8 buf[1];""",
            r"""(*VM_CONTEXT::env).GetBooleanArrayRegion(""",
            r"""vmc->tmp->val_1.lza, vmc->getRegister(vmc->tmp->src2), 1, (jboolean *) buf);""",
            r"""vmc->setRegister(vmc->tmp->dst, *(jboolean *) buf);""",
            r"""LOG_D_VM("+ AGET[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        pass

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aget_Byte(Instruction):
    def __init__(self):
        super().__init__(0x48, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lba = (jbyteArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lba)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lba);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""u8 buf[1];""",
            r"""(*VM_CONTEXT::env).GetByteArrayRegion(""",
            r"""vmc->tmp->val_1.lba, vmc->getRegister(vmc->tmp->src2), 1, (jbyte *) buf);""",
            r"""vmc->setRegister(vmc->tmp->dst, *(jbyte *) buf);""",
            r"""LOG_D_VM("+ AGET[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aget_Char(Instruction):
    def __init__(self):
        super().__init__(0x49, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lca = (jcharArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lca)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lca);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""u8 buf[1];""",
            r"""(*VM_CONTEXT::env).GetCharArrayRegion(""",
            r"""vmc->tmp->val_1.lca, vmc->getRegister(vmc->tmp->src2), 1, (jchar *) buf);""",
            r"""vmc->setRegister(vmc->tmp->dst, *(jchar *) buf);""",
            r"""LOG_D_VM("+ AGET[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aget_Short(Instruction):
    def __init__(self):
        super().__init__(0x4a, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lsa = (jshortArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lsa)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lsa);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""u8 buf[1];""",
            r"""(*VM_CONTEXT::env).GetShortArrayRegion(""",
            r"""vmc->tmp->val_1.lsa, vmc->getRegister(vmc->tmp->src2), 1, (jshort *) buf);""",
            r"""vmc->setRegister(vmc->tmp->dst, *(jshort *) buf);""",
            r"""LOG_D_VM("+ AGET[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aput(Instruction):
    def __init__(self):
        super().__init__(0x4b, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lia = (jintArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lia)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lia);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ APUT[%u]=%d", vmc->getRegister(vmc->tmp->src2), vmc->getRegisterInt(vmc->tmp->dst));""",
            r"""u8 buf[1];""",
            r"""*(jint *) buf = vmc->getRegister(vmc->tmp->dst);""",
            r"""(*VM_CONTEXT::env).SetIntArrayRegion(""",
            r"""vmc->tmp->val_1.lia, vmc->getRegister(vmc->tmp->src2), 1, (jint *) buf);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aput_Wide(Instruction):
    def __init__(self):
        super().__init__(0x4c, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lja = (jlongArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lja)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lja);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ APUT[%u]=%ld", vmc->getRegister(vmc->tmp->src2), vmc->getRegisterLong(vmc->tmp->dst));""",
            r"""u8 buf[1];""",
            r"""*(jlong *) buf = vmc->getRegisterLong(vmc->tmp->dst);""",
            r"""(*VM_CONTEXT::env).SetLongArrayRegion(""",
            r"""vmc->tmp->val_1.lja, vmc->getRegister(vmc->tmp->src2), 1, (jlong *) buf);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aput_Object(Instruction):
    def __init__(self):
        super().__init__(0x4d, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lla = (jobjectArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lla)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lla);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ APUT[%u]=%p", vmc->getRegister(vmc->tmp->src2), vmc->getRegisterAsObject(vmc->tmp->dst));""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->dst);""",
            r"""(*VM_CONTEXT::env).SetObjectArrayElement(""",
            r"""vmc->tmp->val_1.lla, vmc->getRegister(vmc->tmp->src2), vmc->tmp->val_2.l);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aput_Boolean(Instruction):
    def __init__(self):
        super().__init__(0x4e, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-boolean", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lza = (jbooleanArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lza)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lza);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ APUT[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
            r"""u8 buf[1];""",
            r"""*(jboolean *) buf = vmc->getRegister(vmc->tmp->dst);""",
            r"""(*VM_CONTEXT::env).SetBooleanArrayRegion(""",
            r"""vmc->tmp->val_1.lza, vmc->getRegister(vmc->tmp->src2), 1, (jboolean *) buf);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aput_Byte(Instruction):
    def __init__(self):
        super().__init__(0x4f, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lba = (jbyteArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lba)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lba);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ APUT[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
            r"""u8 buf[1];""",
            r"""*(jbyte *) buf = vmc->getRegister(vmc->tmp->dst);""",
            r"""(*VM_CONTEXT::env).SetByteArrayRegion(""",
            r"""vmc->tmp->val_1.lba, vmc->getRegister(vmc->tmp->src2), 1, (jbyte *) buf);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aput_Char(Instruction):
    def __init__(self):
        super().__init__(0x50, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lca = (jcharArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lca)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lca);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ APUT[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
            r"""u8 buf[1];""",
            r"""*(jchar *) buf = vmc->getRegister(vmc->tmp->dst);""",
            r"""(*VM_CONTEXT::env).SetCharArrayRegion(""",
            r"""vmc->tmp->val_1.lca, vmc->getRegister(vmc->tmp->src2), 1, (jchar *) buf);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Aput_Short(Instruction):
    def __init__(self):
        super().__init__(0x51, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */""",
            r"""LOG_D_VM("aget%s v%u,v%u,v%u", "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.lsa = (jshortArray) vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lsa)) {""",
            r"""    return;""",
            r"""}""",
            r"""vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lsa);""",
            r"""if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {""",
            r"""    JavaException::throwArrayIndexOutOfBoundsException(""",
            r"""            vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ APUT[%u]=%u", vmc->getRegister(vmc->tmp->src2), vmc->getRegister(vmc->tmp->dst));""",
            r"""u8 buf[1];""",
            r"""*(jshort *) buf = vmc->getRegister(vmc->tmp->dst);""",
            r"""(*VM_CONTEXT::env).SetShortArrayRegion(""",
            r"""vmc->tmp->val_1.lsa, vmc->getRegister(vmc->tmp->src2), 1, (jshort *) buf);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget(Instruction):
    def __init__(self):
        super().__init__(0x52, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, val.i);""",
            r"""LOG_D_VM("+ IGET '%s'=%d", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Wide(Instruction):
    def __init__(self):
        super().__init__(0x53, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, val.j);""",
            r"""LOG_D_VM("+ IGET '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Object(Instruction):
    def __init__(self):
        super().__init__(0x54, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, val.l);""",
            r"""LOG_D_VM("+ IGET '%s'=%p", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Boolean(Instruction):
    def __init__(self):
        super().__init__(0x55, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-bool", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.z);""",
            r"""LOG_D_VM("+ IGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Byte(Instruction):
    def __init__(self):
        super().__init__(0x56, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.b);""",
            r"""LOG_D_VM("+ IGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Char(Instruction):
    def __init__(self):
        super().__init__(0x57, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.c);""",
            r"""LOG_D_VM("+ IGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Short(Instruction):
    def __init__(self):
        super().__init__(0x58, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.s);""",
            r"""LOG_D_VM("+ IGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput(Instruction):
    def __init__(self):
        super().__init__(0x59, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.i = vmc->getRegisterInt(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Wide(Instruction):
    def __init__(self):
        super().__init__(0x5a, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.j = vmc->getRegisterLong(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Object(Instruction):
    def __init__(self):
        super().__init__(0x5b, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.l = vmc->getRegisterAsObject(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%p", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Boolean(Instruction):
    def __init__(self):
        super().__init__(0x5c, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-bool", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.z = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Byte(Instruction):
    def __init__(self):
        super().__init__(0x5d, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.b = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Char(Instruction):
    def __init__(self):
        super().__init__(0x5e, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.c = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Short(Instruction):
    def __init__(self):
        super().__init__(0x5f, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.s = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget(Instruction):
    def __init__(self):
        super().__init__(0x60, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-normal", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, val.i);""",
            r"""LOG_D_VM("+ SGET '%s'=%d", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Wide(Instruction):
    def __init__(self):
        super().__init__(0x61, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-wide", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, val.j);""",
            r"""LOG_D_VM("+ SGET '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Object(Instruction):
    def __init__(self):
        super().__init__(0x62, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-object", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, val.l);""",
            r"""LOG_D_VM("+ SGET '%s'=%p", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Boolean(Instruction):
    def __init__(self):
        super().__init__(0x63, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-boolean", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.z);""",
            r"""LOG_D_VM("+ SGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Byte(Instruction):
    def __init__(self):
        super().__init__(0x64, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-byte", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.b);""",
            r"""LOG_D_VM("+ SGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Char(Instruction):
    def __init__(self):
        super().__init__(0x65, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-char", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.c);""",
            r"""LOG_D_VM("+ SGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Short(Instruction):
    def __init__(self):
        super().__init__(0x66, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-short", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.s);""",
            r"""LOG_D_VM("+ SGET '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput(Instruction):
    def __init__(self):
        super().__init__(0x67, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-normal", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.i = vmc->getRegisterInt(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%d", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Wide(Instruction):
    def __init__(self):
        super().__init__(0x68, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-wide", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.j = vmc->getRegisterLong(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Object(Instruction):
    def __init__(self):
        super().__init__(0x69, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-object", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.l = vmc->getRegisterAsObject(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%p", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Boolean(Instruction):
    def __init__(self):
        super().__init__(0x6a, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-boolean", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.z = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Byte(Instruction):
    def __init__(self):
        super().__init__(0x6b, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-byte", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.b = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Char(Instruction):
    def __init__(self):
        super().__init__(0x6c, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-char", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.c = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Short(Instruction):
    def __init__(self):
        super().__init__(0x6d, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-short", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.s = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%u", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegister(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Virtual(Instruction):
    def __init__(self):
        super().__init__(0x6e, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-virtual args=%u @%u {regs=0x%08x %x}", vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);""",
            r"""""",
            r"""vmc->setState(VmMethodContextState::Method);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Super(Instruction):
    def __init__(self):
        super().__init__(0x6f, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-super args=%u @%u {regs=0x%08x %x}", vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);""",
            r"""vmc->setState(VmMethodContextState::SuperMethod);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Direct(Instruction):
    def __init__(self):
        super().__init__(0x70, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-direct args=%u @%u {regs=0x%08x %x}", vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);""",
            r"""vmc->setState(VmMethodContextState::Method);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Static(Instruction):
    def __init__(self):
        super().__init__(0x71, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-static args=%u @%u {regs=0x%08x %x}", vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);""",
            r"""vmc->setState(VmMethodContextState::StaticMethod);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Interface(Instruction):
    def __init__(self):
        super().__init__(0x72, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-interface args=%u @%u {regs=0x%08x %x}", vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);""",
            r"""vmc->setState(VmMethodContextState::Method);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Virtual_Range(Instruction):
    def __init__(self):
        super().__init__(0x74, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-virtual-range args=%u @%u {regs=v%u-v%u}", vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);""",
            r"""vmc->setState(VmMethodContextState::MethodRange);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Super_Range(Instruction):
    def __init__(self):
        super().__init__(0x75, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-super-range args=%u @%u {regs=v%u-v%u}", vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);""",
            r"""vmc->setState(VmMethodContextState::SuperMethodRange);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Direct_Range(Instruction):
    def __init__(self):
        super().__init__(0x76, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-direct-range args=%u @%u {regs=v%u-v%u}", vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);""",
            r"""vmc->setState(VmMethodContextState::MethodRange);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Static_Range(Instruction):
    def __init__(self):
        super().__init__(0x77, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-static-range args=%u @%u {regs=v%u-v%u}", vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);""",
            r"""vmc->setState(VmMethodContextState::StaticMethodRange);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Invoke_Interface_Range(Instruction):
    def __init__(self):
        super().__init__(0x78, 1)
        self.handle = [
            r"""LOG_D_VM("|invoke-interface-range args=%u @%u {regs=v%u-v%u}", vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);""",
            r"""vmc->setState(VmMethodContextState::MethodRange);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.src1 = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.dst = self.fetch(buf, pr, 2)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Neg_Int(Instruction):
    def __init__(self):
        super().__init__(0x7b, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "neg-int", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, -vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Not_Int(Instruction):
    def __init__(self):
        super().__init__(0x7c, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "not-int", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) ^ 0xffffffff);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Neg_Long(Instruction):
    def __init__(self):
        super().__init__(0x7d, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "neg-long", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, -vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Not_Long(Instruction):
    def __init__(self):
        super().__init__(0x7e, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "not-long", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) ^ 0xffffffffffffffffULL);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Neg_Float(Instruction):
    def __init__(self):
        super().__init__(0x7f, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "neg-float", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, -vmc->getRegisterFloat(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Neg_Double(Instruction):
    def __init__(self):
        super().__init__(0x80, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "neg-double", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, -vmc->getRegisterDouble(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Int2Long(Instruction):
    def __init__(self):
        super().__init__(0x81, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "int-to-long", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Int2Float(Instruction):
    def __init__(self):
        super().__init__(0x82, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "int-to-float", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Int2Double(Instruction):
    def __init__(self):
        super().__init__(0x83, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "int-to-double", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Long2Int(Instruction):
    def __init__(self):
        super().__init__(0x84, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "long-to-int", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Long2Float(Instruction):
    def __init__(self):
        super().__init__(0x85, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "long-to-float", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Long2Double(Instruction):
    def __init__(self):
        super().__init__(0x86, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "long-to-double", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Float2Int(Instruction):
    def __init__(self):
        super().__init__(0x87, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "float-to-int", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""jint min = (jint) 1 << (sizeof(jint) * 8 - 1);      // get min""",
            r"""jint max = ~min;                                    // get max""",
            r"""vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_1.f >= max) {                   /* +inf */""",
            r"""    vmc->tmp->val_2.i = max;""",
            r"""} else if (vmc->tmp->val_1.f <= min) {            /* -inf */""",
            r"""    vmc->tmp->val_2.i = min;""",
            r"""} else if (vmc->tmp->val_1.f != vmc->tmp->val_1.f) {    /* NaN */""",
            r"""    vmc->tmp->val_2.i = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.i = (jint) vmc->tmp->val_1.f;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Float2Long(Instruction):
    def __init__(self):
        super().__init__(0x88, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "float-to-long", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""jlong min = (jlong) 1 << (sizeof(jlong) * 8 - 1);       // get min""",
            r"""jlong max = ~min;                                       // get max""",
            r"""vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_1.f >= max) {                   /* +inf */""",
            r"""    vmc->tmp->val_2.j = max;""",
            r"""} else if (vmc->tmp->val_1.f <= min) {            /* -inf */""",
            r"""    vmc->tmp->val_2.j = min;""",
            r"""} else if (vmc->tmp->val_1.f != vmc->tmp->val_1.f) {    /* NaN */""",
            r"""    vmc->tmp->val_2.j = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.j = (jint) vmc->tmp->val_1.f;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Float2Double(Instruction):
    def __init__(self):
        super().__init__(0x89, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "float-to-double", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Double2Int(Instruction):
    def __init__(self):
        super().__init__(0x8a, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "double-to-int", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""jint min = (jint) 1 << (sizeof(jint) * 8 - 1);      // get min""",
            r"""jint max = ~min;                                    // get max""",
            r"""vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_1.d >= max) {                   /* +inf */""",
            r"""    vmc->tmp->val_2.i = max;""",
            r"""} else if (vmc->tmp->val_1.d <= min) {            /* -inf */""",
            r"""    vmc->tmp->val_2.i = min;""",
            r"""} else if (vmc->tmp->val_1.d != vmc->tmp->val_1.d) {    /* NaN */""",
            r"""    vmc->tmp->val_2.i = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.i = (jint) vmc->tmp->val_1.d;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Double2Long(Instruction):
    def __init__(self):
        super().__init__(0x8b, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "double-to-long", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""jlong min = (jlong) 1 << (sizeof(jlong) * 8 - 1);       // get min""",
            r"""jlong max = ~min;                                       // get max""",
            r"""vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_1.d >= max) {                   /* +inf */""",
            r"""    vmc->tmp->val_2.j = max;""",
            r"""} else if (vmc->tmp->val_1.d <= min) {            /* -inf */""",
            r"""    vmc->tmp->val_2.j = min;""",
            r"""} else if (vmc->tmp->val_1.d != vmc->tmp->val_1.d) {    /* NaN */""",
            r"""    vmc->tmp->val_2.j = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.j = (jint) vmc->tmp->val_1.d;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Double2Float(Instruction):
    def __init__(self):
        super().__init__(0x8c, 1)
        self.handle = [
            r"""LOG_D_VM("|%s v%u,v%u", "double-to-float", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Int2Byte(Instruction):
    def __init__(self):
        super().__init__(0x8d, 1)
        self.handle = [
            r"""LOG_D_VM("|int-to-%s v%u,v%u", "byte", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, (s1) vmc->getRegister(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Int2Char(Instruction):
    def __init__(self):
        super().__init__(0x8e, 1)
        self.handle = [
            r"""LOG_D_VM("|int-to-%s v%u,v%u", "char", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, (u2) vmc->getRegister(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Int2Short(Instruction):
    def __init__(self):
        super().__init__(0x8f, 1)
        self.handle = [
            r"""LOG_D_VM("|int-to-%s v%u,v%u", "short", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, (s2) vmc->getRegister(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Int(Instruction):
    def __init__(self):
        super().__init__(0x90, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) + vmc->getRegisterInt(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Int(Instruction):
    def __init__(self):
        super().__init__(0x91, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) - vmc->getRegisterInt(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Int(Instruction):
    def __init__(self):
        super().__init__(0x92, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) * vmc->getRegisterInt(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Int(Instruction):
    def __init__(self):
        super().__init__(0x93, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_2.i == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {""",
            r"""    vmc->tmp->val_2.i = vmc->tmp->val_1.i;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.i = vmc->tmp->val_1.i / vmc->tmp->val_2.i;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Int(Instruction):
    def __init__(self):
        super().__init__(0x94, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_2.i == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {""",
            r"""    vmc->tmp->val_2.i = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.i = vmc->tmp->val_1.i % vmc->tmp->val_2.i;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_And_Int(Instruction):
    def __init__(self):
        super().__init__(0x95, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) & vmc->getRegister(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Or_Int(Instruction):
    def __init__(self):
        super().__init__(0x96, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) | vmc->getRegister(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Xor_Int(Instruction):
    def __init__(self):
        super().__init__(0x97, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) ^ vmc->getRegister(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shl_Int(Instruction):
    def __init__(self):
        super().__init__(0x98, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) << (vmc->getRegister(vmc->tmp->src2) & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shr_Int(Instruction):
    def __init__(self):
        super().__init__(0x99, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) >> (vmc->getRegister(vmc->tmp->src2) & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Ushr_Int(Instruction):
    def __init__(self):
        super().__init__(0x9a, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-int v%u,v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) >> (vmc->getRegister(vmc->tmp->src2) & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Long(Instruction):
    def __init__(self):
        super().__init__(0x9b, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1) + vmc->getRegisterLong(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Long(Instruction):
    def __init__(self):
        super().__init__(0x9c, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1) - vmc->getRegisterLong(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Long(Instruction):
    def __init__(self):
        super().__init__(0x9d, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1) * vmc->getRegisterLong(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Long(Instruction):
    def __init__(self):
        super().__init__(0x9e, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_2.j == 0LL) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {""",
            r"""    vmc->tmp->val_2.j = vmc->tmp->val_1.j;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.j = vmc->tmp->val_1.j / vmc->tmp->val_2.j;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Long(Instruction):
    def __init__(self):
        super().__init__(0x9f, 1)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->src1);""",
            r"""vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src2);""",
            r"""if (vmc->tmp->val_2.j == 0LL) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {""",
            r"""    vmc->tmp->val_2.j = 0LL;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.j = vmc->tmp->val_1.j % vmc->tmp->val_2.j;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_And_Long(Instruction):
    def __init__(self):
        super().__init__(0xa0, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->src1) & vmc->getRegisterWide(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Or_Long(Instruction):
    def __init__(self):
        super().__init__(0xa1, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->src1) | vmc->getRegisterWide(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Xor_Long(Instruction):
    def __init__(self):
        super().__init__(0xa2, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->src1) ^ vmc->getRegisterWide(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shl_Long(Instruction):
    def __init__(self):
        super().__init__(0xa3, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1) << (vmc->getRegister(vmc->tmp->src2) & 0x3fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shr_Long(Instruction):
    def __init__(self):
        super().__init__(0xa4, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1) >> (vmc->getRegister(vmc->tmp->src2) & 0x3fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Ushr_Long(Instruction):
    def __init__(self):
        super().__init__(0xa5, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-long v%u,v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->src1) >> (vmc->getRegister(vmc->tmp->src2) & 0x3fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Float(Instruction):
    def __init__(self):
        super().__init__(0xa6, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-float v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->src1) + vmc->getRegisterFloat(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Float(Instruction):
    def __init__(self):
        super().__init__(0xa7, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-float v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->src1) - vmc->getRegisterFloat(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Float(Instruction):
    def __init__(self):
        super().__init__(0xa8, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-float v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->src1) * vmc->getRegisterFloat(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Float(Instruction):
    def __init__(self):
        super().__init__(0xa9, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-float v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->src1) / vmc->getRegisterFloat(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Float(Instruction):
    def __init__(self):
        super().__init__(0xaa, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-float v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, std::fmodf(vmc->getRegisterFloat(vmc->tmp->src1), vmc->getRegisterFloat(vmc->tmp->src2)));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Double(Instruction):
    def __init__(self):
        super().__init__(0xab, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-double v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->src1) + vmc->getRegisterDouble(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Double(Instruction):
    def __init__(self):
        super().__init__(0xac, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-double v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->src1) - vmc->getRegisterDouble(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Double(Instruction):
    def __init__(self):
        super().__init__(0xad, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-double v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->src1) * vmc->getRegisterDouble(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Double(Instruction):
    def __init__(self):
        super().__init__(0xae, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-double v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->src1) / vmc->getRegisterDouble(vmc->tmp->src2));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Double(Instruction):
    def __init__(self):
        super().__init__(0xaf, 2)
        self.handle = [
            r"""vmc->tmp->src2 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("|%s-double v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, std::fmod(vmc->getRegisterDouble(vmc->tmp->src1), vmc->getRegisterDouble(vmc->tmp->src2)));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb0, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->dst) + vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb1, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->dst) - vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb2, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->dst) * vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb3, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->dst);""",
            r"""vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.i == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {""",
            r"""    vmc->tmp->val_2.i = vmc->tmp->val_1.i;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.i = vmc->tmp->val_1.i / vmc->tmp->val_2.i;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb4, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->dst);""",
            r"""vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.i == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {""",
            r"""    vmc->tmp->val_2.i = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.i = vmc->tmp->val_1.i % vmc->tmp->val_2.i;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_And_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb5, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->dst) & vmc->getRegister(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Or_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb6, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->dst) * vmc->getRegister(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Xor_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb7, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->dst) ^ vmc->getRegister(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shl_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb8, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->dst) << (vmc->getRegister(vmc->tmp->src1) & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shr_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xb9, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->dst) >> (vmc->getRegister(vmc->tmp->src1) & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Ushr_Int_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xba, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int-2addr v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->dst) >> (vmc->getRegister(vmc->tmp->src1) & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xbb, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->dst) + vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xbc, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->dst) - vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xbd, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->dst) * vmc->getRegisterLong(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xbe, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->dst);""",
            r"""vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.j == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {""",
            r"""    vmc->tmp->val_2.j = vmc->tmp->val_1.j;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.j = vmc->tmp->val_1.j / vmc->tmp->val_2.j;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xbf, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->dst);""",
            r"""vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.j == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {""",
            r"""    vmc->tmp->val_2.j = 0LL;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.j = vmc->tmp->val_1.j % vmc->tmp->val_2.j;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_And_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc0, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->dst) & vmc->getRegisterWide(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Or_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc1, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->dst) | vmc->getRegisterWide(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Xor_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc2, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->dst) ^ vmc->getRegisterWide(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shl_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc3, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->dst) << (vmc->getRegister(vmc->tmp->src1) & 0x3fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shr_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc4, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->dst) << (vmc->getRegister(vmc->tmp->src1) & 0x3fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Ushr_Long_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc5, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-long-2addr v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, vmc->getRegisterWide(vmc->tmp->dst) << (vmc->getRegister(vmc->tmp->src1) & 0x3fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Float_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc6, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-float-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->dst) + vmc->getRegisterFloat(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Float_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc7, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-float-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->dst) - vmc->getRegisterFloat(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Float_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc8, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-float-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->dst) * vmc->getRegisterFloat(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Float_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xc9, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-float-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->dst) / vmc->getRegisterFloat(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Float_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xca, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-float-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterFloat(vmc->tmp->dst, std::fmodf(vmc->getRegisterFloat(vmc->tmp->dst), vmc->getRegisterFloat(vmc->tmp->src1)));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Double_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xcb, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-double-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->dst) + vmc->getRegisterDouble(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sub_Double_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xcc, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-double-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->dst) - vmc->getRegisterDouble(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Double_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xcd, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-double-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->dst) * vmc->getRegisterDouble(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Double_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xce, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-double-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->dst) / vmc->getRegisterDouble(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Double_2Addr(Instruction):
    def __init__(self):
        super().__init__(0xcf, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-double-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);""",
            r"""vmc->setRegisterDouble(vmc->tmp->dst, std::fmod(vmc->getRegisterDouble(vmc->tmp->dst), vmc->getRegisterDouble(vmc->tmp->src1)));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd0, 2)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) + vmc->tmp->val_1.s2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_RSub_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd1, 2)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "rsub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_1.s2 - vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd2, 2)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) * vmc->tmp->val_1.s2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd3, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s2);""",
            r"""vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.s2 == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s2 != -1) {""",
            r"""    vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 / vmc->tmp->val_2.s4;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd4, 1)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s2);""",
            r"""vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.s2 == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s2 != -1) {""",
            r"""    vmc->tmp->val_2.s4 = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 % vmc->tmp->val_2.s4;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_And_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd5, 2)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) & vmc->tmp->val_1.u2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Or_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd6, 2)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) | vmc->tmp->val_1.u2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Xor_Int_Lit16(Instruction):
    def __init__(self):
        super().__init__(0xd7, 2)
        self.handle = [
            r"""LOG_D_VM("|%s-int/lit16 v%u,v%u,#%d", "xor", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) ^ vmc->tmp->val_1.u2);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Add_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xd8, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) + vmc->tmp->val_1.s1);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_RSub_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xd9, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "rsub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_1.s1 - vmc->getRegisterInt(vmc->tmp->src1));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Mul_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xda, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) * vmc->tmp->val_1.s1);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Div_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xdb, 1)
        self.handle = [
            r"""vmc->tmp->val_2.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s1);""",
            r"""vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.s1 == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s1 != -1) {""",
            r"""    vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 / vmc->tmp->val_2.s4;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Rem_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xdc, 1)
        self.handle = [
            r"""vmc->tmp->val_2.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s1);""",
            r"""vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);""",
            r"""if (vmc->tmp->val_2.s1 == 0) {""",
            r"""    JavaException::throwArithmeticException(vmc, "divide by zero");""",
            r"""    return;""",
            r"""}""",
            r"""if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s1 != -1) {""",
            r"""    vmc->tmp->val_2.s4 = 0;""",
            r"""} else {""",
            r"""    vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 % vmc->tmp->val_2.s4;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_And_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xdd, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) & vmc->tmp->val_1.u1);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Or_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xde, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) | vmc->tmp->val_1.u1);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Xor_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xdf, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) ^ vmc->tmp->val_1.u1);""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shl_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xe0, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "shl", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) << (vmc->tmp->val_1.u1 & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Shr_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xe1, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "shr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1) >> (vmc->tmp->val_1.u1 & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Ushr_Int_Lit8(Instruction):
    def __init__(self):
        super().__init__(0xe2, 2)
        self.handle = [
            r"""vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;""",
            r"""vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;""",
            r"""LOG_D_VM("%s-int/lit8 v%u,v%u,#%d", "ushr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);""",
            r"""vmc->setRegister(vmc->tmp->dst, vmc->getRegister(vmc->tmp->src1) >> (vmc->tmp->val_1.u1 & 0x1fu));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.src1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xe3, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-normal-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterInt(vmc->tmp->dst, val.i);""",
            r"""LOG_D_VM("+ IGET '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xe4, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-normal-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.i = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%d", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xe5, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-normal-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegister(vmc->tmp->dst, val.i);""",
            r"""LOG_D_VM("+ SGET '%s'=%d", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xe6, 1)
        self.handle = [
            r"""LOG_D_VM("sput%s v%u,sfield@%u", "-normal-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.i = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%d", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterInt(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Object_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xe7, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-object-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, val.l);""",
            r"""LOG_D_VM("+ IGET '%s'=0x%08lx", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), (u8) vmc->getRegisterWide(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iget_Wide_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xe8, 1)
        self.handle = [
            r"""LOG_D_VM("|iget%s v%u,v%u,field@%u", "-wide-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterWide(vmc->tmp->dst, val.j);""",
            r"""LOG_D_VM("+ IGET '%s'=%ldx", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Wide_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xe9, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-wide-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.j = vmc->getRegisterLong(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Wide_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xea, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-wide-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterLong(vmc->tmp->dst, val.j);""",
            r"""LOG_D_VM("+ SGET '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Wide_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xeb, 1)
        self.handle = [
            r"""LOG_D_VM("|sput%s v%u,sfield@%u", "-wide-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.j = vmc->getRegisterLong(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%ld", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterLong(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Iput_Object_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xfc, 1)
        self.handle = [
            r"""LOG_D_VM("|iput%s v%u,v%u,field@%u", "-object-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);""",
            r"""vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);""",
            r"""if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {""",
            r"""    return;""",
            r"""}""",
            r"""RegValue val{};""",
            r"""val.l = vmc->getRegisterAsObject(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ IPUT '%s'=%p", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_a(buf, pr)
        self.src1 = self.inst_b(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.T1D1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sget_Object_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xfd, 1)
        self.handle = [
            r"""LOG_D_VM("|sget%s v%u,sfield@%u", "-object-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""vmc->setRegisterAsObject(vmc->tmp->dst, val.l);""",
            r"""LOG_D_VM("+ SGET '%s'=%p", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class ST_CH_Sput_Object_Volatile(Instruction):
    def __init__(self):
        super().__init__(0xfe, 1)
        self.handle = [
            r"""LOG_D_VM("|sput%s v%u,sfield@%u", "-object-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);""",
            r"""RegValue val{};""",
            r"""val.i = vmc->getRegister(vmc->tmp->dst);""",
            r"""if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {""",
            r"""    JavaException::throwJavaException(vmc);""",
            r"""    return;""",
            r"""}""",
            r"""LOG_D_VM("+ SPUT '%s'=%p", vmc->method->resolveFieldName(vmc->tmp->val_1.u4), vmc->getRegisterAsObject(vmc->tmp->dst));""",
        ]

    def parse(self, buf: bytes, pr: Pointer):
        self.dst = self.inst_aa(buf, pr)
        self.val1 = self.fetch(buf, pr, 1)
        self.insn_type = InstructionType.TD1
        pr.add(self.std_len * 2)

    @Debugger.print_all_fields
    def __repr__(self):
        pass


class RegisterInterpretBuilder:
    def __init__(self, dex: DexFile, key_method: List[EncodedMethod]):
        self.log = logging.getLogger(RegisterInterpretBuilder.__name__)
        self.__dex = dex
        self.__key_method = key_method

    @property
    def key_method(self):
        return self.__key_method

    @Log.log_function
    def build(self):
        self.__count_std_bytecode()
        return self

    @Log.log_function
    def __count_std_bytecode(self) -> Dict[int, int]:
        counter = [0 for _ in range(256)]
        error_insns = 0
        correct_insns = 0
        std_insns_len = []
        for em in self.__key_method:
            code = em.code
            self.log.debug("insns: ")
            for pc in range(len(code.insns) // 2):
                self.log.debug('insns[%d]: 0x%02x%02x',
                               pc, code.insns[pc * 2 + 1], code.insns[pc * 2])

            cur = 0
            cur_counter = counter.copy()
            while cur * 2 < len(code.insns):
                opcode = code.insns[cur * 2]
                if opcode in std_insns_len:
                    cur += std_insns_len[opcode]
                else:
                    error_insns += 1
                    break
                cur_counter[opcode] += 1
            else:
                correct_insns += 1
                counter = cur_counter
        for idx, data in enumerate(counter):
            if idx in std_insns_len:
                self.log.debug("opcode[0x%02x]: %d", idx, data)
        self.log.info('correct: %d, error: %d, percent: %.2f',
                      correct_insns, error_insns, error_insns / (correct_insns + error_insns))
        ret: Dict[int, int] = {}
        for k in std_insns_len:
            if counter[k]:
                ret[k] = counter[k]
        return ret

    @Log.log_function
    def __to_byte_h(self) -> bytes:
        file = FileBuilder()

        return bytes(file.build(), encoding='utf-8')

    @Log.log_function
    def __to_byte_cpp(self) -> bytes:
        file = FileBuilder()

        return bytes(file.build(), encoding='utf-8')

    @Debugger.print_all_fields
    def __repr__(self):
        pass
