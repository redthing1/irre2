#pragma once

#include "types.hpp"
#include <variant>
#include <tuple>

namespace irre {

// individual instruction format structs; each knows how to encode/decode itself

struct inst_op {
  opcode op;

  static inst_op decode(word w) { return {static_cast<opcode>((w >> 24) & 0xff)}; }

  word encode() const { return static_cast<word>(op) << 24; }
};

struct inst_op_reg {
  opcode op;
  reg a;

  static inst_op_reg decode(word w) {
    return {static_cast<opcode>((w >> 24) & 0xff), static_cast<reg>((w >> 16) & 0xff)};
  }

  word encode() const { return (static_cast<word>(op) << 24) | (static_cast<word>(a) << 16); }
};

struct inst_op_imm24 {
  opcode op;
  uint32_t addr; // 24-bit address

  static inst_op_imm24 decode(word w) {
    return {
        static_cast<opcode>((w >> 24) & 0xff),
        w & 0xffffff // extract lower 24 bits
    };
  }

  word encode() const {
    return (static_cast<word>(op) << 24) | (addr & 0xffffff); // mask to 24 bits
  }
};

struct inst_op_reg_imm16 {
  opcode op;
  reg a;
  uint16_t imm;

  static inst_op_reg_imm16 decode(word w) {
    return {
        static_cast<opcode>((w >> 24) & 0xff), static_cast<reg>((w >> 16) & 0xff),
        static_cast<uint16_t>(w & 0xffff) // lower 16 bits
    };
  }

  word encode() const { return (static_cast<word>(op) << 24) | (static_cast<word>(a) << 16) | static_cast<word>(imm); }
};

struct inst_op_reg_reg {
  opcode op;
  reg a, b;

  static inst_op_reg_reg decode(word w) {
    return {
        static_cast<opcode>((w >> 24) & 0xff), static_cast<reg>((w >> 16) & 0xff), static_cast<reg>((w >> 8) & 0xff)
    };
  }

  word encode() const {
    return (static_cast<word>(op) << 24) | (static_cast<word>(a) << 16) | (static_cast<word>(b) << 8);
  }
};

struct inst_op_reg_reg_imm8 {
  opcode op;
  reg a, b;
  uint8_t offset;

  static inst_op_reg_reg_imm8 decode(word w) {
    return {
        static_cast<opcode>((w >> 24) & 0xff), static_cast<reg>((w >> 16) & 0xff), static_cast<reg>((w >> 8) & 0xff),
        static_cast<uint8_t>(w & 0xff)
    };
  }

  word encode() const {
    return (static_cast<word>(op) << 24) | (static_cast<word>(a) << 16) | (static_cast<word>(b) << 8) |
           static_cast<word>(offset);
  }
};

struct inst_op_reg_imm8x2 {
  opcode op;
  reg a;
  uint8_t v0, v1;

  static inst_op_reg_imm8x2 decode(word w) {
    return {
        static_cast<opcode>((w >> 24) & 0xff), static_cast<reg>((w >> 16) & 0xff),
        static_cast<uint8_t>((w >> 8) & 0xff), static_cast<uint8_t>(w & 0xff)
    };
  }

  word encode() const {
    return (static_cast<word>(op) << 24) | (static_cast<word>(a) << 16) | (static_cast<word>(v0) << 8) |
           static_cast<word>(v1);
  }
};

struct inst_op_reg_reg_reg {
  opcode op;
  reg a, b, c;

  static inst_op_reg_reg_reg decode(word w) {
    return {
        static_cast<opcode>((w >> 24) & 0xff), static_cast<reg>((w >> 16) & 0xff), static_cast<reg>((w >> 8) & 0xff),
        static_cast<reg>(w & 0xff)
    };
  }

