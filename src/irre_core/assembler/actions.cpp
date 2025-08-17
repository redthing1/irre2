#include "actions.hpp"
#include "../arch/encoding.hpp"

namespace irre::assembler::actions {

std::optional<reg> parse_register(const std::string& reg_str) {
  // general purpose registers r0-r31
  if (reg_str == "r0") {
    return reg::r0;
  }
  if (reg_str == "r1") {
    return reg::r1;
  }
  if (reg_str == "r2") {
    return reg::r2;
  }
  if (reg_str == "r3") {
    return reg::r3;
  }
  if (reg_str == "r4") {
    return reg::r4;
  }
  if (reg_str == "r5") {
    return reg::r5;
  }
  if (reg_str == "r6") {
    return reg::r6;
  }
  if (reg_str == "r7") {
    return reg::r7;
  }
  if (reg_str == "r8") {
    return reg::r8;
  }
  if (reg_str == "r9") {
    return reg::r9;
  }
  if (reg_str == "r10") {
    return reg::r10;
  }
  if (reg_str == "r11") {
    return reg::r11;
  }
  if (reg_str == "r12") {
    return reg::r12;
  }
  if (reg_str == "r13") {
    return reg::r13;
  }
  if (reg_str == "r14") {
    return reg::r14;
  }
  if (reg_str == "r15") {
    return reg::r15;
  }
  if (reg_str == "r16") {
    return reg::r16;
  }
  if (reg_str == "r17") {
    return reg::r17;
  }
  if (reg_str == "r18") {
    return reg::r18;
  }
  if (reg_str == "r19") {
    return reg::r19;
  }
  if (reg_str == "r20") {
    return reg::r20;
  }
  if (reg_str == "r21") {
    return reg::r21;
  }
  if (reg_str == "r22") {
    return reg::r22;
  }
  if (reg_str == "r23") {
    return reg::r23;
  }
  if (reg_str == "r24") {
    return reg::r24;
  }
  if (reg_str == "r25") {
    return reg::r25;
  }
  if (reg_str == "r26") {
    return reg::r26;
  }
  if (reg_str == "r27") {
    return reg::r27;
  }
  if (reg_str == "r28") {
    return reg::r28;
  }
  if (reg_str == "r29") {
    return reg::r29;
  }
  if (reg_str == "r30") {
    return reg::r30;
  }
  if (reg_str == "r31") {
    return reg::r31;
  }

  // special registers
  if (reg_str == "pc") {
    return reg::pc;
  }
  if (reg_str == "lr") {
    return reg::lr;
  }
  if (reg_str == "ad") {
    return reg::ad;
  }
  if (reg_str == "at") {
    return reg::at;
  }
  if (reg_str == "sp") {
    return reg::sp;
  }

  return std::nullopt;
}

std::optional<opcode> parse_mnemonic(const std::string& mnemonic) {
  // arithmetic and logical operations
  if (mnemonic == "nop") {
    return opcode::nop;
  }
  if (mnemonic == "add") {
    return opcode::add;
  }
  if (mnemonic == "sub") {
    return opcode::sub;
  }
  if (mnemonic == "and") {
    return opcode::and_;
  }
  if (mnemonic == "orr") {
    return opcode::orr;
  }
  if (mnemonic == "xor") {
    return opcode::xor_;
  }
  if (mnemonic == "not") {
    return opcode::not_;
  }
  if (mnemonic == "lsh") {
    return opcode::lsh;
  }
  if (mnemonic == "ash") {
    return opcode::ash;
  }
  if (mnemonic == "tcu") {
    return opcode::tcu;
  }
  if (mnemonic == "tcs") {
    return opcode::tcs;
  }

  // data movement
  if (mnemonic == "set") {
    return opcode::set;
  }
  if (mnemonic == "mov") {
    return opcode::mov;
  }

  // memory operations
  if (mnemonic == "ldw") {
    return opcode::ldw;
  }
  if (mnemonic == "stw") {
    return opcode::stw;
  }
  if (mnemonic == "ldb") {
    return opcode::ldb;
  }
  if (mnemonic == "stb") {
    return opcode::stb;
  }

  // control flow - unconditional
  if (mnemonic == "jmi") {
    return opcode::jmi;
  }
  if (mnemonic == "jmp") {
    return opcode::jmp;
  }

  // control flow - conditional
  if (mnemonic == "bve") {
    return opcode::bve;
  }
  if (mnemonic == "bvn") {
    return opcode::bvn;
  }

  // function calls
  if (mnemonic == "cal") {
    return opcode::cal;
  }
  if (mnemonic == "ret") {
    return opcode::ret;
  }

  // extended arithmetic
  if (mnemonic == "mul") {
    return opcode::mul;
  }
  if (mnemonic == "div") {
    return opcode::div;
  }
  if (mnemonic == "mod") {
    return opcode::mod;
  }

  // advanced operations
  if (mnemonic == "sia") {
    return opcode::sia;
  }
  if (mnemonic == "sup") {
    return opcode::sup;
  }
  if (mnemonic == "sxt") {
    return opcode::sxt;
  }
  if (mnemonic == "seq") {
    return opcode::seq;
  }

  // system operations
  if (mnemonic == "int") {
    return opcode::int_;
  }
  if (mnemonic == "snd") {
    return opcode::snd;
  }
  if (mnemonic == "hlt") {
    return opcode::hlt;
  }

  return std::nullopt;
}

result<uint32_t, std::string> parse_immediate(const std::string& imm_str) {
  if (imm_str.empty()) {
    return std::string("empty immediate value");
  }

  std::string str = imm_str;

  try {
    // Handle hex ($) and decimal (#) prefixes
    if (str[0] == '$') {
      str = str.substr(1);
      if (str.empty()) {
        return std::string("missing hex digits after $");
      }
      bool negative = false;
      if (str[0] == '-') {
        negative = true;
        str = str.substr(1);
        if (str.empty()) {
          return std::string("missing hex digits after $-");
        }
      }
      uint32_t value = std::stoul(str, nullptr, 16);
      if (negative) {
        value = static_cast<uint32_t>(-static_cast<int32_t>(value));
      }
      return value;
    } else if (str[0] == '#') {
      str = str.substr(1);
      if (str.empty()) {
        return std::string("missing decimal digits after #");
      }
      bool negative = false;
      if (str[0] == '-') {
        negative = true;
        str = str.substr(1);
        if (str.empty()) {
          return std::string("missing decimal digits after #-");
        }
      }
      uint32_t value = std::stoul(str, nullptr, 10);
      if (negative) {
        value = static_cast<uint32_t>(-static_cast<int32_t>(value));
      }
      return value;
    } else {
      // Plain number - assume decimal
      return static_cast<uint32_t>(std::stoul(str));
    }
  } catch (const std::invalid_argument&) {
    return std::string("invalid number format: " + imm_str);
  } catch (const std::out_of_range&) {
    return std::string("number out of range: " + imm_str);
  }
}

bool is_immediate(const std::string& str) { return str[0] == '#' || str[0] == '$' || std::isdigit(str[0]); }

bool is_pseudo_instruction(const std::string& mnemonic) { return mnemonic == "adi" || mnemonic == "sbi"; }

validation_result validate_immediate_range(uint32_t value, size_t bits) {
  uint32_t max_unsigned = (1u << bits) - 1;
  
  // Check if value fits in unsigned range [0, 2^bits - 1]
  if (value <= max_unsigned) {
    return validation_result::ok();
  }
  
  // Check if value represents a negative number in two's complement
  // For negative values: 2^32 - 2^(bits-1) <= value <= 2^32 - 1
  uint32_t min_negative = UINT32_MAX - ((1u << (bits - 1)) - 1);
  if (value >= min_negative) {
    return validation_result::ok();
  }
  
  // Value is out of range for both signed and unsigned interpretation
  int32_t signed_value = static_cast<int32_t>(value);
  return validation_result::fail(
      validation_error::immediate_out_of_range, 
      "immediate value " + std::to_string(signed_value) + " exceeds " +
      std::to_string(bits) + "-bit range (valid: -" + 
      std::to_string(1u << (bits - 1)) + " to " + 
      std::to_string(max_unsigned) + ")"
  );
}

validation_result validate_instruction_operands(opcode op, const std::vector<std::string>& operands) {
  auto fmt = get_format(op);

  switch (fmt) {
  case format::op:
    if (operands.size() != 0) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 0 operands, got " + std::to_string(operands.size())
      );
    }
    break;

