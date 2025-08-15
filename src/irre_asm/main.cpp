#include <iostream>
#include <fstream>
#include <string>
#include "../irre_core/assembler/assembler.hpp"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <input.asm> <output.obj>" << std::endl;
    return 1;
  }

  const std::string input_file = argv[1];
  const std::string output_file = argv[2];

  // read input file
  std::ifstream file(input_file);
  if (!file) {
    std::cerr << "error: cannot open input file: " << input_file << std::endl;
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

  std::ofstream out(output_file, std::ios::binary);
  if (!out) {
    std::cerr << "error: cannot create output file: " << output_file << std::endl;
    return 1;
  }

  out.write(reinterpret_cast<const char*>(binary.data()), binary.size());
  out.close();

  std::cout << "assembled " << input_file << " -> " << output_file << " (" << binary.size() << " bytes)" << std::endl;

  return 0;
}