  word encode() const {
    return (static_cast<word>(op) << 24) | (static_cast<word>(a) << 16) | (static_cast<word>(b) << 8) |
           static_cast<word>(c);
  }
};

// unified instruction type using variant
using instruction = std::variant<
    inst_op, inst_op_reg, inst_op_imm24, inst_op_reg_imm16, inst_op_reg_reg, inst_op_reg_reg_imm8, inst_op_reg_imm8x2,
    inst_op_reg_reg_reg>;

// helper to get opcode from any instruction variant
inline opcode get_opcode(const instruction& inst) {
  return std::visit([](const auto& i) { return i.op; }, inst);
}

// helper to get format from any instruction variant
inline format get_format(const instruction& inst) { return get_format(get_opcode(inst)); }

// convenience constructors for each format
namespace make {
inline instruction op(opcode op) { return inst_op{op}; }

inline instruction op_reg(opcode op, reg a) { return inst_op_reg{op, a}; }

inline instruction op_imm24(opcode op, uint32_t addr) { return inst_op_imm24{op, addr}; }

inline instruction op_reg_imm16(opcode op, reg a, uint16_t imm) { return inst_op_reg_imm16{op, a, imm}; }

inline instruction op_reg_reg(opcode op, reg a, reg b) { return inst_op_reg_reg{op, a, b}; }

inline instruction op_reg_reg_imm8(opcode op, reg a, reg b, uint8_t offset) {
  return inst_op_reg_reg_imm8{op, a, b, offset};
}

inline instruction op_reg_imm8x2(opcode op, reg a, uint8_t v0, uint8_t v1) { return inst_op_reg_imm8x2{op, a, v0, v1}; }

inline instruction op_reg_reg_reg(opcode op, reg a, reg b, reg c) { return inst_op_reg_reg_reg{op, a, b, c}; }

// convenience constructors for common instructions
inline instruction nop() { return op(opcode::nop); }
inline instruction hlt() { return op(opcode::hlt); }
inline instruction ret() { return op(opcode::ret); }

inline instruction add(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::add, a, b, c); }
inline instruction sub(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::sub, a, b, c); }
inline instruction mul(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::mul, a, b, c); }
inline instruction div(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::div, a, b, c); }
inline instruction mod(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::mod, a, b, c); }

inline instruction mov(reg a, reg b) { return op_reg_reg(opcode::mov, a, b); }
inline instruction set(reg a, uint16_t imm) { return op_reg_imm16(opcode::set, a, imm); }

inline instruction ldw(reg a, reg b, uint8_t offset) { return op_reg_reg_imm8(opcode::ldw, a, b, offset); }
inline instruction stw(reg a, reg b, uint8_t offset) { return op_reg_reg_imm8(opcode::stw, a, b, offset); }
inline instruction ldb(reg a, reg b, uint8_t offset) { return op_reg_reg_imm8(opcode::ldb, a, b, offset); }
inline instruction stb(reg a, reg b, uint8_t offset) { return op_reg_reg_imm8(opcode::stb, a, b, offset); }

inline instruction jmp(reg a) { return op_reg(opcode::jmp, a); }
inline instruction jmi(uint32_t addr) { return op_imm24(opcode::jmi, addr); }
inline instruction cal(reg a) { return op_reg(opcode::cal, a); }

inline instruction bve(reg a, reg b, uint8_t v) { return op_reg_reg_imm8(opcode::bve, a, b, v); }
inline instruction bvn(reg a, reg b, uint8_t v) { return op_reg_reg_imm8(opcode::bvn, a, b, v); }

inline instruction int_(uint32_t code) { return op_imm24(opcode::int_, code); }
inline instruction snd(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::snd, a, b, c); }

inline instruction sia(reg a, uint8_t v0, uint8_t v1) { return op_reg_imm8x2(opcode::sia, a, v0, v1); }
inline instruction sup(reg a, uint16_t v0) { return op_reg_imm16(opcode::sup, a, v0); }
inline instruction sxt(reg a, reg b) { return op_reg_reg(opcode::sxt, a, b); }
inline instruction seq(reg a, reg b, uint8_t v0) { return op_reg_reg_imm8(opcode::seq, a, b, v0); }

// logical operations
inline instruction and_(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::and_, a, b, c); }
inline instruction orr(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::orr, a, b, c); }
inline instruction xor_(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::xor_, a, b, c); }
inline instruction not_(reg a, reg b) { return op_reg_reg(opcode::not_, a, b); }

// shift operations
inline instruction lsh(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::lsh, a, b, c); }
inline instruction ash(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::ash, a, b, c); }

// comparison operations
inline instruction tcu(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::tcu, a, b, c); }
inline instruction tcs(reg a, reg b, reg c) { return op_reg_reg_reg(opcode::tcs, a, b, c); }
} // namespace make
} // namespace irre