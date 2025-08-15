#pragma once

#include "../types.hpp"
#include "../instruction.hpp"
#include "../encoding.hpp"
#include "../util.hpp"
#include "object.hpp"
#include <vector>
#include <string>
#include <sstream>

namespace irre::assembler {

// disassembler error types
enum class disasm_error { decode_failed, invalid_size, file_error, empty_input };

constexpr const char* disasm_error_message(disasm_error err) {
  switch (err) {
  case disasm_error::decode_failed:
    return "failed to decode instructions";
  case disasm_error::invalid_size:
    return "invalid input size";
  case disasm_error::file_error:
    return "file operation failed";
  case disasm_error::empty_input:
    return "empty input";
  default:
    return "unknown disassembler error";
  }
}

// disassembler configuration options
struct disasm_options {
  bool show_addresses = true;         // show instruction addresses
  bool show_hex_bytes = true;         // show raw hex bytes
  bool show_offsets = false;          // show file offsets instead of addresses
  std::string address_format = "hex"; // "hex" or "decimal"
  uint32_t base_address = 0;          // base address for raw byte disassembly
};

// disassembler output formats
enum class disasm_format {
  basic,    // plain assembly text
  annotated // addresses + hex bytes + assembly
};

// main disassembler class
class disassembler {
public:
  explicit disassembler(const disasm_options& opts = {}) : options_(opts) {}

  // disassemble an object file
  result<std::string, disasm_error> disassemble_object(
      const object_file& obj, disasm_format fmt = disasm_format::annotated
  );

  // disassemble raw instruction bytes
  result<std::string, disasm_error> disassemble_bytes(
      const std::vector<byte>& bytes, disasm_format fmt = disasm_format::annotated
  );

  // disassemble a single instruction at given address
  result<std::string, disasm_error> disassemble_instruction(
      const instruction& inst, uint32_t addr = 0, const std::vector<byte>* raw_bytes = nullptr
  );

  // set disassembler options
  void set_options(const disasm_options& opts) { options_ = opts; }
  const disasm_options& get_options() const { return options_; }

private:
  disasm_options options_;

  // format an address according to options
  std::string format_address(uint32_t addr) const;

  // format hex bytes for instruction
  std::string format_hex_bytes(const std::vector<byte>& bytes) const;

  // create annotated output line
  std::string format_annotated_line(
      uint32_t addr, const std::vector<byte>& inst_bytes, const std::string& assembly
  ) const;
};

// convenience functions
namespace disasm {

// disassemble object file with default options
inline result<std::string, disasm_error> object(const object_file& obj) {
  disassembler disasm;
  return disasm.disassemble_object(obj);
}

// disassemble raw bytes with default options
inline result<std::string, disasm_error> bytes(const std::vector<byte>& data) {
  disassembler disasm;
  return disasm.disassemble_bytes(data);
}

// disassemble from file
result<std::string, disasm_error> from_file(const std::string& filename);

} // namespace disasm

} // namespace irre::assembler