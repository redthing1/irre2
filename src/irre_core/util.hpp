#pragma once

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
} // namespace irre