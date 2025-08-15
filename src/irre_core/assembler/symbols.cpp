#include "symbols.hpp"
#include "../arch/instruction.hpp"

namespace irre::assembler {

irre::result<bool, symbol_error_info> symbol_table::build(const std::vector<asm_item>& items) {
  symbols_.clear();
  symbol_locations_.clear();
  uint32_t address = 0;

  for (const auto& item : items) {
    auto result = std::visit(
        [&](const auto& content) -> irre::result<bool, symbol_error_info> {
          using T = std::decay_t<decltype(content)>;

          if constexpr (std::is_same_v<T, label_def>) {
            // check for duplicate labels
            if (symbols_.find(content.name) != symbols_.end()) {
              return symbol_error_info(symbol_error::duplicate_symbol, content.name, content.location);
            }
            symbols_[content.name] = address;
            symbol_locations_[content.name] = content.location;
            return true;
          } else if constexpr (std::is_same_v<T, instruction> || std::is_same_v<T, unresolved_instruction>) {
            // instructions take 4 bytes each
            address += 4;
            return true;
          } else if constexpr (std::is_same_v<T, data_block>) {
            // data blocks take their actual size
            address += static_cast<uint32_t>(content.bytes.size());
            return true;
          }
        },
        item
    );

    if (result.is_err()) {
      return result.error();
    }
  }

  return true;
}

irre::result<uint32_t, symbol_error_info> symbol_table::resolve(
    const std::string& name, const source_location& location
) const {
  auto it = symbols_.find(name);
  if (it == symbols_.end()) {
    return symbol_error_info(symbol_error::undefined_symbol, name, location);
  }
  return it->second;
}

bool symbol_table::has_symbol(const std::string& name) const { return symbols_.find(name) != symbols_.end(); }

uint32_t symbol_table::get_entry_address(const std::string& entry_label) const {
  auto it = symbols_.find(entry_label);
  return (it != symbols_.end()) ? it->second : 0;
}

irre::result<std::vector<instruction>, symbol_error_info> symbol_resolver::resolve(const std::vector<asm_item>& items) {
  std::vector<instruction> result;

  for (const auto& item : items) {
    // only process instructions (skip labels and data)
    if (std::holds_alternative<instruction>(item)) {
      result.push_back(std::get<instruction>(item));
    } else if (std::holds_alternative<unresolved_instruction>(item)) {
      auto resolve_result = resolve_instruction(std::get<unresolved_instruction>(item));
      if (resolve_result.is_err()) {
        return resolve_result.error();
      }
      result.push_back(resolve_result.value());
    }
    // skip labels and data blocks for instruction sequence
  }

  return result;
}

irre::result<instruction, symbol_error_info> symbol_resolver::resolve_instruction(
    const unresolved_instruction& unresolved
) {
  // determine instruction format and build appropriate instruction
  auto format = get_format(unresolved.op);

  switch (format) {
  case format::op: {
    return make::op(unresolved.op);
  }

  case format::op_reg: {
    if (unresolved.operands.size() != 1) {
      return symbol_error_info(symbol_error::invalid_symbol_reference, "<operand>", unresolved.location);
    }
    auto reg_result = resolve_operand(unresolved.operands[0], unresolved.location);
    if (reg_result.is_err()) {
      return reg_result.error();
    }
    auto reg_val = std::get<reg>(reg_result.value());
    return make::op_reg(unresolved.op, reg_val);
  }

  case format::op_imm24: {
    if (unresolved.operands.size() != 1) {
      return symbol_error_info(symbol_error::invalid_symbol_reference, "<operand>", unresolved.location);
    }
    auto imm_result = resolve_operand(unresolved.operands[0], unresolved.location);
    if (imm_result.is_err()) {
      return imm_result.error();
    }
    auto imm_val = std::get<uint32_t>(imm_result.value());
    return make::op_imm24(unresolved.op, imm_val);
  }

  case format::op_reg_imm16: {
    if (unresolved.operands.size() != 2) {
      return symbol_error_info(symbol_error::invalid_symbol_reference, "<operand>", unresolved.location);
    }
    auto reg_result = resolve_operand(unresolved.operands[0], unresolved.location);
    auto imm_result = resolve_operand(unresolved.operands[1], unresolved.location);
    if (reg_result.is_err()) {
      return reg_result.error();
    }
    if (imm_result.is_err()) {
      return imm_result.error();
    }
    auto reg_val = std::get<reg>(reg_result.value());
    auto imm_val = static_cast<uint16_t>(std::get<uint32_t>(imm_result.value()));
    return make::op_reg_imm16(unresolved.op, reg_val, imm_val);
  }

  case format::op_reg_reg: {
    if (unresolved.operands.size() != 2) {
      return symbol_error_info(symbol_error::invalid_symbol_reference, "<operand>", unresolved.location);
    }
    auto reg1_result = resolve_operand(unresolved.operands[0], unresolved.location);
    auto reg2_result = resolve_operand(unresolved.operands[1], unresolved.location);
    if (reg1_result.is_err()) {
      return reg1_result.error();
    }
    if (reg2_result.is_err()) {
      return reg2_result.error();
    }
    auto reg1 = std::get<reg>(reg1_result.value());
    auto reg2 = std::get<reg>(reg2_result.value());
    return make::op_reg_reg(unresolved.op, reg1, reg2);
  }

  case format::op_reg_reg_imm8: {
    if (unresolved.operands.size() != 3) {
      return symbol_error_info(symbol_error::invalid_symbol_reference, "<operand>", unresolved.location);
    }
    auto reg1_result = resolve_operand(unresolved.operands[0], unresolved.location);
    auto reg2_result = resolve_operand(unresolved.operands[1], unresolved.location);
    auto imm_result = resolve_operand(unresolved.operands[2], unresolved.location, true); // 8-bit
    if (reg1_result.is_err()) {
      return reg1_result.error();
    }
    if (reg2_result.is_err()) {
      return reg2_result.error();
    }
    if (imm_result.is_err()) {
      return imm_result.error();
    }
    auto reg1 = std::get<reg>(reg1_result.value());
    auto reg2 = std::get<reg>(reg2_result.value());
    auto imm = std::get<uint8_t>(imm_result.value());
    return make::op_reg_reg_imm8(unresolved.op, reg1, reg2, imm);
  }

  case format::op_reg_imm8x2: {
    if (unresolved.operands.size() != 3) {
      return symbol_error_info(symbol_error::invalid_symbol_reference, "<operand>", unresolved.location);
    }
    auto reg_result = resolve_operand(unresolved.operands[0], unresolved.location);
    auto imm1_result = resolve_operand(unresolved.operands[1], unresolved.location, true); // 8-bit
    auto imm2_result = resolve_operand(unresolved.operands[2], unresolved.location, true); // 8-bit
    if (reg_result.is_err()) {
      return reg_result.error();
    }
    if (imm1_result.is_err()) {
      return imm1_result.error();
    }
    if (imm2_result.is_err()) {
      return imm2_result.error();
    }
    auto reg_val = std::get<reg>(reg_result.value());
    auto imm1 = std::get<uint8_t>(imm1_result.value());
    auto imm2 = std::get<uint8_t>(imm2_result.value());
    return make::op_reg_imm8x2(unresolved.op, reg_val, imm1, imm2);
  }

  case format::op_reg_reg_reg: {
    if (unresolved.operands.size() != 3) {
      return symbol_error_info(symbol_error::invalid_symbol_reference, "<operand>", unresolved.location);
    }
    auto reg1_result = resolve_operand(unresolved.operands[0], unresolved.location);
    auto reg2_result = resolve_operand(unresolved.operands[1], unresolved.location);
    auto reg3_result = resolve_operand(unresolved.operands[2], unresolved.location);
    if (reg1_result.is_err()) {
      return reg1_result.error();
    }
    if (reg2_result.is_err()) {
      return reg2_result.error();
    }
    if (reg3_result.is_err()) {
      return reg3_result.error();
    }
    auto reg1 = std::get<reg>(reg1_result.value());
    auto reg2 = std::get<reg>(reg2_result.value());
    auto reg3 = std::get<reg>(reg3_result.value());
    return make::op_reg_reg_reg(unresolved.op, reg1, reg2, reg3);
  }

  default:
    return symbol_error_info(symbol_error::invalid_symbol_reference, "<unknown>", unresolved.location);
  }
}

irre::result<std::variant<reg, uint32_t, uint8_t>, symbol_error_info> symbol_resolver::resolve_operand(
    const std::variant<reg, uint32_t, std::string>& operand, const source_location& location, bool is_8bit
) {

  return std::visit(
      [&](const auto& op) -> irre::result<std::variant<reg, uint32_t, uint8_t>, symbol_error_info> {
        using T = std::decay_t<decltype(op)>;

        if constexpr (std::is_same_v<T, reg>) {
          return std::variant<reg, uint32_t, uint8_t>(op);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
          if (is_8bit) {
            return std::variant<reg, uint32_t, uint8_t>(static_cast<uint8_t>(op));
          } else {
            return std::variant<reg, uint32_t, uint8_t>(op);
          }
        } else if constexpr (std::is_same_v<T, std::string>) {
          // resolve symbol with location context
          auto addr_result = symbols_.resolve(op, location);
          if (addr_result.is_err()) {
            return addr_result.error();
          }
          if (is_8bit) {
            return std::variant<reg, uint32_t, uint8_t>(static_cast<uint8_t>(addr_result.value()));
          } else {
            return std::variant<reg, uint32_t, uint8_t>(addr_result.value());
          }
        }
      },
      operand
  );
}

} // namespace irre::assembler