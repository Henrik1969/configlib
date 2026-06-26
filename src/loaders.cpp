// SPDX-License-Identifier: MIT

#include <configlib/loaders.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>

extern char** environ;

namespace configlib {

namespace {

std::string lower_copy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string trim_copy(std::string text) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    auto first = std::find_if(text.begin(), text.end(), not_space);
    if (first == text.end()) return {};
    auto last = std::find_if(text.rbegin(), text.rend(), not_space).base();
    return std::string(first, last);
}

bool starts_with_comment(const std::string& text) {
    return text.starts_with('#') || text.starts_with(';');
}

std::string strip_inline_comment(std::string text) {
    bool in_single = false;
    bool in_double = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '\\' && i + 1 < text.size()) { ++i; continue; }
        if (c == '\'' && !in_double) in_single = !in_single;
        if (c == '"' && !in_single) in_double = !in_double;
        if (!in_single && !in_double && (c == '#' || c == ';')) return trim_copy(text.substr(0, i));
    }
    return trim_copy(std::move(text));
}

std::string unquote_copy(std::string text) {
    text = trim_copy(std::move(text));
    if (text.size() >= 2) {
        const char first = text.front();
        const char last = text.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            std::string out;
            out.reserve(text.size() - 2);
            for (std::size_t i = 1; i + 1 < text.size(); ++i) {
                if (text[i] == '\\' && i + 2 < text.size()) {
                    const char next = text[++i];
                    switch (next) {
                        case 'n': out.push_back('\n'); break;
                        case 't': out.push_back('\t'); break;
                        case 'r': out.push_back('\r'); break;
                        default: out.push_back(next); break;
                    }
                } else {
                    out.push_back(text[i]);
                }
            }
            return out;
        }
    }
    return text;
}

std::string env_suffix_to_key(std::string suffix) {
    suffix = lower_copy(std::move(suffix));
    std::replace(suffix.begin(), suffix.end(), '_', '.');
    return suffix;
}

void complain_unknown(DiagnosticLog& diagnostics, UnknownInputPolicy policy, const std::string& code, const std::string& message, Source source = Source{}) {
    if (policy == UnknownInputPolicy::Warn) diagnostics.warning(code, message, KeyPath{}, std::move(source));
    if (policy == UnknownInputPolicy::Error) diagnostics.error(code, message, KeyPath{}, std::move(source));
}

std::optional<ValueType> expected_type_for(const PolicySet& policies, const KeyPath& key) {
    if (const auto* policy = policies.find_key_policy(key)) return policy->expected_type;
    return std::nullopt;
}

void add_env_fact(LoadReport& report, const PolicySet& policies, KeyPath key, const std::string& variable, const std::string& raw, std::optional<ValueType> declared_type) {
    if (!key.valid()) {
        report.diagnostics.error("CONFIG_ENV_BAD_KEY", "environment variable mapped to an invalid dotted key", key, Source::environment(variable));
        return;
    }
    const auto chosen_type = declared_type ? declared_type : expected_type_for(policies, key);
    if (chosen_type) {
        auto parsed = parse_value(raw, *chosen_type);
        if (!parsed) {
            report.diagnostics.error("CONFIG_ENV_PARSE_FAILED", "environment value could not be parsed as expected type", key, Source::environment(variable));
            return;
        }
        report.facts.add(std::move(key), *parsed, Source::environment(variable), policies.precedence_for(SourceKind::Environment));
        return;
    }
    report.facts.add(std::move(key), Value(raw), Source::environment(variable), policies.precedence_for(SourceKind::Environment));
}

const CliOptionRule* find_cli_rule(const std::vector<CliOptionRule>& rules, const std::string& option) {
    auto it = std::find_if(rules.begin(), rules.end(), [&](const CliOptionRule& rule) { return rule.option == option; });
    if (it == rules.end()) return nullptr;
    return &*it;
}

