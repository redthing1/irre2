#pragma once

#include "../arch/instruction.hpp"
#include "../arch/encoding.hpp"
#include "../assembler/object.hpp"
#include "memory.hpp"
#include "state.hpp"
#include "execution_visitor.hpp"
#include "observer.hpp"
#include <functional>
#include <vector>
#include <memory>

namespace irre::emu {

// main irre virtual machine
class vm {
public:
  explicit vm(size_t memory_size = 1024 * 1024) // default 1mb
      : memory_(memory_size), state_(), visitor_(memory_, state_) {
    reset();
  }

  // observer management
  void add_observer(std::shared_ptr<execution_observer> observer) { observers_.push_back(std::move(observer)); }

  void remove_all_observers() { observers_.clear(); }

  // load program from object file
  void load_program(const assembler::object_file& obj) {
    // clear memory first
    memory_.clear();

    address current_address = 0;

    // load code section at address 0
    if (!obj.code.empty()) {
      memory_.load_data(current_address, obj.code);
      current_address += static_cast<address>(obj.code.size());
    }

    // load data section immediately after code
    if (!obj.data.empty()) {
      memory_.load_data(current_address, obj.data);
    }

    // set entry point
    state_.registers.set_pc(obj.entry_offset);

    // initialize stack pointer to end of memory
    state_.registers.set_sp(static_cast<word>(memory_.size() - 4));

    // set state to running
    state_.set_state(execution_state::running);
    state_.reset_stats();
  }

  // load raw binary data
  void load_binary(const std::vector<byte>& data, address start_addr = 0) {
    memory_.clear();
    memory_.load_data(start_addr, data);
    state_.registers.set_pc(start_addr);
    state_.registers.set_sp(static_cast<word>(memory_.size() - 4));
    state_.set_state(execution_state::running);
    state_.reset_stats();
  }

  // single step execution
  bool step() {
    if (!state_.is_running()) {
      return false;
    }

    // fetch instruction from pc
    address pc = state_.registers.pc();
    
    // check for misaligned instruction access
    if (pc % 4 != 0) {
      state_.error(error_info(runtime_error::misaligned_instruction, pc, 0, 
                            "instruction fetch at unaligned address"));
      return false;
    }
    
    if (!memory_.is_valid_range(pc, 4)) {
      state_.error(runtime_error::invalid_memory_access, pc);
      return false;
    }

    word instruction_word = memory_.read_word(pc);

    // decode instruction
    auto decode_result = codec::decode(instruction_word);
    if (decode_result.is_err()) {
      // Format detailed error message for invalid instruction
      char msg[128];
      std::snprintf(msg, sizeof(msg), "invalid instruction: 0x%08x (%02x %02x %02x %02x)", 
                   instruction_word,
                   static_cast<uint8_t>(instruction_word & 0xFF),
                   static_cast<uint8_t>((instruction_word >> 8) & 0xFF),
                   static_cast<uint8_t>((instruction_word >> 16) & 0xFF),
                   static_cast<uint8_t>((instruction_word >> 24) & 0xFF));
      state_.error(error_info(runtime_error::invalid_instruction, pc, instruction_word, msg));
      return false;
    }

    instruction inst = decode_result.value();

    // notify observers before execution
    execution_observer::execution_context ctx{pc, instruction_word, inst, state_.registers, memory_};
    for (auto& observer : observers_) {
      observer->pre_execute(ctx);
    }

    // save pc for potential branching logic
    address next_pc = pc + 4;

    // set observers for visitor and execute instruction
    visitor_.set_observers(&observers_);
    visitor_(inst);

    // if execution didn't change pc (no jumps/branches), advance normally
    if (state_.registers.pc() == pc && state_.is_running()) {
      state_.registers.set_pc(next_pc);
    }

    // notify observers after execution
    for (auto& observer : observers_) {
      observer->post_execute(ctx);
    }

    // update statistics
    state_.inc_instruction_count();

    return state_.is_running();
  }

  // run until halt or error
  void run(size_t max_instructions = 0) {
    size_t count = 0;
    while (step()) {
      if (max_instructions > 0 && ++count >= max_instructions) {
        break;
      }
    }
  }

  // reset vm to initial state
  void reset() {
    state_.registers.clear();
    state_.set_state(execution_state::halted);
    state_.reset_stats();
    memory_.clear();
  }

  // device and system handlers
  void on_interrupt(std::function<void(word)> handler) { state_.on_interrupt = std::move(handler); }

  void on_error(std::function<void(const error_info&)> handler) { state_.on_error = std::move(handler); }

  void on_device_access(std::function<word(word, word, word)> handler) { state_.on_device_access = std::move(handler); }

  // accessors
  const vm_state& get_state() const { return state_; }
  const memory& get_memory() const { return memory_; }

  // mutable accessors for testing/debugging
  vm_state& get_state_mut() { return state_; }
  memory& get_memory_mut() { return memory_; }

  // convenience accessors
  word get_register(reg r) const { return state_.registers.read(r); }
  void set_register(reg r, word value) { state_.registers.write(r, value); }

  word get_pc() const { return state_.registers.pc(); }
  void set_pc(word value) { state_.registers.set_pc(value); }

  execution_state get_execution_state() const { return state_.state; }

  // get execution statistics
  std::string get_stats() const { return state_.get_stats(); }

  // memory access helpers
  word read_memory_word(address addr) const { return memory_.read_word(addr); }
  byte read_memory_byte(address addr) const { return memory_.read_byte(addr); }

  void write_memory_word(address addr, word value) { memory_.write_word(addr, value); }
  void write_memory_byte(address addr, byte value) { memory_.write_byte(addr, value); }

private:
  memory memory_;
  vm_state state_;
  execution_visitor visitor_;
  std::vector<std::shared_ptr<execution_observer>> observers_;
};

} // namespace irre::emu