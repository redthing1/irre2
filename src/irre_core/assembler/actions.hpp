#pragma once

#include "grammar.hpp"
#include "object.hpp"
#include "../arch/instruction.hpp"
#include <tao/pegtl.hpp>
#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <optional>

namespace irre::assembler::actions {

namespace pegtl = tao::pegtl;

// validation error types
enum class validation_error {
  unknown_instruction,
  unknown_register,
  invalid_immediate,
  operand_count_mismatch,
  operand_type_mismatch,
  immediate_out_of_range
};

struct validation_result {
  bool success;
  validation_error error;
  std::string message;

  static validation_result ok() { return {true, validation_error::unknown_instruction, ""}; }
  static validation_result fail(validation_error err, const std::string& msg) { return {false, err, msg}; }
};

// state maintained during parsing
struct parse_state {
  std::vector<asm_item> items;
  std::string entry_point;
  std::string current_section = "code";
  std::vector<validation_result> errors; // collect validation errors

  void emit_label(const std::string& name) { items.push_back(label_def{name}); }

  void emit_concrete_instruction(instruction inst) { items.push_back(inst); }

  void emit_unresolved_instruction(opcode op, const std::vector<std::variant<reg, uint32_t, std::string>>& operands) {
    items.push_back(unresolved_instruction{op, operands});
  }

  void set_entry_point(const std::string& label) { entry_point = label; }

  void set_section(const std::string& section) { current_section = section; }

  void emit_data(const std::vector<uint8_t>& data) { items.push_back(data_block{data}); }

  void add_error(const validation_result& error) {
    if (!error.success) {
      errors.push_back(error);
    }
  }
};

// helper functions
std::optional<reg> parse_register(const std::string& reg_str);
std::optional<opcode> parse_mnemonic(const std::string& mnemonic);
result<uint32_t, std::string> parse_immediate(const std::string& imm_str);
bool is_immediate(const std::string& str);
bool is_pseudo_instruction(const std::string& mnemonic);
validation_result validate_instruction_operands(opcode op, const std::vector<std::string>& operands);
validation_result validate_immediate_range(uint32_t value, size_t bits);
std::vector<std::vector<std::string>> expand_pseudo_instruction(
    const std::string& mnemonic, const std::vector<std::string>& operands
);
validation_result process_single_instruction(
    parse_state& s, const std::string& mnemonic, const std::vector<std::string>& operand_strs
);

// default action - do nothing
template <typename Rule> struct action {};

// label definitions
template <> struct action<grammar::label_def> {
  template <typename Input> static void apply(const Input& in, parse_state& s) {
    std::string label_text = in.string();
    // remove the colon from "label:"
    std::string name = label_text.substr(0, label_text.length() - 1);
    s.emit_label(name);
  }
};

// directive actions
template <> struct action<grammar::directive_entry> {
  template <typename Input> static void apply(const Input& in, parse_state& s) {
    std::string text = in.string();
    // extract label from "%entry: main" format
    auto colon_pos = text.find(':');
    if (colon_pos != std::string::npos) {
      std::string label = text.substr(colon_pos + 1);
      // trim whitespace
      size_t start = label.find_first_not_of(" \t");
      if (start != std::string::npos) {
        label = label.substr(start);
        s.set_entry_point(label);
      }
    }
  }
};

template <> struct action<grammar::directive_section> {
  template <typename Input> static void apply(const Input& in, parse_state& s) {
    std::string text = in.string();
    std::istringstream iss(text);
    std::string directive, section;
    iss >> directive >> section; // skip "%section" and get section name
    s.set_section(section);
  }
};

template <> struct action<grammar::directive_data> {
  template <typename Input> static void apply(const Input& in, parse_state& s) {
    std::string text = in.string();
    // For now, just emit placeholder data
    std::vector<uint8_t> data = {0x00}; // placeholder
    s.emit_data(data);
  }
};

// generic instruction parsing
template <> struct action<grammar::instruction> {
  template <typename Input> static void apply(const Input& in, parse_state& s) {
    std::string text = in.string();
    std::istringstream iss(text);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) {
      tokens.push_back(token);
    }

    if (tokens.empty()) {
      return;
    }

    std::string mnemonic = tokens[0];
    std::vector<std::string> operand_strs(tokens.begin() + 1, tokens.end());

    // Check if this is a pseudo-instruction
    if (is_pseudo_instruction(mnemonic)) {
      // Expand pseudo-instruction into multiple real instructions
      auto expansions = expand_pseudo_instruction(mnemonic, operand_strs);
      for (const auto& expansion : expansions) {
        if (!expansion.empty()) {
          std::string real_mnemonic = expansion[0];
          std::vector<std::string> real_operands(expansion.begin() + 1, expansion.end());
          auto result = process_single_instruction(s, real_mnemonic, real_operands);
          s.add_error(result);
        }
      }
    } else {
      // Process as regular instruction
      auto result = process_single_instruction(s, mnemonic, operand_strs);
      s.add_error(result);
    }
  }
};

} // namespace irre::assembler::actions