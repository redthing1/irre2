#pragma once

#include "../arch/types.hpp"
#include "../arch/instruction.hpp"
#include "../util.hpp"
#include <vector>
#include <variant>
#include <string>

namespace irre::assembler {

// source location information
struct source_location {
  size_t line;
  size_t column;

  source_location(size_t l = 0, size_t c = 0) : line(l), column(c) {}
};

// items we build during parsing
struct label_def {
  std::string name;
  source_location location;
};

struct unresolved_instruction {
  opcode op;
  std::vector<std::variant<reg, uint32_t, std::string>> operands;
  source_location location;
};

struct data_block {
  std::vector<byte> bytes;
  source_location location;
};

// assembly items - either concrete instructions, unresolved instructions with labels, labels, or data
using asm_item = std::variant<instruction, unresolved_instruction, label_def, data_block>;

// improved object file format
struct object_file {
  static constexpr uint16_t version = 0x0001;

  uint32_t entry_offset = 0; // offset to entry point in code section
  std::vector<byte> code;    // encoded instructions
  std::vector<byte> data;    // data section bytes

  // write object file to binary format
  std::vector<byte> to_binary() const;

  // read object file from binary format with proper error handling
  static result<object_file, std::string> from_binary(const std::vector<byte>& binary);
};

} // namespace irre::assembler