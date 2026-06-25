#include <configlib/value.hpp>

#include <sstream>

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

bool Value::as_bool(bool fallback) const {
    if (auto ptr = std::get_if<bool>(&value_)) return *ptr;
    return fallback;
}

std::int64_t Value::as_int(std::int64_t fallback) const {
    if (auto ptr = std::get_if<std::int64_t>(&value_)) return *ptr;
    return fallback;
}

double Value::as_double(double fallback) const {
    if (auto ptr = std::get_if<double>(&value_)) return *ptr;
    if (auto iptr = std::get_if<std::int64_t>(&value_)) return static_cast<double>(*iptr);
    return fallback;
}

std::string Value::as_string(std::string fallback) const {
    if (auto ptr = std::get_if<std::string>(&value_)) return *ptr;
    return fallback;
}

bool Value::is_null() const { return std::holds_alternative<std::monostate>(value_); }

const char* value_type_name(ValueType type) {
    switch (type) {
        case ValueType::Null: return "null";
        case ValueType::Bool: return "bool";
        case ValueType::Int: return "int";
        case ValueType::Double: return "double";
        case ValueType::String: return "string";
    }
    return "unknown";
}

} // namespace configlib
