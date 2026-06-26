// SPDX-License-Identifier: MIT

#include <configlib/schema.hpp>

#include <sstream>
#include <utility>

namespace configlib {

namespace {

void validate_value_against_rule(const SchemaRule& rule,
                                 const Value& value,
                                 DiagnosticLog& diagnostics,
                                 KeyPath reported_key) {
    if (rule.expected_type && value.type() != *rule.expected_type) {
        std::ostringstream message;
        message << "schema expected " << value_type_name(*rule.expected_type)
                << " but got " << value_type_name(value.type());
        diagnostics.error("SCHEMA_TYPE_MISMATCH", message.str(), reported_key);
        return;
    }

    if (value.type() == ValueType::String && !rule.allowed_strings.empty()) {
        const auto text = value.as_string().value_or(std::string{});
        if (!rule.allowed_strings.contains(text)) {
            std::ostringstream message;
            message << "schema value '" << text << "' is not in the allowed set";
            diagnostics.error("SCHEMA_STRING_NOT_ALLOWED", message.str(), reported_key);
        }
    }

    if (value.type() == ValueType::Int) {
        const auto number = value.as_integer().value_or(0);
        if (rule.min_int && number < *rule.min_int) {
            std::ostringstream message;
            message << "schema integer value " << number << " is below minimum " << *rule.min_int;
            diagnostics.error("SCHEMA_INT_BELOW_MIN", message.str(), reported_key);
        }
        if (rule.max_int && number > *rule.max_int) {
            std::ostringstream message;
            message << "schema integer value " << number << " is above maximum " << *rule.max_int;
            diagnostics.error("SCHEMA_INT_ABOVE_MAX", message.str(), reported_key);
        }
    }

    if (value.type() == ValueType::Double || value.type() == ValueType::Int) {
        const auto number = value.type() == ValueType::Double
            ? value.as_floating().value_or(0.0)
            : static_cast<double>(value.as_integer().value_or(0));
        if (rule.min_double && number < *rule.min_double) {
            std::ostringstream message;
            message << "schema numeric value " << number << " is below minimum " << *rule.min_double;
            diagnostics.error("SCHEMA_DOUBLE_BELOW_MIN", message.str(), reported_key);
        }
        if (rule.max_double && number > *rule.max_double) {
            std::ostringstream message;
            message << "schema numeric value " << number << " is above maximum " << *rule.max_double;
            diagnostics.error("SCHEMA_DOUBLE_ABOVE_MAX", message.str(), reported_key);
        }
    }
}

} // namespace

SchemaPathBuilder::SchemaPathBuilder(SchemaRule& rule) : rule_(&rule) {}

SchemaPathBuilder& SchemaPathBuilder::type(ValueType type) {
    rule_->expected_type = type;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::boolean() { return type(ValueType::Bool); }
SchemaPathBuilder& SchemaPathBuilder::integer() { return type(ValueType::Int); }
SchemaPathBuilder& SchemaPathBuilder::floating() { return type(ValueType::Double); }
SchemaPathBuilder& SchemaPathBuilder::string() { return type(ValueType::String); }

SchemaPathBuilder& SchemaPathBuilder::required() {
    rule_->presence = SchemaPresence::Required;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::optional() {
    rule_->presence = SchemaPresence::Optional;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::documented_default(Value value) {
    rule_->documented_default_value = std::move(value);
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::default_value(Value value) {
    return documented_default(std::move(value));
}

SchemaPathBuilder& SchemaPathBuilder::allowed(std::vector<std::string> values) {
    rule_->allowed_strings.clear();
    rule_->allowed_strings.insert(values.begin(), values.end());
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::min(std::int64_t value) {
    rule_->min_int = value;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::max(std::int64_t value) {
    rule_->max_int = value;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::range(std::int64_t min_value, std::int64_t max_value) {
    rule_->min_int = min_value;
    rule_->max_int = max_value;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::min(double value) {
    rule_->min_double = value;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::max(double value) {
    rule_->max_double = value;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::range(double min_value, double max_value) {
    rule_->min_double = min_value;
    rule_->max_double = max_value;
    return *this;
}

SchemaPathBuilder& SchemaPathBuilder::describe(std::string text) {
    rule_->description = std::move(text);
    return *this;
}

SchemaValidationResult::SchemaValidationResult(DiagnosticLog diagnostics)
    : diagnostics_(std::move(diagnostics)) {}

bool SchemaValidationResult::ok() const { return !diagnostics_.has_errors(); }
const DiagnosticLog& SchemaValidationResult::diagnostics() const { return diagnostics_; }

SchemaPathBuilder ConfigSchema::path(KeyPath key) {
    const auto dotted = key.dotted();
    auto& rule = rules_[dotted];
    rule.key = std::move(key);
    return SchemaPathBuilder(rule);
}

const SchemaRule* ConfigSchema::find(const KeyPath& key) const {
    auto it = rules_.find(key.dotted());
    if (it == rules_.end()) return nullptr;
    return &it->second;
}

const std::map<std::string, SchemaRule>& ConfigSchema::rules() const { return rules_; }

SchemaValidationResult ConfigSchema::validate(const ResolvedConfig& config) const {
    DiagnosticLog diagnostics;
    for (const auto& [_, rule] : rules_) {
        auto value = config.get(rule.key);
        if (!value) {
            if (rule.presence == SchemaPresence::Required) {
                diagnostics.error("SCHEMA_REQUIRED_MISSING",
                                  "required schema key is missing",
                                  rule.key);
            }
            continue;
        }
        validate_value_against_rule(rule, *value, diagnostics, rule.key);
    }
    return SchemaValidationResult(std::move(diagnostics));
}

SchemaValidationResult ConfigSchema::validate(const ConfigView& view) const {
    DiagnosticLog diagnostics;
    for (const auto& [_, rule] : rules_) {
        auto value = view.get(rule.key);
        if (!value) {
            if (rule.presence == SchemaPresence::Required) {
                diagnostics.error("SCHEMA_REQUIRED_MISSING",
                                  "required schema key is missing",
                                  rule.key);
            }
            continue;
        }
        validate_value_against_rule(rule, *value, diagnostics, rule.key);
    }
    return SchemaValidationResult(std::move(diagnostics));
}

const char* schema_presence_name(SchemaPresence presence) {
    switch (presence) {
        case SchemaPresence::Optional: return "optional";
        case SchemaPresence::Required: return "required";
    }
    return "unknown";
}

} // namespace configlib
