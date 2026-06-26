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

enum class SchemaPresence {
    Optional,
    Required
};

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

class SchemaValidationResult {
public:
    explicit SchemaValidationResult(DiagnosticLog diagnostics);

    [[nodiscard]] bool ok() const;
    [[nodiscard]] const DiagnosticLog& diagnostics() const;

private:
    DiagnosticLog diagnostics_;
};

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

[[nodiscard]] const char* schema_presence_name(SchemaPresence presence);

} // namespace configlib
