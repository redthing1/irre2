#pragma once

#include "../arch/instruction.hpp"
#include "../util.hpp"
#include "object.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace irre::assembler {

// errors that can occur during assembly
enum class assemble_error {
  parse_error,
  undefined_symbol,
  invalid_instruction,
  invalid_register,
  invalid_immediate,
  duplicate_label,
  invalid_directive
};

constexpr const char* assemble_error_message(assemble_error err) {
  switch (err) {
  case assemble_error::parse_error:
    return "parse error";
  case assemble_error::undefined_symbol:
    return "undefined symbol";
  case assemble_error::invalid_instruction:
    return "invalid instruction";
  case assemble_error::invalid_register:
    return "invalid register";
  case assemble_error::invalid_immediate:
    return "invalid immediate value";
  case assemble_error::duplicate_label:
    return "duplicate label";
  case assemble_error::invalid_directive:
    return "invalid directive";
  default:
    return "unknown error";
  }
}

// assembly error with location information
struct assembly_error {
  assemble_error error;
  std::string message;
  size_t line;
  size_t column;
};

// main assembler class
class assembler {
public:
  // assemble source text into object file
  irre::result<object_file, assembly_error> assemble(const std::string& source);

private:
  // internal state during assembly
  struct assembly_state {
    std::vector<asm_item> items;
    std::unordered_map<std::string, uint32_t> symbols;
    std::string entry_label;
    size_t current_address = 0;
    size_t current_line = 1;
    size_t current_column = 1;
  };

  // parse source into intermediate representation
  irre::result<assembly_state, assembly_error> parse(const std::string& source);

  // resolve all symbol references
  irre::result<std::vector<instruction>, assembly_error> resolve_symbols(const assembly_state& state);

  // encode instructions to binary
  std::vector<byte> encode_instructions(const std::vector<instruction>& instructions);
  
  // collect data blocks from assembly items
  std::vector<byte> collect_data_blocks(const std::vector<asm_item>& items);
};

} // namespace irre::assembler