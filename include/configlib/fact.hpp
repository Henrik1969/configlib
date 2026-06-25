// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <vector>

#include <configlib/key.hpp>
#include <configlib/source.hpp>
#include <configlib/value.hpp>

namespace configlib {

enum class FactRole {
    Application,
    MetaConfig,
    Policy
};

struct Fact {
    KeyPath key;
    Value value;
    Source source;
    int precedence{0};
    FactRole role{FactRole::Application};
    std::size_t insertion_order{0};
};

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

[[nodiscard]] const char* fact_role_name(FactRole role);

} // namespace configlib
