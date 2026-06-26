// SPDX-License-Identifier: MIT

#pragma once

#include <configlib/diagnostic.hpp>
#include <configlib/key.hpp>
#include <configlib/value.hpp>
#include <configlib/view.hpp>

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace configlib {

/// Result of projecting a scoped view into a typed application struct.
template <typename T>
class BindingResult {
public:
    BindingResult(T value, DiagnosticLog diagnostics)
        : value_(std::move(value)), diagnostics_(std::move(diagnostics)) {}

    [[nodiscard]] bool ok() const { return !diagnostics_.has_errors(); }
    [[nodiscard]] const T& value() const { return value_; }
    [[nodiscard]] T& value() { return value_; }
    [[nodiscard]] const DiagnosticLog& diagnostics() const { return diagnostics_; }

private:
    T value_{};
    DiagnosticLog diagnostics_{};
};

/// Explicit typed projection from `ConfigView` into an application struct.
///
/// Bindings are convenience snapshots. They are not the source of governed
/// truth; the `ConfigStore` remains authoritative.
template <typename T>
class StructBinding {
public:
    StructBinding() = default;
    explicit StructBinding(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const std::string& name() const { return name_; }

    StructBinding& string(KeyPath key, std::string T::*member, std::string fallback = {}) {
        add_string(std::move(key), member, std::move(fallback), false);
        return *this;
    }

    StructBinding& required_string(KeyPath key, std::string T::*member) {
        add_string(std::move(key), member, {}, true);
        return *this;
    }

    StructBinding& boolean(KeyPath key, bool T::*member, bool fallback = false) {
        add_bool(std::move(key), member, fallback, false);
        return *this;
    }

    StructBinding& required_boolean(KeyPath key, bool T::*member) {
        add_bool(std::move(key), member, false, true);
        return *this;
    }

    StructBinding& integer(KeyPath key, std::int64_t T::*member, std::int64_t fallback = 0) {
        add_int64(std::move(key), member, fallback, false);
        return *this;
    }

    StructBinding& required_integer(KeyPath key, std::int64_t T::*member) {
        add_int64(std::move(key), member, 0, true);
        return *this;
    }

    StructBinding& integer(KeyPath key, int T::*member, int fallback = 0) {
        add_int(std::move(key), member, fallback, false);
        return *this;
    }

    StructBinding& required_integer(KeyPath key, int T::*member) {
        add_int(std::move(key), member, 0, true);
        return *this;
    }

    StructBinding& floating(KeyPath key, double T::*member, double fallback = 0.0) {
        add_double(std::move(key), member, fallback, false);
        return *this;
    }

    StructBinding& required_floating(KeyPath key, double T::*member) {
        add_double(std::move(key), member, 0.0, true);
        return *this;
    }

    [[nodiscard]] BindingResult<T> read(const ConfigView& view) const {
        T out{};
        DiagnosticLog diagnostics;
        for (const auto& field : fields_) {
            field.apply(view, out, diagnostics);
        }
        return BindingResult<T>(std::move(out), std::move(diagnostics));
    }

private:
    struct Field {
        KeyPath key;
        std::string expected;
        bool required{false};
        std::function<void(const ConfigView&, T&, DiagnosticLog&)> apply;
    };

    std::string name_;
    std::vector<Field> fields_;

    static void report_missing(const KeyPath& key, DiagnosticLog& diagnostics) {
        diagnostics.error("BINDING_REQUIRED_MISSING",
                          "required bound key is missing",
                          key);
    }

    static void report_type_mismatch(const KeyPath& key,
                                     const char* expected,
                                     ValueType actual,
                                     DiagnosticLog& diagnostics) {
        std::ostringstream message;
        message << "bound key expected " << expected << " but got " << value_type_name(actual);
        diagnostics.error("BINDING_TYPE_MISMATCH", message.str(), key);
    }

    void add_string(KeyPath key, std::string T::*member, std::string fallback, bool required) {
        const KeyPath captured_key = std::move(key);
        fields_.push_back({captured_key, "string", required,
            [captured_key, member, fallback = std::move(fallback), required](const ConfigView& view, T& out, DiagnosticLog& diagnostics) {
                auto value = view.get(captured_key);
                if (!value) {
                    if (required) report_missing(captured_key, diagnostics);
                    out.*member = fallback;
                    return;
                }
                if (value->type() != ValueType::String) {
                    report_type_mismatch(captured_key, "string", value->type(), diagnostics);
                    out.*member = fallback;
                    return;
                }
                out.*member = value->as_string().value_or(std::string{});
            }});
    }

    void add_bool(KeyPath key, bool T::*member, bool fallback, bool required) {
        const KeyPath captured_key = std::move(key);
        fields_.push_back({captured_key, "bool", required,
            [captured_key, member, fallback, required](const ConfigView& view, T& out, DiagnosticLog& diagnostics) {
                auto value = view.get(captured_key);
                if (!value) {
                    if (required) report_missing(captured_key, diagnostics);
                    out.*member = fallback;
                    return;
                }
                if (value->type() != ValueType::Bool) {
                    report_type_mismatch(captured_key, "bool", value->type(), diagnostics);
                    out.*member = fallback;
                    return;
                }
                out.*member = value->as_boolean().value_or(false);
            }});
    }

    void add_int64(KeyPath key, std::int64_t T::*member, std::int64_t fallback, bool required) {
        const KeyPath captured_key = std::move(key);
        fields_.push_back({captured_key, "int", required,
            [captured_key, member, fallback, required](const ConfigView& view, T& out, DiagnosticLog& diagnostics) {
                auto value = view.get(captured_key);
                if (!value) {
                    if (required) report_missing(captured_key, diagnostics);
                    out.*member = fallback;
                    return;
                }
                if (value->type() != ValueType::Int) {
                    report_type_mismatch(captured_key, "int", value->type(), diagnostics);
                    out.*member = fallback;
                    return;
                }
                out.*member = value->as_integer().value_or(0);
            }});
    }

    void add_int(KeyPath key, int T::*member, int fallback, bool required) {
        const KeyPath captured_key = std::move(key);
        fields_.push_back({captured_key, "int", required,
            [captured_key, member, fallback, required](const ConfigView& view, T& out, DiagnosticLog& diagnostics) {
                auto value = view.get(captured_key);
                if (!value) {
                    if (required) report_missing(captured_key, diagnostics);
                    out.*member = fallback;
                    return;
                }
                if (value->type() != ValueType::Int) {
                    report_type_mismatch(captured_key, "int", value->type(), diagnostics);
                    out.*member = fallback;
                    return;
                }
                out.*member = static_cast<int>(value->as_integer().value_or(0));
            }});
    }

    void add_double(KeyPath key, double T::*member, double fallback, bool required) {
        const KeyPath captured_key = std::move(key);
        fields_.push_back({captured_key, "double", required,
            [captured_key, member, fallback, required](const ConfigView& view, T& out, DiagnosticLog& diagnostics) {
                auto value = view.get(captured_key);
                if (!value) {
                    if (required) report_missing(captured_key, diagnostics);
                    out.*member = fallback;
                    return;
                }
                if (value->type() == ValueType::Double) {
                    out.*member = value->as_floating().value_or(0.0);
                    return;
                }
                if (value->type() == ValueType::Int) {
                    out.*member = static_cast<double>(value->as_integer().value_or(0));
                    return;
                }
                report_type_mismatch(captured_key, "double", value->type(), diagnostics);
                out.*member = fallback;
            }});
    }
};

} // namespace configlib
