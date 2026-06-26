// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <configlib/diagnostic.hpp>
#include <configlib/fact.hpp>
#include <configlib/policy.hpp>
#include <configlib/value.hpp>

namespace configlib {

/// Resolved value for one key plus provenance and candidate facts.
struct ResolvedEntry {
    KeyPath key;
    Value value;
    Source chosen_source;
    std::vector<Fact> candidates;
};

/// Immutable resolved configuration view produced by `resolve()`.
///
/// Getters named `get_*()` return `std::optional` and never supply implicit
/// fallback. Getters named `get_*_or()` use an explicit caller fallback.
class ResolvedConfig {
public:
    void set(ResolvedEntry entry);

    [[nodiscard]] bool contains(const KeyPath& key) const;
    [[nodiscard]] std::optional<Value> get_value(const KeyPath& key) const;
    [[nodiscard]] std::optional<std::string> get_string(const KeyPath& key) const;
    [[nodiscard]] std::optional<std::int64_t> get_integer(const KeyPath& key) const;
    [[nodiscard]] std::optional<bool> get_boolean(const KeyPath& key) const;
    [[nodiscard]] std::optional<double> get_floating(const KeyPath& key) const;

    [[nodiscard]] Value get_value_or(const KeyPath& key, const Value& fallback) const;
    [[nodiscard]] std::string get_string_or(const KeyPath& key, std::string fallback) const;
    [[nodiscard]] std::int64_t get_integer_or(const KeyPath& key, std::int64_t fallback) const;
    [[nodiscard]] bool get_boolean_or(const KeyPath& key, bool fallback) const;
    [[nodiscard]] double get_floating_or(const KeyPath& key, double fallback) const;

    [[nodiscard]] std::optional<Value> get(const KeyPath& key) const;
    [[nodiscard]] std::int64_t get_int(const KeyPath& key, std::int64_t fallback = 0) const;
    [[nodiscard]] bool get_bool(const KeyPath& key, bool fallback = false) const;
    [[nodiscard]] double get_double(const KeyPath& key, double fallback = 0.0) const;
    [[nodiscard]] const ResolvedEntry* explain(const KeyPath& key) const;
    [[nodiscard]] const std::map<std::string, ResolvedEntry>& entries() const;
    [[nodiscard]] std::string format_explanation(const KeyPath& key) const;

private:
    std::map<std::string, ResolvedEntry> entries_;
};

/// Result of resolving a `FactSet` using a `PolicySet`.
///
/// A result carries both the primary value (`ResolvedConfig`) and diagnostics.
/// `ok()` is false if diagnostics contain errors.
class ResolveResult {
public:
    ResolveResult(ResolvedConfig config, DiagnosticLog diagnostics);

    [[nodiscard]] bool ok() const;
    [[nodiscard]] const ResolvedConfig& config() const;
    [[nodiscard]] const DiagnosticLog& diagnostics() const;

private:
    ResolvedConfig config_;
    DiagnosticLog diagnostics_;
};

/// Resolves raw facts into a `ResolvedConfig` using the supplied policy set.
ResolveResult resolve(const FactSet& facts, const PolicySet& policies);

} // namespace configlib
