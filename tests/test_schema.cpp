// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <iostream>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

int main() {
    using namespace configlib;

    {
        FactSet facts;
        facts.add_default(KeyPath("xyz.logging.level"), Value("debug"));
        facts.add_default(KeyPath("xyz.server.port"), Value(8080));
        facts.add_default(KeyPath("xyz.ai.remote_allowed"), Value(false));

        PolicySet policy;
        policy.require(KeyPath("xyz.logging.level"), ValueType::String);
        policy.require(KeyPath("xyz.server.port"), ValueType::Int);
        policy.require(KeyPath("xyz.ai.remote_allowed"), ValueType::Bool);

        auto resolved = resolve(facts, policy);
        REQUIRE(resolved.ok());

        ConfigSchema schema;
        schema.path(KeyPath("xyz.logging.level"))
            .string()
            .required()
            .allowed({"trace", "debug", "info", "warn", "error"});
        schema.path(KeyPath("xyz.server.port"))
            .integer()
            .required()
            .range(static_cast<std::int64_t>(1), static_cast<std::int64_t>(65535));
        schema.path(KeyPath("xyz.ai.remote_allowed"))
            .boolean()
            .required();

        auto checked = schema.validate(resolved.config());
        REQUIRE(checked.ok());
    }

    {
        ResolvedConfig config;
        ConfigSchema schema;
        schema.path(KeyPath("missing.required")).string().required();
        auto checked = schema.validate(config);
        REQUIRE(!checked.ok());
        REQUIRE(!checked.diagnostics().items().empty());
        REQUIRE(checked.diagnostics().items().front().code == "SCHEMA_REQUIRED_MISSING");
    }

    {
        ResolvedConfig config;
        config.set(ResolvedEntry{KeyPath("server.port"), Value("bad"), Source{}, {}});
        ConfigSchema schema;
        schema.path(KeyPath("server.port")).integer().required();
        auto checked = schema.validate(config);
        REQUIRE(!checked.ok());
        REQUIRE(checked.diagnostics().items().front().code == "SCHEMA_TYPE_MISMATCH");
    }

    {
        ResolvedConfig config;
        config.set(ResolvedEntry{KeyPath("logging.level"), Value("verbose-ish"), Source{}, {}});
        ConfigSchema schema;
        schema.path(KeyPath("logging.level"))
            .string()
            .required()
            .allowed({"debug", "info", "warn", "error"});
        auto checked = schema.validate(config);
        REQUIRE(!checked.ok());
        REQUIRE(checked.diagnostics().items().front().code == "SCHEMA_STRING_NOT_ALLOWED");
    }

    {
        ResolvedConfig config;
        config.set(ResolvedEntry{KeyPath("server.port"), Value(70000), Source{}, {}});
        ConfigSchema schema;
        schema.path(KeyPath("server.port"))
            .integer()
            .required()
            .range(static_cast<std::int64_t>(1), static_cast<std::int64_t>(65535));
        auto checked = schema.validate(config);
        REQUIRE(!checked.ok());
        REQUIRE(checked.diagnostics().items().front().code == "SCHEMA_INT_ABOVE_MAX");
    }

    {
        FactSet facts;
        facts.add_default(KeyPath("xyz.logging.level"), Value("info"));
        PolicySet policy;
        policy.require(KeyPath("xyz.logging.level"), ValueType::String);
        auto resolved = resolve(facts, policy);
        REQUIRE(resolved.ok());

        ConfigStore store = ConfigStore::from_result(std::move(resolved), policy);
        auto logging = store.view(KeyPath("xyz.logging"));
        ConfigSchema local_schema;
        local_schema.path(KeyPath("level"))
            .string()
            .required()
            .allowed({"debug", "info"});
        auto checked = local_schema.validate(logging);
        REQUIRE(checked.ok());
    }

    return EXIT_SUCCESS;
}
