// SPDX-License-Identifier: MIT

#include <configlib/value.hpp>

#include <sstream>
#include <utility>

namespace configlib {

Value::Value() : value_(std::monostate{}) {}
Value::Value(bool value) : value_(value) {}
Value::Value(int value) : value_(static_cast<std::int64_t>(value)) {}
Value::Value(std::int64_t value) : value_(value) {}
Value::Value(double value) : value_(value) {}
Value::Value(const char* value) : value_(std::string(value ? value : "")) {}
Value::Value(std::string value) : value_(std::move(value)) {}

ValueType Value::type() const {
    switch (value_.index()) {
        case 0: return ValueType::Null;
        case 1: return ValueType::Bool;
        case 2: return ValueType::Int;
        case 3: return ValueType::Double;
        case 4: return ValueType::String;
        default: return ValueType::Null;
    }
}

const Value::Storage& Value::storage() const { return value_; }

std::string Value::to_string() const {
    switch (type()) {
        case ValueType::Null: return "null";
        case ValueType::Bool: return std::get<bool>(value_) ? "true" : "false";
        case ValueType::Int: return std::to_string(std::get<std::int64_t>(value_));
        case ValueType::Double: {
            std::ostringstream out;
            out << std::get<double>(value_);
            return out.str();
        }
        case ValueType::String: return std::get<std::string>(value_);
    }
    return {};
}

std::optional<bool> Value::as_boolean() const {
    if (auto ptr = std::get_if<bool>(&value_)) return *ptr;
    return std::nullopt;
}

std::optional<std::int64_t> Value::as_integer() const {
    if (auto ptr = std::get_if<std::int64_t>(&value_)) return *ptr;
    return std::nullopt;
}

std::optional<double> Value::as_floating() const {
    if (auto ptr = std::get_if<double>(&value_)) return *ptr;
    if (auto iptr = std::get_if<std::int64_t>(&value_)) return static_cast<double>(*iptr);
    return std::nullopt;
}

std::optional<std::string> Value::as_string() const {
    if (auto ptr = std::get_if<std::string>(&value_)) return *ptr;
    return std::nullopt;
}

bool Value::as_boolean_or(bool fallback) const { return as_boolean().value_or(fallback); }
std::int64_t Value::as_integer_or(std::int64_t fallback) const { return as_integer().value_or(fallback); }
double Value::as_floating_or(double fallback) const { return as_floating().value_or(fallback); }
std::string Value::as_string_or(std::string_view fallback) const {
    if (auto value = as_string()) return *value;
    return std::string(fallback);
}

bool Value::is_null() const { return std::holds_alternative<std::monostate>(value_); }
bool Value::is_boolean() const { return std::holds_alternative<bool>(value_); }
bool Value::is_integer() const { return std::holds_alternative<std::int64_t>(value_); }
bool Value::is_floating() const { return std::holds_alternative<double>(value_); }
bool Value::is_string() const { return std::holds_alternative<std::string>(value_); }

const char* value_type_name(ValueType type) {
    switch (type) {
        case ValueType::Null: return "null";
        case ValueType::Bool: return "bool";
        case ValueType::Int: return "integer";
        case ValueType::Double: return "floating";
        case ValueType::String: return "string";
    }
    return "unknown";
}

} // namespace configlib
