#pragma once

#include "instruction.hpp"
#include "types.hpp"
#include <cstdio>
#include <string>
#include <variant>

namespace irre {

// generic result type
template <typename T, typename E> class result {
private:
  std::variant<T, E> value_;

public:
  result(T value) : value_(std::move(value)) {}
  result(E error) : value_(error) {}

  bool is_ok() const { return std::holds_alternative<T>(value_); }
  bool is_err() const { return std::holds_alternative<E>(value_); }

  const T& value() const { return std::get<T>(value_); }
  T& value() { return std::get<T>(value_); }

  const E& error() const { return std::get<E>(value_); }

  // unwrap with default
  T value_or(T default_val) const { return is_ok() ? value() : std::move(default_val); }

  // map operation
  template <typename F> auto map(F&& f) const -> result<decltype(f(value())), E> {
    if (is_ok()) {
      return f(value());
    } else {
      return error();
    }
  }
};

// instruction formatting
inline std::string format_instruction(const instruction& inst) {
  return std::visit([](const auto& i) -> std::string {
    const auto mnemonic = get_mnemonic(i.op);
    
    if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op>) {
      return mnemonic;
    } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op_reg>) {
      return std::string(mnemonic) + " " + reg_name(i.a);
    } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op_imm24>) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%s 0x%06x", mnemonic, i.addr);
      return buf;
    } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op_reg_imm16>) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%s %s 0x%04x", mnemonic, reg_name(i.a), i.imm);
      return buf;
    } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op_reg_reg>) {
      return std::string(mnemonic) + " " + reg_name(i.a) + " " + reg_name(i.b);
    } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op_reg_reg_imm8>) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%s %s %s 0x%02x", mnemonic, reg_name(i.a), reg_name(i.b), i.offset);
      return buf;
    } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op_reg_imm8x2>) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%s %s 0x%02x 0x%02x", mnemonic, reg_name(i.a), i.v0, i.v1);
      return buf;
    } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>, inst_op_reg_reg_reg>) {
      return std::string(mnemonic) + " " + reg_name(i.a) + " " + reg_name(i.b) + " " + reg_name(i.c);
    }
  }, inst);
}

} // namespace irre