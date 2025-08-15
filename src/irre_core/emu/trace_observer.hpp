#pragma once

#include "observer.hpp"
#include "../arch/semantics.hpp"
#include "../arch/encoding.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>

namespace irre::emu {

// trace observer with optional semantic information
class trace_observer : public execution_observer {
public:
  enum class verbosity {
    basic,      // just instruction disassembly
    semantic    // include register/memory values
  };
  
  explicit trace_observer(verbosity level = verbosity::basic) 
    : verbosity_(level) {}

  void pre_execute(const execution_context& ctx) override {
    // always print basic trace
    std::cout << "0x" << std::hex << std::setfill('0') << std::setw(8) 
              << ctx.pc << ": 0x" << std::setw(8) << ctx.raw_instruction 
              << " " << std::dec << format_instruction(ctx.inst);
    
    if (verbosity_ == verbosity::semantic) {
      // capture pre-execution values
      auto flow = semantics::analyze_data_flow(ctx.inst);
      for (reg r : flow.reads) {
        current_.pre_reg_values[r] = ctx.regs.read(r);
      }
    }
    std::cout << std::endl;
  }
  
  void post_execute(const execution_context& ctx) override {
    if (verbosity_ == verbosity::semantic) {
      // capture post-execution values  
      auto flow = semantics::analyze_data_flow(ctx.inst);
      for (reg r : flow.writes) {
        current_.post_reg_values[r] = ctx.regs.read(r);
      }
      
      // format and print semantic info
      print_semantics();
    }
    
    // reset for next instruction
    current_ = {};
  }
  
  void on_memory_read(address addr, word value) override {
    if (verbosity_ == verbosity::semantic) {
      current_.mem_read = {addr, value};
    }
  }
  
  void on_memory_write(address addr, word value) override {
    if (verbosity_ == verbosity::semantic) {
      current_.mem_write = {addr, value};
    }
  }

private:
  verbosity verbosity_;
  
  // track values for current instruction
  struct {
    std::map<reg, word> pre_reg_values;
    std::map<reg, word> post_reg_values;
    std::optional<std::pair<address, word>> mem_read;
    std::optional<std::pair<address, word>> mem_write;
  } current_;
  
  void print_semantics() {
    std::stringstream ss;
    ss << "            ";
    
    // input values (reads)
    bool has_inputs = false;
    if (!current_.pre_reg_values.empty()) {
      ss << "← ";
      bool first = true;
      for (auto& [r, val] : current_.pre_reg_values) {
        if (!first) ss << " ";
        ss << reg_name(r) << "=0x" << std::hex << val;
        first = false;
      }
      has_inputs = true;
    }
    
    // memory reads
    if (current_.mem_read) {
      if (has_inputs) ss << " ";
      else ss << "← ";
      ss << "mem[0x" << std::hex << current_.mem_read->first 
         << "]=0x" << current_.mem_read->second;
      has_inputs = true;
    }
    
    // output values (writes)
    if (!current_.post_reg_values.empty() || current_.mem_write) {
      if (has_inputs) ss << " ";
      ss << "→ ";
      
      bool first = true;
      for (auto& [r, val] : current_.post_reg_values) {
        if (!first) ss << " ";
        ss << reg_name(r) << "=0x" << std::hex << val;
        first = false;
      }
      
      // memory writes
      if (current_.mem_write) {
        if (!first) ss << " ";
        ss << "mem[0x" << std::hex << current_.mem_write->first 
           << "]=0x" << current_.mem_write->second;
      }
    }
    
    std::string result = ss.str();
    if (result != "            ") {
      std::cout << result << std::endl;
    }
  }
};

} // namespace irre::emu