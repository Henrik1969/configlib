// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    facts.add_file(KeyPath("logging.level"), Value("warn"), "./app.conf");
    facts.add_cli(KeyPath("server.port"), Value(8080), "--port");
    facts.add_default(KeyPath("auth.token"), Value("secret-token"));

    PolicySet policies;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    policies.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});
    policies.require(KeyPath("server.port"), ValueType::Int);
    policies.int_range(KeyPath("server.port"), 1, 65535);
    policies.optional(KeyPath("auth.token"), ValueType::String);

    AccessPolicy access;
    access.runtime_mutable(KeyPath("server.port"), false)
          .secret(KeyPath("auth.token"));

    auto result = resolve(facts, policies);
    REQUIRE(result.ok());
    auto store = ConfigStore::from_result(std::move(result), policies, access);

    REQUIRE(store.get_string(KeyPath("logging.level")) == "warn");
    REQUIRE(store.get_integer_or(KeyPath("server.port"), 0) == 8080);

    auto tx = store.begin_transaction();
    tx.set(KeyPath("logging.level"), Value("trace"));
    REQUIRE(tx.commit());
    REQUIRE(store.get_string(KeyPath("logging.level")) == "trace");
    REQUIRE(store.has_runtime_change(KeyPath("logging.level")));

    const auto changed = store.export_config(ExportMode::ChangedOnly);
    REQUIRE(changed.find("logging.level = trace") != std::string::npos);
    REQUIRE(changed.find("server.port") == std::string::npos);

    auto bad_value = store.begin_transaction();
    bad_value.set(KeyPath("logging.level"), Value("nonsense"));
    REQUIRE(!bad_value.commit());
    REQUIRE(bad_value.diagnostics().has_errors());
    REQUIRE(store.get_string(KeyPath("logging.level")) == "trace");

    auto denied = store.begin_transaction();
    denied.set(KeyPath("server.port"), Value(9000));
    REQUIRE(!denied.commit());
    REQUIRE(store.get_integer_or(KeyPath("server.port"), 0) == 8080);

    auto reset_base = store.begin_transaction();
    reset_base.reset_to_base(KeyPath("logging.level"));
    REQUIRE(reset_base.commit());
    REQUIRE(store.get_string(KeyPath("logging.level")) == "warn");
    REQUIRE(!store.has_runtime_change(KeyPath("logging.level")));

    auto reset_default = store.begin_transaction();
    reset_default.reset_to_default(KeyPath("logging.level"));
    REQUIRE(reset_default.commit());
    REQUIRE(store.get_string(KeyPath("logging.level")) == "info");

    auto erased = store.begin_transaction();
    erased.erase(KeyPath("logging.level"));
    REQUIRE(erased.commit());
    REQUIRE(!store.contains(KeyPath("logging.level")));

    const auto effective = store.export_config();
    REQUIRE(effective.find("auth.token") == std::string::npos);


    {
        FactSet export_facts;
        export_facts.add_default(KeyPath("public.value"), Value("hello"));
        export_facts.add_default(KeyPath("private.value"), Value("must-not-leak"));
        PolicySet export_policies;
        auto export_result = resolve(export_facts, export_policies);
        REQUIRE(export_result.ok());
        AccessPolicy export_access;
        export_access.exportable(KeyPath("private.value"), false);
        auto export_store = ConfigStore::from_result(std::move(export_result), export_policies, export_access);
        const auto redacted = export_store.export_config(ExportMode::EffectiveRedacted);
        REQUIRE(redacted.find("public.value = hello") != std::string::npos);
        REQUIRE(redacted.find("private.value") == std::string::npos);
        REQUIRE(redacted.find("must-not-leak") == std::string::npos);
    }

    return EXIT_SUCCESS;
}