LoadReport load_key_value_file(const std::string& path, const PolicySet& policies) {
    LoadReport report;
    std::ifstream in(path);
    if (!in) {
        report.diagnostics.error("CONFIG_FILE_OPEN_FAILED", "configuration file could not be opened", KeyPath{}, Source::file(path));
        return report;
    }

    std::string section;
    std::string line;
    std::size_t line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        auto text = trim_copy(line);
        if (text.empty() || starts_with_comment(text)) continue;
        text = strip_inline_comment(std::move(text));
        if (text.empty()) continue;

        if (text.front() == '[' && text.back() == ']') {
            section = trim_copy(text.substr(1, text.size() - 2));
            if (section.empty()) {
                report.diagnostics.error("CONFIG_FILE_BAD_SECTION", "empty section name", KeyPath{}, Source::file(path + ":" + std::to_string(line_no)));
            }
            continue;
        }

        const auto eq = text.find('=');
        if (eq == std::string::npos) {
            report.diagnostics.error("CONFIG_FILE_BAD_LINE", "expected key=value", KeyPath{}, Source::file(path + ":" + std::to_string(line_no)));
            continue;
        }

        auto key_text = trim_copy(text.substr(0, eq));
        auto value_text = strip_inline_comment(text.substr(eq + 1));
        if (key_text.empty()) {
            report.diagnostics.error("CONFIG_FILE_BAD_KEY", "empty key", KeyPath{}, Source::file(path + ":" + std::to_string(line_no)));
            continue;
        }

        if (!section.empty() && key_text.find('.') == std::string::npos) key_text = section + "." + key_text;
        KeyPath key(key_text);
        if (!key.valid()) {
            report.diagnostics.error("CONFIG_FILE_BAD_KEY", "invalid dotted key", key, Source::file(path + ":" + std::to_string(line_no)));
            continue;
        }
        Value value;
        if (auto expected = expected_type_for(policies, key)) {
            auto parsed = parse_value(unquote_copy(value_text), *expected);
            if (!parsed) {
                report.diagnostics.error("CONFIG_FILE_PARSE_FAILED", "file value could not be parsed as expected type", key, Source::file(path + ":" + std::to_string(line_no)));
                continue;
            }
            value = *parsed;
        } else {
            value = infer_value(value_text);
        }

        report.facts.add(std::move(key), std::move(value), Source::file(path + ":" + std::to_string(line_no)), policies.precedence_for(SourceKind::File));
    }

    return report;
}

} // namespace

EnvironmentLoaderPolicy& EnvironmentLoaderPolicy::enabled(bool value) { enabled_ = value; return *this; }
EnvironmentLoaderPolicy& EnvironmentLoaderPolicy::prefix(std::string value) { prefix_ = std::move(value); return *this; }
EnvironmentLoaderPolicy& EnvironmentLoaderPolicy::mapping_style(EnvMappingStyle style) { mapping_style_ = style; return *this; }
EnvironmentLoaderPolicy& EnvironmentLoaderPolicy::unknown_inputs(UnknownInputPolicy policy) { unknown_inputs_ = policy; return *this; }
EnvironmentLoaderPolicy& EnvironmentLoaderPolicy::map(std::string variable, KeyPath key, std::optional<ValueType> type) { explicit_rules_.push_back({std::move(variable), std::move(key), type}); return *this; }
bool EnvironmentLoaderPolicy::is_enabled() const { return enabled_; }
const std::string& EnvironmentLoaderPolicy::prefix() const { return prefix_; }
EnvMappingStyle EnvironmentLoaderPolicy::mapping_style() const { return mapping_style_; }
UnknownInputPolicy EnvironmentLoaderPolicy::unknown_inputs() const { return unknown_inputs_; }
const std::vector<EnvVarRule>& EnvironmentLoaderPolicy::explicit_rules() const { return explicit_rules_; }

CliLoaderPolicy& CliLoaderPolicy::enabled(bool value) { enabled_ = value; return *this; }
CliLoaderPolicy& CliLoaderPolicy::unknown_inputs(UnknownInputPolicy policy) { unknown_inputs_ = policy; return *this; }
CliLoaderPolicy& CliLoaderPolicy::option(std::string option_name, KeyPath key, ValueType type) { rules_.push_back({std::move(option_name), std::move(key), type, true}); return *this; }
CliLoaderPolicy& CliLoaderPolicy::flag(std::string option_name, KeyPath key) { rules_.push_back({std::move(option_name), std::move(key), ValueType::Bool, false}); return *this; }
bool CliLoaderPolicy::is_enabled() const { return enabled_; }
UnknownInputPolicy CliLoaderPolicy::unknown_inputs() const { return unknown_inputs_; }
const std::vector<CliOptionRule>& CliLoaderPolicy::rules() const { return rules_; }