  case format::op_reg:
    if (operands.size() != 1) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 1 operand, got " + std::to_string(operands.size())
      );
    }
    if (!parse_register(operands[0]) && !is_immediate(operands[0])) {
      return validation_result::fail(
          validation_error::operand_type_mismatch,
          "instruction '" + std::string(get_mnemonic(op)) + "' expects register operand"
      );
    }
    break;

  case format::op_imm24:
    if (operands.size() != 1) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 1 operand, got " + std::to_string(operands.size())
      );
    }
    if (is_immediate(operands[0])) {
      auto imm_result = parse_immediate(operands[0]);
      if (imm_result.is_err()) {
        return validation_result::fail(validation_error::invalid_immediate, imm_result.error());
      }
      auto range_check = validate_immediate_range(imm_result.value(), 24);
      if (!range_check.success) {
        return range_check;
      }
    }
    break;

  case format::op_reg_imm16:
    if (operands.size() != 2) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 2 operands, got " + std::to_string(operands.size())
      );
    }
    if (!parse_register(operands[0])) {
      return validation_result::fail(
          validation_error::operand_type_mismatch,
          "instruction '" + std::string(get_mnemonic(op)) + "' first operand must be register"
      );
    }
    if (is_immediate(operands[1])) {
      auto imm_result = parse_immediate(operands[1]);
      if (imm_result.is_err()) {
        return validation_result::fail(validation_error::invalid_immediate, imm_result.error());
      }
      auto range_check = validate_immediate_range(imm_result.value(), 16);
      if (!range_check.success) {
        return range_check;
      }
    }
    break;

  case format::op_reg_reg:
    if (operands.size() != 2) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 2 operands, got " + std::to_string(operands.size())
      );
    }
    if (!parse_register(operands[0])) {
      return validation_result::fail(
          validation_error::operand_type_mismatch,
          "instruction '" + std::string(get_mnemonic(op)) + "' first operand must be register"
      );
    }
    if (!parse_register(operands[1])) {
      return validation_result::fail(
          validation_error::operand_type_mismatch,
          "instruction '" + std::string(get_mnemonic(op)) + "' second operand must be register"
      );
    }
    break;

  case format::op_reg_reg_imm8:
    if (operands.size() != 3) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 3 operands, got " + std::to_string(operands.size())
      );
    }
    if (!parse_register(operands[0])) {
      return validation_result::fail(
          validation_error::operand_type_mismatch,
          "instruction '" + std::string(get_mnemonic(op)) + "' first operand must be register"
      );
    }
    if (!parse_register(operands[1])) {
      return validation_result::fail(
          validation_error::operand_type_mismatch,
          "instruction '" + std::string(get_mnemonic(op)) + "' second operand must be register"
      );
    }
    if (is_immediate(operands[2])) {
      auto imm_result = parse_immediate(operands[2]);
      if (imm_result.is_err()) {
        return validation_result::fail(validation_error::invalid_immediate, imm_result.error());
      }
      auto range_check = validate_immediate_range(imm_result.value(), 8);
      if (!range_check.success) {
        return range_check;
      }
    }
    break;

  case format::op_reg_imm8x2:
    if (operands.size() != 3) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 3 operands, got " + std::to_string(operands.size())
      );
    }
    if (!parse_register(operands[0])) {
      return validation_result::fail(
          validation_error::operand_type_mismatch,
          "instruction '" + std::string(get_mnemonic(op)) + "' first operand must be register"
      );
    }
    for (size_t i = 1; i < 3; i++) {
      if (is_immediate(operands[i])) {
        auto imm_result = parse_immediate(operands[i]);
        if (imm_result.is_err()) {
          return validation_result::fail(validation_error::invalid_immediate, imm_result.error());
        }
        auto range_check = validate_immediate_range(imm_result.value(), 8);
        if (!range_check.success) {
          return range_check;
        }
      }
    }
    break;

  case format::op_reg_reg_reg:
    if (operands.size() != 3) {
      return validation_result::fail(
          validation_error::operand_count_mismatch, "instruction '" + std::string(get_mnemonic(op)) +
                                                        "' expects 3 operands, got " + std::to_string(operands.size())
      );
    }
    for (const auto& operand : operands) {
      if (!parse_register(operand)) {
        return validation_result::fail(
            validation_error::operand_type_mismatch,
            "instruction '" + std::string(get_mnemonic(op)) + "' all operands must be registers"
        );
      }
    }
    break;
    
  case format::invalid:
    return validation_result::fail(
        validation_error::unknown_instruction, 
        "invalid opcode '" + std::string(get_mnemonic(op)) + "'"
    );
  }

  return validation_result::ok();
}

