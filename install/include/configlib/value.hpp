// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace configlib {

/// Runtime type tag for a scalar configuration value.
///
/// `configlib` v0.8 models values as scalar values only: null, boolean,
/// integer, floating point, and string. Object/array formats may be provided
/// later by loaders or plugins, but the core resolved value model remains
/// deliberately small.
enum class ValueType {
    Null,
    Bool,
    Int,
    Double,
    String
};

/// Scalar configuration value used by facts, resolved configs, stores, views, and schemas.
///
/// Conversion functions named `as_*()` never silently fallback; they return
/// `std::optional`. Explicit fallback is only available through `as_*_or()`.
class Value {
public:
    using Storage = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

    Value();
    Value(bool value);
    Value(int value);
    Value(std::int64_t value);
    Value(double value);
    Value(const char* value);
    Value(std::string value);

    [[nodiscard]] ValueType type() const;
    [[nodiscard]] const Storage& storage() const;
    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] std::optional<bool> as_boolean() const;
    [[nodiscard]] std::optional<std::int64_t> as_integer() const;
    [[nodiscard]] std::optional<double> as_floating() const;
    [[nodiscard]] std::optional<std::string> as_string() const;

    [[nodiscard]] bool as_boolean_or(bool fallback) const;
    [[nodiscard]] std::int64_t as_integer_or(std::int64_t fallback) const;
    [[nodiscard]] double as_floating_or(double fallback) const;
    [[nodiscard]] std::string as_string_or(std::string_view fallback) const;

    [[nodiscard]] bool is_null() const;
    [[nodiscard]] bool is_boolean() const;
    [[nodiscard]] bool is_integer() const;
    [[nodiscard]] bool is_floating() const;
    [[nodiscard]] bool is_string() const;

private:
    Storage value_;
};

/// Returns a stable human-readable name for a `ValueType`.
[[nodiscard]] const char* value_type_name(ValueType type);

} // namespace configlib
