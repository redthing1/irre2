"""
IRRE2 Architecture Plugin for Binary Ninja

This module implements the IRRE2 architecture support for Binary Ninja,
including instruction decoding, disassembly, and LLIL lifting.
"""

from binaryninja import (
    Architecture,
    RegisterInfo,
    InstructionInfo,
    InstructionTextToken,
    InstructionTextTokenType,
    BranchType,
    LowLevelILFunction,
    Platform,
    CallingConvention,
    log_error,
)
from typing import Optional, List, Tuple

from .irre_types import (
    Reg,
    Opcode,
    Format,
    REG_NAMES,
    INSTRUCTION_SIZE,
    ADDRESS_SIZE,
    INSTRUCTION_ALIGNMENT,
    is_branch_instruction,
    is_conditional_branch,
    is_call_instruction,
    is_return_instruction,
)
from .irre_decoder import InstructionDecoder, DecodedInstruction
from .irre_lifter import IRRE2Lifter


class IRRE2Architecture(Architecture):
    """IRRE2 Architecture implementation for Binary Ninja"""

    name = "irre2"
    address_size = ADDRESS_SIZE
    default_int_size = ADDRESS_SIZE
    instr_alignment = INSTRUCTION_ALIGNMENT
    max_instr_length = INSTRUCTION_SIZE

    # sp and lr
    stack_pointer = "sp"
    link_reg = "lr"

    # reg defs
    regs = {name: RegisterInfo(name, 4) for name in REG_NAMES.values()}

    def _decode_instruction(self, data: bytes) -> Optional[DecodedInstruction]:
        """Helper to decode instruction with length checking"""
        return (
            InstructionDecoder.decode_bytes(data, 0)
            if len(data) >= INSTRUCTION_SIZE
            else None
        )

    def get_instruction_info(self, data: bytes, addr: int) -> Optional[InstructionInfo]:
        """Get instruction information for control flow analysis"""
        decoded = self._decode_instruction(data)
        if not decoded:
            return None

        info = InstructionInfo()
        info.length = INSTRUCTION_SIZE

        if is_branch_instruction(decoded.opcode):
            self._add_branch_info(info, decoded, addr)

        return info

    # branch info
    _BRANCH_HANDLERS = {
        Opcode.JMI: lambda info, ops, addr: info.add_branch(
            BranchType.UnconditionalBranch, ops[0]
        ),
        Opcode.JMP: lambda info, ops, addr: info.add_branch(BranchType.IndirectBranch),
        Opcode.BVE: lambda info, ops, addr: [
            info.add_branch(BranchType.IndirectBranch),
            info.add_branch(BranchType.FalseBranch, addr + INSTRUCTION_SIZE),
        ],
        Opcode.BVN: lambda info, ops, addr: [
            info.add_branch(BranchType.IndirectBranch),
            info.add_branch(BranchType.FalseBranch, addr + INSTRUCTION_SIZE),
        ],
        Opcode.CAL: lambda info, ops, addr: info.add_branch(BranchType.CallDestination),
        Opcode.RET: lambda info, ops, addr: info.add_branch(BranchType.FunctionReturn),
    }

    def _add_branch_info(
        self, info: InstructionInfo, decoded: DecodedInstruction, addr: int
    ):
        """Add branch information to InstructionInfo"""
        handler = self._BRANCH_HANDLERS.get(decoded.opcode)
        if handler:
            handler(info, decoded.operands, addr)

    def get_instruction_text(
        self, data: bytes, addr: int
    ) -> Optional[Tuple[List[InstructionTextToken], int]]:
        """Generate disassembly text for instruction"""
        decoded = self._decode_instruction(data)
        if not decoded:
            return None

        tokens = self._generate_instruction_tokens(decoded)
        return tokens, INSTRUCTION_SIZE

    def _generate_instruction_tokens(
        self, decoded: DecodedInstruction
    ) -> List[InstructionTextToken]:
        """Generate instruction text tokens for syntax highlighting"""
        tokens = []

        # mnem
        tokens.append(
            InstructionTextToken(
                InstructionTextTokenType.InstructionToken, decoded.mnemonic
            )
        )

        # operands
        if decoded.operands:
            tokens.append(InstructionTextToken(InstructionTextTokenType.TextToken, " "))

            for i, operand in enumerate(decoded.operands):
                if i > 0:
                    tokens.append(
                        InstructionTextToken(
                            InstructionTextTokenType.OperandSeparatorToken, ", "
                        )
                    )

                tokens.extend(self._format_operand(operand, decoded.format, i))

        return tokens

    def _format_operand(
        self, operand, fmt: Format, operand_index: int
    ) -> List[InstructionTextToken]:
        """Format a single operand as tokens"""
        if isinstance(operand, Reg):
            return [
                InstructionTextToken(
                    InstructionTextTokenType.RegisterToken, REG_NAMES[operand]
                )
            ]

        elif isinstance(operand, int):
            # check if this is an address operand (24-bit/16-bit)
            is_address = fmt == Format.OP_IMM24 or (
                fmt == Format.OP_REG_IMM16 and operand_index == 1
            )

            if is_address:
                token_type = (
                    InstructionTextTokenType.PossibleAddressToken
                    if operand > 0x100
                    else InstructionTextTokenType.IntegerToken
                )
                return [InstructionTextToken(token_type, f"${operand:x}")]
            else:
                return [
                    InstructionTextToken(
                        InstructionTextTokenType.IntegerToken, str(operand)
                    )
                ]

        else:
            return [
                InstructionTextToken(InstructionTextTokenType.TextToken, str(operand))
            ]

    def get_instruction_low_level_il(
        self, data: bytes, addr: int, il: LowLevelILFunction
    ) -> Optional[int]:
        """Lift instruction to LLIL"""
        decoded = self._decode_instruction(data)
        if not decoded:
            return None

        lifter = IRRE2Lifter(il, addr)
        return lifter.lift_instruction(decoded)


IRRE2Architecture.register()
