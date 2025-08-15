#pragma once

#include "../arch/types.hpp"
#include "../arch/instruction.hpp"
#include "state.hpp"
#include "memory.hpp"

namespace irre::emu {

// base class for vm execution observers
class execution_observer {
public:
  virtual ~execution_observer() = default;
  
  // execution context passed to observers
  struct execution_context {
    address pc;
    word raw_instruction;
    const instruction& inst;
    const register_file& regs;
    const memory& mem;
  };
  
  // called before instruction execution
  virtual void pre_execute(const execution_context& ctx) {}
  
  // called after instruction execution
  virtual void post_execute(const execution_context& ctx) {}
  
  // called when memory is read during instruction execution
  virtual void on_memory_read(address addr, word value) {}
  
  // called when memory is written during instruction execution
  virtual void on_memory_write(address addr, word value) {}
  
  // called on runtime errors
  virtual void on_error(runtime_error err) {}
  
  // called when vm halts
  virtual void on_halt() {}
};

} // namespace irre::emu