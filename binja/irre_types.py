"""
IRRE2 Architecture Types and Constants for Binary Ninja

This module contains all the type definitions, enums, and constants
needed for the IRRE2 architecture plugin, ported from the C++ implementation.
"""

from enum import IntEnum
from typing import Dict, Optional, Tuple, NamedTuple


class Format(IntEnum):
    """IRRE instruction formats from the specification"""

    OP = 0  # op - no arguments
    OP_REG = 1  # op rA - one register
    OP_IMM24 = 2  # op v0 - 24-bit immediate
    OP_REG_IMM16 = 3  # op rA v0 - register + 16-bit immediate
    OP_REG_REG = 4  # op rA rB - two registers
    OP_REG_REG_IMM8 = 5  # op rA rB v0 - two registers + 8-bit immediate
    OP_REG_IMM8X2 = 6  # op rA v0 v1 - register + two 8-bit immediates
    OP_REG_REG_REG = 7  # op rA rB rC - three registers


class Reg(IntEnum):
    """IRRE register set (37 total registers)"""

    # General purpose registers r0-r31
    R0 = 0x00
    R1 = 0x01
    R2 = 0x02
    R3 = 0x03
    R4 = 0x04
    R5 = 0x05
    R6 = 0x06
    R7 = 0x07
    R8 = 0x08
    R9 = 0x09
    R10 = 0x0A
    R11 = 0x0B
    R12 = 0x0C
    R13 = 0x0D
    R14 = 0x0E
    R15 = 0x0F
    R16 = 0x10
    R17 = 0x11
    R18 = 0x12
    R19 = 0x13
    R20 = 0x14
    R21 = 0x15
    R22 = 0x16
    R23 = 0x17
    R24 = 0x18
    R25 = 0x19
    R26 = 0x1A
    R27 = 0x1B
    R28 = 0x1C
    R29 = 0x1D
    R30 = 0x1E
    R31 = 0x1F

    # Special registers
    PC = 0x20  # Program counter
    LR = 0x21  # Link register (return address)
    AD = 0x22  # Address temporary
    AT = 0x23  # Arithmetic temporary
    SP = 0x24  # Stack pointer


class Opcode(IntEnum):
    """IRRE instruction opcodes"""

    # Arithmetic and logical operations
    NOP = 0x00  # No operation
    ADD = 0x01  # Unsigned addition
    SUB = 0x02  # Unsigned subtraction
    AND = 0x03  # Logical and
    ORR = 0x04  # Logical or
    XOR = 0x05  # Logical xor
    NOT = 0x06  # Logical not
    LSH = 0x07  # Logical shift
    ASH = 0x08  # Arithmetic shift
    TCU = 0x09  # Test compare unsigned
    TCS = 0x0A  # Test compare signed

    # Data movement
    SET = 0x0B  # Set register to immediate
    MOV = 0x0C  # Move register to register

    # Memory operations
    LDW = 0x0D  # Load word
    STW = 0x0E  # Store word
    LDB = 0x0F  # Load byte
    STB = 0x10  # Store byte

    # Control flow - unconditional
    JMI = 0x20  # Jump immediate
    JMP = 0x21  # Jump register

    # Control flow - conditional
    BVE = 0x24  # Branch if equal
    BVN = 0x25  # Branch if not equal

    # Function calls
    CAL = 0x2A  # Call
    RET = 0x2B  # Return

    # Extended arithmetic
    MUL = 0x30  # Multiply
    DIV = 0x31  # Divide
    MOD = 0x32  # Modulus

    # Advanced operations
    SIA = 0x40  # Shift and add
    SUP = 0x41  # Set upper
    SXT = 0x42  # Sign extend
    SEQ = 0x43  # Set if equal

    # System operations
    INT = 0xF0  # Interrupt
    SND = 0xFD  # Send to device
    HLT = 0xFF  # Halt


class OpcodeInfo(NamedTuple):
    """Opcode metadata for instruction decoding"""

    mnemonic: str
    format: Format


