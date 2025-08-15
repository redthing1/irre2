#pragma once

#include "../arch/instruction.hpp"
#include "../arch/types.hpp"
#include "memory.hpp"
#include "state.hpp"
#include "observer.hpp"
#include <variant>
#include <vector>
#include <memory>

namespace irre::emu {

// visitor class that executes irre instructions
class execution_visitor {
public:
  execution_visitor(memory& mem, vm_state& state) : mem_(mem), state_(state) {}

  // set observers for memory operation notifications
  void set_observers(const std::vector<std::shared_ptr<execution_observer>>* observers) { observers_ = observers; }

  // main visitor entry point
  void operator()(const instruction& inst) { std::visit(*this, inst); }

  // no-argument instructions
  void operator()(const inst_op& inst) {
    switch (inst.op) {
    case opcode::nop:
      // do nothing
      break;

    case opcode::ret:
      // return: pc = lr; lr = 0
      state_.registers.set_pc(state_.registers.lr());
      state_.registers.set_lr(0);
      break;

    case opcode::hlt:
      state_.set_state(execution_state::halted);
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

  // single register instructions
  void operator()(const inst_op_reg& inst) {
    switch (inst.op) {
    case opcode::jmp:
      // jump to address in register
      state_.registers.set_pc(state_.registers.read(inst.a));
      break;

    case opcode::cal:
      // call: lr = pc + 4; pc = rA
      state_.registers.set_lr(state_.registers.pc() + 4);
      state_.registers.set_pc(state_.registers.read(inst.a));
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

  // 24-bit immediate instructions
  void operator()(const inst_op_imm24& inst) {
    switch (inst.op) {
    case opcode::jmi:
      // jump immediate
      state_.registers.set_pc(inst.addr);
      break;

    case opcode::int_:
      // interrupt with code
      state_.interrupt(inst.addr);
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

  // register + 16-bit immediate instructions
  void operator()(const inst_op_reg_imm16& inst) {
    switch (inst.op) {
    case opcode::set:
      // set register to immediate value
      state_.registers.write(inst.a, inst.imm);
      break;

    case opcode::sup:
      // set upper 16 bits: rA = (rA & 0xFFFF) | (imm << 16)
      {
        word current = state_.registers.read(inst.a);
        word new_value = (current & 0xFFFF) | (static_cast<word>(inst.imm) << 16);
        state_.registers.write(inst.a, new_value);
      }
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

  // two register instructions
  void operator()(const inst_op_reg_reg& inst) {
    switch (inst.op) {
    case opcode::mov:
      // move register: rA = rB
      state_.registers.write(inst.a, state_.registers.read(inst.b));
      break;

    case opcode::not_:
      // bitwise not: rA = ~rB
      state_.registers.write(inst.a, ~state_.registers.read(inst.b));
      break;

    case opcode::sxt:
      // sign extend: rA = sign_extend(rB)
      {
        word value = state_.registers.read(inst.b);
        // assume we're sign extending from 16 bits for now
        if (value & 0x8000) {
          value |= 0xFFFF0000; // extend sign bit
        } else {
          value &= 0x0000FFFF; // clear upper bits
        }
        state_.registers.write(inst.a, value);
      }
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

  // two registers + 8-bit immediate instructions
  void operator()(const inst_op_reg_reg_imm8& inst) {
    switch (inst.op) {
    case opcode::ldw:
      // load word: rA = memory[rB + offset]
      {
        address addr = state_.registers.read(inst.b) + static_cast<signed_word>(static_cast<int8_t>(inst.offset));
        try {
          word value = mem_.read_word(addr);
          notify_memory_read(addr, value);
          state_.registers.write(inst.a, value);
        } catch (const std::out_of_range&) {
          state_.error(runtime_error::invalid_memory_access);
        }
      }
      break;

    case opcode::stw:
      // store word: memory[rB + offset] = rA
      {
        address addr = state_.registers.read(inst.b) + static_cast<signed_word>(static_cast<int8_t>(inst.offset));
        word value = state_.registers.read(inst.a);
        try {
          mem_.write_word(addr, value);
          notify_memory_write(addr, value);
        } catch (const std::out_of_range&) {
          state_.error(runtime_error::invalid_memory_access);
        }
      }
      break;

    case opcode::ldb:
      // load byte: rA = memory[rB + offset] (zero-extended)
      {
        address addr = state_.registers.read(inst.b) + static_cast<signed_word>(static_cast<int8_t>(inst.offset));
        try {
          byte byte_value = mem_.read_byte(addr);
          word value = static_cast<word>(byte_value);
          notify_memory_read(addr, value);
          state_.registers.write(inst.a, value);
        } catch (const std::out_of_range&) {
          state_.error(runtime_error::invalid_memory_access);
        }
      }
      break;

    case opcode::stb:
      // store byte: memory[rB + offset] = rA (lower 8 bits)
      {
        address addr = state_.registers.read(inst.b) + static_cast<signed_word>(static_cast<int8_t>(inst.offset));
        word reg_value = state_.registers.read(inst.a);
        byte byte_value = static_cast<byte>(reg_value & 0xFF);
        try {
          mem_.write_byte(addr, byte_value);
          notify_memory_write(addr, static_cast<word>(byte_value));
        } catch (const std::out_of_range&) {
          state_.error(runtime_error::invalid_memory_access);
        }
      }
      break;

    case opcode::bve:
      // branch if equal: if rB == offset then pc = rA
      if (state_.registers.read(inst.b) == inst.offset) {
        state_.registers.set_pc(state_.registers.read(inst.a));
      }
      break;

    case opcode::bvn:
      // branch if not equal: if rB != offset then pc = rA
      if (state_.registers.read(inst.b) != inst.offset) {
        state_.registers.set_pc(state_.registers.read(inst.a));
      }
      break;

    case opcode::seq:
      // set if equal: rA = (rB == offset) ? 1 : 0
      state_.registers.write(inst.a, (state_.registers.read(inst.b) == inst.offset) ? 1 : 0);
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

  // register + two 8-bit immediate instructions
  void operator()(const inst_op_reg_imm8x2& inst) {
    switch (inst.op) {
    case opcode::sia:
      // shift and add: rA = rA + (v0 << v1)
      {
        word current = state_.registers.read(inst.a);
        word shifted = static_cast<word>(inst.v0) << inst.v1;
        state_.registers.write(inst.a, current + shifted);
      }
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

  // three register instructions
  void operator()(const inst_op_reg_reg_reg& inst) {
    word b_val = state_.registers.read(inst.b);
    word c_val = state_.registers.read(inst.c);

    switch (inst.op) {
    case opcode::add:
      state_.registers.write(inst.a, b_val + c_val);
      break;

    case opcode::sub:
      state_.registers.write(inst.a, b_val - c_val);
      break;

    case opcode::mul:
      state_.registers.write(inst.a, b_val * c_val);
      break;

    case opcode::div:
      if (c_val == 0) {
        state_.error(runtime_error::division_by_zero);
      } else {
        state_.registers.write(inst.a, b_val / c_val);
      }
      break;

    case opcode::mod:
      if (c_val == 0) {
        state_.error(runtime_error::division_by_zero);
      } else {
        state_.registers.write(inst.a, b_val % c_val);
      }
      break;

    case opcode::and_:
      state_.registers.write(inst.a, b_val & c_val);
      break;

    case opcode::orr:
      state_.registers.write(inst.a, b_val | c_val);
      break;

    case opcode::xor_:
      state_.registers.write(inst.a, b_val ^ c_val);
      break;

    case opcode::lsh:
      // logical shift: positive = left, negative = right
      {
        signed_word shift = static_cast<signed_word>(c_val);
        if (shift < -32 || shift > 32) {
          state_.error(runtime_error::invalid_instruction);
          break;
        }
        if (shift >= 0) {
          state_.registers.write(inst.a, b_val << shift);
        } else {
          state_.registers.write(inst.a, b_val >> (-shift));
        }
      }
      break;

    case opcode::ash:
      // arithmetic shift: preserves sign on right shift
      {
        signed_word shift = static_cast<signed_word>(c_val);
        if (shift < -32 || shift > 32) {
          state_.error(runtime_error::invalid_instruction);
          break;
        }
        signed_word signed_b = static_cast<signed_word>(b_val);
        if (shift >= 0) {
          state_.registers.write(inst.a, static_cast<word>(signed_b << shift));
        } else {
          state_.registers.write(inst.a, static_cast<word>(signed_b >> (-shift)));
        }
      }
      break;

    case opcode::tcu:
      // test compare unsigned: result = sign(b - c)
      if (b_val < c_val) {
        state_.registers.write(inst.a, static_cast<word>(-1));
      } else if (b_val > c_val) {
        state_.registers.write(inst.a, 1);
      } else {
        state_.registers.write(inst.a, 0);
      }
      break;

    case opcode::tcs:
      // test compare signed: result = sign(b - c)
      {
        signed_word signed_b = static_cast<signed_word>(b_val);
        signed_word signed_c = static_cast<signed_word>(c_val);
        if (signed_b < signed_c) {
          state_.registers.write(inst.a, static_cast<word>(-1));
        } else if (signed_b > signed_c) {
          state_.registers.write(inst.a, 1);
        } else {
          state_.registers.write(inst.a, 0);
        }
      }
      break;

    case opcode::snd:
      // send to device: send command rB to device rA with argument rC; result in rC
      {
        word device_id = state_.registers.read(inst.a);
        word command = state_.registers.read(inst.b);
        word argument = state_.registers.read(inst.c);
        word result = state_.device_access(device_id, command, argument);
        state_.registers.write(inst.c, result);
      }
      break;

    default:
      state_.error(runtime_error::invalid_instruction);
    }
  }

private:
  memory& mem_;
  vm_state& state_;
  const std::vector<std::shared_ptr<execution_observer>>* observers_ = nullptr;

  // notify observers of memory operations
  void notify_memory_read(address addr, word value) {
    if (observers_) {
      for (auto& observer : *observers_) {
        observer->on_memory_read(addr, value);
      }
    }
  }

  void notify_memory_write(address addr, word value) {
    if (observers_) {
      for (auto& observer : *observers_) {
        observer->on_memory_write(addr, value);
      }
    }
  }
};

} // namespace irre::emu