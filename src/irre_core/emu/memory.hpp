#pragma once

#include "../arch/types.hpp"
#include <vector>
#include <stdexcept>
#include <cstring>

namespace irre::emu {

// memory management for irre vm
class memory {
public:
  explicit memory(size_t size) : data_(size, 0) {}

  // get memory size
  size_t size() const { return data_.size(); }

  // read word (32-bit) from memory with little-endian conversion
  word read_word(address addr) const {
    if (addr + 4 > data_.size()) {
      throw std::out_of_range("memory access out of bounds");
    }

    // little-endian: lsb at lowest address
    return static_cast<word>(data_[addr]) | (static_cast<word>(data_[addr + 1]) << 8) |
           (static_cast<word>(data_[addr + 2]) << 16) | (static_cast<word>(data_[addr + 3]) << 24);
  }

  // write word (32-bit) to memory with little-endian conversion
  void write_word(address addr, word value) {
    if (addr + 4 > data_.size()) {
      throw std::out_of_range("memory access out of bounds");
    }

    // little-endian: lsb at lowest address
    data_[addr] = static_cast<byte>(value & 0xff);
    data_[addr + 1] = static_cast<byte>((value >> 8) & 0xff);
    data_[addr + 2] = static_cast<byte>((value >> 16) & 0xff);
    data_[addr + 3] = static_cast<byte>((value >> 24) & 0xff);
  }

  // read byte from memory
  byte read_byte(address addr) const {
    if (addr >= data_.size()) {
      throw std::out_of_range("memory access out of bounds");
    }
    return data_[addr];
  }

  // write byte to memory
  void write_byte(address addr, byte value) {
    if (addr >= data_.size()) {
      throw std::out_of_range("memory access out of bounds");
    }
    data_[addr] = value;
  }

  // load data into memory starting at address
  void load_data(address addr, const std::vector<byte>& data) {
    if (addr + data.size() > data_.size()) {
      throw std::out_of_range("data too large for memory");
    }
    std::memcpy(&data_[addr], data.data(), data.size());
  }

  // load data from raw pointer
  void load_data(address addr, const byte* data, size_t size) {
    if (addr + size > data_.size()) {
      throw std::out_of_range("data too large for memory");
    }
    std::memcpy(&data_[addr], data, size);
  }

  // get pointer to memory region (c++17 compatible)
  const byte* view(address addr, size_t size) const {
    if (addr + size > data_.size()) {
      throw std::out_of_range("memory view out of bounds");
    }
    return &data_[addr];
  }

  // get mutable pointer to memory region
  byte* view_mut(address addr, size_t size) {
    if (addr + size > data_.size()) {
      throw std::out_of_range("memory view out of bounds");
    }
    return &data_[addr];
  }

  // clear all memory
  void clear() { std::fill(data_.begin(), data_.end(), 0); }

  // check if address range is valid
  bool is_valid_range(address addr, size_t size) const { return addr + size <= data_.size(); }

  // get raw pointer to memory (for debugging/advanced use)
  const byte* raw() const { return data_.data(); }

private:
  std::vector<byte> data_;
};

} // namespace irre::emu