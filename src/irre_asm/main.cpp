#include <iostream>
#include <fstream>
#include <string>
#include "../irre_core/assembler/assembler.hpp"
#include "../third_party/args_cpp/args.hpp"

namespace {
args::Group arguments("arguments");
args::HelpFlag help_flag(arguments, "help", "show help information", {'h', "help"});
} // namespace

int main(int argc, char* argv[]) {
  args::ArgumentParser parser("irre v2.x assembler", "assembles irre assembly source code into object files");
  parser.helpParams.showTerminator = false;

  args::GlobalOptions globals(parser, arguments);
  args::Positional<std::string> input_file(parser, "input", "input assembly file (.asm)");
  args::Positional<std::string> output_file(parser, "output", "output object file (.o, .obj)");

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::Error& e) {
    std::cerr << "error: " << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  if (!input_file || !output_file) {
    std::cerr << "error: both input and output files are required" << std::endl;
    std::cerr << parser;
    return 1;
  }

  // read input file
  std::ifstream file(args::get(input_file));
  if (!file) {
    std::cerr << "error: cannot open input file: " << args::get(input_file) << std::endl;
    return 1;
  }

  std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  // assemble source
  irre::assembler::assembler asm_engine;
  auto result = asm_engine.assemble(source);

  if (result.is_err()) {
    const auto& error = result.error();
    std::cerr << "assembly error at line " << error.line << ", column " << error.column << ": " << error.message
              << std::endl;
    return 1;
  }

  // write output file
  auto obj = result.value();
  auto binary = obj.to_binary();

  std::ofstream out(args::get(output_file), std::ios::binary);
  if (!out) {
    std::cerr << "error: cannot create output file: " << args::get(output_file) << std::endl;
    return 1;
  }

  out.write(reinterpret_cast<const char*>(binary.data()), binary.size());
  out.close();

  std::cout << "assembled " << args::get(input_file) << " -> " << args::get(output_file) << " (" << binary.size()
            << " bytes)" << std::endl;

  return 0;
}