# Opcode information table
OPCODE_INFO: Dict[Opcode, OpcodeInfo] = {
    Opcode.NOP: OpcodeInfo("nop", Format.OP),
    Opcode.ADD: OpcodeInfo("add", Format.OP_REG_REG_REG),
    Opcode.SUB: OpcodeInfo("sub", Format.OP_REG_REG_REG),
    Opcode.AND: OpcodeInfo("and", Format.OP_REG_REG_REG),
    Opcode.ORR: OpcodeInfo("orr", Format.OP_REG_REG_REG),
    Opcode.XOR: OpcodeInfo("xor", Format.OP_REG_REG_REG),
    Opcode.NOT: OpcodeInfo("not", Format.OP_REG_REG),
    Opcode.LSH: OpcodeInfo("lsh", Format.OP_REG_REG_REG),
    Opcode.ASH: OpcodeInfo("ash", Format.OP_REG_REG_REG),
    Opcode.TCU: OpcodeInfo("tcu", Format.OP_REG_REG_REG),
    Opcode.TCS: OpcodeInfo("tcs", Format.OP_REG_REG_REG),
    Opcode.SET: OpcodeInfo("set", Format.OP_REG_IMM16),
    Opcode.MOV: OpcodeInfo("mov", Format.OP_REG_REG),
    Opcode.LDW: OpcodeInfo("ldw", Format.OP_REG_REG_IMM8),
    Opcode.STW: OpcodeInfo("stw", Format.OP_REG_REG_IMM8),
    Opcode.LDB: OpcodeInfo("ldb", Format.OP_REG_REG_IMM8),
    Opcode.STB: OpcodeInfo("stb", Format.OP_REG_REG_IMM8),
    Opcode.JMI: OpcodeInfo("jmi", Format.OP_IMM24),
    Opcode.JMP: OpcodeInfo("jmp", Format.OP_REG),
    Opcode.BVE: OpcodeInfo("bve", Format.OP_REG_REG_IMM8),
    Opcode.BVN: OpcodeInfo("bvn", Format.OP_REG_REG_IMM8),
    Opcode.CAL: OpcodeInfo("cal", Format.OP_REG),
    Opcode.RET: OpcodeInfo("ret", Format.OP),
    Opcode.MUL: OpcodeInfo("mul", Format.OP_REG_REG_REG),
    Opcode.DIV: OpcodeInfo("div", Format.OP_REG_REG_REG),
    Opcode.MOD: OpcodeInfo("mod", Format.OP_REG_REG_REG),
    Opcode.SIA: OpcodeInfo("sia", Format.OP_REG_IMM8X2),
    Opcode.SUP: OpcodeInfo("sup", Format.OP_REG_IMM16),
    Opcode.SXT: OpcodeInfo("sxt", Format.OP_REG_REG),
    Opcode.SEQ: OpcodeInfo("seq", Format.OP_REG_REG_IMM8),
    Opcode.INT: OpcodeInfo("int", Format.OP_IMM24),
    Opcode.SND: OpcodeInfo("snd", Format.OP_REG_REG_REG),
    Opcode.HLT: OpcodeInfo("hlt", Format.OP),
}

# Register name mapping - generated programmatically
REG_NAMES: Dict[Reg, str] = {
    **{Reg(i): f"r{i}" for i in range(32)},  # r0-r31
    Reg.PC: "pc",
    Reg.LR: "lr",
    Reg.AD: "ad",
    Reg.AT: "at",
    Reg.SP: "sp",
}

# Architecture constants
WORD_SIZE = 4  # 32-bit words
ADDRESS_SIZE = 4  # 32-bit addresses
INSTRUCTION_SIZE = 4  # Fixed 32-bit instructions
INSTRUCTION_ALIGNMENT = 4  # Instructions are word-aligned


def is_gpr(reg: Reg) -> bool:
    """Check if register is a general-purpose register"""
    return reg <= Reg.R31


def is_special(reg: Reg) -> bool:
    """Check if register is a special register"""
    return Reg.PC <= reg <= Reg.SP


def get_opcode_info(opcode: Opcode) -> Optional[OpcodeInfo]:
    """Get opcode information"""
    return OPCODE_INFO.get(opcode)


def get_reg_name(reg: Reg) -> str:
    """Get register name"""
    return REG_NAMES.get(reg, "???")


def is_valid_reg(reg_val: int) -> bool:
    """Check if register value is valid"""
    return 0 <= reg_val <= Reg.SP


# Instruction classification sets
BRANCH_INSTRUCTIONS = frozenset(
    {Opcode.JMI, Opcode.JMP, Opcode.BVE, Opcode.BVN, Opcode.CAL, Opcode.RET}
)

CONDITIONAL_BRANCHES = frozenset({Opcode.BVE, Opcode.BVN})


def is_branch_instruction(opcode: Opcode) -> bool:
    """Check if instruction is a branch/jump"""
    return opcode in BRANCH_INSTRUCTIONS


def is_conditional_branch(opcode: Opcode) -> bool:
    """Check if instruction is a conditional branch"""
    return opcode in CONDITIONAL_BRANCHES


def is_call_instruction(opcode: Opcode) -> bool:
    """Check if instruction is a call"""
    return opcode == Opcode.CAL


def is_return_instruction(opcode: Opcode) -> bool:
    """Check if instruction is a return"""
    return opcode == Opcode.RET
