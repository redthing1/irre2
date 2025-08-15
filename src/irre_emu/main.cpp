#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include "../irre_core/emu/vm.hpp"
#include "../irre_core/emu/devices.hpp"
#include "../irre_core/emu/trace_observer.hpp"
#include "../irre_core/assembler/object.hpp"
#include "../third_party/args_cpp/args.hpp"

namespace {
args::Group arguments("arguments");
args::HelpFlag help_flag(arguments, "help", "show help information", {'h', "help"});
} // namespace

int main(int argc, char* argv[]) {
  args::ArgumentParser parser("irre v2.x emulator", "runs irre object files");
  parser.helpParams.showTerminator = false;

  args::GlobalOptions globals(parser, arguments);
  args::Positional<std::string> object_file(parser, "object", "irre object file (.o)");
  args::Flag debug_flag(parser, "debug", "enable debug output", {'d', "debug"});
  args::Flag trace_flag(parser, "trace", "enable instruction tracing", {'t', "trace"});
  args::Flag semantics_flag(parser, "semantics", "show instruction semantics (requires --trace)", {"semantics"});
  args::ValueFlag<size_t> memory_size(parser, "memory", "memory size in bytes", {'m', "memory"}, 1024 * 1024);
  args::ValueFlag<size_t> max_instructions(
      parser, "max-instructions", "maximum instructions to execute (0=unlimited)", {'L', "max-instructions"}, 0
  );

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

  if (!object_file) {
    std::cerr << "error: object file is required" << std::endl;
    std::cerr << parser;
    return 1;
  }

  // validate semantics flag
  if (semantics_flag && !trace_flag) {
    std::cerr << "error: --semantics requires --trace" << std::endl;
    return 1;
  }

  // read object file
  std::ifstream file(args::get(object_file), std::ios::binary);
  if (!file) {
    std::cerr << "error: cannot open object file: " << args::get(object_file) << std::endl;
    return 1;
  }

  std::vector<irre::byte> binary_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  // parse object file
  auto obj_result = irre::assembler::object_file::from_binary(binary_data);
  if (obj_result.is_err()) {
    std::cerr << "error parsing object file: " << obj_result.error() << std::endl;
    return 1;
  }

  auto obj = obj_result.value();

  // create virtual machine
  irre::emu::vm machine(args::get(memory_size));

  // setup console device
  auto console = std::make_unique<irre::emu::console_device>();
  auto* console_ptr = console.get();

  irre::emu::device_registry devices;
  devices.register_device(irre::emu::device_ids::console, std::move(console));

  // setup callbacks
  machine.on_device_access([&devices](irre::word device_id, irre::word command, irre::word argument) {
    return devices.access_device(device_id, command, argument);
  });

  machine.on_error([](irre::emu::runtime_error err) {
    std::cerr << "runtime error: ";
    switch (err) {
    case irre::emu::runtime_error::invalid_memory_access:
      std::cerr << "invalid memory access";
      break;
    case irre::emu::runtime_error::division_by_zero:
      std::cerr << "division by zero";
      break;
    case irre::emu::runtime_error::invalid_register:
      std::cerr << "invalid register";
      break;
    case irre::emu::runtime_error::invalid_instruction:
      std::cerr << "invalid instruction";
      break;
    case irre::emu::runtime_error::device_error:
      std::cerr << "device error";
      break;
    }
    std::cerr << std::endl;
  });

  if (trace_flag) {
    auto verbosity =
        semantics_flag ? irre::emu::trace_observer::verbosity::semantic : irre::emu::trace_observer::verbosity::basic;
    machine.add_observer(std::make_shared<irre::emu::trace_observer>(verbosity));
  }

  if (debug_flag) {
    std::cout << "loading program with " << obj.code.size() << " bytes of code" << std::endl;
    std::cout << "entry point: 0x" << std::hex << obj.entry_offset << std::dec << std::endl;
    std::cout << "memory size: " << args::get(memory_size) << " bytes" << std::endl;
  }

  // load and run program
  machine.load_program(obj);
  machine.run(args::get(max_instructions));

  if (debug_flag) {
    std::cout << "execution completed" << std::endl;
    std::cout << "final state: ";
    switch (machine.get_execution_state()) {
    case irre::emu::execution_state::running:
      std::cout << "running";
      break;
    case irre::emu::execution_state::halted:
      std::cout << "halted";
      break;
    case irre::emu::execution_state::error:
      std::cout << "error";
      break;
    }
    std::cout << std::endl;
    std::cout << machine.get_stats() << std::endl;

    // dump all registers in neat columns
    std::cout << "registers:" << std::endl;
    for (int i = 0; i < 32; i += 4) {
      std::cout << "  ";
      for (int j = 0; j < 4 && i + j < 32; j++) {
        auto r = static_cast<irre::reg>(i + j);
        auto val = machine.get_register(r);
        std::cout << std::setw(3) << irre::reg_name(r) << "=0x" << std::hex << std::setfill('0') << std::setw(8) << val
                  << std::dec << "  ";
      }
      std::cout << std::endl;
    }
    // special registers
    std::cout << "  ";
    std::cout << "pc =0x" << std::hex << std::setw(8) << machine.get_register(irre::reg::pc) << "  ";
    std::cout << "lr =0x" << std::setw(8) << machine.get_register(irre::reg::lr) << "  ";
    std::cout << "sp =0x" << std::setw(8) << machine.get_register(irre::reg::sp) << "  ";
    std::cout << "ad =0x" << std::setw(8) << machine.get_register(irre::reg::ad) << "  ";
    std::cout << "at =0x" << std::setw(8) << machine.get_register(irre::reg::at) << std::dec << std::endl;
  }

  // output console content if any
  auto output = console_ptr->get_output();
  if (!output.empty()) {
    std::cout << output;
  }

  return (machine.get_execution_state() == irre::emu::execution_state::halted) ? 0 : 1;
}