add_library(irre_core STATIC 
  irre.cpp irre.hpp
  util.hpp
  arch/types.hpp
  arch/instruction.hpp
  arch/encoding.hpp
  arch/semantics.hpp
  assembler/assembler.cpp
  assembler/assembler.hpp
  assembler/grammar.hpp
  assembler/actions.cpp
  assembler/actions.hpp
  assembler/symbols.cpp
  assembler/symbols.hpp
  assembler/object.cpp
  assembler/object.hpp
  assembler/disassembler.cpp
  assembler/disassembler.hpp
  emu/memory.hpp
  emu/state.hpp
  emu/execution_visitor.hpp
  emu/vm.hpp
  emu/devices.hpp
)

# link with PEGTL for parsing
target_include_directories(irre_core PUBLIC ../third_party/pegtl/include)

