// SPDX-License-Identifier: MIT

#include <configlib/store.hpp>
#include <configlib/view.hpp>

#include <algorithm>
#include <sstream>
#include <utility>

namespace configlib {

namespace {

bool validate_policy_value(const KeyPolicy* policy, const KeyPath& key, const Value& value, DiagnosticLog& diagnostics) {
    if (!policy) return true;
    bool ok = true;

    if (policy->expected_type && value.type() != *policy->expected_type) {
        diagnostics.error(
            "CONFIG_STORE_TYPE_MISMATCH",
            "expected " + std::string(value_type_name(*policy->expected_type)) + " but got " + value_type_name(value.type()),
            key,
            Source::runtime("transaction")
        );
        ok = false;
    }

    if (!policy->allowed_strings.empty() && value.type() == ValueType::String) {
        const auto text = value.as_string().value_or(std::string{});
        if (!policy->allowed_strings.contains(text)) {
            diagnostics.error("CONFIG_STORE_STRING_NOT_ALLOWED", "value '" + text + "' is not allowed", key, Source::runtime("transaction"));
            ok = false;
        }
    }

    if (value.type() == ValueType::Int) {
        const auto n = value.as_integer().value_or(0);
        if (policy->min_int && n < *policy->min_int) {
            diagnostics.error("CONFIG_STORE_INT_TOO_SMALL", "integer value below configured minimum", key, Source::runtime("transaction"));
            ok = false;
        }
        if (policy->max_int && n > *policy->max_int) {
            diagnostics.error("CONFIG_STORE_INT_TOO_LARGE", "integer value above configured maximum", key, Source::runtime("transaction"));
            ok = false;
        }
    }

    return ok;
}

void write_entry(std::ostringstream& out, const std::string& key, const Value& value, bool secret) {
    out << key << " = ";
    if (secret) out << "<redacted>";
    else out << value.to_string();
    out << '\n';
}

} // namespace

AccessRule& AccessPolicy::path(KeyPath key) {
    const auto dotted = key.dotted();
    auto [it, inserted] = rules_.try_emplace(dotted);
    if (inserted) it->second.key = std::move(key);
    return it->second;
}

AccessPolicy& AccessPolicy::runtime_mutable(KeyPath key, bool value) {
    path(std::move(key)).runtime_mutable = value;
    return *this;
}

AccessPolicy& AccessPolicy::exportable(KeyPath key, bool value) {
    path(std::move(key)).exportable = value;
    return *this;
}

AccessPolicy& AccessPolicy::secret(KeyPath key, bool value) {
    auto& rule = path(std::move(key));
    rule.secret = value;
    if (value) rule.exportable = false;
    return *this;
}

AccessPolicy& AccessPolicy::resettable(KeyPath key, bool value) {
    path(std::move(key)).resettable = value;
    return *this;
}

const AccessRule* AccessPolicy::find(const KeyPath& key) const {
    auto it = rules_.find(key.dotted());
    if (it == rules_.end()) return nullptr;
    return &it->second;
}

bool AccessPolicy::is_runtime_mutable(const KeyPath& key) const {
    const auto* rule = find(key);
    return !rule || rule->runtime_mutable;
}

bool AccessPolicy::is_exportable(const KeyPath& key) const {
    const auto* rule = find(key);
    return !rule || rule->exportable;
}

bool AccessPolicy::is_secret(const KeyPath& key) const {
    const auto* rule = find(key);
    return rule && rule->secret;
}

bool AccessPolicy::is_resettable(const KeyPath& key) const {
    const auto* rule = find(key);
    return !rule || rule->resettable;
}

const std::map<std::string, AccessRule>& AccessPolicy::rules() const { return rules_; }

ConfigTransaction::ConfigTransaction(ConfigStore& store) : store_(&store) {}

ConfigTransaction& ConfigTransaction::set_value(KeyPath key, Value value) {
    const auto dotted = key.dotted();
    staged_sets_[dotted] = {std::move(key), std::move(value)};
    staged_erases_.erase(dotted);
    staged_reset_base_.erase(dotted);
    staged_reset_default_.erase(dotted);
    return *this;
}

ConfigTransaction& ConfigTransaction::set_string(KeyPath key, std::string value) {
    return set_value(std::move(key), Value(std::move(value)));
}

ConfigTransaction& ConfigTransaction::set_integer(KeyPath key, std::int64_t value) {
    return set_value(std::move(key), Value(value));
}

ConfigTransaction& ConfigTransaction::set_boolean(KeyPath key, bool value) {
    return set_value(std::move(key), Value(value));
}

ConfigTransaction& ConfigTransaction::set_floating(KeyPath key, double value) {
    return set_value(std::move(key), Value(value));
}

ConfigTransaction& ConfigTransaction::set(KeyPath key, Value value) {
    return set_value(std::move(key), std::move(value));
}

ConfigTransaction& ConfigTransaction::erase(KeyPath key) {
    const auto dotted = key.dotted();
    staged_sets_.erase(dotted);
    staged_erases_.insert(dotted);
    staged_reset_base_.erase(dotted);
    staged_reset_default_.erase(dotted);
    return *this;
}

ConfigTransaction& ConfigTransaction::reset_to_base(KeyPath key) {
    const auto dotted = key.dotted();
    staged_sets_.erase(dotted);
    staged_erases_.erase(dotted);
    staged_reset_default_.erase(dotted);
    staged_reset_base_.insert(dotted);
    return *this;
}

ConfigTransaction& ConfigTransaction::reset_to_default(KeyPath key) {
    const auto dotted = key.dotted();
    staged_sets_.erase(dotted);
    staged_erases_.erase(dotted);
    staged_reset_base_.erase(dotted);
    staged_reset_default_.insert(dotted);
    return *this;
}

bool ConfigTransaction::validate() const {
    diagnostics_ = DiagnosticLog{};
    if (!store_) {
        diagnostics_.error("CONFIG_STORE_NO_STORE", "transaction has no owning store");
        return false;
    }

    bool ok = true;
    for (const auto& [_, item] : staged_sets_) {
        ok = store_->validate_mutation(item.first, item.second, diagnostics_) && ok;
    }
    for (const auto& dotted : staged_erases_) {
        KeyPath key(dotted);
        ok = store_->validate_mutation(key, std::nullopt, diagnostics_) && ok;
    }
    for (const auto& dotted : staged_reset_base_) {
        KeyPath key(dotted);
        if (!store_->access_.is_resettable(key)) {
            diagnostics_.error("CONFIG_STORE_RESET_DENIED", "key may not be reset", key, Source::runtime("transaction"));
            ok = false;
        }
    }
    for (const auto& dotted : staged_reset_default_) {
        KeyPath key(dotted);
        if (!store_->access_.is_resettable(key)) {
            diagnostics_.error("CONFIG_STORE_RESET_DENIED", "key may not be reset", key, Source::runtime("transaction"));
            ok = false;
        }
        const auto default_value = store_->default_value_for(key);
        if (!default_value) {
            diagnostics_.error("CONFIG_STORE_DEFAULT_MISSING", "cannot reset key to default because no default is known", key, Source::runtime("transaction"));
            ok = false;
        } else {
            ok = store_->validate_mutation(key, default_value, diagnostics_) && ok;
        }
    }
    return ok && !diagnostics_.has_errors();
}

const DiagnosticLog& ConfigTransaction::diagnostics() const { return diagnostics_; }

bool ConfigTransaction::commit() {
    if (!validate()) return false;
    if (!store_) return false;
    return store_->apply_transaction(*this);
}

void ConfigTransaction::rollback() {
    staged_sets_.clear();
    staged_erases_.clear();
    staged_reset_base_.clear();
    staged_reset_default_.clear();
    diagnostics_ = DiagnosticLog{};
}

ConfigStore ConfigStore::from_result(ResolveResult result, PolicySet policies, AccessPolicy access) {
    ConfigStore store;
    store.base_ = result.config();
    store.policies_ = std::move(policies);
    store.access_ = std::move(access);
    store.diagnostics_ = result.diagnostics();

    for (const auto& [_, entry] : store.base_.entries()) {
        if (entry.chosen_source.kind() == SourceKind::InternalDefault) {
            store.defaults_.set(entry);
        }
        for (const auto& candidate : entry.candidates) {
            if (candidate.source.kind() == SourceKind::InternalDefault) {
                store.defaults_.set({candidate.key, candidate.value, candidate.source, {candidate}});
            }
        }
    }
    return store;
}

ConfigStore ConfigStore::from_layers(ResolvedConfig defaults, ResolvedConfig base, PolicySet policies, AccessPolicy access) {
    ConfigStore store;
    store.defaults_ = std::move(defaults);
    store.base_ = std::move(base);
    store.policies_ = std::move(policies);
    store.access_ = std::move(access);
    return store;
}

bool ConfigStore::contains(const KeyPath& key) const { return get_value(key).has_value(); }

std::optional<Value> ConfigStore::get_value(const KeyPath& key) const {
    const auto dotted = key.dotted();
    auto override_it = runtime_overrides_.find(dotted);
    if (override_it != runtime_overrides_.end()) return override_it->second.second;
    if (runtime_erases_.contains(dotted)) return std::nullopt;
    return base_.get_value(key);
}

std::optional<std::string> ConfigStore::get_string(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_string();
}

std::optional<std::int64_t> ConfigStore::get_integer(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_integer();
}

std::optional<bool> ConfigStore::get_boolean(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_boolean();
}

std::optional<double> ConfigStore::get_floating(const KeyPath& key) const {
    auto value = get_value(key);
    if (!value) return std::nullopt;
    return value->as_floating();
}

Value ConfigStore::get_value_or(const KeyPath& key, const Value& fallback) const {
    auto value = get_value(key);
    return value ? *value : fallback;
}

std::string ConfigStore::get_string_or(const KeyPath& key, std::string fallback) const {
    auto value = get_string(key);
    return value ? *value : fallback;
}

std::int64_t ConfigStore::get_integer_or(const KeyPath& key, std::int64_t fallback) const {
    auto value = get_integer(key);
    return value ? *value : fallback;
}

bool ConfigStore::get_boolean_or(const KeyPath& key, bool fallback) const {
    auto value = get_boolean(key);
    return value ? *value : fallback;
}

double ConfigStore::get_floating_or(const KeyPath& key, double fallback) const {
    auto value = get_floating(key);
    return value ? *value : fallback;
}

std::optional<Value> ConfigStore::get(const KeyPath& key) const { return get_value(key); }

std::int64_t ConfigStore::get_int(const KeyPath& key, std::int64_t fallback) const {
    return get_integer_or(key, fallback);
}

bool ConfigStore::get_bool(const KeyPath& key, bool fallback) const {
    return get_boolean_or(key, fallback);
}

bool ConfigStore::has_runtime_change(const KeyPath& key) const {
    const auto dotted = key.dotted();
    return runtime_overrides_.contains(dotted) || runtime_erases_.contains(dotted);
}

std::string ConfigStore::explain(const KeyPath& key) const {
    const auto dotted = key.dotted();
    std::ostringstream out;
    auto runtime_it = runtime_overrides_.find(dotted);
    if (runtime_it != runtime_overrides_.end()) {
        out << dotted << " = " << runtime_it->second.second.to_string() << '\n';
        out << "chosen: runtime:runtime-override\n";
        if (const auto* base = base_.explain(key)) {
            out << "base: " << base->value.to_string() << " from " << base->chosen_source.describe() << '\n';
        }
        return out.str();
    }
    if (runtime_erases_.contains(dotted)) {
        out << dotted << " is erased by runtime override\n";
        return out.str();
    }
    return base_.format_explanation(key);
}

std::string ConfigStore::export_config(ExportMode mode) const {
    std::ostringstream out;
    std::set<std::string> emitted;

    auto emit_key = [&](const KeyPath& key, const Value& value) {
        const auto dotted = key.dotted();
        if (emitted.contains(dotted)) return;
        if (!access_.is_exportable(key) && mode != ExportMode::EffectiveRedacted && mode != ExportMode::ChangedOnlyRedacted && mode != ExportMode::RuntimeChangesOnlyRedacted) return;
        write_entry(out, dotted, value, access_.is_secret(key));
        emitted.insert(dotted);
    };

    if (mode == ExportMode::ChangedOnly || mode == ExportMode::ChangedOnlyRedacted || mode == ExportMode::RuntimeChangesOnly || mode == ExportMode::RuntimeChangesOnlyRedacted) {
        for (const auto& [_, item] : runtime_overrides_) emit_key(item.first, item.second);
        for (const auto& dotted : runtime_erases_) {
            KeyPath key(dotted);
            if (access_.is_exportable(key)) out << dotted << " = <erased>\n";
        }
        return out.str();
    }

    for (const auto& [_, item] : runtime_overrides_) emit_key(item.first, item.second);
    for (const auto& [dotted, entry] : base_.entries()) {
        if (!runtime_erases_.contains(dotted)) emit_key(entry.key, entry.value);
    }
    return out.str();
}

const DiagnosticLog& ConfigStore::diagnostics() const { return diagnostics_; }

ConfigView ConfigStore::view(KeyPath prefix) const { return ConfigView(*this, std::move(prefix)); }

ConfigTransaction ConfigStore::begin_transaction() { return ConfigTransaction(*this); }

const ResolvedConfig& ConfigStore::base_config() const { return base_; }
const ResolvedConfig& ConfigStore::defaults_config() const { return defaults_; }
const PolicySet& ConfigStore::policies() const { return policies_; }
const AccessPolicy& ConfigStore::access_policy() const { return access_; }

std::optional<Value> ConfigStore::default_value_for(const KeyPath& key) const {
    return defaults_.get_value(key);
}

bool ConfigStore::validate_mutation(const KeyPath& key, const std::optional<Value>& value, DiagnosticLog& diagnostics) const {
    bool ok = true;
    if (!access_.is_runtime_mutable(key)) {
        diagnostics.error("CONFIG_STORE_MUTATION_DENIED", "key may not be changed at runtime", key, Source::runtime("transaction"));
        ok = false;
    }
    if (value) ok = validate_policy_value(policies_.find_key_policy(key), key, *value, diagnostics) && ok;
    return ok;
}

bool ConfigStore::apply_transaction(const ConfigTransaction& tx) {
    for (const auto& [dotted, item] : tx.staged_sets_) {
        runtime_overrides_[dotted] = item;
        runtime_erases_.erase(dotted);
    }
    for (const auto& dotted : tx.staged_erases_) {
        runtime_overrides_.erase(dotted);
        runtime_erases_.insert(dotted);
    }
    for (const auto& dotted : tx.staged_reset_base_) {
        runtime_overrides_.erase(dotted);
        runtime_erases_.erase(dotted);
    }
    for (const auto& dotted : tx.staged_reset_default_) {
        KeyPath key(dotted);
        if (auto value = default_value_for(key)) {
            runtime_overrides_[dotted] = {key, *value};
            runtime_erases_.erase(dotted);
        }
    }
    return true;
}

const char* export_mode_name(ExportMode mode) {
    switch (mode) {
        case ExportMode::Effective: return "effective";
        case ExportMode::EffectiveRedacted: return "effective-redacted";
        case ExportMode::ChangedOnly: return "changed-only";
        case ExportMode::ChangedOnlyRedacted: return "changed-only-redacted";
        case ExportMode::RuntimeChangesOnly: return "runtime-changes-only";
        case ExportMode::RuntimeChangesOnlyRedacted: return "runtime-changes-only-redacted";
    }
    return "unknown";
}

} // namespace configlib