InternalDefaultsProvider& InternalDefaultsProvider::set(KeyPath key, Value value) { defaults_[key.dotted()] = std::move(value); return *this; }
InternalDefaultsProvider& InternalDefaultsProvider::set_string(KeyPath key, std::string value) { return set(std::move(key), Value(std::move(value))); }
InternalDefaultsProvider& InternalDefaultsProvider::set_int(KeyPath key, std::int64_t value) { return set(std::move(key), Value(value)); }
InternalDefaultsProvider& InternalDefaultsProvider::set_bool(KeyPath key, bool value) { return set(std::move(key), Value(value)); }
const std::map<std::string, Value>& InternalDefaultsProvider::defaults() const { return defaults_; }
bool InternalDefaultsProvider::empty() const { return defaults_.empty(); }

FileDiscoveryPolicy& FileDiscoveryPolicy::enabled(bool value) { enabled_ = value; return *this; }
FileDiscoveryPolicy& FileDiscoveryPolicy::search_path(std::string path, bool required) { search_paths_.push_back({std::move(path), required}); return *this; }
FileDiscoveryPolicy& FileDiscoveryPolicy::require_path(std::string path) { return search_path(std::move(path), true); }
FileDiscoveryPolicy& FileDiscoveryPolicy::when_none_found(AbsenceAction action) { absence_action_ = action; return *this; }
FileDiscoveryPolicy& FileDiscoveryPolicy::format(ConfigFileFormat file_format) { format_ = file_format; return *this; }
FileDiscoveryPolicy& FileDiscoveryPolicy::internal_defaults(InternalDefaultsProvider provider) { internal_defaults_ = std::move(provider); return *this; }
bool FileDiscoveryPolicy::is_enabled() const { return enabled_; }
const std::vector<FileSearchRule>& FileDiscoveryPolicy::search_paths() const { return search_paths_; }
AbsenceAction FileDiscoveryPolicy::absence_action() const { return absence_action_; }
ConfigFileFormat FileDiscoveryPolicy::format() const { return format_; }
const InternalDefaultsProvider& FileDiscoveryPolicy::internal_defaults() const { return internal_defaults_; }

bool LoadReport::ok() const { return !diagnostics.has_errors(); }
bool DiscoveryReport::ok() const { return load.ok(); }

LoadReport load_internal_defaults(const InternalDefaultsProvider& provider, const PolicySet& policies) {
    LoadReport report;
    for (const auto& [key, value] : provider.defaults()) {
        KeyPath path(key);
        if (!path.valid()) {
            report.diagnostics.error("CONFIG_DEFAULT_BAD_KEY", "internal default has an invalid dotted key", path, Source::internal_default("internal-defaults-provider"));
            continue;
        }
        report.facts.add(std::move(path), value, Source::internal_default("internal-defaults-provider"), policies.precedence_for(SourceKind::InternalDefault));
    }
    return report;
}

LoadReport load_environment(const EnvironmentLoaderPolicy& env_policy, const PolicySet& policies) {
    std::map<std::string, std::string> snapshot;
    if (environ) {
        for (char** current = environ; *current; ++current) {
            std::string item(*current);
            const auto pos = item.find('=');
            if (pos == std::string::npos) continue;
            snapshot.emplace(item.substr(0, pos), item.substr(pos + 1));
        }
    }
    return load_environment(env_policy, policies, snapshot);
}

