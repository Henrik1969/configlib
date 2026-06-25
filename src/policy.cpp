// SPDX-License-Identifier: MIT

#include <configlib/policy.hpp>

namespace configlib {

PolicySet::PolicySet() {
    set_precedence(SourceKind::InternalDefault, 10)
       .set_precedence(SourceKind::File, 30)
       .set_precedence(SourceKind::Environment, 50)
       .set_precedence(SourceKind::CLI, 60)
       .set_precedence(SourceKind::Runtime, 70)
       .set_precedence(SourceKind::Unknown, 0);
}

PolicySet& PolicySet::set_precedence(SourceKind kind, int precedence) {
    precedence_[kind] = precedence;
    return *this;
}

int PolicySet::precedence_for(SourceKind kind) const {
    auto it = precedence_.find(kind);
    if (it != precedence_.end()) return it->second;
    return 0;
}

KeyPolicy& PolicySet::key(KeyPath key_path) {
    const auto dotted = key_path.dotted();
    auto [it, inserted] = key_policies_.try_emplace(dotted);
    if (inserted) it->second.key = std::move(key_path);
    return it->second;
}

KeyPolicy& PolicySet::require(KeyPath key_path, ValueType type) {
    auto& kp = key(std::move(key_path));
    kp.expected_type = type;
    kp.missing = MissingPolicy::Required;
    return kp;
}

KeyPolicy& PolicySet::optional(KeyPath key_path, ValueType type) {
    auto& kp = key(std::move(key_path));
    kp.expected_type = type;
    kp.missing = MissingPolicy::Optional;
    return kp;
}

KeyPolicy& PolicySet::defaulted(KeyPath key_path, Value value) {
    auto& kp = key(std::move(key_path));
    kp.expected_type = value.type();
    kp.missing = MissingPolicy::UseDefault;
    kp.default_value = std::move(value);
    return kp;
}

PolicySet& PolicySet::allowed_strings(KeyPath key_path, std::vector<std::string> values) {
    auto& kp = key(std::move(key_path));
    kp.allowed_strings.clear();
    for (auto& value : values) kp.allowed_strings.insert(std::move(value));
    return *this;
}

PolicySet& PolicySet::int_range(KeyPath key_path, std::int64_t min_value, std::int64_t max_value) {
    auto& kp = key(std::move(key_path));
    kp.min_int = min_value;
    kp.max_int = max_value;
    return *this;
}

PolicySet& PolicySet::conflict(KeyPath key_path, ConflictPolicy policy) {
    key(std::move(key_path)).conflict = policy;
    return *this;
}

const std::map<std::string, KeyPolicy>& PolicySet::key_policies() const { return key_policies_; }

const KeyPolicy* PolicySet::find_key_policy(const KeyPath& key_path) const {
    auto it = key_policies_.find(key_path.dotted());
    if (it == key_policies_.end()) return nullptr;
    return &it->second;
}

PolicySet PolicySet::default_precedence() {
    PolicySet out;
    out.precedence_.clear();
    out.set_precedence(SourceKind::InternalDefault, 10)
       .set_precedence(SourceKind::File, 30)
       .set_precedence(SourceKind::Environment, 50)
       .set_precedence(SourceKind::CLI, 60)
       .set_precedence(SourceKind::Runtime, 70)
       .set_precedence(SourceKind::Unknown, 0);
    return out;
}

const char* missing_policy_name(MissingPolicy policy) {
    switch (policy) {
        case MissingPolicy::Optional: return "optional";
        case MissingPolicy::Required: return "required";
        case MissingPolicy::UseDefault: return "use-default";
    }
    return "unknown";
}

const char* conflict_policy_name(ConflictPolicy policy) {
    switch (policy) {
        case ConflictPolicy::HighestPrecedenceWins: return "highest-precedence-wins";
        case ConflictPolicy::RejectConflict: return "reject-conflict";
        case ConflictPolicy::FirstWins: return "first-wins";
        case ConflictPolicy::LastWins: return "last-wins";
    }
    return "unknown";
}

} // namespace configlib
