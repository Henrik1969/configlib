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

struct ResolvedEntry {
    KeyPath key;
    Value value;
    Source chosen_source;
    std::vector<Fact> candidates;
};

class ResolvedConfig {
public:
    void set(ResolvedEntry entry);

    [[nodiscard]] bool contains(const KeyPath& key) const;
    [[nodiscard]] std::optional<Value> get(const KeyPath& key) const;
    [[nodiscard]] std::string get_string(const KeyPath& key, std::string fallback = {}) const;
    [[nodiscard]] std::int64_t get_int(const KeyPath& key, std::int64_t fallback = 0) const;
    [[nodiscard]] bool get_bool(const KeyPath& key, bool fallback = false) const;
    [[nodiscard]] const ResolvedEntry* explain(const KeyPath& key) const;
    [[nodiscard]] const std::map<std::string, ResolvedEntry>& entries() const;
    [[nodiscard]] std::string format_explanation(const KeyPath& key) const;

private:
    std::map<std::string, ResolvedEntry> entries_;
};

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

ResolveResult resolve(const FactSet& facts, const PolicySet& policies);

} // namespace configlib
