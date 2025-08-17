#include "disassembler.hpp"
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <optional>

namespace irre::assembler {

result<std::string, disasm_error> disassembler::disassemble_object(const object_file& obj, disasm_format fmt) {
  if (obj.code.empty()) {
    return std::string(""); // empty object file
  }

  // decode instruction sequence from code section, handling errors gracefully
  struct disasm_entry {
    std::optional<instruction> inst;
    std::string error_msg;
    bool is_error = false;
  };
  
  std::vector<disasm_entry> entries;
  
  if (obj.code.size() % 4 != 0) {
    return disasm_error::invalid_size;
  }

  entries.reserve(obj.code.size() / 4);
  for (size_t i = 0; i < obj.code.size(); i += 4) {
    auto inst_result = codec::decode_bytes(&obj.code[i]);
    if (inst_result.is_ok()) {
      entries.push_back({inst_result.value(), "", false});
    } else {
      // Create an error entry for invalid instructions
      word invalid_word = static_cast<word>(obj.code[i]) | 
                         (static_cast<word>(obj.code[i+1]) << 8) |
                         (static_cast<word>(obj.code[i+2]) << 16) |
                         (static_cast<word>(obj.code[i+3]) << 24);
      
      std::ostringstream error_msg;
      error_msg << "; ERROR: " << decode_error_message(inst_result.error()) 
                << " (0x" << std::hex << std::setfill('0') << std::setw(8) << invalid_word
                << " = " << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(obj.code[i]) << " "
                << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(obj.code[i+1]) << " "
                << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(obj.code[i+2]) << " "
                << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(obj.code[i+3]) << ")";
      
      entries.push_back({std::nullopt, error_msg.str(), true});
    }
  }

  std::ostringstream output;

  // add header comment for object file info
  if (fmt == disasm_format::annotated) {
    output << "; irre object file disassembly\n";
    output << "; entry point: 0x" << std::hex << obj.entry_offset << std::dec << "\n";
    output << "; code size: " << obj.code.size() << " bytes (" << entries.size() << " instructions)\n";
    if (!obj.data.empty()) {
      output << "; data size: " << obj.data.size() << " bytes\n";
    }
    output << "\n";
  }

  // disassemble each entry (instruction or error)
  for (size_t i = 0; i < entries.size(); ++i) {
    uint32_t addr = static_cast<uint32_t>(i * 4);
    const auto& entry = entries[i];

    if (entry.is_error) {
      // Show error message with address
      output << format_address(addr) << ": " << entry.error_msg;
    } else {
      // extract raw bytes for this instruction
      std::vector<byte> inst_bytes;
      if (i * 4 + 4 <= obj.code.size()) {
        inst_bytes.assign(obj.code.begin() + i * 4, obj.code.begin() + i * 4 + 4);
      }

      auto line_result = disassemble_instruction(entry.inst.value(), addr, &inst_bytes);
      if (line_result.is_err()) {
        return line_result.error();
      }

      output << line_result.value();
    }

    if (i < entries.size() - 1) {
      output << "\n";
    }
  }

  // add data section if present
  if (!obj.data.empty() && fmt == disasm_format::annotated) {
    output << "\n\n; data section (" << obj.data.size() << " bytes)\n";
    uint32_t data_addr = static_cast<uint32_t>(obj.code.size());

    for (size_t i = 0; i < obj.data.size(); i += 16) {
      output << format_address(data_addr + i) << ": ";

      // hex bytes (up to 16 per line)
      size_t line_end = std::min(i + 16, obj.data.size());
      for (size_t j = i; j < line_end; ++j) {
        output << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(obj.data[j]);
      }

      if (i + 16 < obj.data.size()) {
        output << "\n";
      }
    }
    output << std::dec; // reset to decimal
  }

  return output.str();
}

result<std::string, disasm_error> disassembler::disassemble_bytes(const std::vector<byte>& bytes, disasm_format fmt) {
  if (bytes.empty()) {
    return std::string(""); // empty input
  }

  if (bytes.size() % 4 != 0) {
    return disasm_error::invalid_size;
  }

  // decode instruction sequence, handling errors gracefully
  struct disasm_entry {
    std::optional<instruction> inst;
    std::string error_msg;
    bool is_error = false;
  };
  
  std::vector<disasm_entry> entries;
  entries.reserve(bytes.size() / 4);
  
  for (size_t i = 0; i < bytes.size(); i += 4) {
    auto inst_result = codec::decode_bytes(&bytes[i]);
    if (inst_result.is_ok()) {
      entries.push_back({inst_result.value(), "", false});
    } else {
      // Create an error entry for invalid instructions
      word invalid_word = static_cast<word>(bytes[i]) | 
                         (static_cast<word>(bytes[i+1]) << 8) |
                         (static_cast<word>(bytes[i+2]) << 16) |
                         (static_cast<word>(bytes[i+3]) << 24);
      
      std::ostringstream error_msg;
      error_msg << "; ERROR: " << decode_error_message(inst_result.error()) 
                << " (0x" << std::hex << std::setfill('0') << std::setw(8) << invalid_word
                << " = " << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(bytes[i]) << " "
                << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(bytes[i+1]) << " "
                << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(bytes[i+2]) << " "
                << std::hex << std::setfill('0') << std::setw(2) 
                << static_cast<int>(bytes[i+3]) << ")";
      
      entries.push_back({std::nullopt, error_msg.str(), true});
    }
  }
  std::ostringstream output;

  // add header comment for raw bytes
  if (fmt == disasm_format::annotated) {
    output << "; raw bytes disassembly\n";
    output << "; base address: 0x" << std::hex << options_.base_address << std::dec << "\n";
    output << "; size: " << bytes.size() << " bytes (" << entries.size() << " instructions)\n\n";
  }

  // disassemble each entry (instruction or error)
  for (size_t i = 0; i < entries.size(); ++i) {
    uint32_t addr = options_.base_address + static_cast<uint32_t>(i * 4);
    const auto& entry = entries[i];

    if (entry.is_error) {
      // Show error message with address
      output << format_address(addr) << ": " << entry.error_msg;
    } else {
      // extract raw bytes for this instruction
      std::vector<byte> inst_bytes(bytes.begin() + i * 4, bytes.begin() + i * 4 + 4);

      auto line_result = disassemble_instruction(entry.inst.value(), addr, &inst_bytes);
      if (line_result.is_err()) {
        return line_result.error();
      }

      output << line_result.value();
    }

    if (i < entries.size() - 1) {
      output << "\n";
    }
  }

  return output.str();
}

result<std::string, disasm_error> disassembler::disassemble_instruction(
    const instruction& inst, uint32_t addr, const std::vector<byte>* raw_bytes
) {
  std::ostringstream output;

  // get assembly representation
  std::string assembly = format_instruction(inst);

  if (options_.show_addresses || options_.show_hex_bytes) {
    // annotated format
    std::vector<byte> bytes;
    if (raw_bytes) {
      bytes = *raw_bytes;
    } else {
      // encode instruction to get bytes
      auto encoded_bytes = codec::encode_bytes(inst);
      bytes.assign(encoded_bytes.begin(), encoded_bytes.end());
    }

    output << format_annotated_line(addr, bytes, assembly);
  } else {
    // basic format - just assembly
    output << assembly;
  }

  return output.str();
}

std::string disassembler::format_address(uint32_t addr) const {
  std::ostringstream ss;

  if (options_.address_format == "decimal") {
    ss << std::dec << std::setfill(' ') << std::setw(8) << addr;
  } else {
    // default to hex
    ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << addr;
  }

  return ss.str();
}

std::string disassembler::format_hex_bytes(const std::vector<byte>& bytes) const {
  std::ostringstream ss;

  // show bytes in the exact order they appear in the file (little-endian)
  for (size_t i = 0; i < bytes.size(); ++i) {
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(bytes[i]);
  }

  return ss.str();
}

std::string disassembler::format_annotated_line(
    uint32_t addr, const std::vector<byte>& inst_bytes, const std::string& assembly
) const {
  std::ostringstream ss;

  // address column
  if (options_.show_addresses) {
    ss << format_address(addr) << ": ";
  }

  // hex bytes column
  if (options_.show_hex_bytes) {
    ss << format_hex_bytes(inst_bytes);
    ss << "  "; // padding between hex and assembly
  }

  // assembly column
  ss << assembly;

  return ss.str();
}

// convenience functions implementation
namespace disasm {

result<std::string, disasm_error> from_file(const std::string& filename) {
  // try to read as object file first
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    return disasm_error::file_error;
  }

  // read entire file
  std::vector<byte> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  if (file_data.empty()) {
    return disasm_error::empty_input;
  }

  // try parsing as object file
  auto obj_result = object_file::from_binary(file_data);
  if (obj_result.is_ok()) {
    // successfully parsed as object file
    disassembler disasm;
    return disasm.disassemble_object(obj_result.value());
  }

  // failed as object file, try as raw bytes
  if (file_data.size() % 4 != 0) {
    return disasm_error::invalid_size;
  }

  disassembler disasm;
  return disasm.disassemble_bytes(file_data);
}

} // namespace disasm

} // namespace irre::assembler