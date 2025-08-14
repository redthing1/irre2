#pragma once

#include "instruction.hpp"
#include <vector>
#include <optional>
#include <string>

namespace irre::semantics {

// data flow analysis results
struct data_flow {
  std::vector<reg> reads;    // registers read by this instruction
  std::vector<reg> writes;   // registers written by this instruction
  bool reads_memory;         // does this instruction read from memory?
  bool writes_memory;        // does this instruction write to memory?
};

// analyze data flow for an instruction
inline data_flow analyze_data_flow(const instruction& inst) {
  return std::visit(
      [](const auto& i) -> data_flow {
        using T = std::decay_t<decltype(i)>;

        if constexpr (std::is_same_v<T, inst_op>) {
          switch (i.op) {
          case opcode::nop:
            return {{}, {}, false, false};
          case opcode::ret:
            // ret reads lr, writes pc and lr (sets lr to 0)
            return {{reg::lr}, {reg::pc, reg::lr}, false, false};
          case opcode::hlt:
            return {{}, {}, false, false};
          default:
            return {{}, {}, false, false};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg>) {
          switch (i.op) {
          case opcode::jmp:
            // jmp reads register for target, writes pc
            return {{i.a}, {reg::pc}, false, false};
          case opcode::cal:
            // cal reads register for target, writes lr and pc
            return {{i.a}, {reg::lr, reg::pc}, false, false};
          default:
            return {{}, {}, false, false};
          }
        } else if constexpr (std::is_same_v<T, inst_op_imm24>) {
          switch (i.op) {
          case opcode::jmi:
            // jump immediate writes pc
            return {{}, {reg::pc}, false, false};
          case opcode::int_:
            // interrupt - implementation defined side effects
            return {{}, {}, false, false};
          default:
            return {{}, {}, false, false};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_imm16>) {
          switch (i.op) {
          case opcode::set:
            // set writes to register
            return {{}, {i.a}, false, false};
          case opcode::sup:
            // sup reads and writes same register
            return {{i.a}, {i.a}, false, false};
          default:
            return {{}, {}, false, false};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg>) {
          switch (i.op) {
          case opcode::mov:
            // mov reads b, writes a
            return {{i.b}, {i.a}, false, false};
          case opcode::not_:
            // not reads b, writes a
            return {{i.b}, {i.a}, false, false};
          case opcode::sxt:
            // sxt reads b, writes a
            return {{i.b}, {i.a}, false, false};
          default:
            return {{}, {}, false, false};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg_imm8>) {
          switch (i.op) {
          case opcode::ldw:
          case opcode::ldb:
            // load reads address register, writes data register, reads memory
            return {{i.b}, {i.a}, true, false};
          case opcode::stw:
          case opcode::stb:
            // store reads both registers, writes memory
            return {{i.a, i.b}, {}, false, true};
          case opcode::bve:
          case opcode::bvn:
            // branch reads address register and comparison register, conditionally writes pc
            return {{i.a, i.b}, {reg::pc}, false, false};
          case opcode::seq:
            // seq reads b, writes a
            return {{i.b}, {i.a}, false, false};
          default:
            return {{}, {}, false, false};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_imm8x2>) {
          switch (i.op) {
          case opcode::sia:
            // sia reads and writes same register
            return {{i.a}, {i.a}, false, false};
          default:
            return {{}, {}, false, false};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg_reg>) {
          switch (i.op) {
          // arithmetic operations: read b and c, write a
          case opcode::add:
          case opcode::sub:
          case opcode::mul:
          case opcode::div:
          case opcode::mod:
          case opcode::and_:
          case opcode::orr:
          case opcode::xor_:
          case opcode::lsh:
          case opcode::ash:
          case opcode::tcu:
          case opcode::tcs:
            return {{i.b, i.c}, {i.a}, false, false};
          case opcode::snd:
            // snd reads all three registers, writes c
            return {{i.a, i.b, i.c}, {i.c}, false, false};
          default:
            return {{}, {}, false, false};
          }
        }

        return {{}, {}, false, false};
      },
      inst
  );
}

// control flow analysis types
enum class control_flow_type {
  sequential,         // normal instruction, pc += 4
  unconditional_jump, // always changes pc
  conditional_branch, // may change pc based on condition
  function_call,      // saves return address and jumps
  function_return,    // returns to saved address
  halt,               // stops execution
  system              // system call, interrupt, etc.
};

struct control_flow {
  control_flow_type type;
  std::optional<reg> target_reg;          // register containing target (for indirect)
  std::optional<uint32_t> target_addr;    // immediate target address (for direct)
  std::optional<reg> condition_reg;       // register checked for branch condition
  std::optional<uint8_t> condition_value; // value compared against for branch
};

// analyze control flow for an instruction
inline control_flow analyze_control_flow(const instruction& inst) {
  return std::visit(
      [](const auto& i) -> control_flow {
        using T = std::decay_t<decltype(i)>;

        if constexpr (std::is_same_v<T, inst_op>) {
          switch (i.op) {
          case opcode::nop:
            return {control_flow_type::sequential, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          case opcode::ret:
            return {control_flow_type::function_return, reg::lr, std::nullopt, std::nullopt, std::nullopt};
          case opcode::hlt:
            return {control_flow_type::halt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          default:
            return {control_flow_type::sequential, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg>) {
          switch (i.op) {
          case opcode::jmp:
            return {control_flow_type::unconditional_jump, i.a, std::nullopt, std::nullopt, std::nullopt};
          case opcode::cal:
            return {control_flow_type::function_call, i.a, std::nullopt, std::nullopt, std::nullopt};
          default:
            return {control_flow_type::sequential, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          }
        } else if constexpr (std::is_same_v<T, inst_op_imm24>) {
          switch (i.op) {
          case opcode::jmi:
            return {control_flow_type::unconditional_jump, std::nullopt, i.addr, std::nullopt, std::nullopt};
          case opcode::int_:
            return {control_flow_type::system, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          default:
            return {control_flow_type::sequential, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg_imm8>) {
          switch (i.op) {
          case opcode::bve:
            return {control_flow_type::conditional_branch, i.a, std::nullopt, i.b, i.offset};
          case opcode::bvn:
            return {control_flow_type::conditional_branch, i.a, std::nullopt, i.b, i.offset};
          default:
            return {control_flow_type::sequential, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg_reg>) {
          switch (i.op) {
          case opcode::snd:
            return {control_flow_type::system, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          default:
            return {control_flow_type::sequential, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
          }
        }

        return {control_flow_type::sequential, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
      },
      inst
  );
}

// generate human-readable description of instruction semantics
inline std::string describe_execution(const instruction& inst) {
  return std::visit(
      [](const auto& i) -> std::string {
        using T = std::decay_t<decltype(i)>;

        if constexpr (std::is_same_v<T, inst_op>) {
          switch (i.op) {
          case opcode::nop:
            return "do nothing";
          case opcode::ret:
            return "return to address in lr";
          case opcode::hlt:
            return "halt execution";
          default:
            return "unknown operation";
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg>) {
          switch (i.op) {
          case opcode::jmp:
            return std::string("jump to address in ") + reg_name(i.a);
          case opcode::cal:
            return std::string("call function at address in ") + reg_name(i.a);
          default:
            return "unknown register operation";
          }
        } else if constexpr (std::is_same_v<T, inst_op_imm24>) {
          switch (i.op) {
          case opcode::jmi:
            return std::string("jump to address 0x") + std::to_string(i.addr);
          case opcode::int_:
            return std::string("raise interrupt ") + std::to_string(i.addr);
          default:
            return "unknown immediate operation";
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_imm16>) {
          switch (i.op) {
          case opcode::set:
            return std::string(reg_name(i.a)) + " = " + std::to_string(i.imm);
          case opcode::sup:
            return std::string("set upper 16 bits of ") + reg_name(i.a) + " to " + std::to_string(i.imm);
          default:
            return "unknown reg+immediate operation";
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg>) {
          switch (i.op) {
          case opcode::mov:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b);
          case opcode::not_:
            return std::string(reg_name(i.a)) + " = ~" + reg_name(i.b);
          case opcode::sxt:
            return std::string(reg_name(i.a)) + " = sign_extend(" + reg_name(i.b) + ")";
          default:
            return "unknown two-register operation";
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg_imm8>) {
          switch (i.op) {
          case opcode::ldw:
            return std::string(reg_name(i.a)) + " = memory[" + reg_name(i.b) + " + " +
                   std::to_string(static_cast<int8_t>(i.offset)) + "]";
          case opcode::stw:
            return std::string("memory[") + reg_name(i.b) + " + " + std::to_string(static_cast<int8_t>(i.offset)) +
                   "] = " + reg_name(i.a);
          case opcode::ldb:
            return std::string(reg_name(i.a)) + " = byte[" + reg_name(i.b) + " + " +
                   std::to_string(static_cast<int8_t>(i.offset)) + "]";
          case opcode::stb:
            return std::string("byte[") + reg_name(i.b) + " + " + std::to_string(static_cast<int8_t>(i.offset)) +
                   "] = " + reg_name(i.a);
          case opcode::bve:
            return std::string("if ") + reg_name(i.b) + " == " + std::to_string(i.offset) + " then jump to " +
                   reg_name(i.a);
          case opcode::bvn:
            return std::string("if ") + reg_name(i.b) + " != " + std::to_string(i.offset) + " then jump to " +
                   reg_name(i.a);
          case opcode::seq:
            return std::string(reg_name(i.a)) + " = (" + reg_name(i.b) + " == " + std::to_string(i.offset) +
                   " ? 1 : 0)";
          default:
            return "unknown reg+reg+immediate operation";
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_imm8x2>) {
          switch (i.op) {
          case opcode::sia:
            return std::string(reg_name(i.a)) + " += (" + std::to_string(i.v0) + " << " + std::to_string(i.v1) + ")";
          default:
            return "unknown reg+two-immediate operation";
          }
        } else if constexpr (std::is_same_v<T, inst_op_reg_reg_reg>) {
          switch (i.op) {
          case opcode::add:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " + " + reg_name(i.c);
          case opcode::sub:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " - " + reg_name(i.c);
          case opcode::mul:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " * " + reg_name(i.c);
          case opcode::div:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " / " + reg_name(i.c);
          case opcode::mod:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " % " + reg_name(i.c);
          case opcode::and_:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " & " + reg_name(i.c);
          case opcode::orr:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " | " + reg_name(i.c);
          case opcode::xor_:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " ^ " + reg_name(i.c);
          case opcode::lsh:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " << " + reg_name(i.c);
          case opcode::ash:
            return std::string(reg_name(i.a)) + " = " + reg_name(i.b) + " >>> " + reg_name(i.c);
          case opcode::tcu:
            return std::string(reg_name(i.a)) + " = unsigned_compare(" + reg_name(i.b) + ", " + reg_name(i.c) + ")";
          case opcode::tcs:
            return std::string(reg_name(i.a)) + " = signed_compare(" + reg_name(i.b) + ", " + reg_name(i.c) + ")";
          case opcode::snd:
            return std::string(reg_name(i.c)) + " = device_send(" + reg_name(i.a) + ", " + reg_name(i.b) + ", " +
                   reg_name(i.c) + ")";
          default:
            return "unknown three-register operation";
          }
        }

        return "unknown instruction";
      },
      inst
  );
}

// utility functions
namespace utils {

// check if instruction reads a specific register
inline bool reads_register(const instruction& inst, reg r) {
  auto flow = analyze_data_flow(inst);
  return std::find(flow.reads.begin(), flow.reads.end(), r) != flow.reads.end();
}

// check if instruction writes a specific register
inline bool writes_register(const instruction& inst, reg r) {
  auto flow = analyze_data_flow(inst);
  return std::find(flow.writes.begin(), flow.writes.end(), r) != flow.writes.end();
}

// check if instruction is a branch/jump
inline bool is_control_flow(const instruction& inst) {
  auto flow = analyze_control_flow(inst);
  return flow.type != control_flow_type::sequential;
}

// check if instruction has side effects (memory, I/O, system)
inline bool has_side_effects(const instruction& inst) {
  auto data = analyze_data_flow(inst);
  auto ctrl = analyze_control_flow(inst);

  return data.reads_memory || data.writes_memory || ctrl.type == control_flow_type::system ||
         ctrl.type == control_flow_type::halt;
}

// get all registers used by instruction (read or write)
inline std::vector<reg> get_all_registers(const instruction& inst) {
  auto flow = analyze_data_flow(inst);
  std::vector<reg> result = flow.reads;

  for (reg written_reg : flow.writes) {
    // avoid duplicates
    if (std::find(result.begin(), result.end(), written_reg) == result.end()) {
      result.push_back(written_reg);
    }
  }

  return result;
}
} // namespace utils
} // namespace irre::semantics