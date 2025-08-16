"""
IRRE2 Instruction Decoder for Binary Ninja

This module handles decoding of IRRE2 instructions from raw bytes,
ported from the C++ codec implementation.
"""

import struct
from typing import Optional, NamedTuple
from .irre_types import (
    Opcode,
    Reg,
    Format,
    get_opcode_info,
    get_reg_name,
    is_valid_reg,
)


class DecodedInstruction(NamedTuple):
    """Decoded instruction data"""

    opcode: Opcode
    format: Format
    mnemonic: str
    operands: tuple  # Operands depend on format


class InstructionDecoder:
    """IRRE2 instruction decoder"""

    # Operand decoders for each format
    _DECODERS = {
        Format.OP: lambda w, a1, a2, a3: (),
        Format.OP_REG: lambda w, a1, a2, a3: (Reg(a1),) if is_valid_reg(a1) else None,
        Format.OP_IMM24: lambda w, a1, a2, a3: (w & 0xFFFFFF,),
        Format.OP_REG_IMM16: lambda w, a1, a2, a3: (
            (Reg(a1), w & 0xFFFF) if is_valid_reg(a1) else None
        ),
        Format.OP_REG_REG: lambda w, a1, a2, a3: (
            (Reg(a1), Reg(a2)) if all(is_valid_reg(r) for r in [a1, a2]) else None
        ),
        Format.OP_REG_REG_IMM8: lambda w, a1, a2, a3: (
            (Reg(a1), Reg(a2), a3) if all(is_valid_reg(r) for r in [a1, a2]) else None
        ),
        Format.OP_REG_IMM8X2: lambda w, a1, a2, a3: (
            (Reg(a1), a2, a3) if is_valid_reg(a1) else None
        ),
        Format.OP_REG_REG_REG: lambda w, a1, a2, a3: (
            (Reg(a1), Reg(a2), Reg(a3))
            if all(is_valid_reg(r) for r in [a1, a2, a3])
            else None
        ),
    }

    @staticmethod
    def decode_bytes(data: bytes, offset: int = 0) -> Optional[DecodedInstruction]:
        """Decode instruction from byte array (little-endian)"""
        if len(data) < offset + 4:
            return None

        try:
            word = struct.unpack("<I", data[offset : offset + 4])[0]
            return InstructionDecoder.decode_word(word)
        except struct.error:
            return None

    @staticmethod
    def decode_word(word: int) -> Optional[DecodedInstruction]:
        """Decode instruction from 32-bit word"""
        try:
            # Extract opcode and validate
            opcode_val = (word >> 24) & 0xFF
            opcode = Opcode(opcode_val)
        except ValueError:
            return None

        # Get opcode info
        opcode_info = get_opcode_info(opcode)
        if not opcode_info:
            return None

        # Extract operand fields
        a1, a2, a3 = (word >> 16) & 0xFF, (word >> 8) & 0xFF, word & 0xFF

        # Decode operands using lookup table
        decoder = InstructionDecoder._DECODERS.get(opcode_info.format)
        if not decoder:
            return None

        operands = decoder(word, a1, a2, a3)
        if operands is None:
            return None

        return DecodedInstruction(
            opcode=opcode,
            format=opcode_info.format,
            mnemonic=opcode_info.mnemonic,
            operands=operands,
        )

    # Format functions for each instruction format
    _FORMATTERS = {
        Format.OP: lambda ops: [],
        Format.OP_REG: lambda ops: [get_reg_name(ops[0])],
        Format.OP_IMM24: lambda ops: [f"${ops[0]:x}"],
        Format.OP_REG_IMM16: lambda ops: [get_reg_name(ops[0]), f"${ops[1]:x}"],
        Format.OP_REG_REG: lambda ops: [get_reg_name(ops[0]), get_reg_name(ops[1])],
        Format.OP_REG_REG_IMM8: lambda ops: [
            get_reg_name(ops[0]),
            get_reg_name(ops[1]),
            str(ops[2]),
        ],
        Format.OP_REG_IMM8X2: lambda ops: [
            get_reg_name(ops[0]),
            str(ops[1]),
            str(ops[2]),
        ],
        Format.OP_REG_REG_REG: lambda ops: [
            get_reg_name(ops[0]),
            get_reg_name(ops[1]),
            get_reg_name(ops[2]),
        ],
    }

    @staticmethod
    def format_instruction(decoded: DecodedInstruction) -> str:
        """Format decoded instruction as assembly string"""
        if not decoded:
            return "???"

        formatter = InstructionDecoder._FORMATTERS.get(decoded.format)
        if not formatter:
            return "???"

        operand_parts = formatter(decoded.operands)
        if operand_parts:
            return f"{decoded.mnemonic} {' '.join(operand_parts)}"
        else:
            return decoded.mnemonic

    @staticmethod
    def get_instruction_length(data: bytes, offset: int = 0) -> int:
        """Get instruction length (always 4 for IRRE2)"""
        return 4 if len(data) >= offset + 4 else 0

    @staticmethod
    def is_valid_instruction(data: bytes, offset: int = 0) -> bool:
        """Check if bytes represent a valid instruction"""
        return InstructionDecoder.decode_bytes(data, offset) is not None
