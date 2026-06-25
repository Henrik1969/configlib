#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace configlib {

enum class ValueType {
    Null,
    Bool,
    Int,
    Double,
    String
};

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

    [[nodiscard]] bool as_bool(bool fallback = false) const;
    [[nodiscard]] std::int64_t as_int(std::int64_t fallback = 0) const;
    [[nodiscard]] double as_double(double fallback = 0.0) const;
    [[nodiscard]] std::string as_string(std::string fallback = {}) const;

    [[nodiscard]] bool is_null() const;

private:
    Storage value_;
};

[[nodiscard]] const char* value_type_name(ValueType type);

} // namespace configlib
