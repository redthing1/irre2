#pragma once

#include "object.hpp"
#include "../util.hpp"
#include <unordered_map>
#include <string>
#include <vector>

namespace irre::assembler {

// symbol resolution errors
enum class symbol_error { undefined_symbol, duplicate_symbol, invalid_symbol_reference };

// detailed symbol error with context
struct symbol_error_info {
  symbol_error error;
  std::string symbol_name;
  source_location location;
  std::string message;

  symbol_error_info(symbol_error err, const std::string& name, const source_location& loc = {})
      : error(err), symbol_name(name), location(loc) {
    switch (err) {
    case symbol_error::undefined_symbol:
      message = "undefined symbol '" + name + "'";
      break;
    case symbol_error::duplicate_symbol:
      message = "duplicate symbol '" + name + "'";
      break;
    case symbol_error::invalid_symbol_reference:
      message = "invalid symbol reference '" + name + "'";
      break;
    }
  }
};

constexpr const char* symbol_error_message(symbol_error err) {
  switch (err) {
  case symbol_error::undefined_symbol:
    return "undefined symbol";
  case symbol_error::duplicate_symbol:
    return "duplicate symbol";
  case symbol_error::invalid_symbol_reference:
    return "invalid symbol reference";
  default:
    return "unknown symbol error";
  }
}

// convenience function for getting message from symbol_error_info
inline const std::string& symbol_error_message(const symbol_error_info& err) { return err.message; }

// symbol table for resolving labels to addresses
class symbol_table {
public:
  // build symbol table from assembly items
  irre::result<bool, symbol_error_info> build(const std::vector<asm_item>& items);

  // resolve a symbol name to its address
  irre::result<uint32_t, symbol_error_info> resolve(
      const std::string& name, const source_location& location = {}
  ) const;

  // check if symbol exists
  bool has_symbol(const std::string& name) const;

  // get entry point address if set
  uint32_t get_entry_address(const std::string& entry_label) const;

private:
  std::unordered_map<std::string, uint32_t> symbols_;
  std::unordered_map<std::string, source_location> symbol_locations_; // track where symbols were defined
};

// symbol resolver - converts unresolved instructions to concrete ones
class symbol_resolver {
public:
  symbol_resolver(const symbol_table& symbols) : symbols_(symbols) {}

  // resolve all symbols in assembly items to concrete instructions
  irre::result<std::vector<instruction>, symbol_error_info> resolve(const std::vector<asm_item>& items);

private:
  const symbol_table& symbols_;

  // resolve a single unresolved instruction
  irre::result<instruction, symbol_error_info> resolve_instruction(const unresolved_instruction& unresolved);

  // convert operand to appropriate type for instruction format
  irre::result<std::variant<reg, uint32_t, uint8_t>, symbol_error_info> resolve_operand(
      const std::variant<reg, uint32_t, std::string>& operand, const source_location& location, bool is_8bit = false
  );
};

} // namespace irre::assembler