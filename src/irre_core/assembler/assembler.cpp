#include "assembler.hpp"
#include "grammar.hpp"
#include "actions.hpp"
#include "symbols.hpp"
#include "../encoding.hpp"
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/trace.hpp>

namespace irre::assembler {

namespace pegtl = tao::pegtl;

irre::result<object_file, assembly_error> assembler::assemble(const std::string& source) {
  // step 1: parse source into assembly items
  auto parse_result = parse(source);
  if (parse_result.is_err()) {
    return parse_result.error();
  }

  auto state = parse_result.value();

  // step 2: resolve symbols
  auto resolve_result = resolve_symbols(state);
  if (resolve_result.is_err()) {
    return assembly_error{assemble_error::undefined_symbol, resolve_result.error().message, 0, 0};
  }

  auto instructions = resolve_result.value();

  // step 3: encode instructions to binary
  auto code_bytes = encode_instructions(instructions);

  // step 4: build object file
  object_file obj;
  obj.code = std::move(code_bytes);

  // set entry point if specified
  if (!state.entry_label.empty()) {
    symbol_table symbols;
    if (symbols.build(state.items).is_ok()) {
      obj.entry_offset = symbols.get_entry_address(state.entry_label);
    }
  }

  return obj;
}

irre::result<assembler::assembly_state, assembly_error> assembler::parse(const std::string& source) {
  actions::parse_state state;

  try {
    pegtl::memory_input input(source, "assembly");

    // parse with PEGTL
    bool success = pegtl::parse<grammar::program, actions::action>(input, state);

    if (!success) {
      return assembly_error{assemble_error::parse_error, "failed to parse assembly", input.line(), input.column()};
    }

    // check for validation errors
    if (!state.errors.empty()) {
      // return first validation error found
      const auto& first_error = state.errors[0];
      assemble_error error_type = assemble_error::parse_error;

      switch (first_error.error) {
      case actions::validation_error::unknown_instruction:
        error_type = assemble_error::invalid_instruction;
        break;
      case actions::validation_error::unknown_register:
        error_type = assemble_error::invalid_register;
        break;
      case actions::validation_error::invalid_immediate:
      case actions::validation_error::immediate_out_of_range:
        error_type = assemble_error::invalid_immediate;
        break;
      case actions::validation_error::operand_count_mismatch:
      case actions::validation_error::operand_type_mismatch:
        error_type = assemble_error::invalid_instruction;
        break;
      }

      return assembly_error{error_type, first_error.message, input.line(), input.column()};
    }

    // convert to assembly_state
    assembly_state result;
    result.items = std::move(state.items);
    result.entry_label = std::move(state.entry_point);

    return result;

  } catch (const pegtl::parse_error& e) {
    return assembly_error{assemble_error::parse_error, e.what(), 0, 0};
  }
}

irre::result<std::vector<instruction>, assembly_error> assembler::resolve_symbols(const assembly_state& state) {
  // build symbol table
  symbol_table symbols;
  auto build_result = symbols.build(state.items);
  if (build_result.is_err()) {
    const auto& err = build_result.error();
    return assembly_error{assemble_error::undefined_symbol, err.message, err.location.line, err.location.column};
  }

  // resolve all symbols
  symbol_resolver resolver(symbols);
  auto resolve_result = resolver.resolve(state.items);
  if (resolve_result.is_err()) {
    const auto& err = resolve_result.error();
    return assembly_error{assemble_error::undefined_symbol, err.message, err.location.line, err.location.column};
  }

  return resolve_result.value();
}

std::vector<byte> assembler::encode_instructions(const std::vector<instruction>& instructions) {
  std::vector<byte> result;
  result.reserve(instructions.size() * 4); // each instruction is 4 bytes

  for (const auto& inst : instructions) {
    auto bytes = codec::encode_bytes(inst);
    result.insert(result.end(), bytes.begin(), bytes.end());
  }

  return result;
}

} // namespace irre::assembler