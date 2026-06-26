// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <configlib/key.hpp>
#include <configlib/source.hpp>
#include <configlib/value.hpp>

namespace configlib {

/// Legacy/simple policy for missing keys during resolution.
enum class MissingPolicy {
    Optional,
    Required,
    UseDefault
};

/// Policy for resolving multiple candidate facts for one key.
enum class ConflictPolicy {
    HighestPrecedenceWins,
    RejectConflict,
    FirstWins,
    LastWins
};

/// Per-key resolution policy.
///
/// `PolicySet` governs resolution behavior. Broader structural validation
/// belongs to `ConfigSchema`; runtime/export behavior belongs to `AccessPolicy`.
struct KeyPolicy {
    KeyPath key;
    std::optional<ValueType> expected_type;
    MissingPolicy missing{MissingPolicy::Optional};
    ConflictPolicy conflict{ConflictPolicy::HighestPrecedenceWins};
    std::optional<Value> default_value;
    std::set<std::string> allowed_strings;
    std::optional<std::int64_t> min_int;
    std::optional<std::int64_t> max_int;
};

/// Resolution policy set: source precedence, missing behavior, and conflicts.
///
/// Policy governs how facts are resolved. It is not the schema and not the
/// runtime access/export policy.
class PolicySet {
public:
    PolicySet();

    PolicySet& set_precedence(SourceKind kind, int precedence);
    [[nodiscard]] int precedence_for(SourceKind kind) const;

    KeyPolicy& key(KeyPath key);
    KeyPolicy& require(KeyPath key, ValueType type);
    KeyPolicy& optional(KeyPath key, ValueType type);
    KeyPolicy& defaulted(KeyPath key, Value value);

    PolicySet& allowed_strings(KeyPath key, std::vector<std::string> values);
    PolicySet& int_range(KeyPath key, std::int64_t min_value, std::int64_t max_value);
    PolicySet& conflict(KeyPath key, ConflictPolicy policy);

    [[nodiscard]] const std::map<std::string, KeyPolicy>& key_policies() const;
    [[nodiscard]] const KeyPolicy* find_key_policy(const KeyPath& key) const;

    static PolicySet default_precedence();

private:
    std::map<SourceKind, int> precedence_;
    std::map<std::string, KeyPolicy> key_policies_;
};

/// Returns a stable human-readable name for a `MissingPolicy`.
[[nodiscard]] const char* missing_policy_name(MissingPolicy policy);
/// Returns a stable human-readable name for a `ConflictPolicy`.
[[nodiscard]] const char* conflict_policy_name(ConflictPolicy policy);

} // namespace configlib
