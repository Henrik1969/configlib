#pragma once

#include <configlib/diagnostic.hpp>
#include <configlib/fact.hpp>
#include <configlib/policy.hpp>
#include <configlib/value.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace configlib {

enum class UnknownInputPolicy {
    Ignore,
    Warn,
    Error
};

enum class EnvMappingStyle {
    ExplicitOnly,
    PrefixToDottedLowercase
};

enum class AbsenceAction {
    Ignore,
    Warn,
    Error,
    UseInternalDefaults
};

enum class ConfigFileFormat {
    KeyValue
};

struct EnvVarRule {
    std::string variable;
    KeyPath key;
    std::optional<ValueType> type;
};

class EnvironmentLoaderPolicy {
public:
    EnvironmentLoaderPolicy& enabled(bool value = true);
    EnvironmentLoaderPolicy& prefix(std::string value);
    EnvironmentLoaderPolicy& mapping_style(EnvMappingStyle style);
    EnvironmentLoaderPolicy& unknown_inputs(UnknownInputPolicy policy);
    EnvironmentLoaderPolicy& map(std::string variable, KeyPath key, std::optional<ValueType> type = std::nullopt);

    [[nodiscard]] bool is_enabled() const;
    [[nodiscard]] const std::string& prefix() const;
    [[nodiscard]] EnvMappingStyle mapping_style() const;
    [[nodiscard]] UnknownInputPolicy unknown_inputs() const;
    [[nodiscard]] const std::vector<EnvVarRule>& explicit_rules() const;

private:
    bool enabled_{false};
    std::string prefix_;
    EnvMappingStyle mapping_style_{EnvMappingStyle::ExplicitOnly};
    UnknownInputPolicy unknown_inputs_{UnknownInputPolicy::Ignore};
    std::vector<EnvVarRule> explicit_rules_;
};

struct CliOptionRule {
    std::string option;
    KeyPath key;
    ValueType type{ValueType::String};
    bool takes_value{true};
};

class CliLoaderPolicy {
public:
    CliLoaderPolicy& enabled(bool value = true);
    CliLoaderPolicy& unknown_inputs(UnknownInputPolicy policy);
    CliLoaderPolicy& option(std::string option, KeyPath key, ValueType type = ValueType::String);
    CliLoaderPolicy& flag(std::string option, KeyPath key);

    [[nodiscard]] bool is_enabled() const;
    [[nodiscard]] UnknownInputPolicy unknown_inputs() const;
    [[nodiscard]] const std::vector<CliOptionRule>& rules() const;

private:
    bool enabled_{false};
    UnknownInputPolicy unknown_inputs_{UnknownInputPolicy::Ignore};
    std::vector<CliOptionRule> rules_;
};

class InternalDefaultsProvider {
public:
    InternalDefaultsProvider& set(KeyPath key, Value value);
    InternalDefaultsProvider& set_string(KeyPath key, std::string value);
    InternalDefaultsProvider& set_int(KeyPath key, std::int64_t value);
    InternalDefaultsProvider& set_bool(KeyPath key, bool value);

    [[nodiscard]] const std::map<std::string, Value>& defaults() const;
    [[nodiscard]] bool empty() const;

private:
    std::map<std::string, Value> defaults_;
};

struct FileSearchRule {
    std::string path;
    bool required{false};
};

class FileDiscoveryPolicy {
public:
    FileDiscoveryPolicy& enabled(bool value = true);
    FileDiscoveryPolicy& search_path(std::string path, bool required = false);
    FileDiscoveryPolicy& require_path(std::string path);
    FileDiscoveryPolicy& when_none_found(AbsenceAction action);
    FileDiscoveryPolicy& format(ConfigFileFormat format);
    FileDiscoveryPolicy& internal_defaults(InternalDefaultsProvider provider);

    [[nodiscard]] bool is_enabled() const;
    [[nodiscard]] const std::vector<FileSearchRule>& search_paths() const;
    [[nodiscard]] AbsenceAction absence_action() const;
    [[nodiscard]] ConfigFileFormat format() const;
    [[nodiscard]] const InternalDefaultsProvider& internal_defaults() const;

private:
    bool enabled_{false};
    std::vector<FileSearchRule> search_paths_;
    AbsenceAction absence_action_{AbsenceAction::Ignore};
    ConfigFileFormat format_{ConfigFileFormat::KeyValue};
    InternalDefaultsProvider internal_defaults_;
};

struct DiscoveredConfigFile {
    std::string path;
    ConfigFileFormat format{ConfigFileFormat::KeyValue};
};

struct LoadReport {
    FactSet facts;
    DiagnosticLog diagnostics;

    [[nodiscard]] bool ok() const;
};

struct DiscoveryReport {
    std::vector<DiscoveredConfigFile> files;
    LoadReport load;

    [[nodiscard]] bool ok() const;
};

[[nodiscard]] LoadReport load_internal_defaults(const InternalDefaultsProvider& provider, const PolicySet& policies);
[[nodiscard]] LoadReport load_environment(const EnvironmentLoaderPolicy& env_policy, const PolicySet& policies);
[[nodiscard]] LoadReport load_environment(const EnvironmentLoaderPolicy& env_policy, const PolicySet& policies, const std::map<std::string, std::string>& environment);
[[nodiscard]] LoadReport load_cli(const CliLoaderPolicy& cli_policy, const PolicySet& policies, int argc, const char* const argv[]);
[[nodiscard]] LoadReport load_cli(const CliLoaderPolicy& cli_policy, const PolicySet& policies, const std::vector<std::string>& args);
[[nodiscard]] DiscoveryReport discover_config_files(const FileDiscoveryPolicy& discovery_policy, const PolicySet& policies);
[[nodiscard]] LoadReport load_config_file(const std::string& path, const PolicySet& policies, ConfigFileFormat format = ConfigFileFormat::KeyValue);
[[nodiscard]] LoadReport load_config_files(const std::vector<DiscoveredConfigFile>& files, const PolicySet& policies);

[[nodiscard]] std::optional<Value> parse_value(std::string text, ValueType type);
[[nodiscard]] Value infer_value(std::string text);
[[nodiscard]] const char* unknown_input_policy_name(UnknownInputPolicy policy);
[[nodiscard]] const char* env_mapping_style_name(EnvMappingStyle style);
[[nodiscard]] const char* absence_action_name(AbsenceAction action);
[[nodiscard]] const char* config_file_format_name(ConfigFileFormat format);

} // namespace configlib
