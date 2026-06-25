// SPDX-License-Identifier: MIT

#pragma once

#include <configlib/diagnostic.hpp>
#include <configlib/key.hpp>
#include <configlib/policy.hpp>
#include <configlib/result.hpp>
#include <configlib/source.hpp>
#include <configlib/value.hpp>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace configlib {

enum class ExportMode {
    Effective,
    ChangedOnly,
    RuntimeChangesOnly,
    Redacted
};

struct AccessRule {
    KeyPath key;
    bool runtime_mutable{true};
    bool exportable{true};
    bool secret{false};
    bool resettable{true};
};

class AccessPolicy {
public:
    AccessRule& path(KeyPath key);
    AccessPolicy& runtime_mutable(KeyPath key, bool value);
    AccessPolicy& exportable(KeyPath key, bool value);
    AccessPolicy& secret(KeyPath key, bool value = true);
    AccessPolicy& resettable(KeyPath key, bool value);

    [[nodiscard]] const AccessRule* find(const KeyPath& key) const;
    [[nodiscard]] bool is_runtime_mutable(const KeyPath& key) const;
    [[nodiscard]] bool is_exportable(const KeyPath& key) const;
    [[nodiscard]] bool is_secret(const KeyPath& key) const;
    [[nodiscard]] bool is_resettable(const KeyPath& key) const;
    [[nodiscard]] const std::map<std::string, AccessRule>& rules() const;

private:
    std::map<std::string, AccessRule> rules_;
};

class ConfigStore;
class ConfigView;

class ConfigTransaction {
public:
    explicit ConfigTransaction(ConfigStore& store);

    ConfigTransaction& set(KeyPath key, Value value);
    ConfigTransaction& erase(KeyPath key);
    ConfigTransaction& reset_to_base(KeyPath key);
    ConfigTransaction& reset_to_default(KeyPath key);

    [[nodiscard]] bool validate() const;
    [[nodiscard]] const DiagnosticLog& diagnostics() const;

    bool commit();
    void rollback();

private:
    ConfigStore* store_{nullptr};
    std::map<std::string, std::pair<KeyPath, Value>> staged_sets_;
    std::set<std::string> staged_erases_;
    std::set<std::string> staged_reset_base_;
    std::set<std::string> staged_reset_default_;
    mutable DiagnosticLog diagnostics_;

    friend class ConfigStore;
class ConfigView;
};

class ConfigStore {
public:
    ConfigStore() = default;

    static ConfigStore from_result(ResolveResult result, PolicySet policies = {}, AccessPolicy access = {});
    static ConfigStore from_layers(ResolvedConfig defaults, ResolvedConfig base, PolicySet policies = {}, AccessPolicy access = {});

    [[nodiscard]] bool contains(const KeyPath& key) const;
    [[nodiscard]] std::optional<Value> get(const KeyPath& key) const;
    [[nodiscard]] std::string get_string(const KeyPath& key, std::string fallback = {}) const;
    [[nodiscard]] std::int64_t get_int(const KeyPath& key, std::int64_t fallback = 0) const;
    [[nodiscard]] bool get_bool(const KeyPath& key, bool fallback = false) const;

    [[nodiscard]] bool has_runtime_change(const KeyPath& key) const;
    [[nodiscard]] std::string explain(const KeyPath& key) const;
    [[nodiscard]] std::string export_config(ExportMode mode = ExportMode::Effective) const;
    [[nodiscard]] DiagnosticLog diagnostics() const;
    [[nodiscard]] ConfigView view(KeyPath prefix) const;

    ConfigTransaction begin_transaction();

    [[nodiscard]] const ResolvedConfig& base_config() const;
    [[nodiscard]] const ResolvedConfig& defaults_config() const;
    [[nodiscard]] const PolicySet& policies() const;
    [[nodiscard]] const AccessPolicy& access_policy() const;

private:
    ResolvedConfig defaults_;
    ResolvedConfig base_;
    PolicySet policies_;
    AccessPolicy access_;
    std::map<std::string, std::pair<KeyPath, Value>> runtime_overrides_;
    std::set<std::string> runtime_erases_;
    DiagnosticLog diagnostics_;

    [[nodiscard]] std::optional<Value> default_value_for(const KeyPath& key) const;
    [[nodiscard]] bool validate_mutation(const KeyPath& key, const std::optional<Value>& value, DiagnosticLog& diagnostics) const;
    bool apply_transaction(const ConfigTransaction& tx);

    friend class ConfigTransaction;
    friend class ConfigView;
};

[[nodiscard]] const char* export_mode_name(ExportMode mode);

} // namespace configlib
