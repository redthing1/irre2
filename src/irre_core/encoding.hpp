#pragma once

#include "instruction.hpp"
#include "util.hpp"
#include <array>
#include <vector>

namespace irre {

// encoding/decoding specific errors
enum class decode_error { invalid_opcode, invalid_register, malformed_instruction };

constexpr const char* decode_error_message(decode_error err) {
  switch (err) {
  case decode_error::invalid_opcode:
    return "invalid opcode";
  case decode_error::invalid_register:
    return "invalid register";
  case decode_error::malformed_instruction:
    return "malformed instruction";
  default:
    return "unknown error";
  }
}

// main codec class for encoding/decoding instructions
class codec {
public:
  // encode instruction to 32-bit word
  static word encode(const instruction& inst) {
    return std::visit([](const auto& i) { return i.encode(); }, inst);
  }

  // decode 32-bit word to instruction
  static result<instruction, decode_error> decode(word w) {
    opcode op = static_cast<opcode>((w >> 24) & 0xff);

    // validate opcode exists in our mapping
    auto info = get_opcode_info(op);
    if (info.fmt == format::op && op != opcode::nop && op != opcode::ret && op != opcode::hlt) {
      // if format is 'op' but opcode isn't one of the known op-only instructions,
      // it's likely an invalid opcode
      bool valid_op_only = (op == opcode::nop || op == opcode::ret || op == opcode::hlt);
      if (!valid_op_only) {
        return decode_error::invalid_opcode;
      }
    }

    // validate registers if present
    auto validate_reg = [](uint8_t reg_val) -> bool { return reg_val <= static_cast<uint8_t>(reg::sp); };

    uint8_t a1 = (w >> 16) & 0xff;
    uint8_t a2 = (w >> 8) & 0xff;
    uint8_t a3 = w & 0xff;

    // dispatch based on opcode format
    switch (get_format(op)) {
    case format::op:
      return instruction(inst_op::decode(w));

    case format::op_reg:
      if (!validate_reg(a1)) {
        return decode_error::invalid_register;
      }
      return instruction(inst_op_reg::decode(w));

    case format::op_imm24:
      return instruction(inst_op_imm24::decode(w));

    case format::op_reg_imm16:
      if (!validate_reg(a1)) {
        return decode_error::invalid_register;
      }
      return instruction(inst_op_reg_imm16::decode(w));

    case format::op_reg_reg:
      if (!validate_reg(a1) || !validate_reg(a2)) {
        return decode_error::invalid_register;
      }
      return instruction(inst_op_reg_reg::decode(w));

    case format::op_reg_reg_imm8:
      if (!validate_reg(a1) || !validate_reg(a2)) {
        return decode_error::invalid_register;
      }
      return instruction(inst_op_reg_reg_imm8::decode(w));

    case format::op_reg_imm8x2:
      if (!validate_reg(a1)) {
        return decode_error::invalid_register;
      }
      return instruction(inst_op_reg_imm8x2::decode(w));

    case format::op_reg_reg_reg:
      if (!validate_reg(a1) || !validate_reg(a2) || !validate_reg(a3)) {
        return decode_error::invalid_register;
      }
      return instruction(inst_op_reg_reg_reg::decode(w));

    default:
      return decode_error::invalid_opcode;
    }
  }

  // encode to byte array (little-endian)
  static std::array<byte, 4> encode_bytes(const instruction& inst) {
    word w = encode(inst);
    return {
        static_cast<byte>(w & 0xff),         // bits 7-0
        static_cast<byte>((w >> 8) & 0xff),  // bits 15-8
        static_cast<byte>((w >> 16) & 0xff), // bits 23-16
        static_cast<byte>((w >> 24) & 0xff)  // bits 31-24
    };
  }

  // decode from byte array (little-endian)
  static result<instruction, decode_error> decode_bytes(const std::array<byte, 4>& bytes) {
    word w = static_cast<word>(bytes[0]) | (static_cast<word>(bytes[1]) << 8) | (static_cast<word>(bytes[2]) << 16) |
             (static_cast<word>(bytes[3]) << 24);
    return decode(w);
  }

  // decode from raw byte pointer (little-endian)
  static result<instruction, decode_error> decode_bytes(const byte* data) {
    if (!data) {
      return decode_error::malformed_instruction;
    }

    std::array<byte, 4> bytes = {data[0], data[1], data[2], data[3]};
    return decode_bytes(bytes);
  }
};

// utility functions for working with byte streams
namespace byte_utils {

// encode instruction sequence to byte vector
template <typename It> std::vector<byte> encode_sequence(It begin, It end) {
  std::vector<byte> result;
  result.reserve(std::distance(begin, end) * 4);

  for (auto it = begin; it != end; ++it) {
    auto bytes = codec::encode_bytes(*it);
    result.insert(result.end(), bytes.begin(), bytes.end());
  }

  return result;
}

// decode byte sequence to instructions
inline result<std::vector<instruction>, decode_error> decode_sequence(const std::vector<byte>& bytes) {
  std::vector<instruction> result_vec;

  if (bytes.size() % 4 != 0) {
    return decode_error::malformed_instruction;
  }

  result_vec.reserve(bytes.size() / 4);

  for (size_t i = 0; i < bytes.size(); i += 4) {
    auto inst_result = codec::decode_bytes(&bytes[i]);
    if (inst_result.is_err()) {
      return inst_result.error();
    }
    result_vec.push_back(inst_result.value());
  }

  return result_vec;
}

// validate instruction sequence
inline result<bool, decode_error> validate_sequence(const std::vector<byte>& bytes) {
  if (bytes.size() % 4 != 0) {
    return decode_error::malformed_instruction;
  }

  for (size_t i = 0; i < bytes.size(); i += 4) {
    auto inst_result = codec::decode_bytes(&bytes[i]);
    if (inst_result.is_err()) {
      return inst_result.error();
    }
  }

  return true;
}
} // namespace byte_utils
} // namespace irre