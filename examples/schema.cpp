// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <iostream>

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("xyz.logging.level"), Value("info"));
    facts.add_cli(KeyPath("xyz.logging.level"), Value("debug"), "--log-level");
    facts.add_default(KeyPath("xyz.server.port"), Value(8080));
    facts.add_default(KeyPath("xyz.ai.remote_allowed"), Value(false));

    PolicySet policy;
    policy.require(KeyPath("xyz.logging.level"), ValueType::String);
    policy.require(KeyPath("xyz.server.port"), ValueType::Int);
    policy.require(KeyPath("xyz.ai.remote_allowed"), ValueType::Bool);

    auto resolved = resolve(facts, policy);
    if (!resolved.ok()) {
        std::cerr << resolved.diagnostics().format();
        return 1;
    }

    ConfigSchema schema;
    schema.path(KeyPath("xyz.logging.level"))
        .string()
        .required()
        .documented_default(Value("info"))
        .allowed({"trace", "debug", "info", "warn", "error"});

    schema.path(KeyPath("xyz.server.port"))
        .integer()
        .required()
        .documented_default(Value(8080))
        .range(static_cast<std::int64_t>(1), static_cast<std::int64_t>(65535));

    schema.path(KeyPath("xyz.ai.remote_allowed"))
        .boolean()
        .required()
        .documented_default(Value(false));

    auto checked = schema.validate(resolved.config());
    if (!checked.ok()) {
        std::cerr << checked.diagnostics().format();
        return 1;
    }

    ConfigStore store = ConfigStore::from_result(std::move(resolved), policy);
    auto logging = store.view(KeyPath("xyz.logging"));
    std::cout << "logging.level = " << logging.get_string_or(KeyPath("level"), "info") << '\n';
    std::cout << "schema validation ok\n";
    return 0;
}
