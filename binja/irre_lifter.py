"""
IRRE2 LLIL Lifting Helpers for Binary Ninja

This module contains helper functions for lifting IRRE2 instructions
to Binary Ninja's Low Level Intermediate Language (LLIL).
"""

from binaryninja import LowLevelILFunction, LowLevelILLabel, Architecture
from typing import Optional

from .irre_types import (
    Opcode,
    Reg,
    Format,
    REG_NAMES,
    is_branch_instruction,
    is_conditional_branch,
    is_call_instruction,
    is_return_instruction,
)
from .irre_decoder import DecodedInstruction


class IRRE2Lifter:
    """LLIL lifting logic for IRRE2 instructions"""

    INSTRUCTION_LENGTH = 4  # IRRE2 instructions are always 4 bytes

    def __init__(self, il: LowLevelILFunction, addr: int):
        self.il = il
        self.addr = addr

    def lift_instruction(self, decoded: DecodedInstruction) -> int:
        """
        Lift a decoded instruction to LLIL

        Returns instruction length in bytes
        """
        opcode = decoded.opcode
        operands = decoded.operands

        try:
            # Arithmetic operations
            if opcode == Opcode.ADD:
                return self._lift_add(operands)
            elif opcode == Opcode.SUB:
                return self._lift_sub(operands)
            elif opcode == Opcode.MUL:
                return self._lift_mul(operands)
            elif opcode == Opcode.DIV:
                return self._lift_div(operands)
            elif opcode == Opcode.MOD:
                return self._lift_mod(operands)

            # Logical operations
            elif opcode == Opcode.AND:
                return self._lift_and(operands)
            elif opcode == Opcode.ORR:
                return self._lift_orr(operands)
            elif opcode == Opcode.XOR:
                return self._lift_xor(operands)
            elif opcode == Opcode.NOT:
                return self._lift_not(operands)

            # Shift operations
            elif opcode == Opcode.LSH:
                return self._lift_lsh(operands)
            elif opcode == Opcode.ASH:
                return self._lift_ash(operands)

            # Data movement
            elif opcode == Opcode.SET:
                return self._lift_set(operands)
            elif opcode == Opcode.MOV:
                return self._lift_mov(operands)
            elif opcode == Opcode.SUP:
                return self._lift_sup(operands)
            elif opcode == Opcode.SXT:
                return self._lift_sxt(operands)

            # Memory operations
            elif opcode == Opcode.LDW:
                return self._lift_ldw(operands)
            elif opcode == Opcode.STW:
                return self._lift_stw(operands)
            elif opcode == Opcode.LDB:
                return self._lift_ldb(operands)
            elif opcode == Opcode.STB:
                return self._lift_stb(operands)

            # Control flow
            elif opcode == Opcode.JMI:
                return self._lift_jmi(operands)
            elif opcode == Opcode.JMP:
                return self._lift_jmp(operands)
            elif opcode == Opcode.BVE:
                return self._lift_bve(operands)
            elif opcode == Opcode.BVN:
                return self._lift_bvn(operands)
            elif opcode == Opcode.CAL:
                return self._lift_cal(operands)
            elif opcode == Opcode.RET:
                return self._lift_ret(operands)

            # Comparison operations
            elif opcode == Opcode.TCU:
                return self._lift_tcu(operands)
            elif opcode == Opcode.TCS:
                return self._lift_tcs(operands)
            elif opcode == Opcode.SEQ:
                return self._lift_seq(operands)

            # Special operations
            elif opcode == Opcode.NOP:
                return self._lift_nop(operands)
            elif opcode == Opcode.HLT:
                return self._lift_hlt(operands)
            elif opcode == Opcode.SIA:
                return self._lift_sia(operands)
            elif opcode == Opcode.INT:
                return self._lift_int(operands)
            elif opcode == Opcode.SND:
                return self._lift_snd(operands)
            else:
                # Unknown instruction
                self.il.append(self.il.unimplemented())
                return self.INSTRUCTION_LENGTH

        except Exception:
            # If lifting fails, mark as unimplemented
            self.il.append(self.il.unimplemented())
            return self.INSTRUCTION_LENGTH

    def _get_reg_name(self, reg: Reg) -> str:
        """Get register name string"""
        return REG_NAMES[reg]

    def _lift_add(self, operands) -> int:
        """Lift ADD instruction: rA = rB + rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.add(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_sub(self, operands) -> int:
        """Lift SUB instruction: rA = rB - rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.sub(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_mul(self, operands) -> int:
        """Lift MUL instruction: rA = rB * rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.mult(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_div(self, operands) -> int:
        """Lift DIV instruction: rA = rB / rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.div_unsigned(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_mod(self, operands) -> int:
        """Lift MOD instruction: rA = rB % rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.mod_unsigned(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_and(self, operands) -> int:
        """Lift AND instruction: rA = rB & rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.and_expr(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_orr(self, operands) -> int:
        """Lift ORR instruction: rA = rB | rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.or_expr(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_xor(self, operands) -> int:
        """Lift XOR instruction: rA = rB ^ rC"""
        reg_a, reg_b, reg_c = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.xor_expr(
                4,
                self.il.reg(4, self._get_reg_name(reg_b)),
                self.il.reg(4, self._get_reg_name(reg_c)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_not(self, operands) -> int:
        """Lift NOT instruction: rA = ~rB"""
        reg_a, reg_b = operands
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.not_expr(4, self.il.reg(4, self._get_reg_name(reg_b))),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_lsh(self, operands) -> int:
        """Lift LSH instruction: rA = rB << rC (logical shift, bidirectional)"""
        reg_a, reg_b, reg_c = operands
        reg_a_name = self._get_reg_name(reg_a)
        reg_b_val = self.il.reg(4, self._get_reg_name(reg_b))
        reg_c_val = self.il.reg(4, self._get_reg_name(reg_c))

        # Check if rC >= 0 (shift left) or < 0 (shift right)
        condition = self.il.compare_signed_greater_equal(
            4, reg_c_val, self.il.const(4, 0)
        )

        true_label = LowLevelILLabel()
        false_label = LowLevelILLabel()
        done_label = LowLevelILLabel()

        # Conditional branch
        self.il.append(self.il.if_expr(condition, true_label, false_label))

        # rC >= 0: shift left
        self.il.mark_label(true_label)
        left_result = self.il.shift_left(4, reg_b_val, reg_c_val)
        self.il.append(self.il.set_reg(4, reg_a_name, left_result))
        self.il.append(self.il.goto(done_label))

        # rC < 0: logical shift right by -rC
        self.il.mark_label(false_label)
        neg_c = self.il.neg(4, reg_c_val)
        right_result = self.il.logical_shift_right(4, reg_b_val, neg_c)
        self.il.append(self.il.set_reg(4, reg_a_name, right_result))

        self.il.mark_label(done_label)
        return self.INSTRUCTION_LENGTH

    def _lift_ash(self, operands) -> int:
        """Lift ASH instruction: rA = rB << rC (arithmetic shift, bidirectional)"""
        reg_a, reg_b, reg_c = operands
        reg_a_name = self._get_reg_name(reg_a)
        reg_b_val = self.il.reg(4, self._get_reg_name(reg_b))
        reg_c_val = self.il.reg(4, self._get_reg_name(reg_c))

        # Check if rC >= 0 (shift left) or < 0 (arithmetic shift right)
        condition = self.il.compare_signed_greater_equal(
            4, reg_c_val, self.il.const(4, 0)
        )

        true_label = LowLevelILLabel()
        false_label = LowLevelILLabel()
        done_label = LowLevelILLabel()

        # Conditional branch
        self.il.append(self.il.if_expr(condition, true_label, false_label))

        # rC >= 0: shift left
        self.il.mark_label(true_label)
        left_result = self.il.shift_left(4, reg_b_val, reg_c_val)
        self.il.append(self.il.set_reg(4, reg_a_name, left_result))
        self.il.append(self.il.goto(done_label))

        # rC < 0: arithmetic shift right by -rC
        self.il.mark_label(false_label)
        neg_c = self.il.neg(4, reg_c_val)
        right_result = self.il.arith_shift_right(4, reg_b_val, neg_c)
        self.il.append(self.il.set_reg(4, reg_a_name, right_result))

        self.il.mark_label(done_label)
        return self.INSTRUCTION_LENGTH

    def _lift_set(self, operands) -> int:
        """Lift SET instruction: rA = immediate"""
        reg_a, imm = operands
        expr = self.il.set_reg(4, self._get_reg_name(reg_a), self.il.const(4, imm))
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_mov(self, operands) -> int:
        """Lift MOV instruction: rA = rB"""
        reg_a, reg_b = operands
        expr = self.il.set_reg(
            4, self._get_reg_name(reg_a), self.il.reg(4, self._get_reg_name(reg_b))
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_sup(self, operands) -> int:
        """Lift SUP instruction: set upper 16 bits of rA"""
        reg_a, imm = operands
        # rA = (rA & 0xFFFF) | (imm << 16)
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.or_expr(
                4,
                self.il.and_expr(
                    4,
                    self.il.reg(4, self._get_reg_name(reg_a)),
                    self.il.const(4, 0xFFFF),
                ),
                self.il.shift_left(4, self.il.const(4, imm), self.il.const(4, 16)),
            ),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_sxt(self, operands) -> int:
        """Lift SXT instruction: sign extend lower 16 bits of rB into rA"""
        reg_a, reg_b = operands
        reg_b_val = self.il.reg(4, self._get_reg_name(reg_b))

        # Sign extend lower 16 bits: shift left to move sign bit to top, then arithmetic right
        shifted_up = self.il.shift_left(4, reg_b_val, self.il.const(4, 16))
        result = self.il.arith_shift_right(4, shifted_up, self.il.const(4, 16))

        expr = self.il.set_reg(4, self._get_reg_name(reg_a), result)
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_ldw(self, operands) -> int:
        """Lift LDW instruction: rA = memory[rB + offset]"""
        reg_a, reg_b, offset = operands
        addr = self.il.add(
            4, self.il.reg(4, self._get_reg_name(reg_b)), self.il.const(4, offset)
        )
        expr = self.il.set_reg(4, self._get_reg_name(reg_a), self.il.load(4, addr))
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_stw(self, operands) -> int:
        """Lift STW instruction: memory[rB + offset] = rA"""
        reg_a, reg_b, offset = operands
        addr = self.il.add(
            4, self.il.reg(4, self._get_reg_name(reg_b)), self.il.const(4, offset)
        )
        expr = self.il.store(4, addr, self.il.reg(4, self._get_reg_name(reg_a)))
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_ldb(self, operands) -> int:
        """Lift LDB instruction: rA = memory[rB + offset] (byte)"""
        reg_a, reg_b, offset = operands
        addr = self.il.add(
            4, self.il.reg(4, self._get_reg_name(reg_b)), self.il.const(4, offset)
        )
        expr = self.il.set_reg(
            4, self._get_reg_name(reg_a), self.il.zero_extend(4, self.il.load(1, addr))
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_stb(self, operands) -> int:
        """Lift STB instruction: memory[rB + offset] = rA (byte)"""
        reg_a, reg_b, offset = operands
        addr = self.il.add(
            4, self.il.reg(4, self._get_reg_name(reg_b)), self.il.const(4, offset)
        )
        expr = self.il.store(1, addr, self.il.reg(4, self._get_reg_name(reg_a)))
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_jmi(self, operands) -> int:
        """Lift JMI instruction: jump to immediate address"""
        target_addr = operands[0]

        # Try to get or create a label for the target address
        target_label = self.il.get_label_for_address(Architecture["irre2"], target_addr)
        if target_label is None:
            self.il.add_label_for_address(Architecture["irre2"], target_addr)
            target_label = self.il.get_label_for_address(
                Architecture["irre2"], target_addr
            )

        if target_label:
            # Use goto for labeled addresses
            self.il.append(self.il.goto(target_label))
        else:
            # Fallback to direct jump
            self.il.append(self.il.jump(self.il.const_pointer(4, target_addr)))

        return self.INSTRUCTION_LENGTH

    def _lift_jmp(self, operands) -> int:
        """Lift JMP instruction: jump to register value"""
        reg = operands[0]
        expr = self.il.jump(self.il.reg(4, self._get_reg_name(reg)))
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_bve(self, operands) -> int:
        """Lift BVE instruction: branch if rB == immediate to address in rA"""
        reg_a, reg_b, imm = operands
        condition = self.il.compare_equal(
            4, self.il.reg(4, self._get_reg_name(reg_b)), self.il.const(4, imm)
        )

        true_label = LowLevelILLabel()
        false_label = LowLevelILLabel()

        self.il.append(self.il.if_expr(condition, true_label, false_label))

        self.il.mark_label(true_label)
        self.il.append(self.il.jump(self.il.reg(4, self._get_reg_name(reg_a))))

        self.il.mark_label(false_label)
        return self.INSTRUCTION_LENGTH

    def _lift_bvn(self, operands) -> int:
        """Lift BVN instruction: branch if rB != immediate to address in rA"""
        reg_a, reg_b, imm = operands
        condition = self.il.compare_not_equal(
            4, self.il.reg(4, self._get_reg_name(reg_b)), self.il.const(4, imm)
        )

        true_label = LowLevelILLabel()
        false_label = LowLevelILLabel()

        self.il.append(self.il.if_expr(condition, true_label, false_label))

        self.il.mark_label(true_label)
        self.il.append(self.il.jump(self.il.reg(4, self._get_reg_name(reg_a))))

        self.il.mark_label(false_label)
        return self.INSTRUCTION_LENGTH

    def _lift_cal(self, operands) -> int:
        """Lift CAL instruction: call to address in register"""
        reg = operands[0]
        expr = self.il.call(self.il.reg(4, self._get_reg_name(reg)))
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_ret(self, operands) -> int:
        """Lift RET instruction: return (pc = lr; lr = 0)"""
        # IRRE2 spec: "Return: pc = lr; lr = 0"
        # Save lr value before clearing it
        lr_value = self.il.reg(4, "lr")

        # Clear lr register and return in sequence
        self.il.append(self.il.set_reg(4, "lr", self.il.const(4, 0)))
        self.il.append(self.il.ret(lr_value))
        return self.INSTRUCTION_LENGTH

    def _lift_tcu(self, operands) -> int:
        """Lift TCU instruction: test compare unsigned - returns sign(-1, 0, +1)"""
        reg_a, reg_b, reg_c = operands
        reg_a_name = self._get_reg_name(reg_a)
        reg_b_val = self.il.reg(4, self._get_reg_name(reg_b))
        reg_c_val = self.il.reg(4, self._get_reg_name(reg_c))

        # Use arithmetic on boolean comparison results
        # greater_than returns 1 if rB > rC, 0 otherwise
        # less_than returns 1 if rB < rC, 0 otherwise
        # greater_than - less_than gives us -1, 0, or +1
        greater_than = self.il.compare_unsigned_greater_than(4, reg_b_val, reg_c_val)
        less_than = self.il.compare_unsigned_less_than(4, reg_b_val, reg_c_val)
        result = self.il.sub(4, greater_than, less_than)

        self.il.append(self.il.set_reg(4, reg_a_name, result))
        return self.INSTRUCTION_LENGTH

    def _lift_tcs(self, operands) -> int:
        """Lift TCS instruction: test compare signed - returns sign(-1, 0, +1)"""
        reg_a, reg_b, reg_c = operands
        reg_a_name = self._get_reg_name(reg_a)
        reg_b_val = self.il.reg(4, self._get_reg_name(reg_b))
        reg_c_val = self.il.reg(4, self._get_reg_name(reg_c))

        # Use arithmetic on boolean comparison results
        # greater_than returns 1 if rB > rC, 0 otherwise
        # less_than returns 1 if rB < rC, 0 otherwise
        # greater_than - less_than gives us -1, 0, or +1
        greater_than = self.il.compare_signed_greater_than(4, reg_b_val, reg_c_val)
        less_than = self.il.compare_signed_less_than(4, reg_b_val, reg_c_val)
        result = self.il.sub(4, greater_than, less_than)

        self.il.append(self.il.set_reg(4, reg_a_name, result))
        return self.INSTRUCTION_LENGTH

    def _lift_seq(self, operands) -> int:
        """Lift SEQ instruction: set if equal"""
        reg_a, reg_b, imm = operands
        condition = self.il.compare_equal(
            4, self.il.reg(4, self._get_reg_name(reg_b)), self.il.const(4, imm)
        )
        expr = self.il.set_reg(4, self._get_reg_name(reg_a), condition)
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_sia(self, operands) -> int:
        """Lift SIA instruction: shift and add"""
        reg_a, v0, v1 = operands
        # rA = rA + (v0 << v1)
        shifted = self.il.shift_left(4, self.il.const(4, v0), self.il.const(4, v1))
        expr = self.il.set_reg(
            4,
            self._get_reg_name(reg_a),
            self.il.add(4, self.il.reg(4, self._get_reg_name(reg_a)), shifted),
        )
        self.il.append(expr)
        return self.INSTRUCTION_LENGTH

    def _lift_nop(self, operands) -> int:
        """Lift NOP instruction: no operation"""
        self.il.append(self.il.nop())
        return self.INSTRUCTION_LENGTH

    def _lift_hlt(self, operands) -> int:
        """Lift HLT instruction: halt"""
        self.il.append(self.il.no_ret())
        return self.INSTRUCTION_LENGTH

    def _lift_int(self, operands) -> int:
        """Lift INT instruction: interrupt"""
        code = operands[0]
        # Use system call with interrupt code
        self.il.append(self.il.system_call(self.il.const(4, code)))
        return self.INSTRUCTION_LENGTH

    def _lift_snd(self, operands) -> int:
        """Lift SND instruction: send to device"""
        reg_a, reg_b, reg_c = operands
        # Mark as unimplemented for now - this is a device-specific instruction
        self.il.append(self.il.unimplemented())
        return self.INSTRUCTION_LENGTH
