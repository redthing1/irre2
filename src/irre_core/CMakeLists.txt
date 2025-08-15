add_library(irre_core STATIC 
  irre.cpp irre.hpp
  assembler/assembler.cpp
  assembler/assembler.hpp
  assembler/grammar.hpp
  assembler/actions.cpp
  assembler/actions.hpp
  assembler/symbols.cpp
  assembler/symbols.hpp
  assembler/object.cpp
  assembler/object.hpp
)

# link with PEGTL for parsing
target_include_directories(irre_core PUBLIC ../third_party/pegtl/include)

