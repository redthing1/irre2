#pragma once

#include "../arch/types.hpp"
#include "../arch/instruction.hpp"
#include <array>
#include <functional>

namespace irre::emu {

// execution state of the vm
enum class execution_state {
  running, // vm is executing instructions
  halted,  // vm has halted normally
  error    // vm encountered an error
};

// runtime error types
enum class runtime_error {
  invalid_memory_access,
  division_by_zero,
  invalid_register,
  invalid_instruction,
  device_error
};

// vm register file - manages all 37 irre registers
class register_file {
public:
  register_file() { clear(); }

  // read register value
  word read(reg r) const {
    auto idx = static_cast<size_t>(r);
    if (idx >= registers_.size()) {
      throw std::invalid_argument("invalid register");
    }
    return registers_[idx];
  }

  // write register value
  void write(reg r, word value) {
    auto idx = static_cast<size_t>(r);
    if (idx >= registers_.size()) {
      throw std::invalid_argument("invalid register");
    }
    registers_[idx] = value;
  }

  // convenience accessors for special registers
  word pc() const { return read(reg::pc); }
  void set_pc(word value) { write(reg::pc, value); }

  word lr() const { return read(reg::lr); }
  void set_lr(word value) { write(reg::lr, value); }

  word sp() const { return read(reg::sp); }
  void set_sp(word value) { write(reg::sp, value); }

  word ad() const { return read(reg::ad); }
  void set_ad(word value) { write(reg::ad, value); }

  word at() const { return read(reg::at); }
  void set_at(word value) { write(reg::at, value); }

  // clear all registers to zero
  void clear() { registers_.fill(0); }

  // get raw register array for debugging
  const std::array<word, 37>& raw() const { return registers_; }

private:
  std::array<word, 37> registers_;
};

// vm execution context and state
class vm_state {
public:
  register_file registers;
  execution_state state = execution_state::halted;

  // execution statistics
  size_t instruction_count = 0;
  size_t cycle_count = 0;

  // callback functions for system events
  std::function<void(word)> on_interrupt;
  std::function<void(runtime_error)> on_error;
  std::function<word(word, word, word)> on_device_access;

  // set execution state
  void set_state(execution_state new_state) { state = new_state; }

  // check if vm is running
  bool is_running() const { return state == execution_state::running; }

  // trigger runtime error
  void error(runtime_error err) {
    state = execution_state::error;
    if (on_error) {
      on_error(err);
    }
  }

  // trigger interrupt
  void interrupt(word code) {
    if (on_interrupt) {
      on_interrupt(code);
    }
  }

  // perform device access
  word device_access(word device_id, word command, word argument) {
    if (on_device_access) {
      return on_device_access(device_id, command, argument);
    }
    return 0; // no device handler
  }

  // increment instruction counter
  void inc_instruction_count() {
    ++instruction_count;
    ++cycle_count; // simple 1:1 mapping for now
  }

  // reset statistics
  void reset_stats() {
    instruction_count = 0;
    cycle_count = 0;
  }

  // get execution info string
  std::string get_stats() const {
    return "instructions: " + std::to_string(instruction_count) + ", cycles: " + std::to_string(cycle_count);
  }
};

} // namespace irre::emu