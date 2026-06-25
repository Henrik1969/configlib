#include <configlib/result.hpp>

#include <algorithm>
#include <map>
#include <set>

namespace configlib {

namespace {

bool same_value(const Value& lhs, const Value& rhs) {
    return lhs.to_string() == rhs.to_string() && lhs.type() == rhs.type();
}

bool validate_value(const KeyPolicy& policy, const Fact& fact, DiagnosticLog& diagnostics) {
    bool ok = true;
    if (policy.expected_type && fact.value.type() != *policy.expected_type) {
        diagnostics.error(
            "CONFIG_TYPE_MISMATCH",
            "expected " + std::string(value_type_name(*policy.expected_type)) +
                " but got " + value_type_name(fact.value.type()),
            fact.key,
            fact.source
        );
        ok = false;
    }

    if (!policy.allowed_strings.empty() && fact.value.type() == ValueType::String) {
        const auto value = fact.value.as_string();
        if (!policy.allowed_strings.contains(value)) {
            diagnostics.error("CONFIG_STRING_NOT_ALLOWED", "value '" + value + "' is not allowed", fact.key, fact.source);
            ok = false;
        }
    }

    if (fact.value.type() == ValueType::Int) {
        const auto value = fact.value.as_int();
        if (policy.min_int && value < *policy.min_int) {
            diagnostics.error("CONFIG_INT_TOO_SMALL", "integer value below configured minimum", fact.key, fact.source);
            ok = false;
        }
        if (policy.max_int && value > *policy.max_int) {
            diagnostics.error("CONFIG_INT_TOO_LARGE", "integer value above configured maximum", fact.key, fact.source);
            ok = false;
        }
    }

    return ok;
}

Fact choose_candidate(std::vector<Fact> candidates, ConflictPolicy conflict, DiagnosticLog& diagnostics) {
    std::sort(candidates.begin(), candidates.end(), [](const Fact& lhs, const Fact& rhs) {
        if (lhs.precedence != rhs.precedence) return lhs.precedence > rhs.precedence;
        return lhs.insertion_order > rhs.insertion_order;
    });

    if (conflict == ConflictPolicy::FirstWins) {
        return *std::min_element(candidates.begin(), candidates.end(), [](const Fact& lhs, const Fact& rhs) {
            return lhs.insertion_order < rhs.insertion_order;
        });
    }

    if (conflict == ConflictPolicy::LastWins) {
        return *std::max_element(candidates.begin(), candidates.end(), [](const Fact& lhs, const Fact& rhs) {
            return lhs.insertion_order < rhs.insertion_order;
        });
    }

    if (conflict == ConflictPolicy::RejectConflict) {
        const auto& first = candidates.front();
        for (const auto& candidate : candidates) {
            if (!same_value(first.value, candidate.value)) {
                diagnostics.error("CONFIG_CONFLICT", "conflicting values for key", candidate.key, candidate.source);
            }
        }
    }

    return candidates.front();
}

} // namespace

ResolveResult resolve(const FactSet& facts, const PolicySet& policies) {
    DiagnosticLog diagnostics;
    ResolvedConfig config;

    std::map<std::string, std::vector<Fact>> grouped;
    for (auto fact : facts.all()) {
        if (fact.role != FactRole::Application) continue;
        if (fact.precedence == 0) fact.precedence = policies.precedence_for(fact.source.kind());
        grouped[fact.key.dotted()].push_back(std::move(fact));
    }

    for (const auto& [key_name, policy] : policies.key_policies()) {
        if (!grouped.contains(key_name)) {
            if (policy.missing == MissingPolicy::Required) {
                diagnostics.error("CONFIG_REQUIRED_MISSING", "required configuration key is missing", policy.key);
            } else if (policy.missing == MissingPolicy::UseDefault && policy.default_value) {
                Fact default_fact{policy.key, *policy.default_value, Source::internal_default("policy-default"), policies.precedence_for(SourceKind::InternalDefault), FactRole::Application, 0};
                grouped[key_name].push_back(default_fact);
            }
        }
    }

    for (auto& [key_name, candidates] : grouped) {
        KeyPath key(key_name);
        const KeyPolicy* policy = policies.find_key_policy(key);
        KeyPolicy implicit_policy;
        implicit_policy.key = key;
        if (!policy) policy = &implicit_policy;

        std::vector<Fact> valid_candidates;
        valid_candidates.reserve(candidates.size());
        for (const auto& candidate : candidates) {
            if (validate_value(*policy, candidate, diagnostics)) {
                valid_candidates.push_back(candidate);
            }
        }
        if (valid_candidates.empty()) continue;

        Fact chosen = choose_candidate(valid_candidates, policy->conflict, diagnostics);
        config.set({chosen.key, chosen.value, chosen.source, valid_candidates});
    }

    return {std::move(config), std::move(diagnostics)};
}

} // namespace configlib
