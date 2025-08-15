#pragma once

#include "arch/instruction.hpp"
#include "arch/types.hpp"
#include <cstdio>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

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

// portable byte i/o utilities for endian-safe serialization
namespace byte_io {

// write little-endian values portably
inline void write_u32_le(std::vector<byte>& buffer, uint32_t value) {
  buffer.push_back(static_cast<byte>(value & 0xff)); // lsb
  buffer.push_back(static_cast<byte>((value >> 8) & 0xff));
  buffer.push_back(static_cast<byte>((value >> 16) & 0xff));
  buffer.push_back(static_cast<byte>((value >> 24) & 0xff)); // msb
}

inline void write_u16_le(std::vector<byte>& buffer, uint16_t value) {
  buffer.push_back(static_cast<byte>(value & 0xff));        // lsb
  buffer.push_back(static_cast<byte>((value >> 8) & 0xff)); // msb
}

// read little-endian values portably
inline uint32_t read_u32_le(const byte* data) {
  return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

inline uint16_t read_u16_le(const byte* data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

// magic number utilities
inline void write_magic(std::vector<byte>& buffer) {
  buffer.push_back('R'); // 0x52
  buffer.push_back('G'); // 0x47
  buffer.push_back('V'); // 0x56
  buffer.push_back('M'); // 0x4d
}

inline bool check_magic(const byte* data) {
  return data[0] == 'R' && data[1] == 'G' && data[2] == 'V' && data[3] == 'M';
}

} // namespace byte_io

} // namespace irre