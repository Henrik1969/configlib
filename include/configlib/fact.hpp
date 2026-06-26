// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <vector>

#include <configlib/key.hpp>
#include <configlib/source.hpp>
#include <configlib/value.hpp>

namespace configlib {

/// Role of a fact in the configuration pipeline.
enum class FactRole {
    Application,
    MetaConfig,
    Policy
};

/// One raw configuration claim before resolution.
///
/// A fact is a key/value claim with source provenance, precedence, role, and
/// insertion order. Resolution policies decide which fact wins for a key.
struct Fact {
    KeyPath key;
    Value value;
    Source source;
    int precedence{0};
    FactRole role{FactRole::Application};
    std::size_t insertion_order{0};
};

/// Ordered collection of raw configuration facts.
///
/// Loaders and providers produce facts. The resolver consumes facts together
/// with policy to create a `ResolvedConfig`.
class FactSet {
public:
    Fact& add(KeyPath key, Value value, Source source, int precedence, FactRole role = FactRole::Application);
    Fact& add_default(KeyPath key, Value value);
    Fact& add_file(KeyPath key, Value value, std::string path);
    Fact& add_env(KeyPath key, Value value, std::string variable_name);
    Fact& add_cli(KeyPath key, Value value, std::string option_name);
    Fact& add_runtime(KeyPath key, Value value, std::string name = "runtime");

    [[nodiscard]] const std::vector<Fact>& all() const;
    [[nodiscard]] std::vector<Fact> for_key(const KeyPath& key) const;
    [[nodiscard]] bool empty() const;
    void clear();

private:
    std::vector<Fact> facts_;
};

/// Returns a stable human-readable name for a `FactRole`.
[[nodiscard]] const char* fact_role_name(FactRole role);

} // namespace configlib
