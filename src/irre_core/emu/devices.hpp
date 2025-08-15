#pragma once

#include "../arch/types.hpp"
#include <memory>
#include <unordered_map>
#include <string>

namespace irre::emu {

// abstract base class for vm devices
class device {
public:
  virtual ~device() = default;

  // handle device command
  // device_id: id of the device being accessed
  // command: command to execute
  // argument: argument to the command
  // returns: result value
  virtual word handle_command(word device_id, word command, word argument) = 0;

  // get device name for debugging
  virtual std::string get_name() const = 0;

  // reset device to initial state
  virtual void reset() {}
};

// simple console device for text output
class console_device : public device {
public:
  word handle_command(word device_id, word command, word argument) override {
    switch (command) {
    case 0: // putchar
      output_ += static_cast<char>(argument & 0xFF);
      return 1; // success

    case 1:     // puts - output null-terminated string (not implemented)
      return 0; // not implemented

    case 2: // clear
      output_.clear();
      return 1; // success

    default:
      return 0; // unknown command
    }
  }

  std::string get_name() const override { return "console"; }

  void reset() override { output_.clear(); }

  // get accumulated output
  const std::string& get_output() const { return output_; }

private:
  std::string output_;
};

// null device - does nothing
class null_device : public device {
public:
  word handle_command(word device_id, word command, word argument) override {
    return 0; // always returns 0
  }

  std::string get_name() const override { return "null"; }
};

// device registry for managing devices
class device_registry {
public:
  // register a device with an id
  void register_device(word device_id, std::unique_ptr<device> dev) { devices_[device_id] = std::move(dev); }

  // handle device access
  word access_device(word device_id, word command, word argument) {
    auto it = devices_.find(device_id);
    if (it != devices_.end()) {
      return it->second->handle_command(device_id, command, argument);
    }
    return 0; // device not found
  }

  // check if device exists
  bool has_device(word device_id) const { return devices_.find(device_id) != devices_.end(); }

  // get device by id (for testing/debugging)
  device* get_device(word device_id) {
    auto it = devices_.find(device_id);
    return (it != devices_.end()) ? it->second.get() : nullptr;
  }

  // reset all devices
  void reset_all() {
    for (auto& [id, dev] : devices_) {
      dev->reset();
    }
  }

  // clear all devices
  void clear() { devices_.clear(); }

private:
  std::unordered_map<word, std::unique_ptr<device>> devices_;
};

// standard device ids (conventions)
namespace device_ids {
constexpr word console = 0;
constexpr word timer = 1;
constexpr word input = 2;
constexpr word storage = 3;
} // namespace device_ids

} // namespace irre::emu