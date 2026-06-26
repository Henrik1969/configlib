// SPDX-License-Identifier: MIT

#pragma once

#include <configlib/key.hpp>
#include <configlib/store.hpp>
#include <configlib/value.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace configlib {

/// Read-only scoped non-owning view into a `ConfigStore` subtree.
///
/// A view borrows store state and must not outlive the store. It is created by
/// `ConfigStore::view()` or explicitly from an existing store and prefix.
class ConfigView {
public:
    ConfigView() = delete;
    ConfigView(const ConfigStore& store, KeyPath prefix);

    [[nodiscard]] const KeyPath& prefix() const;
    [[nodiscard]] std::string prefix_string() const;
    [[nodiscard]] bool empty() const;

    [[nodiscard]] bool contains(const KeyPath& local_key) const;
    [[nodiscard]] std::optional<Value> get_value(const KeyPath& local_key) const;
    [[nodiscard]] std::optional<std::string> get_string(const KeyPath& local_key) const;
    [[nodiscard]] std::optional<std::int64_t> get_integer(const KeyPath& local_key) const;
    [[nodiscard]] std::optional<bool> get_boolean(const KeyPath& local_key) const;
    [[nodiscard]] std::optional<double> get_floating(const KeyPath& local_key) const;

    [[nodiscard]] Value get_value_or(const KeyPath& local_key, const Value& fallback) const;
    [[nodiscard]] std::string get_string_or(const KeyPath& local_key, std::string fallback) const;
    [[nodiscard]] std::int64_t get_integer_or(const KeyPath& local_key, std::int64_t fallback) const;
    [[nodiscard]] bool get_boolean_or(const KeyPath& local_key, bool fallback) const;
    [[nodiscard]] double get_floating_or(const KeyPath& local_key, double fallback) const;

    [[nodiscard]] std::optional<Value> get(const KeyPath& local_key) const;
    [[nodiscard]] std::int64_t get_int(const KeyPath& local_key, std::int64_t fallback = 0) const;
    [[nodiscard]] bool get_bool(const KeyPath& local_key, bool fallback = false) const;
    [[nodiscard]] double get_double(const KeyPath& local_key, double fallback = 0.0) const;
    [[nodiscard]] std::int64_t get_int_or(const KeyPath& local_key, std::int64_t fallback) const;
    [[nodiscard]] bool get_bool_or(const KeyPath& local_key, bool fallback) const;
    [[nodiscard]] double get_double_or(const KeyPath& local_key, double fallback) const;

    [[nodiscard]] std::vector<std::string> keys() const;
    [[nodiscard]] std::string explain(const KeyPath& local_key) const;
    [[nodiscard]] std::string export_config(ExportMode mode = ExportMode::Effective) const;
    [[nodiscard]] std::string export_local_config(ExportMode mode = ExportMode::Effective) const;

private:
    const ConfigStore* store_{nullptr};
    KeyPath prefix_;

    [[nodiscard]] KeyPath qualify(const KeyPath& local_key) const;
    [[nodiscard]] bool owns_full_key(const std::string& dotted) const;
    [[nodiscard]] std::string localize(const std::string& dotted) const;
};

} // namespace configlib
