#include <configlib/fact.hpp>

namespace configlib {

Fact& FactSet::add(KeyPath key, Value value, Source source, int precedence, FactRole role) {
    facts_.push_back({std::move(key), std::move(value), std::move(source), precedence, role, facts_.size()});
    return facts_.back();
}

Fact& FactSet::add_default(KeyPath key, Value value) { return add(std::move(key), std::move(value), Source::internal_default(), 10); }
Fact& FactSet::add_file(KeyPath key, Value value, std::string path) { return add(std::move(key), std::move(value), Source::file(std::move(path)), 30); }
Fact& FactSet::add_env(KeyPath key, Value value, std::string variable_name) { return add(std::move(key), std::move(value), Source::environment(std::move(variable_name)), 50); }
Fact& FactSet::add_cli(KeyPath key, Value value, std::string option_name) { return add(std::move(key), std::move(value), Source::cli(std::move(option_name)), 60); }
Fact& FactSet::add_runtime(KeyPath key, Value value, std::string name) { return add(std::move(key), std::move(value), Source::runtime(std::move(name)), 70); }

const std::vector<Fact>& FactSet::all() const { return facts_; }

std::vector<Fact> FactSet::for_key(const KeyPath& key) const {
    std::vector<Fact> out;
    for (const auto& fact : facts_) {
        if (fact.key == key) out.push_back(fact);
    }
    return out;
}

bool FactSet::empty() const { return facts_.empty(); }
void FactSet::clear() { facts_.clear(); }

const char* fact_role_name(FactRole role) {
    switch (role) {
        case FactRole::Application: return "application";
        case FactRole::MetaConfig: return "meta-config";
        case FactRole::Policy: return "policy";
    }
    return "unknown";
}

} // namespace configlib
