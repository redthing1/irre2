#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include "irre_core/assembler/disassembler.hpp"
#include "../third_party/args_cpp/args.hpp"

namespace asmr = irre::assembler;
using irre::byte;

namespace {
args::Group arguments("arguments");
args::HelpFlag help_flag(arguments, "help", "show help information", {'h', "help"});
args::Flag version_flag(arguments, "version", "show version information", {'v', "version"});

// format conversion helpers
asmr::disasm_format parse_format(const std::string& format_str) {
  if (format_str == "basic") {
    return asmr::disasm_format::basic;
  } else if (format_str == "annotated") {
    return asmr::disasm_format::annotated;
  } else {
    throw args::ValidationError("unknown format '" + format_str + "'");
  }
}

uint32_t parse_number(const std::string& str) {
  if (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X") {
    return std::stoul(str, nullptr, 16);
  }
  return std::stoul(str, nullptr, 10);
}
} // namespace

int main(int argc, char* argv[]) {
  args::ArgumentParser parser("irre v2.x disassembler", "disassembles irre object files and raw instruction bytes");
  parser.helpParams.showTerminator = false;

  args::GlobalOptions globals(parser, arguments);
  args::Positional<std::string> input_file(parser, "input", "input file (.o, .obj, .bin)");
  args::ValueFlag<std::string> output_file(parser, "output", "output file (default: stdout)", {'o', "output"});
  args::MapFlag<std::string, asmr::disasm_format> format_flag(
      parser, "format", "output format", {'f', "format"},
      {{"basic", asmr::disasm_format::basic}, {"annotated", asmr::disasm_format::annotated}},
      asmr::disasm_format::annotated
  );
  args::Flag no_addresses(parser, "no-addresses", "don't show instruction addresses", {"no-addresses"});
  args::Flag no_hex(parser, "no-hex", "don't show hex bytes", {"no-hex"});
  args::Flag decimal_addr(parser, "decimal-addr", "show addresses in decimal instead of hex", {"decimal-addr"});
  args::ValueFlag<std::string> base_addr(
      parser, "base-addr", "set base address for raw bytes (hex or decimal)", {"base-addr"}
  );

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    std::cout << "\nsupported file formats:\n";
    std::cout << "  - irre object files (.o, .obj)\n";
    std::cout << "  - raw instruction bytes (.bin)\n";
    std::cout << "  - auto-detect based on file content\n\n";
    std::cout << "examples:\n";
    std::cout << "  " << argv[0] << " program.o\n";
    std::cout << "  " << argv[0] << " -f basic --no-addresses code.bin\n";
    std::cout << "  " << argv[0] << " --base-addr 0x1000 -o output.asm firmware.bin\n";
    return 0;
  } catch (args::Error& e) {
    std::cerr << "error: " << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  if (version_flag) {
    std::cout << "irre disassembler v2.0\n";
    std::cout << "part of the irre v2.x toolchain\n";
    return 0;
  }

  if (!input_file) {
    std::cerr << "error: no input file specified\n";
    std::cerr << parser;
    return 1;
  }

  // configure disassembler options
  asmr::disasm_options disasm_opts;
  if (no_addresses) {
    disasm_opts.show_addresses = false;
  }
  if (no_hex) {
    disasm_opts.show_hex_bytes = false;
  }
  if (decimal_addr) {
    disasm_opts.address_format = "decimal";
  }
  if (base_addr) {
    try {
      disasm_opts.base_address = parse_number(args::get(base_addr));
    } catch (const std::exception& e) {
      std::cerr << "error: invalid base address '" << args::get(base_addr) << "'\n";
      return 1;
    }
  }

  // create disassembler with options
  asmr::disassembler disasm(disasm_opts);

  // read file manually to use our configured disassembler
  std::ifstream file(args::get(input_file), std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "error: failed to open file: " << args::get(input_file) << "\n";
    return 1;
  }

  std::vector<irre::byte> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  if (file_data.empty()) {
    std::cerr << "error: empty file: " << args::get(input_file) << "\n";
    return 1;
  }

  // try parsing as object file first
  auto obj_result = asmr::object_file::from_binary(file_data);
  irre::result<std::string, asmr::disasm_error> result = asmr::disasm_error::file_error; // default initialization

  if (obj_result.is_ok()) {
    // disassemble as object file
    result = disasm.disassemble_object(obj_result.value(), args::get(format_flag));
  } else {
    // try as raw bytes
    if (file_data.size() % 4 != 0) {
      std::cerr << "error: file size must be multiple of 4 bytes for raw disassembly\n";
      return 1;
    }
    result = disasm.disassemble_bytes(file_data, args::get(format_flag));
  }

  if (result.is_err()) {
    std::cerr << "error: " << asmr::disasm_error_message(result.error()) << "\n";
    return 1;
  }

  std::string output = result.value();

  // write output
  if (!output_file) {
    // write to stdout
    std::cout << output << std::endl;
  } else {
    // write to file
    std::ofstream outfile(args::get(output_file));
    if (!outfile.is_open()) {
      std::cerr << "error: failed to open output file '" << args::get(output_file) << "'\n";
      return 1;
    }
    outfile << output << std::endl;
    outfile.close();

    if (outfile.fail()) {
      std::cerr << "error: failed to write to output file '" << args::get(output_file) << "'\n";
      return 1;
    }
  }

  return 0;
}