std::vector<std::vector<std::string>> expand_pseudo_instruction(
    const std::string& mnemonic, const std::vector<std::string>& operands
) {
  std::vector<std::vector<std::string>> result;

  if (mnemonic == "adi") {
    // adi rA rB #imm -> set at #imm; add rA rB at
    if (operands.size() == 3) {
      result.push_back({"set", "at", operands[2]});
      result.push_back({"add", operands[0], operands[1], "at"});
    }
  } else if (mnemonic == "sbi") {
    // sbi rA rB #imm -> set at #imm; sub rA rB at
    if (operands.size() == 3) {
      result.push_back({"set", "at", operands[2]});
      result.push_back({"sub", operands[0], operands[1], "at"});
    }
  }

  return result;
}

validation_result process_single_instruction(
    parse_state& s, const std::string& mnemonic, const std::vector<std::string>& operand_strs
) {
  auto opcode_opt = parse_mnemonic(mnemonic);
  if (!opcode_opt) {
    return validation_result::fail(validation_error::unknown_instruction, "unknown instruction: " + mnemonic);
  }

  opcode op = *opcode_opt;

  // validate operands before processing
  auto validation = validate_instruction_operands(op, operand_strs);
  if (!validation.success) {
    return validation;
  }

  // Try to build concrete instruction if all operands are resolved
  std::vector<std::variant<reg, uint32_t, std::string>> operands;
  bool has_labels = false;

  for (const auto& operand_str : operand_strs) {
    if (auto reg = parse_register(operand_str)) {
      operands.push_back(*reg);
    } else if (is_immediate(operand_str)) {
      auto imm_result = parse_immediate(operand_str);
      if (imm_result.is_err()) {
        return validation_result::fail(validation_error::invalid_immediate, imm_result.error());
      }
      operands.push_back(imm_result.value());
    } else {
      // label reference
      operands.push_back(operand_str);
      has_labels = true;
    }
  }

  if (!has_labels) {
    // Build concrete instruction based on format
    try {
      auto format = get_format(op);

      switch (format) {
      case format::op:
        if (operands.size() == 0) {
          s.emit_concrete_instruction(make::op(op));
        }
        break;

      case format::op_reg:
        if (operands.size() == 1) {
          auto reg = std::get<::irre::reg>(operands[0]);
          s.emit_concrete_instruction(make::op_reg(op, reg));
        }
        break;

      case format::op_imm24:
        if (operands.size() == 1) {
          auto imm = std::get<uint32_t>(operands[0]);
          s.emit_concrete_instruction(make::op_imm24(op, imm));
        }
        break;

      case format::op_reg_imm16:
        if (operands.size() == 2) {
          auto reg = std::get<::irre::reg>(operands[0]);
          auto imm = std::get<uint32_t>(operands[1]);
          s.emit_concrete_instruction(make::op_reg_imm16(op, reg, static_cast<uint16_t>(imm)));
        }
        break;

      case format::op_reg_reg:
        if (operands.size() == 2) {
          auto reg1 = std::get<::irre::reg>(operands[0]);
          auto reg2 = std::get<::irre::reg>(operands[1]);
          s.emit_concrete_instruction(make::op_reg_reg(op, reg1, reg2));
        }
        break;

      case format::op_reg_reg_imm8:
        if (operands.size() == 3) {
          auto reg1 = std::get<::irre::reg>(operands[0]);
          auto reg2 = std::get<::irre::reg>(operands[1]);
          auto imm = std::get<uint32_t>(operands[2]);
          s.emit_concrete_instruction(make::op_reg_reg_imm8(op, reg1, reg2, static_cast<uint8_t>(imm)));
        }
        break;

      case format::op_reg_imm8x2:
        if (operands.size() == 3) {
          auto reg = std::get<::irre::reg>(operands[0]);
          auto imm1 = std::get<uint32_t>(operands[1]);
          auto imm2 = std::get<uint32_t>(operands[2]);
          s.emit_concrete_instruction(
              make::op_reg_imm8x2(op, reg, static_cast<uint8_t>(imm1), static_cast<uint8_t>(imm2))
          );
        }
        break;

      case format::op_reg_reg_reg:
        if (operands.size() == 3) {
          auto reg1 = std::get<::irre::reg>(operands[0]);
          auto reg2 = std::get<::irre::reg>(operands[1]);
          auto reg3 = std::get<::irre::reg>(operands[2]);
          s.emit_concrete_instruction(make::op_reg_reg_reg(op, reg1, reg2, reg3));
        }
        break;

      case format::invalid:
        // Invalid opcodes should have been caught in validation
        break;

      default:
        s.emit_unresolved_instruction(op, operands);
        break;
      }
    } catch (const std::bad_variant_access&) {
      // fallback to unresolved
      s.emit_unresolved_instruction(op, operands);
    }
  } else {
    // Has label references, emit unresolved
    s.emit_unresolved_instruction(op, operands);
  }

  return validation_result::ok();
}

