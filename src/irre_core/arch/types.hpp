#pragma once

#include <cstdint>

namespace irre {
// basic types for irre architecture
using byte = uint8_t;
using word = uint32_t;
using address = uint32_t;
using signed_word = int32_t;

// instruction formats from the irre specification
enum class format {
  op,              // op - no arguments
  op_reg,          // op rA - one register
  op_imm24,        // op v0 - 24-bit immediate
  op_reg_imm16,    // op rA v0 - register + 16-bit immediate
  op_reg_reg,      // op rA rB - two registers
  op_reg_reg_imm8, // op rA rB v0 - two registers + 8-bit immediate
  op_reg_imm8x2,   // op rA v0 v1 - register + two 8-bit immediates
  op_reg_reg_reg   // op rA rB rC - three registers
};

// irre register set (37 total registers)
enum class reg : uint8_t {
  // general purpose registers r0-r31
  r0 = 0x00,
  r1,
  r2,
  r3,
  r4,
  r5,
  r6,
  r7,
  r8,
  r9,
  r10,
  r11,
  r12,
  r13,
  r14,
  r15,
  r16,
  r17,
  r18,
  r19,
  r20,
  r21,
  r22,
  r23,
  r24,
  r25,
  r26,
  r27,
  r28,
  r29,
  r30,
  r31,

  // special registers
  pc = 0x20, // program counter
  lr = 0x21, // link register (return address)
  ad = 0x22, // address temporary
  at = 0x23, // arithmetic temporary
  sp = 0x24  // stack pointer
};

// register utility functions
constexpr bool is_gpr(reg r) { return static_cast<uint8_t>(r) <= 0x1f; }

constexpr bool is_special(reg r) { return static_cast<uint8_t>(r) >= 0x20 && static_cast<uint8_t>(r) <= 0x24; }

constexpr const char* reg_name(reg r) {
  switch (r) {
  case reg::r0:
    return "r0";
  case reg::r1:
    return "r1";
  case reg::r2:
    return "r2";
  case reg::r3:
    return "r3";
  case reg::r4:
    return "r4";
  case reg::r5:
    return "r5";
  case reg::r6:
    return "r6";
  case reg::r7:
    return "r7";
  case reg::r8:
    return "r8";
  case reg::r9:
    return "r9";
  case reg::r10:
    return "r10";
  case reg::r11:
    return "r11";
  case reg::r12:
    return "r12";
  case reg::r13:
    return "r13";
  case reg::r14:
    return "r14";
  case reg::r15:
    return "r15";
  case reg::r16:
    return "r16";
  case reg::r17:
    return "r17";
  case reg::r18:
    return "r18";
  case reg::r19:
    return "r19";
  case reg::r20:
    return "r20";
  case reg::r21:
    return "r21";
  case reg::r22:
    return "r22";
  case reg::r23:
    return "r23";
  case reg::r24:
    return "r24";
  case reg::r25:
    return "r25";
  case reg::r26:
    return "r26";
  case reg::r27:
    return "r27";
  case reg::r28:
    return "r28";
  case reg::r29:
    return "r29";
  case reg::r30:
    return "r30";
  case reg::r31:
    return "r31";
  case reg::pc:
    return "pc";
  case reg::lr:
    return "lr";
  case reg::ad:
    return "ad";
  case reg::at:
    return "at";
  case reg::sp:
    return "sp";
  default:
    return "???";
  }
}

// irre instruction opcodes
enum class opcode : uint8_t {
  // arithmetic and logical operations
  nop = 0x00,  // no operation
  add = 0x01,  // unsigned addition
  sub = 0x02,  // unsigned subtraction
  and_ = 0x03, // logical and
  orr = 0x04,  // logical or
  xor_ = 0x05, // logical xor
  not_ = 0x06, // logical not
  lsh = 0x07,  // logical shift
  ash = 0x08,  // arithmetic shift
  tcu = 0x09,  // test compare unsigned
  tcs = 0x0a,  // test compare signed

