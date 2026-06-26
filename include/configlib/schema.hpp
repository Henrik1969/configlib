// SPDX-License-Identifier: MIT

#pragma once

#include <configlib/diagnostic.hpp>
#include <configlib/key.hpp>
#include <configlib/result.hpp>
#include <configlib/value.hpp>
#include <configlib/view.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace configlib {

/// Whether a schema key is optional or required.
enum class SchemaPresence {
    Optional,
    Required
};

/// Shape rule for one configuration key.
///
/// Schema rules validate presence, type, allowed strings, numeric bounds, and
/// documentation metadata. Runtime mutability, exportability, and secrecy are
/// deliberately not schema concerns; those belong to `AccessPolicy`.
struct SchemaRule {
    KeyPath key;
    std::optional<ValueType> expected_type;
    SchemaPresence presence{SchemaPresence::Optional};
    std::optional<Value> documented_default_value;
    std::set<std::string> allowed_strings;
    std::optional<std::int64_t> min_int;
    std::optional<std::int64_t> max_int;
    std::optional<double> min_double;
    std::optional<double> max_double;
    std::string description;
};

/// Fluent builder for a `SchemaRule`.
///
/// `documented_default()` records default metadata for documentation. It does
/// not inject facts. Authoritative defaults are facts from providers/loaders.
class SchemaPathBuilder {
public:
    explicit SchemaPathBuilder(SchemaRule& rule);

    SchemaPathBuilder& type(ValueType type);
    SchemaPathBuilder& boolean();
    SchemaPathBuilder& integer();
    SchemaPathBuilder& floating();
    SchemaPathBuilder& string();

    SchemaPathBuilder& required();
    SchemaPathBuilder& optional();
    SchemaPathBuilder& documented_default(Value value);
    SchemaPathBuilder& default_value(Value value);
    SchemaPathBuilder& allowed(std::vector<std::string> values);
    SchemaPathBuilder& min(std::int64_t value);
    SchemaPathBuilder& max(std::int64_t value);
    SchemaPathBuilder& range(std::int64_t min_value, std::int64_t max_value);
    SchemaPathBuilder& min(double value);
    SchemaPathBuilder& max(double value);
    SchemaPathBuilder& range(double min_value, double max_value);
    SchemaPathBuilder& describe(std::string text);

private:
    SchemaRule* rule_{nullptr};
};

/// Result of validating a config or view against a `ConfigSchema`.
class SchemaValidationResult {
public:
    explicit SchemaValidationResult(DiagnosticLog diagnostics);

    [[nodiscard]] bool ok() const;
    [[nodiscard]] const DiagnosticLog& diagnostics() const;

private:
    DiagnosticLog diagnostics_;
};

/// In-code contract describing valid configuration shape.
///
/// A schema validates resolved configuration or scoped views. It does not
/// perform discovery, precedence resolution, runtime mutation, or export policy.
class ConfigSchema {
public:
    SchemaPathBuilder path(KeyPath key);

    [[nodiscard]] const SchemaRule* find(const KeyPath& key) const;
    [[nodiscard]] const std::map<std::string, SchemaRule>& rules() const;

    [[nodiscard]] SchemaValidationResult validate(const ResolvedConfig& config) const;
    [[nodiscard]] SchemaValidationResult validate(const ConfigView& view) const;

private:
    std::map<std::string, SchemaRule> rules_;
};

/// Returns a stable human-readable name for `SchemaPresence`.
[[nodiscard]] const char* schema_presence_name(SchemaPresence presence);

} // namespace configlib