result<std::vector<uint8_t>, std::string> parse_data_content(const std::string& data_str) {
  std::vector<uint8_t> result;
  size_t i = 0;
  
  while (i < data_str.length()) {
    // Skip whitespace
    while (i < data_str.length() && std::isspace(data_str[i])) {
      i++;
    }
    
    if (i >= data_str.length()) {
      break;
    }
    
    // Stop at comment
    if (data_str[i] == ';') {
      break;
    }
    
    if (data_str[i] == '"') {
      // Parse string literal
      i++; // skip opening quote
      while (i < data_str.length() && data_str[i] != '"') {
        if (data_str[i] == '\\' && i + 1 < data_str.length()) {
          // Handle escape sequences
          i++;
          switch (data_str[i]) {
          case 'n':
            result.push_back('\n');
            break;
          case 't':
            result.push_back('\t');
            break;
          case 'r':
            result.push_back('\r');
            break;
          case '\\':
            result.push_back('\\');
            break;
          case '"':
            result.push_back('"');
            break;
          case '0':
            result.push_back('\0');
            break;
          default:
            return std::string("invalid escape sequence: \\" + std::string(1, data_str[i]));
          }
        } else {
          result.push_back(static_cast<uint8_t>(data_str[i]));
        }
        i++;
      }
      
      if (i >= data_str.length()) {
        return std::string("unterminated string literal");
      }
      
      i++; // skip closing quote
    } else {
      // Parse numeric value
      size_t start = i;
      
      // Find end of token
      while (i < data_str.length() && !std::isspace(data_str[i])) {
        i++;
      }
      
      std::string token = data_str.substr(start, i - start);
      auto imm_result = parse_immediate(token);
      
      if (imm_result.is_err()) {
        return std::string("invalid number: " + token + " (" + imm_result.error() + ")");
      }
      
      uint32_t value = imm_result.value();
      
      // Store as little-endian 32-bit word
      result.push_back(static_cast<uint8_t>(value & 0xFF));
      result.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
      result.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
      result.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }
  }
  
  return result;
}

} // namespace irre::assembler::actions