  // data movement
  set = 0x0b, // set register to immediate
  mov = 0x0c, // move register to register

  // memory operations
  ldw = 0x0d, // load word
  stw = 0x0e, // store word
  ldb = 0x0f, // load byte
  stb = 0x10, // store byte

  // control flow - unconditional
  jmi = 0x20, // jump immediate
  jmp = 0x21, // jump register

  // control flow - conditional
  bve = 0x24, // branch if equal
  bvn = 0x25, // branch if not equal

  // function calls
  cal = 0x2a, // call
  ret = 0x2b, // return

  // extended arithmetic
  mul = 0x30, // multiply
  div = 0x31, // divide
  mod = 0x32, // modulus

  // advanced operations
  sia = 0x40, // shift and add
  sup = 0x41, // set upper
  sxt = 0x42, // sign extend
  seq = 0x43, // set if equal

  // system operations
  int_ = 0xf0, // interrupt
  snd = 0xfd,  // send to device
  hlt = 0xff   // halt
};

// opcode metadata for instruction decoding
struct opcode_info {
  const char* mnemonic;
  format fmt;
};

constexpr opcode_info get_opcode_info(opcode op) {
  switch (op) {
  case opcode::nop:
    return {"nop", format::op};
  case opcode::add:
    return {"add", format::op_reg_reg_reg};
  case opcode::sub:
    return {"sub", format::op_reg_reg_reg};
  case opcode::and_:
    return {"and", format::op_reg_reg_reg};
  case opcode::orr:
    return {"orr", format::op_reg_reg_reg};
  case opcode::xor_:
    return {"xor", format::op_reg_reg_reg};
  case opcode::not_:
    return {"not", format::op_reg_reg};
  case opcode::lsh:
    return {"lsh", format::op_reg_reg_reg};
  case opcode::ash:
    return {"ash", format::op_reg_reg_reg};
  case opcode::tcu:
    return {"tcu", format::op_reg_reg_reg};
  case opcode::tcs:
    return {"tcs", format::op_reg_reg_reg};
  case opcode::set:
    return {"set", format::op_reg_imm16};
  case opcode::mov:
    return {"mov", format::op_reg_reg};
  case opcode::ldw:
    return {"ldw", format::op_reg_reg_imm8};
  case opcode::stw:
    return {"stw", format::op_reg_reg_imm8};
  case opcode::ldb:
    return {"ldb", format::op_reg_reg_imm8};
  case opcode::stb:
    return {"stb", format::op_reg_reg_imm8};
  case opcode::jmi:
    return {"jmi", format::op_imm24};
  case opcode::jmp:
    return {"jmp", format::op_reg};
  case opcode::bve:
    return {"bve", format::op_reg_reg_imm8};
  case opcode::bvn:
    return {"bvn", format::op_reg_reg_imm8};
  case opcode::cal:
    return {"cal", format::op_reg};
  case opcode::ret:
    return {"ret", format::op};
  case opcode::mul:
    return {"mul", format::op_reg_reg_reg};
  case opcode::div:
    return {"div", format::op_reg_reg_reg};
  case opcode::mod:
    return {"mod", format::op_reg_reg_reg};
  case opcode::sia:
    return {"sia", format::op_reg_imm8x2};
  case opcode::sup:
    return {"sup", format::op_reg_imm16};
  case opcode::sxt:
    return {"sxt", format::op_reg_reg};
  case opcode::seq:
    return {"seq", format::op_reg_reg_imm8};
  case opcode::int_:
    return {"int", format::op_imm24};
  case opcode::snd:
    return {"snd", format::op_reg_reg_reg};
  case opcode::hlt:
    return {"hlt", format::op};
  default:
    return {"???", format::op};
  }
}

constexpr format get_format(opcode op) { return get_opcode_info(op).fmt; }

constexpr const char* get_mnemonic(opcode op) { return get_opcode_info(op).mnemonic; }
} // namespace irre