LoadReport load_environment(const EnvironmentLoaderPolicy& env_policy, const PolicySet& policies, const std::map<std::string, std::string>& environment) {
    LoadReport report;
    if (!env_policy.is_enabled()) return report;

    std::set<std::string> consumed;
    for (const auto& rule : env_policy.explicit_rules()) {
        auto it = environment.find(rule.variable);
        if (it == environment.end()) continue;
        consumed.insert(rule.variable);
        add_env_fact(report, policies, rule.key, rule.variable, it->second, rule.type);
    }

    if (env_policy.mapping_style() == EnvMappingStyle::PrefixToDottedLowercase && !env_policy.prefix().empty()) {
        for (const auto& [name, value] : environment) {
            if (consumed.contains(name)) continue;
            if (!name.starts_with(env_policy.prefix())) continue;
            const auto suffix = name.substr(env_policy.prefix().size());
            if (suffix.empty()) {
                complain_unknown(report.diagnostics, env_policy.unknown_inputs(), "CONFIG_ENV_UNKNOWN", "environment variable has configured prefix but no key suffix", Source::environment(name));
                continue;
            }
            KeyPath key(env_suffix_to_key(suffix));
            add_env_fact(report, policies, key, name, value, std::nullopt);
            consumed.insert(name);
        }
    }

    if (env_policy.unknown_inputs() != UnknownInputPolicy::Ignore && !env_policy.prefix().empty()) {
        for (const auto& [name, ignored] : environment) {
            (void)ignored;
            if (consumed.contains(name)) continue;
            if (!name.starts_with(env_policy.prefix())) continue;
            complain_unknown(report.diagnostics, env_policy.unknown_inputs(), "CONFIG_ENV_UNKNOWN", "environment variable matched prefix but no loader rule consumed it", Source::environment(name));
        }
    }

    return report;
}

LoadReport load_cli(const CliLoaderPolicy& cli_policy, const PolicySet& policies, int argc, const char* const argv[]) {
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc > 0 ? argc : 0));
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i] ? argv[i] : "");
    return load_cli(cli_policy, policies, args);
}

LoadReport load_cli(const CliLoaderPolicy& cli_policy, const PolicySet& policies, const std::vector<std::string>& args) {
    LoadReport report;
    if (!cli_policy.is_enabled()) return report;

    for (std::size_t i = 0; i < args.size(); ++i) {
        std::string token = args[i];
        if (i == 0 && !token.starts_with("-")) continue;
        if (!token.starts_with("-")) {
            complain_unknown(report.diagnostics, cli_policy.unknown_inputs(), "CONFIG_CLI_UNKNOWN", "positional argument was not consumed", Source::cli(token));
            continue;
        }

        std::string option = token;
        std::optional<std::string> inline_value;
        const auto eq = token.find('=');
        if (eq != std::string::npos) {
            option = token.substr(0, eq);
            inline_value = token.substr(eq + 1);
        }

        const auto* rule = find_cli_rule(cli_policy.rules(), option);
        if (!rule) {
            complain_unknown(report.diagnostics, cli_policy.unknown_inputs(), "CONFIG_CLI_UNKNOWN", "CLI option has no loader rule", Source::cli(option));
            continue;
        }

        if (!rule->key.valid()) {
            report.diagnostics.error("CONFIG_CLI_BAD_KEY", "CLI option mapped to an invalid dotted key", rule->key, Source::cli(option));
            continue;
        }

        if (!rule->takes_value) {
            report.facts.add(rule->key, Value(true), Source::cli(option), policies.precedence_for(SourceKind::CLI));
            continue;
        }

        std::string raw;
        if (inline_value) raw = *inline_value;
        else if (i + 1 < args.size()) raw = args[++i];
        else {
            report.diagnostics.error("CONFIG_CLI_MISSING_VALUE", "CLI option expected a value", rule->key, Source::cli(option));
            continue;
        }

        auto parsed = parse_value(raw, rule->type);
        if (!parsed) {
            report.diagnostics.error("CONFIG_CLI_PARSE_FAILED", "CLI value could not be parsed as expected type", rule->key, Source::cli(option));
            continue;
        }
        report.facts.add(rule->key, *parsed, Source::cli(option), policies.precedence_for(SourceKind::CLI));
    }

    return report;
}

