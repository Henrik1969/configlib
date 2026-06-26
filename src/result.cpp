// SPDX-License-Identifier: MIT

#include <configlib/result.hpp>

#include <sstream>

namespace configlib {

void ResolvedConfig::set(ResolvedEntry entry) {
    entries_[entry.key.dotted()] = std::move(entry);
}

bool ResolvedConfig::contains(const KeyPath& key) const { return entries_.contains(key.dotted()); }

std::optional<Value> ResolvedConfig::get_value(const KeyPath& key) const {
    auto it = entries_.find(key.dotted());
    if (it == entries_.end()) return std::nullopt;
    return it->second.value;
}

std::optional<std::string> ResolvedConfig::get_string(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_string();
}

std::optional<std::int64_t> ResolvedConfig::get_integer(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_integer();
}

std::optional<bool> ResolvedConfig::get_boolean(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_boolean();
}

std::optional<double> ResolvedConfig::get_floating(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_floating();
}

Value ResolvedConfig::get_value_or(const KeyPath& key, const Value& fallback) const {
    auto value = get_value(key);
    return value ? *value : fallback;
}

std::string ResolvedConfig::get_string_or(const KeyPath& key, std::string fallback) const {
    auto value = get_string(key);
    return value ? *value : fallback;
}

std::int64_t ResolvedConfig::get_integer_or(const KeyPath& key, std::int64_t fallback) const {
    auto value = get_integer(key);
    return value ? *value : fallback;
}

bool ResolvedConfig::get_boolean_or(const KeyPath& key, bool fallback) const {
    auto value = get_boolean(key);
    return value ? *value : fallback;
}

double ResolvedConfig::get_floating_or(const KeyPath& key, double fallback) const {
    auto value = get_floating(key);
    return value ? *value : fallback;
}

std::optional<Value> ResolvedConfig::get(const KeyPath& key) const { return get_value(key); }

const ResolvedEntry* ResolvedConfig::explain(const KeyPath& key) const {
    auto it = entries_.find(key.dotted());
    if (it == entries_.end()) return nullptr;
    return &it->second;
}

const std::map<std::string, ResolvedEntry>& ResolvedConfig::entries() const { return entries_; }

std::string ResolvedConfig::format_explanation(const KeyPath& key) const {
    const auto* entry = explain(key);
    if (!entry) return "not found: " + key.dotted();

    std::ostringstream out;
    out << entry->key.dotted() << " = " << entry->value.to_string() << '\n';
    out << "chosen: " << entry->chosen_source.describe() << '\n';
    if (entry->candidates.size() > 1) {
        out << "candidates:\n";
        for (const auto& fact : entry->candidates) {
            out << "  - precedence=" << fact.precedence
                << " source=" << fact.source.describe()
                << " value=" << fact.value.to_string() << '\n';
        }
    }
    return out.str();
}

ResolveResult::ResolveResult(ResolvedConfig config, DiagnosticLog diagnostics)
    : config_(std::move(config)), diagnostics_(std::move(diagnostics)) {}

bool ResolveResult::ok() const { return !diagnostics_.has_errors(); }
const ResolvedConfig& ResolveResult::config() const { return config_; }
const DiagnosticLog& ResolveResult::diagnostics() const { return diagnostics_; }

} // namespace configlib