DiscoveryReport discover_config_files(const FileDiscoveryPolicy& discovery_policy, const PolicySet& policies) {
    DiscoveryReport report;
    if (!discovery_policy.is_enabled()) return report;

    for (const auto& rule : discovery_policy.search_paths()) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(rule.path, ec)) {
            report.files.push_back({rule.path, discovery_policy.format()});
        } else if (rule.required) {
            report.load.diagnostics.error("CONFIG_FILE_REQUIRED_MISSING", "required configuration file was not found", KeyPath{}, Source::file(rule.path));
        }
    }

    if (!report.files.empty()) {
        auto loaded = load_config_files(report.files, policies);
        for (const auto& fact : loaded.facts.all()) report.load.facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
        for (const auto& diagnostic : loaded.diagnostics.items()) report.load.diagnostics.add(diagnostic);
        return report;
    }

    switch (discovery_policy.absence_action()) {
        case AbsenceAction::Ignore:
            break;
        case AbsenceAction::Warn:
            report.load.diagnostics.warning("CONFIG_FILE_NONE_FOUND", "no configuration files were found");
            break;
        case AbsenceAction::Error:
            report.load.diagnostics.error("CONFIG_FILE_NONE_FOUND", "no configuration files were found");
            break;
        case AbsenceAction::UseInternalDefaults: {
            auto defaults = load_internal_defaults(discovery_policy.internal_defaults(), policies);
            for (const auto& fact : defaults.facts.all()) report.load.facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
            for (const auto& diagnostic : defaults.diagnostics.items()) report.load.diagnostics.add(diagnostic);
            report.load.diagnostics.info("CONFIG_FILE_DEFAULTS_USED", "no configuration files were found; internal defaults were supplied");
            break;
        }
    }

    return report;
}

LoadReport load_config_file(const std::string& path, const PolicySet& policies, ConfigFileFormat format) {
    switch (format) {
        case ConfigFileFormat::KeyValue: return load_key_value_file(path, policies);
    }
    LoadReport report;
    report.diagnostics.error("CONFIG_FILE_FORMAT_UNSUPPORTED", "configuration file format is not supported", KeyPath{}, Source::file(path));
    return report;
}

LoadReport load_config_files(const std::vector<DiscoveredConfigFile>& files, const PolicySet& policies) {
    LoadReport report;
    for (const auto& file : files) {
        auto loaded = load_config_file(file.path, policies, file.format);
        for (const auto& fact : loaded.facts.all()) report.facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
        for (const auto& diagnostic : loaded.diagnostics.items()) report.diagnostics.add(diagnostic);
    }
    return report;
}

std::optional<Value> parse_value(std::string text, ValueType type) {
    switch (type) {
        case ValueType::Null: return Value{};
        case ValueType::String: return Value(std::move(text));
        case ValueType::Bool: {
            auto lowered = lower_copy(trim_copy(text));
            if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") return Value(true);
            if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") return Value(false);
            return std::nullopt;
        }
        case ValueType::Int: {
            try {
                std::size_t used = 0;
                long long value = std::stoll(trim_copy(text), &used, 0);
                if (used != trim_copy(text).size()) return std::nullopt;
                return Value(static_cast<std::int64_t>(value));
            } catch (...) { return std::nullopt; }
        }
        case ValueType::Double: {
            try {
                std::size_t used = 0;
                double value = std::stod(trim_copy(text), &used);
                if (used != trim_copy(text).size()) return std::nullopt;
                return Value(value);
            } catch (...) { return std::nullopt; }
        }
    }
    return std::nullopt;
}

Value infer_value(std::string text) {
    text = unquote_copy(std::move(text));
    if (auto b = parse_value(text, ValueType::Bool)) return *b;
    if (auto i = parse_value(text, ValueType::Int)) return *i;
    if (text.find('.') != std::string::npos) {
        if (auto d = parse_value(text, ValueType::Double)) return *d;
    }
    return Value(std::move(text));
}

const char* unknown_input_policy_name(UnknownInputPolicy policy) {
    switch (policy) {
        case UnknownInputPolicy::Ignore: return "ignore";
        case UnknownInputPolicy::Warn: return "warn";
        case UnknownInputPolicy::Error: return "error";
    }
    return "unknown";
}

const char* env_mapping_style_name(EnvMappingStyle style) {
    switch (style) {
        case EnvMappingStyle::ExplicitOnly: return "explicit-only";
        case EnvMappingStyle::PrefixToDottedLowercase: return "prefix-to-dotted-lowercase";
    }
    return "unknown";
}

const char* absence_action_name(AbsenceAction action) {
    switch (action) {
        case AbsenceAction::Ignore: return "ignore";
        case AbsenceAction::Warn: return "warn";
        case AbsenceAction::Error: return "error";
        case AbsenceAction::UseInternalDefaults: return "use-internal-defaults";
    }
    return "unknown";
}

const char* config_file_format_name(ConfigFileFormat format) {
    switch (format) {
        case ConfigFileFormat::KeyValue: return "key-value";
    }
    return "unknown";
}

} // namespace configlib
